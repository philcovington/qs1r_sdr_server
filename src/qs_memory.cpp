#include "../headers/qs_memory.h"
#include "../headers/qs_dataproc.h"
#include <iostream>

QsMemory::QsMemory() {
    for (int i = 0; i < MAX_RECEIVERS; i++) {
        m_volume[i] = QS_DEFAULT_VOLUME;

        // AGC
        m_agc_decay_speed[i] = QS_DEFAULT_AGC_LONG_DECAY;
        m_agc_fixed_gain[i] = QS_DEFAULT_AGC_FIXED_GAIN;
        m_agc_threshold[i] = QS_DEFAULT_AGC_THRESHOLD;
        m_agc_slope[i] = QS_DEFAULT_AGC_SLOPE;
        m_agc_hangtime[i] = QS_DEFAULT_AGC_HANGTIME;
        m_agc_hangtime_switch[i] = QS_DEFAULT_AGC_HANGSWITCH;
        m_agc_current_gain[i] = -160.0;
        m_agc_current_gain_c[i] = 0;

        // MAIN FILTER
        m_filter_low[i] = QS_DEFAULT_FILTER_LO;
        m_filter_high[i] = QS_DEFAULT_FILTER_HI;

        // TONE GENERATOR
        m_tone_frequency[i] = QS_DEFAULT_TONE_FREQ;

        // CW OFFSET GENERATOR
        m_offset_frequency[i] = QS_DEFAULT_CW_FREQ;

        m_display_freq_offset[i] = QS_DEFAULT_DISPL_FREQ_OFFSET;

        // DEMOD MODE
        m_demod_mode[i] = (QSDEMODMODE)QS_DEFAULT_DEMOD_MODE;

        // BINAURAL
        m_binaural_mode[i] = QS_DEFAULT_BINAURAL_MODE;

        // AVG NOISE BLANKER
        m_avg_nb_threshold[i] = QS_DEFAULT_ANB_THRESH;
        m_avg_nb_switch[i] = QS_DEFAULT_ANB_ON;

        // BLOCK NOISE BLANKER
        m_block_nb_threshold[i] = QS_DEFAULT_BNB_THRESH;
        m_block_nb_switch[i] = QS_DEFAULT_BNB_ON;

        // ANF
        m_autonotch_switch[i] = QS_DEFAULT_AUTONOTCH_ON;
        m_autonotch_rate[i] = QS_DEFAULT_AUTONOTCH_RATE;
        m_autonotch_delay[i] = QS_DEFAULT_AUTONOTCH_DELAY;
        m_autonotch_taps[i] = QS_DEFAULT_AUTONOTCH_TAPS;
        m_autonotch_leak[i] = QS_DEFAULT_AUTONOTCH_LEAK;

        // NR
        m_noise_reduction_switch[i] = QS_DEFAULT_NOISERED_ON;
        m_noise_reduction_rate[i] = QS_DEFAULT_NOISERED_RATE;
        m_noise_reduction_delay[i] = QS_DEFAULT_NOISERED_DELAY;
        m_noise_reduction_taps[i] = QS_DEFAULT_NOISERED_TAPS;
        m_noise_reduction_leak[i] = QS_DEFAULT_NOISERED_LEAK;

        // S METER
        m_s_meter_cv[i] = -160.0;
        m_s_meter_cv_c[i] = 0;

        // SQUELCH
        m_squelch_switch[i] = QS_DEFAULT_SQUELCH_ON;
        m_squelch_threshold[i] = QS_DEFAULT_SQUELCH_THRESH;
        m_rx_frequency[i] = QS_DEFAULT_FREQ;

        for (int j = 0; j < MAX_MAN_NOTCHES; j++) {
            m_notch_frequency[i][j] = 1000.0;
            m_notch_bandwidth[i][j] = 100.0;
            m_notch_enabled[i][j] = false;
        }
    }

    m_smeter_correction = 0.0;
    m_ps_correction = 0.0;
    m_post_ps_correction = 0.0;
    m_wb_correction = 0.0;
    m_enc_clock_correction = QS_DEFAULT_CLOCK_CORRECT;
    m_data_proc_rate = QS_DEFAULT_DATA_PROC_RATE;
    m_data_post_proc_rate = QS_DEFAULT_DATA_POSTPROC_RATE;
    m_rt_audio_rate = QS_DEFAULT_RTA_RATE;
    m_rt_audio_bypass = QS_DEFAULT_RT_BYPASS;
    m_dac_bypass = QS_DEFAULT_DAC_BYPASS;
    m_adc_pga_on = QS_DEFAULT_PGA;
    m_adc_rand_on = QS_DEFAULT_RAND;
    m_adc_dith_on = QS_DEFAULT_DITH;
    m_rt_audio_frames = QS_DEFAULT_RT_FRAMES;
    m_wav_rec_continuous = QS_DEFAULT_WAV_CONT;
    m_wav_rec_prebuffer = QS_DEFAULT_WAV_PREBUF;
    m_wav_in_loop = QS_DEFAULT_WAV_IN_LOOPS;
    m_wav_rec_prebuffer_time = QS_DEFAULT_WAV_PREBUFTIME;
    m_wav_rec_path = std::string(QS_DEFAULT_WAV_PATH);
    m_wav_in_filename = QS_DEFAULT_WAV_IN_NAME;
    m_wav_play_starttime = time_t();
    m_read_block_size = QS_DEFAULT_DSP_BLOCKSIZE;
    m_ps_block_size = QS_DEFAULT_PS_BLOCKSIZE;
    m_dac_block_size = QS_DEFAULT_DAC_BLOCKSIZE;
    m_tx_block_size = QS_DEFAULT_TX_BLOCKSIZE;
    m_resampler_quality = QS_DEFAULT_RS_QUAL;
    m_resampler_rate = QS_DEFAULT_RT_RATE;
    m_enc_clock_freq = QS_DEFAULT_ENC_FREQ;
    m_tx_filter_low = QS_DEFAULT_TX_FILT_LO;
    m_tx_filter_high = QS_DEFAULT_TX_FILT_HI;
    m_tx_carrier_level = QS_DEFAULT_TX_CARRIER_LEVEL;
    m_tx_pdac_level = QS_DEFAULT_TX_PDAC_LEVEL;
    m_cw_sidetone_freq = QS_DEFAULT_CW_SIDETONE_FREQ;
    m_cw_sidetone_volume = QS_DEFAULT_CW_SIDETONE_VOLUME * 127.0;
    m_cw_speed = QS_DEFAULT_CW_SPEED_WPM;
    m_cw_mode = QS_DEFAULT_CW_MODEB;
    m_straight_key_mode = QS_DEFAULT_CW_STRAIGHT_KEY;
    m_mic_gain_db = QS_DEFAULT_MIC_GAIN;
    m_gate_level_db = QS_DEFAULT_GATE_LEVEL;
    m_tx_offset_freq = 0.0;
    m_tx_frequency = QS_DEFAULT_FREQ;

    m_gate_level_val = pow(10.0, (m_gate_level_db / 20.0));
    m_mic_gain_val = pow(10.0, (m_mic_gain_db / 20.0));
}

