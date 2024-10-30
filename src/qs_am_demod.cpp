// #include "../include/qs_am_demod.hpp"

// QsAMDemodulator ::QsAMDemodulator() : m_am_mag(0.0), m_am_z0(0.0), m_am_z1(0.0), m_am_dc_alpha(0.999) {}

// void QsAMDemodulator ::init() {
//     m_am_mag = 0.0;
//     m_am_z0 = 0.0;
//     m_am_z1 = 0.0;
// }

// void QsAMDemodulator ::process(qs_vect_cpx &src_dst) {
//     for (m_cpx_iterator = src_dst.begin(); m_cpx_iterator != src_dst.end(); m_cpx_iterator++) {
//         m_am_mag = sqrt((*m_cpx_iterator).real() * (*m_cpx_iterator).real() +
//                         (*m_cpx_iterator).imag() * (*m_cpx_iterator).imag());
//         m_am_z0 = m_am_mag + (m_am_z1 * m_am_dc_alpha);
//         (*m_cpx_iterator) = Cpx((m_am_z0 - m_am_z1), (m_am_z0 - m_am_z1));
//         m_am_z1 = m_am_z0;
//     }
// }

#include "../include/qs_am_demod.hpp"
#include <cmath>
#include <complex>

QsAMDemodulator::QsAMDemodulator() : m_am_mag(0.0), m_am_z0(0.0), m_am_z1(0.0), m_am_dc_alpha(0.999) {}

void QsAMDemodulator::init() {
    m_am_mag = 0.0;
    m_am_z0 = 0.0;
    m_am_z1 = 0.0;
    m_alpha = 0.1f;
    m_is_init = true;
}

void QsAMDemodulator::process(qs_vect_cpx &src_dst) {
    if (!m_is_init) {
        throw std::runtime_error("QsAMDemodulator::process must call init() first!");
    }
    m_alpha = QsGlobal::g_memory->getAMPostFilterAlpha();
    // Initialize previous output for the low-pass filter state
    float prev_output = 0.0f;

    for (auto &sample : src_dst) {
        // Calculate the magnitude of the complex sample manually
        float real_part = sample.real();
        float imag_part = sample.imag();
        m_am_mag = std::sqrt(real_part * real_part + imag_part * imag_part);

        // Apply the DC removal filter
        m_am_z0 = m_am_mag + (m_am_z1 * m_am_dc_alpha);

        // Update the sample with the demodulated value
        float demodulated_value = m_am_z0 - m_am_z1;

        // Apply low-pass filter after demodulation        
        float output = m_alpha * demodulated_value + (1.0f - m_alpha) * prev_output;
        prev_output = output;

        // Update sample with filtered output
        sample = Cpx(output, output);

        // Update the state for the next iteration
        m_am_z1 = m_am_z0;
    }
}
