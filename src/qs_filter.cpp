#include "../include/qs_filter.hpp"
#include "../include/qs_dataproc.hpp"
#include "../include/qs_globals.hpp"
#include <cmath>

QsFilter::QsFilter()
    : m_size(4096), m_samplerate(62500), m_filter_lo(100), m_filter_hi(3000.0), m_one_over_norm(1.0 / (m_size * 2.0)),
      p_ovlpfft(new QsFFT()), p_filtfft(new QsFFT()) {}

void QsFilter ::init(String filtertype, float samplerate, int size) {
    m_size = size;
    m_samplerate = samplerate;

    p_ovlpfft->resize(m_size * 2);
    p_filtfft->resize(m_size * 2);

    m_filter_lo = 100;
    m_filter_hi = 3000;

    m_one_over_norm = 1.0 / (m_size * 2.0);

    tmpfilt0_re.resize(size * 2);
    tmpfilt0_im.resize(size * 2);

    cpx_0.resize(size * 2);
    cpx_1.resize(size * 2);

    filt_cpx0.resize(size * 2);
    ovlp.resize(size);

    Cpx val(0.0, 0.0);
    std::fill(cpx_0.begin(), cpx_0.end(), val);
    std::fill(cpx_1.begin(), cpx_1.end(), val);
    std::fill(ovlp.begin(), ovlp.end(), val);
    std::fill(filt_cpx0.begin(), filt_cpx0.end(), val);

    if (filtertype.contains("lowpass")) {
        m_filter_type = 1;
    } else if (filtertype.contains("bandpass")) {
        m_filter_type = 2;
    } else if (filtertype.contains("bandstop")) {
        m_filter_type = 3;
    } else {
        m_filter_type = 2;
    }
    MakeFilter(m_filter_lo, m_filter_hi, m_filter_type);
}

void QsFilter ::doFilter(qs_vect_cpx &src_dst) {
    QsDataProc::Zero(&cpx_0[0] + m_size, m_size);
    QsDataProc::Copy(&src_dst[0], &cpx_0[0], m_size);

    // filter
    p_ovlpfft->doDFTForward(cpx_0, m_size * 2);
    QsDataProc::Multiply(filt_cpx0, cpx_0, cpx_1, m_size * 2);
    p_ovlpfft->doDFTInverse(cpx_1, m_size * 2, m_one_over_norm);

    // overlap add
    QsDataProc::Add(&cpx_1[0], &ovlp[0], &cpx_0[0], m_size);
    QsDataProc::Copy(&cpx_1[0] + m_size, &ovlp[0], m_size);

    QsDataProc::Copy(&cpx_0[0], &src_dst[0], m_size);
}

void QsFilter ::doFilter(qs_vect_cpx &src, qs_vect_cpx &dst) {
    QsDataProc::Zero(&cpx_0[0] + m_size, m_size);
    QsDataProc::Copy(&src[0], &cpx_0[0], m_size);

    // filter
    p_ovlpfft->doDFTForward(cpx_0, m_size * 2);
    QsDataProc::Multiply(filt_cpx0, cpx_0, cpx_1, m_size * 2);
    p_ovlpfft->doDFTInverse(cpx_1, m_size * 2, m_one_over_norm);

    // overlap add
    QsDataProc::Add(&cpx_1[0], &ovlp[0], &cpx_0[0], m_size);
    QsDataProc::Copy(&cpx_1[0] + m_size, &ovlp[0], m_size);

    QsDataProc::Copy(&cpx_0[0], &dst[0], m_size);
}

