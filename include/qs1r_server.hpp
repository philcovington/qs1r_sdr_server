#pragma once

#include "../include/qs_bytearray.hpp"
#include "../include/qs_cmdproc.hpp"
#include "../include/qs_defines.hpp"
#include "../include/qs_globals.hpp"
#include "../include/qs_signalops.hpp"
#include "../include/qs_sleep.hpp"
#include "../include/qs_stringclass.hpp"
#include "../include/qs_stringlistclass.hpp"
#include "../include/qs_threading.hpp"
#include "../include/qs_uuid.hpp"
#include "../include/qs_mapclass.hpp"
#include <memory>
#include <vector>

#include <syslog.h>
#include <unistd.h>

class QsDspProcessor;
class QsDataProcessor;
class QsDataPostProcessor;
class QsIOLib_LibUSB;
class QsSleep;
class QsState;
class QsAudio;
class SdrMaxV;
class QsFilter;
class QsFFT;
class QsIoThread;

enum DACCLKSEL { CLK_48k = 0, CLK_50k= 1};

class QS1RServer {

  public:
    QS1RServer();
    ~QS1RServer();    
    
    std::unique_ptr<QsState> p_qsState;    
    std::unique_ptr<QsIoThread> p_io_thread;

    unsigned int controlRegister0Value();
    unsigned int controlRegister1Value();
  
    bool pgaMode();
    bool randMode();
    bool ditherMode();

    StringList getSupportedSampleRates();

    void shutdown();

    // QS1E Hardware
    bool checkForQS1E();
    bool isHardwareInit();

    void PttState(bool);

    void showStartupMessage(); 
    void showStartupMessageWithReady();  
    void quit();

    void setStatusText(String text);
  
    void initQsAudio(double rate);
    int initQS1RHardware();    
    int initThreads();
    void initCircBuffers();
    void initQsMemory();
    void initSMeterCorrectionMap();

    void loadFPGAFile(String filename);
    void updateFPGARegisters();

    int setPgaMode(bool on);
    int setRandMode(bool on);
    int setDitherMode(bool on);
    void setDacOutputDisable(bool on);
    void setDacExtMuteEnable(bool on);
    void setDdcMasterReset(bool on);
    void setWideBandBypass(bool on);
    void setDDCSamplerate(int value);
    void setDacClockSelect(DACCLKSEL value);

    void setTxPdacLevel(unsigned int value);

    void clearFpgaControlRegisters();

    int initialize();

    void setupIo();
    void startIo(bool iswav = false);
    void stopIo();
    bool isDspProcessorRunning();

    void setRxFrequency(double value, int rx_num, bool force = false);
    void getRxFrequency(double &value, int rx_num);

    void setTxFrequency(double value, bool force = false);
    void getTxFrequency(double &value);

    void setFreqCorrection(double value);
    void getFreqCorrection(double &value);

    void setSMeterCorrection(double value);
    void getSMeterCorrection(double &value);

    void setFilter(double width, int rx_num=0);    

    // Board Initialization and Tests
    void findQS1RDevice();
    void loadQS1RFirmware();
    void loadQS1RFPGA();
    void writeQS1REEPROM();
    void readQS1REEPROM();

    void setSampleRateDirect(int value);
    void setTxPttDirect(bool value);

    double getRxFrequency();
    void setRxFrequency(double freq);
    String getRxMode();
    void setRxMode(String mode);
    
    bool setFpgaForSampleRate(double samplerate);    
    void updateClockCorrection(double);    
    void unregisteredHardwareTimeout();
    void initSupportedSampleRatesList();    
    void qs1rReadFailure();
    void initWBPowerSpectrum();
    void clearAllBuffers();    
    bool getDacOutputDisable();

    String getModeString(QSDEMODMODE mode);
    QSDEMODMODE modeStringToMode(String smode);
    Map<int, double> SMETERCORRECTMAP;
    double SMETERCORRECT = 0.0;

    void setSquelchOn(bool on);
    void setSquelchThreshold(double threshold);

    void setVolume(double volume);    

  private:
    bool error_flag;
    bool m_is_hardware_init;
    bool m_is_io_setup;
    bool m_is_io_running;

    bool m_is_factory_init_enabled;
    bool m_is_was_factory_init;

    bool hardware_is_registered;
    bool m_is_fpga_loaded; 

    double m_proc_samplerate;
    double m_freq_offset_rx1;
    double m_freq_offset_rx2;
    double m_step_size;
    double m_prev_vol_val;

    int m_local_rx_num_selector;

    String m_driver_type;
    ByteArray spec_ba;
    StringList cmdList;
    StringList m_supported_samplerates;

    QsSleep sleep;

    CMD cmd;

    UUID readQS1RUuid();
    bool writeQS1RSN(String uuid);
    String readQS1REEPROMData();

    int m_wb_bsize;
    int m_wb_bsizeX2;
    int m_wb_bsizeDiv2;
    float m_wb_correction;
    float m_wb_one_over_norm;

    std::unique_ptr<QsFFT> p_wb_fft;

    qs_vect_cpx wb_window;

    qs_vect_cpx wb_cpx_buf1;
    qs_vect_cpx wb_cpx_buf2;
    qs_vect_f wb_f_buf1;

    qs_vect_s wb_adc_data;
    qs_vect_f wb_adc_f;
    qs_vect_cpx wb_adc_cpx;

    float wb_mean;

    std::vector<float> wb_fvPsdBm;
    ByteArray wb_bvPsdBm;

    double estimateDownConvertorRate(double rate, double bandwidth);    
    int frequencyToPhaseIncrement(double freq);    

    int m_status_message_backing_register;

    // Results
    String result_scr_write;

    // ---------------
};
