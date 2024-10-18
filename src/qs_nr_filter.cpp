#include "../include/qs_nr_filter.hpp"

QsNoiseReductionFilter ::QsNoiseReductionFilter()
    : m_nr_switch(false), m_nr_lms_sz(0), m_nr_delay(0), m_nr_dl_indx(0), m_nr_mask(0), m_nr_adapt_rate(0),
      m_nr_leakage(0), m_nr_adapt_size(0) {}

void QsNoiseReductionFilter ::init(unsigned int size) {
    m_nr_lms_sz = size;
    m_nr_mask = m_nr_lms_sz - 1;

    m_nr_switch = QsGlobal::g_memory->getNoiseReductionOn();
    m_nr_adapt_rate = QsGlobal::g_memory->getNoiseReductionRate();
    m_nr_leakage = QsGlobal::g_memory->getNoiseReductionLeak();
    m_nr_adapt_size = QsGlobal::g_memory->getNoiseReductionTaps();
    m_nr_delay = QsGlobal::g_memory->getNoiseReductionDelay();
    m_nr_dl_indx = 0;

    m_nr_delay_line.resize(m_nr_lms_sz);
    QsSignalOps::Zero(m_nr_delay_line);
    m_nr_coeff.resize(m_nr_lms_sz * 2);
    QsSignalOps::Zero(m_nr_coeff);
}

void QsNoiseReductionFilter ::process(qs_vect_cpx &src_dst) {
    m_nr_switch = QsGlobal::g_memory->getNoiseReductionOn();

    if (m_nr_switch) {
        m_nr_adapt_rate = QsGlobal::g_memory->getNoiseReductionRate();
        m_nr_leakage = QsGlobal::g_memory->getNoiseReductionLeak();

        if (m_nr_lms_sz != src_dst.size()) {
            m_nr_lms_sz = src_dst.size();
            m_nr_mask = m_nr_lms_sz - 1;
            m_nr_delay_line.resize(m_nr_lms_sz);
            QsSignalOps::Zero(m_nr_delay_line);
            m_nr_coeff.resize(m_nr_lms_sz * 2);
            QsSignalOps::Zero(m_nr_coeff);
        }

        if (m_nr_adapt_size != QsGlobal::g_memory->getNoiseReductionTaps()) {
            m_nr_adapt_size = QsGlobal::g_memory->getNoiseReductionTaps();
            m_nr_dl_indx = 0;
            QsSignalOps::Zero(m_nr_delay_line);
            QsSignalOps::Zero(m_nr_coeff);
        }

        if (m_nr_delay != QsGlobal::g_memory->getNoiseReductionDelay()) {
            m_nr_delay = QsGlobal::g_memory->getNoiseReductionDelay();
            m_nr_dl_indx = 0;
            QsSignalOps::Zero(m_nr_delay_line);
            QsSignalOps::Zero(m_nr_coeff);
        }

        double scl1 = 1.0 - m_nr_adapt_rate * m_nr_leakage;

        for (m_cpx_iterator = src_dst.begin(); m_cpx_iterator != src_dst.end(); m_cpx_iterator++) {
            m_nr_delay_line[m_nr_dl_indx] = (*m_cpx_iterator).real();
            double accum = 0.0;
            double sum_sq = 0.0;

            for (int j = 0; j < m_nr_adapt_size; j++) {
                int k = (j + m_nr_delay + m_nr_dl_indx) & m_nr_mask;
                sum_sq += m_nr_delay_line[k] * m_nr_delay_line[k];
                accum += m_nr_coeff[j] * m_nr_delay_line[k];
            }

            double error = (*m_cpx_iterator).real() - accum;
            double out = accum * 1.5;
            (*m_cpx_iterator) = Cpx(out, out);

            double scl2 = m_nr_adapt_rate / (sum_sq + 1e-10);
            error *= scl2;

            for (int j = 0; j < m_nr_adapt_size; j++) {
                int k = (j + m_nr_delay + m_nr_dl_indx) & m_nr_mask;
                m_nr_coeff[j] = m_nr_coeff[j] * scl1 + error * m_nr_delay_line[k];
            }
            m_nr_dl_indx = (m_nr_dl_indx + m_nr_mask) & m_nr_mask;
        }
    }
}