void QsFilter ::doFilter(qs_vect_f &src_dst_re, qs_vect_f &src_dst_im) {
    QsDataProc::Zero(&cpx_0[0] + m_size, m_size);
    QsDataProc::Copy(&src_dst_re[0], &src_dst_im[0], &cpx_0[0], m_size);

    // filter
    p_ovlpfft->doDFTForward(cpx_0, m_size * 2);
    QsDataProc::Multiply(filt_cpx0, cpx_0, cpx_1, m_size * 2);
    p_ovlpfft->doDFTInverse(cpx_1, m_size * 2, m_one_over_norm);

    // overlap add
    QsDataProc::Add(&cpx_1[0], &ovlp[0], &cpx_0[0], m_size);
    QsDataProc::Copy(&cpx_1[0] + m_size, &ovlp[0], m_size);

    QsDataProc::Copy(&cpx_0[0], &src_dst_re[0], &src_dst_im[0], m_size);
}

void QsFilter ::doFilter(qs_vect_f &src, qs_vect_cpx &dst) {
    QsDataProc::Zero(&cpx_0[0] + m_size, m_size);
    QsDataProc::RealToComplex(&src[0], &src[0], &cpx_0[0], m_size);

    // filter
    p_ovlpfft->doDFTForward(cpx_0, m_size * 2);
    QsDataProc::Multiply(filt_cpx0, cpx_0, cpx_1, m_size * 2);
    p_ovlpfft->doDFTInverse(cpx_1, m_size * 2, m_one_over_norm);

    // overlap add
    QsDataProc::Add(&cpx_1[0], &ovlp[0], &cpx_0[0], m_size);
    QsDataProc::Copy(&cpx_1[0] + m_size, &ovlp[0], m_size);

    QsDataProc::Copy(&cpx_0[0], &dst[0], m_size);
}

void QsFilter ::doFilter(qs_vect_f &src, qs_vect_f &dst_re, qs_vect_f &dst_im) {
    QsDataProc::Zero(&cpx_0[0] + m_size, m_size);
    QsDataProc::RealToComplex(&src[0], &src[0], &cpx_0[0], m_size);

    // filter
    p_ovlpfft->doDFTForward(cpx_0, m_size * 2);
    QsDataProc::Multiply(filt_cpx0, cpx_0, cpx_1, m_size * 2);
    p_ovlpfft->doDFTInverse(cpx_1, m_size * 2, m_one_over_norm);

    // overlap add
    QsDataProc::Add(&cpx_1[0], &ovlp[0], &cpx_0[0], m_size);
    QsDataProc::Copy(&cpx_1[0] + m_size, &ovlp[0], m_size);

    QsDataProc::Copy(&cpx_0[0], &dst_re[0], &dst_im[0], m_size);
}

void QsFilter ::setSampleRate(double value) { m_samplerate = value; }

void QsFilter ::setFilterLo(int value) {
    m_filter_lo = value;
    MakeFilter(m_filter_lo, m_filter_hi, m_filter_type);
}

void QsFilter ::setFilterHi(int value) {
    m_filter_hi = value;
    MakeFilter(m_filter_lo, m_filter_hi, m_filter_type);
}

void QsFilter ::setFilter(int loval, int hival) {
    m_filter_lo = loval;
    m_filter_hi = hival;
    MakeFilter(m_filter_lo, m_filter_hi, m_filter_type);
}

int QsFilter ::getFilterLo() const { return m_filter_lo; }

int QsFilter ::getFilterHi() const { return m_filter_hi; }

void QsFilter ::MakeFilter(float lo, float hi, int ftype) {
    QsDataProc::Zero(&filt_cpx0[0], m_size * 2);

    switch (ftype) {
    case 1: // lowpass
        MakeFirLowpass(hi, m_samplerate, 12, tmpfilt0_re, tmpfilt0_im, m_size);
        break;
    case 2: // bandpass
        MakeFirBandpass(lo, hi, m_samplerate, 12, tmpfilt0_re, tmpfilt0_im, m_size);
        break;
    case 3: // bandstop
        MakeFirBandstop(lo, hi, m_samplerate, 12, tmpfilt0_re, tmpfilt0_im, m_size);
        break;

    default:
        MakeFirBandpass(lo, hi, m_samplerate, 12, tmpfilt0_re, tmpfilt0_im, m_size);
        break;
    }

    QsDataProc::RealToComplex(&tmpfilt0_re[0], &tmpfilt0_im[0], &filt_cpx0[0], m_size);
    p_filtfft->doDFTForward(filt_cpx0, m_size * 2);
}

