#include "../include/qs1r_server.hpp"
#include "../include/qs_audio.hpp"
#include "../include/qs_bitstream.hpp"
#include "../include/qs_bytearray.hpp"
#include "../include/qs_dac_writer.hpp"
#include "../include/qs_datareader.hpp"
#include "../include/qs_datastreamclass.hpp"
#include "../include/qs_debugloggerclass.hpp"
#include "../include/qs_dsp_proc.hpp"
#include "../include/qs_fft.hpp"
#include "../include/qs_file.hpp"
#include "../include/qs_filter.hpp"
#include "../include/qs_firmware.hpp"
#include "../include/qs_globals.hpp"
#include "../include/qs_io_libusb.hpp"
#include "../include/qs_io_thread.hpp"
#include "../include/qs_listclass.hpp"
#include "../include/qs_memory.hpp"
#include "../include/qs_signalops.hpp"
#include "../include/qs_sleep.hpp"
#include "../include/qs_state.hpp"
#include "../include/qs_stringclass.hpp"
#include "../include/qs_uuid.hpp"
#include "qs1r_server.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <sstream>

QS1RServer::QS1RServer()
    : p_rta(std::make_unique<QsAudio>()), p_qsState(std::make_unique<QsState>()),
      p_io_thread(std::make_unique<QsIoThread>()), m_is_fpga_loaded(false), m_is_io_setup(false),
      m_is_factory_init_enabled(false), m_is_was_factory_init(false), m_gui_rx1_is_connected(false),
      m_gui_rx2_is_connected(false), m_driver_type("None"), m_local_rx_num_selector(1), m_freq_offset_rx1(0.0),
      m_freq_offset_rx2(0.0), m_proc_samplerate(50000.0), m_post_proc_samplerate(50000.0), m_step_size(500.0),
      m_status_message_backing_register(0), m_prev_vol_val(0) {

    QsGlobal::g_server = this;

    p_qsState->init();
    m_is_hardware_init = false;
    QsGlobal::g_is_hardware_init = false;

    initQsMemory();
    sleep.msleep(500);
    initialize();
}

QS1RServer::~QS1RServer() { QsGlobal::g_server = nullptr; }

void QS1RServer::shutdown() {

    if (m_is_io_running) {
        stopIo();
    }

    if (p_io_thread->isRunning()) {
        _debug() << "stopping io thread...";
        p_io_thread->stop();
        p_io_thread->wait(std::chrono::milliseconds(10000));
    }

    _debug() << "Close Event";

    p_qsState->setStartupSampleRate(QsGlobal::g_memory->getDataProcRate());
    p_qsState->setStartupFrequency(QsGlobal::g_memory->getRxLOFrequency());
    p_qsState->setStartupFilterLow(QsGlobal::g_memory->getFilterLo());
    p_qsState->setStartupFilterHigh(QsGlobal::g_memory->getFilterHi());
    p_qsState->setStartupMode(QsGlobal::g_memory->getDemodMode());
    p_qsState->setStartupVolume(QsGlobal::g_memory->getVolume());
    p_qsState->setStartupAGCDecaySpeed(QsGlobal::g_memory->getAgcDecaySpeed());
    p_qsState->setStartupAGCThreshold(QsGlobal::g_memory->getAgcThreshold());
    p_qsState->setPGA(QsGlobal::g_memory->getAdcPgaOn());
    p_qsState->setRAND(QsGlobal::g_memory->getAdcRandomOn());
    p_qsState->setDITH(QsGlobal::g_memory->getAdcDitherOn());

    QsGlobal::g_io->close();
}

// -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
// ************************************************************
// INIT CODE
// ************************************************************
// ------------------------------------------------------------

void QS1RServer::initialize() {
    error_flag = false;
    initSupportedSampleRatesList();
    showStartupMessage();
    initSMeterCorrectionMap();
    initRingBuffers();
    initThreads();
    if (initQS1RHardware() != 0) {
        shutdown();
    }
    sleep.sleep(2);
    updateFPGARegisters();
    setFpgaForSampleRate(50000);
    setDacOutputDisable(false);
    _debug() << "Qs1r server initialization complete.";
}

void QS1RServer::initSupportedSampleRatesList() {
    _debug() << "initializing sample rate list...";
    m_supported_samplerates.clear();
    m_supported_samplerates.append(String("25000").toStdString());
    m_supported_samplerates.append(String("50000").toStdString());
    m_supported_samplerates.append(String("125000").toStdString());
    m_supported_samplerates.append(String("250000").toStdString());
    m_supported_samplerates.append(String("500000").toStdString());
    m_supported_samplerates.append(String("625000").toStdString());
    m_supported_samplerates.append(String("1250000").toStdString());
    m_supported_samplerates.append(String("1562500").toStdString());
    m_supported_samplerates.append(String("2500000").toStdString());
}

int QS1RServer::initRingBuffers() {
    _debug() << "initializing ring buffers...";
    QsGlobal::g_cpx_readin_ring = std::make_unique<QsCircularBuffer<std::complex<float>>>();
    QsGlobal::g_cpx_readin_ring->init(2048);
    QsGlobal::g_cpx_sd_ring = std::make_unique<QsCircularBuffer<std::complex<float>>>();
    QsGlobal::g_cpx_sd_ring->init(2048);
    QsGlobal::g_float_rt_ring = std::make_unique<QsCircularBuffer<float>>();
    QsGlobal::g_float_rt_ring->init(2048);
    QsGlobal::g_float_dac_ring = std::make_unique<QsCircularBuffer<float>>();
    QsGlobal::g_float_dac_ring->init(2048);
    return 0;
}

int QS1RServer::initThreads() {
    _debug() << "initializing threads...";
    QsGlobal::g_data_reader->init();
    QsGlobal::g_dsp_proc->init();
    QsGlobal::g_dac_writer->init();
    return 0;
}

void QS1RServer::clearAllBuffers() {
    _debug() << "clearing ring buffers...";
    QsGlobal::g_cpx_readin_ring->empty();
    QsGlobal::g_cpx_sd_ring->empty();
    QsGlobal::g_float_dac_ring->empty();
    QsGlobal::g_float_rt_ring->empty();
}

// ------------------------------------------------------------
// Initialize QsAudio here
//
// Note: make sure __WINDOWS_DS__ and __LITTLE_ENDIAN__
// are defined or QsAudio will not work on Windows.
// ------------------------------------------------------------
void QS1RServer::initQsAudio(double rate) {
    // see if we have an audio output device
    _debug() << "initializing QsAudio...";
    if (m_is_io_running) {
        stopIo();
    }

    p_rta->stopStream();

    int frames = QsGlobal::g_memory->getRtAudioFrames();
    int out_dev_id = p_qsState->rtAudioOutDevId();
    int in_dev_id = p_qsState->rtAudioInDevId();

    bool ok = false;

    p_rta->initAudio(frames, rate, in_dev_id, out_dev_id, ok);

    if (!ok) {
        setStatusText("Soundcard output init error.");
    }
}

// ------------------------------------------------------------
// Initializes Rx Persistance with qsStateSettings
// ------------------------------------------------------------
void QS1RServer::initQsMemory() {
    _debug() << "initializing QsMemory...";
    if (QsGlobal::g_memory == nullptr) {
        std::cerr << "You need to make an instance of QsMemory first!";
    }
    QsGlobal::g_memory->setRxLOFrequency(p_qsState->startupFrequency());
    QsGlobal::g_memory->setFilterLo(p_qsState->startupFilterLow());
    QsGlobal::g_memory->setFilterHi(p_qsState->startupFilterHigh());
    QsGlobal::g_memory->setDemodMode(p_qsState->startupMode());
    QsGlobal::g_memory->setAgcDecaySpeed(p_qsState->startupAGCDecaySpeed());
    QsGlobal::g_memory->setAgcThreshold(p_qsState->startupAGCThreshold());
    QsGlobal::g_memory->setAdcPgaOn(p_qsState->pga());
    QsGlobal::g_memory->setAdcRandomOn(p_qsState->rand());
    QsGlobal::g_memory->setAdcDitherOn(p_qsState->dith());
    QsGlobal::g_memory->setDataProcRate(p_qsState->startupSampleRate());
    QsGlobal::g_memory->setSMeterCorrection(p_qsState->smeterCorrection());
    QsGlobal::g_memory->setEncodeFreqCorrect(p_qsState->clockCorrection());
    QsGlobal::g_memory->setRtAudioFrames(p_qsState->rtAudioFrameSize());
    QsGlobal::g_memory->setDataProcRate(p_qsState->startupSampleRate());
    QsGlobal::g_memory->setReadBlockSize(p_qsState->blockSize());
    QsGlobal::g_memory->setResamplerQuality(p_qsState->rsQual());
    QsGlobal::g_memory->setEncodeFreqCorrect(p_qsState->clockCorrection());
    QsGlobal::g_memory->setEncodeClockFrequency(p_qsState->encodeClockFrequency());
    QsGlobal::g_memory->setTxBlockSize(p_qsState->txBlockSize());
    m_prev_vol_val = QsGlobal::g_memory->getVolume();
}

// ------------------------------------------------------------
// Initialize the QS1R Hardware
// ------------------------------------------------------------
int QS1RServer::initQS1RHardware() {
    _debug() << "==========================";
    _debug() << "initializing hardware...";
    _debug() << "==========================";
    unsigned int index = 0;
    m_driver_type = "None";
    int ret = -1;

    if (m_is_hardware_init) {
        QsGlobal::g_io->close();
    }

    m_is_hardware_init = false;

    _debug() << "Trying the libusb driver with index [" << index << "]... wait...";

    ret = QsGlobal::g_io->findQsDevice(QS1R_VID, QS1R_PID, index);

    if (ret != 0) {
        _debug() << "Could not find any QS1R devices!";
        return -1;
    }

    ret = QsGlobal::g_io->open();

    if (ret == 0) {
        int fpga_id = 0;
        int fw_id = 0;
        _debug() << "Open success!";
        _debug() << "FW S/N: " << (fw_id = QsGlobal::g_io->readFwSn());

        if (fw_id != ID_FWWR) {
            _debug() << "Attempting to load firmware...";
            int result = QsGlobal::g_io->loadFirmware(firmware_hex);
            if (result == 0) {
                _debug() << "Firmware load success!";
                QsGlobal::g_io->close();
                sleep.msleep(5000);
                ret = QsGlobal::g_io->findQsDevice(QS1R_VID, QS1R_PID, index);
                if (ret != 0) {
                    _debug() << "Could not find any QS1R devices!";
                    return -1;
                } else {
                    ret = QsGlobal::g_io->open();
                    if (ret != 0) {
                        _debug() << "Open Device Error: " << libusb_error_name(ret);
                        return -1;
                    }
                }
            }
        } else {
            _debug() << "Firmware is already loaded!";
        }

        _debug() << "FW S/N: " << std::dec << (fw_id = QsGlobal::g_io->readFwSn());

        _debug() << "FPGA ID returned: " << std::hex << (fpga_id = QsGlobal::g_io->readMultibusInt(MB_VERSION_REG))
                 << std::dec;

        if (fpga_id != ID_1RXWR) {
            _debug() << "Attempting to load FPGA bitstream...";
            int result = QsGlobal::g_io->loadFpgaFromBitstream(fpga_bitstream, fpga_bitstream_size);
            if (result == 0) {
                _debug() << "FPGA load success!";
                sleep.msleep(2000);
            }
        } else {
            _debug() << "FPGA already loaded!";
        }

        _debug() << "FPGA ID returned: " << std::hex << (fpga_id = QsGlobal::g_io->readMultibusInt(MB_VERSION_REG))
                 << std::dec;

    } else {
        std::cerr << "Error opening device!";
        return -1;
    }

    _debug() << "==========================";
    _debug() << "QS1R index [" << index << "] hardware was successfully initialized!";
    _debug() << "==========================";
    m_is_hardware_init = true;
    QsGlobal::g_is_hardware_init = true;
    return 0;
}

