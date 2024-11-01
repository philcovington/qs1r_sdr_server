#include "../include/qs_squelch.hpp"

QsSquelch::QsSquelch()
    : m_sq_switch(false), m_sq_thresh(0), m_attack(0.7), m_decay(0.7), m_sq_hist(-120.0), m_is_init(false),
      m_ctcss_tone(0), m_sampleRate(50000), m_squelchOpen(false), m_tone_threshold(0.02) {}

void QsSquelch::init(double attack, double decay, CtcssTone tone) {
    m_sq_switch = QsGlobal::g_memory->getSquelchOn();
    m_sq_thresh = QsGlobal::g_memory->getSquelchThreshold();
    m_sq_hist = -120.0; // Initialize squelch history with a low value
    m_attack = attack;
    m_decay = decay;
    m_sq_hysteresis = QsGlobal::g_memory->getSquelchHysteresis(); // Initialize hysteresis

    auto it = CtcssToneFrequencies.find(tone);
    if (it != CtcssToneFrequencies.end()) {
        m_ctcss_tone = it->second;
        m_sampleRate = QsGlobal::g_memory->getDataProcRate();
        m_tone_threshold = QsGlobal::g_memory->getCTCSSThreshold();
        m_lowPassFilter.init(300.0, m_sampleRate);  // Initialize low-pass filter with 300 Hz cutoff
        m_is_init = true;
    } else {
        m_is_init = false;
    }
}

void QsSquelch::init(double attack, double decay, double tone) {
    m_sq_switch = QsGlobal::g_memory->getSquelchOn();
    m_sq_thresh = QsGlobal::g_memory->getSquelchThreshold();
    m_sq_hist = -120.0; // Initialize squelch history with a low value
    m_attack = attack;
    m_decay = decay;
    m_sq_hysteresis = QsGlobal::g_memory->getSquelchHysteresis(); // Initialize hysteresis
    m_ctcss_tone = tone;
    m_sampleRate = QsGlobal::g_memory->getDataProcRate();
    m_tone_threshold = QsGlobal::g_memory->getCTCSSThreshold();
    m_lowPassFilter.init(300.0, m_sampleRate);  // Initialize low-pass filter with 300 Hz cutoff
    m_is_init = true;
}

void QsSquelch::reset(double attack, double decay, CtcssTone tone) { init(attack, decay, tone); }

void QsSquelch::reset(double attack, double decay, double tone) { init(attack, decay, tone); }

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
            m_sq_hist = (m_sq_hist * (1.0 - m_attack)) + (s_meter_value * m_attack); // Fast rise
        } else {
            m_sq_hist = (m_sq_hist * m_decay) + (s_meter_value * (1.0 - m_decay)); // Slow decay
        }

        // Adjusted threshold with hysteresis
        double effective_thresh = m_sq_thresh + m_sq_hysteresis;

        // Check if squelch history is above the adjusted threshold
        if (m_sq_hist >= effective_thresh) {
            // If a specific CTCSS tone is required
            if (m_ctcss_tone != 0) {
                if (detectTone(src_dst)) {
                    m_squelchOpen = true; // Tone detected, open squelch
                    QsGlobal::g_memory->setSquelchOpened(true);
                } else {
                    QsSignalOps::Zero(src_dst); // Mute signal if no tone detected
                    m_squelchOpen = false;
                    m_magnitude = 0.0;
                    QsGlobal::g_memory->setSquelchOpened(false);
                }
            } else {
                // No CTCSS tone required, open squelch based on effective threshold
                m_squelchOpen = true;
                QsGlobal::g_memory->setSquelchOpened(true);
            }
        } else {
            QsSignalOps::Zero(src_dst); // Mute signal if below threshold
            m_squelchOpen = false;
            m_magnitude = 0.0;
            QsGlobal::g_memory->setSquelchOpened(false);
        }
    }
}

bool QsSquelch::detectTone(qs_vect_cpx &src_dst) {
    double omega = 2.0 * M_PI * m_ctcss_tone / m_sampleRate;
    double cosine = std::cos(omega);
    double sine = std::sin(omega);
    double coeff = 2.0 * cosine;
    double q0 = 0.0, q1 = 0.0, q2 = 0.0;
    size_t length = src_dst.size();
    m_tone_threshold = QsGlobal::g_memory->getCTCSSThreshold();

    qs_vect_cpx filtered_data = m_lowPassFilter.applyToData(src_dst);

    // Iterate over src_dst and apply the Goertzel algorithm
    for (size_t i = 0; i < length; ++i) {
        // Use the real part of each complex sample for tone detection
        double sample = src_dst[i].real();

        q0 = coeff * q1 - q2 + sample;
        q2 = q1;
        q1 = q0;
    }

    double real = (q1 - q2 * cosine);
    double imag = (q2 * sine);
    m_magnitude = std::sqrt(real * real + imag * imag) / length;

    // Check if the detected tone magnitude exceeds the threshold
    return m_magnitude >= m_tone_threshold;
}

void QsSquelch::setToneFrequency(double frequency) {
    if (m_is_init) {
        m_ctcss_tone = frequency;
    } else {
        throw std::runtime_error("QsSquelch::process must call init() first!");
    }
}
void QsSquelch::setToneFrequency(CtcssTone tone) {
    if (m_is_init) {
        auto it = CtcssToneFrequencies.find(tone);
        if (it != CtcssToneFrequencies.end()) {
            m_ctcss_tone = it->second;
            m_sampleRate = QsGlobal::g_memory->getDataProcRate();
        } else {
            throw std::runtime_error("CTCSS tone is invalid!");
        }
    } else {
        throw std::runtime_error("QsSquelch::process must call init() first!");
    }
}

double QsSquelch::getToneFrequency() { return m_ctcss_tone; }

bool QsSquelch::isSquelchOpen() const { return m_squelchOpen; }

double QsSquelch::getCTCSSMagnitude() const { return m_magnitude; }

double QsSquelch::estimateSquelchLevel() { return 0; }
