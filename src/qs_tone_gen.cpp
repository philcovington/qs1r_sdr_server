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
      m_tg_osc1_im(0.0), m_tg_lo_freq(0.0), m_tg_osc_re(0.0), m_tg_osc_im(0.0), m_tg_amplitude(1.0) {}

void QsToneGenerator::init(QSDSPPOS pos) {
    m_tg_pos = pos;

    switch (m_tg_pos) {
    case rateDataRate:
        m_rate = QsGlobal::g_memory->getDataProcRate();
        m_tg_lo_freq = QsGlobal::g_memory->getToneLoFrequency();
        m_test_mode = false;
        break;
    case ratePostDataRate:
        m_rate = QsGlobal::g_memory->getDataProcRate();
        m_tg_lo_freq = QsGlobal::g_memory->getOffsetGeneratorFrequency();
        m_test_mode = false;
        break;
    case rateTxDataRate:
        m_rate = QsGlobal::g_memory->getDataProcRate();
        m_tg_lo_freq = QsGlobal::g_memory->getTxOffsetFrequency();
        m_test_mode = false;
        break;
    case rate24000:
        m_rate = 24000;
        m_test_mode = true;
        m_tg_lo_freq = 1000.0;
        m_tg_amplitude = 0.01f;
        break;
    case rate48000:
        m_rate = 48000;
        m_test_mode = true;
        m_tg_lo_freq = 1000.0;
        m_tg_amplitude = 0.01f;
        break;
    case rate50000:
        m_rate = 50000;
        m_test_mode = true;
        m_tg_lo_freq = 1000.0;
        m_tg_amplitude = 0.01f;
        break;
    default:
        throw std::runtime_error("Unknown position for tone generator");
    }

    m_tg_inc = TWO_PI * m_tg_lo_freq / m_rate;
    m_tg_osc_cos = cos(m_tg_inc);
    m_tg_osc_sin = sin(m_tg_inc);
    m_is_init = true;
}

void QsToneGenerator::setFrequency(float frequency) { m_load_freq = frequency; }
void QsToneGenerator::setAmplitude(float amplitude) { m_tg_amplitude = amplitude; }

template <typename T> void QsToneGenerator::process(std::vector<T> &src_dst) {
    if (!m_is_init) {
        throw std::runtime_error("QsToneGenerator::process must call init() first!");
    }
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
    default:
        new_lo_freq = m_load_freq;
        break;
    }
    
    if (new_lo_freq != m_tg_lo_freq) {
        m_tg_lo_freq = new_lo_freq;
        m_tg_inc = TWO_PI * m_tg_lo_freq / m_rate;
        m_tg_osc_cos = cos(m_tg_inc);
        m_tg_osc_sin = sin(m_tg_inc);
    }

    if (m_tg_lo_freq == 0.0) {
        return;
    }

    for (auto &sample : src_dst) {
        // Calculate oscillation components
        m_tg_osc_re = m_tg_osc1_re * m_tg_osc_cos - m_tg_osc1_im * m_tg_osc_sin;
        m_tg_osc_im = m_tg_osc1_im * m_tg_osc_cos + m_tg_osc1_re * m_tg_osc_sin;
        double tg_gain = 1.95 - (m_tg_osc1_re * m_tg_osc1_re + m_tg_osc1_im * m_tg_osc_im);
        m_tg_osc1_re = tg_gain * m_tg_osc_re;
        m_tg_osc1_im = tg_gain * m_tg_osc_im;

        if (m_test_mode) {
            // Generate a pure tone with the set frequency
            if constexpr (std::is_same_v<T, std::complex<float>>) {
                sample = std::complex<float>(m_tg_amplitude * m_tg_osc_re, m_tg_amplitude * m_tg_osc_im);
            } else if constexpr (std::is_same_v<T, float>) {
                sample = m_tg_amplitude * m_tg_osc_re;
            }
        } else {
            // Mix the tone with the input signal
            if constexpr (std::is_same_v<T, std::complex<float>>) {
                sample.real(m_tg_amplitude * (sample.real() * m_tg_osc_re - sample.imag() * m_tg_osc_im));
                sample.imag(m_tg_amplitude * (sample.real() * m_tg_osc_im + sample.imag() * m_tg_osc_re));
            } else if constexpr (std::is_same_v<T, float>) {
                sample = m_tg_amplitude * sample * m_tg_osc_re;
            }
        }
    }
}

// Explicit template instantiation for required types
template void QsToneGenerator::process<std::complex<float>>(std::vector<std::complex<float>> &);
template void QsToneGenerator::process<float>(std::vector<float> &);
