// qs_defaults.hpp
#pragma once

#define QS_DEFAULT_DSP_BLOCKSIZE 4096
#define QS_DEFAULT_DSP_RATE 50000.0
#define QS_DEFAULT_TX_DSP_RATE 50000.0
#define QS_DEFAULT_FREQ 10e6
#define QS_DEFAULT_DISPL_FREQ_OFFSET 1.0

//****************************************************//
//-----------------------ADC--------------------------//
//****************************************************//
#define QS_DEFAULT_RAND true
#define QS_DEFAULT_DITH false
#define QS_DEFAULT_PGA true

//****************************************************//
//-----------------------DAC--------------------------//
//****************************************************//
#define QS_DEFAULT_DAC_BYPASS false
#define QS_DEFAULT_DAC_BLOCKSIZE 2048
#define QS_DEFAULT_EXT_MUTE_ENABLE false

//****************************************************//
//---------------------VOLUME-------------------------//
//****************************************************//
#define QS_DEFAULT_VOLUME -60.0

//****************************************************//
//----------------------AGC---------------------------//
//****************************************************//
#define QS_DEFAULT_AGC_MODE 1
#define QS_DEFAULT_AGC_FIXED_GAIN 70.0
#define QS_DEFAULT_AGC_THRESHOLD -90.0
#define QS_DEFAULT_AGC_ATTACK 1.0
#define QS_DEFAULT_AGC_SLOPE 1.0
#define QS_DEFAULT_AGC_LONG_DECAY 5000
#define QS_DEFAULT_AGC_SLOW_DECAY 1000
#define QS_DEFAULT_AGC_MED_DECAY 500
#define QS_DEFAULT_AGC_FAST_DECAY 100
#define QS_DEFAULT_AGC_HANGTIME 500
#define QS_DEFAULT_AGC_HANGSWITCH false

//****************************************************//
//---------------------FILTER-------------------------//
//****************************************************//
#define QS_DEFAULT_FILTER_LO 100
#define QS_DEFAULT_FILTER_HI 3000
#define QS_DEFAULT_MAIN_FILTER_SIZE 1024
#define QS_DEFAULT_POST_FILTER_SIZE 256
#define QS_DEFAULT_TX_FILT_LO 100
#define QS_DEFAULT_TX_FILT_HI 3000

//****************************************************//
//--------------------TONE GEN------------------------//
//****************************************************//
#define QS_DEFAULT_TONE_FREQ 0.0
#define QS_DEFAULT_CW_FREQ 600.0

//****************************************************//
//---------------------DEMOD--------------------------//
//****************************************************//
#define QS_DEFAULT_DEMOD_MODE 0 // dmAM
#define QS_DEFAULT_BINAURAL_MODE false

#define QS_DEFAULT_SAM_BW 500.0
#define QS_DEFAULT_SAM_LIMIT 1000.0
#define QS_DEFAULT_SAM_ALPHA 2.0
#define QS_DEFAULT_SAM_BETA 177.78
#define QS_DEFAULT_SAM_ZETA 0.15

#define QS_DEFAULT_FMN_BW 6000.0
#define QS_DEFAULT_FMN_LIMIT 8000.0
#define QS_DEFAULT_FMN_ZETA 0.707

#define QS_DEFAULT_FMW_BW 6000.0
#define QS_DEFAULT_FMW_LIMIT 15000.0
#define QS_DEFAULT_FMW_ZETA 0.707

//****************************************************//
//-----------------------MNB--------------------------//
//****************************************************//
#define QS_DEFAULT_MNB_THRESH 1
#define QS_DEFAULT_MNB_ON false

//****************************************************//
//-----------------------ANB--------------------------//
//****************************************************//
#define QS_DEFAULT_ANB_THRESH 4.0
#define QS_DEFAULT_ANB_ON false

//****************************************************//
//-----------------------BNB--------------------------//
//****************************************************//
#define QS_DEFAULT_BNB_THRESH 4.0
#define QS_DEFAULT_BNB_ON false

//****************************************************//
//-------------------AUTO NOTCH-----------------------//
//****************************************************//
#define QS_DEFAULT_AUTONOTCH_ON false
#define QS_DEFAULT_AUTONOTCH_RATE 0.002
#define QS_DEFAULT_AUTONOTCH_LEAK 0.001
#define QS_DEFAULT_AUTONOTCH_DELAY 64
#define QS_DEFAULT_AUTONOTCH_TAPS 128

