/**
 * @file    qs_dsp_proc.hpp
 * @brief   Digital Signal Processing (DSP) processor for managing various audio processing tasks.
 *
 * This class is responsible for initializing, managing, and processing digital signals
 * through various audio processing components, including noise blankers, filters, and demodulators.
 * It utilizes libresamplerate resampling for audio signal conversion and handles buffer management
 * for efficient signal processing.
 *
 * Features:
 * - Integration with multiple DSP components such as tone generators, noise blankers, and filters.
 * - Resampling functionality to adjust audio sample rates.
 * - Thread management for real-time audio processing.
 * - Buffer management for input and output signals.
 *
 * Usage:
 * - Create an instance of QsDspProcessor and call init() to set up DSP components.
 * - Use run() to start processing audio signals and stop() to halt processing.
 * - Clear buffers using clearBuffers() as needed.
 *
 * @note This class is designed to work in real-time audio processing applications.
 *
 * @author  Philip A Covington
 * @date    2024-10-17
 */

#pragma once

#include "../include/qs_agc.hpp"
#include "../include/qs_am_demod.hpp"
#include "../include/qs_auto_notch_filter.hpp"
#include "../include/qs_avg_nb.hpp"
#include "../include/qs_blk_nb.hpp"
#include "../include/qs_debugloggerclass.hpp"
#include "../include/qs_defines.hpp"
#include "../include/qs_downcnv.hpp"
#include "../include/qs_fmn_demod.hpp"
#include "../include/qs_fmw_demod.hpp"
#include "../include/qs_globals.hpp"
#include "../include/qs_iir_filter.hpp"
#include "../include/qs_io_libusb.hpp"
#include "../include/qs_main_rx_filter.hpp"
#include "../include/qs_nr_filter.hpp"
#include "../include/qs_post_rx_filter.hpp"
#include "../include/qs_resampler.hpp"
#include "../include/qs_sam_demod.hpp"
#include "../include/qs_signalops.hpp"
#include "../include/qs_sleep.hpp"
#include "../include/qs_smeter.hpp"
#include "../include/qs_squelch.hpp"
#include "../include/qs_state.hpp"
#include "../include/qs_threading.hpp"
#include "../include/qs_tone_gen.hpp"
#include "../include/qs_volume.hpp"
#include <atomic>
#include <memory>
#include <thread>

class QsDspProcessor {
  public:
    // Unique pointers to DSP components
    std::unique_ptr<QsToneGenerator> p_tg0;
    std::unique_ptr<QsAveragingNoiseBlanker> p_anb;
    std::unique_ptr<QsBlockNoiseBlanker> p_bnb;
    std::unique_ptr<QsDownConvertor> p_downconv;
    std::unique_ptr<QsToneGenerator> p_tg1;
    std::unique_ptr<QsAgc> p_agc;
    std::unique_ptr<QsMainRxFilter> p_main_filter;
    std::unique_ptr<QsPostRxFilter> p_post_filter;
    std::unique_ptr<QsAMDemodulator> p_am;
    std::unique_ptr<QsSAMDemodulator> p_sam;
    std::unique_ptr<QsFMNDemodulator> p_fmn;
    std::unique_ptr<QsFMWDemodulator> p_fmw;
    std::unique_ptr<QsNoiseReductionFilter> p_nr;
    std::unique_ptr<QsAutoNotchFilter> p_anf;
    std::unique_ptr<QsSMeter> p_sm;
    std::unique_ptr<QsSquelch> p_sq;
    std::unique_ptr<QsVolume> p_vol;
    std::unique_ptr<QS_IIR> p_iir0;
    std::unique_ptr<QS_IIR> p_iir1;
    std::unique_ptr<QS_IIR> p_iir2;
    std::unique_ptr<QS_IIR> p_iir3;
    std::unique_ptr<QS_IIR> p_iir4;
    std::unique_ptr<QS_IIR> p_iir5;
    std::unique_ptr<QS_IIR> p_iir6;
    std::unique_ptr<QS_IIR> p_iir7;

    explicit QsDspProcessor();
    ~QsDspProcessor();

    void clearBuffers();
    void init(int rx_num = 1);
    void reinit();
    void start();
    void stop();
    bool isRunning();

  private:
    void run();

    // Member variables for DSP processing
    unsigned int m_rx_num;
    unsigned int m_bsize;
    unsigned int m_bsizeX2;
    unsigned int m_sd_buffer_size;
    unsigned int m_ps_size;
    unsigned int m_req_outframes;
    unsigned int m_outframesX2;

    std::atomic<bool> m_thread_go;
    std::atomic<bool> m_is_running;
    bool m_dac_bypass;
    bool m_rt_audio_bypass;

    double m_processing_rate;
    double m_post_processing_rate;
    double m_rs_rate;

    qs_vect_cpx in_cpx;
    qs_vect_cpx rs_cpx;
    qs_vect_f re_f;
    qs_vect_f im_f;
    qs_vect_cpx rs_cpx_n;

    QsSleep sleep;

    std::unique_ptr<Resampler> resampler;
    double m_rs_output_rate;
    double m_rs_input_rate;
    int m_rs_quality;
    qs_vect_f rs_in_interleaved;
    qs_vect_f rs_out_interleaved;

    std::thread m_thread;
    void initResampler(int size);
    void initManualNotch();
};
