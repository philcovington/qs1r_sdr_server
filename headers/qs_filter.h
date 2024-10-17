/**
 * @file qs_filter.h
 * @brief Provides a class for implementing digital filtering operations.
 * 
 * The QsFilter class allows the creation and application of various types of FIR filters,
 * including low-pass, band-pass, and band-stop filters. It utilizes the Fast Fourier Transform
 * (FFT) for efficient filtering operations and supports multiple windowing functions.
 * 
 * Features:
 * - Support for low-pass, band-pass, and band-stop filters.
 * - Customizable filter properties such as cutoff frequency and sample rate.
 * - Multiple overloads for filtering functions to handle different input types.
 * 
 * Usage:
 * To use this class, create an instance of QsFilter, initialize it with the desired filter type 
 * and sample rate, and then call the filtering method with the input data.
 * 
 * Notes:
 * Ensure to call the init() method to set up the filter parameters before using the filtering functions.
 * 
 * Author: Philip A Covington
 * Date: 2024-10-17
 */

#pragma once

#include "../headers/qs_dataproc.h"
#include "../headers/qs_defines.h"
#include "../headers/qs_fft.h"
#include "../headers/stringclass.h"

#include <algorithm>
#include <iterator>
#include <memory>

#define BLACKMANHARRIS_WINDOW 12

class QsFilter {

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

    void MakeFirLowpass(float cutoff, float samplerate, int wtype, qs_vect_f &taps_re, qs_vect_f &taps_im, int length);

    void MakeFirBandpass(float lo, float hi, float samplerate, int wtype, qs_vect_f &taps_re, qs_vect_f &taps_im,
                         int length);

    void MakeFirBandstop(float lo, float hi, float samplerate, int wtype, qs_vect_f &taps_re, qs_vect_f &taps_im,
                         int length);

    void MakeFilter(float lo, float hi, int ftype);

    int m_filter_type;

  public:
    explicit QsFilter();

    void doFilter(qs_vect_cpx &src_dst);
    void doFilter(qs_vect_cpx &src, qs_vect_cpx &dst);
    void doFilter(qs_vect_f &src_dst_re, qs_vect_f &src_dst_im);
    void doFilter(qs_vect_f &src, qs_vect_cpx &dst);
    void doFilter(qs_vect_f &src, qs_vect_f &dst_re, qs_vect_f &dst_im);

    void init(String filtertype, float samplerate, int size);

    void setFilterLo(int value);
    void setFilterHi(int value);
    void setFilter(int loval, int hival);
    void setSampleRate(double value);

    int getFilterLo() const;
    int getFilterHi() const;

    static void MakeWindow(int wtype, int size, qs_vect_cpx &window);
    static void MakeWindow(int wtype, int size, qs_vect_f &window);
    static qs_vect_f MakeWindow(int wtype, int size);
    static qs_vect_cpx MakeWindowComplex(int wtype, int size);
};
