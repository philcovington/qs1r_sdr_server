#include "../include/qs_iir_filter.hpp"
#include "../include/qs_globals.hpp"

double asinh(double value) {
    double returned;

    if (value > 0)
        returned = log(value + sqrt(value * value + 1));
    else
        returned = -log(-value + sqrt(value * value + 1));

    return (returned);
}

QS_IIR ::QS_IIR() {
    m_notch_num = 0;
    m_type = iirLowPass;
    m_f0Freq = 2000.0;
    m_bwHz = 100.0;

    switch (m_type) {
    case iirLowPass:
        initLowPass(m_f0Freq, m_bwHz, QsGlobal::g_memory->getDataPostProcRate());
        break;
    case iirHighPass:
        initHighPass(m_f0Freq, m_bwHz, QsGlobal::g_memory->getDataPostProcRate());
        break;
    case iirBandPass:
        initBandPass(m_f0Freq, m_bwHz, QsGlobal::g_memory->getDataPostProcRate());
        break;
    case iirBandReject:
        initBandReject(m_f0Freq, m_bwHz, QsGlobal::g_memory->getDataPostProcRate());
        break;
    case iirTxDcBlock:
        m_f0Freq = QS_DEFAULT_TX_DCBLOCK_F0;
        m_bwHz = QS_DEFAULT_TX_DCBLOCK_BW;
        initHighPass(m_f0Freq, m_bwHz, QS_DEFAULT_RT_RATE);
        break;
    }
}

void QS_IIR ::init(unsigned int notch_num, QSIIRTYPE type) {
    m_notch_num = notch_num;
    m_type = type;

    if (m_type != iirTxDcBlock) {
        m_f0Freq = QsGlobal::g_memory->getNotchFrequency(m_notch_num);
        m_bwHz = QsGlobal::g_memory->getNotchBandwidth(m_notch_num);
    }

    switch (m_type) {
    case iirLowPass:
        initLowPass(m_f0Freq, m_bwHz, QsGlobal::g_memory->getDataPostProcRate());
        break;
    case iirHighPass:
        initHighPass(m_f0Freq, m_bwHz, QsGlobal::g_memory->getDataPostProcRate());
        break;
    case iirBandPass:
        initBandPass(m_f0Freq, m_bwHz, QsGlobal::g_memory->getDataPostProcRate());
        break;
    case iirBandReject:
        initBandReject(m_f0Freq, m_bwHz, QsGlobal::g_memory->getDataPostProcRate());
        break;
    case iirTxDcBlock:
        m_f0Freq = QS_DEFAULT_TX_DCBLOCK_F0;
        m_bwHz = QS_DEFAULT_TX_DCBLOCK_BW;
        initHighPass(m_f0Freq, m_bwHz, QS_DEFAULT_RT_RATE);
        break;
    }
}

void QS_IIR ::initLowPass(float f0freq, float bw_hz, float rate) {
    m_f0Freq = f0freq;
    m_bwHz = bw_hz;

    float w0 = 6.283185307179586476925286766559 * f0freq / rate;

    m_Q = f0freq / bw_hz;

    float alpha = sin(w0) / (2.0 * m_Q);
    float A = 1.0 / (1.0 + alpha);

    m_B0 = A * ((1.0 - cos(w0)) / 2.0);
    m_B1 = A * (1.0 - cos(w0));
    m_B2 = A * ((1.0 - cos(w0)) / 2.0);
    m_A1 = A * (-2.0 * cos(w0));
    m_A2 = A * (1.0 - alpha);

    m_w1a = 0.0;
    m_w2a = 0.0;
    m_w1b = 0.0;
    m_w2b = 0.0;
}

void QS_IIR ::initHighPass(float f0freq, float bw_hz, float rate) {
    m_f0Freq = f0freq;
    m_bwHz = bw_hz;

    float w0 = 6.283185307179586476925286766559 * f0freq / rate;

    m_Q = f0freq / bw_hz;

    float alpha = sin(w0) / (2.0 * m_Q);
    float A = 1.0 / (1.0 + alpha);

    m_B0 = A * ((1.0 + cos(w0)) / 2.0);
    m_B1 = -A * (1.0 + cos(w0));
    m_B2 = A * ((1.0 + cos(w0)) / 2.0);
    m_A1 = A * (-2.0 * cos(w0));
    m_A2 = A * (1.0 - alpha);

    m_w1a = 0.0;
    m_w2a = 0.0;
    m_w1b = 0.0;
    m_w2b = 0.0;
}

