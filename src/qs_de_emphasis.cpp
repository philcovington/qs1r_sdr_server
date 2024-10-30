#include "../include/qs_de_emphasis.hpp"

DeEmphasis::DeEmphasis() : m_prev_output(0.0f) {}

DeEmphasis::~DeEmphasis() {}

void DeEmphasis::init(uint32_t sample_rate, double deemphasis) {
    m_samplerate = sample_rate;
    m_de_emphasis_time_const = deemphasis;

    // Calculate filter coefficient
    m_alpha = 1.0f / (1.0f + m_samplerate * m_de_emphasis_time_const);
	m_is_init = true;
}

void DeEmphasis::process(qs_vect_cpx &src_dst) {
	if (!m_is_init) {
        throw std::runtime_error("DeEmphasis::process must call init() first!");
    }
    for (auto &sample : src_dst) {
        // Separate real and imaginary parts
        float input_real = sample.real();
        float input_imag = sample.imag();

        // Apply de-emphasis to both real and imaginary parts
        float output_real = m_alpha * input_real + (1.0f - m_alpha) * m_prev_output;
        float output_imag = m_alpha * input_imag + (1.0f - m_alpha) * m_prev_output;

        // Update previous output for both channels
        m_prev_output = output_real;

        // Update the sample with the de-emphasized real and imaginary parts
        sample = std::complex<float>(output_real, output_imag);
    }
}
