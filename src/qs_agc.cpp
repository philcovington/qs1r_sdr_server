// #include "../include/qs_agc.hpp"
// #include "../include/qs_globals.hpp"
// #include "../include/qs_signalops.hpp"
// #include <iterator>
// #include <stdexcept>

// using namespace std;

// QsAgc ::QsAgc()
//     : m_post_processing_rate(0), m_agc_use_hang(false), m_agc_threshold(-90), m_agc_manual_gain(0), m_agc_slope(0),
//       m_agc_hang_time(0), m_agc_hang_time_set(0), m_agc_decay(QS_DEFAULT_AGC_LONG_DECAY), m_agc_decay_set(0),
//       m_agc_sample_rate(0), m_agc_sigdly_ptr(0), m_agc_hang_timer(0), m_agc_peak(0), m_agc_decay_avg(0),
//       m_agc_attack_avg(0), m_agc_magbuffer_pos(0), m_agc_current_gain(0), m_agc_fixed_manual_gain(0), m_agc_knee(0),
//       m_agc_gain_slope(0), m_agc_fixed_gain(0), m_agc_attack_rise_alpha(0), m_agc_attack_fall_alpha(0),
//       m_agc_decay_rise_alpha(0), m_agc_decay_fall_alpha(0), m_agc_delay_samples(0), m_agc_window_samples(0) {}

// void QsAgc ::init() {
//     m_agc_decay = QsGlobal::g_memory->getAgcDecaySpeed();
//     m_post_processing_rate = QsGlobal::g_memory->getDataPostProcRate();
//     m_agc_use_hang = QsGlobal::g_memory->getAgcHangTimeSwitch();
//     m_agc_threshold = QsGlobal::g_memory->getAgcThreshold();
//     m_agc_manual_gain = QsGlobal::g_memory->getAgcFixedGain();
//     m_agc_slope = QsGlobal::g_memory->getAgcSlope();
//     m_agc_hang_time = QsGlobal::g_memory->getAgcHangTime();
//     m_agc_hang_time_set = m_post_processing_rate * m_agc_hang_time * 0.001; // .001 for mS

//     if (m_agc_use_hang)
//         m_agc_decay_set = m_agc_decay + m_agc_hang_time;
//     else
//         m_agc_decay_set = m_agc_decay;

//     if (m_agc_sample_rate != m_post_processing_rate) {
//         m_agc_sample_rate = m_post_processing_rate;

//         m_agc_signalDelayBuffer.resize((int)AGC_MAX_BUFFER_SZ, cpx_zero);
//         m_agc_magBuffer.resize((int)AGC_MAX_BUFFER_SZ, 0.0);

//         m_agc_sigdly_ptr = 0;
//         m_agc_hang_timer = 0;
//         m_agc_peak = -16;
//         m_agc_decay_avg = -5;
//         m_agc_attack_avg = -5;
//         m_agc_magbuffer_pos = 0;
//         m_agc_current_gain = 0.0;
//     }

//     m_agc_fixed_manual_gain = pow(10.0, (m_agc_manual_gain / 20.0));

//     m_agc_knee = m_agc_threshold / 20.0; // scale to -8.0 to 0.0
//     m_agc_gain_slope = m_agc_slope / 100.0;
//     m_agc_fixed_gain = AGC_OUTPUT_SCALING * pow(10.0, m_agc_knee * (m_agc_gain_slope - 1.0));

//     m_agc_attack_rise_alpha = (1.0 - exp(-1.0 / (m_post_processing_rate * AGC_ATTACK_RISE_TC)));
//     m_agc_attack_fall_alpha = (1.0 - exp(-1.0 / (m_post_processing_rate * AGC_ATTACK_FALL_TC)));

//     m_agc_decay_rise_alpha =
//         (1.0 - exp(-1.0 / (m_post_processing_rate * m_agc_decay_set * 0.001 * AGC_RISEFALL_RATIO)));

