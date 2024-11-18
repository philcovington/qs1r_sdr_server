#include "../include/qs_fir_filter.hpp"
#include <algorithm>

QsFirFilter::QsFirFilter()
    : m_size(2048), m_samplerate(50000), m_filter_lo(100), m_filter_hi(3000.0), m_one_over_norm(1.0 / (m_size * 2.0)),
      p_ovlpfft(new QsFFT()), p_filtfft(new QsFFT()) {}

void QsFirFilter::init(double lo_freq, double hi_freq, int rate, int size) {
    m_size = size;
    m_samplerate = rate;

    p_ovlpfft->resize(m_size * 2);
    p_filtfft->resize(m_size * 2);

    m_filter_lo = lo_freq;
    m_filter_hi = hi_freq;

    m_one_over_norm = 1.0 / (m_size * 2.0);

    tmpfilt0_re.resize(size * 2);
    tmpfilt0_im.resize(size * 2);

    cpx_0.resize(size * 2);
    cpx_1.resize(size * 2);

    filt_cpx0.resize(size * 2);
    ovlp.resize(size);

    std::fill(cpx_0.begin(), cpx_0.end(), std::complex<float>(0.0f, 0.0f));
    std::fill(cpx_1.begin(), cpx_1.end(), std::complex<float>(0.0f, 0.0f));
    std::fill(ovlp.begin(), ovlp.end(), std::complex<float>(0.0f, 0.0f));
    std::fill(filt_cpx0.begin(), filt_cpx0.end(), std::complex<float>(0.0f, 0.0f));
    std::fill(tmpfilt0_re.begin(), tmpfilt0_re.end(), 0.0f);
    std::fill(tmpfilt0_im.begin(), tmpfilt0_im.end(), 0.0f);  

    MakeFilter(m_filter_lo, m_filter_hi);
    m_is_init = true;
}

void QsFirFilter::setFilter(double lo_freq, double hi_freq) {
	m_filter_lo = lo_freq;
    m_filter_hi = hi_freq;	
	MakeFilter(m_filter_lo, m_filter_hi);
}

void QsFirFilter::setFilterLo(double lo_freq) {
    m_filter_lo = lo_freq;
    MakeFilter(m_filter_lo, m_filter_hi);
}
void QsFirFilter::setFilterHi(double hi_freq) {
    m_filter_hi = hi_freq;	
	MakeFilter(m_filter_lo, m_filter_hi);
}

double QsFirFilter::getFilterLo() { return m_filter_lo; }
double QsFirFilter::getFilterHi() { return m_filter_hi; }

void QsFirFilter::process(qs_vect_cpx &src_dst) {
    if (!m_is_init) { 
        throw std::runtime_error("QsFirFilter::process must call init() first!"); 
    }

    // Copy input to working buffer and zero-pad
    std::copy(src_dst.begin(), src_dst.begin() + m_size, cpx_0.begin());
    std::fill(cpx_0.begin() + m_size, cpx_0.end(), std::complex<float>(0.0f, 0.0f));

    // Forward FFT
    p_ovlpfft->doDFTForward(cpx_0, m_size * 2);

    // Apply filter in the frequency domain
    std::transform(cpx_0.begin(), cpx_0.begin() + m_size * 2, filt_cpx0.begin(), cpx_1.begin(), std::multiplies<std::complex<float>>());

    // Inverse FFT
    p_ovlpfft->doDFTInverse(cpx_1, m_size * 2, m_one_over_norm);

    // Overlap-add
    std::transform(cpx_1.begin(), cpx_1.begin() + m_size, ovlp.begin(), cpx_1.begin(), std::plus<std::complex<float>>());
    std::copy(cpx_1.begin() + m_size, cpx_1.begin() + m_size * 2, ovlp.begin());

    // Write filtered output to src_dst
    std::copy(cpx_1.begin(), cpx_1.begin() + m_size, src_dst.begin());
}


void QsFirFilter::MakeFilter(float lo, float hi) {
    std::fill(filt_cpx0.begin(), filt_cpx0.begin() + m_size * 2, std::complex<float>(0.0f, 0.0f));

    MakeFirBandpass(lo, hi, m_samplerate, 12, tmpfilt0_re, tmpfilt0_im, m_size);
    
	std::transform(tmpfilt0_re.begin(), tmpfilt0_re.begin() + m_size, tmpfilt0_im.begin(), filt_cpx0.begin(), [](const float& re, const float& im) { return std::complex<float>(re, im); });
    p_filtfft->doDFTForward(filt_cpx0, m_size * 2);
}

void QsFirFilter::MakeFirBandpass(float lo, float hi, float samplerate, int wtype, qs_vect_f &taps_re,
                                      qs_vect_f &taps_im, int length) {
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

void QsFirFilter::MakeWindow(int wtype, int size, qs_vect_cpx &window) {
    qs_vect_f fwindow;
    fwindow.resize(size);
    MakeWindow(wtype, size, fwindow);
    for (int i = 0; i < size; i++) {
    	window[i] = std::complex<float>(fwindow[i], fwindow[i]); // Assign both real and imag from fwindow[i]
	}
}

qs_vect_cpx QsFirFilter::MakeWindowComplex(int wtype, int size) {
    qs_vect_cpx window(size);
    qs_vect_f fwindow = MakeWindow(wtype, size);
    for (int i = 0; i < size; i++) {
    	window[i] = std::complex<float>(fwindow[i], fwindow[i]); // Assign both real and imag from fwindow[i]
	}
    return window;
}

void QsFirFilter::MakeWindow(int wtype, int size, qs_vect_f &window) {
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

qs_vect_f QsFirFilter::MakeWindow(int wtype, int size) {
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
