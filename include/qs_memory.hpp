// qs_memory.hpp
#pragma once

#include "../include/qs_defaults.hpp"
#include "../include/qs_defines.hpp"
#include <string>

class QsMemory {

  public:
    QsMemory();

    void clockCorrectionChanged(double);

    // VOLUME
    void setVolume(double value, int rx_num = 0);
    double getVolume(int rx_num = 0);

    // AGC
    void setAgcDecaySpeed(double value, int rx_num = 0);
    double getAgcDecaySpeed(int rx_num = 0);

    void setAgcFixedGain(double value, int rx_num = 0);
    double getAgcFixedGain(int rx_num = 0);

    void setAgcThreshold(double value, int rx_num = 0);
    double getAgcThreshold(int rx_num = 0);

    void setAgcSlope(double value, int rx_num = 0);
    double getAgcSlope(int rx_num = 0);

    void setAgcHangTime(double value, int rx_num = 0);
    double getAgcHangTime(int rx_num = 0);

    void setAgcHangTimeSwitch(bool value, int rx_num = 0);
    bool getAgcHangTimeSwitch(int rx_num = 0);

    void setAgcCurrentGain(double value, int rx_num = 0);
    double getAgcCurrentGain(int rx_num = 0);

    void setAgcCurrentGainC(unsigned char value, int rx_num = 0);
    unsigned char getAgcCurrentGainC(int rx_num = 0);

    // MAIN FILTER
    void setFilterLo(int value, int rx_num = 0);
    int getFilterLo(int rx_num = 0);

    void setFilterHi(int value, int rx_num = 0);
    int getFilterHi(int rx_num = 0);

    // TONE GENERATOR
    void setToneLoFrequency(double value, int rx_num = 0);
    double getToneLoFrequency(int rx_num = 0);

    // CW OFFSET GENERATOR
    void setOffsetGeneratorFrequency(double value, int rx_num = 0);
    double getOffsetGeneratorFrequency(int rx_num = 0);

    void setDisplayFreqOffset(double value, int rx_num = 0);
    double getDisplayFreqOffset(int rx_num = 0);

    // DEMOD MODE
    void setDemodMode(QSDEMODMODE value, int rx_num = 0);
    QSDEMODMODE getDemodMode(int rx_num = 0);

    // BINAURAL
    void setBinauralMode(bool value, int rx_num = 0);
    bool getBinauralMode(int rx_num = 0);

    // AVG NOISEBLANKER
    void setAvgNoiseBlankerThreshold(double value, int rx_num = 0);
    double getAvgNoiseBlankerThreshold(int rx_num = 0);

    void setAvgNoiseBlankerOn(bool value, int rx_num = 0);
    bool getAvgNoiseBlankerOn(int rx_num = 0);

    // BLOCK NOISEBLANKER
    void setBlockNoiseBlankerThreshold(double value, int rx_num = 0);
    double getBlockNoiseBlankerThreshold(int rx_num = 0);

    void setBlockNoiseBlankerOn(bool value, int rx_num = 0);
    bool getBlockNoiseBlankerOn(int rx_num = 0);

    // ANF

    void setAutoNotchOn(bool value, int rx_num = 0);
    bool getAutoNotchOn(int rx_num = 0);

    void setAutoNotchRate(double value, int rx_num = 0);
    double getAutoNotchRate(int rx_num = 0);

    void setAutoNotchLeak(double value, int rx_num = 0);
    double getAutoNotchLeak(int rx_num = 0);

    void setAutoNotchDelay(int value, int rx_num = 0);
    int getAutoNotchDelay(int rx_num = 0);

    void setAutoNotchTaps(int value, int rx_num = 0);
    int getAutoNotchTaps(int rx_num = 0);

    // NR

    void setNoiseReductionOn(bool value, int rx_num = 0);
    bool getNoiseReductionOn(int rx_num = 0);

    void setNoiseReductionRate(double value, int rx_num = 0);
    double getNoiseReductionRate(int rx_num = 0);

    void setNoiseReductionLeak(double value, int rx_num = 0);
    double getNoiseReductionLeak(int rx_num = 0);

    void setNoiseReductionDelay(int value, int rx_num = 0);
    int getNoiseReductionDelay(int rx_num = 0);