// ------------------------------------------------------------
// CHECK FOR THE PRESENCE OF A QS1E BOARD
// BY READING THE PDAC EEPROM
// ------------------------------------------------------------
bool QS1RServer::checkForQS1E() {
    unsigned char buf[5] = {0x0, 0x0, 0x0, 0x0, 0x0};
    if (QsGlobal::g_io->readI2C(QS1E_PDAC_ADDR, buf, 0x5) == 5) {
        return true;
    } else {
        return false;
    }
}

// ------------------------------------------------------------
// CHECK IF QS1R HAS BEEN DETECTED AND INITIALIZED
// ------------------------------------------------------------
bool QS1RServer::isHardwareInit() { return m_is_hardware_init; }

// ------------------------------------------------------------
// SET THE POWER DAC LEVEL ON QS1E BOARD
// ------------------------------------------------------------
void QS1RServer::setTxPdacLevel(unsigned int value) {
    value = (unsigned int)std::round((((double)value) / 100.0) * 4095.0);
    unsigned char buf[2] = {0x0, 0x0};
    buf[0] = (value >> 8) & 0x0f;
    buf[1] = value & 0xff;

    if (QsGlobal::g_io->writeI2C(QS1E_PDAC_ADDR, buf, 0x2) == -1) {
        _debug() << "An error occured writing to the QS1E PDAC register.";
    } else {
        _debug() << "Set QS1E PDAC to " << String::number(value, 16);
    }
}

// ------------------------------------------------------------
// INITIALIZE THE S METER CORRECTION MAP
// ------------------------------------------------------------
void QS1RServer::initSMeterCorrectionMap() {
    _debug() << "initializing s-meter correction map...";
    SMETERCORRECTMAP[25000] = -17.7;
    SMETERCORRECTMAP[50000] = -17.6;
    SMETERCORRECTMAP[125000] = -16.3;
    SMETERCORRECTMAP[250000] = -16.3;
    SMETERCORRECTMAP[500000] = -16.3;
    SMETERCORRECTMAP[625000] = -21.0;
    SMETERCORRECTMAP[1250000] = -21.0;
    SMETERCORRECTMAP[1562500] = -21.7;
    SMETERCORRECTMAP[2500000] = -21.0;
    SMETERCORRECT = SMETERCORRECTMAP[50000];
}

// ------------------------------------------------------------
// LOADS THE INITIAL FPGA REGISTER SETTINGS
// ------------------------------------------------------------
void QS1RServer::updateFPGARegisters() {
    _debug() << "updating fpga registers...";
    clearFpgaControlRegisters();

    // do a master reset of DDC in FPGA
    setDdcMasterReset(true);
    setDdcMasterReset(false);

    // Set initial Ext Mute Enable Mode

    setDacExtMuteEnable(p_qsState->extMuteEnable());

    // Set initial Wideband bypass mode

    setWideBandBypass(false);

    // Set initial PGA mode

    setPgaMode(p_qsState->pga());

    // Set initial Rand mode

    setRandMode(p_qsState->rand());

    // Set initial Dither mode

    setDitherMode(p_qsState->dith());

    // Set initial startup frequency

    setRxFrequency(QsGlobal::g_memory->getRxLOFrequency(), 1);
}

// ------------------------------------------------------------
// LOADS THE SPECIFIED FPGA CONFIGURATION FILE
// ------------------------------------------------------------
void QS1RServer::loadFPGAFile(String filename) {
    m_is_fpga_loaded = false;
    if (QsGlobal::g_io->loadFpga(filename.toStdString()) == -1) {
        setStatusText("Error: Could not load QS1R FPGA File");
        return;
    }
    // do a master reset of DDC in FPGA
    setDdcMasterReset(true);
    setDdcMasterReset(false);
    m_is_fpga_loaded = true;
}

// ------------------------------------------------------------
// Manages the status text
// ------------------------------------------------------------
void QS1RServer::setStatusText(String text) { _debug() << text; }

// ------------------------------------------------------------
// Displays the server startup message
// ------------------------------------------------------------
void QS1RServer::showStartupMessage() { _debug() << "Server is starting..."; }
// ------------------------------------------------------------
// Displays the server startup message with ready
// ------------------------------------------------------------
void QS1RServer::showStartupMessageWithReady() { _debug() << "Server is starting..."; }

// ------------------------------------------------------------
// Quits the server application
// ------------------------------------------------------------
void QS1RServer::quit() { this->shutdown(); }

// ------------------------------------------------------------
// Returns the supported sample rates
// ------------------------------------------------------------
StringList QS1RServer::getSupportedSampleRates() { return m_supported_samplerates; }

// ------------------------------------------------------------
// Calculates the FPGA register settings for a given
// sample rate and writes them to the FPGA.
// Returns false if sample rate is not supported.
// ------------------------------------------------------------
bool QS1RServer::setFpgaForSampleRate(double samplerate) {
    bool was_io_running = m_is_io_running;

    if (was_io_running)
        stopIo();

#define SR_OUT0 48000.0
#define SR_OUT1 48000.0

    switch ((int)samplerate) {
    case 2500000: // BW:2000000
        m_proc_samplerate = samplerate;
        QsGlobal::g_memory->setResamplerRate(SR_OUT0);
        break;
    case 1562500: // BW: 1250000
        m_proc_samplerate = samplerate;
        QsGlobal::g_memory->setResamplerRate(SR_OUT0);
        break;
    case 1250000: // BW: 1000000
        m_proc_samplerate = samplerate;
        QsGlobal::g_memory->setResamplerRate(SR_OUT0);
        break;
    case 625000: // BW: 500000
        m_proc_samplerate = samplerate;
        QsGlobal::g_memory->setResamplerRate(SR_OUT0);
        break;
    case 500000: // BW: 400000
        m_proc_samplerate = samplerate;
        QsGlobal::g_memory->setResamplerRate(SR_OUT0);
        break;
    case 250000: // BW: 200000
        m_proc_samplerate = samplerate;
        m_post_proc_samplerate = 31250.0;
        QsGlobal::g_memory->setResamplerRate(SR_OUT0);
        break;
    case 125000: // BW 100000
        m_proc_samplerate = samplerate;
        QsGlobal::g_memory->setResamplerRate(SR_OUT0);
        break;
    case 50000: // BW: 40000
        m_proc_samplerate = samplerate;
        QsGlobal::g_memory->setResamplerRate(SR_OUT0);
        break;
    case 25000: // BW: 20000
        m_proc_samplerate = samplerate;
        QsGlobal::g_memory->setResamplerRate(SR_OUT0);
        break;
    default: // not supported
        m_proc_samplerate = samplerate;
        QsGlobal::g_memory->setResamplerRate(SR_OUT0);
        return false;
    }

    m_post_proc_samplerate = estimateDownConvertorRate(m_proc_samplerate, 20000.0);

    QsGlobal::g_memory->setDataProcRate(m_proc_samplerate);
    QsGlobal::g_memory->setDataPostProcRate(m_post_proc_samplerate);

    SMETERCORRECT = SMETERCORRECTMAP[(int)m_proc_samplerate];

    // setup DDC for proper samplerate
    setDDCSamplerate((int)m_proc_samplerate);

    // set the dac clock select
    if (QsGlobal::g_memory->getResamplerRate() == 24000.0) {
        setDacClock24kSelect(true);
    } else {
        setDacClock24kSelect(false);
    }

    if (was_io_running) {
        setupIo();
        startIo();
    }

    return true;
}

double QS1RServer::estimateDownConvertorRate(double in_rate, double bandwidth) {
    double f = in_rate;
    double hb51tapbw = (.5 - .333);
    double min_output_rate = (7900.0 * 2.0);
    while ((f > (bandwidth / hb51tapbw)) && (f > min_output_rate)) {
        f /= 2.0;
    }
    return f;
}

int QS1RServer::frequencyToPhaseIncrement(double freq) {
    double clk_correction = QsGlobal::g_memory->getEncodeFreqCorrect();
    double encode_clk_freq = QsGlobal::g_memory->getEncodeClockFrequency();
    return std::round((freq) / (encode_clk_freq + clk_correction) * 4294967296.0);
}

String QS1RServer::getModeString(QSDEMODMODE mode) {
    String mode_str;

    switch (mode) {
    case dmAM:
        mode_str = "AM";
        break;
    case dmSAM:
        mode_str = "SAM";
        break;
    case dmFMN:
        mode_str = "FMN";
        break;
    case dmFMW:
        mode_str = "FMW";
        break;
    case dmDSB:
        mode_str = "DSB";
        break;
    case dmLSB:
        mode_str = "LSB";
        break;
    case dmUSB:
        mode_str = "USB";
        break;
    case dmCW:
        mode_str = "CW";
        break;
    case dmDIG:
        mode_str = "DIG";
        break;
    default:
        mode_str = "???";
        break;
    }
    return mode_str;
}

QSDEMODMODE QS1RServer::modeStringToMode(String smode) {
    if (smode.compare("AM") == 0) {
        return dmAM;
    } else if (smode.compare("SAM") == 0) {
        return dmSAM;
    } else if (smode.compare("FMN") == 0) {
        return dmFMN;
    } else if (smode.compare("FMW") == 0) {
        return dmFMW;
    } else if (smode.compare("DSB") == 0) {
        return dmDSB;
    } else if (smode.compare("LSB") == 0) {
        return dmLSB;
    } else if (smode.compare("USB") == 0) {
        return dmUSB;
    } else if (smode.compare("CW") == 0) {
        return dmCW;
    } else if (smode.compare("DIG") == 0) {
        return dmDIG;
    } else {
        return dmAM;
    }
}

// ------------------------------------------------------------
// Sets up the DSP chain
// ------------------------------------------------------------
void QS1RServer::setupIo() {
    int rx_num = 1;

    m_is_io_setup = false;
    m_is_io_running = false;

    QsGlobal::g_data_reader->init();

    bool dac_bypass = false;
    dac_bypass = QsGlobal::g_memory->getDacBypass();

    QsGlobal::g_dsp_proc->init(rx_num);

    QsGlobal::g_dac_writer->init();

#ifndef __DAC_OUT__
    setDacOutputDisable(true);
#endif

    m_is_io_setup = true;

    _debug() << "-setupIo successful-";
}

// ------------------------------------------------------------
// Starts the DSP processing
// ------------------------------------------------------------
void QS1RServer::startIo(bool iswav) {
    if (m_is_io_running) {
        stopIo();
    }

    if (!m_is_io_setup) {
        setupIo();
    }

    if (!m_is_io_setup) {
        setStatusText("Error: Error on io setup!");
        return;
    }

    // do a master reset of DDC in FPGA
    setDdcMasterReset(true);
    setDdcMasterReset(false);

    // start the dsp processor thread
    if (!QsGlobal::g_dsp_proc->isRunning())
        QsGlobal::g_dsp_proc->start();

    if (!QsGlobal::g_data_reader->isRunning())
        QsGlobal::g_data_reader->start();

#ifdef __DAC_OUT__
    if (!QsGlobal::g_dac_writer->isRunning())
        QsGlobal::g_dac_writer->start();
#endif
#ifdef __SOUND_OUT__
    initQsAudio(QsGlobal::g_memory->getResamplerRate());
    p_rta->startStream();
    QsGlobal::g_float_rt_ring->empty();
#endif
    m_is_io_running = true;

    setRxFrequency(QsGlobal::g_memory->getRxLOFrequency(), 1, true);
}