//     if (m_agc_use_hang)
//         m_agc_decay_fall_alpha = (1.0 - exp(-1.0 / (m_post_processing_rate * AGC_RELEASE_TC)));
//     else
//         m_agc_decay_fall_alpha =
//             (1.0 - exp(-1.0 / (m_post_processing_rate * m_agc_decay_set * 0.001))); // no hang .001 for mS

//     m_agc_delay_samples = m_post_processing_rate * AGC_DELAY_TC;
//     m_agc_window_samples = m_post_processing_rate * AGC_WINDOW_TC;

//     if (m_agc_delay_samples > AGC_MAX_BUFFER_SZ - 1)
//         m_agc_delay_samples = AGC_MAX_BUFFER_SZ - 1;
// }

// void QsAgc ::process(qs_vect_cpx &src_dst) {
//     if (m_agc_decay != QsGlobal::g_memory->getAgcDecaySpeed()) {
//         m_agc_decay = QsGlobal::g_memory->getAgcDecaySpeed();

//         if (m_agc_use_hang)
//             m_agc_decay_set = m_agc_decay + m_agc_hang_time;
//         else
//             m_agc_decay_set = m_agc_decay;

//         m_agc_decay_rise_alpha =
//             (1.0 - exp(-1.0 / (m_post_processing_rate * m_agc_decay_set * 0.001 * AGC_RISEFALL_RATIO)));

//         if (m_agc_use_hang)
//             m_agc_decay_fall_alpha = (1.0 - exp(-1.0 / (m_post_processing_rate * AGC_RELEASE_TC)));
//         else
//             m_agc_decay_fall_alpha =
//                 (1.0 - exp(-1.0 / (m_post_processing_rate * m_agc_decay_set * 0.001))); // no hang .001 for mS
//     }

//     if (m_agc_threshold != QsGlobal::g_memory->getAgcThreshold()) {
//         m_agc_threshold = QsGlobal::g_memory->getAgcThreshold();
//         init();
//     }

//     if (m_agc_manual_gain != QsGlobal::g_memory->getAgcFixedGain()) {
//         m_agc_manual_gain = QsGlobal::g_memory->getAgcFixedGain();
//         init();
//     }

//     if (m_agc_slope != QsGlobal::g_memory->getAgcSlope()) {
//         m_agc_slope = QsGlobal::g_memory->getAgcSlope();
//         init();
//     }

//     if (m_agc_hang_time != QsGlobal::g_memory->getAgcHangTime()) {
//         m_agc_hang_time = QsGlobal::g_memory->getAgcHangTime();
//         m_agc_hang_time_set = m_post_processing_rate * m_agc_hang_time * 0.001; // .001 for mS
//         init();
//     }

//     m_agc_use_hang = QsGlobal::g_memory->getAgcHangTimeSwitch();

//     if (m_agc_decay == 0) {
//         for (m_cpx_iterator = src_dst.begin(); m_cpx_iterator != src_dst.end(); m_cpx_iterator++)
//             (*m_cpx_iterator) *= m_agc_fixed_manual_gain;
//     } else {
//         float mag;
//         Cpx delayed_input;
//         for (m_cpx_iterator = src_dst.begin(); m_cpx_iterator != src_dst.end(); m_cpx_iterator++) {
//             Cpx in = (*m_cpx_iterator);                                // latest input sample
//             delayed_input = m_agc_signalDelayBuffer[m_agc_sigdly_ptr]; // get delayed sample
//             m_agc_signalDelayBuffer[m_agc_sigdly_ptr++] = in;          // put input into delay buffer

//             if (m_agc_sigdly_ptr >= m_agc_delay_samples)
//                 m_agc_sigdly_ptr = 0; // buffer wrap around

//             mag = fabs(in.real());
//             float min_im = fabs(in.imag());
//             if (min_im > mag)
//                 mag = min_im;
//             mag = log10(mag + AGC_MIN_CONSTANT);

//             float tmp = m_agc_magBuffer[m_agc_magbuffer_pos]; // get oldest mag sample from mag buffer
//             m_agc_magBuffer[m_agc_magbuffer_pos++] = mag;     // put latest mag value into mag buffer
//             if (m_agc_magbuffer_pos >= m_agc_window_samples)
//                 m_agc_magbuffer_pos = 0; // buffer wrap around