//****************************************************//
//-----------------NOISE REDUCTION--------------------//
//****************************************************//
#define QS_DEFAULT_NOISERED_ON false
#define QS_DEFAULT_NOISERED_RATE 0.1
#define QS_DEFAULT_NOISERED_LEAK 1.0
#define QS_DEFAULT_NOISERED_DELAY 256
#define QS_DEFAULT_NOISERED_TAPS 512

//****************************************************//
//--------------------SQUELCH-------------------------//
//****************************************************//
#define QS_DEFAULT_SQUELCH_ON false
#define QS_DEFAULT_SQUELCH_THRESH -120.0

//****************************************************//
//----------------POWER SPECTRUM----------------------//
//****************************************************//
#define QS_DEFAULT_POST_PS_CORRECT 26.6
#define QS_DEFAULT_WB_PS_CORRECT 18.2
#define QS_DEFAULT_PS_RATE 15
#define QS_DEFAULT_PS_ON true
#define QS_DEFAULT_PS_BLOCKSIZE 4096

//****************************************************//
//------------------ENCODE FREQ-----------------------//
//****************************************************//
#define QS_DEFAULT_CLOCK_CORRECT 0.0
#define QS_DEFAULT_ENC_FREQ 125e6

//****************************************************//
//----------------------RESAMPLER---------------------//
//****************************************************//
#define QS_DEFAULT_RS_QUAL 3

//****************************************************//
//----------------------RT AUDIO----------------------//
//****************************************************//
#define QS_DEFAULT_RT_FRAMES 1024
#define QS_DEFAULT_RT_RATE 48000.0

//****************************************************//
//--------------DATA PROCESSOR SAMPLE RATE------------//
//****************************************************//
#define QS_DEFAULT_DATA_PROC_RATE 50000.0

//****************************************************//
//--------------DATA POSTPROCESSOR SAMPLE RATE--------//
//****************************************************//
#define QS_DEFAULT_DATA_POSTPROC_RATE 50000.0

//****************************************************//
//--------------FIRST DOWNSAMPLE FACTOR---------------//
//****************************************************//
#define QS_DEFAULT_FIRST_DS_FACTOR 1

//****************************************************//
//-----------------RT AUDIO RATE----------------------//
//****************************************************//
#define QS_DEFAULT_RTA_RATE 48000

//****************************************************//
//--------------RT AUDIO BYPASS-----------------------//
//****************************************************//
#define QS_DEFAULT_RT_BYPASS false

//****************************************************//
//--------------RT AUDIO DEVID------------------------//
//****************************************************//
#define QS_DEFAULT_RTA_IN_DEVID -1
#define QS_DEFAULT_RTA_OUT_DEVID -1

//****************************************************//
//--------------RT AUDIO USE DEFAULT------------------//
//****************************************************//
#define QS_DEFAULT_USE_DEFAULT_RTA_DEV true

//****************************************************//
//-----------------DAC BYPASS--------------------------//
//****************************************************//
#define QS_DEFAULT_DAC_BYPASS false

//****************************************************//
//--------------WAV FILE RECORDING--------------------//
//****************************************************//
#define QS_DEFAULT_WAV_CONT false
#define QS_DEFAULT_WAV_PREBUF false
#define QS_DEFAULT_WAV_PREBUFTIME 10.0
#define QS_DEFAULT_WAV_PATH "/SDRMAXIV Recordings/"
#define QS_DEFAULT_WAV_IN_NAME "NONE"
#define QS_DEFAULT_WAV_IN_LOOPS true

//****************************************************//
//--------------SPECTRUM OFFSET VALUE-----------------//
//****************************************************//
#define QS_DEFAULT_SPEC_OFFSET 200.0

//****************************************************//
//---------------------TX SETTINGS--------------------//
//****************************************************//
#define QS_DEFAULT_TX_BLOCKSIZE 2048
#define QS_DEFAULT_TX_CARRIER_LEVEL 0.5
#define QS_DEFAULT_TX_PDAC_LEVEL 100
#define QS_DEFAULT_CW_SIDETONE_FREQ 600.0
#define QS_DEFAULT_CW_SIDETONE_VOLUME 0.45
#define QS_DEFAULT_CW_SPEED_WPM 10
#define QS_DEFAULT_CW_MODEB true
#define QS_DEFAULT_CW_STRAIGHT_KEY false
#define QS_DEFAULT_MIC_GAIN 0.0
#define QS_DEFAULT_GATE_LEVEL -30.0
#define QS_DEFAULT_TX_DCBLOCK_F0 10.0
#define QS_DEFAULT_TX_DCBLOCK_BW 20.0