void QsFilter ::MakeFirLowpass(float cutoff, float samplerate, int wtype, qs_vect_f &taps_re, qs_vect_f &taps_im,
                               int length) {

    qs_vect_f window;
    window.resize(length);

    float fc = cutoff / samplerate;

    if (fc > 0.5) {
        return;
    }

    int midpoint = length >> 1;

    MakeWindow(wtype, length, window);

    for (int i = 1; i <= length; i++) {
        int j = i - 1;
        if (i != midpoint) {
            taps_re[j] = (sin(TWO_PI * (i - midpoint) * fc) / (ONE_PI * (i - midpoint))) * window[j];
            taps_im[j] = (cos(TWO_PI * (i - midpoint) * fc) / (ONE_PI * (i - midpoint))) * window[j];
        } else {
            taps_re[midpoint - 1] = 2.0 * fc;
            taps_im[midpoint - 1] = 2.0 * fc;
        }
    }
}

void QsFilter ::MakeFirBandpass(float lo, float hi, float samplerate, int wtype, qs_vect_f &taps_re, qs_vect_f &taps_im,
                                int length) {
    qs_vect_f window;
    window.resize(length);

    float fl = lo / samplerate;
    float fh = hi / samplerate;
    float fc = (fh - fl) / 2.0;
    float ff = (fl + fh) * ONE_PI;

    int midpoint = length >> 1;

    MakeWindow(wtype, length, window);

    for (int i = 1; i <= length; i++) {
        int j = i - 1;
        int k = i - midpoint;
        float temp = 0.0;
        float phase = k * ff * -1;
        if (i != midpoint)
            temp = ((sin(TWO_PI * k * fc) / (ONE_PI * k))) * window[j];
        else
            temp = 2.0 * fc;
        temp *= 2.0;
        taps_re[j] = temp * (cos(phase));
        taps_im[j] = temp * (sin(phase));
    }
}

void QsFilter ::MakeFirBandstop(float lo, float hi, float samplerate, int wtype, qs_vect_f &taps_re, qs_vect_f &taps_im,
                                int length) {
    qs_vect_f window;
    window.resize(length);

    float fh = lo / samplerate; // swap
    float fl = hi / samplerate;
    float fc = (fh - fl) / 2.0;
    float fswap = (fl - fh);
    float ff = (fl + fh) * ONE_PI;

    int midpoint = length >> 1;

    MakeWindow(wtype, length, window);

    for (int i = 1; i <= length; i++) {
        int j = i - 1;
        int k = i - midpoint;
        float temp = 0.0;
        float phase = k * ff * -1;
        if (i != midpoint)
            temp = ((sin(TWO_PI * k * fc) / (ONE_PI * k))) * window[j];
        else
            temp = 1.0 - fswap;
        temp *= 2.0;
        taps_re[j] = temp * (cos(phase));
        taps_im[j] = temp * (sin(phase));
    }
}

void QsFilter::MakeWindow(int wtype, int size, qs_vect_cpx &window) {
    qs_vect_f fwindow;
    fwindow.resize(size);
    MakeWindow(wtype, size, fwindow);

    for (int i = 0; i < size; i++) {
        window[i] = std::complex<float>(fwindow[i], fwindow[i]); // Set both real and imaginary parts
    }
}

qs_vect_cpx QsFilter::MakeWindowComplex(int wtype, int size) {
    qs_vect_cpx window(size);
    qs_vect_f fwindow = MakeWindow(wtype, size);

    for (int i = 0; i < size; i++) {
        window[i] = std::complex<float>(fwindow[i], fwindow[i]); // Set both real and imaginary parts
    }
    return window;
}