//             if (mag > m_agc_peak)
//                 m_agc_peak = mag;
//             else {
//                 if (tmp == m_agc_peak) // find new peak
//                 {
//                     m_agc_peak = -8.0;
//                     for (int n = 0; n < m_agc_window_samples; n++) {
//                         tmp = m_agc_magBuffer[n];
//                         if (tmp > m_agc_peak)
//                             m_agc_peak = tmp;
//                     }
//                 }
//             }

//             if (m_agc_use_hang) // using hang timer
//             {
//                 if (m_agc_peak > m_agc_attack_avg)
//                     m_agc_attack_avg =
//                         (1.0 - m_agc_attack_rise_alpha) * m_agc_attack_avg + m_agc_attack_rise_alpha * m_agc_peak;
//                 else
//                     m_agc_attack_avg =
//                         (1.0 - m_agc_attack_fall_alpha) * m_agc_attack_avg + m_agc_attack_fall_alpha * m_agc_peak;

//                 if (m_agc_peak > m_agc_decay_avg) {
//                     m_agc_decay_avg =
//                         (1.0 - m_agc_decay_rise_alpha) * m_agc_decay_avg + m_agc_decay_rise_alpha * m_agc_peak;
//                     m_agc_hang_timer = 0;
//                 } else // if signal decreasing use hang time
//                 {
//                     if (m_agc_hang_timer < m_agc_hang_time_set)
//                         m_agc_hang_timer++;
//                     else
//                         m_agc_decay_avg =
//                             (1.0 - m_agc_decay_fall_alpha) * m_agc_decay_avg + m_agc_decay_fall_alpha * m_agc_peak;
//                 }
//             } else // use exponential decay
//             {
//                 if (m_agc_peak > m_agc_attack_avg)
//                     m_agc_attack_avg =
//                         (1.0 - m_agc_attack_rise_alpha) * m_agc_attack_avg + m_agc_attack_rise_alpha * m_agc_peak;
//                 else
//                     m_agc_attack_avg =
//                         (1.0 - m_agc_attack_fall_alpha) * m_agc_attack_avg + m_agc_attack_fall_alpha * m_agc_peak;

//                 if (m_agc_peak > m_agc_decay_avg)
//                     m_agc_decay_avg =
//                         (1.0 - m_agc_decay_rise_alpha) * m_agc_decay_avg + m_agc_decay_rise_alpha * m_agc_peak;
//                 else
//                     m_agc_decay_avg =
//                         (1.0 - m_agc_decay_fall_alpha) * m_agc_decay_avg + m_agc_decay_fall_alpha * m_agc_peak;
//             }

//             if (m_agc_attack_avg > m_agc_decay_avg)
//                 mag = m_agc_attack_avg;
//             else
//                 mag = m_agc_decay_avg;

//             if (mag <= m_agc_knee)
//                 m_agc_current_gain = m_agc_fixed_gain;
//             else
//                 m_agc_current_gain = AGC_OUTPUT_SCALING * pow(10.0, mag * (m_agc_gain_slope - 1.0));

//             (*m_cpx_iterator) = delayed_input * (float)m_agc_current_gain;
//         }
//         double gain = 20.0 * log10(m_agc_current_gain) + 3.0;
//         QsGlobal::g_memory->setAgcCurrentGain(gain);
//         QsGlobal::g_memory->setAgcCurrentGainC(std::round(gain));
//     }
// }

#include "../include/qs_agc.hpp"
#include "../include/qs_globals.hpp"
#include "../include/qs_signalops.hpp"
#include <iterator>
#include <stdexcept>

using namespace std;

