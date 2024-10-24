#include "../include/qs_fm_demod.hpp"

#include "../include/qs_globals.hpp" // Include global definitions and dependencies
#include <algorithm>                 // for std::clamp
#include <cmath>

QsFMCombinedDemodulator::QsFMCombinedDemodulator()
    : m_mode(NARROW), m_bw(0), m_limit(0), m_zeta(0), m_norm(0), m_cos(0), m_sin(0), m_ncoPhase(0), m_phaseError(0),
      m_ncoFreq(0), m_ncoHighLimit(0), m_ncoLowLimit(0), m_alpha(0), m_beta(0), m_freqDcError(0), m_dc_alpha(0),
      m_outgain(0) {}

void QsFMCombinedDemodulator::init(DemodMode mode) {
    m_mode = mode;

    // Set default parameters based on the selected mode
    if (mode == NARROW) {
        m_bw = QS_DEFAULT_FMN_BW;
        m_limit = QS_DEFAULT_FMN_LIMIT;
        m_zeta = QS_DEFAULT_FMN_ZETA;
    } else { // WIDE
        m_bw = QS_DEFAULT_FMW_BW;
        m_limit = QS_DEFAULT_FMW_LIMIT;
        m_zeta = QS_DEFAULT_FMW_ZETA;
    }

    m_norm = TWO_PI / QsGlobal::g_memory->getDataPostProcRate();
    m_ncoPhase = 0.0;
    m_ncoFreq = 0.0;
    m_ncoHighLimit = m_limit * m_norm;
    m_ncoLowLimit = -m_limit * m_norm;
    m_alpha = 2.0 * m_zeta * m_bw * m_norm;
    m_beta = (m_alpha * m_alpha) / (4.0 * m_zeta * m_zeta);
    m_freqDcError = 0.0;
    m_dc_alpha = 1.0 - exp(-1.0 / (QsGlobal::g_memory->getDataPostProcRate() * 0.01));
    m_outgain = 0.45 * QsGlobal::g_memory->getDataPostProcRate() / (ONE_PI * m_bw);
}

void QsFMCombinedDemodulator::process(qs_vect_cpx &src_dst, DemodMode mode) {
    if (m_mode != mode) {
        init(mode);
    }
    for (auto &sample : src_dst) {
        // Precompute trigonometric functions for the NCO phase
        m_sin = sin(m_ncoPhase);
        m_cos = cos(m_ncoPhase);

        // Mix the incoming signal with the NCO
        Cpx mixed(m_cos * sample.real() - m_sin * sample.imag(), m_cos * sample.imag() + m_sin * sample.real());

        // Avoid division by zero by checking the magnitude
        if (std::abs(mixed.real()) < 1e-12 && std::abs(mixed.imag()) < 1e-12) {
            mixed = Cpx(1e-12, 0);
        }

        // Compute phase error
        m_phaseError = -atan2(mixed.imag(), mixed.real());

        // Update NCO frequency using the phase error
        m_ncoFreq += (m_beta * m_phaseError);

        // Clamp NCO frequency to its limits
        m_ncoFreq = std::clamp(m_ncoFreq, m_ncoLowLimit, m_ncoHighLimit);

        // Update NCO phase and ensure it wraps around TWO_PI
        m_ncoPhase += (m_ncoFreq + m_alpha * m_phaseError);
        m_ncoPhase = fmod(m_ncoPhase, TWO_PI);

        // DC error compensation
        m_freqDcError = (1.0 - m_dc_alpha) * m_freqDcError + m_dc_alpha * m_ncoFreq;

        // Output the demodulated FM signal
        float demodulated_value = (m_ncoFreq - m_freqDcError) * m_outgain;
        sample = Cpx(demodulated_value, demodulated_value);
    }
}
