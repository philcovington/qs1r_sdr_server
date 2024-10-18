/**
 * @file    qs_hilbert.hpp
 * @brief   Hilbert transform filter for signal processing.
 * 
 * This class performs a Hilbert transform on input signals, converting real 
 * input samples into complex output samples or processing complex input 
 * signals. It uses a finite impulse response (FIR) filter with a specified 
 * number of taps.
 * 
 * Features:
 * - Apply Hilbert transform to real or complex input signals.
 * - Supports customizable filter taps for the transform.
 * 
 * Usage:
 * - Initialize with the number of taps.
 * - Call process() to transform real or complex input into complex output.
 * 
 * @note This class is primarily used for signal processing in DSP applications.
 * 
 * @author  Philip A Covington
 * @date    2024-10-17
 */
#pragma once

#include "../include/qs_signalops.hpp"

class QsHilbert {

  public:
    explicit QsHilbert(int ntaps);

    int process(qs_vect_f &in_f, qs_vect_cpx &out_cpx, unsigned int length);
    int process(qs_vect_cpx &in_cpx, qs_vect_cpx &out_cpx, unsigned int length);

  private:
    float filter(const float input[]);
    qs_vect_f MakeHilbertTaps(int wtype, unsigned int length);
    qs_vect_f m_taps;
};
