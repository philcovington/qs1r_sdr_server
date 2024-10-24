/**
 * @file QsAgc.h
 * @brief Automatic Gain Control (AGC) class for adaptive signal processing.
 *
 * This file defines the QsAgc class, which provides automatic gain control 
 * to adaptively adjust the amplitude of complex signal vectors (qs_vect_cpx).
 * The AGC algorithm supports configurable attack and decay rates, manual gain, 
 * and hang time for handling dynamic signals.
 *
 * Features:
 * - Configurable AGC attack, decay, and hang time constants
 * - Adjustable thresholds, slope, and manual gain for fine control
 * - Peak, average attack, and decay tracking for smooth gain adjustment
 * - Signal delay buffer and magnitude buffer for sample processing
 * - Adjustable sample rate and processing rate parameters
 *
 * Usage:
 * 1. Initialize an instance of QsAgc using the constructor.
 * 2. Call `init()` to set up internal parameters.
 * 3. Use `process(qs_vect_cpx &src_dst)` to apply AGC on a given complex signal vector.
 *
 * Notes:
 * - Constants such as attack, decay, and scaling factors are defined as macros for easy adjustment.
 * - The AGC algorithm modifies the input signal in place through an iterator.
 *
 * Author: Philip A Covington
 * Date: 2024-10-17
 */

#pragma once

#include "../include/qs_signalops.hpp"
#include "../include/qs_defines.hpp"

#define AGC_ATTACK_RISE_TC 0.0005
#define AGC_ATTACK_FALL_TC 0.020
#define AGC_RISEFALL_RATIO 0.3
#define AGC_RELEASE_TC 0.001
#define AGC_OUTPUT_SCALING 0.75
#define AGC_MIN_CONSTANT 1e-8
#define AGC_WINDOW_TC 0.018
#define AGC_DELAY_TC 0.015
#define AGC_MAX_MAN_GAIN 0.75
#define AGC_MAX_BUFFER_SZ 2048

class QsAgc {

  public:
    explicit QsAgc();

    void init();
    void process(qs_vect_cpx &src_dst);

  private:

    float update_avg(float avg, float value, float rise_alpha, float fall_alpha);
    int m_post_processing_rate;

    bool m_agc_use_hang;

    double m_agc_threshold;
    double m_agc_manual_gain;
    double m_agc_slope;

    int m_agc_hang_time;
    int m_agc_hang_time_set;

    double m_agc_decay;
    double m_agc_decay_set;
    double m_agc_sample_rate;

    qs_vect_cpx m_agc_signalDelayBuffer;
    qs_vect_f m_agc_magBuffer;
    qs_vect_cpx::iterator m_cpx_iterator;

    int m_agc_sigdly_ptr;
    int m_agc_hang_timer;

    float m_agc_peak;
    float m_agc_decay_avg;
    float m_agc_attack_avg;

    int m_agc_magbuffer_pos;

    double m_agc_current_gain;
    double m_agc_fixed_manual_gain;

    float m_agc_knee;
    float m_agc_gain_slope;

    double m_agc_fixed_gain;

    float m_agc_attack_rise_alpha;
    float m_agc_attack_fall_alpha;
    float m_agc_decay_rise_alpha;
    float m_agc_decay_fall_alpha;

    int m_agc_delay_samples;
    int m_agc_window_samples;
};