void QsFilter ::MakeWindow(int wtype, int size, qs_vect_f &window) {
    int i, j, midn, midp1, midm1;
    float freq, rate, sr1, angle, expn, expsum, cx, two_pi;

    midn = size / 2;
    midp1 = (size + 1) / 2;
    midm1 = (size - 1) / 2;
    two_pi = 8.0 * atan(1.0);
    freq = two_pi / size;
    rate = 1.0 / midn;
    angle = 0.0;
    expn = log(2.0) / midn + 1.0;
    expsum = 1.0;

    switch (wtype) {
    case 1: // RECTANGULAR_WINDOW
        for (i = 0; i < size; i++)
            window[i] = 1.0;
        break;
    case 2: // HANNING_WINDOW
        for (i = 0, j = size - 1, angle = 0.0; i <= midn; i++, j--, angle += freq)
            window[j] = (window[i] = 0.5 - 0.5 * cos(angle));
        break;
    case 3: // WELCH_WINDOW
        for (i = 0, j = size - 1; i <= midn; i++, j--)
            window[j] = (window[i] = 1.0 - sqrt((float)((i - midm1) / midp1)));
        break;
    case 4: // PARZEN_WINDOW
        for (i = 0, j = size - 1; i <= midn; i++, j--)
            window[j] = (window[i] = 1.0 - (fabs((float)(i - midm1)) / midp1));
        break;
    case 5: // BARTLETT_WINDOW
        for (i = 0, j = size - 1, angle = 0.0; i <= midn; i++, j--, angle += rate)
            window[j] = (window[i] = angle);
        break;
    case 6: // HAMMING_WINDOW
        for (i = 0, j = size - 1, angle = 0.0; i <= midn; i++, j--, angle += freq)
            window[j] = (window[i] = 0.5F - 0.46 * cos(angle));
        break;
    case 7: // BLACKMAN2_WINDOW
        for (i = 0, j = size - 1, angle = 0.0; i <= midn; i++, j--, angle += freq) {
            cx = cos(angle);
            window[j] = (window[i] = (.34401 + (cx * (-.49755 + (cx * .15844)))));
        }
        break;
    case 8: // BLACKMAN3_WINDOW
        for (i = 0, j = size - 1, angle = 0.0; i <= midn; i++, j--, angle += freq) {
            cx = cos(angle);
            window[j] = (window[i] = (.21747 + (cx * (-.45325 + (cx * (.28256 - (cx * .04672)))))));
        }
        break;
    case 9: // BLACKMAN4_WINDOW
        for (i = 0, j = size - 1, angle = 0.0; i <= midn; i++, j--, angle += freq) {
            cx = cos(angle);
            window[j] =
                (window[i] = (.084037 + (cx * (-.29145 + (cx * (.375696 + (cx * (-.20762 + (cx * .041194)))))))));
        }
        break;
    case 10: // EXPONENTIAL_WINDOW
        for (i = 0, j = size - 1; i <= midn; i++, j--) {
            window[j] = (window[i] = expsum - 1.0);
            expsum *= expn;
        }
        break;
    case 11: // RIEMANN_WINDOW
        sr1 = two_pi / size;
        for (i = 0, j = size - 1; i <= midn; i++, j--) {
            if (i == midn)
                window[j] = (window[i] = 1.0);
            else {
                cx = sr1 * (midn - i);
                window[i] = sin(cx) / cx;
                window[j] = window[i];
            }
        }
        break;
    case 12: // BLACKMANHARRIS_WINDOW
    {
        float a0 = 0.35875F, a1 = 0.48829F, a2 = 0.14128F, a3 = 0.01168F;

        for (i = 0; i < size; i++) {
            window[i] = a0 - a1 * cos(two_pi * (i + 0.5) / size) + a2 * cos(2.0 * two_pi * (i + 0.5) / size) -
                        a3 * cos(3.0 * two_pi * (i + 0.5) / size);
        }
    } break;
    default:
        return;
    }
}

