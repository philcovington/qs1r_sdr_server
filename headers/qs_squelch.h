/**
 * @file QsSquelch.h
 * @brief A class for implementing squelch functionality in signal processing.
 *
 * The QsSquelch class provides methods to manage squelch operations,
 * which are used to suppress noise in audio or signal processing applications.
 * The class allows for the initialization of squelch parameters and processing
 * of input data based on the configured threshold.
 *
 * Features:
 * - Enable or disable squelch functionality.
 * - Set and get squelch threshold for signal processing.
 * - Process a vector of complex signals, applying squelch logic.
 *
 * Usage:
 * To use the QsSquelch class, create an instance, initialize it, and then
 * call the process method with the source/destination vector:
 *
 *   QsSquelch squelch;
 *   squelch.init();                   // Initialize squelch parameters
 *   squelch.process(src_dst);         // Process signals with squelch logic
 *
 * This class is useful in scenarios where it is necessary to filter out
 * unwanted signals or noise below a certain threshold in communication systems
 * or audio processing.
 *
 * Author: [Philip A Covington]
 * Date: [2024-10-16]
 */

#pragma once

#include "../headers/qs_dataproc.h"
#include "../headers/qs_globals.h"

class QsSquelch {
  private:
    // SQUELCH
    bool m_sq_switch;
    double m_sq_thresh;
    double m_sq_hist;

  public:
    QsSquelch();

    void init();
    void process(qs_vect_cpx &src_dst);
};