    void setNoiseReductionTaps(int value, int rx_num = 0);
    int getNoiseReductionTaps(int rx_num = 0);

    // S METER

    void setSMeterCurrentValue(double value, int rx_num = 0);
    double getSMeterCurrentValue(int rx_num = 0);

    void setSMeterCurrentValueC(unsigned char value, int rx_num = 0);
    unsigned char getSMeterCurrentValueC(int rx_num = 0);

    // SQUELCH

    void setSquelchOn(bool value, int rx_num = 0);
    bool getSquelchOn(int rx_num = 0);

    void setSquelchThreshold(double value, int rx_num = 0);
    double getSquelchThreshold(int rx_num = 0);

    // RX FREQ

    void setRxLOFrequency(double value, int rx_num = 0);
    double getRxLOFrequency(int rx_num = 0);

    // MAN NOTCH
    void setNotchFrequency(int number, float f0, int rx_num = 0);
    float getNotchFrequency(int number, int rx_num = 0);

    void setNotchBandwidth(int number, float hz, int rx_num = 0);
    float getNotchBandwidth(int number, int rx_num = 0);

    void setNotchEnabled(int number, bool value, int rx_num = 0);
    bool getNotchEnabled(int number, int rx_num = 0);

    // not based on number of receivers follows:

    void setSMeterCorrection(double value);
    double getSMeterCorrection();

    void setPsCorrection(double value);
    double getPsCorrection();

    void setPostPsCorrection(double value);
    double getPostPsCorrection();

    void setWBPsCorrection(double value);
    double getWBPsCorrection();

    void setEncodeFreqCorrect(double value);
    double getEncodeFreqCorrect();

    // DATA PROCESSING RATE
    void setDataProcRate(double value);
    double getDataProcRate();

    // DATA POST-PROCESSING RATE
    void setDataPostProcRate(double value);
    double getDataPostProcRate();

    void setRtAudioRate(double value);
    double getRtAudioRate();

    void setRtAudioBypass(bool value);
    bool getRtAudioBypass();

    void setDacBypass(bool value);
    bool getDacBypass();

    void setAdcPgaOn(bool value);
    bool getAdcPgaOn();

    void setAdcRandomOn(bool value);
    bool getAdcRandomOn();

    void setAdcDitherOn(bool value);
    bool getAdcDitherOn();

    void setRtAudioFrames(int value);
    int getRtAudioFrames();

    void setReadBlockSize(int value);
    int getReadBlockSize();

    void setPsBlockSize(int value);
    int getPsBlockSize();

    void setDACBlockSize(int value);
    int getDACBlockSize();

    void setResamplerQuality(int value);
    int getResamplerQuality();

    void setResamplerRate(double value);
    double getResamplerRate();

    // ENCODE CLOCK FREQ

    void setEncodeClockFrequency(double value);
    double getEncodeClockFrequency();

    // TX

    void setTxLOFrequency(double value);
    double getTxLOFrequency();

    void setTxBlockSize(int size);
    int getTxBlockSize();

    void setTxFilterLo(uint32_t value);
    uint32_t getTxFilterLo();

    void setTxFilterHi(uint32_t value);
    uint32_t getTxFilterHi();

    void setTxCarrierLevel(double value);
    double getTxCarrierLevel();

    void setTxPdacLevel(uint32_t value);
    uint32_t getTxPdacLevel();

    void setCwSidetoneFreq(double value);
    double getCwSidetoneFreq();

    void setCwSidetoneVolume(double value);
    double getCwSidetoneVolume();

    void setTxOffsetFrequency(double value);
    double getTxOffsetFrequency();

    void setCwSpeed(int value);
    int getCwSpeed();

    void setCwMode(bool value);
    bool getCwMode();

    void setCwStraightKeyMode(bool value);
    bool getCwStraightKeyMode();

    void setMicGainDb(double value);
    void setMicGainDb(int value);
    double getMicGainDb();
    double getMicGainVal();

    void setGateLevelDb(double value);
    void setGateLevelDb(int value);
    double getGateLevelDb();
    double getGateLevelVal();

  private:
    double m_volume[MAX_RECEIVERS];