void QS_IIR ::initBandPass(float f0freq, float bw_hz, float rate) {
    m_f0Freq = f0freq;
    m_bwHz = bw_hz;

    float w0 = 6.283185307179586476925286766559 * f0freq / rate;

    m_Q = f0freq / bw_hz;

    float alpha = sin(w0) / (2.0 * m_Q);
    float A = 1.0 / (1.0 + alpha);

    m_B0 = A * alpha;
    m_B1 = 0.0;
    m_B2 = A * -alpha;
    m_A1 = A * (-2.0 * cos(w0));
    m_A2 = A * (1.0 - alpha);

    m_w1a = 0.0;
    m_w2a = 0.0;
    m_w1b = 0.0;
    m_w2b = 0.0;
}

void QS_IIR ::initBandReject(float f0freq, float bw_hz, float rate) {
    m_f0Freq = f0freq;
    m_bwHz = bw_hz;

    float w0 = 6.283185307179586476925286766559 * f0freq / rate;

    m_Q = f0freq / bw_hz;

    float alpha = sin(w0) / (2.0 * m_Q);
    float A = 1.0 / (1.0 + alpha);

    m_B0 = A * 1.0;
    m_B1 = A * (-2.0 * cos(w0));
    m_B2 = A * 1.0;
    m_A1 = A * (-2.0 * cos(w0));
    m_A2 = A * (1.0 - alpha);

    m_w1a = 0.0;
    m_w2a = 0.0;
    m_w1b = 0.0;
    m_w2b = 0.0;
}

void QS_IIR ::process(qs_vect_f &src_dst) {
    if (!QsGlobal::g_memory->getNotchEnabled(m_notch_num))
        return;

    if (m_type != iirTxDcBlock) {
        if (m_f0Freq != QsGlobal::g_memory->getNotchFrequency(m_notch_num) ||
            m_bwHz != QsGlobal::g_memory->getNotchBandwidth(m_notch_num)) {
            m_f0Freq = QsGlobal::g_memory->getNotchFrequency(m_notch_num);
            m_bwHz = QsGlobal::g_memory->getNotchBandwidth(m_notch_num);
            switch (m_type) {
            case iirLowPass:
                initLowPass(m_f0Freq, m_bwHz, QsGlobal::g_memory->getDataPostProcRate());
                break;
            case iirHighPass:
                initHighPass(m_f0Freq, m_bwHz, QsGlobal::g_memory->getDataPostProcRate());
                break;
            case iirBandPass:
                initBandPass(m_f0Freq, m_bwHz, QsGlobal::g_memory->getDataPostProcRate());
                break;
            case iirBandReject:
                initBandReject(m_f0Freq, m_bwHz, QsGlobal::g_memory->getDataPostProcRate());
                break;
            }
        }
    }

    for (f_itr = src_dst.begin(); f_itr != src_dst.end(); f_itr++) {
        float w0 = (*f_itr) - m_A1 * m_w1a - m_A2 * m_w2a;
        (*f_itr) = m_B0 * w0 + m_B1 * m_w1a + m_B2 * m_w2a;
        m_w2a = m_w1a;
        m_w1a = w0;
    }
}

void QS_IIR ::process(qs_vect_cpx &src_dst) {
    if (!QsGlobal::g_memory->getNotchEnabled(m_notch_num))
        return;

    if (m_type != iirTxDcBlock) {
        if (m_f0Freq != QsGlobal::g_memory->getNotchFrequency(m_notch_num) ||
            m_bwHz != QsGlobal::g_memory->getNotchBandwidth(m_notch_num)) {
            m_f0Freq = QsGlobal::g_memory->getNotchFrequency(m_notch_num);
            m_bwHz = QsGlobal::g_memory->getNotchBandwidth(m_notch_num);
            switch (m_type) {
            case iirLowPass:
                initLowPass(m_f0Freq, m_bwHz, QsGlobal::g_memory->getDataPostProcRate());
                break;
            case iirHighPass:
                initHighPass(m_f0Freq, m_bwHz, QsGlobal::g_memory->getDataPostProcRate());
                break;
            case iirBandPass:
                initBandPass(m_f0Freq, m_bwHz, QsGlobal::g_memory->getDataPostProcRate());
                break;
            case iirBandReject:
                initBandReject(m_f0Freq, m_bwHz, QsGlobal::g_memory->getDataPostProcRate());
                break;
            }
        }
    }

    for (cpx_itr = src_dst.begin(); cpx_itr != src_dst.end(); ++cpx_itr) {
        // Process real part
        float w0a = cpx_itr->real() - m_A1 * m_w1a - m_A2 * m_w2a;
        cpx_itr->real(m_B0 * w0a + m_B1 * m_w1a + m_B2 * m_w2a);
        m_w2a = m_w1a;
        m_w1a = w0a;

        // Process imaginary part
        float w0b = cpx_itr->imag() - m_A1 * m_w1b - m_A2 * m_w2b;
        cpx_itr->imag(m_B0 * w0b + m_B1 * m_w1b + m_B2 * m_w2b);
        m_w2b = m_w1b;
        m_w1b = w0b;
    }
}