QsAgc::QsAgc()
    : m_post_processing_rate(0), m_agc_use_hang(false), m_agc_threshold(-90), m_agc_manual_gain(0), m_agc_slope(0),
      m_agc_hang_time(0), m_agc_hang_time_set(0), m_agc_decay(QS_DEFAULT_AGC_LONG_DECAY), m_agc_decay_set(0),
      m_agc_sample_rate(0), m_agc_sigdly_ptr(0), m_agc_hang_timer(0), m_agc_peak(0), m_agc_decay_avg(0),
      m_agc_attack_avg(0), m_agc_magbuffer_pos(0), m_agc_current_gain(0), m_agc_fixed_manual_gain(0), m_agc_knee(0),
      m_agc_gain_slope(0), m_agc_fixed_gain(0), m_agc_attack_rise_alpha(0), m_agc_attack_fall_alpha(0),
      m_agc_decay_rise_alpha(0), m_agc_decay_fall_alpha(0), m_agc_delay_samples(0), m_agc_window_samples(0) {}

void QsAgc::init() {
    m_agc_decay = QsGlobal::g_memory->getAgcDecaySpeed();
    m_post_processing_rate = QsGlobal::g_memory->getDataPostProcRate();
    m_agc_use_hang = QsGlobal::g_memory->getAgcHangTimeSwitch();
    m_agc_threshold = QsGlobal::g_memory->getAgcThreshold();
    m_agc_manual_gain = QsGlobal::g_memory->getAgcFixedGain();
    m_agc_slope = QsGlobal::g_memory->getAgcSlope();
    m_agc_hang_time = QsGlobal::g_memory->getAgcHangTime();
    m_agc_hang_time_set = m_post_processing_rate * m_agc_hang_time * 0.001; // Convert to ms

    m_agc_decay_set = m_agc_use_hang ? m_agc_decay + m_agc_hang_time : m_agc_decay;

    if (m_agc_sample_rate != m_post_processing_rate) {
        m_agc_sample_rate = m_post_processing_rate;
        m_agc_signalDelayBuffer.resize(AGC_MAX_BUFFER_SZ, cpx_zero);
        m_agc_magBuffer.resize(AGC_MAX_BUFFER_SZ, 0.0);
        m_agc_sigdly_ptr = 0;
        m_agc_hang_timer = 0;
        m_agc_peak = -16;
        m_agc_decay_avg = -5;
        m_agc_attack_avg = -5;
        m_agc_magbuffer_pos = 0;
        m_agc_current_gain = 0.0;
    }

    m_agc_fixed_manual_gain = pow(10.0, m_agc_manual_gain / 20.0);
    m_agc_knee = m_agc_threshold / 20.0;
    m_agc_gain_slope = m_agc_slope / 100.0;
    m_agc_fixed_gain = AGC_OUTPUT_SCALING * pow(10.0, m_agc_knee * (m_agc_gain_slope - 1.0));

    m_agc_attack_rise_alpha = 1.0 - exp(-1.0 / (m_post_processing_rate * AGC_ATTACK_RISE_TC));
    m_agc_attack_fall_alpha = 1.0 - exp(-1.0 / (m_post_processing_rate * AGC_ATTACK_FALL_TC));

    m_agc_decay_rise_alpha = 1.0 - exp(-1.0 / (m_post_processing_rate * m_agc_decay_set * 0.001 * AGC_RISEFALL_RATIO));
    m_agc_decay_fall_alpha = m_agc_use_hang
                                 ? 1.0 - exp(-1.0 / (m_post_processing_rate * AGC_RELEASE_TC))
                                 : 1.0 - exp(-1.0 / (m_post_processing_rate * m_agc_decay_set * 0.001)); // No hang

    m_agc_delay_samples = min((int)(m_post_processing_rate * AGC_DELAY_TC), AGC_MAX_BUFFER_SZ - 1);
    m_agc_window_samples = m_post_processing_rate * AGC_WINDOW_TC;
}

float QsAgc::update_avg(float avg, float value, float rise_alpha, float fall_alpha) {
    return value > avg ? (1.0 - rise_alpha) * avg + rise_alpha * value : (1.0 - fall_alpha) * avg + fall_alpha * value;
}

