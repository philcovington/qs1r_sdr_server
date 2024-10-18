#include "../include/qs_auto_notch_filter.hpp"

QsAutoNotchFilter ::QsAutoNotchFilter()
    : m_anf_lms_sz(0), m_anf_mask(0), m_anf_switch(false), m_anf_adapt_rate(0), m_anf_leakage(0), m_anf_adapt_size(0),
      m_anf_delay(0), m_anf_dl_indx(0) {}

void QsAutoNotchFilter ::init(unsigned int size) {
    m_anf_lms_sz = size;
    m_anf_mask = m_anf_lms_sz - 1;

    m_anf_switch = QsGlobal::g_memory->getAutoNotchOn();
    m_anf_adapt_rate = QsGlobal::g_memory->getAutoNotchRate();
    m_anf_leakage = QsGlobal::g_memory->getAutoNotchLeak();
    m_anf_adapt_size = QsGlobal::g_memory->getAutoNotchTaps();
    m_anf_delay = QsGlobal::g_memory->getAutoNotchDelay();
    m_anf_dl_indx = 0;

    m_anf_delay_line.resize(size);
    QsDataProc::Zero(m_anf_delay_line);
    m_anf_coeff.resize(size * 2);
    QsDataProc::Zero(m_anf_coeff);
}

void QsAutoNotchFilter ::process(qs_vect_cpx &src_dst) {
    m_anf_switch = QsGlobal::g_memory->getAutoNotchOn();

    if (m_anf_switch) {
        m_anf_adapt_rate = QsGlobal::g_memory->getAutoNotchRate();
        m_anf_leakage = QsGlobal::g_memory->getAutoNotchLeak();

        if (m_anf_lms_sz != src_dst.size()) {
            m_anf_lms_sz = src_dst.size();
            m_anf_mask = m_anf_lms_sz - 1;
            m_anf_delay_line.resize(m_anf_lms_sz);
            QsDataProc::Zero(m_anf_delay_line);
            m_anf_coeff.resize(m_anf_lms_sz * 2);
            QsDataProc::Zero(m_anf_coeff);
        }

        if (m_anf_adapt_size != QsGlobal::g_memory->getAutoNotchTaps()) {
            m_anf_adapt_size = QsGlobal::g_memory->getAutoNotchTaps();
            m_anf_dl_indx = 0;
            QsDataProc::Zero(m_anf_delay_line);
            QsDataProc::Zero(m_anf_coeff);
        }

        if (m_anf_delay != QsGlobal::g_memory->getAutoNotchDelay()) {
            m_anf_delay = QsGlobal::g_memory->getAutoNotchDelay();
            m_anf_dl_indx = 0;
            QsDataProc::Zero(m_anf_delay_line);
            QsDataProc::Zero(m_anf_coeff);
        }

        double scl1 = 1.0 - m_anf_adapt_rate * m_anf_leakage;

        for (m_cpx_iterator = src_dst.begin(); m_cpx_iterator != src_dst.end(); m_cpx_iterator++) {
            m_anf_delay_line[m_anf_dl_indx] = (*m_cpx_iterator).real();
            double accum = 0.0;
            double sum_sq = 0.0;

            for (int j = 0; j < m_anf_adapt_size; j++) {
                int k = (j + m_anf_delay + m_anf_dl_indx) & m_anf_mask;
                sum_sq += m_anf_delay_line[k] * m_anf_delay_line[k];
                accum += m_anf_coeff[j] * m_anf_delay_line[k];
            }

            double error = (*m_cpx_iterator).real() - accum;
            (*m_cpx_iterator) = Cpx(error, error);

            double scl2 = m_anf_adapt_rate / (sum_sq + 1e-10);
            error *= scl2;

            for (int j = 0; j < m_anf_adapt_size; j++) {
                int k = (j + m_anf_delay + m_anf_dl_indx) & m_anf_mask;
                m_anf_coeff[j] = m_anf_coeff[j] * scl1 + error * m_anf_delay_line[k];
            }
            m_anf_dl_indx = (m_anf_dl_indx + m_anf_mask) & m_anf_mask;
        }
    }
}
