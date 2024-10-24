// #include "../include/qs_tone_gen.hpp"
// #include "../include/qs_signalops.hpp"
// #include "../include/qs_globals.hpp"

// #include <cmath>

// QsToneGenerator ::QsToneGenerator()
//     : m_tg_pos(rateDataRate), m_rate(0), m_tg_inc(0.0), m_tg_osc_cos(0.0), m_tg_osc_sin(0.0), m_tg_osc1_re(1.0),
//       m_tg_osc1_im(0.0), m_tg_lo_freq(0.0), m_tg_osc_re(0.0), m_tg_osc_im(0.0) {}

// void QsToneGenerator ::init(QSDSPPOS pos) {
//     m_tg_pos = pos;
//     if (m_tg_pos == rateDataRate) {
//         m_rate = QsGlobal::g_memory->getDataProcRate();
//         m_tg_lo_freq = QsGlobal::g_memory->getToneLoFrequency();
//     } else {
//         m_rate = QsGlobal::g_memory->getDataPostProcRate();
//         m_tg_lo_freq = QsGlobal::g_memory->getOffsetGeneratorFrequency();
//     }
//     m_tg_inc = TWO_PI * (m_tg_lo_freq) / m_rate;
//     m_tg_osc_cos = cos(m_tg_inc);
//     m_tg_osc_sin = sin(m_tg_inc);
// }

// void QsToneGenerator ::process(qs_vect_cpx &src_dst) {
//     if (m_tg_pos == rateDataRate && m_tg_lo_freq != QsGlobal::g_memory->getToneLoFrequency()) {
//         m_tg_lo_freq = QsGlobal::g_memory->getToneLoFrequency();
//         m_tg_inc = TWO_PI * (m_tg_lo_freq) / m_rate;
//         m_tg_osc_cos = cos(m_tg_inc);
//         m_tg_osc_sin = sin(m_tg_inc);
//     } else if (m_tg_pos == ratePostDataRate && m_tg_lo_freq != QsGlobal::g_memory->getOffsetGeneratorFrequency()) {
//         m_tg_lo_freq = QsGlobal::g_memory->getOffsetGeneratorFrequency();
//         m_tg_inc = TWO_PI * (m_tg_lo_freq) / m_rate;
//         m_tg_osc_cos = cos(m_tg_inc);
//         m_tg_osc_sin = sin(m_tg_inc);
//     } else if (m_tg_pos == rateTxDataRate && m_tg_lo_freq != QsGlobal::g_memory->getTxOffsetFrequency()) {
//         m_tg_lo_freq = QsGlobal::g_memory->getTxOffsetFrequency();
//         m_tg_inc = TWO_PI * (m_tg_lo_freq) / m_rate;
//         m_tg_osc_cos = cos(m_tg_inc);
//         m_tg_osc_sin = sin(m_tg_inc);
//     }

//     for (cpx_itr = src_dst.begin(); cpx_itr != src_dst.end(); cpx_itr++) {
//         Cpx tg_temp = *cpx_itr; // Use dereference operator for iterator

//         // Calculate oscillation components
//         m_tg_osc_re = m_tg_osc1_re * m_tg_osc_cos - m_tg_osc1_im * m_tg_osc_sin;
//         m_tg_osc_im = m_tg_osc1_im * m_tg_osc_cos + m_tg_osc1_re * m_tg_osc_sin;

//         // Compute gain based on oscillation magnitude
//         double tg_gain = 1.95 - (m_tg_osc1_re * m_tg_osc1_re + m_tg_osc1_im * m_tg_osc1_im);

//         // Update oscillation state
//         m_tg_osc1_re = tg_gain * m_tg_osc_re;
//         m_tg_osc1_im = tg_gain * m_tg_osc_im;

//         // Update the complex value at the iterator position
//         *cpx_itr = Cpx(tg_temp.real() * m_tg_osc_re - tg_temp.imag() * m_tg_osc_im,
//                        tg_temp.real() * m_tg_osc_im + tg_temp.imag() * m_tg_osc_re);
//     }
// }

