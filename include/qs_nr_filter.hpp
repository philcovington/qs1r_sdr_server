/**
 * @file    qs_noise_reduction_filter.h
 * @brief   Noise Reduction Filter for signal processing.
 * 
 * This class implements a noise reduction filter designed to enhance the 
 * quality of signals by reducing unwanted noise components. The filter 
 * utilizes an adaptive algorithm to dynamically adjust its coefficients 
 * based on the incoming signal characteristics, allowing for effective 
 * noise suppression while preserving the integrity of the desired signal.
 * 
 * Features:
 * - Adaptive noise reduction based on the signal content.
 * - Configurable adaptation rate and leakage for performance tuning.
 * - Flexible delay handling for varied signal processing needs.
 * 
 * Usage:
 * - Create an instance of the class and call init() to set the desired 
 *   size.
 * - Use process() to apply the noise reduction to a vector of complex signals.
 * 
 * @note This class is intended for use in digital signal processing (DSP) 
 *       applications requiring effective noise mitigation.
 * 
 * @author  Philip A Covington
 * @date    2024-10-17
 */

#pragma once

#include "../include/qs_dataproc.hpp"
#include "../include/qs_globals.hpp"

using namespace std;

class QsNoiseReductionFilter {
  private:
    bool m_nr_switch;
    int m_nr_lms_sz;
    int m_nr_delay;
    int m_nr_dl_indx;
    int m_nr_mask;
    double m_nr_adapt_rate;
    double m_nr_leakage;
    double m_nr_adapt_size;

    qs_vect_f m_nr_delay_line;
    qs_vect_f m_nr_coeff;

    qs_vect_cpx::iterator m_cpx_iterator;

  public:
    QsNoiseReductionFilter();

    void init(unsigned int size);
    void process(qs_vect_cpx &src_dst);
};
