// #include "../include/qs_nr_filter.hpp"

// QsNoiseReductionFilter ::QsNoiseReductionFilter()
//     : m_nr_switch(false), m_nr_lms_sz(0), m_nr_delay(0), m_nr_dl_indx(0), m_nr_mask(0), m_nr_adapt_rate(0),
//       m_nr_leakage(0), m_nr_adapt_size(0) {}

// void QsNoiseReductionFilter ::init(unsigned int size) {
//     m_nr_lms_sz = size;
//     m_nr_mask = m_nr_lms_sz - 1;

//     m_nr_switch = QsGlobal::g_memory->getNoiseReductionOn();
//     m_nr_adapt_rate = QsGlobal::g_memory->getNoiseReductionRate();
//     m_nr_leakage = QsGlobal::g_memory->getNoiseReductionLeak();
//     m_nr_adapt_size = QsGlobal::g_memory->getNoiseReductionTaps();
//     m_nr_delay = QsGlobal::g_memory->getNoiseReductionDelay();
//     m_nr_dl_indx = 0;

//     m_nr_delay_line.resize(m_nr_lms_sz);
//     QsSignalOps::Zero(m_nr_delay_line);
//     m_nr_coeff.resize(m_nr_lms_sz * 2);
//     QsSignalOps::Zero(m_nr_coeff);
// }

// void QsNoiseReductionFilter ::process(qs_vect_cpx &src_dst) {
//     m_nr_switch = QsGlobal::g_memory->getNoiseReductionOn();

//     if (m_nr_switch) {
//         m_nr_adapt_rate = QsGlobal::g_memory->getNoiseReductionRate();
//         m_nr_leakage = QsGlobal::g_memory->getNoiseReductionLeak();

//         if (m_nr_lms_sz != src_dst.size()) {
//             m_nr_lms_sz = src_dst.size();
//             m_nr_mask = m_nr_lms_sz - 1;
//             m_nr_delay_line.resize(m_nr_lms_sz);
//             QsSignalOps::Zero(m_nr_delay_line);
//             m_nr_coeff.resize(m_nr_lms_sz * 2);
//             QsSignalOps::Zero(m_nr_coeff);
//         }

//         if (m_nr_adapt_size != QsGlobal::g_memory->getNoiseReductionTaps()) {
//             m_nr_adapt_size = QsGlobal::g_memory->getNoiseReductionTaps();
//             m_nr_dl_indx = 0;
//             QsSignalOps::Zero(m_nr_delay_line);
//             QsSignalOps::Zero(m_nr_coeff);
//         }

//         if (m_nr_delay != QsGlobal::g_memory->getNoiseReductionDelay()) {
//             m_nr_delay = QsGlobal::g_memory->getNoiseReductionDelay();
//             m_nr_dl_indx = 0;
//             QsSignalOps::Zero(m_nr_delay_line);
//             QsSignalOps::Zero(m_nr_coeff);
//         }

//         double scl1 = 1.0 - m_nr_adapt_rate * m_nr_leakage;

//         for (m_cpx_iterator = src_dst.begin(); m_cpx_iterator != src_dst.end(); m_cpx_iterator++) {
//             m_nr_delay_line[m_nr_dl_indx] = (*m_cpx_iterator).real();
//             double accum = 0.0;
//             double sum_sq = 0.0;

//             for (int j = 0; j < m_nr_adapt_size; j++) {
//                 int k = (j + m_nr_delay + m_nr_dl_indx) & m_nr_mask;
//                 sum_sq += m_nr_delay_line[k] * m_nr_delay_line[k];
//                 accum += m_nr_coeff[j] * m_nr_delay_line[k];
//             }

//             double error = (*m_cpx_iterator).real() - accum;
//             double out = accum * 1.5;
//             (*m_cpx_iterator) = Cpx(out, out);

//             double scl2 = m_nr_adapt_rate / (sum_sq + 1e-10);
//             error *= scl2;

//             for (int j = 0; j < m_nr_adapt_size; j++) {
//                 int k = (j + m_nr_delay + m_nr_dl_indx) & m_nr_mask;
//                 m_nr_coeff[j] = m_nr_coeff[j] * scl1 + error * m_nr_delay_line[k];
//             }
//             m_nr_dl_indx = (m_nr_dl_indx + m_nr_mask) & m_nr_mask;
//         }
//     }
// }

#include "../include/qs_nr_filter.hpp"

QsNoiseReductionFilter::QsNoiseReductionFilter()
    : m_nr_switch(false), m_nr_lms_sz(0), m_nr_delay(0), m_nr_dl_indx(0), m_nr_mask(0), m_nr_adapt_rate(0),
      m_nr_leakage(0), m_nr_adapt_size(0) {}