// ------------------------------------------------------------
// Stops the DSP processing
// ------------------------------------------------------------
void QS1RServer::stopIo() {
    if (!m_is_io_running)
        return;

    // The order of stopping threads below
    // is important!

    _debug() << "stopping tx thread...";

#ifdef __SOUND_OUT__
    _debug() << "stopping rt audio...";
    p_rta->stopStream();
#endif

#ifdef __DAC_OUT__
    _debug() << "stopping dac writer...";
    if (QsGlobal::g_dac_writer->isRunning()) {
        QsGlobal::g_dac_writer->stop();
    }
#endif

    _debug() << "stopping dsp processor...";
    if (QsGlobal::g_dsp_proc->isRunning()) {
        QsGlobal::g_dsp_proc->stop();
    }

    _debug() << "stopping data reader...";
    if (QsGlobal::g_data_reader->isRunning()) {
        QsGlobal::g_data_reader->stop();
    }

    QsGlobal::g_data_reader->clearBuffers();
    QsGlobal::g_dsp_proc->clearBuffers();

    m_is_io_running = false;
}

// ------------------------------------------------------------
//
// ************Radio Hardware Control Section******************
//
// ------------------------------------------------------------

// ------------------------------------------------------------
// Updates clock correction value
// ------------------------------------------------------------
void QS1RServer::updateClockCorrection(double value) { QsGlobal::g_memory->setEncodeFreqCorrect(value); }

// ------------------------------------------------------------
// Returns the display frequency offset
// ------------------------------------------------------------
void QS1RServer::getDisplayFreqOffset(double &value, int rx_num) {
    value = QsGlobal::g_memory->getDisplayFreqOffset(rx_num - 1);
}

// ------------------------------------------------------------
// Sets the RX Vfo frequency
// ------------------------------------------------------------
void QS1RServer::setRxFrequency(double value, int rx_num, bool force) {
    if (value < 0.0)
        value = 0.0;

    double clk_correction = QsGlobal::g_memory->getEncodeFreqCorrect();
    double encode_clk_freq = QsGlobal::g_memory->getEncodeClockFrequency();

    m_freq_offset_rx1 = QsGlobal::g_memory->getDisplayFreqOffset();

    if (rx_num == 1) {
        double rx1_frequency = QsGlobal::g_memory->getRxLOFrequency();
        if (value != rx1_frequency || force == true) {
            rx1_frequency = value;
            QsGlobal::g_memory->setRxLOFrequency(value);

            int val =
                std::round((rx1_frequency + m_freq_offset_rx1) / (encode_clk_freq + clk_correction) * 4294967296.0);
            if (m_is_hardware_init) {
                QsGlobal::g_io->writeMultibusInt(MB_FREQRX0_REG, val);
            }
        }
    }
}

// ------------------------------------------------------------
// Returns the RX Vfo frequency
// ------------------------------------------------------------
void QS1RServer::getRxFrequency(double &value, int rx_num) { value = QsGlobal::g_memory->getRxLOFrequency(rx_num - 1); }

double QS1RServer::getRxFrequency() { return QsGlobal::g_memory->getRxLOFrequency(0); }

// ------------------------------------------------------------
// Sets the TX Vfo frequency
// ------------------------------------------------------------
void QS1RServer::setTxFrequency(double value, bool force) {
    if (value < 0.0)
        value = 0.0;

    double clk_correction = QsGlobal::g_memory->getEncodeFreqCorrect();
    double encode_clk_freq = QsGlobal::g_memory->getEncodeClockFrequency();

    double tx_frequency = QsGlobal::g_memory->getTxLOFrequency();
    if (value != tx_frequency || force == true) {
        tx_frequency = value;
        QsGlobal::g_memory->setTxLOFrequency(value);

        int val = std::round((tx_frequency + m_freq_offset_rx1) / (encode_clk_freq + clk_correction) * 4294967296.0);
        if (m_is_hardware_init) {
            QsGlobal::g_io->writeMultibusInt(MB_TX_FREQ, val);
        }
    }
}

// ------------------------------------------------------------
// Returns the TX Vfo frequency
// ------------------------------------------------------------
void QS1RServer::getTxFrequency(double &value) { value = QsGlobal::g_memory->getTxLOFrequency(); }

// ------------------------------------------------------------
// Sets the Vfo frequency
// ------------------------------------------------------------
void QS1RServer::setRxFrequency(double freq) { setRxFrequency(freq, 1, true); }

// ------------------------------------------------------------
// Returns a mode string
// ------------------------------------------------------------
String QS1RServer::getRxMode() { return getModeString(QsGlobal::g_memory->getDemodMode()); }

// ------------------------------------------------------------
// Sets the mode
// ------------------------------------------------------------
void QS1RServer::setRxMode(String mode) { QsGlobal::g_memory->setDemodMode(modeStringToMode(mode.toUpper())); }

// ------------------------------------------------------------
// Prints debug messages from script
// ------------------------------------------------------------
void QS1RServer::scriptDebugPrint(String msg) { _debug() << "from script: " + msg; }

int QS1RServer::startAllThreads() {
    _debug() << "Starting datareader thread...";
    QsGlobal::g_data_reader->start(); 
    _debug() << "Starting dsp processor thread...";
    QsGlobal::g_dsp_proc->start();
    _debug() << "Starting dac writer thread...";
    QsGlobal::g_dac_writer->start(); 
    _debug() << "Running for 10 seconds...";
    sleep.sleep(10); 
    _debug() << "Stopping dac writer thread...";
    QsGlobal::g_dac_writer->stop();
    _debug() << "Stopping dsp processor thread...";
    QsGlobal::g_dsp_proc->stop(); 
    _debug() << "Stopping datareader thread...";
    QsGlobal::g_data_reader->stop();
    return 0;    
}
// Testing
int QS1RServer::startDataReader() {
    if (QsGlobal::g_data_reader == nullptr) {
        QsGlobal::g_data_reader = std::make_unique<QsDataReader>();
    }
    if (QsGlobal::g_cpx_readin_ring == nullptr) {
        QsGlobal::g_cpx_readin_ring = std::make_unique<QsCircularBuffer<std::complex<float>>>();
    }
    _debug() << "Starting datareader thread...";
    QsGlobal::g_data_reader->init();
    QsGlobal::g_data_reader->start();
    _debug() << "Sleeping for 3 seconds...";
    sleep.sleep(3);
    _debug() << "Stopping datareader thread...";
    QsGlobal::g_data_reader->stop();
    return 0;
}

// Testing
int QS1RServer::startDACWriter() {
    if (QsGlobal::g_dac_writer == nullptr) {
        QsGlobal::g_dac_writer = std::make_unique<QsDacWriter>();
    }
    if (QsGlobal::g_float_dac_ring == nullptr) {
        QsGlobal::g_float_dac_ring = std::make_unique<QsCircularBuffer<float>>();
    }
    _debug() << "Starting dac writer thread...";
    QsGlobal::g_dac_writer->init();
    QsGlobal::g_dac_writer->start();
    _debug() << "Thread will run for 10 seconds...";
    sleep.sleep(10);
    _debug() << "Stopping dac writer thread...";
    QsGlobal::g_dac_writer->stop();
    return 0;
}

// Testing
int QS1RServer::startDSPProcessor() {
    if (QsGlobal::g_dsp_proc == nullptr) {
        QsGlobal::g_dsp_proc = std::make_unique<QsDspProcessor>();
    }
    _debug() << "Starting dsp processor thread...";
    QsGlobal::g_dsp_proc->init();
    QsGlobal::g_dsp_proc->start();
    _debug() << "Sleeping for 3 seconds...";
    sleep.sleep(3);
    _debug() << "Stopping dsp processor thread...";
    QsGlobal::g_dsp_proc->stop();
    return 0;
}

// ------------------------------------------------------------
// Sets the encode clock correction for all rx
// ------------------------------------------------------------
void QS1RServer::setFreqCorrection(double value) {
    QsGlobal::g_memory->setEncodeFreqCorrect(value);
    p_qsState->setClockCorrection(value);
    setRxFrequency(QsGlobal::g_memory->getRxLOFrequency(), 1, true);
}

void QS1RServer::getFreqCorrection(double &value) { value = QsGlobal::g_memory->getEncodeFreqCorrect(); }

// ------------------------------------------------------------
// Sets the s-meter correction factor
// ------------------------------------------------------------
void QS1RServer::setSMeterCorrection(double value) {
    QsGlobal::g_memory->setSMeterCorrection(value);
    p_qsState->setSMeterCorrection(value);
}

void QS1RServer::getSMeterCorrection(double &value) { value = QsGlobal::g_memory->getSMeterCorrection(); }

// ------------------------------------------------------------
// Clears the FPGA control registers
// ------------------------------------------------------------
void QS1RServer::clearFpgaControlRegisters() {
    if (!m_is_hardware_init) {
        setStatusText("Error: Please initialize QS1R Hardware first!");
        return;
    }
    QsGlobal::g_io->writeMultibusInt(MB_CONTRL0, 0);
    QsGlobal::g_io->writeMultibusInt(MB_CONTRL1, 0);
}

// ------------------------------------------------------------
// Sets/Clears the ADC PGA mode
// ------------------------------------------------------------
int QS1RServer::setPgaMode(bool on) {
    if (!m_is_hardware_init) {
        setStatusText("Error: Please initialize QS1R Hardware first!");
        return -1;
    }
    unsigned int result = QsGlobal::g_io->readMultibusInt(MB_CONTRL1);
    if (on) {
        result |= PGA;
    } else {
        result &= ~PGA;
    }

    int res = QsGlobal::g_io->writeMultibusInt(MB_CONTRL1, result);

    p_qsState->setPGA(on);
    return res;
}

bool QS1RServer::pgaMode() {
    unsigned int result = QsGlobal::g_io->readMultibusInt(MB_CONTRL1);
    if ((result & PGA) == PGA)
        return true;
    else
        return false;
}

// ------------------------------------------------------------
// Sets/Clears the ADC Random mode
// ------------------------------------------------------------
int QS1RServer::setRandMode(bool on) {
    if (!m_is_hardware_init) {
        setStatusText("Error: Please initialize QS1R Hardware first!");
        return -1;
    }
    unsigned int result = QsGlobal::g_io->readMultibusInt(MB_CONTRL1);
    if (on) {
        result |= RANDOM;
    } else {
        result &= ~RANDOM;
    }

    int res = QsGlobal::g_io->writeMultibusInt(MB_CONTRL1, result);

    p_qsState->setRAND(on);
    return res;
}

bool QS1RServer::randMode() {
    unsigned int result = QsGlobal::g_io->readMultibusInt(MB_CONTRL1);
    if ((result & RANDOM) == RANDOM)
        return true;
    else
        return false;
}

// ------------------------------------------------------------
// Sets/Clears the ADC Dither mode
// ------------------------------------------------------------
int QS1RServer::setDitherMode(bool on) {
    if (!m_is_hardware_init) {
        setStatusText("Error: Please initialize QS1R Hardware first!");
        return -1;
    }
    unsigned int result = QsGlobal::g_io->readMultibusInt(MB_CONTRL1);
    if (on) {
        result |= DITHER;
    } else {
        result &= ~DITHER;
    }

    int res = QsGlobal::g_io->writeMultibusInt(MB_CONTRL1, result);

    p_qsState->setDITH(on);

    return res;
}

bool QS1RServer::ditherMode() {
    unsigned int result = QsGlobal::g_io->readMultibusInt(MB_CONTRL1);
    if ((result & DITHER) == DITHER)
        return true;
    else
        return false;
}

// ------------------------------------------------------------
// Sets/Clears the DAC output disable bit
// ------------------------------------------------------------
void QS1RServer::setDacOutputDisable(bool on) {
    if (!m_is_hardware_init) {
        setStatusText("Error: Please initialize QS1R Hardware first!");
        return;
    }
    unsigned int result = QsGlobal::g_io->readMultibusInt(MB_CONTRL0);
    if (on) {
        result |= DAC_BYPASS;
    } else {
        result &= ~DAC_BYPASS;
    }
    QsGlobal::g_io->writeMultibusInt(MB_CONTRL0, result);
}