//****************************************************//
//--------------------VOLUME--------------------------//
//****************************************************//

void QsMemory::setVolume(double value, int rx_num) { m_volume[rx_num] = value; }

double QsMemory::getVolume(int rx_num) { return m_volume[rx_num]; }

//****************************************************//
//----------------------AGC---------------------------//
//****************************************************//

void QsMemory::setAgcDecaySpeed(double value, int rx_num) { m_agc_decay_speed[rx_num] = value; }

double QsMemory::getAgcDecaySpeed(int rx_num) { return m_agc_decay_speed[rx_num]; }

void QsMemory::setAgcFixedGain(double value, int rx_num) { m_agc_fixed_gain[rx_num] = value; }

double QsMemory::getAgcFixedGain(int rx_num) { return m_agc_fixed_gain[rx_num]; }

void QsMemory::setAgcThreshold(double value, int rx_num) {
    m_agc_threshold[rx_num] = value;
    if (value == 0)
        std::cout << ("agc thesh was 0") << std::endl;
}

double QsMemory::getAgcThreshold(int rx_num) {
    if (m_agc_threshold[rx_num] == 0)
        std::cout << ("agc thesh was 0") << std::endl;
    return m_agc_threshold[rx_num];
}

void QsMemory::setAgcSlope(double value, int rx_num) { m_agc_slope[rx_num] = value; }

double QsMemory::getAgcSlope(int rx_num) { return m_agc_slope[rx_num]; }

void QsMemory::setAgcHangTime(double value, int rx_num) { m_agc_hangtime[rx_num] = value; }

