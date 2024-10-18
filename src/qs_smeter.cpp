#include "../include/qs_smeter.hpp"
#include "../include/qs_defines.hpp"

QsSMeter ::QsSMeter() : m_sm_tmp_val(0), m_sm_value(0) {}

void QsSMeter ::init() {
    m_sm_tmp_val = 0.0;
    m_sm_value = 0.0;
}

void QsSMeter ::process(qs_vect_cpx &src_dst) {
    m_sm_tmp_val = 0.0;
    for (m_cpx_iterator = src_dst.begin(); m_cpx_iterator != src_dst.end(); m_cpx_iterator++) {
        m_sm_tmp_val +=
            (*m_cpx_iterator).real() * (*m_cpx_iterator).real() + (*m_cpx_iterator).imag() * (*m_cpx_iterator).imag();
    }
    // m_sm_value = ( m_sm_value * 0.5 ) + ( ( 10.0 * log10( m_sm_tmp_val + 1e-200 ) ) * 0.5 );
    m_sm_value = 10.0 * log10(m_sm_tmp_val + 1e-200);
    double corrected_sm = m_sm_value + QsGlobal::g_memory->getSMeterCorrection() + SMETERCORRECT;
    QsGlobal::g_memory->setSMeterCurrentValue(corrected_sm);
    QsGlobal::g_memory->setSMeterCurrentValueC((unsigned char)std::round(corrected_sm + QS_DEFAULT_SPEC_OFFSET));
}
