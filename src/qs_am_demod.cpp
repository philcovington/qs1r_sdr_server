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
#include <complex>
#include <cmath>

QsAMDemodulator::QsAMDemodulator() : m_am_mag(0.0), m_am_z0(0.0), m_am_z1(0.0), m_am_dc_alpha(0.999) {}

void QsAMDemodulator::init() {
    m_am_mag = 0.0;
    m_am_z0 = 0.0;
    m_am_z1 = 0.0;
}

void QsAMDemodulator::process(qs_vect_cpx &src_dst) {
    for (auto &sample : src_dst) {
        // Compute the magnitude using std::norm() to avoid recalculating real/imaginary parts
        m_am_mag = std::sqrt(std::norm(sample));
        
        // Apply the DC removal filter
        m_am_z0 = m_am_mag + m_am_dc_alpha * m_am_z1;
        
        // Update the sample in place
        float demodulated_value = m_am_z0 - m_am_z1;
        sample = Cpx(demodulated_value, demodulated_value);
        
        // Update filter state
        m_am_z1 = m_am_z0;
    }
}
