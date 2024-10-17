#include "../headers/qs_avg_nb.hpp"

QsAveragingNoiseBlanker ::QsAveragingNoiseBlanker()
    : m_anb_avg_sig(0), m_anb_magnitude(0), m_anb_avg_magn(0), m_anb_switch(false), m_anb_thres(0) {}

void QsAveragingNoiseBlanker ::init() {
    m_anb_avg_sig = cpx_zero;
    m_anb_magnitude = 0.0;
    m_anb_avg_magn = 0.0;
}

void QsAveragingNoiseBlanker::process(qs_vect_cpx &src_dst) {
    m_anb_switch = QsGlobal::g_memory->getAvgNoiseBlankerOn();
    m_anb_thres = QsGlobal::g_memory->getAvgNoiseBlankerThreshold();
    if (m_anb_switch) {
        for (m_cpx_iterator = src_dst.begin(); m_cpx_iterator != src_dst.end(); m_cpx_iterator++) {
            m_anb_magnitude = sqrt((*m_cpx_iterator).real() * (*m_cpx_iterator).real() +
                                   (*m_cpx_iterator).imag() * (*m_cpx_iterator).imag());
            m_anb_avg_sig = Cpx((m_anb_avg_sig.real() * 0.75) + ((*m_cpx_iterator).real() * 0.25),
                                (m_anb_avg_sig.imag() * 0.75) + ((*m_cpx_iterator).imag() * 0.25));
            m_anb_avg_magn = (0.999 * m_anb_avg_magn) + (0.001 * m_anb_magnitude);
            if (m_anb_magnitude > (m_anb_thres * m_anb_avg_magn)) {
                *m_cpx_iterator = m_anb_avg_sig;
            }
        }
    }
}
