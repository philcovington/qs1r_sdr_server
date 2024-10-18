#pragma once

#include "../include/qs_defaults.hpp"
#include "../include/qs_defines.hpp"
#include "../include/qs_stringclass.hpp"
#include "../include/qs_settingsclass.hpp"
#include <memory>

using namespace std;

class QsState {
  private:
    std::unique_ptr<Settings> settings;

    bool m_rand_is_on;
    bool m_dith_is_on;
    bool m_pga_is_on;
    bool m_dac_bypass_is_on;
    bool m_rta_audio_bypass;
    bool m_ext_mute_enable_is_on;

    int m_block_size;
    int m_ps_block_size;
    int m_tx_block_size;
    int m_rs_quality;
    int m_rta_audio_frames;
    int m_main_filter_taps;
    int m_rta_in_dev_id;
    int m_rta_out_dev_id;

    double m_startup_sample_rate;
    double m_startup_freq;
    int m_startup_filter_low;
    int m_startup_filter_high;
    QSDEMODMODE m_startup_mode;
    double m_startup_volume;
    double m_startup_agc_decay_speed;
    double m_startup_agc_threshold;

    double m_clock_correction;
    double m_encode_clk_freq;
    double m_smeter_correction;
    double m_ps_correction;
    double m_post_ps_correction;
    double m_wb_ps_correction;

    String m_wav_record_path;

  public:
    QsState();

    void init();

    bool rand();
    bool pga();
    bool dith();
    bool dacBypass();
    bool rtAudioBypass();
    bool extMuteEnable();

    int blockSize();
    int psBlockSize();
    int txBlockSize();
    int rsQual();
    int rtAudioFrameSize();
    int mainFilterTapSize();
    int rtAudioInDevId();
    int rtAudioOutDevId();

    double startupSampleRate();
    double startupFrequency();
    int startupFilterLow();
    int startupFilterHigh();
    QSDEMODMODE startupMode();
    double startupVolume();
    double startupAGCDecaySpeed();
    double startupAGCThreshold();

    double clockCorrection();
    double encodeClockFrequency();
    double smeterCorrection();
    double powerSpecCorrection();
    double postPsCorrection();
    double wbPsCorrection();

    String wavRecordPath();

    void readSettings();
    void setBlockSize(int blocksz);
    void setPsBlockSize(int blocksz);
    void setTxBlockSize(int blocksz);

    void setStartupSampleRate(double value);
    void setStartupFrequency(double value);
    void setStartupFilterLow(int value);
    void setStartupFilterHigh(int value);
    void setStartupMode(QSDEMODMODE value);
    void setStartupVolume(double value);
    void setStartupAGCDecaySpeed(double value);
    void setStartupAGCThreshold(double value);

    void setPGA(bool on);
    void setRAND(bool on);
    void setDITH(bool on);
    void setClockCorrection(double value);
    void setSMeterCorrection(double value);
    void setPowerSpecCorrection(double value);
    void setPostPsCorrection(double value);
    void setWBPsCorrection(double value);
    void setMainFilterTapSize(int value);
    void setDacBypass(bool on);
    void setRtAudioBypass(bool on);
    void setRtAudioInDevId(int value);
    void setRtAudioOutDevId(int value);
    void setExtMuteEnable(bool on);
    void setWavRecordPath(String value);
};
