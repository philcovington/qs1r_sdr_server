/**
 * @file qs_squelch.hpp
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
 * Author: Philip A Covington
 * Date: 2024-10-16
 */

#pragma once

#include "../include/qs_bandpass_filter.hpp"
#include "../include/qs_butterworth_lowpass.hpp"
#include "../include/qs_globals.hpp"
#include "../include/qs_signalops.hpp"
#include <deque>
#include <string>
#include <unordered_map>

enum class CtcssTone {
    TONE_67_0,
    TONE_69_3,
    TONE_71_9,
    TONE_74_4,
    TONE_77_0,
    TONE_79_7,
    TONE_82_5,
    TONE_85_4,
    TONE_88_5,
    TONE_91_5,
    TONE_94_8,
    TONE_97_4,
    TONE_100_0,
    TONE_103_5,
    TONE_107_2,
    TONE_110_9,
    TONE_114_8,
    TONE_118_8,
    TONE_123_0,
    TONE_127_3,
    TONE_131_8,
    TONE_136_5,
    TONE_141_3,
    TONE_146_2,
    TONE_151_4,
    TONE_156_7,
    TONE_159_8,
    TONE_162_2,
    TONE_165_5,
    TONE_167_9,
    TONE_171_3,
    TONE_173_8,
    TONE_177_3,
    TONE_179_9,
    TONE_183_5,
    TONE_186_2,
    TONE_189_9,
    TONE_192_8,
    TONE_196_6,
    TONE_199_5,
    TONE_203_5,
    TONE_206_5,
    TONE_210_7,
    TONE_218_1,
    TONE_225_7,
    TONE_229_1,
    TONE_233_6,
    TONE_241_8,
    TONE_250_3,
    TONE_254_1,
    TONE_NONE = 9999
};

class QsSquelch {
  private:
    bool detectTone(qs_vect_cpx &src_dst);

    // SQUELCH
    bool m_sq_switch;
    double m_sq_thresh;
    double m_sq_hist;
    double m_sq_hysteresis;
    double m_attack;
    double m_decay;

    ButterworthLowPass m_lowPassFilter;
    BandPassFilter m_bandpassFilter;

    size_t m_blocksize;
    double m_sampleRate;     // Sample rate of the audio (e.g., 48000 Hz)
    double m_ctcss_tone;     // CTCSS tone frequency to detect
    bool m_squelchOpen;      // Current squelch state
    double m_tone_threshold; // Detection threshold
    double m_magnitude;

    bool m_isToneDetected = false;
    bool m_is_init = false;

  public:
    QsSquelch();

    const std::unordered_map<CtcssTone, double> CtcssToneFrequencies = {
        {CtcssTone::TONE_67_0, 67.0},   {CtcssTone::TONE_69_3, 69.3},   {CtcssTone::TONE_71_9, 71.9},
        {CtcssTone::TONE_74_4, 74.4},   {CtcssTone::TONE_77_0, 77.0},   {CtcssTone::TONE_79_7, 79.7},
        {CtcssTone::TONE_82_5, 82.5},   {CtcssTone::TONE_85_4, 85.4},   {CtcssTone::TONE_88_5, 88.5},
        {CtcssTone::TONE_91_5, 91.5},   {CtcssTone::TONE_94_8, 94.8},   {CtcssTone::TONE_97_4, 97.4},
        {CtcssTone::TONE_100_0, 100.0}, {CtcssTone::TONE_103_5, 103.5}, {CtcssTone::TONE_107_2, 107.2},
        {CtcssTone::TONE_110_9, 110.9}, {CtcssTone::TONE_114_8, 114.8}, {CtcssTone::TONE_118_8, 118.8},
        {CtcssTone::TONE_123_0, 123.0}, {CtcssTone::TONE_127_3, 127.3}, {CtcssTone::TONE_131_8, 131.8},
        {CtcssTone::TONE_136_5, 136.5}, {CtcssTone::TONE_141_3, 141.3}, {CtcssTone::TONE_146_2, 146.2},
        {CtcssTone::TONE_151_4, 151.4}, {CtcssTone::TONE_156_7, 156.7}, {CtcssTone::TONE_159_8, 159.8},
        {CtcssTone::TONE_162_2, 162.2}, {CtcssTone::TONE_165_5, 165.5}, {CtcssTone::TONE_167_9, 167.9},
        {CtcssTone::TONE_171_3, 171.3}, {CtcssTone::TONE_173_8, 173.8}, {CtcssTone::TONE_177_3, 177.3},
        {CtcssTone::TONE_179_9, 179.9}, {CtcssTone::TONE_183_5, 183.5}, {CtcssTone::TONE_186_2, 186.2},
        {CtcssTone::TONE_189_9, 189.9}, {CtcssTone::TONE_192_8, 192.8}, {CtcssTone::TONE_196_6, 196.6},
        {CtcssTone::TONE_199_5, 199.5}, {CtcssTone::TONE_203_5, 203.5}, {CtcssTone::TONE_206_5, 206.5},
        {CtcssTone::TONE_210_7, 210.7}, {CtcssTone::TONE_218_1, 218.1}, {CtcssTone::TONE_225_7, 225.7},
        {CtcssTone::TONE_229_1, 229.1}, {CtcssTone::TONE_233_6, 233.6}, {CtcssTone::TONE_241_8, 241.8},
        {CtcssTone::TONE_250_3, 250.3}, {CtcssTone::TONE_254_1, 254.1}, {CtcssTone::TONE_NONE, 0}};

    void init(double attack, double decay, CtcssTone tone = CtcssTone::TONE_NONE);
    void init(double attack, double decay, double ctcss_frequency = 0);
    void reset(double attack, double decay, CtcssTone tone = CtcssTone::TONE_NONE);
    void reset(double attack, double decay, double ctcss_frequency = 0);
    void process(qs_vect_cpx &src_dst);

    void setToneFrequency(double frequency);
    void setToneFrequency(CtcssTone tone);
    double getToneFrequency();
    void setThreshold(double threshold);
    bool isSquelchOpen() const;
    double getCTCSSMagnitude() const;

    double estimateSquelchLevel();
};