void QsAgc::process(qs_vect_cpx &src_dst) {
    if (m_agc_decay != QsGlobal::g_memory->getAgcDecaySpeed()) {
        m_agc_decay = QsGlobal::g_memory->getAgcDecaySpeed();
        m_agc_decay_set = m_agc_use_hang ? m_agc_decay + m_agc_hang_time : m_agc_decay;
        m_agc_decay_rise_alpha =
            1.0 - exp(-1.0 / (m_post_processing_rate * m_agc_decay_set * 0.001 * AGC_RISEFALL_RATIO));
        m_agc_decay_fall_alpha = m_agc_use_hang ? 1.0 - exp(-1.0 / (m_post_processing_rate * AGC_RELEASE_TC))
                                                : 1.0 - exp(-1.0 / (m_post_processing_rate * m_agc_decay_set * 0.001));
    }

    if (m_agc_threshold != QsGlobal::g_memory->getAgcThreshold() ||
        m_agc_manual_gain != QsGlobal::g_memory->getAgcFixedGain() ||
        m_agc_slope != QsGlobal::g_memory->getAgcSlope() || m_agc_hang_time != QsGlobal::g_memory->getAgcHangTime()) {
        init(); // Re-initialize on parameter change
    }

    if (m_agc_decay == 0) {
        for (auto &sample : src_dst) {
            sample *= m_agc_fixed_manual_gain;
        }
    } else {
        float mag;
        Cpx delayed_input;
        for (auto &sample : src_dst) {
            Cpx in = sample;
            delayed_input = m_agc_signalDelayBuffer[m_agc_sigdly_ptr];
            m_agc_signalDelayBuffer[m_agc_sigdly_ptr++] = in;

            if (m_agc_sigdly_ptr >= m_agc_delay_samples)
                m_agc_sigdly_ptr = 0; // Wrap buffer

            mag = log10(fmax(fabs(in.real()), fabs(in.imag())) + AGC_MIN_CONSTANT);

            float old_mag = m_agc_magBuffer[m_agc_magbuffer_pos];
            m_agc_magBuffer[m_agc_magbuffer_pos++] = mag;
            if (m_agc_magbuffer_pos >= m_agc_window_samples)
                m_agc_magbuffer_pos = 0; // Wrap buffer

            if (mag > m_agc_peak) {
                m_agc_peak = mag;
            } else if (old_mag == m_agc_peak) {
                m_agc_peak = *max_element(m_agc_magBuffer.begin(), m_agc_magBuffer.begin() + m_agc_window_samples);
            }

            if (m_agc_use_hang) {
                m_agc_attack_avg = update_avg(m_agc_attack_avg, mag, m_agc_attack_rise_alpha, m_agc_attack_fall_alpha);
                if (mag > m_agc_decay_avg) {
                    m_agc_decay_avg = update_avg(m_agc_decay_avg, mag, m_agc_decay_rise_alpha, m_agc_decay_fall_alpha);
                    m_agc_hang_timer = 0;
                } else if (m_agc_hang_timer < m_agc_hang_time_set) {
                    m_agc_hang_timer++;
                } else {
                    m_agc_decay_avg = update_avg(m_agc_decay_avg, mag, m_agc_decay_rise_alpha, m_agc_decay_fall_alpha);
                }
            } else {
                m_agc_attack_avg = update_avg(m_agc_attack_avg, mag, m_agc_attack_rise_alpha, m_agc_attack_fall_alpha);
                m_agc_decay_avg = update_avg(m_agc_decay_avg, mag, m_agc_decay_rise_alpha, m_agc_decay_fall_alpha);
            }

            mag = max(m_agc_attack_avg, m_agc_decay_avg);
            m_agc_current_gain =
                mag <= m_agc_knee ? m_agc_fixed_gain : AGC_OUTPUT_SCALING * pow(10.0, mag * (m_agc_gain_slope - 1.0));

            sample = delayed_input * static_cast<float>(m_agc_current_gain);
        }

        double gain_db = 20.0 * log10(m_agc_current_gain) + 3.0;
        QsGlobal::g_memory->setAgcCurrentGain(gain_db);
        QsGlobal::g_memory->setAgcCurrentGainC(round(gain_db));
    }
}


