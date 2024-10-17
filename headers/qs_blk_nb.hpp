/**
 * @file    qs_blk_nb.hpp
 * @brief   Block Noise Blanker for signal processing.
 * 
 * This class implements a block noise blanker designed to suppress noise 
 * in complex input signals. It utilizes a delay line to maintain a history 
 * of incoming samples and applies a threshold to determine if the signal 
 * should be blanked. If the signal magnitude exceeds the specified threshold, 
 * it replaces the noisy signal with an averaged signal based on previous samples.
 * 
 * Features:
 * - Real-time processing of complex signals with noise suppression.
 * - Adjustable threshold and hangtime for blanking.
 * - Ability to enable or disable the noise blanker.
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

#include "../headers/qs_dataproc.h"
#include "../headers/qs_globals.h"

class QsBlockNoiseBlanker {
  private:
    float m_bnb_magnitude;
    float m_bnb_avg_magn;
    bool m_bnb_switch;
    double m_bnb_thres;
    int m_bnb_sig_index;
    int m_bnb_dly_index;
    int m_bnb_hangtime;
    Cpx m_bnb_avg_sig;

    qs_vect_cpx bnb_delay_line;

    qs_vect_cpx::iterator m_cpx_iterator;

  public:
    QsBlockNoiseBlanker();

    void init();
    void process(qs_vect_cpx &src_dst);

    void setBnbOn(bool value);
    void getBnbOn(bool &value);
    void setBnbThreshold(double level);
    void getBnbThreshold(double &value);
};
