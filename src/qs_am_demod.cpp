#include "../headers/qs_am_demod.h"

QsAMDemodulator ::QsAMDemodulator() : m_am_mag(0.0), m_am_z0(0.0), m_am_z1(0.0), m_am_dc_alpha(0.999) {}

void QsAMDemodulator ::init() {
    m_am_mag = 0.0;
    m_am_z0 = 0.0;
    m_am_z1 = 0.0;
}

void QsAMDemodulator ::process(qs_vect_cpx &src_dst) {
    for (m_cpx_iterator = src_dst.begin(); m_cpx_iterator != src_dst.end(); m_cpx_iterator++) {
        m_am_mag = sqrt((*m_cpx_iterator).real() * (*m_cpx_iterator).real() +
                        (*m_cpx_iterator).imag() * (*m_cpx_iterator).imag());
        m_am_z0 = m_am_mag + (m_am_z1 * m_am_dc_alpha);
        (*m_cpx_iterator) = Cpx((m_am_z0 - m_am_z1), (m_am_z0 - m_am_z1));
        m_am_z1 = m_am_z0;
    }
}