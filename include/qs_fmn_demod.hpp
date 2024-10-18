/**
 * @file qs_fmn_demod.hpp
 * @brief QsFMNDemodulator Class Header
 *
 * This header defines the QsFMNDemodulator class, which implements a
 * demodulation algorithm for Frequency Modulated Narrow (FMN) signals.
 * It utilizes various parameters for tuning and processing FMN signals
 * to extract the original information from the modulated signal.
 *
 * @features
 * - Initializes key parameters for FMN demodulation.
 * - Processes input complex signals to demodulate FMN.
 *
 * @usage
 * 1. Create an instance of the QsFMNDemodulator class.
 * 2. Call the `init()` method to set up the initial parameters.
 * 3. Use the `process()` method to demodulate incoming signals.
 *
 * @notes
 * - Ensure that the data processing rate is correctly set in
 *   the QsGlobal memory before initialization.
 * - This class requires the `qs_globals.hpp` and `qs_signalops.hpp`
 *   headers to function correctly.
 *
 * @author Philip A Covington
 * @date 2024-10-16
 */

#pragma once

#include "../include/qs_signalops.hpp"
#include "../include/qs_globals.hpp"

using namespace std;

class QsFMNDemodulator {
  private:
    float m_fmn_bw;
    float m_fmn_limit;
    float m_fmn_zeta; // damping factor
    float m_fmn_norm;
    float m_fmn_cos;
    float m_fmn_sin;
    float m_fmn_ncoPhase;
    float m_fmn_phaseError;
    float m_fmn_ncoFreq;
    float m_fmn_ncoHighLimit;
    float m_fmn_ncoLowLimit;
    float m_fmn_alpha;
    float m_fmn_beta;
    float m_fmn_freqDcError;
    float m_fmn_dc_alpha;
    float m_fmn_outgain;

    qs_vect_cpx::iterator m_cpx_iterator;

  public:
    QsFMNDemodulator();

    void init();
    void process(qs_vect_cpx &src_dst);
};