bool QS1RServer::getDacOutputDisable() {
    if (!m_is_hardware_init) {
        setStatusText("Error: Please initialize QS1R Hardware first!");
        return false;
    }
    unsigned int result = QsGlobal::g_io->readMultibusInt(MB_CONTRL0);
    return ((result & DAC_BYPASS) == DAC_BYPASS);
}

// ------------------------------------------------------------
// Sets/Clears the DAC external mute enable bit
// ------------------------------------------------------------
void QS1RServer::setDacExtMuteEnable(bool on) {
    if (!m_is_hardware_init) {
        setStatusText("Error: Please initialize QS1R Hardware first!");
        return;
    }
    unsigned int result = QsGlobal::g_io->readMultibusInt(MB_CONTRL0);
    if (on) {
        result |= DAC_EXT_MUTE_EN;
    } else {
        result &= ~DAC_EXT_MUTE_EN;
    }
    QsGlobal::g_io->writeMultibusInt(MB_CONTRL0, result);
}

// ------------------------------------------------------------
// Sets/Clears the DDC Master reset bit
// ------------------------------------------------------------
void QS1RServer::setDdcMasterReset(bool on) {
    if (!m_is_hardware_init) {
        setStatusText("Error: Please initialize QS1R Hardware first!");
        return;
    }
    unsigned int result = QsGlobal::g_io->readMultibusInt(MB_CONTRL0);
    if (on) {
        result |= MASTER_RESET;
    } else {
        result &= ~MASTER_RESET;
    }
    QsGlobal::g_io->writeMultibusInt(MB_CONTRL0, result);
}

// ------------------------------------------------------------
// Sets/Clears the DDC Wide Band Bypass Bit
// ------------------------------------------------------------
void QS1RServer::setWideBandBypass(bool on) {
    if (!m_is_hardware_init) {
        setStatusText("Error: Please initialize QS1R Hardware first!");
        return;
    }
    unsigned int result = QsGlobal::g_io->readMultibusInt(MB_CONTRL0);
    if (on) {
        result |= WB_BYPASS;
    } else {
        result &= ~WB_BYPASS;
    }
    QsGlobal::g_io->writeMultibusInt(MB_CONTRL0, result);
}

unsigned int QS1RServer::controlRegister0Value() { return QsGlobal::g_io->readMultibusInt(MB_CONTRL0); }

unsigned int QS1RServer::controlRegister1Value() { return QsGlobal::g_io->readMultibusInt(MB_CONTRL1); }

// ------------------------------------------------------------
// Sets the FPGA DDC Sample Rate
// ------------------------------------------------------------
void QS1RServer::setDDCSamplerate(int value) {
    if (!m_is_hardware_init) {
        setStatusText("Error: Please initialize QS1R Hardware first!");
        return;
    }
    QsGlobal::g_io->writeMultibusInt(MB_SAMPLERATE, value);
}

// ------------------------------------------------------------
// Sets the DAC Clock Rate 0 = 48k, 1 = 24k
// ------------------------------------------------------------
void QS1RServer::setDacClock24kSelect(bool value) {
    if (!m_is_hardware_init) {
        setStatusText("Error: Please initialize QS1R Hardware first!");
        return;
    }
    unsigned int result = QsGlobal::g_io->readMultibusInt(MB_CONTRL0);
    if (value) {
        result |= DAC_CLK_SEL;
    } else {
        result &= ~DAC_CLK_SEL;
    }
    QsGlobal::g_io->writeMultibusInt(MB_CONTRL0, result);
}

// ------------------------------------------------------------
// Sets the DAC Clock Rate 0 = 25k, 1 = 50k
// ------------------------------------------------------------
void QS1RServer::setDacClock50kSelect(bool value) {
    if (!m_is_hardware_init) {
        setStatusText("Error: Please initialize QS1R Hardware first!");
        return;
    }
    unsigned int result = QsGlobal::g_io->readMultibusInt(MB_CONTRL0);
    if (value) {
        result |= DAC_CLK_SEL;
    } else {
        result &= ~DAC_CLK_SEL;
    }
    QsGlobal::g_io->writeMultibusInt(MB_CONTRL0, result);
}

// ------------------------------------------------------------
// Sets the filters appropriately by mode
// ------------------------------------------------------------
void QS1RServer::setFilter(double width, int rx_num) {
    QSDEMODMODE mode = QsGlobal::g_memory->getDemodMode(rx_num - 1);

    double send_width_lo = 0.0;
    double send_width_hi = 0.0;

    switch (mode) {
    case dmAM:
        send_width_lo = -width;
        send_width_hi = width;
        break;
    case dmSAM:
        send_width_lo = -width;
        send_width_hi = width;
        break;
    case dmFMN:
        send_width_lo = -width;
        send_width_hi = width;
        break;
    case dmFMW:
        send_width_lo = -width;
        send_width_hi = width;
        break;
    case dmDSB:
        send_width_lo = -width;
        send_width_hi = width;
        break;
    case dmLSB:
        send_width_lo = -width;
        send_width_hi = -10.0;
        break;
    case dmUSB:
        send_width_lo = 10.0;
        send_width_hi = width;
        break;
    case dmCW:
        send_width_lo = 10.0;
        send_width_hi = width;
        break;
    case dmDIG:
        send_width_lo = 10.0;
        send_width_hi = width;
        break;
    default:
        break;
    }

    QsGlobal::g_memory->setFilterHi(send_width_hi);
    QsGlobal::g_memory->setFilterLo(send_width_lo);
}

// ------------------------------------------------------------
// Update Status Message when QS1R read fails
// ------------------------------------------------------------
void QS1RServer::qs1rReadFailure() { _debug() << "Read failure!"; }

// ------------------------------------------------------------
// Manufacture and Test Functions
// ------------------------------------------------------------

UUID QS1RServer::readQS1RUuid() {
    UUID uuid;
    ByteArray buffer(16, 0);

    if (QsGlobal::g_io->readEEPROM(QS1R_EEPROM_ADDR, 16, (unsigned char *)buffer.data(), 16)) {
        DataStream in(buffer);
        in >> uuid;
    } else {
        setStatusText("Failure reading serial number.");
    }
    return uuid;
}

bool QS1RServer::writeQS1RSN(String uuid) {

    ByteArray buffer(16, 0);

    DataStream out(buffer);

    out << uuid.toStdString();

    if (QsGlobal::g_io->writeEEPROM(QS1R_EEPROM_ADDR, 16, (unsigned char *)buffer.data(), 16)) {
        setStatusText("Updated Serial Number");
        return true;
    } else {
        setStatusText("Could not update serial number");
        return false;
    }
}

void QS1RServer::unregisteredHardwareTimeout() {
    if (!hardware_is_registered) {
        setStatusText("Server is shutting down...");
        quit();
    }
}

void QS1RServer::writeQS1REEPROM() {
    unsigned char buf[16];

    buf[0] = 0xC0; // C0 LOAD
    buf[1] = 0xFE; // lo byte QS1R VID
    buf[2] = 0xFF; // hi byte QS1R VID
    buf[3] = 0x08; // lo byte QS1R PID
    buf[4] = 0x00; // hi byte QS1R PID
    buf[5] = 0x0D; // lo byte DID < revision
    buf[6] = 0x00; // hi byte DID
    buf[7] = 0x00; // configuration byte
    buf[8] = 0x31; // 125.000<NULL>
    buf[9] = 0x32;
    buf[10] = 0x35;
    buf[11] = 0x2E;
    buf[12] = 0x30;
    buf[13] = 0x30;
    buf[14] = 0x30;
    buf[15] = 0x00;

    if (QsGlobal::g_io->writeEEPROM(QS1R_EEPROM_ADDR, 0, buf, 16)) {
        setStatusText("EEPROM updated successfully.");
    } else {
        setStatusText("Failure updating EEPROM.");
    }
}

String QS1RServer::readQS1REEPROMData() {

    // #pragma pack(push)

    // #pragma pack (1)

    struct EepromData {
        unsigned char load;
        unsigned short vid;
        unsigned short pid;
        unsigned short rev;
        unsigned char config;
        char freq[8];
    };
    // #pragma pack(pop)

    const int length = 16;
    const int read_length = 16;
    unsigned char buf[length];
    memset(buf, 0, length);
    String eeprom_data_str = "";

    if (QsGlobal::g_io->readEEPROM(QS1R_EEPROM_ADDR, 0, &buf[0], read_length)) {
        EepromData data;

        memcpy(&data, buf, read_length);

        eeprom_data_str.append("{LOAD: " + String::number(data.load, 16));
        eeprom_data_str.append(", VID: " + String::number(data.vid, 16));
        eeprom_data_str.append(", PID: " + String::number(data.pid, 16));
        eeprom_data_str.append(", REV: " + String::number(data.rev, 16));
        eeprom_data_str.append(", CONFIG: " + String::number(data.config, 16));
        eeprom_data_str.append(", FREQ: " + String::fromAscii((const char *)data.freq, 8));
        eeprom_data_str.append(String("}"));
        return eeprom_data_str;
    } else {
        return "Cannot read EEPROM Data";
    }
}

void QS1RServer::readQS1REEPROM() {

    // #pragma pack(push)

    // #pragma pack (1)

    struct EepromData {
        unsigned char load;
        unsigned short vid;
        unsigned short pid;
        unsigned short rev;
        unsigned char config;
        char freq[8];
    };
    // #pragma pack(pop)

    const int length = 16;
    const int read_length = 16;
    unsigned char buf[length];
    memset(buf, 0, length);

    if (QsGlobal::g_io->readEEPROM(QS1R_EEPROM_ADDR, 0, &buf[0], read_length)) {
        EepromData data;

        memcpy(&data, buf, read_length);

        setStatusText("EEPROM READ FOLLOWS:");
        setStatusText("LOAD: " + String::number(data.load, 16));
        setStatusText("VID: " + String::number(data.vid, 16));
        setStatusText("PID: " + String::number(data.pid, 16));
        setStatusText("REV: " + String::number(data.rev, 16));
        setStatusText("CONFIG: " + String::number(data.config, 16));
        setStatusText("FREQ: " + String::fromAscii((const char *)data.freq, 8));

        setStatusText("RAW READ FOLLOWS:");

        String str;
        str.clear();
        for (int i = 0; i < read_length; i++) {
            if (i == 15) {
                str.append(String::number((int)buf[i], 16));
            } else {
                str.append(String::number((int)buf[i], 16));
                str.append(String(":"));
            }
        }
        setStatusText(str);
        setStatusText("EEPROM read successful.");
    } else {
        setStatusText("Failure reading EEPROM.");
    }
}