double QsMemory::getAgcHangTime(int rx_num) { return m_agc_hangtime[rx_num]; }

void QsMemory::setAgcHangTimeSwitch(bool value, int rx_num) { m_agc_hangtime_switch[rx_num] = value; }

bool QsMemory::getAgcHangTimeSwitch(int rx_num) { return m_agc_hangtime_switch[rx_num]; }

void QsMemory::setAgcCurrentGain(double value, int rx_num) { m_agc_current_gain[rx_num] = value; }

double QsMemory::getAgcCurrentGain(int rx_num) { return m_agc_current_gain[rx_num]; }

void QsMemory::setAgcCurrentGainC(unsigned char value, int rx_num) { m_agc_current_gain_c[rx_num] = value; }

unsigned char QsMemory::getAgcCurrentGainC(int rx_num) { return m_agc_current_gain_c[rx_num]; }

//****************************************************//
//---------------------FILTER-------------------------//
//****************************************************//

void QsMemory::setFilterLo(int value, int rx_num) { m_filter_low[rx_num] = value; }

int QsMemory::getFilterLo(int rx_num) { return m_filter_low[rx_num]; }

void QsMemory::setFilterHi(int value, int rx_num) { m_filter_high[rx_num] = value; }

int QsMemory::getFilterHi(int rx_num) { return m_filter_high[rx_num]; }

void QsMemory::setTxFilterLo(unsigned int value) { m_tx_filter_low = value; }

unsigned int QsMemory::getTxFilterLo() { return m_tx_filter_low; }

void QsMemory::setTxFilterHi(unsigned int value) { m_tx_filter_high = value; }

unsigned int QsMemory::getTxFilterHi() { return m_tx_filter_high; }

//***************************************************//
//------------------QS1E-----------------------------//
//***************************************************//

void QsMemory::setTxPdacLevel(unsigned int value) {
    m_tx_pdac_level = qBound(static_cast<unsigned int>(0), value, static_cast<unsigned int>(100));
}

unsigned int QsMemory::getTxPdacLevel() { return m_tx_pdac_level; }

void QsMemory::setTxCarrierLevel(double value) { m_tx_carrier_level = value; }

double QsMemory::getTxCarrierLevel() { return m_tx_carrier_level; }

void QsMemory::setCwSidetoneFreq(double value) { m_cw_sidetone_freq = value; }

double QsMemory::getCwSidetoneFreq() { return m_cw_sidetone_freq; }

void QsMemory::setTxOffsetFrequency(double value) { m_tx_offset_freq = value; }

double QsMemory::getTxOffsetFrequency() { return m_tx_offset_freq; }

void QsMemory::setCwSidetoneVolume(double value) { m_cw_sidetone_volume = value; }

double QsMemory::getCwSidetoneVolume() { return m_cw_sidetone_volume; }

void QsMemory::setCwSpeed(int value) { m_cw_speed = value; }

int QsMemory::getCwSpeed() { return m_cw_speed; }

void QsMemory::setCwMode(bool value) { m_cw_mode = value; }

bool QsMemory::getCwMode() { return m_cw_mode; }

void QsMemory::setCwStraightKeyMode(bool value) { m_straight_key_mode = value; }

bool QsMemory::getCwStraightKeyMode() { return m_straight_key_mode; }

void QsMemory::setMicGainDb(double value) {
    m_mic_gain_db = value;
    m_mic_gain_val = pow(10.0, (m_mic_gain_db / 20.0));
}

void QsMemory::setMicGainDb(int value) {
    m_mic_gain_db = (double)value;
    m_mic_gain_val = pow(10.0, (m_mic_gain_db / 20.0));
}

double QsMemory::getMicGainDb() { return m_mic_gain_db; }

double QsMemory::getMicGainVal() { return m_mic_gain_val; }

void QsMemory::setGateLevelDb(double value) {
    m_gate_level_db = value;
    m_gate_level_val = pow(10.0, (m_gate_level_db / 20.0));
}

void QsMemory::setGateLevelDb(int value) {
    m_gate_level_db = (double)value;
    m_gate_level_val = pow(10.0, (m_gate_level_db / 20.0));
}

double QsMemory::getGateLevelDb() { return m_gate_level_db; }

double QsMemory::getGateLevelVal() { return m_gate_level_val; }

void QsMemory::setTxLOFrequency(double value) { m_tx_frequency = value; }

double QsMemory::getTxLOFrequency() { return m_tx_frequency; }

