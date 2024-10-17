/**
 * @file QsAMDemodulator.h
 * @brief AM Demodulator class for processing complex signal vectors.
 *
 * This file defines the QsAMDemodulator class, which is responsible for 
 * demodulating AM signals from a complex signal vector (qs_vect_cpx).
 *
 * Features:
 * - AM demodulation using magnitude calculation
 * - State tracking with z0 and z1 variables for AM signal correction
 * - DC correction through an adjustable alpha constant
 *
 * Usage:
 * 1. Initialize an instance of QsAMDemodulator using the constructor.
 * 2. Call `init()` to reset and initialize the demodulator.
 * 3. Use `process(qs_vect_cpx &src_dst)` to demodulate a given complex signal vector in place.
 *
 * Notes:
 * - The class operates on a complex float iterator, modifying the input vector directly.
 *
 * Author: Philip A Covington
 * Date: 2024-10-17
 */

#pragma once

#include "../headers/qs_dataproc.h"
#include "../headers/qs_globals.h"

class QsAMDemodulator {

  private:
    float m_am_mag;
    float m_am_z0;
    float m_am_z1;
    const float m_am_dc_alpha;

    qs_vect_cpx::iterator m_cpx_iterator;

  public:
    QsAMDemodulator();

    void init();
    void process(qs_vect_cpx &src_dst);
};
