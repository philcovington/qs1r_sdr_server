/**
 * @file qs_tone_generator.hpp
 * @brief This file contains the definition of the QsToneGenerator class, which generates
 *        sine waves at specified frequencies for frequency translation applications.
 *
 * @features
 * - Generates sine wave signals at defined frequencies.
 * - Supports different operational modes indicated by the QSDSPPOS enumeration.
 *
 * @usage
 * To use the QsToneGenerator class, create an instance, initialize it with the desired 
 * operating position using the `init()` method, and then call the `process()` method to 
 * generate the sine wave signals into the provided complex vector.
 *
 * @notes
 * Ensure that the input vector is correctly sized and initialized before calling the 
 * `process()` method to avoid any unexpected behavior.
 *
 * @author Philip A Covington
 * @date 2024-10-17
 */

#pragma once

#include "../include/qs_dataproc.hpp"

class QsToneGenerator {

  public:
    enum QSDSPPOS { rateDataRate = 1, ratePostDataRate = 2, rateTxDataRate = 3 };

    explicit QsToneGenerator();

    void process(qs_vect_cpx &src_dst);
    void init(QSDSPPOS pos);

  private:
    // TONE GENERATOR
    QSDSPPOS m_tg_pos;
    double m_rate;
    double m_tg_inc;
    double m_tg_osc_cos;
    double m_tg_osc_sin;
    double m_tg_osc1_re;
    double m_tg_osc1_im;
    double m_tg_lo_freq;
    double m_tg_osc_re;
    double m_tg_osc_im;

    qs_vect_cpx::iterator cpx_itr;
};
