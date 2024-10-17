#include "../headers/qs_tone_gen.h"
#include "../headers/qs_dataproc.h"
#include "../headers/qs_globals.h"

#include <cmath>

QsToneGenerator ::QsToneGenerator()
    : m_tg_pos(rateDataRate), m_rate(0), m_tg_inc(0.0), m_tg_osc_cos(0.0), m_tg_osc_sin(0.0), m_tg_osc1_re(1.0),
      m_tg_osc1_im(0.0), m_tg_lo_freq(0.0), m_tg_osc_re(0.0), m_tg_osc_im(0.0) {}

void QsToneGenerator ::init(QSDSPPOS pos) {
    m_tg_pos = pos;
    if (m_tg_pos == rateDataRate) {
        m_rate = QsGlobal::g_memory->getDataProcRate();
        m_tg_lo_freq = QsGlobal::g_memory->getToneLoFrequency();
    } else {
        m_rate = QsGlobal::g_memory->getDataPostProcRate();
        m_tg_lo_freq = QsGlobal::g_memory->getOffsetGeneratorFrequency();
    }
    m_tg_inc = TWO_PI * (m_tg_lo_freq) / m_rate;
    m_tg_osc_cos = cos(m_tg_inc);
    m_tg_osc_sin = sin(m_tg_inc);
}

void QsToneGenerator ::process(qs_vect_cpx &src_dst) {
    if (m_tg_pos == rateDataRate && m_tg_lo_freq != QsGlobal::g_memory->getToneLoFrequency()) {
        m_tg_lo_freq = QsGlobal::g_memory->getToneLoFrequency();
        m_tg_inc = TWO_PI * (m_tg_lo_freq) / m_rate;
        m_tg_osc_cos = cos(m_tg_inc);
        m_tg_osc_sin = sin(m_tg_inc);
    } else if (m_tg_pos == ratePostDataRate && m_tg_lo_freq != QsGlobal::g_memory->getOffsetGeneratorFrequency()) {
        m_tg_lo_freq = QsGlobal::g_memory->getOffsetGeneratorFrequency();
        m_tg_inc = TWO_PI * (m_tg_lo_freq) / m_rate;
        m_tg_osc_cos = cos(m_tg_inc);
        m_tg_osc_sin = sin(m_tg_inc);
    } else if (m_tg_pos == rateTxDataRate && m_tg_lo_freq != QsGlobal::g_memory->getTxOffsetFrequency()) {
        m_tg_lo_freq = QsGlobal::g_memory->getTxOffsetFrequency();
        m_tg_inc = TWO_PI * (m_tg_lo_freq) / m_rate;
        m_tg_osc_cos = cos(m_tg_inc);
        m_tg_osc_sin = sin(m_tg_inc);
    }

    for (cpx_itr = src_dst.begin(); cpx_itr != src_dst.end(); cpx_itr++) {
        Cpx tg_temp = *cpx_itr; // Use dereference operator for iterator

        // Calculate oscillation components
        m_tg_osc_re = m_tg_osc1_re * m_tg_osc_cos - m_tg_osc1_im * m_tg_osc_sin;
        m_tg_osc_im = m_tg_osc1_im * m_tg_osc_cos + m_tg_osc1_re * m_tg_osc_sin;

        // Compute gain based on oscillation magnitude
        double tg_gain = 1.95 - (m_tg_osc1_re * m_tg_osc1_re + m_tg_osc1_im * m_tg_osc1_im);

        // Update oscillation state
        m_tg_osc1_re = tg_gain * m_tg_osc_re;
        m_tg_osc1_im = tg_gain * m_tg_osc_im;

        // Update the complex value at the iterator position
        *cpx_itr = Cpx(tg_temp.real() * m_tg_osc_re - tg_temp.imag() * m_tg_osc_im,
                       tg_temp.real() * m_tg_osc_im + tg_temp.imag() * m_tg_osc_re);
    }
}
