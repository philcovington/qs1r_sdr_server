#include "../include/qs_test_tone.hpp"

void QsTestTone::init(float frequency, float amplitude, uint32_t samplerate) {
	// Test tone
    m_tt_amplitude = amplitude;
    m_tt_frequency = frequency;
    m_tt_phase = 0.0;
    m_tt_phaseIncrement = 2.0f * M_PI * m_tt_frequency / samplerate;
}

void QsTestTone::setFrequency(float frequency) { m_tt_frequency = frequency; }
void QsTestTone::setAmplitude(float amplitude) { m_tt_amplitude = amplitude; }

void QsTestTone::process(qs_vect_f &in_out, size_t blocksize) {
	// Fill the buffer with a sine wave
    for (size_t i = 0; i < blocksize; ++i) {
        in_out[i] = m_tt_amplitude * sin(m_tt_phase);
        m_tt_phase += m_tt_phaseIncrement;

        // Keep phase between 0 and 2 * PI
        if (m_tt_phase > 2.0f * M_PI) {
            m_tt_phase -= 2.0f * M_PI;
        }
    }
}