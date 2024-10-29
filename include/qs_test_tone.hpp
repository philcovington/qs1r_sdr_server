/**
 * @file qs_testtone.hpp
 * @brief A class for generating a sine wave test tone.
 *
 * This class provides functionality to generate a sine wave test tone 
 * with a specified frequency, amplitude, and sample rate. It can be used 
 * for testing audio systems and verifying signal processing algorithms.
 *
 * Features:
 * - Initialization with configurable frequency, amplitude, and sample rate.
 * - Functions to set the frequency and amplitude of the generated tone.
 * - A `process` function that generates the test tone and adds it to an 
 *   input audio signal.
 *
 * Usage:
 * 
 *  QsTestTone testTone;
 *  testTone.init(440.0f, 0.5f, 48000); // Initialize with A440, 0.5 amplitude, 48kHz sample rate
 *  qs_vect_f audioData = qs_vect_f(1024); // Create a vector to hold audio data
 *  testTone.process(audioData); // Fill the vector with the test tone
 *  testTone.setFrequency(880.0f); // Change the frequency to A880
 *  testTone.process(audioData); // Generate a new tone with the updated frequency
 * 
 * Author: Philip A Covington
 * Date: 2024-10-29
 */

#pragma once

#include "../include/qs_globals.hpp"
#include "../include/qs_signalops.hpp"
#include "../include/qs_types.hpp"

class QsTestTone {
  public:
    void init(float frequency=1000.0f, float amplitude=0.1f, uint32_t samplerate=50000);
    void setFrequency(float frequency);
    void setAmplitude(float amplitude);
    void process(qs_vect_f &src_dst);

  private:
    float m_tt_amplitude = 0.1f;
    float m_tt_frequency = 1000.0f;
    float m_tt_phase = 0;
    float m_tt_phaseIncrement = 0;
    uint32_t m_tt_samplerate = 50000;
    qs_vect_f::iterator f_itr;
};