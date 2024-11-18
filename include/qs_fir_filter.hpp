/**
 * @file    qs_post_rx_filter.hpp
 * @brief   Post-receive filter class for signal processing.
 * 
 * This class provides functionality for applying various filtering techniques
 * on complex input signals after reception. It supports the creation of different
 * window types and includes methods for signal processing and bandpass filtering.
 * 
 * Features:
 * - Implements various windowing functions, including Blackman-Harris.
 * - Capable of processing complex signals with customizable filter parameters.
 * 
 * Usage:
 * - Initialize with the desired size.
 * - Use MakeWindow methods to create filtering windows.
 * - Call process() to filter incoming complex signals.
 * 
 * @note This class is designed for use in digital signal processing applications
 *       following signal reception.
 * 
 * @author  Philip A Covington
 * @date    2024-10-17
 */

#pragma once

#include "../include/qs_fft.hpp"
#include "../include/qs_globals.hpp"
#include "../include/qs_types.hpp"
#include <algorithm>
#include <iterator>
#include <memory>

#define BLACKMANHARRIS_WINDOW 12

class QsFirFilter {

  public:
    QsFirFilter();

    void init(double lo_freq, double hi_freq, int rate, int size);
    void process(qs_vect_cpx &src_dst);

    static void MakeWindow(int wtype, int size, qs_vect_cpx &window);
    static void MakeWindow(int wtype, int size, qs_vect_f &window);
    static qs_vect_f MakeWindow(int wtype, int size);
    static qs_vect_cpx MakeWindowComplex(int wtype, int size);

	void setFilter(double lo_freq, double hi_freq);
  void setFilterLo(double lo_freq);
  void setFilterHi(double hi_freq);
  double getFilterLo();
  double getFilterHi();

  private:
    int m_size;
    float m_samplerate;
    int m_filter_lo;
    int m_filter_hi;
    float m_one_over_norm;
    bool m_is_init = false;

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
