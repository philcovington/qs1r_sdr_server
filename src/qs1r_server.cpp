#include "../include/qs1r_server.hpp"
#include "../include/qs_bytearray.hpp"
#include "../include/qs_file.hpp"
#include "../include/qs_debugloggerclass.hpp"
#include "../include/qs_listclass.hpp"
#include "../include/qs_audio.hpp"
#include "../include/qs_dac_writer.hpp"
#include "../include/qs_datareader.hpp"
#include "../include/qs_dsp_proc.hpp"
#include "../include/qs_fft.hpp"
#include "../include/qs_filter.hpp"
#include "../include/qs_io_libusb.hpp"
#include "../include/qs_io_thread.hpp"
#include "../include/qs_memory.hpp"
#include "../include/qs_state.hpp"
#include "../include/qs_stringclass.hpp"

QS1RServer::QS1RServer()
    : p_dac_writer(new QsDacWriter()), p_rta(new QsAudio()), p_qsState(new QsState()), p_dsp_proc(new QsDspProcessor()),
      p_io_thread(new QsIoThread()), m_is_fpga_loaded(false), m_is_io_setup(false), m_is_factory_init_enabled(false),
      m_is_was_factory_init(false), m_is_rt_audio_bypass(false), m_gui_rx1_is_connected(false),
      m_gui_rx2_is_connected(false), m_driver_type("None"), m_local_rx_num_selector(1), m_freq_offset_rx1(0.0),
      m_freq_offset_rx2(0.0), m_proc_samplerate(50000.0), m_post_proc_samplerate(50000.0), m_step_size(500.0),
      m_status_message_backing_register(0), m_prev_vol_val(0), gui(NULL), m_wb_bsize(WB_BLOCK_SIZE),
      m_wb_bsizeX2(m_wb_bsize * 2), m_wb_bsizeDiv2(m_wb_bsize / 2), p_wb_fft(new QsFFT()) {

    p_qsState->init();

    initQsMemory();

    QsGlobal::g_server = this;

    p_wb_fft->resize(m_wb_bsize);

    p_comport = 0;

    // delay
    initialize();
}

QS1RServer::~QS1RServer() {}

void QS1RServer::shutdown() {

    if (m_is_io_running) {
        stopIo();
    }

    if (p_io_thread->isRunning()) {
        _debug() << "stopping io thread...";
        p_io_thread->stop();
        p_io_thread->wait(10000);
        p_io_thread->terminate();        
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
    status_string.clear();
    error_string.clear();
    setErrorText("Log initialized");
    error_flag = false;
    initSupportedSampleRatesList();
    initCommandEntryBox();
    showStartupMessage();
    m_is_rt_audio_bypass = QsGlobal::g_memory->getRtAudioBypass();
    boostTicks();
    initCmdSockets();
    initTCPCmdServers();
    initSpectrumDataServer();
    initSMeterCorrectionMap();
    initPSCorrectionMap();
    initAudioSetupDialog();
    initQS1RHardware();
    // initSerialComms( );
}

void QS1RServer::initSupportedSampleRatesList() {

    m_supported_samplerates.clear();
    m_supported_samplerates.append(String("25000"));
    m_supported_samplerates.append(String("50000"));
    m_supported_samplerates.append(String("125000"));
    m_supported_samplerates.append(String("250000"));
    m_supported_samplerates.append(String("500000"));
    m_supported_samplerates.append(String("625000"));
    m_supported_samplerates.append(String("1250000"));
    m_supported_samplerates.append(String("1562500"));
    m_supported_samplerates.append(String("2500000"));
}

void QS1RServer::clearAllBuffers() {
    QsGlobal::g_cpx_ps1_ring->empty();
    QsGlobal::g_cpx_ps2_ring->empty();
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
        m_is_rt_audio_bypass = true;
        setStatusText("Soundcard output is bypassed due to earlier error.");
    }
}

// ------------------------------------------------------------
// Initializes Rx Persistance with qsStateSettings
// ------------------------------------------------------------
void QS1RServer::initQsMemory() {
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
    QsGlobal::g_memory->setRtAudioBypass(p_qsState->rtAudioBypass());
    QsGlobal::g_memory->setDacBypass(p_qsState->dacBypass());
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
void QS1RServer::initQS1RHardware() {
    m_driver_type = "None";

    if (m_is_hardware_init) {
        QsGlobal::g_io->close();
    }

    m_is_hardware_init = false;

    setStatusText("Trying the libusb driver...wait...");

    List<QsDevice> dev_list;
    dev_list.clear();

    if (QsGlobal::g_io->findQsDevice(dev_list) == -1) // try to initialize board
    {
        setStatusText("Error: STAGE1: Could not find QS1R!");
        setErrorText("STAGE1: Could not find QS1R!");
        setStatusText("");
        setStatusText("Type 'setup audio' to configure audio in/out, or");
        setStatusText("Type 'gui' to start the GUI.");
        QsGlobal::g_io->close();
        return;
    } else {
        m_driver_type = "libUSB";
        setStatusText("Using the libusb driver");
        if (dev_list.size() < 1 || QsGlobal::g_io->open(dev_list[0].dev) == -1) {
            setStatusText("Error: STAGE1: Could open QS1R device!");
            setErrorText("STAGE1: Could open QS1R device!");
            setStatusText("");
            setStatusText("Type 'setup audio' to configure audio in/out, or");
            setStatusText("Type 'gui' to start the GUI.");
            return;
        }
    }

    setStatusText("Attempting to load firmware...wait...");

    if (QsGlobal::g_io->loadFirmware(String(RESOURCE_FIRMWARE_FILENAME)) == -1) {
        setStatusText("Error: STAGE1: Could not load QS1R Firmware");
        setErrorText("STAGE1: Could not load QS1R Firmware");
        setStatusText("");
        setStatusText("Type 'setup audio' to configure audio in/out, or");
        setStatusText("Type 'gui' to start the GUI.");
        return;
    } else {
        setStatusText("QS1R Firmware loaded...");
    }

    QsGlobal::g_io->close();
    qssleep.msleep(500);

    int count = 0;

    while (count < LOAD_DELAY_COUNT) {
        dev_list.clear();
        if (QsGlobal::g_io->findQsDevice(dev_list) == -1) // try to initialize board
        {
            count++;
            qssleep.msleep(100);
        } else {
            if (dev_list.size() < 1 || QsGlobal::g_io->open(dev_list[0].dev) == -1) {
                setStatusText("Error: STAGE2: Could not find QS1R!");
                setErrorText("Stage2: Could not find QS1R!");
                setStatusText("");
                setStatusText("Type 'setup audio' to configure audio in/out, or");
                setStatusText("Type 'gui' to start the GUI.");
                return;
            }
            if (QsGlobal::g_io->writeMultibusInt(MB_CONTRL1, 0x0) == -1) // try write to multibus
            {
                count++;
                qssleep.msleep(100);
            } else {
                break;
            }
        }
    }
    if (count >= LOAD_DELAY_COUNT) {
        setStatusText("Error: STAGE3: Could not find QS1R!");
        setErrorText("STAGE3: Could not find QS1R!");
        setStatusText("");
        setStatusText("Type 'setup audio' to configure audio in/out, or");
        setStatusText("Type 'gui' to start the GUI.");

        return;
    }
    m_is_hardware_init = true;
    m_is_was_factory_init = false;

    p_qsState->readSettings();

    setFpgaForSampleRate(p_qsState->startupSampleRate());

    // do a final check of version numbers

    unsigned int id_fx2fw = 0;
    unsigned int id_fpga = 0;

    id_fx2fw = QsGlobal::g_io->readFwSn();
    if (id_fx2fw != ID_FWWR) {
        _debug() << "Incorrect Firmware ID!";
        setStatusText("Incorrect Firmware ID!");
    }

    _debug() << "Firmware ID: " << String::number(id_fx2fw, 10);

    id_fpga = QsGlobal::g_io->readMultibusInt(MB_VERSION_REG);
    if (id_fpga != ID_1RXWR) {
        _debug() << "Incorrect FPGA HDL ID!";
        setStatusText("Incorrect FPGA HDL ID!");
    }

    _debug() << "FPGA ID: " << String::number(id_fpga, 16);

    initWBPowerSpectrum();
    initWBDataServer(); // this needs to be done after qsio is valid ( not NULL )

#ifdef HARDWARE_SN_CHECK
    hardwareSNCheck();
#else
    hardware_is_registered = true;
#endif

    setStatusText("QS1R Hardware: Ready");    
    setStatusText("");
    setStatusText("Type 'setup audio' to configure audio in/out, or");
    setStatusText("Type 'gui' to start the GUI.");
}

bool QS1RServer::hardwareSNCheck() { return hardware_is_registered; }

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
    clearFpgaControlRegisters();

    // do a master reset of DDC in FPGA
    setDdcMasterReset(true);
    setDdcMasterReset(false);

    // Set initial DAC bypass mode

    setDacOutputDisable(p_qsState->dacBypass());

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
        setErrorText("Could not load QS1R FPGA File");
        return;
    }
    // do a master reset of DDC in FPGA
    setDdcMasterReset(true);
    setDdcMasterReset(false);
    m_is_fpga_loaded = true;
}