qs_vect_f QsFilter ::MakeWindow(int wtype, int size) {
    int i, j, midn, midp1, midm1;
    float freq, rate, sr1, angle, expn, expsum, cx, two_pi;

    midn = size / 2;
    midp1 = (size + 1) / 2;
    midm1 = (size - 1) / 2;
    two_pi = 8.0 * atan(1.0);
    freq = two_pi / size;
    rate = 1.0 / midn;
    angle = 0.0;
    expn = log(2.0) / midn + 1.0;
    expsum = 1.0;

    qs_vect_f window(size);

    switch (wtype) {
    case 1: // RECTANGULAR_WINDOW
        for (i = 0; i < size; i++)
            window[i] = 1.0;
        break;
    case 2: // HANNING_WINDOW
        for (i = 0, j = size - 1, angle = 0.0; i <= midn; i++, j--, angle += freq)
            window[j] = (window[i] = 0.5 - 0.5 * cos(angle));
        break;
    case 3: // WELCH_WINDOW
        for (i = 0, j = size - 1; i <= midn; i++, j--)
            window[j] = (window[i] = 1.0 - sqrt((float)((i - midm1) / midp1)));
        break;
    case 4: // PARZEN_WINDOW
        for (i = 0, j = size - 1; i <= midn; i++, j--)
            window[j] = (window[i] = 1.0 - (fabs((float)(i - midm1)) / midp1));
        break;
    case 5: // BARTLETT_WINDOW
        for (i = 0, j = size - 1, angle = 0.0; i <= midn; i++, j--, angle += rate)
            window[j] = (window[i] = angle);
        break;
    case 6: // HAMMING_WINDOW
        for (i = 0, j = size - 1, angle = 0.0; i <= midn; i++, j--, angle += freq)
            window[j] = (window[i] = 0.5F - 0.46 * cos(angle));
        break;
    case 7: // BLACKMAN2_WINDOW
        for (i = 0, j = size - 1, angle = 0.0; i <= midn; i++, j--, angle += freq) {
            cx = cos(angle);
            window[j] = (window[i] = (.34401 + (cx * (-.49755 + (cx * .15844)))));
        }
        break;
    case 8: // BLACKMAN3_WINDOW
        for (i = 0, j = size - 1, angle = 0.0; i <= midn; i++, j--, angle += freq) {
            cx = cos(angle);
            window[j] = (window[i] = (.21747 + (cx * (-.45325 + (cx * (.28256 - (cx * .04672)))))));
        }
        break;
    case 9: // BLACKMAN4_WINDOW
        for (i = 0, j = size - 1, angle = 0.0; i <= midn; i++, j--, angle += freq) {
            cx = cos(angle);
            window[j] =
                (window[i] = (.084037 + (cx * (-.29145 + (cx * (.375696 + (cx * (-.20762 + (cx * .041194)))))))));
        }
        break;
    case 10: // EXPONENTIAL_WINDOW
        for (i = 0, j = size - 1; i <= midn; i++, j--) {
            window[j] = (window[i] = expsum - 1.0);
            expsum *= expn;
        }
        break;
    case 11: // RIEMANN_WINDOW
        sr1 = two_pi / size;
        for (i = 0, j = size - 1; i <= midn; i++, j--) {
            if (i == midn)
                window[j] = (window[i] = 1.0);
            else {
                cx = sr1 * (midn - i);
                window[i] = sin(cx) / cx;
                window[j] = window[i];
            }
        }
        break;
    case 12: // BLACKMANHARRIS_WINDOW
    {
        float a0 = 0.35875F, a1 = 0.48829F, a2 = 0.14128F, a3 = 0.01168F;

        for (i = 0; i < size; i++) {
            window[i] = a0 - a1 * cos(two_pi * (i + 0.5) / size) + a2 * cos(2.0 * two_pi * (i + 0.5) / size) -
                        a3 * cos(3.0 * two_pi * (i + 0.5) / size);
        }
    } break;
    default:
        break;
    }
    return window;
}
