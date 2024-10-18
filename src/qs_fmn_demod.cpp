#include "../include/qs_fmn_demod.hpp"

QsFMNDemodulator ::QsFMNDemodulator()
    : m_fmn_bw(0), m_fmn_limit(0), m_fmn_zeta(0), m_fmn_norm(0), m_fmn_cos(0), m_fmn_sin(0), m_fmn_ncoPhase(0),
      m_fmn_phaseError(0), m_fmn_ncoFreq(0), m_fmn_ncoHighLimit(0), m_fmn_ncoLowLimit(0), m_fmn_alpha(0), m_fmn_beta(0),
      m_fmn_freqDcError(0), m_fmn_dc_alpha(0), m_fmn_outgain(0) {}

void QsFMNDemodulator ::init() {
    // FMN DEMOD
    m_fmn_bw = QS_DEFAULT_FMN_BW;
    m_fmn_limit = QS_DEFAULT_FMN_LIMIT;
    m_fmn_zeta = QS_DEFAULT_FMN_ZETA; // damping factor
    m_fmn_norm = TWO_PI / QsGlobal::g_memory->getDataPostProcRate();
    m_fmn_cos = 0.0;
    m_fmn_sin = 0.0;
    m_fmn_ncoPhase = 0.0;
    m_fmn_phaseError = 0.0;
    m_fmn_ncoFreq = 0.0;
    m_fmn_ncoHighLimit = m_fmn_limit * m_fmn_norm;
    m_fmn_ncoLowLimit = -m_fmn_limit * m_fmn_norm;
    m_fmn_alpha = 2.0 * m_fmn_zeta * m_fmn_bw * m_fmn_norm;
    m_fmn_beta = (m_fmn_alpha * m_fmn_alpha) / (4.0 * m_fmn_zeta * m_fmn_zeta);
    m_fmn_freqDcError = 0.0;
    m_fmn_dc_alpha = (1.0 - exp(-1.0 / (QsGlobal::g_memory->getDataPostProcRate() * 0.01)));
    m_fmn_outgain = 0.45 * QsGlobal::g_memory->getDataPostProcRate() / (ONE_PI * m_fmn_bw);
}

void QsFMNDemodulator ::process(qs_vect_cpx &src_dst) {
    Cpx m_fmn_tmp(0, 0);
    for (m_cpx_iterator = src_dst.begin(); m_cpx_iterator != src_dst.end(); m_cpx_iterator++) {
        m_fmn_sin = sin(m_fmn_ncoPhase);
        m_fmn_cos = cos(m_fmn_ncoPhase);

        m_fmn_tmp = Cpx(m_fmn_cos * (*m_cpx_iterator).real() - m_fmn_sin * (*m_cpx_iterator).imag(),
                        m_fmn_cos * (*m_cpx_iterator).imag() + m_fmn_sin * (*m_cpx_iterator).real());

        if (m_fmn_tmp.real() == 0.0 && m_fmn_tmp.imag() == 0.0) {
            m_fmn_tmp = Cpx(1e-12, 0); // Avoid division by zero
        }

        m_fmn_phaseError = -atan2(m_fmn_tmp.imag(), m_fmn_tmp.real());
        m_fmn_ncoFreq += (m_fmn_beta * m_fmn_phaseError);

        if (m_fmn_ncoFreq > m_fmn_ncoHighLimit)
            m_fmn_ncoFreq = m_fmn_ncoHighLimit;
        else if (m_fmn_ncoFreq < m_fmn_ncoLowLimit)
            m_fmn_ncoFreq = m_fmn_ncoLowLimit;

        m_fmn_ncoPhase += (m_fmn_ncoFreq + m_fmn_alpha * m_fmn_phaseError);
        m_fmn_ncoPhase = fmod(m_fmn_ncoPhase, (float)TWO_PI);
        m_fmn_freqDcError = (1.0 - m_fmn_dc_alpha) * m_fmn_freqDcError + m_fmn_dc_alpha * m_fmn_ncoFreq;

        (*m_cpx_iterator) = Cpx((m_fmn_ncoFreq - m_fmn_freqDcError) * m_fmn_outgain,
                                (m_fmn_ncoFreq - m_fmn_freqDcError) * m_fmn_outgain);
    }
}
