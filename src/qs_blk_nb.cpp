#include "../include/qs_blk_nb.hpp"

QsBlockNoiseBlanker ::QsBlockNoiseBlanker()
    : m_bnb_magnitude(0), m_bnb_avg_magn(0), m_bnb_switch(false), m_bnb_thres(0), m_bnb_sig_index(0),
      m_bnb_dly_index(0), m_bnb_hangtime(0), m_bnb_avg_sig(0) {}

void QsBlockNoiseBlanker ::init() {
    bnb_delay_line.resize(8, cpx_zero);
    m_bnb_avg_sig = cpx_zero;

    m_bnb_magnitude = 0.0;
    m_bnb_avg_magn = 0.0;
    m_bnb_sig_index = 0;
    m_bnb_dly_index = 0;
    m_bnb_hangtime = 0;
}

void QsBlockNoiseBlanker ::process(qs_vect_cpx &src_dst) {
    m_bnb_switch = QsGlobal::g_memory->getBlockNoiseBlankerOn();
    m_bnb_thres = QsGlobal::g_memory->getBlockNoiseBlankerThreshold();
    if (m_bnb_switch) {
        for (m_cpx_iterator = src_dst.begin(); m_cpx_iterator != src_dst.end(); m_cpx_iterator++) {
            m_bnb_magnitude = sqrt((*m_cpx_iterator).real() * (*m_cpx_iterator).real() +
                                   (*m_cpx_iterator).imag() * (*m_cpx_iterator).imag());
            bnb_delay_line[m_bnb_sig_index] = (*m_cpx_iterator);
            m_bnb_avg_magn = (0.999 * m_bnb_avg_magn) + (0.001 * m_bnb_magnitude);
            if ((m_bnb_hangtime == 0) && (m_bnb_magnitude > m_bnb_thres * m_bnb_avg_magn)) {
                m_bnb_hangtime = 7;
            }
            if (m_bnb_hangtime > 0) {
                (*m_cpx_iterator) = cpx_zero;
                m_bnb_hangtime--;
            } else {
                (*m_cpx_iterator) = bnb_delay_line[m_bnb_dly_index];
            }
            m_bnb_sig_index = (m_bnb_sig_index + 7) & 7;
            m_bnb_dly_index = (m_bnb_dly_index + 7) & 7;
        }
    }
}
