#include "../include/qs_state.hpp"
#include "../include/qs_settingsclass.hpp"

QsState::QsState() : settings(std::make_unique<Settings>("./qs1r_settings.json")) {}

void QsState::init() {
    settings->setValue("SERVER", SDRMAXV_VERSION);
    readSettings();
}

void QsState::readSettings() {

    m_block_size = settings->value("BlockSize", QS_DEFAULT_DSP_BLOCKSIZE);    
    m_rs_quality = (settings->value("ResamplerQuality", QS_DEFAULT_RS_QUAL));
    m_rta_audio_frames = (settings->value("RtAudioFrameSize", QS_DEFAULT_RT_FRAMES));
    m_clock_correction = (settings->value("ClockCorrection", QS_DEFAULT_CLOCK_CORRECT));
    m_encode_clk_freq = (settings->value("EncodeClockFreq", QS_DEFAULT_ENC_FREQ));
    m_smeter_correction = (settings->value("SMeterCorrection", 0.0));
    m_main_filter_taps = (settings->value("MainFilterTaps", QS_DEFAULT_MAIN_FILTER_SIZE));    
    m_startup_sample_rate = (settings->value("SampleRate", QS_DEFAULT_DSP_RATE));
    m_startup_freq = (settings->value("Frequency", QS_DEFAULT_FREQ));
    m_startup_filter_low = (settings->value("FilterLow", QS_DEFAULT_FILTER_LO));
    m_startup_filter_high = (settings->value("FilterHigh", QS_DEFAULT_FILTER_HI));
    m_startup_mode = ((QSDEMODMODE)settings->value("Mode", QS_DEFAULT_DEMOD_MODE));
    m_startup_volume = (settings->value("Volume", QS_DEFAULT_VOLUME));
    m_startup_agc_decay_speed = (settings->value("AGCDecaySpeed", QS_DEFAULT_AGC_LONG_DECAY));
    m_startup_agc_threshold = (settings->value("AGCThreshold", QS_DEFAULT_AGC_THRESHOLD));    
    m_rand_is_on = (settings->value("RAND", QS_DEFAULT_RAND));
    m_dith_is_on = (settings->value("DITH", QS_DEFAULT_DITH));
    m_pga_is_on = (settings->value("PGA", QS_DEFAULT_PGA));    
    m_ext_mute_enable_is_on = (settings->value("EXTMUTEENABLE", QS_DEFAULT_EXT_MUTE_ENABLE));
    m_rta_in_dev_id = (settings->value("AUDIOINID", QS_DEFAULT_RTA_IN_DEVID));
    m_rta_out_dev_id = (settings->value("AUDIOOUTID", QS_DEFAULT_RTA_OUT_DEVID));
}

void QsState::setBlockSize(int blocksz) {
    m_block_size = blocksz;

    settings->setValue("BlockSize", m_block_size);
}

int QsState::blockSize() { return m_block_size; }

void QsState::setPsBlockSize(int blocksz) {
    m_ps_block_size = blocksz;

    settings->setValue("PsBlockSize", m_ps_block_size);
}

void QsState::setTxBlockSize(int blocksz) {
    m_tx_block_size = blocksz;

    settings->setValue("TxBlockSize", m_tx_block_size);
}

int QsState::psBlockSize() { return m_ps_block_size; }

int QsState::txBlockSize() { return m_tx_block_size; }

void QsState::setStartupSampleRate(double value) {
    m_startup_sample_rate = value;
    ;
    settings->setValue("SampleRate", m_startup_sample_rate);
}

double QsState::startupSampleRate() { return m_startup_sample_rate; }

void QsState::setStartupFrequency(double value) {
    m_startup_freq = value;
    ;
    settings->setValue("Frequency", m_startup_freq);
}

double QsState::startupFrequency() { return m_startup_freq; }

void QsState::setStartupFilterLow(int value) {
    m_startup_filter_low = value;
    ;
    settings->setValue("FilterLow", m_startup_filter_low);
}

int QsState::startupFilterLow() { return m_startup_filter_low; }

void QsState::setStartupFilterHigh(int value) {
    m_startup_filter_high = value;
    ;
    settings->setValue("FilterHigh", m_startup_filter_high);
}

int QsState::startupFilterHigh() { return m_startup_filter_high; }

void QsState::setStartupMode(QSDEMODMODE value) {
    m_startup_mode = value;
    ;
    settings->setValue("Mode", (int)m_startup_mode);
}

QSDEMODMODE QsState::startupMode() { return m_startup_mode; }

void QsState::setStartupVolume(double value) {
    m_startup_volume = value;
    ;
    settings->setValue("Volume", m_startup_volume);
}

double QsState::startupVolume() { return m_startup_volume; }

void QsState::setStartupAGCDecaySpeed(double value) {
    m_startup_agc_decay_speed = value;
    ;
    settings->setValue("AGCDecaySpeed", (int)m_startup_agc_decay_speed);
}

double QsState::startupAGCDecaySpeed() { return m_startup_agc_decay_speed; }

void QsState::setStartupAGCThreshold(double value) {
    m_startup_agc_threshold = value;
    ;
    settings->setValue("AGCThreshold", m_startup_agc_threshold);
}

double QsState::startupAGCThreshold() { return m_startup_agc_threshold; }

void QsState::setPGA(bool on) {
    m_pga_is_on = on;
    ;
    settings->setValue("PGA", m_pga_is_on);
}

void QsState::setRAND(bool on) {
    m_rand_is_on = on;
    ;
    settings->setValue("RAND", m_rand_is_on);
}

void QsState::setDITH(bool on) {
    m_dith_is_on = on;
    ;
    settings->setValue("DITH", m_dith_is_on);
}

bool QsState::rand() { return m_rand_is_on; }

bool QsState::dith() { return m_dith_is_on; }

bool QsState::pga() { return m_pga_is_on; }

int QsState::rsQual() { return m_rs_quality; }

int QsState::rtAudioFrameSize() { return m_rta_audio_frames; }

int QsState::rtAudioInDevId() { return m_rta_in_dev_id; }

int QsState::rtAudioOutDevId() { return m_rta_out_dev_id; }

void QsState::setClockCorrection(double value) {
    m_clock_correction = value;

    settings->setValue("ClockCorrection", m_clock_correction);
}

double QsState::clockCorrection() { return m_clock_correction; }

double QsState::encodeClockFrequency() { return m_encode_clk_freq; }

void QsState::setSMeterCorrection(double value) {
    m_smeter_correction = value;

    settings->setValue("SMeterCorrection", m_smeter_correction);
}

double QsState::smeterCorrection() { return m_smeter_correction; }

int QsState::mainFilterTapSize() { return m_main_filter_taps; }

void QsState::setRtAudioInDevId(int value) {
    m_rta_in_dev_id = value;
    ;
    settings->setValue("AUDIOINID", m_rta_in_dev_id);
}

void QsState::setRtAudioOutDevId(int value) {
    m_rta_out_dev_id = value;
    ;
    settings->setValue("AUDIOOUTID", m_rta_out_dev_id);
}

void QsState::setExtMuteEnable(bool on) {
    m_ext_mute_enable_is_on = on;
    ;
    settings->setValue("EXTMUTEENABLE", m_ext_mute_enable_is_on);
}

bool QsState::extMuteEnable() { return m_ext_mute_enable_is_on; }

