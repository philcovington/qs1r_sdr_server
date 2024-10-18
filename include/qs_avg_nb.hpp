/**
 * @file    qs_avg_nb.hpp
 * @brief   Averaging Noise Blanker for signal processing.
 * 
 * This class implements an averaging noise blanker that processes complex 
 * input signals to suppress noise. It calculates an average signal based 
 * on incoming samples and applies a threshold to determine if the signal 
 * exceeds the noise level. If the incoming signal magnitude exceeds the 
 * specified threshold, it replaces the noisy signal with the averaged signal.
 * 
 * Features:
 * - Real-time processing of complex signals.
 * - Adjustable threshold for noise blanking.
 * - Averaging mechanism to adaptively filter out noise.
 * 
 * Usage:
 * - Create an instance of the class and call init() to initialize.
 * - Call process() with a vector of complex signals to apply the noise blanking.
 * 
 * @note This class is designed for use in digital signal processing (DSP) applications.
 * 
 * @author  Philip A Covington
 * @date    2024-10-17
 */

#pragma once

#include "../include/qs_signalops.hpp"
#include "../include/qs_globals.hpp"

using namespace std;

class QsAveragingNoiseBlanker {
  private:
    Cpx m_anb_avg_sig;
    float m_anb_magnitude;
    float m_anb_avg_magn;
    bool m_anb_switch;
    double m_anb_thres;

    qs_vect_cpx::iterator m_cpx_iterator;

  public:
    QsAveragingNoiseBlanker();

    void init();
    void process(qs_vect_cpx &src_dst);
};
