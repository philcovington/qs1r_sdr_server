// #include "../include/qs_smeter.hpp"
// #include "../include/qs_defines.hpp"

// QsSMeter ::QsSMeter() : m_sm_tmp_val(0), m_sm_value(0) {}

// void QsSMeter ::init() {
//     m_sm_tmp_val = 0.0;
//     m_sm_value = 0.0;
// }

// void QsSMeter ::process(qs_vect_cpx &src_dst) {
//     m_sm_tmp_val = 0.0;
//     for (m_cpx_iterator = src_dst.begin(); m_cpx_iterator != src_dst.end(); m_cpx_iterator++) {
//         m_sm_tmp_val +=
//             (*m_cpx_iterator).real() * (*m_cpx_iterator).real() + (*m_cpx_iterator).imag() *
//             (*m_cpx_iterator).imag();
//     }
//     // m_sm_value = ( m_sm_value * 0.5 ) + ( ( 10.0 * log10( m_sm_tmp_val + 1e-200 ) ) * 0.5 );
//     m_sm_value = 10.0 * log10(m_sm_tmp_val + 1e-200);
//     //double corrected_sm = m_sm_value + QsGlobal::g_memory->getSMeterCorrection() + SMETERCORRECT;
//     double corrected_sm = m_sm_value + QsGlobal::g_memory->getSMeterCorrection();
//     QsGlobal::g_memory->setSMeterCurrentValue(corrected_sm);
//     QsGlobal::g_memory->setSMeterCurrentValueC((unsigned char)std::round(corrected_sm + QS_DEFAULT_SPEC_OFFSET));
// }

#include "../include/qs_smeter.hpp"
#include "../include/qs_defines.hpp"

QsSMeter::QsSMeter() : m_sm_tmp_val(0.0), m_sm_value(0.0) {}

void QsSMeter::init() {
    m_sm_tmp_val = 0.0;
    m_sm_value = 0.0;
}

void QsSMeter::process(qs_vect_cpx &src_dst) {
    // Reset temporary value for new calculation
    m_sm_tmp_val = 0.0;

    // Accumulate the power of each complex sample
    for (const auto &cpx : src_dst) {
        m_sm_tmp_val += std::norm(cpx); // Efficient way to calculate real^2 + imag^2
    }

    // Compute the current S-meter value in dB
    m_sm_value = 10.0 * log10(m_sm_tmp_val + 1e-200); // Avoid log(0)

    // Apply S-meter correction
    double corrected_sm = m_sm_value + QsGlobal::g_memory->getSMeterCorrection();

    // Update global memory with the corrected S-meter value
    QsGlobal::g_memory->setSMeterCurrentValue(corrected_sm);

    // Store the current S-meter value as an unsigned char with offset correction
    QsGlobal::g_memory->setSMeterCurrentValueC(
        static_cast<unsigned char>(std::round(corrected_sm + QS_DEFAULT_SPEC_OFFSET)));
}