void QsNoiseReductionFilter::init(unsigned int size) {
    m_nr_lms_sz = size;
    m_nr_mask = m_nr_lms_sz - 1;

    // Initialize filter parameters from global memory
    m_nr_switch = QsGlobal::g_memory->getNoiseReductionOn();
    m_nr_adapt_rate = QsGlobal::g_memory->getNoiseReductionRate();
    m_nr_leakage = QsGlobal::g_memory->getNoiseReductionLeak();
    m_nr_adapt_size = QsGlobal::g_memory->getNoiseReductionTaps();
    m_nr_delay = QsGlobal::g_memory->getNoiseReductionDelay();
    m_nr_dl_indx = 0;

    // Resize and zero-initialize the delay line and coefficients
    m_nr_delay_line.resize(m_nr_lms_sz);
    QsSignalOps::Zero(m_nr_delay_line);
    m_nr_coeff.resize(m_nr_lms_sz * 2); // Allocating twice the size for future flexibility
    QsSignalOps::Zero(m_nr_coeff);
}

void QsNoiseReductionFilter::process(qs_vect_cpx &src_dst) {
    // Fetch updated noise reduction switch state
    m_nr_switch = QsGlobal::g_memory->getNoiseReductionOn();

    if (m_nr_switch) {
        // Fetch updated parameters if noise reduction is enabled
        m_nr_adapt_rate = QsGlobal::g_memory->getNoiseReductionRate();
        m_nr_leakage = QsGlobal::g_memory->getNoiseReductionLeak();

        // Adapt filter size if the input size has changed
        if (m_nr_lms_sz != src_dst.size()) {
            m_nr_lms_sz = src_dst.size();
            m_nr_mask = m_nr_lms_sz - 1;
            m_nr_delay_line.resize(m_nr_lms_sz);
            QsSignalOps::Zero(m_nr_delay_line);
            m_nr_coeff.resize(m_nr_lms_sz * 2);
            QsSignalOps::Zero(m_nr_coeff);
        }

        // Update filter taps if the number of adaptive taps has changed
        unsigned int new_adapt_size = QsGlobal::g_memory->getNoiseReductionTaps();
        if (m_nr_adapt_size != new_adapt_size) {
            m_nr_adapt_size = new_adapt_size;
            m_nr_dl_indx = 0;
            QsSignalOps::Zero(m_nr_delay_line);
            QsSignalOps::Zero(m_nr_coeff);
        }

        // Update delay if the noise reduction delay has changed
        unsigned int new_delay = QsGlobal::g_memory->getNoiseReductionDelay();
        if (m_nr_delay != new_delay) {
            m_nr_delay = new_delay;
            m_nr_dl_indx = 0;
            QsSignalOps::Zero(m_nr_delay_line);
            QsSignalOps::Zero(m_nr_coeff);
        }

        // Precompute scaling factor for coefficient leakage
        double scl1 = 1.0 - m_nr_adapt_rate * m_nr_leakage;

        // Process each sample in the input/output vector
        for (auto &sample : src_dst) {
            // Update the delay line with the current sample's real part
            m_nr_delay_line[m_nr_dl_indx] = sample.real();
            double accum = 0.0;
            double sum_sq = 0.0;

            // Compute the output based on adaptive coefficients and delay line
            for (unsigned int j = 0; j < m_nr_adapt_size; j++) {
                unsigned int k = (j + m_nr_delay + m_nr_dl_indx) & m_nr_mask;
                sum_sq += m_nr_delay_line[k] * m_nr_delay_line[k];
                accum += m_nr_coeff[j] * m_nr_delay_line[k];
            }

            // Compute the error and apply scaling
            double error = sample.real() - accum;
            double out = accum * 1.5; // Apply gain to the accumulated value
            sample = Cpx(out, out);   // Set real and imaginary parts to the same value

            // Adaptive update of coefficients
            double scl2 = m_nr_adapt_rate / (sum_sq + 1e-10); // Prevent division by zero
            error *= scl2;

            for (unsigned int j = 0; j < m_nr_adapt_size; j++) {
                unsigned int k = (j + m_nr_delay + m_nr_dl_indx) & m_nr_mask;
                m_nr_coeff[j] = m_nr_coeff[j] * scl1 + error * m_nr_delay_line[k];
            }

            // Update delay line index
            m_nr_dl_indx = (m_nr_dl_indx + 1) & m_nr_mask;
        }
    }
}
