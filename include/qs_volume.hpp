/*
 * QsVolume.h
 * 
 * Overview:
 * This class provides functionality to adjust the amplitude (or volume) of complex signals
 * and floating-point signals. It allows for dynamic volume control in decibels.
 * 
 * Features:
 * - Adjusts amplitude of complex signals represented as `qs_vect_cpx`.
 * - Adjusts amplitude of real-valued signals represented as `qs_vect_f`.
 * 
 * Usage:
 * - Create an instance of `QsVolume` and use the `process` methods to modify the amplitude
 *   of input signals.
 * 
 * Notes:
 * - Volume adjustments are based on a logarithmic scale (decibels).
 * - Ensure that the input signals are properly initialized before processing.
 * 
 * Author: Philip A Covington
 * Date: 2024-10-17
 */

#pragma once

#include "../include/qs_dataproc.hpp"
#include "../include/qs_globals.hpp"

using namespace std;

class QsVolume {

  private:
    float m_volume_db;
    float m_volume_val;

    qs_vect_cpx::iterator cpx_itr;
    qs_vect_f::iterator f_itr;

  public:
    QsVolume();

    void process(qs_vect_cpx &src_dst);
    void process(qs_vect_f &src_dst);
};