// ------------------------------------------------------------
// ************************************************************
// THIS IS THE MAIN COMMAND PROCESSOR
// Please keep it at the end of this file to make it easier
// to find.
// ************************************************************
// ------------------------------------------------------------
// ------------------------------------------------------------
// Runs the received command through the command parser
// and attempts to execute the command if valid.
// Returns "?" if the command was invalid.
// ------------------------------------------------------------
String QS1RServer::doCommandProcessor(String value, int rx_num) {
    if (rx_num > NUMBER_OF_RECEIVERS)
        return "NAK";

    String response;

    response = "?";

    value = value.simplified();

#ifdef _DEBUG_CMDS_
    _debug() << "cmd: " << value;
#endif

    cmd = cmd.processCMD(value);

    if (cmd.RW == CMD::cmd_error) // command not found
        return response;

    //****************************************************//
    //----------------------A-----------------------------//
    //****************************************************//

    else if (cmd.cmd.compare("AgcDecaySpeed") == 0) // sets agc decay speed
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setAgcDecaySpeed(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getAgcDecaySpeed(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //
    // AgcFixedGain n, n = 0,1,...
    //
    else if (cmd.cmd.compare("AgcFixedGain") == 0) // sets agc fixed gain
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setAgcFixedGain(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getAgcFixedGain(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //
    // AgcThreshold n, n = 0,1,...
    //
    else if (cmd.cmd.compare("AgcThreshold") == 0) // sets agc threshold
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setAgcThreshold(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getAgcThreshold(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //
    // AgcSlope n, n = 0,1,...
    //
    else if (cmd.cmd.compare("AgcSlope") == 0) // sets agc slope
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setAgcSlope(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getAgcSlope(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //
    // AgcHangTime n, n = 0,1...
    //
    else if (cmd.cmd.compare("AgcHangTime") == 0) // sets agc hang time
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setAgcHangTime(cmd.ivalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getAgcHangTime(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //
    // AgcHangTimeSwitch b, b = 0,1
    //
    else if (cmd.cmd.compare("AgcHangTimeSwitch") == 0) // sets agc hang time switch
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setAgcHangTimeSwitch(cmd.ivalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            bool value = QsGlobal::g_memory->getAgcHangTimeSwitch(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //
    // AnbSwitch n, n = 0,1
    //
    else if (cmd.cmd.compare("AnbSwitch") == 0) // turns on/off averaging noise blanker
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setAvgNoiseBlankerOn((bool)cmd.ivalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            bool value = QsGlobal::g_memory->getAvgNoiseBlankerOn(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //
    // AnbThreshold d, d = 0.0,1,...
    //
    else if (cmd.cmd.compare("AnbThreshold") == 0) // set averaging noise blanker threshold
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setAvgNoiseBlankerThreshold(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getAvgNoiseBlankerThreshold(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //
    // AutoNotchSwitch n, n = 0,1
    //
    else if (cmd.cmd.compare("AutoNotchSwitch") == 0) // turns on/off automatic notch
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setAutoNotchOn((bool)cmd.ivalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            bool value = QsGlobal::g_memory->getAutoNotchOn(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    else if (cmd.cmd.compare("AutoNotchRate") == 0) // sets autonotch rate
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setAutoNotchRate(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getAutoNotchRate(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //****************************************************//
    //----------------------B-----------------------------//
    //****************************************************//

    //
    // BinauralSwitch n, n = 0,1
    //
    else if (cmd.cmd.compare("BinauralSwitch") == 0) // turns on/off binaural mode
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setBinauralMode(cmd.ivalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            bool value = QsGlobal::g_memory->getBinauralMode(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //
    // BnbSwitch n, n = 0,1
    //
    else if (cmd.cmd.compare("BnbSwitch") == 0) // turns on/off block noise blanker
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setBlockNoiseBlankerOn((bool)cmd.ivalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            bool value = QsGlobal::g_memory->getBlockNoiseBlankerOn(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //
    // BnbThreshold d, d = 0.0,1,...
    //
    else if (cmd.cmd.compare("BnbThreshold") == 0) // sets block noise blanker threshold
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setBlockNoiseBlankerThreshold(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getBlockNoiseBlankerThreshold(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //****************************************************//
    //----------------------C-----------------------------//
    //****************************************************//

    //****************************************************//
    //----------------------D-----------------------------//
    //****************************************************//

    //
    // DitherSwitch n, n = 0,1
    //
    else if (cmd.cmd.compare("DitherSwitch") == 0) // turn on/off adc dither mode
    {
        if (cmd.RW == CMD::cmd_write) {
            setDitherMode((bool)cmd.ivalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            int value = ditherMode();
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
            ;
        }
    }

    //
    // DacOutputDisable n, n = 0,1
    //
    else if (cmd.cmd.compare("DacOutputDisable") == 0) // turn on/off dac disable
    {
        if (cmd.RW == CMD::cmd_write) {
            setDacOutputDisable((bool)cmd.ivalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            if (getDacOutputDisable())
                response = "1";
            else
                response = "0";
        }
    }

    //
    // DisplayFreqOffset d
    //
    else if (cmd.cmd.compare("DisplayFreqOffset") == 0 || cmd.cmd.compare("dfo") == 0) //  display offset freq
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setDisplayFreqOffset(cmd.dvalue, 0);
            setRxFrequency(QsGlobal::g_memory->getRxLOFrequency(), 1, true);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getDisplayFreqOffset(rx_num - 1);
            getDisplayFreqOffset(value, rx_num);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //****************************************************//
    //----------------------E-----------------------------//
    //****************************************************//

    //
    // Exit
    //
    else if (cmd.cmd.compare("Exit") == 0) // exits the server
    {
        if (cmd.RW == CMD::cmd_write) {
            response = "OK";
            setStatusText("QS1RServer quitting...");
            sleep.msleep(500);
            quit();
        } else if (cmd.RW == CMD::cmd_read) {
            response = "NAK";
        }
    }

    //
    // EncodeClockCorrection d, d = ...
    //
    else if (cmd.cmd.compare("EncodeClockCorrection") == 0) // set encode clock frequency correction in Hz
    {
        if (cmd.RW == CMD::cmd_write) {
            setFreqCorrection(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getEncodeFreqCorrect();
            getFreqCorrection(value);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //****************************************************//
    //----------------------F-----------------------------//
    //****************************************************//

    //
    // Freq d, d = 0.0 to 62.5e6 Hz
    //
    else if (cmd.cmd.compare("Freq") == 0 || cmd.cmd.compare("fHz") == 0) // set frequency in Hz
    {
        if (cmd.RW == CMD::cmd_write) {
            setRxFrequency(cmd.dvalue, rx_num);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getRxLOFrequency(rx_num - 1);
            getRxFrequency(value, rx_num);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //
    // fkHz d, d = 0.0 to 62.5e3 kHz
    //
    else if (cmd.cmd.compare("fkHz") == 0) // set frequency in kHz
    {
        if (cmd.RW == CMD::cmd_write) {
            setRxFrequency(cmd.dvalue * 1.0e3, rx_num);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getRxLOFrequency(rx_num - 1);
            getRxFrequency(value, rx_num);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //
    // fMHz d, d = 0.0 to 62.5 MHz
    //
    else if (cmd.cmd.compare("fMHz") == 0) // set frquency in MHz
    {
        if (cmd.RW == CMD::cmd_write) {
            setRxFrequency(cmd.dvalue * 1.0e6, rx_num);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getRxLOFrequency(rx_num - 1);
            getRxFrequency(value, rx_num);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value / 1.0e6));
        }
    }

    //
    // FilterLow d, d = 0 to 20000.0
    //
    else if (cmd.cmd.compare("FilterLow") == 0 || cmd.cmd.compare("fl") == 0) // filter low value
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setFilterLo(cmd.ivalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            int value = QsGlobal::g_memory->getFilterLo(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //
    // FilterHigh d, d = 0 to 20000.0
    //
    else if (cmd.cmd.compare("FilterHigh") == 0 || cmd.cmd.compare("fh") == 0) // filter hi value
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setFilterHi(cmd.ivalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            int value = QsGlobal::g_memory->getFilterHi(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //
    // FilterSet d,d d = 0 to 20000.0
    //
    else if (cmd.cmd.compare("Filter") == 0) // filter value
    {
        if (cmd.RW == CMD::cmd_write) {
            if (cmd.slist.size() == 2) {
                String fl_str = cmd.slist[0];
                String fh_str = cmd.slist[1];
                int flo = fl_str.toInt();
                int fhi = fh_str.toInt();
                QsGlobal::g_memory->setFilterHi(fhi);
                QsGlobal::g_memory->setFilterLo(flo);
                response = "OK";
            }
        } else if (cmd.RW == CMD::cmd_read) {
            int value1 = QsGlobal::g_memory->getFilterLo(rx_num - 1);
            int value2 = QsGlobal::g_memory->getFilterHi(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value1));
            response.append(String(", "));
            response.append(String::number(value2));
        }
    }

    //
    // FilterLowTx d, d = 0 to 20000.0
    //
    else if (cmd.cmd.compare("FilterLowTx") == 0 || cmd.cmd.compare("fltx") == 0) // tx filter low value
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setTxFilterLo(cmd.ivalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            int value = QsGlobal::g_memory->getTxFilterLo();
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //
    // FilterHighTx d, d = 0 to 20000.0
    //
    else if (cmd.cmd.compare("FilterTxHigh") == 0 || cmd.cmd.compare("fhtx") == 0) // txfilter hi value
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setTxFilterHi(cmd.ivalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            int value = QsGlobal::g_memory->getTxFilterHi();
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //
    // FIRMWAREID
    //
    else if (cmd.cmd.compare("FirmwareID") == 0) // filter tap size
    {
        if (cmd.RW == CMD::cmd_write) {
            response = "NAK";
        } else if (cmd.RW == CMD::cmd_read) {
            int value = QsGlobal::g_io->readFwSn();
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //
    // FPGACONFIGID
    //
    else if (cmd.cmd.compare("FpgaConfigID") == 0) // filter tap size
    {
        if (cmd.RW == CMD::cmd_write) {
            response = "NAK";
        } else if (cmd.RW == CMD::cmd_read) {
            int value = QsGlobal::g_io->readMultibusInt(MB_VERSION_REG);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value, 16));
        }
    }

    //****************************************************//
    //----------------------G-----------------------------//
    //****************************************************//

    //****************************************************//
    //----------------------H-----------------------------//
    //****************************************************//

    //****************************************************//
    //----------------------I-----------------------------//
    //****************************************************//

    //
    // InitHardware
    //
    else if (cmd.cmd.compare("InitHardware") == 0) // initialize the hardware
    {
        if (cmd.RW == CMD::cmd_write) {
            initQS1RHardware();
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(m_is_hardware_init));
        }
    }

    //****************************************************//
    //----------------------L-----------------------------//
    //****************************************************//

    //****************************************************//
    //----------------------M-----------------------------//
    //****************************************************//

    //
    // Mode n, n = QSDEMODMODE enum
    //
    else if (cmd.cmd.compare("Mode") == 0) // get demod mode
    {
        if (cmd.RW == CMD::cmd_write) {
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            QSDEMODMODE value = QsGlobal::g_memory->getDemodMode(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }
    //
    // ModeStr n, n = QSDEMODMODE enum
    //
    else if (cmd.cmd.compare("ModeStr") == 0) // get demod mode
    {
        if (cmd.RW == CMD::cmd_write) {

            int value = (int)modeStringToMode(cmd.svalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            QSDEMODMODE value = QsGlobal::g_memory->getDemodMode(rx_num - 1);
            String mode_str = getModeString(value);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }
    //
    // mAM Set AM mode
    //
    else if (cmd.cmd.compare("mAM") == 0) {
        if (cmd.RW == CMD::cmd_write) {
            unsigned int fl = 100.0;
            unsigned int fh = 4000.0;
            QsGlobal::g_memory->setDemodMode(dmAM);
            QsGlobal::g_memory->setFilterHi(fh);
            QsGlobal::g_memory->setFilterLo(fl);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            response = "NAK";
        }
    }
    //
    // mSAM Set SAM mode
    //
    else if (cmd.cmd.compare("mSAM") == 0) {
        if (cmd.RW == CMD::cmd_write) {
            unsigned int fl = 100.0;
            unsigned int fh = 4000.0;
            QsGlobal::g_memory->setDemodMode(dmSAM);
            QsGlobal::g_memory->setFilterHi(4000.0);
            QsGlobal::g_memory->setFilterLo(-4000.0);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            response = "NAK";
        }
    }
    //
    // mLSB Set LSB mode
    //
    else if (cmd.cmd.compare("mLSB") == 0) {
        if (cmd.RW == CMD::cmd_write) {
            unsigned int fl = 100.0;
            unsigned int fh = 3000.0;
            QsGlobal::g_memory->setDemodMode(dmLSB);
            QsGlobal::g_memory->setFilterHi(fh);
            QsGlobal::g_memory->setFilterLo(fl);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            response = "NAK";
        }
    }
    //
    // mUSB Set USB mode
    //
    else if (cmd.cmd.compare("mUSB") == 0) {
        if (cmd.RW == CMD::cmd_write) {
            unsigned int fl = 100.0;
            unsigned int fh = 3000.0;
            QsGlobal::g_memory->setDemodMode(dmUSB);
            QsGlobal::g_memory->setFilterHi(fh);
            QsGlobal::g_memory->setFilterLo(fl);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            response = "NAK";
        }
    }
    //
    // mDSB Set DSB mode
    //
    else if (cmd.cmd.compare("mDSB") == 0) {
        if (cmd.RW == CMD::cmd_write) {
            unsigned int fl = 100.0;
            unsigned int fh = 3000.0;
            QsGlobal::g_memory->setDemodMode(dmDSB);
            QsGlobal::g_memory->setFilterHi(fh);
            QsGlobal::g_memory->setFilterLo(fl);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            response = "NAK";
        }
    }
    //
    // mCW Set CW mode
    //
    else if (cmd.cmd.compare("mCW") == 0) {
        if (cmd.RW == CMD::cmd_write) {
            unsigned int fl = 100.0;
            unsigned int fh = 250.0;
            QsGlobal::g_memory->setDemodMode(dmCW);
            QsGlobal::g_memory->setFilterHi(fh);
            QsGlobal::g_memory->setFilterLo(fl);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            response = "NAK";
        }
    }

    //
    // Mute b, b = 0 or 1
    //
    else if (cmd.cmd.compare("mute") == 0) {
        if (cmd.RW == CMD::cmd_write) {
            if (cmd.ivalue == 1) {
                m_prev_vol_val = QsGlobal::g_memory->getVolume(rx_num - 1);
                QsGlobal::g_memory->setVolume(-120, rx_num - 1);
            } else if (cmd.ivalue == 0) {
                QsGlobal::g_memory->setVolume(m_prev_vol_val, rx_num - 1);
            }
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            response = "NAK";
        }
    }

    //****************************************************//
    //----------------------N-----------------------------//
    //****************************************************//

    //
    // NoiseReductionSwitch n, n = 0,1
    //
    else if (cmd.cmd.compare("NoiseReductionSwitch") == 0) // turn on/off noise reduction
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setNoiseReductionOn((bool)cmd.ivalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            bool value = QsGlobal::g_memory->getNoiseReductionOn(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    else if (cmd.cmd.compare("NoiseReductionRate") == 0) // sets noise reduction rate
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setNoiseReductionRate(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getNoiseReductionRate(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //****************************************************//
    //----------------------O-----------------------------//
    //****************************************************//

    //
    // OffsetGenFreq d, n = -20000.0 to 20000.0
    //
    else if (cmd.cmd.compare("OffsetGenFreq") == 0 || cmd.cmd.compare("ogf") == 0) //  oscillator offset freq
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setOffsetGeneratorFrequency(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getOffsetGeneratorFrequency(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //****************************************************//
    //----------------------P-----------------------------//
    //****************************************************//

    //
    // PgaSwitch n, n = 0,1
    //
    else if (cmd.cmd.compare("PgaSwitch") == 0) // turn off/on adc pga
    {
        if (cmd.RW == CMD::cmd_write) {
            setPgaMode((bool)cmd.ivalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(pgaMode()));
        }
    }

    //
    // PsCorrection d, d = -120.0 to +120.0
    //
    else if (cmd.cmd.compare("PsCorrection") == 0) // set power spectrum correction in dbm
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setPsCorrection(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getPsCorrection();
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //
    // PostPsCorrection d, d = -120.0 to +120.0
    //
    else if (cmd.cmd.compare("PostPsCorrection") == 0) // set power spectrum correction in dbm
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setPostPsCorrection(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getPostPsCorrection();
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //****************************************************//
    //----------------------R-----------------------------//
    //****************************************************//

    //
    // RandSwitch n, n = 0,1
    //
    else if (cmd.cmd.compare("RandSwitch") == 0) // turn on/off adc random mode
    {
        if (cmd.RW == CMD::cmd_write) {
            setRandMode((bool)cmd.ivalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(randMode()));
        }
    }

    //
    // ReadQS1RSN
    //
    else if (cmd.cmd.compare("_ReadQS1RSN_") == 0) // read qs1r serial number
    {
        if (cmd.RW == CMD::cmd_write) {
            response = "NAK";
        } else if (cmd.RW == CMD::cmd_read) {
            UUID value = readQS1RUuid();
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(value.toString());
        }
    }

    //****************************************************//
    //----------------------S-----------------------------//
    //****************************************************//

    //
    // Start
    //
    else if (cmd.cmd.compare("Start") == 0) // start dsp
    {
        if (cmd.RW == CMD::cmd_write) {
            startIo(false);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(m_is_io_running));
        }
    }

    //
    // Status=mode,frequency,offset;
    //
    else if (cmd.cmd.compare("Status") == 0) // get status for CAT Comms
    {
        if (cmd.RW == CMD::cmd_write) {
            response = "NAK";
        } else if (cmd.RW == CMD::cmd_read) {
            QSDEMODMODE value = QsGlobal::g_memory->getDemodMode(rx_num - 1);
            String mode_str = getModeString(value);
            double dvalue = QsGlobal::g_memory->getRxLOFrequency(rx_num - 1);
            getRxFrequency(dvalue, rx_num);
            double tvalue = QsGlobal::g_memory->getToneLoFrequency(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String(mode_str));
            response.append(String(","));
            response.append(String::number(dvalue));
            response.append(String(","));
            response.append(String::number(tvalue, 0));
            response.append(String(";"));
        }
    }

    //
    // Stop
    //
    else if (cmd.cmd.compare("Stop") == 0) // stop all io
    {
        if (cmd.RW == CMD::cmd_write) {
            stopIo();
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(!m_is_io_running));
        }
    }

    //
    // SampleRate d, d = {supported sample rate}
    //
    else if (cmd.cmd.compare("SampleRate") == 0) // sample rate
    {
        if (cmd.RW == CMD::cmd_write) {
            if (setFpgaForSampleRate(cmd.dvalue))
                response = "OK";
            else
                response = "NAK";
        } else if (cmd.RW == CMD::cmd_read) {
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(QsGlobal::g_memory->getDataProcRate()));
        }
    }

    //
    // SupportedSampleRates
    //
    else if (cmd.cmd.compare("SupportedSampleRates") == 0) // sample rate
    {
        if (cmd.RW == CMD::cmd_write) {
            response = "NAK";
        } else if (cmd.RW == CMD::cmd_read) {
            StringList list = getSupportedSampleRates();
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(list.join(", "));
        }
    }

    //
    // SquelchSwitch n, n = 0,1
    //
    else if (cmd.cmd.compare("SquelchSwitch") == 0) // set squelch on/off
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setSquelchOn((bool)cmd.ivalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            bool value = QsGlobal::g_memory->getSquelchOn(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //
    // SquelchThreshold d, d = -120.0 to +120.0
    //
    else if (cmd.cmd.compare("SquelchThreshold") == 0) // set squelch threshold
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setSquelchThreshold(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getSquelchThreshold(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //
    // SmeterCorrection d, d = -120.0 to +120.0
    //
    else if (cmd.cmd.compare("SmeterCorrection") == 0) // s meter correction in dbm
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setSMeterCorrection(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getSMeterCorrection();
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //
    // SmeterValue
    //
    else if (cmd.cmd.compare("SmeterValue") == 0) // get s meter value
    {
        if (cmd.RW == CMD::cmd_write) {
            response = "NAK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getSMeterCurrentValue(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //****************************************************//
    //----------------------T-----------------------------//
    //****************************************************//

    //
    // ToneFrequency d, n = -20000.0 to 20000.0
    //
    else if (cmd.cmd.compare("ToneFrequency") == 0 || cmd.cmd.compare("tf") == 0) // tone oscillator tone freq
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setToneLoFrequency(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getToneLoFrequency(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //****************************************************//
    //----------------------U-----------------------------//
    //****************************************************//

    else if (cmd.cmd.compare("UpdateRxFreq") == 0) // forces an update of the rx frequency
    {
        if (cmd.RW == CMD::cmd_write) {
            double value = QsGlobal::g_memory->getRxLOFrequency(rx_num - 1);
            setRxFrequency(value, rx_num, true);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            response = "NAK";
        }
    }

    //****************************************************//
    //----------------------V-----------------------------//
    //****************************************************//

    //
    // Version
    //
    else if (cmd.cmd.compare("Version") == 0) // software version number
    {
        if (cmd.RW == CMD::cmd_write) {
            response = "NAK";
        } else if (cmd.RW == CMD::cmd_read) {
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String(SDRMAXV_VERSION));
        }
    }

    //
    // Vol d, d = -120 to 0
    //
    else if (cmd.cmd.compare("Vol") == 0 || cmd.cmd.compare("v") == 0) // rx volume
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setVolume(std::clamp(cmd.dvalue, -120.0, 0.0));
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getVolume(rx_num - 1);
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    //****************************************************//
    //----------------------W-----------------------------//
    //****************************************************//

    //****************************************************//
    //----------------------Z-----------------------------//
    //****************************************************//

    else if (cmd.cmd.compare("ZWB") == 0) // aquires a wb block ( test only )
    {
        if (cmd.RW == CMD::cmd_write) {
            response = "NAK";
        } else if (cmd.RW == CMD::cmd_read) {
            short *data = new short[WB_BLOCK_SIZE];
            int value = QsGlobal::g_io->readEP8((unsigned char *)data, WB_BLOCK_SIZE * sizeof(short));
            delete[] data;
            response.append(cmd.cmd);
            response.append(String("="));
            response.append(String::number(value));
        }
    }

    return response;
}

// ------------------------------------------------------------
// Parses and executes commands from the command entry box
// ------------------------------------------------------------
void QS1RServer::parseLocalCommand() {
    String resp = ";";

    if (resp == "quit" || resp == "exit") // quit
    {
        setStatusText("QS1RServer quitting...");
        sleep.msleep(1500);
        quit();
    } else if (resp == "pdac_reset") {
        unsigned char buf = 0x6;

        int result = QsGlobal::g_io->writeI2C(0x00, &buf, 0x1);

        setStatusText("result is " + String::number(result));
    } else if (resp == "pdac_wakeup") {
        unsigned char buf = 0x9;

        int result = QsGlobal::g_io->writeI2C(0x00, &buf, 0x1);

        setStatusText("result is " + String::number(result));
    } else if (resp == "set_pdac_fs") {
        unsigned char buf[2] = {0xf, 0xff};

        int result = QsGlobal::g_io->writeI2C(QS1E_PDAC_ADDR, buf, 0x2);

        setStatusText("result is " + String::number(result));
    } else if (resp == "set_pdac_hs") {
        unsigned char buf[2] = {0x7, 0xff};

        int result = QsGlobal::g_io->writeI2C(QS1E_PDAC_ADDR, buf, 0x2);

        setStatusText("result is " + String::number(result));
    } else if (resp == "set_pdac_zero") {
        unsigned char buf[2] = {0x0, 0x00};

        int result = QsGlobal::g_io->writeI2C(QS1E_PDAC_ADDR, buf, 0x2);

        setStatusText("result is " + String::number(result));
    } else if (resp.contains("set_pdac ")) {
        String str = resp.remove("set_pdac ");
        bool ok = false;
        int value = str.toInt(&ok);
        if (ok) {
            unsigned char buf[2] = {0x0, 0x0};
            buf[0] = (value >> 8) & 0x0f;
            buf[1] = value & 0xff;

            int result = QsGlobal::g_io->writeI2C(QS1E_PDAC_ADDR, buf, 0x2);

            setStatusText("result is " + String::number(result));
        } else {
            setStatusText("Not a valid input");
        }
    } else if (resp.contains("get_pdac")) {
        unsigned char buf[5] = {0x0, 0x0, 0x0, 0x0, 0x0};

        int result = QsGlobal::g_io->readI2C(QS1E_PDAC_ADDR, buf, 0x5);

        bool rdy_busy_bit = (bool)((buf[0] & 0x80) == 0x80);
        bool por_bit = (bool)((buf[0] & 0x80) == 0x40);
        bool pd1_bit = (bool)((buf[0] & 0x04) == 0x04);
        bool pd0_bit = (bool)((buf[0] & 0x02) == 0x02);

        unsigned short dac_register_data = (buf[1] << 4) + (buf[2] >> 4);
        unsigned short eeprom_register_data = ((buf[3] & 0xf) << 8) + buf[4];

        bool eeprom_pd1_bit = (bool)((buf[3] & 0x40) == 0x40);
        bool eeprom_pd0_bit = (bool)((buf[3] & 0x20) == 0x20);

        setStatusText("bytes read is " + String::number(result));
        setStatusText("Ready/Busy: " + String::number(rdy_busy_bit));
        setStatusText("POR: " + String::number(por_bit));
        setStatusText("PD1: " + String::number(pd1_bit));
        setStatusText("PD0: " + String::number(pd0_bit));
        setStatusText("DAC DATA: " + String::number(dac_register_data, 16) + "(" +
                      String::number(dac_register_data, 10) + ")");
        setStatusText("EEPROM PD1: " + String::number(eeprom_pd1_bit));
        setStatusText("EEPROM PD1: " + String::number(eeprom_pd0_bit));
        setStatusText("EEPROM DATA: " + String::number(eeprom_register_data, 16) + "(" +
                      String::number(eeprom_register_data, 10) + ")");
    } else if (resp.contains("set_txf ")) {
        String str = resp.remove("set_txf ");
        bool ok = false;
        int ivalue = str.toInt(&ok);
        if (ok) {
            int freq = static_cast<int>(std::round(static_cast<double>(ivalue) / 125.0e6 * std::pow(2.0, 32.0)));
            int result = QsGlobal::g_io->writeMultibusInt(MB_TX_FREQ, freq);
            setStatusText("result is " + String::number(result));
        } else {
            double dvalue = str.toDouble(&ok);
            if (ok) {
                int freq = static_cast<int>(std::round(static_cast<double>(ivalue) / 125.0e6 * std::pow(2.0, 32.0)));
                int result = QsGlobal::g_io->writeMultibusInt(MB_TX_FREQ, freq);
                setStatusText("result is " + String::number(result));
            } else {
                setStatusText("Not a valid input");
            }
        }
    } else if (resp.contains("echo_ep0_ep1")) {
        constexpr std::size_t sz = 64;
        std::vector<std::byte> buf(sz, std::byte{22});
        std::vector<std::byte> buf1(sz, std::byte{99});

        int result = QsGlobal::g_io->sendControlMessage(VRT_VENDOR_OUT, VRQ_ECHO_TO_EP1IN, 0, 0, buf.data(), sz, 10000);
        _debug() << "echo result control message was " << result;
        result = QsGlobal::g_io->readEP1(reinterpret_cast<unsigned char *>(buf1.data()), sz);
        _debug() << "echo read ep1 was " << result;
        for (int i = 0; i < result; i++) {
            unsigned char value = static_cast<unsigned char>(buf1[i]);
            _debug() << " buf1[" << i << "] : " << String::number(value);
        }
    } else if (resp.contains("read_ep1")) {
        std::vector<std::byte> buf(4, std::byte{0});
        int result = QsGlobal::g_io->readEP1(reinterpret_cast<unsigned char *>(buf.data()), 4);
        // Loop through buf and convert each std::byte to a number
        for (size_t i = 0; i < buf.size(); ++i) {
            unsigned char value = static_cast<unsigned char>(buf[i]); // Cast std::byte to unsigned char
            _debug() << "read [" << i << "] : " << String::number(value);
        }
    } else if (resp.contains("enable_int5")) {
        int result = QsGlobal::g_io->sendInterrupt5Gate();
        _debug() << "enable int5 result: " << String::number(result);
    } else if (resp == "clear") {
        ;
        showStartupMessage();
    } else if (resp == "start") {
        startIo();
        String str = String("Started at sample rate: ") + String::number(m_proc_samplerate);
        setStatusText(str);
    } else if (resp == "stop") {
        stopIo();
        setStatusText("Stopped");
    } else if (resp == "u") {
        double val = 0.0;
        getRxFrequency(val, 1);
        setRxFrequency(val + m_step_size, 1);
        setStatusText("Setting Freq to: " + String::number(val));
    } else if (resp == "d") {
        double val = 0.0;
        getRxFrequency(val, 1);
        setRxFrequency(val - m_step_size, 1);
        setStatusText("Setting Freq to: " + String::number(val));
    } else if (resp == "am") {
        QsGlobal::g_memory->setDemodMode(dmAM);
        QsGlobal::g_memory->setFilterHi(4000);
        QsGlobal::g_memory->setFilterLo(-4000);
        setStatusText("Setting mode to AM, filter BW: 4000 Hz");
    } else if (resp == "sam") {
        QsGlobal::g_memory->setDemodMode(dmSAM);
        QsGlobal::g_memory->setFilterHi(4000);
        QsGlobal::g_memory->setFilterLo(-4000);
        setStatusText("Setting mode to SAM, filter BW: 4000 Hz");
    } else if (resp == "lsb") {
        QsGlobal::g_memory->setDemodMode(dmLSB);
        QsGlobal::g_memory->setFilterHi(-3000.0);
        QsGlobal::g_memory->setFilterLo(-100.0);
        setStatusText("Setting mode to LSB, filter BW: 3000 Hz");
    } else if (resp == "usb") {
        QsGlobal::g_memory->setDemodMode(dmUSB);
        QsGlobal::g_memory->setFilterHi(3000.0);
        QsGlobal::g_memory->setFilterLo(100.0);
        setStatusText("Setting mode to USB, filter BW: 3000 Hz");
    } else if (resp == "dsb") {
        QsGlobal::g_memory->setDemodMode(dmDSB);
        QsGlobal::g_memory->setFilterHi(3000.0);
        QsGlobal::g_memory->setFilterLo(-3000.0);
        setStatusText("Setting mode to DSB, filter BW: 3000 Hz");
    } else if (resp == "cw") {
        QsGlobal::g_memory->setDemodMode(dmCW);
        QsGlobal::g_memory->setFilterHi(250);
        QsGlobal::g_memory->setFilterLo(-250);
        setStatusText("Setting mode to CW, filter BW: 500 Hz");
    } else if (resp.contains("set step_size ")) {
        m_step_size = 0;
        String str = resp.remove("set step_size ");
        bool ok = false;
        double value = str.toDouble(&ok);
        if (ok) {
            m_step_size = value;
            setStatusText("Setting step size to: " + String::number(value));
        } else {
            setStatusText("Step size is invalid.");
        }
    } else if (resp.contains("set freq_hz ")) {
        String str = resp.remove("set freq_hz ");
        bool ok = false;
        double value = str.toDouble(&ok);
        if (ok) {
            setStatusText("Setting frequency to: " + String::number(value) + " Hz");
            setRxFrequency(value, 1);
        } else {
            setStatusText("Value is invalid.");
        }
    } else if (resp == "get freq_hz") {
        double value = 0.0;
        getRxFrequency(value, 1);
        setStatusText("Current frequency: " + String::number(value) + " Hz");
    } else if (resp.contains("set freq_khz ")) {
        String str = resp.remove("set freq_khz ");
        bool ok = false;
        double value = str.toDouble(&ok);
        if (ok) {
            setStatusText("Setting frequency to: " + String::number(value) + " kHz");
            setRxFrequency(value * 1000.0, 1);
        } else {
            setStatusText("Value is invalid.");
        }
    } else if (resp == "get freq_khz") {
        double value = 0.0;
        getRxFrequency(value, 1);
        setStatusText("Current frequency: " + String::number(value / 1000.0) + " kHz");
    } else if (resp.contains("set freq_mhz ")) {
        String str = resp.remove("set freq_mhz ");
        bool ok = false;
        double value = str.toDouble(&ok);
        if (ok) {
            setStatusText("Setting frequency to: " + String::number(value) + " MHz");
            setRxFrequency(value * 1000000.0, 1);
        } else {
            setStatusText("Value is invalid.");
        }
    } else if (resp == "get freq_mhz") {
        double value = 0.0;
        getRxFrequency(value, 1);
        setStatusText("Current frequency: " + String::number(value / 1000000.0) + " MHz");
    } else if (resp.contains("set notch ")) {
        std::string str =
            resp.toStdString().substr(resp.toStdString().find("set notch ") + 11); // 11 is the length of "set notch "
        // Split the string by comma
        std::vector<std::string> sl;
        std::stringstream ss(str);
        std::string item;
        while (std::getline(ss, item, ',')) {
            sl.push_back(item);
        }

        if (sl.size() == 4) {
            int num = std::stoi(sl[0]);      // Convert first element to int
            float f0 = std::stof(sl[1]);     // Convert second element to float
            float hz = std::stof(sl[2]);     // Convert third element to float
            bool en = std::stoi(sl[3]) != 0; // Convert fourth element to bool

            QsGlobal::g_memory->setNotchFrequency(num, f0);
            QsGlobal::g_memory->setNotchBandwidth(num, hz);
            QsGlobal::g_memory->setNotchEnabled(num, en);
        }
    } else if (resp == "reinit hardware") {
        setStatusText("Trying to initialize QS1R Hardware...wait...");
        initQS1RHardware();
    } else if (resp == "reinit audio") {
        if (m_is_io_running)
            stopIo();
#ifdef __SOUND_OUT__
        initQsAudio(SR_OUT0);
#endif
    } else if (resp == "reinit dsp") {
        if (m_is_io_running)
            stopIo();
        setupIo();
    } else if (resp == "get outdevices") {
        setStatusText("\nAudio Output Devices:");

        StringList list = p_rta->getOutputDevices();

        for (int i = 0; i < list.size(); i++) {
            setStatusText(String(list.at(i)));
        }
    } else if (resp == "get indevices") {
        setStatusText("\nAudio Input Devices:");

        StringList list = p_rta->getInputDevices();

        for (int i = 0; i < list.size(); i++) {
            setStatusText(String(list.at(i)));
        }
    } else if (resp.toStdString().find("set indevice ") != std::string::npos) {
        std::string str = resp.toStdString();
        std::size_t str_find = str.find("set indevice "); // Get the position of the substring

        if (str_find != std::string::npos) {                 // Check if the substring was found
            std::string str_sub = str.substr(str_find + 14); // Extract substring after "set indevice "

            // Trim whitespace from str_sub
            str_sub.erase(0, str_sub.find_first_not_of(" \t")); // Trim left
            str_sub.erase(str_sub.find_last_not_of(" \t") + 1); // Trim right.

            String devname;
            bool ok = false;
            int devid = std::stoi(str_sub, nullptr); // Convert to int; use nullptr for error checking

            // Check if the conversion was successful
            if (devid >= 0) {
                if (p_rta->isInputDeviceValid(devid, devname)) {
                    setStatusText("\nSetting input device id to: " + std::to_string(devid));
                    p_qsState->setRtAudioInDevId(devid);
                } else {
                    setStatusText("Device id: " + std::to_string(devid) + " is invalid.");
                }
            } else {
                setStatusText("The device id you entered is invalid.");
            }
        }
    } else if (resp == "get indevice") {
        int id = p_qsState->rtAudioInDevId();
        setStatusText("Input device id is: " + String::number(id));
        String devname;

        p_rta->isInputDeviceValid(id, devname);
        setStatusText("Input device name is: " + devname);
    } else if (resp.contains("set outdevice ")) {
        std::string str = resp.toStdString();
        std::size_t str_find = str.find("set outdevice "); // Get the position of the substring

        if (str_find != std::string::npos) {                 // Check if the substring was found
            std::string str_sub = str.substr(str_find + 15); // Extract substring after "set outdevice "

            // Trim whitespace from str_sub
            str_sub.erase(0, str_sub.find_first_not_of(" \t")); // Trim left
            str_sub.erase(str_sub.find_last_not_of(" \t") + 1); // Trim right.

            String devname;
            bool ok = false;
            int devid = std::stoi(str_sub); // Convert to int; use std::stoi directly

            // Check if the conversion is valid
            if (devid >= 0) {
                if (p_rta->isOutputDeviceValid(devid, devname)) {
                    setStatusText("\nSetting output device id to: " + String::number(devid));
                    p_qsState->setRtAudioOutDevId(devid);
                } else {
                    setStatusText("Device id: " + String::number(devid) + " is invalid.");
                }
            } else {
                setStatusText("The device id you entered is invalid.");
            }
        }
    } else if (resp == "get outdevice") {
        int id = p_qsState->rtAudioOutDevId();
        setStatusText("Output device id is: " + String::number(id));
        String devname;
        p_rta->isOutputDeviceValid(id, devname);
        setStatusText("Output device name is: " + devname);
    } else if (resp.contains("set startup_samplerate ")) {
        String str = resp.remove("set startup_samplerate ");
        bool ok = false;
        int value = str.toInt(&ok);
        if (ok) {
            p_qsState->setStartupSampleRate((double)value);
            setStatusText("Setting startup sample rate to: " + String::number(value));
        } else {
            setStatusText("Sample rate is invalid.");
        }
    } else if (resp == "get startup_samplerate") {
        setStatusText("Startup sample rate is: " + String::number(p_qsState->startupSampleRate()));
    } else if (resp.contains("set startup_frequency ")) {
        String str = resp.remove("set startup_frequency ");
        bool ok = false;
        int value = str.toInt(&ok);
        if (ok) {
            p_qsState->setStartupFrequency((double)value);
            setStatusText("Setting startup frequency to: " + String::number(value));
        } else {
            setStatusText("Frequency is invalid.");
        }
    } else if (resp == "get startup_frequency") {
        setStatusText("Startup frequency is: " + String::number(p_qsState->startupFrequency()));
    } else if (resp.contains("set startup_volume ")) {
        String str = resp.remove("set startup_volume ");
        bool ok = false;
        double value = str.toDouble(&ok);
        if (ok) {
            p_qsState->setStartupVolume(value);
            QsGlobal::g_memory->setVolume(value);
            setStatusText("Setting startup volume to: " + String::number(value));
        } else {
            setStatusText("Volume is invalid.");
        }
    } else if (resp == "get startup_filterhi_value") {
        setStatusText("Startup filterhi value is: " + String::number(p_qsState->startupFilterHigh()));
    } else if (resp.contains("set startup_filterhi_value ")) {
        String str = resp.remove("set startup_filterhi_value ");
        bool ok = false;
        double value = str.toDouble(&ok);
        if (ok) {
            p_qsState->setStartupFilterHigh(value);
            setStatusText("Setting startup filterhi value to: " + String::number(value));
        } else {
            setStatusText("Value is invalid.");
        }
    } else if (resp == "get startup_filterlo_value") {
        setStatusText("Startup filterlo value is: " + String::number(p_qsState->startupFilterHigh()));
    } else if (resp.contains("set startup_filterlo_value ")) {
        String str = resp.remove("set startup_filterlo_value ");
        bool ok = false;
        double value = str.toDouble(&ok);
        if (ok) {
            p_qsState->setStartupFilterLow(value);
            setStatusText("Setting startup filterlo value to: " + String::number(value));
        } else {
            setStatusText("Value is invalid.");
        }
    } else if (resp == "get startup_volume") {
        setStatusText("Startup volume is: " + String::number(p_qsState->startupVolume()));
    } else if (resp.contains("set startup_pga_value ")) {
        String str = resp.remove("set startup_pga_value ");
        bool ok = false;
        bool value = str.toInt(&ok);
        if (ok) {
            p_qsState->setPGA((bool)value);
            setPgaMode((bool)value);
            setStatusText("Setting startup pga value to: " + String::number(value));
        } else {
            setStatusText("Value is invalid.");
        }
    } else if (resp == "get startup_pga_value") {
        setStatusText("Startup pga value is: " + String::number(p_qsState->pga()));
    } else if (resp.contains("set startup_rand_value ")) {
        String str = resp.remove("set startup_rand_value ");
        bool ok = false;
        bool value = str.toInt(&ok);
        if (ok) {
            p_qsState->setRAND((bool)value);
            setRandMode((bool)value);
            setStatusText("Setting startup rand value to: " + String::number(value));
        } else {
            setStatusText("Value is invalid.");
        }
    } else if (resp == "get startup_rand_value") {
        setStatusText("Startup rand value is: " + String::number(p_qsState->rand()));
    } else if (resp.contains("set startup_dither_value ")) {
        String str = resp.remove("set startup_dither_value ");
        bool ok = false;
        bool value = str.toInt(&ok);
        if (ok) {
            p_qsState->setDITH((bool)value);
            setDitherMode((bool)value);
            setStatusText("Setting startup dither value to: " + String::number(value));
        } else {
            setStatusText("Value is invalid.");
        }
    } else if (resp == "get startup_dither_value") {
        setStatusText("Startup dither value is: " + String::number(p_qsState->dith()));
    } else if (resp.contains("set startup_agc_decay_speed ")) {
        String str = resp.remove("set startup_agc_decay_speed ");
        bool ok = false;
        int value = str.toInt(&ok);
        if (ok) {
            p_qsState->setStartupAGCDecaySpeed((double)value);
            setStatusText("Setting startup agc mode to: " + str.toUpper());
        } else {
            setStatusText("Value is invalid.");
        }
    } else if (resp.contains("get startup_agc_decay_speed")) {
        String str = String::number(p_qsState->startupAGCDecaySpeed());
        setStatusText("Startup agc mode is: " + str);
    } else if (resp.contains("set startup_agc_threshold ")) {
        String str = resp.remove("set startup_agc_threshold ");
        bool ok = false;
        double value = str.toDouble(&ok);
        if (ok) {
            p_qsState->setStartupAGCThreshold(value);
            setStatusText("Setting startup agc threshold to: " + String::number(value));
        } else {
            setStatusText("Value is invalid.");
        }
    } else if (resp.contains("get startup_agc_threshold")) {
        setStatusText("Startup agc threshold is: " + String::number(p_qsState->startupAGCThreshold()));
    } else if (resp.contains("set startup_mode ")) {
        String str = resp.remove("set startup_mode ");
        int value = modeStringToInt(str.toStdString());
        if (value != -1) {
            p_qsState->setStartupMode((QSDEMODMODE)value);
            setStatusText("Setting startup mode to: " + str.toUpper());
        } else {
            setStatusText("Value is invalid.");
        }
    } else if (resp.contains("get startup_mode")) {
        String str = modeIntToString(p_qsState->startupMode());
        setStatusText("Startup mode is: " + str);
    } else if (resp.contains("set startup_clock_correction ")) {
        String str = resp.remove("set startup_clock_correction ");
        bool ok = false;
        double value = str.toDouble(&ok);
        if (ok) {
            p_qsState->setClockCorrection(value);
            setStatusText("Setting startup clock correction to: " + String::number(value));
        } else {
            setStatusText("Value is invalid.");
        }
    } else if (resp.contains("get startup_clock_correction")) {
        setStatusText("Startup clock correction is: " + String::number(p_qsState->clockCorrection()));
    } else if (resp.contains("get dac_bypass")) {
        bool bypass = QsGlobal::g_memory->getDacBypass();
        if (bypass) {
            setStatusText("Dac is bypassed, value: 1 ");
        } else {
            setStatusText("Dac is not bypassed, value: 0 ");
        }
    } else if (resp.contains("reset device")) {
        stopIo();
        sleep.msleep(200);
        setStatusText("Resetting QS1R USB interface...");
        QsGlobal::g_io->resetDevice();
        sleep.msleep(2000);
        initQS1RHardware();
        setStatusText("Reinitializing QS1R Hardware...");
    } else if (resp == "help") {
        ;
        setStatusText("Server Commands:");
        setStatusText("");
        setStatusText("clear - clears the command window");
        setStatusText("errorlog - displays the error log");
        setStatusText("exit - quits the server");
        setStatusText("help - shows the command help");
        setStatusText("hide - hides the server window");
        setStatusText("quit - quits the server");
        setStatusText("reinit - try to initialize hardware");
        setStatusText("qtver - displays the QT version");
        setStatusText("u - frequency up by 500 Hz");
        setStatusText("d - frequency down by 500 Hz");
    } else if (resp == "srl-llc") // factory function enable
    {
        m_is_factory_init_enabled = true;
        m_is_was_factory_init = true;
        ;
        setStatusText(" WARNING! Entering Factory Mode ");
        setStatusText("");
        setStatusText(" 1 - find device ");
        setStatusText(" 2 - load firmware ");
        setStatusText(" 3 - load fpga ");
        setStatusText(" 4 - write eeprom ");
        setStatusText(" 5 - read eeprom ");
        setStatusText(" 6 - write s/n ");
        setStatusText(" 7 - read s/n ");
        setStatusText(" m - display this menu ");
        setStatusText(" done - exit factory mode ");
        setStatusText("");
    } else if (!m_is_factory_init_enabled) // try to run through official command parser
    {
        String cmd_resp = doCommandProcessor(resp, m_local_rx_num_selector);

        if (cmd_resp == "?")
            setStatusText("'" + resp + "'" + " : invalid command");
        else
            setStatusText(cmd_resp);
    }

    if (m_is_factory_init_enabled) {
        if (resp == "1") // find device
        {
            // findQS1RDevice();
        } else if (resp == "2") // load firmware
        {
            loadQS1RFirmware();
        } else if (resp == "3") // load fpga
        {
            loadQS1RFPGA();
        } else if (resp == "4") // write eeprom
        {
            writeQS1REEPROM();
        } else if (resp == "5") // read eeprom
        {
            readQS1REEPROM();
        } else if (resp == "6") // write hardware sn
        {
            bool ok = false;
            _debug() << "Program S/N: Enter UUID";
            std::string str_in;
            std::cin >> str_in;

            if (str_in != "") {
                UUID uuid(str_in);
                writeQS1RSN(String::fromStdString(uuid.toString()));
                setStatusText("UUID Written Successfully");
            }
        } else if (resp == "7") // read s/n
        {
            UUID uuid = readQS1RUuid();
            setStatusText(uuid.toString());
        } else if (resp == "m") // menu
        {
            ;
            setStatusText(" WARNING! Entering Factory Mode ");
            setStatusText("");
            setStatusText(" 1 - find device ");
            setStatusText(" 2 - load firmware ");
            setStatusText(" 3 - load fpga ");
            setStatusText(" 4 - write eeprom ");
            setStatusText(" 5 - read eeprom ");
            setStatusText(" 6 - write s/n ");
            setStatusText(" 7 - read s/n ");
            setStatusText(" m - display this menu ");
            setStatusText(" done - exit factory mode ");
            setStatusText("");
        } else if (resp == "done") // exit factory mode
        {
            m_is_factory_init_enabled = false;
            ;
            setStatusText(" Exiting Factory Mode... ");
        }
    }
}

void QS1RServer::loadQS1RFirmware() {};
void QS1RServer::loadQS1RFPGA() {};
