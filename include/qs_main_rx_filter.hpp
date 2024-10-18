/**
 * @file    qs_main_rx_filter.hpp
 * @brief   Main receive filter class for complex signal processing.
 * 
 * This class implements a bandpass FIR filter for processing complex input signals 
 * in a digital signal processing (DSP) system. It provides methods to create window 
 * functions and filter taps, as well as functions for applying the filter to incoming 
 * signals. The filter supports various window types and can generate both real and 
 * complex window functions.
 * 
 * Features:
 * - Initialize and apply a bandpass FIR filter on complex signals.
 * - Generate real and complex window functions (e.g., Blackman-Harris).
 * - Dynamic filter creation based on input parameters such as frequency range and sample rate.
 * 
 * Usage:
 * - Initialize the filter with a specified size.
 * - Use `process()` to filter complex signals.
 * - Generate real or complex windows with static methods like `MakeWindow()`.
 * 
 * @note This class is used in digital signal processing applications within the QS system.
 * 
 * @author  Philip A Covington
 * @date    2024-10-17
 */

#pragma once

#include "../include/qs_signalops.hpp"
#include "../include/qs_defines.hpp"
#include "../include/qs_fft.hpp"
#include "../include/qs_globals.hpp"
#include "../include/qs_stringclass.hpp"

#include <algorithm>
#include <iterator>
#include <memory>

#define BLACKMANHARRIS_WINDOW 12

class QsMainRxFilter {

  public:
    QsMainRxFilter();

    void init(int size);
    void process(qs_vect_cpx &src_dst);

    static void MakeWindow(int wtype, int size, qs_vect_cpx &window);
    static void MakeWindow(int wtype, int size, qs_vect_f &window);
    static qs_vect_f MakeWindow(int wtype, int size);
    static qs_vect_cpx MakeWindowComplex(int wtype, int size);

  private:
    int m_size;
    float m_samplerate;
    int m_filter_lo;
    int m_filter_hi;
    float m_one_over_norm;

    std::unique_ptr<QsFFT> p_ovlpfft;
    std::unique_ptr<QsFFT> p_filtfft;

    qs_vect_f tmpfilt0_re;
    qs_vect_f tmpfilt0_im;
    qs_vect_cpx cpx_0;
    qs_vect_cpx cpx_1;
    qs_vect_cpx ovlp;
    qs_vect_cpx filt_cpx0;

    void MakeFirBandpass(float lo, float hi, float samplerate, int wtype, qs_vect_f &taps_re, qs_vect_f &taps_im,
                         int length);

    void MakeFilter(float lo, float hi);
};
