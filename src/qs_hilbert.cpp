#include "../include/qs_hilbert.hpp"
#include "../include/qs_filter.hpp"

QsHilbert ::QsHilbert(int ntaps) { m_taps = MakeHilbertTaps(12, ntaps); }

int QsHilbert::process(qs_vect_f &in_f, qs_vect_cpx &out_cpx, unsigned int length) {
    for (unsigned int i = 0; i < length; i++) {
        out_cpx[i].real(in_f[i + m_taps.size() / 2]); // Set the real part
        out_cpx[i].imag(filter(&in_f[i]));            // Set the imaginary part using the filter result
    }
    return length;
}

int QsHilbert ::process(qs_vect_cpx &in_cpx, qs_vect_cpx &out_cpx, unsigned int length) {
    qs_vect_f in_f(length);
    QsDataProc::RealFromComplex(in_cpx.data(), in_f.data(), length);
    return process(in_f, out_cpx, length);
}

float QsHilbert ::filter(const float input[]) {
    //    static const int n_para = 4;
    //    unsigned int i = 0;
    //    unsigned int n = ( m_taps.size() / n_para ) * n_para;

    //    float accumulator_0 = 0;
    //    float accumulator_1 = 0;
    //    float accumulator_2 = 0;
    //    float accumulator_3 = 0;

    //    for ( int i = 0; i < n; i += n_para )
    //    {
    //        accumulator_0 += m_taps[i+0] * (float)input[i+0];
    //        accumulator_1 += m_taps[i+1] * (float)input[i+1];
    //        accumulator_2 += m_taps[i+2] * (float)input[i+2];
    //        accumulator_3 += m_taps[i+3] * (float)input[i+3];
    //    }

    //    for ( ; i < m_taps.size(); i++ )
    //        accumulator_0 += m_taps[i] * (float)input[i];

    //    return (float)( accumulator_0 + accumulator_1 + accumulator_2 + accumulator_3 );
    float accumulator_0 = 0;
    for (unsigned int i = 0; i < m_taps.size(); i++) {
        accumulator_0 += m_taps[i] * (float)input[i];
    }
    return accumulator_0;
}

qs_vect_f QsHilbert ::MakeHilbertTaps(int wtype, unsigned int length) {
    if (!(length & 1))
        length -= 1;

    qs_vect_f taps(length);
    qs_vect_f w = QsFilter::MakeWindow(wtype, length);

    unsigned int h = (length - 1) / 2;
    float gain = 0.0;

    for (unsigned int i = 0; i <= h; i++) {
        if (i & 1) {
            float x = 1 / (float)i;
            taps[h + i] = x * w[h + i];
            taps[h - i] = -x * w[h - i];
            gain = taps[h + i] - gain;
        } else {
            taps[h + i] = taps[h - i] = 0.0;
        }
    }
    gain = 2 * fabs(gain);
    for (unsigned int i = 0; i < length; i++) {
        taps[i] /= gain;
    }
    return taps;
}