//****************************************************//
//---------------------TONE GEN-----------------------//
//****************************************************//

void QsMemory::setToneLoFrequency(double value, int rx_num) { m_tone_frequency[rx_num] = value; }

double QsMemory::getToneLoFrequency(int rx_num) { return m_tone_frequency[rx_num]; }

void QsMemory::setOffsetGeneratorFrequency(double value, int rx_num) { m_offset_frequency[rx_num] = value; }

double QsMemory::getOffsetGeneratorFrequency(int rx_num) { return m_offset_frequency[rx_num]; }

//****************************************************//
//------------------DEMODULATOR-----------------------//
//****************************************************//

void QsMemory::setDemodMode(QSDEMODMODE value, int rx_num) { m_demod_mode[rx_num] = value; }

QSDEMODMODE QsMemory::getDemodMode(int rx_num) { return m_demod_mode[rx_num]; }

void QsMemory::setBinauralMode(bool value, int rx_num) { m_binaural_mode[rx_num] = value; }

bool QsMemory::getBinauralMode(int rx_num) { return m_binaural_mode[rx_num]; }

//****************************************************//
//---------------------AVG NB------------------------//
//****************************************************//

void QsMemory::setAvgNoiseBlankerThreshold(double value, int rx_num) { m_avg_nb_threshold[rx_num] = value; }

double QsMemory::getAvgNoiseBlankerThreshold(int rx_num) { return m_avg_nb_threshold[rx_num]; }

void QsMemory::setAvgNoiseBlankerOn(bool value, int rx_num) { m_avg_nb_switch[rx_num] = value; }

bool QsMemory::getAvgNoiseBlankerOn(int rx_num) { return m_avg_nb_switch[rx_num]; }

//***************************************************//
//---------------------BLOCK NB----------------------//
//***************************************************//

void QsMemory::setBlockNoiseBlankerThreshold(double value, int rx_num) { m_block_nb_threshold[rx_num] = value; }

double QsMemory::getBlockNoiseBlankerThreshold(int rx_num) { return m_block_nb_threshold[rx_num]; }

void QsMemory::setBlockNoiseBlankerOn(bool value, int rx_num) { m_block_nb_switch[rx_num] = value; }

bool QsMemory::getBlockNoiseBlankerOn(int rx_num) { return m_block_nb_switch[rx_num]; }

//***************************************************//
//--------------------AUTO NOTCH---------------------//
//***************************************************//

void QsMemory::setAutoNotchOn(bool value, int rx_num) { m_autonotch_switch[rx_num] = value; }

bool QsMemory::getAutoNotchOn(int rx_num) { return m_autonotch_switch[rx_num]; }

void QsMemory::setAutoNotchRate(double value, int rx_num) { m_autonotch_rate[rx_num] = value; }

double QsMemory::getAutoNotchRate(int rx_num) { return m_autonotch_rate[rx_num]; }

void QsMemory::setAutoNotchLeak(double value, int rx_num) { m_autonotch_leak[rx_num] = value; }

double QsMemory::getAutoNotchLeak(int rx_num) { return m_autonotch_leak[rx_num]; }

void QsMemory::setAutoNotchDelay(int value, int rx_num) { m_autonotch_delay[rx_num] = value; }

int QsMemory::getAutoNotchDelay(int rx_num) { return m_autonotch_delay[rx_num]; }

void QsMemory::setAutoNotchTaps(int value, int rx_num) { m_autonotch_taps[rx_num] = value; }

int QsMemory::getAutoNotchTaps(int rx_num) { return m_autonotch_taps[rx_num]; }

//***************************************************//
//------------------NOISE REDUCTION------------------//
//***************************************************//

void QsMemory::setNoiseReductionOn(bool value, int rx_num) { m_noise_reduction_switch[rx_num] = value; }

bool QsMemory::getNoiseReductionOn(int rx_num) { return m_noise_reduction_switch[rx_num]; }

void QsMemory::setNoiseReductionRate(double value, int rx_num) { m_noise_reduction_rate[rx_num] = value; }

double QsMemory::getNoiseReductionRate(int rx_num) { return m_noise_reduction_rate[rx_num]; }

void QsMemory::setNoiseReductionLeak(double value, int rx_num) { m_noise_reduction_leak[rx_num] = value; }

double QsMemory::getNoiseReductionLeak(int rx_num) { return m_noise_reduction_leak[rx_num]; }

