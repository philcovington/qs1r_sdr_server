// #include "../include/qs_sam_demod.hpp"
// #include <stdexcept>

// QsSAMDemodulator ::QsSAMDemodulator()
//     : m_sam_bw(0), m_sam_limit(0), m_sam_zeta(0), m_sam_alpha_constant(0), m_sam_beta_constant(0), m_sam_norm(0),
//       m_sam_cos(0), m_sam_sin(0), m_sam_ncoPhase(0), m_sam_phaseError(0), m_sam_ncoFreq(0), m_sam_ncoHighLimit(0),
//       m_sam_ncoLowLimit(0), m_sam_alpha(0), m_sam_beta(0), m_sam_mag(0), m_sam_atan(0), m_sam_z0(0), m_sam_z1(0),
//       m_sam_y0(0), m_sam_y1(0), m_sam_dc_alpha(0) {}

// void QsSAMDemodulator ::init() {
//     m_sam_bw = QS_DEFAULT_SAM_BW;
//     m_sam_limit = QS_DEFAULT_SAM_LIMIT;
//     m_sam_zeta = QS_DEFAULT_SAM_ZETA; // damping factor
//     m_sam_alpha_constant = QS_DEFAULT_SAM_ALPHA;
//     m_sam_beta_constant = QS_DEFAULT_SAM_BETA;
//     m_sam_norm = TWO_PI / QsGlobal::g_memory->getDataPostProcRate();
//     m_sam_cos = 0.0;
//     m_sam_sin = 0.0;
//     m_sam_ncoPhase = 0.0;
//     m_sam_phaseError = 0.0;
//     m_sam_ncoFreq = 0.0;
//     m_sam_ncoHighLimit = m_sam_limit * m_sam_norm;
//     m_sam_ncoLowLimit = -m_sam_limit * m_sam_norm;
//     m_sam_alpha = m_sam_alpha_constant * m_sam_zeta * m_sam_bw * m_sam_norm;
//     m_sam_beta = (m_sam_alpha * m_sam_alpha) / (m_sam_beta_constant * m_sam_zeta * m_sam_zeta);
//     m_sam_mag = 0.0;
//     m_sam_atan = 0.0;
//     m_sam_z0 = 0.0;
//     m_sam_z1 = 0.0;
//     m_sam_y0 = 0.0;
//     m_sam_y1 = 0.0;
//     m_sam_dc_alpha = 0.999;
// }

// void QsSAMDemodulator::process(qs_vect_cpx &src_dst) {
//     Cpx m_sam_tmp(0, 0);
//     for (m_cpx_vect_iterator = src_dst.begin(); m_cpx_vect_iterator != src_dst.end(); ++m_cpx_vect_iterator) {
//         m_sam_sin = -sin(m_sam_ncoPhase);
//         m_sam_cos = cos(m_sam_ncoPhase);

//         // Use the Cpx methods to access real and imaginary parts
//         m_sam_tmp.real(m_sam_cos * m_cpx_vect_iterator->real() - m_sam_sin * m_cpx_vect_iterator->imag());
//         m_sam_tmp.imag(m_sam_cos * m_cpx_vect_iterator->imag() + m_sam_sin * m_cpx_vect_iterator->real());

//         if (m_sam_tmp.real() == 0.0f && m_sam_tmp.imag() == 0.0f) {
//             m_sam_tmp.real(1e-12);
//         }

//         m_sam_atan = atan2(m_sam_tmp.imag(), m_sam_tmp.real());
//         m_sam_mag = sqrt(m_cpx_vect_iterator->real() * m_cpx_vect_iterator->real() +
//                          m_cpx_vect_iterator->imag() * m_cpx_vect_iterator->imag());
//         m_sam_phaseError = m_sam_mag * m_sam_atan;

//         // Update NCO frequency
//         m_sam_ncoFreq += (m_sam_beta * m_sam_phaseError);
//         if (m_sam_ncoFreq > m_sam_ncoHighLimit) {
//             m_sam_ncoFreq = m_sam_ncoHighLimit;
//         } else if (m_sam_ncoFreq < m_sam_ncoLowLimit) {
//             m_sam_ncoFreq = m_sam_ncoLowLimit;
//         }

//         // Update NCO phase
//         m_sam_ncoPhase += (m_sam_ncoFreq + m_sam_alpha * m_sam_phaseError);
//         m_sam_ncoPhase = fmod(m_sam_ncoPhase, (float)TWO_PI);

//         // Apply the demodulation to the output vector
//         m_sam_z0 = m_sam_tmp.real() + (m_sam_z1 * m_sam_dc_alpha);
//         m_sam_y0 = m_sam_tmp.imag() + (m_sam_y1 * m_sam_dc_alpha);

//         m_cpx_vect_iterator->real(m_sam_z0 - m_sam_z1);
//         m_cpx_vect_iterator->imag(m_sam_y0 - m_sam_y1);