// ------------------------------------------------------------
// Manages the status window and keeps focus to the command
// entry box.
// ------------------------------------------------------------
void QS1RServer::setStatusText(String text) {
    status_string.append(text);
    _debug() << status_string;
}

// ------------------------------------------------------------
// Manages the error log
// ------------------------------------------------------------
void QS1RServer::setErrorText(String text) {
    error_flag = true;
    error_string.append(text);
}

// ------------------------------------------------------------
// Clears the error log
// ------------------------------------------------------------
void QS1RServer::clearErrorText() { error_string.clear(); }

// ------------------------------------------------------------
// Displays the server startup message
// ------------------------------------------------------------
void QS1RServer::showStartupMessage() {
    clearStatusText();
    setStatusText("-------------------------------------------------------------------------");
    setStatusText("SRL-LLC SDRMAXV Ver " + String(SDRMAXV_VERSION));
    setStatusText("Copyright 2012 Software Radio Laboratory LLC");
    setStatusText("-------------------------------------------------------------------------");
}

// ------------------------------------------------------------
// Displays the server startup message with ready
// ------------------------------------------------------------
void QS1RServer::showStartupMessageWithReady() {
    clearStatusText();
    setStatusText("-------------------------------------------------------------------------");
    setStatusText("SRL-LLC SDRMAXV Ver " + String(SDRMAXV_VERSION));
    setStatusText("Copyright 2012 Software Radio Laboratory LLC");
    setStatusText("-------------------------------------------------------------------------");
    setStatusText("QS1R Ready");
}

// ------------------------------------------------------------
// Quits the server application
// ------------------------------------------------------------
void QS1RServer::quit() {}

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

    connect(&(*QsGlobal::g_data_reader), SIGNAL(onQs1rReadFail()), this, SLOT(qs1rReadFailure()));

    bool dac_bypass = false;
    dac_bypass = QsGlobal::g_memory->getDacBypass();

    p_dsp_proc->init(rx_num);

    p_dac_writer->init();

    if (dac_bypass) {
        setDacOutputDisable(true);
    }

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
        setErrorText("Error on io setup!");
        return;
    }

    // do a master reset of DDC in FPGA
    setDdcMasterReset(true);
    setDdcMasterReset(false);

    // start the dsp processor thread
    if (!p_dsp_proc->isRunning())
        p_dsp_proc->start(Thread::TimeCriticalPriority);

    QsGlobal::g_data_reader->setSource(src_qs1r);

    if (!QsGlobal::g_data_reader->isRunning())
        QsGlobal::g_data_reader->start(Thread::TimeCriticalPriority);

    bool dac_bypass = false;
    dac_bypass = p_qsState->dacBypass();

    if (!dac_bypass) {
        if (!p_dac_writer->isRunning())
            p_dac_writer->start(Thread::NormalPriority);
    }

    if (!m_is_rt_audio_bypass) {
        initQsAudio(QsGlobal::g_memory->getResamplerRate());
        p_rta->startStream();
        QsGlobal::g_float_rt_ring->empty();
    }

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

    _debug() << "stopping rt audio...";
    if (!m_is_rt_audio_bypass) {
        p_rta->stopStream();
    }

    _debug() << "stopping dac writer...";
    if (p_dac_writer->isRunning()) {
        p_dac_writer->stop();
        p_dac_writer->wait(10000);
    }

    _debug() << "stopping dsp processor...";
    if (p_dsp_proc->isRunning()) {
        p_dsp_proc->stop();
        p_dsp_proc->wait(10000);
    }

    _debug() << "stopping data reader...";
    if (QsGlobal::g_data_reader->isRunning()) {
        QsGlobal::g_data_reader->stop();
        QsGlobal::g_data_reader->wait(10000);
    }

    _debug() << "stopping wav writer...";

    QsGlobal::g_data_reader->clearBuffers();
    p_dsp_proc->clearBuffers();

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
        setErrorText("Please initialize QS1R Hardware first!");
        return;
    }
    QsGlobal::g_io->writeMultibusInt(MB_CONTRL0, 0);
    QsGlobal::g_io->writeMultibusInt(MB_CONTRL1, 0);
}

