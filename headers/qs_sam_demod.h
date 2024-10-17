/**
 * @file QsSAMDemodulator.h
 * @brief Synchronous Amplitude Modulation (SAM) Demodulator
 * 
 * This class implements a synchronous demodulator for amplitude modulation signals. 
 * It processes complex input samples to extract the modulated information, using
 * a set of internal parameters that define the demodulation characteristics.
 * 
 * Features:
 * - Adjustable bandwidth and limits for the demodulation process.
 * - Calculation of phase errors and normalization of output.
 * - Utilizes iterative processing over complex vectors.
 * 
 * Usage:
 * - Initialize the demodulator with the `init()` method.
 * - Process the input signal using the `process()` method, which modifies the 
 *   provided vector in place.
 * 
 * Notes:
 * - Ensure that the input vector is populated with valid complex samples before 
 *   invoking the `process()` method.
 * 
 * Author: Philip A Covington
 * Date: 202-10-17
 */

#pragma once

#include "../headers/qs_dataproc.h"
#include "../headers/qs_globals.h"

using namespace std;

class QsSAMDemodulator {

  private:
    // SAM DEMOD
    float m_sam_bw;
    float m_sam_limit;
    float m_sam_zeta;
    float m_sam_alpha_constant;
    float m_sam_beta_constant;
    float m_sam_norm;
    float m_sam_cos;
    float m_sam_sin;
    float m_sam_ncoPhase;
    float m_sam_phaseError;
    float m_sam_ncoFreq;
    float m_sam_ncoHighLimit;
    float m_sam_ncoLowLimit;
    float m_sam_alpha;
    float m_sam_beta;
    float m_sam_mag;
    float m_sam_atan;
    float m_sam_z0;
    float m_sam_z1;
    float m_sam_y0;
    float m_sam_y1;
    float m_sam_dc_alpha;

    qs_vect_cpx::iterator m_cpx_vect_iterator;

  public:
    explicit QsSAMDemodulator();

    void init();
    void process(qs_vect_cpx &src_dst);
};