    // AGC
    double m_agc_decay_speed[MAX_RECEIVERS];
    double m_agc_fixed_gain[MAX_RECEIVERS];
    double m_agc_threshold[MAX_RECEIVERS];
    double m_agc_slope[MAX_RECEIVERS];
    double m_agc_hangtime[MAX_RECEIVERS];
    bool m_agc_hangtime_switch[MAX_RECEIVERS];
    double m_agc_current_gain[MAX_RECEIVERS];
    char m_agc_current_gain_c[MAX_RECEIVERS];

    // MAIN FILTER
    int m_filter_low[MAX_RECEIVERS];
    int m_filter_high[MAX_RECEIVERS];

    // TONE GENERATOR
    double m_tone_frequency[MAX_RECEIVERS];

    // CW OFFSET GENERATOR
    double m_offset_frequency[MAX_RECEIVERS];

    // DISPLAY FREQ OFFSET
    double m_display_freq_offset[MAX_RECEIVERS];

    // DEMOD MODE
    QSDEMODMODE m_demod_mode[MAX_RECEIVERS];

    // BINAURAL
    bool m_binaural_mode[MAX_RECEIVERS];

    // ANB
    double m_avg_nb_threshold[MAX_RECEIVERS];
    bool m_avg_nb_switch[MAX_RECEIVERS];

    // BNB
    double m_block_nb_threshold[MAX_RECEIVERS];
    bool m_block_nb_switch[MAX_RECEIVERS];

    // ANF
    bool m_autonotch_switch[MAX_RECEIVERS];
    double m_autonotch_rate[MAX_RECEIVERS];
    double m_autonotch_leak[MAX_RECEIVERS];
    int m_autonotch_delay[MAX_RECEIVERS];
    int m_autonotch_taps[MAX_RECEIVERS];

    // NR
    bool m_noise_reduction_switch[MAX_RECEIVERS];
    double m_noise_reduction_rate[MAX_RECEIVERS];
    double m_noise_reduction_leak[MAX_RECEIVERS];
    int m_noise_reduction_delay[MAX_RECEIVERS];
    int m_noise_reduction_taps[MAX_RECEIVERS];

    // S METER
    double m_s_meter_cv[MAX_RECEIVERS];
    unsigned char m_s_meter_cv_c[MAX_RECEIVERS];

    // SQUELCH
    bool m_squelch_switch[MAX_RECEIVERS];
    double m_squelch_threshold[MAX_RECEIVERS];

    // RX FREQ
    double m_rx_frequency[MAX_RECEIVERS];

    // TX FREQ
    double m_tx_frequency;

    // NOTCH
    double m_notch_frequency[MAX_RECEIVERS][MAX_MAN_NOTCHES];
    double m_notch_bandwidth[MAX_RECEIVERS][MAX_MAN_NOTCHES];
    bool m_notch_enabled[MAX_RECEIVERS][MAX_MAN_NOTCHES];

    double m_smeter_correction;
    double m_ps_correction;
    double m_post_ps_correction;
    double m_wb_correction;

    double m_enc_clock_correction;
    double m_data_proc_rate;
    double m_data_post_proc_rate;
    double m_rt_audio_rate;

    double m_cw_sidetone_freq;
    double m_cw_sidetone_volume;
    int m_cw_speed;
    bool m_cw_mode;
    bool m_straight_key_mode;

    bool m_rt_audio_bypass;
    bool m_dac_bypass;
    bool m_adc_pga_on;
    bool m_adc_rand_on;
    bool m_adc_dith_on;

    int m_rt_audio_frames;

    bool m_wav_rec_continuous;
    bool m_wav_rec_prebuffer;
    bool m_wav_in_loop;

    double m_wav_rec_prebuffer_time;

    std::string m_wav_rec_path;
    std::string m_wav_in_filename;

    time_t m_wav_play_starttime;

    int m_read_block_size;
    int m_ps_block_size;
    int m_dac_block_size;
    int m_tx_block_size;

    int m_resampler_quality;

    double m_resampler_rate;
    double m_enc_clock_freq;

    uint32_t m_tx_filter_low;
    uint32_t m_tx_filter_high;

    double m_tx_offset_freq;
    double m_tx_carrier_level;
    double m_mic_gain_db;
    double m_mic_gain_val;
    double m_gate_level_db;
    double m_gate_level_val;

    uint32_t m_tx_pdac_level;
};
