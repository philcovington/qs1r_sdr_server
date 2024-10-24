// #include "../include/qs_auto_notch_filter.hpp"

// QsAutoNotchFilter ::QsAutoNotchFilter()
//     : m_anf_lms_sz(0), m_anf_mask(0), m_anf_switch(false), m_anf_adapt_rate(0), m_anf_leakage(0),
//     m_anf_adapt_size(0),
//       m_anf_delay(0), m_anf_dl_indx(0) {}

// void QsAutoNotchFilter ::init(unsigned int size) {
//     m_anf_lms_sz = size;
//     m_anf_mask = m_anf_lms_sz - 1;

//     m_anf_switch = QsGlobal::g_memory->getAutoNotchOn();
//     m_anf_adapt_rate = QsGlobal::g_memory->getAutoNotchRate();
//     m_anf_leakage = QsGlobal::g_memory->getAutoNotchLeak();
//     m_anf_adapt_size = QsGlobal::g_memory->getAutoNotchTaps();
//     m_anf_delay = QsGlobal::g_memory->getAutoNotchDelay();
//     m_anf_dl_indx = 0;

//     m_anf_delay_line.resize(size);
//     QsSignalOps::Zero(m_anf_delay_line);
//     m_anf_coeff.resize(size * 2);
//     QsSignalOps::Zero(m_anf_coeff);
// }

// void QsAutoNotchFilter ::process(qs_vect_cpx &src_dst) {
//     m_anf_switch = QsGlobal::g_memory->getAutoNotchOn();

//     if (m_anf_switch) {
//         m_anf_adapt_rate = QsGlobal::g_memory->getAutoNotchRate();
//         m_anf_leakage = QsGlobal::g_memory->getAutoNotchLeak();

//         if (m_anf_lms_sz != src_dst.size()) {
//             m_anf_lms_sz = src_dst.size();
//             m_anf_mask = m_anf_lms_sz - 1;
//             m_anf_delay_line.resize(m_anf_lms_sz);
//             QsSignalOps::Zero(m_anf_delay_line);
//             m_anf_coeff.resize(m_anf_lms_sz * 2);
//             QsSignalOps::Zero(m_anf_coeff);
//         }

//         if (m_anf_adapt_size != QsGlobal::g_memory->getAutoNotchTaps()) {
//             m_anf_adapt_size = QsGlobal::g_memory->getAutoNotchTaps();
//             m_anf_dl_indx = 0;
//             QsSignalOps::Zero(m_anf_delay_line);
//             QsSignalOps::Zero(m_anf_coeff);
//         }

//         if (m_anf_delay != QsGlobal::g_memory->getAutoNotchDelay()) {
//             m_anf_delay = QsGlobal::g_memory->getAutoNotchDelay();
//             m_anf_dl_indx = 0;
//             QsSignalOps::Zero(m_anf_delay_line);
//             QsSignalOps::Zero(m_anf_coeff);
//         }

//         double scl1 = 1.0 - m_anf_adapt_rate * m_anf_leakage;

//         for (m_cpx_iterator = src_dst.begin(); m_cpx_iterator != src_dst.end(); m_cpx_iterator++) {
//             m_anf_delay_line[m_anf_dl_indx] = (*m_cpx_iterator).real();
//             double accum = 0.0;
//             double sum_sq = 0.0;

//             for (int j = 0; j < m_anf_adapt_size; j++) {
//                 int k = (j + m_anf_delay + m_anf_dl_indx) & m_anf_mask;
//                 sum_sq += m_anf_delay_line[k] * m_anf_delay_line[k];
//                 accum += m_anf_coeff[j] * m_anf_delay_line[k];
//             }

//             double error = (*m_cpx_iterator).real() - accum;
//             (*m_cpx_iterator) = Cpx(error, error);

//             double scl2 = m_anf_adapt_rate / (sum_sq + 1e-10);
//             error *= scl2;

//             for (int j = 0; j < m_anf_adapt_size; j++) {
//                 int k = (j + m_anf_delay + m_anf_dl_indx) & m_anf_mask;
//                 m_anf_coeff[j] = m_anf_coeff[j] * scl1 + error * m_anf_delay_line[k];
//             }
//             m_anf_dl_indx = (m_anf_dl_indx + m_anf_mask) & m_anf_mask;
//         }
//     }
// }

#include "../include/qs_auto_notch_filter.hpp"

