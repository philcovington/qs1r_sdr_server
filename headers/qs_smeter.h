/**
 * @file qs_smeter.h
 * @brief This file contains the definition of the QsSMeter class, which is responsible for
 *        processing and measuring complex signal data.
 *
 * @features
 * - Provides functionality to initialize and process complex vector data.
 *
 * @usage
 * To use the QsSMeter class, create an instance, call the `init()` method to set up any 
 * required parameters, and then use the `process()` method to perform measurements 
 * on the provided complex vector data.
 *
 * @notes
 * Ensure that the input vector is properly initialized before calling the `process()` method.
 *
 * @author Philip A Covington
 * @date 2024-10-17
 */

#pragma once

#include "../headers/qs_dataproc.h"
#include "../headers/qs_globals.h"

using namespace std;

class QsSMeter {
  private:
    float m_sm_tmp_val;
    double m_sm_value;

    qs_vect_cpx::iterator m_cpx_iterator;

  public:
    QsSMeter();

    void init();
    void process(qs_vect_cpx &src_dst);
};
