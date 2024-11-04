#include "../include/qs_squelch.hpp"
#include "../include/qs_debugloggerclass.hpp"

QsSquelch::QsSquelch()
    : m_sq_switch(false), m_sq_thresh(-75), m_attack(0.7), m_decay(0.7), m_sq_hist(-120.0), m_is_init(false),
      m_ctcss_tone(0), m_sampleRate(50000), m_squelchOpen(false), m_tone_threshold(0.02), m_blocksize(2048) {}

void QsSquelch::init(double attack, double decay, CtcssTone tone) {
    m_sq_switch = QsGlobal::g_memory->getSquelchOn();
    m_sq_thresh = QsGlobal::g_memory->getSquelchThreshold();
    m_blocksize = QsGlobal::g_memory->getReadBlockSize();
    m_sq_hist = -120.0; // Initialize squelch history with a low value
    m_attack = attack;
    m_decay = decay;
    m_sq_hysteresis = QsGlobal::g_memory->getSquelchHysteresis(); // Initialize hysteresis
    auto it = CtcssToneFrequencies.find(tone);
    if (it != CtcssToneFrequencies.end()) {
        m_ctcss_tone = it->second;
        m_sampleRate = QsGlobal::g_memory->getDataProcRate();
        m_tone_threshold = QsGlobal::g_memory->getCTCSSThreshold();
        m_lowPassFilter.init(300.0, m_sampleRate); // Initialize low-pass filter with 300 Hz cutoff
        m_bandpassFilter.setFilter(m_ctcss_tone, m_sampleRate, 3);
        m_is_init = true;
    } else {
        m_is_init = false;
    }
}

void QsSquelch::init(double attack, double decay, double tone) {
    m_sq_switch = QsGlobal::g_memory->getSquelchOn();
    m_sq_thresh = QsGlobal::g_memory->getSquelchThreshold();
    m_blocksize = QsGlobal::g_memory->getReadBlockSize();
    m_sq_hist = -120.0; // Initialize squelch history with a low value
    m_attack = attack;
    m_decay = decay;
    m_sq_hysteresis = QsGlobal::g_memory->getSquelchHysteresis(); // Initialize hysteresis
    m_ctcss_tone = tone;
    m_sampleRate = QsGlobal::g_memory->getDataProcRate();
    m_tone_threshold = QsGlobal::g_memory->getCTCSSThreshold();
    m_lowPassFilter.init(300.0, m_sampleRate); // Initialize low-pass filter with 300 Hz cutoff
    m_bandpassFilter.setFilter(m_ctcss_tone, m_sampleRate, 3);
    m_is_init = true;
}

void QsSquelch::reset(double attack, double decay, CtcssTone tone) { init(attack, decay, tone); }

void QsSquelch::reset(double attack, double decay, double tone) { init(attack, decay, tone); }

void QsSquelch::process(qs_vect_cpx &src_dst) {
    if (!m_is_init) {
        throw std::runtime_error("QsSquelch::process must call init() first!");
    }

    m_sq_switch = QsGlobal::g_memory->getSquelchOn();

    // Ensure squelchOpen resets at the beginning of each process call
    m_squelchOpen = false;

    if (m_sq_switch) {
        m_sq_thresh = QsGlobal::g_memory->getSquelchThreshold();
        m_sq_hysteresis = QsGlobal::g_memory->getSquelchHysteresis();
        double s_meter_value = QsGlobal::g_memory->getSMeterCurrentValue();

        // Apply weighted averaging for hysteresis
        if (m_sq_hist < s_meter_value) {
            m_sq_hist = (m_sq_hist * (1.0 - m_attack)) + (s_meter_value * m_attack);
        } else {
            m_sq_hist = (m_sq_hist * m_decay) + (s_meter_value * (1.0 - m_decay));
        }

        // Effective threshold with hysteresis
        double effective_thresh = m_sq_thresh + m_sq_hysteresis;

        if (m_sq_hist >= effective_thresh) {
            if (m_ctcss_tone != 0) {
                if (detectTone(src_dst)) {
                    m_squelchOpen = true;
                    QsGlobal::g_memory->setSquelchOpened(true);
                } else {
                    QsSignalOps::Zero(src_dst); // Mute signal if tone is not detected
                    m_squelchOpen = false;
                    QsGlobal::g_memory->setSquelchOpened(false);
                }
            }
        } else {
            QsSignalOps::Zero(src_dst); // Close squelch below threshold
            m_squelchOpen = false;
            QsGlobal::g_memory->setSquelchOpened(false);
        }
    } else {
        QsSignalOps::Zero(src_dst); // Close squelch if switch is off
        m_squelchOpen = false;
        QsGlobal::g_memory->setSquelchOpened(false);
    }
}

bool QsSquelch::detectTone(qs_vect_cpx &src_dst) {
    double omega = 2.0 * M_PI * m_ctcss_tone / m_sampleRate;
    double cosine = std::cos(omega);
    double sine = std::sin(omega);
    double coeff = 2.0 * cosine;
    double q0_real = 0.0, q1_real = 0.0, q2_real = 0.0;
    double q0_imag = 0.0, q1_imag = 0.0, q2_imag = 0.0;
    size_t length = src_dst.size();
    m_magnitude = 0.0;

    m_tone_threshold = QsGlobal::g_memory->getCTCSSThreshold();
    double threshold_on = m_tone_threshold * 1.0;
    double threshold_off = m_tone_threshold * 0.8;

    qs_vect_cpx filtered_data_bp = m_bandpassFilter.applyToData(src_dst);   

    // Go through each complex sample in the filtered data
    for (size_t i = 0; i < length; ++i) {
        double real_sample = filtered_data_bp[i].real();
        double imag_sample = filtered_data_bp[i].imag();

        // Apply Goertzel’s algorithm separately to real and imaginary components
        q0_real = coeff * q1_real - q2_real + real_sample;
        q2_real = q1_real;
        q1_real = q0_real;

        q0_imag = coeff * q1_imag - q2_imag + imag_sample;
        q2_imag = q1_imag;
        q1_imag = q0_imag;
    }

    // Compute magnitude from both real and imaginary accumulations
    double real = (q1_real - q2_real * cosine);
    double imag = (q2_imag * sine);
    m_magnitude = std::sqrt(real * real + imag * imag) / std::sqrt(length);

    // Hysteresis: Reset m_isToneDetected if m_magnitude drops below threshold_off
    if (m_isToneDetected) {
        if (m_magnitude < threshold_off) {
            m_isToneDetected = false;
            m_magnitude = 0.0;
        }
    } else {
        if (m_magnitude > threshold_on) {
            m_isToneDetected = true;
        }
    }

    return m_isToneDetected;
}

void QsSquelch::setToneFrequency(double frequency) {
    if (m_is_init) {
        m_ctcss_tone = frequency;
        m_bandpassFilter.setFilter(m_ctcss_tone, m_sampleRate, 3);
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
            m_bandpassFilter.setFilter(m_ctcss_tone, m_sampleRate, 3);
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
