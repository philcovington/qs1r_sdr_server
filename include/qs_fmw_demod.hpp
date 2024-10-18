/**
 * @file qs_fmwdemod.hpp
 * @brief QsFMWDemodulator Class Header
 * 
 * This header defines the QsFMWDemodulator class, which implements a 
 * demodulation algorithm for Frequency Modulated Wide (FMW) signals. 
 * It processes incoming complex signals to extract the original information 
 * from the modulated wave.
 * 
 * @features 
 * - Initializes parameters for FMW demodulation.
 * - Processes input complex signals to demodulate FMW.
 * 
 * @usage 
 * 1. Create an instance of the QsFMWDemodulator class.
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

class QsFMWDemodulator {
  private:
    float m_fmw_bw;
    float m_fmw_limit;
    float m_fmw_zeta; // damping factor
    float m_fmw_norm;
    float m_fmw_cos;
    float m_fmw_sin;
    float m_fmw_ncoPhase;
    float m_fmw_phaseError;
    float m_fmw_ncoFreq;
    float m_fmw_ncoHighLimit;
    float m_fmw_ncoLowLimit;
    float m_fmw_alpha;
    float m_fmw_beta;
    float m_fmw_freqDcError;
    float m_fmw_dc_alpha;
    float m_fmw_outgain;

    qs_vect_cpx::iterator m_cpx_iterator;

  public:
    QsFMWDemodulator();

    void init();
    void process(qs_vect_cpx &src_dst);
};
