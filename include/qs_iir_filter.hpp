/**
 * @file    qs_iir_filter.hpp
 * @brief   Infinite Impulse Response (IIR) filter class for signal processing.
 * 
 * This class provides different types of IIR filters for processing real and 
 * complex signals, including low-pass, high-pass, band-pass, and band-reject 
 * filters. It also includes a specific DC block filter for transmission.
 * 
 * Features:
 * - Apply different IIR filters to real or complex input signals.
 * - Supports multiple filter types (low-pass, high-pass, band-pass, etc.).
 * - Customizable filter parameters such as frequency and bandwidth.
 * 
 * Usage:
 * - Initialize with filter type and parameters.
 * - Call process() to apply the IIR filter to the input signal.
 * 
 * @note This class is primarily used for signal processing in DSP applications.
 * 
 * @author  Philip A Covington
 * @date    2024-10-17
 */

#pragma once

#include "../include/qs_signalops.hpp"

class QS_IIR {
  public:
    enum QSIIRTYPE { iirLowPass = 1, iirHighPass = 2, iirBandPass = 3, iirBandReject = 4, iirTxDcBlock = 5 };

    QS_IIR();

    void init(unsigned int notch_num, QSIIRTYPE type);

    void process(qs_vect_f &);
    void process(qs_vect_cpx &);

  private:
    int m_notch_num;
    QSIIRTYPE m_type;

    float m_Q;

    float m_A1;
    float m_A2;
    float m_B0;
    float m_B1;
    float m_B2;

    float m_w1a;
    float m_w2a;
    float m_w1b;
    float m_w2b;

    float m_f0Freq;
    float m_bwHz;

    void initLowPass(float f0freq, float bw_hz, float rate);
    void initHighPass(float f0freq, float bw_hz, float rate);
    void initBandPass(float f0freq, float bw_hz, float rate);
    void initBandReject(float f0freq, float bw_hz, float rate);

    qs_vect_cpx::iterator cpx_itr;
    qs_vect_f::iterator f_itr;
};