// ------------------------------------------------------------
// Sets/Clears the ADC PGA mode
// ------------------------------------------------------------
void QS1RServer::setPgaMode(bool on) {
    if (!m_is_hardware_init) {
        setStatusText("Error: Please initialize QS1R Hardware first!");
        setErrorText("Please initialize QS1R Hardware first!");
        return;
    }
    unsigned int result = QsGlobal::g_io->readMultibusInt(MB_CONTRL1);
    if (on) {
        result |= PGA;
    } else {
        result &= ~PGA;
    }

    QsGlobal::g_io->writeMultibusInt(MB_CONTRL1, result);

    p_qsState->setPGA(on);
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
void QS1RServer::setRandMode(bool on) {
    if (!m_is_hardware_init) {
        setStatusText("Error: Please initialize QS1R Hardware first!");
        setErrorText("Please initialize QS1R Hardware first!");
        return;
    }
    unsigned int result = QsGlobal::g_io->readMultibusInt(MB_CONTRL1);
    if (on) {
        result |= RANDOM;
    } else {
        result &= ~RANDOM;
    }
    QsGlobal::g_io->writeMultibusInt(MB_CONTRL1, result);

    p_qsState->setRAND(on);
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
void QS1RServer::setDitherMode(bool on) {
    if (!m_is_hardware_init) {
        setStatusText("Error: Please initialize QS1R Hardware first!");
        setErrorText("Please initialize QS1R Hardware first!");
        return;
    }
    unsigned int result = QsGlobal::g_io->readMultibusInt(MB_CONTRL1);
    if (on) {
        result |= DITHER;
    } else {
        result &= ~DITHER;
    }
    QsGlobal::g_io->writeMultibusInt(MB_CONTRL1, result);

    p_qsState->setDITH(on);
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
        setErrorText("Please initialize QS1R Hardware first!");
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
        setErrorText("Please initialize QS1R Hardware first!");
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
        setErrorText("Please initialize QS1R Hardware first!");
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
        setErrorText("Please initialize QS1R Hardware first!");
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
        setErrorText("Please initialize QS1R Hardware first!");
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
        setErrorText("Please initialize QS1R Hardware first!");
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
        setErrorText("Please initialize QS1R Hardware first!");
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
        setErrorText("Please initialize QS1R Hardware first!");
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
void QS1RServer::qs1rReadFailure() {
    setStatusText("QS1R Read Failure!");
    this->show();
}

// ------------------------------------------------------------
// Manufacture and Test Functions
// ------------------------------------------------------------
void QS1RServer::findQS1RDevice() {
    setStatusText("Looking for QS1R hardware on USB...");

    setStatusText("Trying libusb driver...");

    QsGlobal::g_io->close();

    QList<QsDevice> dev_list;
    dev_list.clear();

    if (QsGlobal::g_io->findQsDevice(dev_list) == -1) // try to initialize board
    {
        setStatusText("Error: Could not find QS1R! Make sure QS1R driver is installed and QS1R is powered on");
        setErrorText("Could not find QS1R! Make sure QS1R driver is installed and QS1R is powered on");
        QsGlobal::g_io->close();
        return;
    } else {
        if (QsGlobal::g_io->open(dev_list[0].dev) == -1) {
            setStatusText("Could not open QS1R device.");
            return;
        } else {
            setStatusText("Using libusb driver.");
        }
    }
    setStatusText("Found QS1R on USB...");
}

void QS1RServer::loadQS1RFirmware() {
    setStatusText("Attempting to load Firmware...");

    if (QsGlobal::g_io->loadFirmware(String(RESOURCE_FIRMWARE_FILENAME)) == -1) {
        setStatusText("Error: Could not load QS1R Firmware");
        setErrorText("Could not load QS1R Firmware");
        return;
    }

    setStatusText("QS1R Firmware loaded successfully.");
}

void QS1RServer::loadQS1RFPGA() {
    setStatusText("Attempting to load FPGA...");

    if (QsGlobal::g_io->loadFpga(String(RESOURCE_FPGA_MASTER)) == -1) {
        setStatusText("Error: Could not load QS1R FPGA File");
        setErrorText("Could not load QS1R FPGA File");
        return;
    }

    m_is_fpga_loaded = true;

    setStatusText("QS1R FPGA loaded successfully.");
}

QUuid QS1RServer::readQS1RUuid() {
    QUuid uuid;
    QByteArray buffer;
    buffer.resize(16);
    buffer.fill(0);

    if (QsGlobal::g_io->readEEPROM(QS1R_EEPROM_ADDR, 16, (unsigned char *)buffer.data(), 16)) {
        QDataStream in(&buffer, QIODevice::ReadOnly);
        in.setVersion(QDataStream::Qt_4_7);
        in >> uuid;
    } else {
        setStatusText("Failure reading serial number.");
        setErrorText("Failure reading serial number.");
    }
    return uuid;
}

String QS1RServer::readQSLSFile() {
    QUuid uuid;
    String file_name = QDir::currentPath() + "/qsls.qsn";
    QFile f(file_name);
    QDataStream in(&f);
    in.setVersion(QDataStream::Qt_4_7);

    QDateTime dt; // dummies
    String name;  // dummies
    String sn;    // dummies

    if (f.open(QIODevice::ReadOnly)) {
        in >> dt >> name >> sn >> uuid;
        f.close();
        return String("Date: %1, Name: %2, SN: %3, UUID: %4").arg(dt.toString()).arg(name).arg(sn).arg(uuid.toString());
    } else {
        f.close();
        return "Registration file does not exist";
    }
}

bool QS1RServer::updateQS1RSN() {
    QUuid uuid;
    String file_name = QDir::currentPath() + "/qsls.qsn";
    QFile f(file_name);
    QDataStream in(&f);
    in.setVersion(QDataStream::Qt_4_7);

    QDateTime dt; // dummies
    String name;  // dummies
    String sn;    // dummies

    if (f.open(QIODevice::ReadOnly)) {
        in >> dt >> name >> sn >> uuid;
        f.close();
        if (writeQS1RSN(uuid)) {
            setStatusText("Registering QS1R to: " + name);
            return true;
        } else {
            return false;
        }
    } else {
        setStatusText("Could find serial number file");
        setErrorText("Could find serial number file");
        f.close();
        return false;
    }
}

bool QS1RServer::validateQS1RSN(QUuid uuid) {
    if (uuid.isNull())
        return false;

    for (int i = 0; i < QS1R_GUID_COUNT; i++) {
        if (uuid == qs1r_guids[i]) {
            return true;
            break;
        }
    }
    return false;
}

bool QS1RServer::writeQS1RSN(QUuid uuid) {
    QByteArray buffer;
    buffer.resize(16);
    buffer.fill(0);

    QDataStream out(&buffer, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_7);

    out << uuid;

    if (QsGlobal::g_io->writeEEPROM(QS1R_EEPROM_ADDR, 16, (unsigned char *)buffer.data(), 16)) {
        setStatusText("Updated Serial Number");
        return true;
    } else {
        setStatusText("Could not update serial number");
        setErrorText("Could not update serial number");
        return false;
    }
}

void QS1RServer::unregisteredHardwareTimeout() {
    if (!hardware_is_registered) {
        setStatusText("Server is shutting down...");
        setErrorText("Server is shutting down...");
        QTimer::singleShot(5000, this, SLOT(quit()));
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
        setErrorText("Failure updating EEPROM.");
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
        eeprom_data_str.append("}");
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
                str.append(":");
            }
        }
        setStatusText(str);
        setStatusText("EEPROM read successful.");
    } else {
        setStatusText("Failure reading EEPROM.");
    }
}

// ------------------------------------------------------------
// Wideband Power Spectrum
// ------------------------------------------------------------
void QS1RServer::initWBPowerSpectrum() {
    wb_window.resize(m_wb_bsize);
    std::fill(wb_window.begin(), wb_window.end(), cpx_one);

    QsFilter::MakeWindow(12, m_wb_bsize, wb_window);

    wb_cpx_buf1.resize(m_wb_bsize);
    std::fill(wb_cpx_buf1.begin(), wb_cpx_buf1.end(), cpx_zero);

    wb_cpx_buf2.resize(m_wb_bsize);
    std::fill(wb_cpx_buf2.begin(), wb_cpx_buf2.end(), cpx_zero);

    wb_f_buf1.resize(m_wb_bsizeDiv2);
    std::fill(wb_f_buf1.begin(), wb_f_buf1.end(), 0.0);

    wb_adc_data.resize(m_wb_bsize);
    std::fill(wb_adc_data.begin(), wb_adc_data.end(), 0);

    wb_adc_f.resize(m_wb_bsizeX2);
    std::fill(wb_adc_f.begin(), wb_adc_f.end(), 0.0);

    wb_adc_cpx.resize(m_wb_bsizeX2);
    std::fill(wb_adc_cpx.begin(), wb_adc_cpx.end(), cpx_zero);

    wb_fvPsdBm.resize(m_wb_bsizeDiv2);
    wb_fvPsdBm.fill(0.0);

    wb_mean = 0;
    m_wb_one_over_norm = 1.0 / m_wb_bsizeDiv2;

    p_wb_fft->resize(m_wb_bsize);
}

std::vector<float> QS1RServer::getWBPowerSpectrum() {
    int count = QsGlobal::g_io->readEP8(reinterpret_cast<unsigned char *>(&wb_adc_data[0]), m_wb_bsize * sizeof(short));

    if (count == m_wb_bsizeX2) {
        // convert to floats
        QsSpl::Convert(wb_adc_data, wb_adc_f, m_wb_bsize);

        // find mean for dc removal
        QsSpl::Mean(wb_adc_f, wb_mean, m_wb_bsize);

        // subtract mean
        QsSpl::Subtract(wb_adc_f, wb_mean, m_wb_bsize);

        // Real to Complex
        QsSpl::RealToComplex(wb_adc_f, wb_adc_f, wb_adc_cpx, m_wb_bsize);

        // window the block
        QsSpl::Multiply(wb_adc_cpx, wb_window, wb_cpx_buf1, m_wb_bsizeDiv2);

        // fwd fft
        p_wb_fft->doDFTForward(wb_cpx_buf1, wb_cpx_buf2, m_wb_bsizeDiv2, m_wb_one_over_norm);

        // get 10log10 magnitude squared
        QsSpl::x10Log10PowerSpectrum(wb_cpx_buf2, wb_f_buf1, m_wb_bsizeDiv2);

        // add correction
        m_wb_correction = QsGlobal::g_memory->getWBPsCorrection();
        QsSpl::Add(wb_f_buf1, static_cast<float>(m_wb_correction + QS_DEFAULT_WB_PS_CORRECT), m_wb_bsizeDiv2);

        // copy data into vector
        wb_fvPsdBm = std::vector<float>::fromStdVector(wb_f_buf1);
    } else {
        wb_fvPsdBm.resize(0);
        wb_fvPsdBm.fill(-170.0);
    }
    return wb_fvPsdBm;
}

// ------------------------------------------------------------
// ************************************************************
// THIS IS THE MAIN COMMAND PROCESSOR
// Please keep it at the end of this file to make it easier
// to find.
// ************************************************************
// ------------------------------------------------------------
// ------------------------------------------------------------
// Runs the received command trough the command parser
// and attempts to execute the command if valid.
// Returns "?" if the command was invalid.
// ------------------------------------------------------------
String QS1RServer::doCommandProcessor(String value, int rx_num, QAbstractSocket *socket) {
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

    QHostAddress addr = socket->peerAddress();
    quint16 port = socket->localPort();

    if (false) {
    }

    //****************************************************//
    //----------------------A-----------------------------//
    //****************************************************//

    else if (cmd.cmd.compare("AgcDecaySpeed") == 0) // sets agc decay speed
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setAgcDecaySpeed(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double agc_decay = QsGlobal::g_memory->getAgcDecaySpeed(rx_num - 1);
            response = cmd.cmd.append("=") + String::number(agc_decay);
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
            double gain = QsGlobal::g_memory->getAgcFixedGain(rx_num - 1);
            response = cmd.cmd.append("=") + String::number(gain);
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
            double gain = QsGlobal::g_memory->getAgcThreshold(rx_num - 1);
            response = cmd.cmd.append("=") + String::number(gain);
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
            double slope = QsGlobal::g_memory->getAgcSlope(rx_num - 1);
            response = cmd.cmd.append("=") + String::number(slope);
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
            double ht = QsGlobal::g_memory->getAgcHangTime(rx_num - 1);
            response = cmd.cmd.append("=") + String::number(ht);
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
            bool sw = QsGlobal::g_memory->getAgcHangTimeSwitch(rx_num - 1);
            response = cmd.cmd.append("=") + String::number(sw);
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
            response = cmd.cmd.append("=") + String::number(value);
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
            response = cmd.cmd.append("=") + String::number(value);
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
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    else if (cmd.cmd.compare("AutoNotchRate") == 0) // sets autonotch rate
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setAutoNotchRate(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getAutoNotchRate(rx_num - 1);
            response = cmd.cmd.append("=") + String::number(value);
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
            response = cmd.cmd.append("=") + String::number(value);
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
            response = cmd.cmd.append("=") + String::number(value);
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
            response = cmd.cmd.append("=") + String::number(value);
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
            response = cmd.cmd.append("=") + String::number((int)ditherMode());
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
    else if (cmd.cmd.compare("DisplayFreqOffset") == 0 ||
             cmd.cmd.compare("dfo") == 0) //  display offset freq
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setDisplayFreqOffset(cmd.dvalue, 0);
            QsGlobal::g_server->setRxFrequency(QsGlobal::g_memory->getRxLOFrequency(), 1, true);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getDisplayFreqOffset(rx_num - 1);
            getDisplayFreqOffset(value, rx_num);
            response = cmd.cmd.append("=") + String::number(value);
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
            qssleep.msleep(500);
            quit();
        } else if (cmd.RW == CMD::cmd_read) {
            response = "NAK";
        }
    }

    //
    // EncodeClockCorrection d, d = ...
    //
    else if (cmd.cmd.compare("EncodeClockCorrection") ==
             0) // set encode clock frequency correction in Hz
    {
        if (cmd.RW == CMD::cmd_write) {
            setFreqCorrection(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getEncodeFreqCorrect();
            getFreqCorrection(value);
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //****************************************************//
    //----------------------F-----------------------------//
    //****************************************************//

    //
    // Freq d, d = 0.0 to 62.5e6 Hz
    //
    else if (cmd.cmd.compare("Freq") == 0 ||
             cmd.cmd.compare("fHz") == 0) // set frequency in Hz
    {
        if (cmd.RW == CMD::cmd_write) {
            setRxFrequency(cmd.dvalue, rx_num);
            response = "OK";
            if (port != RX1_GUI_CMD_TCP_PORT && port != RX2_GUI_CMD_TCP_PORT) {
                String gui_update = "fhz=" + String::number(cmd.dvalue) + "\n";
                sendGUIUpdate(gui_update, rx_num);
            }
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getRxLOFrequency(rx_num - 1);
            getRxFrequency(value, rx_num);
            response = cmd.cmd.append("=") + String::number(value, 'f', 0);
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
            if (port != RX1_GUI_CMD_TCP_PORT && port != RX2_GUI_CMD_TCP_PORT) {
                String gui_update = "fhz=" + String::number(cmd.dvalue * 1.0e3) + "\n";
                sendGUIUpdate(gui_update, rx_num);
            }
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getRxLOFrequency(rx_num - 1);
            getRxFrequency(value, rx_num);
            response = cmd.cmd.append("=") + String::number(value / 1.0e3, 'f', 3);
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
            if (port != RX1_GUI_CMD_TCP_PORT && port != RX2_GUI_CMD_TCP_PORT) {
                String gui_update = "fhz=" + String::number(cmd.dvalue * 1.0e6) + "\n";
                sendGUIUpdate(gui_update, rx_num);
            }
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getRxLOFrequency(rx_num - 1);
            getRxFrequency(value, rx_num);
            response = cmd.cmd.append("=") + String::number(value / 1.0e6, 'f', 6);
        }
    }

    //
    // FilterLow d, d = 0 to 20000.0
    //
    else if (cmd.cmd.compare("FilterLow") == 0 ||
             cmd.cmd.compare("fl") == 0) // filter low value
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setFilterLo(cmd.ivalue);
            response = "OK";
            if (port != RX1_GUI_CMD_TCP_PORT && port != RX2_GUI_CMD_TCP_PORT) {
                String gui_update = "fl=" + String::number(cmd.ivalue) + "\n";
                sendGUIUpdate(gui_update, rx_num);
            }
        } else if (cmd.RW == CMD::cmd_read) {
            int value = QsGlobal::g_memory->getFilterLo(rx_num - 1);
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //
    // FilterHigh d, d = 0 to 20000.0
    //
    else if (cmd.cmd.compare("FilterHigh") == 0 ||
             cmd.cmd.compare("fh") == 0) // filter hi value
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setFilterHi(cmd.ivalue);
            response = "OK";
            if (port != RX1_GUI_CMD_TCP_PORT && port != RX2_GUI_CMD_TCP_PORT) {
                String gui_update = "fh=" + String::number(cmd.ivalue) + "\n";
                sendGUIUpdate(gui_update, rx_num);
            }
        } else if (cmd.RW == CMD::cmd_read) {
            int value = QsGlobal::g_memory->getFilterHi(rx_num - 1);
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //
    // FilterSet d,d d = 0 to 20000.0
    //
    else if (cmd.cmd.compare("Filter") == 0) // filter value
    {
        if (cmd.RW == CMD::cmd_write) {
            if (cmd.slist.count() == 2) {
                int flo = cmd.slist[0].toInt();
                int fhi = cmd.slist[1].toInt();
                QsGlobal::g_memory->setFilterHi(fhi);
                QsGlobal::g_memory->setFilterLo(flo);
                response = "OK";
                if (port != RX1_GUI_CMD_TCP_PORT && port != RX2_GUI_CMD_TCP_PORT) {
                    String gui_update = "filter=" + String::number(flo) + ", " + String::number(fhi) + "\n";
                    sendGUIUpdate(gui_update, rx_num);
                }
            }
        } else if (cmd.RW == CMD::cmd_read) {
            int value1 = QsGlobal::g_memory->getFilterLo(rx_num - 1);
            int value2 = QsGlobal::g_memory->getFilterHi(rx_num - 1);
            response = String("%1, %2;").arg((int)(value1)).arg((int)(value2));
            response.prepend(cmd.cmd.append("="));
        }
    }

    //
    // FilterLowTx d, d = 0 to 20000.0
    //
    else if (cmd.cmd.compare("FilterLowTx") == 0 ||
             cmd.cmd.compare("fltx") == 0) // tx filter low value
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setTxFilterLo(cmd.ivalue);
            response = "OK";
            if (port != RX1_GUI_CMD_TCP_PORT && port != RX2_GUI_CMD_TCP_PORT) {
                String gui_update = "fltx=" + String::number(cmd.ivalue) + "\n";
                sendGUIUpdate(gui_update, rx_num);
            }
        } else if (cmd.RW == CMD::cmd_read) {
            int value = QsGlobal::g_memory->getTxFilterLo();
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //
    // FilterHighTx d, d = 0 to 20000.0
    //
    else if (cmd.cmd.compare("FilterTxHigh") == 0 ||
             cmd.cmd.compare("fhtx") == 0) // txfilter hi value
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setTxFilterHi(cmd.ivalue);
            response = "OK";
            if (port != RX1_GUI_CMD_TCP_PORT && port != RX2_GUI_CMD_TCP_PORT) {
                String gui_update = "fhtx=" + String::number(cmd.ivalue) + "\n";
                sendGUIUpdate(gui_update, rx_num);
            }
        } else if (cmd.RW == CMD::cmd_read) {
            int value = QsGlobal::g_memory->getTxFilterHi();
            response = cmd.cmd.append("=") + String::number(value);
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
            response = cmd.cmd.append("=") + String::number(value);
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
            response = cmd.cmd.append("=") + String::number(value, 16);
        }
    }

    //****************************************************//
    //----------------------G-----------------------------//
    //****************************************************//

    //
    // GUIUpdateHostAddress  Note: should only be used by GUI for update notifications
    //
    else if (cmd.cmd.compare("__gui_update_ip_adddress__") == 0) // GUI Update Host Address
    {
        if (cmd.RW == CMD::cmd_write) {
            if (port == RX1_GUI_CMD_TCP_PORT) {
                gui_update_rx1_host_address = addr;
                response = "OK";
            } else if (port == RX2_GUI_CMD_TCP_PORT) {
                gui_update_rx2_host_address = addr;
                response = "OK";
            } else
                response = "NAK";
        } else if (cmd.RW == CMD::cmd_read) {
            if (port == RX1_GUI_CMD_TCP_PORT) {
                response = cmd.cmd.append("=") + gui_update_rx1_host_address.toString();
            } else if (port == RX1_GUI_CMD_TCP_PORT) {
                response = cmd.cmd.append("=") + gui_update_rx2_host_address.toString();
            } else
                response = "NAK";
        }
    }

    //****************************************************//
    //----------------------H-----------------------------//
    //****************************************************//

    //
    // Hide
    //
    else if (cmd.cmd.compare("Hide") == 0) // hides the server window
    {
        if (cmd.RW == CMD::cmd_write) {
            hideWindow();
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            if (this->isVisible())
                response = "0";
            else
                response = "1";
        }
    }

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
            response = cmd.cmd.append("=") + String::number(m_is_hardware_init);
        }
    }

    //****************************************************//
    //----------------------L-----------------------------//
    //****************************************************//

    //
    // logout
    //
    else if (cmd.cmd.compare("logout") == 0 ||
             cmd.cmd.compare("logoff") == 0) // logout
    {
        if (cmd.RW == CMD::cmd_write) {
            socket->write("GOODBYE\n");
            socket->disconnectFromHost();
            response = "\0";
        } else if (cmd.RW == CMD::cmd_read) {
            response = "NAK";
        }
    }

    //****************************************************//
    //----------------------M-----------------------------//
    //****************************************************//

    //
    // Mode n, n = QSDEMODMODE enum
    //
    else if (cmd.cmd.compare("Mode") == 0) // get demod mode
    {
        if (cmd.RW == CMD::cmd_write) {
            setModeDirect(cmd.ivalue);
            response = "OK";
            if (port != RX1_GUI_CMD_TCP_PORT && port != RX2_GUI_CMD_TCP_PORT) {
                String gui_update = "mode=" + String::number(cmd.ivalue) + "\n";
                sendGUIUpdate(gui_update, rx_num);
            }
        } else if (cmd.RW == CMD::cmd_read) {
            QSDEMODMODE value = QsGlobal::g_memory->getDemodMode(rx_num - 1);
            response = cmd.cmd.append("=") + String::number(value);
        }
    }
    //
    // ModeStr n, n = QSDEMODMODE enum
    //
    else if (cmd.cmd.compare("ModeStr") == 0) // get demod mode
    {
        if (cmd.RW == CMD::cmd_write) {

            int value = (int)modeStringToMode(cmd.svalue);
            setModeDirect(value);
            response = "OK";
            if (port != RX1_GUI_CMD_TCP_PORT && port != RX2_GUI_CMD_TCP_PORT) {
                String gui_update = "mode=" + String::number(value) + "\n";
                sendGUIUpdate(gui_update, rx_num);
            }
        } else if (cmd.RW == CMD::cmd_read) {
            QSDEMODMODE value = QsGlobal::g_memory->getDemodMode(rx_num - 1);
            String mode_str = getModeString(value);
            response = cmd.cmd.append("=") + mode_str;
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
            if (port != RX1_GUI_CMD_TCP_PORT && port != RX2_GUI_CMD_TCP_PORT) {
                String gui_update =
                    "mflh=" + String::number(dmAM) + ", " + String::number(fl) + ", " + String::number(fh) + "\n";
                sendGUIUpdate(gui_update, rx_num);
            }
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
            if (port != RX1_GUI_CMD_TCP_PORT && port != RX2_GUI_CMD_TCP_PORT) {
                String gui_update =
                    "mflh=" + String::number(dmSAM) + ", " + String::number(fl) + ", " + String::number(fh) + "\n";
                sendGUIUpdate(gui_update, rx_num);
            }
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
            if (port != RX1_GUI_CMD_TCP_PORT && port != RX2_GUI_CMD_TCP_PORT) {
                String gui_update =
                    "mflh=" + String::number(dmLSB) + ", " + String::number(fl) + ", " + String::number(fh) + "\n";
                sendGUIUpdate(gui_update, rx_num);
            }
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
            if (port != RX1_GUI_CMD_TCP_PORT && port != RX2_GUI_CMD_TCP_PORT) {
                String gui_update =
                    "mflh=" + String::number(dmUSB) + ", " + String::number(fl) + ", " + String::number(fh) + "\n";
                sendGUIUpdate(gui_update, rx_num);
            }
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
            if (port != RX1_GUI_CMD_TCP_PORT && port != RX2_GUI_CMD_TCP_PORT) {
                String gui_update =
                    "mflh=" + String::number(dmDSB) + ", " + String::number(fl) + ", " + String::number(fh) + "\n";
                sendGUIUpdate(gui_update, rx_num);
            }
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
            if (port != RX1_GUI_CMD_TCP_PORT && port != RX2_GUI_CMD_TCP_PORT) {
                String gui_update =
                    "mflh=" + String::number(dmCW) + ", " + String::number(fl) + ", " + String::number(fh) + "\n";
                sendGUIUpdate(gui_update, rx_num);
            }
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
            if (port != RX1_GUI_CMD_TCP_PORT && port != RX2_GUI_CMD_TCP_PORT) {
                String gui_update = "mute=" + String::number(cmd.ivalue) + "\n";
                sendGUIUpdate(gui_update, rx_num);
            }
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
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    else if (cmd.cmd.compare("NoiseReductionRate") == 0) // sets noise reduction rate
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setNoiseReductionRate(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getNoiseReductionRate(rx_num - 1);
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //****************************************************//
    //----------------------O-----------------------------//
    //****************************************************//

    //
    // OffsetGenFreq d, n = -20000.0 to 20000.0
    //
    else if (cmd.cmd.compare("OffsetGenFreq") == 0 ||
             cmd.cmd.compare("ogf") == 0) //  oscillator offset freq
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setOffsetGeneratorFrequency(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getOffsetGeneratorFrequency(rx_num - 1);
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //****************************************************//
    //----------------------P-----------------------------//
    //****************************************************//

    //
    // ping
    //
    else if (cmd.cmd.compare("ping") == 0) // ping from gui
    {
        if (cmd.RW == CMD::cmd_write) {
            _debug() << "Received ping at: " << QDateTime::currentDateTime().toString();
            response = "OK";
        }
    }

    //
    // PgaSwitch n, n = 0,1
    //
    else if (cmd.cmd.compare("PgaSwitch") == 0) // turn off/on adc pga
    {
        if (cmd.RW == CMD::cmd_write) {
            setPgaMode((bool)cmd.ivalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            response = cmd.cmd.append("=") + String::number((int)pgaMode());
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
            response = cmd.cmd.append("=") + String::number(value);
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
            response = cmd.cmd.append("=") + String::number(value);
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
            response = cmd.cmd.append("=") + String::number((int)randMode());
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
            QUuid uuid = readQS1RUuid();
            response = cmd.cmd.append("=") + uuid.toString();
        }
    }

    //
    // ReadQSLSFile
    //
    else if (cmd.cmd.compare("_ReadQSLSFile_") == 0) // read qs1r serial number
    {
        if (cmd.RW == CMD::cmd_write) {
            response = "NAK";
        } else if (cmd.RW == CMD::cmd_read) {
            response = cmd.cmd.append("=") + readQSLSFile();
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
            response = cmd.cmd.append("=") + String::number(m_is_io_running);
        }
    }

    //
    // StartWavInput
    //
    else if (cmd.cmd.compare("StartWavInput") == 0) // start wav input mode
    {
        if (cmd.RW == CMD::cmd_write) {
            startIo(true);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            response = cmd.cmd.append("=") + String::number(is_wav_in);
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
            response = cmd.cmd.append("=") + mode_str + "," + String::number(dvalue, 'f', 0) + "," +
                       String::number(tvalue, 'f', 0) + ";";
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
            response = cmd.cmd.append("=") + String::number(!m_is_io_running);
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
            response = cmd.cmd.append("=") + String::number(QsGlobal::g_memory->getDataProcRate());
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
            response = cmd.cmd.append("=") + list.join(", ");
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
            response = cmd.cmd.append("=") + String::number(value);
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
            response = cmd.cmd.append("=") + String::number(value);
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
            response = cmd.cmd.append("=") + String::number(value);
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
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //
    // Show
    //
    else if (cmd.cmd.compare("Show") == 0) // shows the server window
    {
        if (cmd.RW == CMD::cmd_write) {
            showWindow();
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            if (this->isVisible())
                response = "1";
            else
                response = "0";
        }
    }

    else if (cmd.cmd.compare("serverpid") == 0) {
        if (cmd.RW == CMD::cmd_write) {
            response = "NAK";
        } else if (cmd.RW == CMD::cmd_read) {
            response = cmd.cmd.append("=") + String::number(qApp->applicationPid());
        }
    }

    //****************************************************//
    //----------------------T-----------------------------//
    //****************************************************//

    //
    // ToneFrequency d, n = -20000.0 to 20000.0
    //
    else if (cmd.cmd.compare("ToneFrequency") == 0 ||
             cmd.cmd.compare("tf") == 0) // tone oscillator tone freq
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setToneLoFrequency(cmd.dvalue);
            response = "OK";
            if (port != RX1_GUI_CMD_TCP_PORT && port != RX2_GUI_CMD_TCP_PORT) {
                String gui_update = "tf=" + String::number(cmd.dvalue) + "\n";
                sendGUIUpdate(gui_update, rx_num);
            }
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getToneLoFrequency(rx_num - 1);
            response = cmd.cmd.append("=") + String::number(value);
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
            response = cmd.cmd.append("=") + String(SDRMAXV_VERSION);
            // setStatusText( "version request made" );
        }
    }

    //
    // Vol d, d = -120 to 0
    //
    else if (cmd.cmd.compare("Vol") == 0 ||
             cmd.cmd.compare("v") == 0) // rx volume
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setVolume(qBound(-120.0, cmd.dvalue, 0.0));
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getVolume(rx_num - 1);
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //****************************************************//
    //----------------------W-----------------------------//
    //****************************************************//

    //
    // WBCorrection d, d = -120.0 to +120.0
    //
    else if (cmd.cmd.compare("WBCorrection") == 0) // set wb power spectrum correction in dbm
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setWBPsCorrection(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getWBPsCorrection();
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //
    // WavRecord n, n=0,1
    //
    else if (cmd.cmd.compare("WavRecord") == 0) // starts or stops wav file recording
    {
        if (cmd.RW == CMD::cmd_write) {
            p_wav_writer->setRecordingOn((bool)cmd.ivalue, rx_num);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            bool value = false;
            p_wav_writer->getRecordingOn(value, rx_num);
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //
    // WavContinuous n, n=0,1
    //
    else if (cmd.cmd.compare("WavContinuous") == 0) // sets wav file continuous recording mode
    {
        if (cmd.RW == CMD::cmd_write) {
            p_wav_writer->setContinuousRecordOn((bool)cmd.ivalue, rx_num);
            QsGlobal::g_memory->setWavRecContinuous((bool)cmd.ivalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            bool value = QsGlobal::g_memory->getWavRecContinuous();
            p_wav_writer->getContinuousRecordOn(value, rx_num);
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //
    // WavPreBuffering n, n=0,1
    //
    else if (cmd.cmd.compare("WavPreBuffering") == 0) // enables wav file prebuffering
    {
        if (cmd.RW == CMD::cmd_write) {
            p_wav_writer->setPrebufferingOn((bool)cmd.ivalue, rx_num);
            QsGlobal::g_memory->setWavRecordPrebufferOn((bool)cmd.ivalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            bool value = QsGlobal::g_memory->getWavRecordPrebufferOn();
            p_wav_writer->getPrebufferingOn(value, rx_num);
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //
    // WavPreBufferTime n, n=0,1,...
    //
    else if (cmd.cmd.compare("WavPreBufferTime") == 0) // wav file prebuffering time
    {
        if (cmd.RW == CMD::cmd_write) {
            p_wav_writer->setPrebufferTime(cmd.dvalue, rx_num);
            QsGlobal::g_memory->setWavRecordPrebufferTime(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_memory->getWavRecordPrebufferTime();
            p_wav_writer->getPrebufferTime(value, rx_num);
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //
    // WavRecordPath s, s = path
    //
    else if (cmd.cmd.compare("WavRecordPath") == 0) // wav record path
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_memory->setWavRecordPath(cmd.svalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            String value = QDir::toNativeSeparators(QsGlobal::g_memory->getWavRecordPath());
            response = cmd.cmd.append("=") + value;
        }
    }

    //
    // WavInLoop n, n=0,1
    //
    else if (cmd.cmd.compare("WavInLoop") == 0) // enables wav in looping
    {
        if (cmd.RW == CMD::cmd_write) {
            setWavInLoopDirect(cmd.ivalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            bool value = QsGlobal::g_memory->getWavInLooping();
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //
    // WavInfo s, r, f, s= filename, r = sample_rate, f = center_frequency
    //
    else if (cmd.cmd.compare("WavInfo") == 0) // sets wav in filename
    {
        if (cmd.RW == CMD::cmd_write) {
            QFile f;
            bool ok = false;
            if (cmd.slist.count() == 3 && f.exists(cmd.slist[0])) {
                QsGlobal::g_memory->setWavInFileName(cmd.slist[0]);
                setWavInputFile(cmd.slist[0], rx_num, ok);
                if (ok) {
                    response = "OK";
                } else
                    response = "NAK";
            } else {
                response = "Wav file: " + cmd.svalue + " does not exist";
            }
        } else if (cmd.RW == CMD::cmd_read) {
            String value = QsGlobal::g_memory->getWavInFileName();
            response = cmd.cmd.append("=") + value;
        }
    }

    //
    // WavInRewind
    //
    else if (cmd.cmd.compare("WavInRewind") == 0) // sets wav in filename
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_data_reader->p_wavreader->setReadPosition(0);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            response = "NAK";
        }
    }

    //
    // WavInPosition
    //
    else if (cmd.cmd.compare("WavInPosition") == 0) // sets wav in play position in file
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_data_reader->p_wavreader->setReadPosition(cmd.ivalue);
            response = "OK";

        } else if (cmd.RW == CMD::cmd_read) {
            int value = QsGlobal::g_data_reader->p_wavreader->currentReadPos();
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //
    // WavInPositionPercent
    //
    else if (cmd.cmd.compare("WavInPositionPercent") == 0) // sets wav in play position in file
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_data_reader->p_wavreader->setReadPositionPercent(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_data_reader->p_wavreader->currentReadPosPercent();
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //
    // WavInLoopStartPosition
    //
    else if (cmd.cmd.compare("WavInLoopStartPosition") ==
             0) // sets wav in play loop start position in file
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_data_reader->p_wavreader->setLoopStartPosition(cmd.ivalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            int value = QsGlobal::g_data_reader->p_wavreader->loopStartPosition();
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //
    // WavInLoopStartPosPercent
    //
    else if (cmd.cmd.compare("WavInLoopStartPosPercent") ==
             0) // sets wav in play loop start position in file
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_data_reader->p_wavreader->setLoopStartPosPercent(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_data_reader->p_wavreader->loopStartPosPercent();
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //
    // WavInLoopEndPosition
    //
    else if (cmd.cmd.compare("WavInLoopEndPosition") ==
             0) // sets wav in play loop end position in file
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_data_reader->p_wavreader->setLoopEndPosition(cmd.ivalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            int value = QsGlobal::g_data_reader->p_wavreader->loopEndPosition();
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //
    // WavInLoopEndPosPercent
    //
    else if (cmd.cmd.compare("WavInLoopEndPosPercent") ==
             0) // sets wav in play loop end position in file
    {
        if (cmd.RW == CMD::cmd_write) {
            QsGlobal::g_data_reader->p_wavreader->setLoopEndPosPercent(cmd.dvalue);
            response = "OK";
        } else if (cmd.RW == CMD::cmd_read) {
            double value = QsGlobal::g_data_reader->p_wavreader->loopEndPosPercent();
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //
    // WavInLength
    //
    else if (cmd.cmd.compare("WavInLength") == 0) // sets wav in play loop start position in file
    {
        if (cmd.RW == CMD::cmd_write) {
            response = "NAK";
        } else if (cmd.RW == CMD::cmd_read) {
            int value = QsGlobal::g_data_reader->p_wavreader->dataLength();
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //
    // WavInCenterFrequency
    //
    else if (cmd.cmd.compare("WavInCenterFrequency") ==
             0) // sets wav in play loop start position in file
    {
        if (cmd.RW == CMD::cmd_write) {
            response = "NAK";
        } else if (cmd.RW == CMD::cmd_read) {
            int value = QsGlobal::g_data_reader->p_wavreader->fileCenterFreq();
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //
    // WavInSampleRate
    //
    else if (cmd.cmd.compare("WavInSampleRate") ==
             0) // sets wav in play loop start position in file
    {
        if (cmd.RW == CMD::cmd_write) {
            response = "NAK";
        } else if (cmd.RW == CMD::cmd_read) {
            int value = QsGlobal::g_data_reader->p_wavreader->fileSampleRate();
            response = cmd.cmd.append("=") + String::number(value);
        }
    }

    //
    // WavInStartTime
    //
    else if (cmd.cmd.compare("WavInStartTime") == 0) // gets wav in start time
    {
        if (cmd.RW == CMD::cmd_write) {
            response = "NAK";
        } else if (cmd.RW == CMD::cmd_read) {
            time_t t = QsGlobal::g_data_reader->p_wavreader->fileStartTime();
            struct tm *gmt;
            gmt = gmtime(&t);
            String str;
            str.sprintf("%04d%02d%02d_%02d%02d%02dZ", gmt->tm_year + 1900, gmt->tm_mon + 1, gmt->tm_mday, gmt->tm_hour,
                        gmt->tm_min, gmt->tm_sec);

            response = cmd.cmd.append("=") + str;
        }
    }

    //****************************************************//
    //----------------------Z-----------------------------//
    //****************************************************//

    else if (cmd.cmd.compare("ZWB") == 0) // aquires a wb block ( test only )
    {
        if (cmd.RW == CMD::cmd_write) {
            response = "NAK";
        } else if (cmd.RW == CMD::cmd_read) {
            short *data = new short[WB_BLOCK_SIZE];
            int count = QsGlobal::g_io->readEP8((unsigned char *)data, WB_BLOCK_SIZE * sizeof(short));
            delete[] data;
            response = cmd.cmd.append("=") + String::number(count);
        }
    }

    return response;
}

// ------------------------------------------------------------
// Parses and executes commands from the command entry box
// ------------------------------------------------------------
void QS1RServer::parseLocalCommand() {
    String resp = ui.commandEntry->text().trimmed().toLower();

    if (resp == "quit" || resp == "exit") // quit
    {
        setStatusText("QS1RServer quitting...");
        QTimer::singleShot(1500, this, SLOT(quit()));
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
            int freq = qRound((double)ivalue / 125.0e6 * (double)pow(2.0, 32.0));
            int result = QsGlobal::g_io->writeMultibusInt(MB_TX_FREQ, freq);
            setStatusText("result is " + String::number(result));
        } else {
            double dvalue = str.toDouble(&ok);
            if (ok) {
                int freq = qRound((double)dvalue / 125.0e6 * (double)pow(2.0, 32.0));
                int result = QsGlobal::g_io->writeMultibusInt(MB_TX_FREQ, freq);
                setStatusText("result is " + String::number(result));
            } else {
                setStatusText("Not a valid input");
            }
        }
    } else if (resp == "ptt_on" || resp == "tx") // transmit
    {
        QsTx::g_tx_ptt = true;
    } else if (resp == "ptt_off" || resp == "rx") // receive
    {
        QsTx::g_tx_ptt = false;
    } else if (resp.contains("set_tx_lo ")) // tx low filter
    {
        String str = resp.remove("set_tx_lo ");
        bool ok = false;
        int ivalue = str.toInt(&ok);
        if (ok) {
            QsGlobal::g_memory->setTxFilterLo(ivalue);
        } else {
            setStatusText("Not a valid input");
        }
    } else if (resp.contains("set_tx_hi ")) // tx high filter
    {
        String str = resp.remove("set_tx_hi ");
        bool ok = false;
        int ivalue = str.toInt(&ok);
        if (ok) {
            QsGlobal::g_memory->setTxFilterHi(ivalue);
        } else {
            setStatusText("Not a valid input");
        }
    } else if (resp.contains("set_tx_carrier ")) // tx carrier level
    {
        String str = resp.remove("set_tx_carrier ");
        bool ok = false;
        double dvalue = str.toDouble(&ok);
        if (ok) {
            QsGlobal::g_memory->setTxCarrierLevel(dvalue);
        } else {
            setStatusText("Not a valid input");
        }
    } else if (resp.contains("echo_ep0_ep1")) {
        int sz = 64;
        char buf[64];
        char buf1[64];
        QsSpl::Fill(buf, 22, sz);
        QsSpl::Fill(buf1, 99, sz);
        int result = QsGlobal::g_io->sendControlMessage(VRT_VENDOR_OUT, VRQ_ECHO_TO_EP1IN, 0, 0, buf, sz, 10000);
        _debug() << "echo result control message was " << result;
        result = QsGlobal::g_io->readEP1((unsigned char *)buf1, sz);
        _debug() << "echo read ep1 was " << result;
        for (int i = 0; i < result; i++) {
            _debug() << " buf1[" << i << "] : " << String::number(buf1[1]);
        }
    } else if (resp.contains("read_ep1")) {
        char buf[4];
        QsSpl::Zero(buf, 4);
        int result = QsGlobal::g_io->readEP1((unsigned char *)buf, 4);
        _debug() << "ep1 read result: " << String::number(result);
        _debug() << "read : " << String::number(buf[0]);
        _debug() << "read : " << String::number(buf[1]);
        _debug() << "read : " << String::number(buf[2]);
        _debug() << "read : " << String::number(buf[3]);
    } else if (resp.contains("enable_int5")) {
        int result = QsGlobal::g_io->sendInterrupt5Gate();
        _debug() << "enable int5 result: " << String::number(result);
    } else if (resp == "gui dark") // show the dark gui
    {
        QsStyle::use_classic_style = false;
        if (gui != NULL) {
            gui->close();
            delete gui;
            gui = NULL;
            qssleep.msleep(500);
        }
        gui = new SdrMaxV();
        gui->setInProcessServerPointer();
        gui->show();
    } else if (resp == "gui" || resp == "show gui" || resp == "gui classic") // show the gui
    {
        QsStyle::use_classic_style = true;
        if (gui != NULL) {
            gui->close();
            delete gui;
            gui = NULL;
            qssleep.msleep(500);
        }
        gui = new SdrMaxV();
        gui->setInProcessServerPointer();
        gui->show();
    } else if (resp == "hide gui") // hide the gui
    {
        if (gui)
            gui->close();
    } else if (resp == "hide") // hide the server window
    {
        setStatusText("hiding window...");
        QTimer::singleShot(1000, this, SLOT(hideWindow()));
    } else if (resp == "clear") {
        clearStatusText();
        showStartupMessage();
    } else if (resp == "errorlog") {
        setStatusText(error_string);
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
        String str = resp.remove("set notch ");
        StringList sl = str.split(",");
        if (sl.count() == 4) {
            int num = sl[0].toInt();
            float f0 = sl[1].toFloat();
            float hz = sl[2].toFloat();
            bool en = sl[3].toInt();
            QsGlobal::g_memory->setNotchFrequency(num, f0);
            QsGlobal::g_memory->setNotchBandwidth(num, hz);
            QsGlobal::g_memory->setNotchEnabled(num, (bool)en);
        }
    } else if (resp == "reinit hardware") {
        setStatusText("Trying to initialize QS1R Hardware...wait...");
        initQS1RHardware();
    } else if (resp == "reinit audio") {
        if (m_is_io_running)
            stopIo();
        if (!m_is_rt_audio_bypass)
            initQsAudio(SR_OUT0);
    } else if (resp == "reinit dsp") {
        if (m_is_io_running)
            stopIo();
        setupIo();
    } else if (resp == "get qtver") {
        setStatusText("Qt Version: " + String(qVersion()));
    } else if (resp == "get outdevices") {
        setStatusText("\nAudio Output Devices:");

        StringList list = p_rta->getOutputDevices();

        for (int i = 0; i < list.count(); i++) {
            setStatusText(String(list.at(i)));
        }
    } else if (resp == "get indevices") {
        setStatusText("\nAudio Input Devices:");

        StringList list = p_rta->getInputDevices();

        for (int i = 0; i < list.count(); i++) {
            setStatusText(String(list.at(i)));
        }
    } else if (resp.contains("set indevice ")) {
        String str = resp.remove("set indevice ").trimmed().toLatin1();
        String devname;
        bool ok = false;
        int devid = str.toInt(&ok);
        if (ok) {
            if (p_rta->isInputDeviceValid(devid, devname)) {
                setStatusText("\nSetting input device id to: " + String::number(devid));
                p_qsState->setRtAudioInDevId(devid);
            } else {
                setStatusText("Device id: " + String::number(devid) + " is invalid.");
            }
        } else {
            setStatusText("The device id you entered is invalid.");
        }
    } else if (resp == "get indevice") {
        int id = p_qsState->rtAudioInDevId();
        setStatusText("Input device id is: " + String::number(id));
        String devname;

        p_rta->isInputDeviceValid(id, devname);
        setStatusText("Input device name is: " + devname);
    } else if (resp.contains("set outdevice ")) {
        String str = resp.remove("set outdevice ").trimmed().toLatin1();
        String devname;
        bool ok = false;
        int devid = str.toInt(&ok);
        if (ok) {
            if (p_rta->isOutputDeviceValid(devid, devname)) {
                setStatusText("\nSetting output device id to: " + String::number(devid));
                p_qsState->setRtAudioOutDevId(devid);
            } else {
                setStatusText("Device id: " + String::number(devid) + " is invalid.");
            }
        } else {
            setStatusText("The device id you entered is invalid.");
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
    } else if (resp.contains("setup audio")) {
        audio_setup_dialog->show();
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
    } else if (resp.contains("set dac_bypass ")) {
        String str = resp.remove("set dac_bypass ");
        bool ok = false;
        int value = str.toInt(&ok);
        if (ok) {
            p_qsState->setDacBypass((bool)value);
            QsGlobal::g_memory->setDacBypass((bool)value);
            setStatusText("Setting dac bypass to: " + String::number(value));
        } else {
            setStatusText("Dac Bypass value is invalid.  Must be 0/1");
        }
    } else if (resp.contains("set startup_agc_decay_speed ")) {
        String str = resp.remove("set startup_agc_decay_speed ");
        bool ok = false;
        int value = str.toInt(&ok);
        if (ok) {
            p_qsState->setStartupAGCDecaySpeed((double)value);
            setStatusText("Setting startup agc mode to: " + str.trimmed().toUpper());
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
        int value = modeStringToInt(str.trimmed().toUpper());
        if (value != -1) {
            p_qsState->setStartupMode((QSDEMODMODE)value);
            setStatusText("Setting startup mode to: " + str.trimmed().toUpper());
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
    } else if (resp.contains("set audio_output_bypass ")) {
        String str = resp.remove("set audio_output_bypass ");
        bool ok = false;
        int value = str.toInt(&ok);
        if (ok) {
            p_qsState->setRtAudioBypass((bool)value);
            QsGlobal::g_memory->setRtAudioBypass((bool)value);
            if (m_is_rt_audio_bypass) {
                m_is_rt_audio_bypass = (bool)value;
                if (!m_is_rt_audio_bypass) {
                    initQsAudio(SR_OUT0);
                }
            } else {
                m_is_rt_audio_bypass = (bool)value;
            }
            if (m_is_io_running)
                stopIo();
            setupIo();
            setStatusText("Setting Audio output bypass to: " + String::number(value));
        } else {
            setStatusText("Audio output bypass value is invalid.  Must be 0/1");
        }
    } else if (resp.contains("get audio_output_bypass")) {
        bool bypass = QsGlobal::g_memory->getRtAudioBypass();
        if (bypass) {
            setStatusText("Audio output is bypassed, value: 1 ");
        } else {
            setStatusText("Audio output is not bypassed, value: 0 ");
        }
    } else if (resp.contains("reset device")) {
        stopIo();
        qssleep.msleep(200);
        setStatusText("Resetting QS1R USB interface...");
        QsGlobal::g_io->resetDevice();
        qssleep.msleep(2000);
        initQS1RHardware();
        setStatusText("Reinitializing QS1R Hardware...");
    }
#ifdef Q_OS_WIN
    else if (resp == "install libusb") {
        typedef BOOL(WINAPI * IW64PFP)(HANDLE, BOOL *);

        BOOL res = FALSE;
        IW64PFP iw64p = (IW64PFP)GetProcAddress(GetModuleHandle(L"kernel32"), "IsWow64Process");

        if (iw64p != NULL) {
            iw64p(GetCurrentProcess(), &res);
        }
        if (res != FALSE) // 64 bit
        {
            if (!QProcess::startDetached("./QS1R-libusb/dpinst64.exe")) {
                setStatusText("Could not locate dpinst64.exe");
            }
        } else // 32 bit
        {
            if (!QProcess::startDetached("./QS1R-libusb/dpinst32.exe", StringList("/LM"))) {
                setStatusText("Could not locate dpinst32.exe");
            }
        }
    }
#endif
    else if (resp == "list serial ports") {
        StringList ser_list;

#ifdef Q_OS_WIN
        QsSerialDeviceLister dl;
        StringList list = dl.getSerialDevices(); // check registry first (only on windows)
        StringListIterator i(list);
        while (i.hasNext()) {
            String sdev = i.next().toUpper().trimmed();
            if (!ser_list.contains(sdev)) {
                ser_list.append(sdev);
            }
        }
#endif
        QextSerialEnumerator se;
        QList<QextPortInfo> se_list = se.getPorts(); // now check with setupapi
        QListIterator<QextPortInfo> li(se_list);
        while (li.hasNext()) {
            QextPortInfo pi = li.next();
            String spi = pi.portName.toUpper().trimmed();
            if (!ser_list.contains(spi)) {
                ser_list.append(spi);
            }
        }
        StringListIterator k(ser_list);
        setStatusText("----------------");
        setStatusText("serial ports:");
        while (k.hasNext()) {
            setStatusText(k.next());
        }
        setStatusText("----------------");
    } else if (resp == "dump info") {
        String dump_file = QDir::homePath() + "/qs1r_dump.qsi";
        QFile f(dump_file);
        QTextStream out(&f);

        if (f.open(QIODevice::WriteOnly)) {
            setStatusText("Generating file...please wait...");
            update();

            QDir dir;
            dir.cd(QDir::currentPath());
            dir.setFilter(QDir::Files | QDir::Dirs | QDir::Hidden | QDir::NoSymLinks);
            dir.setSorting(QDir::DirsFirst | QDir::Name);
            QFileInfoList list = dir.entryInfoList();

            out << QDateTime::currentDateTime().toString() << "\n"
                << "Qt Ver: {" << String(qVersion()) << "}\n"
                << "Driver: {" << m_driver_type << "}\n"
                << "Home Path: {" << QDir::homePath() << "}\n"
                << "App Path: {" << QDir::currentPath() << "}\n"
                << "QS1R UUID: " << readQS1RUuid().toString() << "\n"
                << "QS1R EEPROM: " << readQS1REEPROMData() << "\n"
                << "----QS1R Folder Files----" << "\n\n";

            for (int i = 0; i < list.size(); ++i) {
                QFileInfo fileInfo = list.at(i);
                out << fileInfo.fileName() << " Size: " << String::number(fileInfo.size()) << "\n";
            }

            out << "------Done------" << "\n\n";

            dir.cd(QDir::homePath());
            dir.setFilter(QDir::Files | QDir::Dirs | QDir::Hidden | QDir::NoSymLinks);
            dir.setSorting(QDir::DirsFirst | QDir::Name);
            StringList filters;
            filters << "*.qsi" << "*.db" << "SDRMAXIV Recordings";
            dir.setNameFilters(filters);
            list = dir.entryInfoList();

            out << "----Home Folder Files----" << "\n";

            for (int i = 0; i < list.size(); ++i) {
                QFileInfo fileInfo = list.at(i);
                out << fileInfo.fileName() << " Size: " << String::number(fileInfo.size()) << "\n";
            }

            out << "------Done------" << "\n";

            f.flush();
            setStatusText("Successfully created dump file in: " + dump_file);
        } else {
            setStatusText("Could not create dump file");
        }
        f.close();
    } else if (resp == "help") {
        clearStatusText();
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
        clearStatusText();
        update();
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
        QUdpSocket s;

        String cmd_resp = doCommandProcessor(resp, m_local_rx_num_selector, &s);

        if (cmd_resp == "?")
            setStatusText("'" + resp + "'" + " : invalid command");
        else
            setStatusText(cmd_resp);
    }

    if (m_is_factory_init_enabled) {
        if (resp == "1") // find device
        {
            findQS1RDevice();
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
            String uuid_str = QInputDialog::getText(this, "Program S/N", "Enter UUID", QLineEdit::Normal,
                                                    "{0653f1ae-8d4d-4fe3-9e71-42df3be734cf}", &ok);
            if (ok && !uuid_str.isEmpty()) {
                QUuid uuid(uuid_str.simplified());
                if (!uuid.isNull() && validateQS1RSN(uuid)) {
                    writeQS1RSN(uuid);
                    setStatusText("UUID Written Successfully");
                } else {
                    setStatusText("Not a valid UUID");
                }
            }
        } else if (resp == "7") // read s/n
        {
            QUuid uuid = readQS1RUuid();
            setStatusText(uuid.toString());
        } else if (resp == "m") // menu
        {
            clearStatusText();
            update();
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
            clearStatusText();
            update();
            setStatusText(" Exiting Factory Mode... ");
            QTimer::singleShot(600, this, SLOT(clearStatusText()));
            QTimer::singleShot(650, this, SLOT(showStartupMessageWithReady()));
        }
    }

    ui.commandEntry->clear();
    ui.commandEntry->setFocus();
}

// DIRECT COMMANDS
// These functions are able to be called by the gui directly

void QS1RServer::setTxPttDirect(bool value) {
    QsTx::g_tx_ptt = value;
    if (value) {
        m_status_message_backing_register |= TX_PTT;
        QsGlobal::g_io->writeMultibusInt(MB_STATUS_MSG, m_status_message_backing_register);
    } else {
        m_status_message_backing_register &= ~TX_PTT;
        QsGlobal::g_io->writeMultibusInt(MB_STATUS_MSG, m_status_message_backing_register);
    }
    emit PttState(value);
}

void QS1RServer::setCwSidetoneFreqDirect(double freq) {
    if (m_is_hardware_init)
        QsGlobal::g_io->writeMultibusInt(MB_CW_SIDETONE_FREQ, frequencyToPhaseIncrement(freq));
    QsGlobal::g_memory->setCwSidetoneFreq(freq);
}

void QS1RServer::setCwSidetoneFreqDirect(int freq) {
    if (m_is_hardware_init)
        QsGlobal::g_io->writeMultibusInt(MB_CW_SIDETONE_FREQ, frequencyToPhaseIncrement((double)freq));
    QsGlobal::g_memory->setCwSidetoneFreq((double)freq);
}

void QS1RServer::setCwSidetoneVolumeDirect(double vol) {
    unsigned char volume = (unsigned char)qBound((double)0.0, (double)qRound(127.0 * vol), (double)255.0);
    if (m_is_hardware_init) {
        int result = QsGlobal::g_io->readMultibusInt(MB_CW_SETTINGS_REG);
        int val = (result & 0xffffff00) + volume;
        QsGlobal::g_io->writeMultibusInt(MB_CW_SETTINGS_REG, val);
    }
    QsGlobal::g_memory->setCwSidetoneVolume(vol);
}

void QS1RServer::setCwSidetoneVolumeDirect(int vol) {
    unsigned char volume =
        (unsigned char)qBound((double)0.0, (double)qRound(127.0 * (double)vol / 100.0), (double)255.0);
    if (m_is_hardware_init) {
        int result = QsGlobal::g_io->readMultibusInt(MB_CW_SETTINGS_REG);
        int val = (result & 0xffffff00) + volume;
        QsGlobal::g_io->writeMultibusInt(MB_CW_SETTINGS_REG, val);
    }
    QsGlobal::g_memory->setCwSidetoneVolume(volume);
}

void QS1RServer::setCwSpeedDirect(int wpm) {
    unsigned char speed = qBound((unsigned char)1, (unsigned char)wpm, (unsigned char)99);
    QsGlobal::g_memory->setCwSpeed(wpm);
    if (m_is_hardware_init) {
        int result = QsGlobal::g_io->readMultibusInt(MB_CW_SETTINGS_REG);
        int val = (result & 0xffff00ff) + (speed << 8);
        QsGlobal::g_io->writeMultibusInt(MB_CW_SETTINGS_REG, val);
    }
}

void QS1RServer::setCwModeDirect(bool modeb) {
    unsigned char mode = 0;
    if (modeb) {
        mode = 255;
    } else {
        mode = 0;
    }
    if (m_is_hardware_init) {
        int result = QsGlobal::g_io->readMultibusInt(MB_CW_SETTINGS_REG);
        int val = (result & 0xff00ffff) + (mode << 16);
        QsGlobal::g_io->writeMultibusInt(MB_CW_SETTINGS_REG, val);
    }
}

void QS1RServer::setCwStraightKeyDirect(bool straightkey) {
    unsigned char mode = 0;
    if (straightkey) {
        mode = 255;
    } else {
        mode = 0;
    }
    if (m_is_hardware_init) {
        int result = QsGlobal::g_io->readMultibusInt(MB_CW_SETTINGS_REG);
        int val = (result & 0x00ffffff) + (mode << 24);
        QsGlobal::g_io->writeMultibusInt(MB_CW_SETTINGS_REG, val);
    }
}

void QS1RServer::setOpenAudioSetupDirect() { audio_setup_dialog->show(); }

void QS1RServer::setModeDirect(int value) {
    QsGlobal::g_memory->setDemodMode((QSDEMODMODE)value);
    if (value == dmCW) {
        m_status_message_backing_register |= CW_ENABLE;
        QsGlobal::g_io->writeMultibusInt(MB_STATUS_MSG, m_status_message_backing_register);
    } else {
        m_status_message_backing_register &= ~CW_ENABLE;
        QsGlobal::g_io->writeMultibusInt(MB_STATUS_MSG, m_status_message_backing_register);
    }
}

void QS1RServer::setWavInLoopDirect(int value) {
    QsGlobal::g_data_reader->p_wavreader->setLooping((bool)value);
    QsGlobal::g_memory->setWavInLooping((bool)value);
}

void QS1RServer::setSampleRateDirect(int value) { setFpgaForSampleRate(value); }

// DIRECT CALLS
// These functions are able to be called by the gui directly

bool QS1RServer::getSpectrumDBMValuesDirect(std::vector<float> &vect, int size) { return p_ps->doMainPs(vect, size); }

bool QS1RServer::getPostSpectrumDBMValuesDirect(std::vector<float> &vect, int size) {
    return p_ps->doPostPs(vect, size);
}
