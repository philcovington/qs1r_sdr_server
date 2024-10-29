#include "../include/qs_blk_nb.hpp"

QsBlockNoiseBlanker::QsBlockNoiseBlanker()
    : m_bnb_magnitude(0.0f), m_bnb_avg_magn(0.0f), m_bnb_switch(false), m_bnb_thres(0.0f), m_bnb_sig_index(0),
      m_bnb_dly_index(0), m_bnb_hangtime(0), m_bnb_avg_sig(cpx_zero) {}

void QsBlockNoiseBlanker::init() {
    bnb_delay_line.assign(8, cpx_zero); // Ensure resize with initialization
    m_bnb_avg_sig = cpx_zero;

    m_bnb_magnitude = 0.0f;
    m_bnb_avg_magn = 0.0f;
    m_bnb_sig_index = 0;
    m_bnb_dly_index = 0;
    m_bnb_hangtime = 0;
    m_is_init = true;
}

void QsBlockNoiseBlanker::process(qs_vect_cpx &src_dst) {
    if (!m_is_init) {
        throw std::runtime_error("QsBlockNoiseBlanker::process must call init() first!");
    }
    // Retrieve the switch state and threshold once, outside the loop
    m_bnb_switch = QsGlobal::g_memory->getBlockNoiseBlankerOn();
    m_bnb_thres = QsGlobal::g_memory->getBlockNoiseBlankerThreshold();

    if (!m_bnb_switch)
        return; // Early exit if switch is off

    for (auto &sample : src_dst) {
        // Calculate magnitude squared (faster than std::abs)
        m_bnb_magnitude = std::norm(sample);
        bnb_delay_line[m_bnb_sig_index] = sample; // Update delay line with current sample

        // Update average magnitude
        m_bnb_avg_magn = (0.999f * m_bnb_avg_magn) + (0.001f * m_bnb_magnitude);

        // Determine if hangtime should be set based on threshold comparison
        if (m_bnb_hangtime == 0 && m_bnb_magnitude > m_bnb_thres * m_bnb_avg_magn) {
            m_bnb_hangtime = 7; // Set hangtime duration
        }

        if (m_bnb_hangtime > 0) {
            sample = cpx_zero; // Blank the sample if in hangtime
            m_bnb_hangtime--;
        } else {
            sample = bnb_delay_line[m_bnb_dly_index]; // Restore delayed sample
        }

        // Update indices with modulo operation for circular buffer behavior
        m_bnb_sig_index = (m_bnb_sig_index + 1) % 8;
        m_bnb_dly_index = (m_bnb_dly_index + 1) % 8;
    }
}
