#include "../include/qs_squelch.hpp"

QsSquelch::QsSquelch() : m_sq_switch(false), m_sq_thresh(0), m_sq_hist(-120.0) {}

void QsSquelch::init(double attack, double decay) {
    m_sq_switch = QsGlobal::g_memory->getSquelchOn();
    m_sq_thresh = QsGlobal::g_memory->getSquelchThreshold();
    m_sq_hist = -120.0;                                           // Initialize squelch history with a low value
    m_attack = attack;
    m_decay = decay;
    m_sq_hysteresis = QsGlobal::g_memory->getSquelchHysteresis(); // Initialize hysteresis
    m_is_init = true;
}

void QsSquelch::process(qs_vect_cpx &src_dst) {
    if (!m_is_init) {
        throw std::runtime_error("QsSquelch::process must call init() first!");
    }
    m_sq_switch = QsGlobal::g_memory->getSquelchOn();

    if (m_sq_switch) {
        m_sq_thresh = QsGlobal::g_memory->getSquelchThreshold();        
        m_sq_hysteresis = QsGlobal::g_memory->getSquelchHysteresis();
        double s_meter_value = QsGlobal::g_memory->getSMeterCurrentValue();

        // Apply a weighted average for squelch hysteresis
        if (m_sq_hist < s_meter_value) {
            m_sq_hist = (m_sq_hist * (1.0-m_attack)) + (s_meter_value * m_attack); // Fast rise
        } else {
            m_sq_hist = (m_sq_hist * m_decay) + (s_meter_value * (1.0-m_decay)); // Slow decay
        }

        // Adjusted threshold with hysteresis
        double effective_thresh = m_sq_thresh + m_sq_hysteresis;

        // If squelch history is below the adjusted threshold, zero out the signal
        if (m_sq_hist < effective_thresh) {
            QsSignalOps::Zero(src_dst); // Mute signal if below squelch threshold
            QsGlobal::g_memory->setSquelchOpened(false);
        } else {
            QsGlobal::g_memory->setSquelchOpened(true);    
        }
    }
}
