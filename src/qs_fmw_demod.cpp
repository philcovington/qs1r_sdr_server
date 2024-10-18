#include "../include/qs_fmwdemod.hpp"

QsFMWDemodulator ::QsFMWDemodulator()
    : m_fmw_bw(0), m_fmw_limit(0), m_fmw_zeta(0), m_fmw_norm(0), m_fmw_cos(0), m_fmw_sin(0), m_fmw_ncoPhase(0),
      m_fmw_phaseError(0), m_fmw_ncoFreq(0), m_fmw_ncoHighLimit(0), m_fmw_ncoLowLimit(0), m_fmw_alpha(0), m_fmw_beta(0),
      m_fmw_freqDcError(0), m_fmw_dc_alpha(0), m_fmw_outgain(0) {}

void QsFMWDemodulator ::init() {
    // FMN DEMOD
    m_fmw_bw = QS_DEFAULT_FMW_BW;
    m_fmw_limit = QS_DEFAULT_FMW_LIMIT;
    m_fmw_zeta = QS_DEFAULT_FMW_ZETA; // damping factor
    m_fmw_norm = TWO_PI / QsGlobal::g_memory->getDataPostProcRate();
    m_fmw_cos = 0.0;
    m_fmw_sin = 0.0;
    m_fmw_ncoPhase = 0.0;
    m_fmw_phaseError = 0.0;
    m_fmw_ncoFreq = 0.0;
    m_fmw_ncoHighLimit = m_fmw_limit * m_fmw_norm;
    m_fmw_ncoLowLimit = -m_fmw_limit * m_fmw_norm;
    m_fmw_alpha = 2.0 * m_fmw_zeta * m_fmw_bw * m_fmw_norm;
    m_fmw_beta = (m_fmw_alpha * m_fmw_alpha) / (4.0 * m_fmw_zeta * m_fmw_zeta);
    m_fmw_freqDcError = 0.0;
    m_fmw_dc_alpha = (1.0 - exp(-1.0 / (QsGlobal::g_memory->getDataPostProcRate() * 0.01)));
    m_fmw_outgain = 0.45 * QsGlobal::g_memory->getDataPostProcRate() / (ONE_PI * m_fmw_bw);
}

void QsFMWDemodulator ::process(qs_vect_cpx &src_dst) {
    Cpx m_fmw_tmp(0, 0);
    for (m_cpx_iterator = src_dst.begin(); m_cpx_iterator != src_dst.end(); m_cpx_iterator++) {
        m_fmw_sin = sin(m_fmw_ncoPhase);
        m_fmw_cos = cos(m_fmw_ncoPhase);

        m_fmw_tmp = Cpx(m_fmw_cos * (*m_cpx_iterator).real() - m_fmw_sin * (*m_cpx_iterator).imag(),
                        m_fmw_cos * (*m_cpx_iterator).imag() + m_fmw_sin * (*m_cpx_iterator).real());

        if (m_fmw_tmp.real() == 0.0 && m_fmw_tmp.imag() == 0.0) {
            m_fmw_tmp = Cpx(1e-12, 0); // Avoid division by zero
        }

        m_fmw_phaseError = -atan2(m_fmw_tmp.imag(), m_fmw_tmp.real());
        m_fmw_ncoFreq += (m_fmw_beta * m_fmw_phaseError);

        if (m_fmw_ncoFreq > m_fmw_ncoHighLimit)
            m_fmw_ncoFreq = m_fmw_ncoHighLimit;
        else if (m_fmw_ncoFreq < m_fmw_ncoLowLimit)
            m_fmw_ncoFreq = m_fmw_ncoLowLimit;

        m_fmw_ncoPhase += (m_fmw_ncoFreq + m_fmw_alpha * m_fmw_phaseError);
        m_fmw_ncoPhase = fmod(m_fmw_ncoPhase, (float)TWO_PI);
        m_fmw_freqDcError = (1.0 - m_fmw_dc_alpha) * m_fmw_freqDcError + m_fmw_dc_alpha * m_fmw_ncoFreq;

        (*m_cpx_iterator) = Cpx((m_fmw_ncoFreq - m_fmw_freqDcError) * m_fmw_outgain,
                                (m_fmw_ncoFreq - m_fmw_freqDcError) * m_fmw_outgain);
    }
}
