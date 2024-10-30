/*
 * File: qs_am_demod.hpp
 * Overview: This header file defines the QsAMDemodulator class for amplitude modulation (AM) demodulation.
 * Features:
 *   - Computes the magnitude of complex input samples.
 *   - Applies a DC removal filter to eliminate low-frequency noise.
 *   - Optional post-demodulation low-pass filtering to smooth the output signal.
 * Usage:
 *   - Initialize the demodulator with the init() method before processing.
 *   - Call the process() method with a vector of complex samples to perform demodulation.
 * Notes:
 *   - The low-pass filter parameters can be adjusted based on the desired output characteristics.
 *   - A smaller alpha lowers the cutoff frequency, filtering more of the high-frequency noise, while a larger alpha
 * allows higher frequencies to pass through. 
 * 
 * Author: Philip A Covington 
 * Date: 2024-10-30
 */

#pragma once

#include "../include/qs_globals.hpp"
#include "../include/qs_signalops.hpp"
#include "../include/qs_types.hpp"

class QsAMDemodulator {

  private:
    float m_am_mag;
    float m_am_z0;
    float m_am_z1;
    const float m_am_dc_alpha;
    float m_alpha;
    bool m_is_init = false;

    qs_vect_cpx::iterator m_cpx_iterator;

  public:
    QsAMDemodulator();

    void init();
    void process(qs_vect_cpx &src_dst);
};
