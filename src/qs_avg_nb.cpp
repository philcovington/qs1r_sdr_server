#include "../include/qs_avg_nb.hpp"
#include <stdexcept>
#include <cmath>

QsAveragingNoiseBlanker::QsAveragingNoiseBlanker()
    : m_anb_avg_sig(cpx_zero), m_anb_magnitude(0.0), m_anb_avg_magn(0.0),
      m_anb_switch(false), m_anb_thres(0.0f) {}

void QsAveragingNoiseBlanker::init() {
    m_anb_avg_sig = cpx_zero;
    m_anb_magnitude = 0.0;
    m_anb_avg_magn = 0.0;
    m_is_init = true;
}

void QsAveragingNoiseBlanker::process(qs_vect_cpx &src_dst) {
    if (!m_is_init) {
        throw std::runtime_error("QsAveragingNoiseBlanker::process must call init() first!");
    }
    // Ensure global memory object exists
    if (!QsGlobal::g_memory) {
        throw std::runtime_error("Global memory is null in QsAveragingNoiseBlanker::process.");
    }

    // Get switch and threshold values from global settings
    m_anb_switch = QsGlobal::g_memory->getAvgNoiseBlankerOn();
    m_anb_thres = QsGlobal::g_memory->getAvgNoiseBlankerThreshold();

    if (m_anb_switch) {
        for (auto& sample : src_dst) {
            // Calculate the magnitude of the current sample
            m_anb_magnitude = std::sqrt(sample.real() * sample.real() + sample.imag() * sample.imag());

            // Calculate an average of the signal with exponential smoothing
            m_anb_avg_sig = Cpx(
                (m_anb_avg_sig.real() * 0.75) + (sample.real() * 0.25),
                (m_anb_avg_sig.imag() * 0.75) + (sample.imag() * 0.25)
            );

            // Update the average magnitude with a small smoothing factor
            m_anb_avg_magn = (0.999 * m_anb_avg_magn) + (0.001 * m_anb_magnitude);

            // Replace the sample if magnitude exceeds the threshold times the average magnitude
            if (m_anb_magnitude > (m_anb_thres * m_anb_avg_magn)) {
                sample = m_anb_avg_sig;
            }
        }
    }
}

