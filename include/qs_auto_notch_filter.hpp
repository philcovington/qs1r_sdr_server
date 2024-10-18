/**
 * @file    qs_auto_notch_filter.h
 * @brief   Automatic Notch Filter for signal processing.
 * 
 * This class implements an automatic notch filter that is designed to 
 * suppress narrowband interference in complex input signals. The filter 
 * uses an adaptive algorithm to dynamically adjust its coefficients 
 * based on the characteristics of the incoming signal, providing effective 
 * noise cancellation without significant distortion of the desired signal.
 * 
 * Features:
 * - Automatic adaptation to varying signal conditions.
 * - Configurable adaptation rate and leakage for optimal performance.
 * - Adjustable delay and filter size for flexibility.
 * 
 * Usage:
 * - Create an instance of the class and call init() to initialize with 
 *   the desired size.
 * - Call process() with a vector of complex signals to apply the notch filtering.
 * 
 * @note This class is suitable for use in digital signal processing (DSP) 
 *       applications where interference suppression is needed.
 * 
 * @author  Philip A Covington
 * @date    2024-10-17
 */

#pragma once

#include "../include/qs_signalops.hpp"
#include "../include/qs_globals.hpp"

using namespace std;

class QsAutoNotchFilter {
  private:
    bool m_anf_switch;
    int m_anf_lms_sz;
    double m_anf_adapt_rate;
    double m_anf_leakage;
    double m_anf_adapt_size;
    int m_anf_delay;
    int m_anf_dl_indx;
    int m_anf_mask;

    qs_vect_f m_anf_delay_line;
    qs_vect_f m_anf_coeff;

    qs_vect_cpx::iterator m_cpx_iterator;

  public:
    QsAutoNotchFilter();

    void init(unsigned int size);
    void process(qs_vect_cpx &src_dst);
};