QsAutoNotchFilter::QsAutoNotchFilter()
    : m_anf_lms_sz(0), m_anf_mask(0), m_anf_switch(false), m_anf_adapt_rate(0), m_anf_leakage(0), m_anf_adapt_size(0),
      m_anf_delay(0), m_anf_dl_indx(0) {}

void QsAutoNotchFilter::init(unsigned int size) {
    m_anf_lms_sz = size;
    m_anf_mask = m_anf_lms_sz - 1;

    // Initialize auto-notch filter parameters from global memory
    m_anf_switch = QsGlobal::g_memory->getAutoNotchOn();
    m_anf_adapt_rate = QsGlobal::g_memory->getAutoNotchRate();
    m_anf_leakage = QsGlobal::g_memory->getAutoNotchLeak();
    m_anf_adapt_size = QsGlobal::g_memory->getAutoNotchTaps();
    m_anf_delay = QsGlobal::g_memory->getAutoNotchDelay();
    m_anf_dl_indx = 0;

    // Resize and initialize the delay line and coefficient vectors
    m_anf_delay_line.resize(size);
    QsSignalOps::Zero(m_anf_delay_line);
    m_anf_coeff.resize(size * 2); // Preallocate larger size for flexibility
    QsSignalOps::Zero(m_anf_coeff);
}

void QsAutoNotchFilter::process(qs_vect_cpx &src_dst) {
    // Check if auto-notch filtering is enabled
    m_anf_switch = QsGlobal::g_memory->getAutoNotchOn();

    if (m_anf_switch) {
        // Fetch updated parameters from global memory
        m_anf_adapt_rate = QsGlobal::g_memory->getAutoNotchRate();
        m_anf_leakage = QsGlobal::g_memory->getAutoNotchLeak();

        // Adjust the filter size if the input size has changed
        if (m_anf_lms_sz != src_dst.size()) {
            m_anf_lms_sz = src_dst.size();
            m_anf_mask = m_anf_lms_sz - 1;
            m_anf_delay_line.resize(m_anf_lms_sz);
            QsSignalOps::Zero(m_anf_delay_line);
            m_anf_coeff.resize(m_anf_lms_sz * 2);
            QsSignalOps::Zero(m_anf_coeff);
        }

        // Update number of taps if it has changed
        unsigned int new_adapt_size = QsGlobal::g_memory->getAutoNotchTaps();
        if (m_anf_adapt_size != new_adapt_size) {
            m_anf_adapt_size = new_adapt_size;
            m_anf_dl_indx = 0;
            QsSignalOps::Zero(m_anf_delay_line);
            QsSignalOps::Zero(m_anf_coeff);
        }

        // Update the delay if it has changed
        unsigned int new_delay = QsGlobal::g_memory->getAutoNotchDelay();
        if (m_anf_delay != new_delay) {
            m_anf_delay = new_delay;
            m_anf_dl_indx = 0;
            QsSignalOps::Zero(m_anf_delay_line);
            QsSignalOps::Zero(m_anf_coeff);
        }

        // Precompute scaling factor for leakage
        double scl1 = 1.0 - m_anf_adapt_rate * m_anf_leakage;

        // Process each sample in the source/destination vector
        for (auto &sample : src_dst) {
            // Update the delay line with the current sample's real part
            m_anf_delay_line[m_anf_dl_indx] = sample.real();
            double accum = 0.0;
            double sum_sq = 0.0;

            // Compute the adaptive filter output
            for (unsigned int j = 0; j < m_anf_adapt_size; j++) {
                unsigned int k = (j + m_anf_delay + m_anf_dl_indx) & m_anf_mask;
                sum_sq += m_anf_delay_line[k] * m_anf_delay_line[k];
                accum += m_anf_coeff[j] * m_anf_delay_line[k];
            }

            // Calculate error and apply it to the output
            double error = sample.real() - accum;
            sample = Cpx(error, error); // Set real and imaginary parts to the error

            // Update the adaptive coefficients based on the error
            double scl2 = m_anf_adapt_rate / (sum_sq + 1e-10); // Prevent division by zero
            error *= scl2;

            for (unsigned int j = 0; j < m_anf_adapt_size; j++) {
                unsigned int k = (j + m_anf_delay + m_anf_dl_indx) & m_anf_mask;
                m_anf_coeff[j] = m_anf_coeff[j] * scl1 + error * m_anf_delay_line[k];
            }

            // Update the delay line index
            m_anf_dl_indx = (m_anf_dl_indx + 1) & m_anf_mask;
        }
    }
}