#include "../include/qs_tone_gen.hpp"
#include "../include/qs_globals.hpp"
#include "../include/qs_signalops.hpp"

#include <cmath>

QsToneGenerator::QsToneGenerator()
    : m_tg_pos(rateDataRate), m_rate(0), m_tg_inc(0.0), m_tg_osc_cos(0.0), m_tg_osc_sin(0.0), m_tg_osc1_re(1.0),
      m_tg_osc1_im(0.0), m_tg_lo_freq(0.0), m_tg_osc_re(0.0), m_tg_osc_im(0.0) {}

void QsToneGenerator::init(QSDSPPOS pos) {
    m_tg_pos = pos;

    // Set rate and frequency based on position
    switch (m_tg_pos) {
    case rateDataRate:
        m_rate = QsGlobal::g_memory->getDataProcRate();
        m_tg_lo_freq = QsGlobal::g_memory->getToneLoFrequency();
        break;
    case ratePostDataRate:
        m_rate = QsGlobal::g_memory->getDataPostProcRate();
        m_tg_lo_freq = QsGlobal::g_memory->getOffsetGeneratorFrequency();
        break;
    case rateTxDataRate:
        m_rate = QsGlobal::g_memory->getDataPostProcRate(); // Assuming post-process rate
        m_tg_lo_freq = QsGlobal::g_memory->getTxOffsetFrequency();
        break;
    default:
        throw std::runtime_error("Unknown position for tone generator");
    }

    // Compute increment and sine/cosine components
    m_tg_inc = TWO_PI * m_tg_lo_freq / m_rate;
    m_tg_osc_cos = cos(m_tg_inc);
    m_tg_osc_sin = sin(m_tg_inc);
}

void QsToneGenerator::process(qs_vect_cpx &src_dst) {
    // Check if LO frequency has changed, recalculate increment and oscillation components
    double new_lo_freq = 0.0;
    switch (m_tg_pos) {
    case rateDataRate:
        new_lo_freq = QsGlobal::g_memory->getToneLoFrequency();
        break;
    case ratePostDataRate:
        new_lo_freq = QsGlobal::g_memory->getOffsetGeneratorFrequency();
        break;
    case rateTxDataRate:
        new_lo_freq = QsGlobal::g_memory->getTxOffsetFrequency();
        break;
    }

    if (new_lo_freq != m_tg_lo_freq) {
        m_tg_lo_freq = new_lo_freq;
        m_tg_inc = TWO_PI * m_tg_lo_freq / m_rate;
        m_tg_osc_cos = cos(m_tg_inc);
        m_tg_osc_sin = sin(m_tg_inc);
    }

    // Process the input/output vector
    for (auto &sample : src_dst) {
        // Save current sample for tone mixing
        Cpx tg_temp = sample;

        // Compute oscillation real and imaginary components
        m_tg_osc_re = m_tg_osc1_re * m_tg_osc_cos - m_tg_osc1_im * m_tg_osc_sin;
        m_tg_osc_im = m_tg_osc1_im * m_tg_osc_cos + m_tg_osc1_re * m_tg_osc_sin;

        // Compute gain factor to maintain signal magnitude
        double tg_gain = 1.95 - (m_tg_osc1_re * m_tg_osc1_re + m_tg_osc1_im * m_tg_osc1_im);

        // Update oscillation state with the computed gain
        m_tg_osc1_re = tg_gain * m_tg_osc_re;
        m_tg_osc1_im = tg_gain * m_tg_osc_im;

        // Apply tone mixing to the current sample
        sample.real(tg_temp.real() * m_tg_osc_re - tg_temp.imag() * m_tg_osc_im);
        sample.imag(tg_temp.real() * m_tg_osc_im + tg_temp.imag() * m_tg_osc_re);
    }
}