void QsMemory::setNoiseReductionDelay(int value, int rx_num) { m_noise_reduction_delay[rx_num] = value; }

int QsMemory::getNoiseReductionDelay(int rx_num) { return m_noise_reduction_delay[rx_num]; }

void QsMemory::setNoiseReductionTaps(int value, int rx_num) { m_noise_reduction_taps[rx_num] = value; }

int QsMemory::getNoiseReductionTaps(int rx_num) { return m_noise_reduction_taps[rx_num]; }

//***************************************************//
//-------------------S METER  -----------------------//
//***************************************************//

void QsMemory::setSMeterCurrentValue(double value, int rx_num) { m_s_meter_cv[rx_num] = value; }

double QsMemory::getSMeterCurrentValue(int rx_num) { return m_s_meter_cv[rx_num]; }

void QsMemory::setSMeterCurrentValueC(unsigned char value, int rx_num) { m_s_meter_cv_c[rx_num] = value; }

unsigned char QsMemory::getSMeterCurrentValueC(int rx_num) { return m_s_meter_cv_c[rx_num]; }

//***************************************************//
//---------------------SQUELCH-----------------------//
//***************************************************//
void QsMemory::setSquelchOn(bool value, int rx_num) { m_squelch_switch[rx_num] = value; }

bool QsMemory::getSquelchOn(int rx_num) { return m_squelch_switch[rx_num]; }

void QsMemory::setSquelchThreshold(double value, int rx_num) { m_squelch_threshold[rx_num] = value; }

double QsMemory::getSquelchThreshold(int rx_num) { return m_squelch_threshold[rx_num]; }

//***************************************************//
//-----------------RX LO TUNED FREQ------------------//
//***************************************************//

void QsMemory::setRxLOFrequency(double value, int rx_num) { m_rx_frequency[rx_num] = value; }

double QsMemory::getRxLOFrequency(int rx_num) { return m_rx_frequency[rx_num]; }

//***************************************************//
//-----------------DISPL FREQ OFFSET-----------------//
//***************************************************//

void QsMemory::setDisplayFreqOffset(double value, int rx_num) { m_display_freq_offset[rx_num] = value; }

double QsMemory::getDisplayFreqOffset(int rx_num) { return m_display_freq_offset[rx_num]; }

//***************************************************//
//---------------------NOTCH-------------------------//
//***************************************************//
void QsMemory::setNotchFrequency(int number, float f0, int rx_num) { m_notch_frequency[rx_num][number] = f0; }

float QsMemory::getNotchFrequency(int number, int rx_num) { return m_notch_frequency[rx_num][number]; }

void QsMemory::setNotchBandwidth(int number, float hz, int rx_num) { m_notch_bandwidth[rx_num][number] = hz; }

float QsMemory::getNotchBandwidth(int number, int rx_num) { return m_notch_bandwidth[rx_num][number]; }

void QsMemory::setNotchEnabled(int number, bool value, int rx_num) { m_notch_enabled[rx_num][number] = value; }

bool QsMemory::getNotchEnabled(int number, int rx_num) { return m_notch_enabled[rx_num][number]; }

//***************************************************//
//-------------------PS CORRECTIONS------------------//
//***************************************************//

void QsMemory::setSMeterCorrection(double value) { m_smeter_correction = value; }

double QsMemory::getSMeterCorrection() { return m_smeter_correction; }

void QsMemory::setPsCorrection(double value) { m_ps_correction = value; }

double QsMemory::getPsCorrection() { return m_ps_correction; }

void QsMemory::setPostPsCorrection(double value) { m_post_ps_correction = value; }

double QsMemory::getPostPsCorrection() { return m_post_ps_correction; }

void QsMemory::setWBPsCorrection(double value) { m_wb_correction = value; }

double QsMemory::getWBPsCorrection() { return m_wb_correction; }

//***************************************************//
//---------------------ENC CLOCK---------------------//
//***************************************************//

void QsMemory::setEncodeFreqCorrect(double value) {
    if (m_enc_clock_correction != value) {
        m_enc_clock_correction = value;
        clockCorrectionChanged(value);
    }
}

double QsMemory::getEncodeFreqCorrect() { return m_enc_clock_correction; }

void QsMemory::setEncodeClockFrequency(double value) { m_enc_clock_freq = value; }

double QsMemory::getEncodeClockFrequency() { return m_enc_clock_freq; }

