#include "../include/qs_squelch.hpp"

QsSquelch::QsSquelch() : m_sq_switch(false), m_sq_thresh(0), m_sq_hist(0) {}

void QsSquelch::init() {
    m_sq_switch = QsGlobal::g_memory->getSquelchOn();
    m_sq_thresh = QsGlobal::g_memory->getSquelchThreshold();
    m_sq_hist = -120.0;
}

void QsSquelch::process(qs_vect_cpx &src_dst) {
    m_sq_switch = QsGlobal::g_memory->getSquelchOn();

    if (m_sq_switch) {
        m_sq_thresh = QsGlobal::g_memory->getSquelchThreshold();
        double s_meter_value = QsGlobal::g_memory->getSMeterCurrentValue();

        if (m_sq_hist < s_meter_value)
            m_sq_hist = (m_sq_hist * 0.5) + (s_meter_value * 0.5);
        else
            m_sq_hist = (m_sq_hist * 0.9) + (s_meter_value * 0.1);

        if (m_sq_hist < m_sq_thresh) {
            QsDataProc::Zero(src_dst);
        }
    }
}