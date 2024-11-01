#include "../include/qs_test_tone.hpp"

void QsTestTone::init(float frequency, float amplitude, uint32_t samplerate) {
    // Test tone
    m_tt_amplitude = amplitude;
    m_tt_frequency = frequency;
    m_tt_samplerate = samplerate;
    m_tt_phase = 0.0;
    m_tt_phaseIncrement = 2.0f * M_PI * m_tt_frequency / m_tt_samplerate;
}

void QsTestTone::setFrequency(float frequency) {
    m_tt_frequency = frequency;
    m_tt_phaseIncrement = 2.0f * M_PI * m_tt_frequency / m_tt_samplerate;
}

void QsTestTone::setAmplitude(float amplitude) { m_tt_amplitude = amplitude; }

void QsTestTone::process(qs_vect_f &src_dst) {
    // Fill the buffer with a sine wave
    for (f_itr = src_dst.begin(); f_itr != src_dst.end(); f_itr++) {
        (*f_itr) = m_tt_amplitude * sin(m_tt_phase);
        m_tt_phase += m_tt_phaseIncrement;

        // Keep phase between 0 and 2 * PI
        if (m_tt_phase > 2.0f * M_PI) {
            m_tt_phase -= 2.0f * M_PI;
        }
    }
}

void QsTestTone::process(qs_vect_cpx &src_dst) {
    // Fill the buffer with a complex sine wave
    for (auto cpx_itr = src_dst.begin(); cpx_itr != src_dst.end(); ++cpx_itr) {
        (*cpx_itr).real(m_tt_amplitude * cos(m_tt_phase));
        (*cpx_itr).imag(m_tt_amplitude * sin(m_tt_phase));

        m_tt_phase += m_tt_phaseIncrement;

        // Keep phase between 0 and 2 * PI
        if (m_tt_phase > 2.0 * M_PI) {
            m_tt_phase -= 2.0 * M_PI;
        }
    }
}