//***************************************************//
//------------------DATA RATES-----------------------//
//***************************************************//

void QsMemory::setDataProcRate(double value) { m_data_proc_rate = value; }

double QsMemory::getDataProcRate() { return m_data_proc_rate; }

void QsMemory::setDataPostProcRate(double value) { m_data_post_proc_rate = value; }

double QsMemory::getDataPostProcRate() { return m_data_post_proc_rate; }

void QsMemory::setRtAudioRate(double value) { m_rt_audio_rate = value; }

double QsMemory::getRtAudioRate() { return m_rt_audio_rate; }

//***************************************************//
//----------------------BYPASS-----------------------//
//***************************************************//

void QsMemory::setRtAudioBypass(bool value) { m_rt_audio_bypass = value; }

bool QsMemory::getRtAudioBypass() { return m_rt_audio_bypass; }

void QsMemory::setDacBypass(bool value) { m_dac_bypass = value; }

bool QsMemory::getDacBypass() { return m_dac_bypass; }

//***************************************************//
//-------------------ADC CNTL------------------------//
//***************************************************//

void QsMemory::setAdcPgaOn(bool value) { m_adc_pga_on = value; }

bool QsMemory::getAdcPgaOn() { return m_adc_pga_on; }

void QsMemory::setAdcRandomOn(bool value) { m_adc_rand_on = value; }

bool QsMemory::getAdcRandomOn() { return m_adc_rand_on; }

void QsMemory::setAdcDitherOn(bool value) { m_adc_dith_on = value; }

bool QsMemory::getAdcDitherOn() { return m_adc_dith_on; }

//***************************************************//
//------------------RT AUDIO FRAMES------------------//
//***************************************************//

void QsMemory::setRtAudioFrames(int value) { m_rt_audio_frames = value; }

int QsMemory::getRtAudioFrames() { return m_rt_audio_frames; }

//***************************************************//
//--------------------WAV FILES----------------------//
//***************************************************//

void QsMemory::setWavRecContinuous(bool value) { m_wav_rec_continuous = value; }

bool QsMemory::getWavRecContinuous() { return m_wav_rec_continuous; }

void QsMemory::setWavRecordPrebufferOn(bool value) { m_wav_rec_prebuffer = value; }

bool QsMemory::getWavRecordPrebufferOn() { return m_wav_rec_prebuffer; }

void QsMemory::setWavRecordPrebufferTime(double value) { m_wav_rec_prebuffer_time = value; }

double QsMemory::getWavRecordPrebufferTime() { return m_wav_rec_prebuffer_time; }

void QsMemory::setWavRecordPath(QString value) { m_wav_rec_path = value; }

QString QsMemory::getWavRecordPath() { return m_wav_rec_path; }

void QsMemory::setWavPlaybackStartTime(time_t value) { m_wav_play_starttime = value; }

time_t QsMemory::getWavPlaybackStartTime() { return m_wav_play_starttime; }

void QsMemory::setWavInFileName(std::string value) { m_wav_in_filename = value; }

std::string QsMemory::getWavInFileName() { return m_wav_in_filename; }

void QsMemory::setWavInLooping(bool value) { m_wav_in_loop = value; }

bool QsMemory::getWavInLooping() { return m_wav_in_loop; }

//***************************************************//
//------------------BLOCK SIZES----------------------//
//***************************************************//

void QsMemory::setReadBlockSize(int value) { m_read_block_size = value; }

int QsMemory::getReadBlockSize() { return m_read_block_size; }

void QsMemory::setPsBlockSize(int value) { m_ps_block_size = value; }

int QsMemory::getPsBlockSize() { return m_ps_block_size; }

void QsMemory::setDACBlockSize(int value) { m_dac_block_size = value; }

int QsMemory::getDACBlockSize() { return m_dac_block_size; }

void QsMemory::setTxBlockSize(int size) { m_tx_block_size = size; }

int QsMemory::getTxBlockSize() { return m_tx_block_size; }

//***************************************************//
//---------------RESAMPLER QUALITY-------------------//
//***************************************************//

void QsMemory::setResamplerQuality(int value) { m_resampler_quality = value; }

int QsMemory::getResamplerQuality() { return m_resampler_quality; }

//***************************************************//
//------------------RESAMPLER RATE-------------------//
//***************************************************//

void QsMemory::setResamplerRate(double value) { m_resampler_rate = value; }

double QsMemory::getResamplerRate() { return m_resampler_rate; }