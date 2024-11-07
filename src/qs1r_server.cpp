#include "../include/qs1r_server.hpp"
#include "../include/qs_audio.hpp"
#include "../include/qs_bitstream.hpp"
#include "../include/qs_bytearray.hpp"
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
#include "../include/qs_scanner.hpp"
#include "../include/qs_signalops.hpp"
#include "../include/qs_sleep.hpp"
#include "../include/qs_state.hpp"
#include "../include/qs_stringclass.hpp"
#include "../include/qs_uuid.hpp"
#include "qs1r_server.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <unistd.h>

QS1RServer::QS1RServer()
    : p_qsState(std::make_unique<QsState>()), p_io_thread(std::make_unique<QsIoThread>()), m_is_fpga_loaded(false),
      m_is_io_setup(false), m_is_factory_init_enabled(false), m_is_was_factory_init(false),
      m_gui_rx1_is_connected(false), m_gui_rx2_is_connected(false), m_driver_type("None"), m_local_rx_num_selector(1),
      m_freq_offset_rx1(0.0), m_freq_offset_rx2(0.0), m_proc_samplerate(50000.0), m_step_size(500.0),
      m_status_message_backing_register(0), m_prev_vol_val(0) {

    QsGlobal::g_server = this;

    p_qsState->init();
    m_is_hardware_init = false;
    QsGlobal::g_is_hardware_init = false;

    initQsMemory();
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

int QS1RServer::initialize() {
    error_flag = false;
    // initQsAudio(QsGlobal::g_memory->getRtAudioRate());
    initQsAudio(50000);
    initSupportedSampleRatesList();
    showStartupMessage();
    initSMeterCorrectionMap();
    initThreads();
    initCircBuffers();
    if (initQS1RHardware() != 0) {
        shutdown();
        return -1;
    }
    updateFPGARegisters();
    setFpgaForSampleRate(50000);
    setDacOutputDisable(false);
    setDacClockSelect(CLK_50k);
    
    _debug() << "Qs1r server initialization complete.";
    return 0;
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

int QS1RServer::initThreads() {
    _debug() << "initializing threads...";
    QsGlobal::g_dsp_proc->init();
    QsGlobal::g_scanner = make_unique<QsScanner>();
    QsGlobal::g_scanner->init();
    return 0;
}

void QS1RServer::initCircBuffers() {
    _debug() << "initializing circular buffers...";
    QsGlobal::g_float_rt_ring->init(QsGlobal::g_memory->getReadBlockSize() * 4);
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

    freopen("/dev/null", "w", stderr);

    QsGlobal::g_audio->stopStream();

    int frames = QsGlobal::g_memory->getRtAudioFrames();
    int out_dev_id = p_qsState->rtAudioOutDevId();
    int in_dev_id = -1;

    bool ok = false;

    QsGlobal::g_audio->initAudio(frames, rate, in_dev_id, out_dev_id, ok);

    if (!ok) {
        setStatusText("Soundcard output init error.");
    }
    freopen("/dev/tty", "w", stderr);
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
        _debug() << "+++Could not find any QS1R devices!+++";
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
                    _debug() << "+++Could not find any QS1R devices!+++";
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

    setWideBandBypass(true);

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
    // bool was_io_running = m_is_io_running;

    // if (was_io_running)
    //     stopIo();

#define SR_OUT0 50000.0
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
    QsGlobal::g_memory->setDataProcRate(m_proc_samplerate);

    SMETERCORRECT = SMETERCORRECTMAP[(int)m_proc_samplerate];

    // setup DDC for proper samplerate
    setDDCSamplerate((int)m_proc_samplerate);

    // set the dac clock select
    if (QsGlobal::g_memory->getResamplerRate() == 48000.0) {
        setDacClockSelect(CLK_48k);
    } else {
        setDacClockSelect(CLK_50k);
    }

    // if (was_io_running) {
    //     setupIo();
    //     startIo();
    // }

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

    bool dac_bypass = false;
    dac_bypass = QsGlobal::g_memory->getDacBypass();

    initThreads();

    setDacOutputDisable(false);

    m_is_io_setup = true;

    _debug() << "-setupIo successful-";
}

// ------------------------------------------------------------
// Starts the DSP processing
// ------------------------------------------------------------
void QS1RServer::startIo(bool iswav) {
    if (QsGlobal::g_dsp_proc->isRunning()) {
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

    QsGlobal::g_audio->startStream();

    // start the dsp processor thread
    if (!QsGlobal::g_dsp_proc->isRunning())
        QsGlobal::g_dsp_proc->start();

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

    _debug() << "stopping dsp processor...";
    if (QsGlobal::g_dsp_proc->isRunning()) {
        QsGlobal::g_dsp_proc->stop();
    }

    QsGlobal::g_dsp_proc->clearBuffers();

    m_is_io_running = false;
}

bool QS1RServer::isDspProcessorRunning() { return m_is_io_running; }

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
// Sets the DAC Clock Rate 0 = 48k, 1 = 50k
// ------------------------------------------------------------
void QS1RServer::setDacClockSelect(DACCLKSEL value) {
    if (!m_is_hardware_init) {
        setStatusText("Error: Please initialize QS1R Hardware first!");
        return;
    }
    unsigned int result = QsGlobal::g_io->readMultibusInt(MB_CONTRL0);
    if (value == CLK_50k) {
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
    QSDEMODMODE mode = QsGlobal::g_memory->getDemodMode(rx_num);

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
        return;
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

void QS1RServer::loadQS1RFirmware() {};
void QS1RServer::loadQS1RFPGA() {};

void QS1RServer::setSquelchOn(bool on) { QsGlobal::g_memory->setSquelchOn(on); }
void QS1RServer::setSquelchThreshold(double threshold) { QsGlobal::g_memory->setSquelchThreshold(threshold); }

void QS1RServer::setVolume(double volume) { QsGlobal::g_memory->setVolume(volume); }