//         // Store the previous values for feedback
//         m_sam_z1 = m_sam_z0;
//         m_sam_y1 = m_sam_y0;
//     }
// }

#include "../include/qs_sam_demod.hpp"
#include <cmath>
#include <stdexcept>

QsSAMDemodulator::QsSAMDemodulator()
    : m_sam_bw(0), m_sam_limit(0), m_sam_zeta(0), m_sam_alpha_constant(0), m_sam_beta_constant(0), m_sam_norm(0),
      m_sam_cos(0), m_sam_sin(0), m_sam_ncoPhase(0), m_sam_phaseError(0), m_sam_ncoFreq(0), m_sam_ncoHighLimit(0),
      m_sam_ncoLowLimit(0), m_sam_alpha(0), m_sam_beta(0), m_sam_mag(0), m_sam_atan(0), m_sam_z0(0), m_sam_z1(0),
      m_sam_y0(0), m_sam_y1(0), m_sam_dc_alpha(0) {}

void QsSAMDemodulator::init() {
    // Initialize demodulation parameters
    m_sam_bw = QS_DEFAULT_SAM_BW;
    m_sam_limit = QS_DEFAULT_SAM_LIMIT;
    m_sam_zeta = QS_DEFAULT_SAM_ZETA; // Damping factor
    m_sam_alpha_constant = QS_DEFAULT_SAM_ALPHA;
    m_sam_beta_constant = QS_DEFAULT_SAM_BETA;

    // Precompute normalization factor
    m_sam_norm = TWO_PI / QsGlobal::g_memory->getDataPostProcRate();

    // Initialize internal variables
    m_sam_cos = m_sam_sin = m_sam_ncoPhase = m_sam_phaseError = m_sam_ncoFreq = 0.0;
    m_sam_ncoHighLimit = m_sam_limit * m_sam_norm;
    m_sam_ncoLowLimit = -m_sam_limit * m_sam_norm;

    // Loop filter coefficients
    m_sam_alpha = m_sam_alpha_constant * m_sam_zeta * m_sam_bw * m_sam_norm;
    m_sam_beta = (m_sam_alpha * m_sam_alpha) / (m_sam_beta_constant * m_sam_zeta * m_sam_zeta);

    // Initialize other variables
    m_sam_mag = m_sam_atan = m_sam_z0 = m_sam_z1 = m_sam_y0 = m_sam_y1 = 0.0;
    m_sam_dc_alpha = 0.999; // DC alpha for smoothing
}

void QsSAMDemodulator::process(qs_vect_cpx &src_dst) {
    // Temporary complex variable
    Cpx m_sam_tmp(0, 0);

    // Loop over each sample in the input/output vector
    for (auto &sample : src_dst) {
        // Compute the sine and cosine of the NCO phase
        m_sam_sin = -sin(m_sam_ncoPhase);
        m_sam_cos = cos(m_sam_ncoPhase);

        // Mix the input signal with the NCO signal
        m_sam_tmp.real(m_sam_cos * sample.real() - m_sam_sin * sample.imag());
        m_sam_tmp.imag(m_sam_cos * sample.imag() + m_sam_sin * sample.real());

        // Avoid division by zero by adding a small value to the real part
        if (m_sam_tmp.real() == 0.0f && m_sam_tmp.imag() == 0.0f) {
            m_sam_tmp.real(1e-12);
        }

        // Compute phase error using arctangent and magnitude of the signal
        m_sam_atan = atan2(m_sam_tmp.imag(), m_sam_tmp.real());
        m_sam_mag = std::hypot(sample.real(), sample.imag()); // More stable than sqrt

        // Calculate the phase error multiplied by the magnitude (AM demodulation)
        m_sam_phaseError = m_sam_mag * m_sam_atan;

        // Update NCO frequency with loop filter
        m_sam_ncoFreq += (m_sam_beta * m_sam_phaseError);

        // Constrain NCO frequency within the specified limits
        m_sam_ncoFreq = std::clamp(m_sam_ncoFreq, m_sam_ncoLowLimit, m_sam_ncoHighLimit);

        // Update NCO phase
        m_sam_ncoPhase += (m_sam_ncoFreq + m_sam_alpha * m_sam_phaseError);
        m_sam_ncoPhase = std::fmod(m_sam_ncoPhase, static_cast<float>(TWO_PI)); // Keep phase within [0, 2π]

        // Apply DC removal and update demodulated output
        m_sam_z0 = m_sam_tmp.real() + (m_sam_z1 * m_sam_dc_alpha);
        m_sam_y0 = m_sam_tmp.imag() + (m_sam_y1 * m_sam_dc_alpha);

        // Update output signal with the difference from the previous sample
        sample.real(m_sam_z0 - m_sam_z1);
        sample.imag(m_sam_y0 - m_sam_y1);

        // Store previous values for the next iteration
        m_sam_z1 = m_sam_z0;
        m_sam_y1 = m_sam_y0;
    }
}
