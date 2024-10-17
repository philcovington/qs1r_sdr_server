#include "../headers/qs_downcnv.h"
#include "../headers/debugloggerclass.h"

#define MIN_OUTPUT_RATE (7900.0 * 2.0)
#define MAX_HALF_BAND_BUFSIZE 32768

QsDownConvertor ::QsDownConvertor() {
    m_InRate = 100000.0;
    m_MaxBW = 10000.0;
    for (int i = 0; i < MAX_STAGES; i++)
        m_pDecimators[i] = NULL;
}

QsDownConvertor ::~QsDownConvertor() { deleteFilters(); }

void QsDownConvertor ::deleteFilters() {
    for (int i = 0; i < MAX_STAGES; i++) {
        if (m_pDecimators[i]) {
            delete m_pDecimators[i];
            m_pDecimators[i] = NULL;
        }
    }
}

double QsDownConvertor ::setRate(double in_rate, double bandwidth) {
    int n = 0;
    double f = in_rate;

    if ((m_InRate != in_rate) || (m_MaxBW != bandwidth)) {
        m_InRate = in_rate;
        m_MaxBW = bandwidth;

        _debug() << "Inrate=" << m_InRate << " BW=" << m_MaxBW;

        // m_Mutex.lock();
        deleteFilters();

        while ((f > (m_MaxBW / HB51TAP_MAX)) && (f > MIN_OUTPUT_RATE)) {
            if (f >= (m_MaxBW / CIC3_MAX)) // See if can use CIC order 3
                m_pDecimators[n++] = new QsDownConvertor::CIC3DecimateBy2;
            else if (f >= (m_MaxBW / HB11TAP_MAX)) // See if can use fixed 11 Tap Halfband
                m_pDecimators[n++] = new QsDownConvertor::HalfBand11TapDecimateBy2();
            else if (f >= (m_MaxBW / HB15TAP_MAX)) // See if can use Halfband 15 Tap
                m_pDecimators[n++] = new QsDownConvertor::HalfBandDecimateBy2(HB15TAP_LENGTH, HB15TAP_H);
            else if (f >= (m_MaxBW / HB19TAP_MAX)) // See if can use Halfband 19 Tap
                m_pDecimators[n++] = new QsDownConvertor::HalfBandDecimateBy2(HB19TAP_LENGTH, HB19TAP_H);
            else if (f >= (m_MaxBW / HB23TAP_MAX)) // See if can use Halfband 23 Tap
                m_pDecimators[n++] = new QsDownConvertor::HalfBandDecimateBy2(HB23TAP_LENGTH, HB23TAP_H);
            else if (f >= (m_MaxBW / HB27TAP_MAX)) // See if can use Halfband 27 Tap
                m_pDecimators[n++] = new QsDownConvertor::HalfBandDecimateBy2(HB27TAP_LENGTH, HB27TAP_H);
            else if (f >= (m_MaxBW / HB31TAP_MAX)) // See if can use Halfband 31 Tap
                m_pDecimators[n++] = new QsDownConvertor::HalfBandDecimateBy2(HB31TAP_LENGTH, HB31TAP_H);
            else if (f >= (m_MaxBW / HB35TAP_MAX)) // See if can use Halfband 35 Tap
                m_pDecimators[n++] = new QsDownConvertor::HalfBandDecimateBy2(HB35TAP_LENGTH, HB35TAP_H);
            else if (f >= (m_MaxBW / HB39TAP_MAX)) // See if can use Halfband 39 Tap
                m_pDecimators[n++] = new QsDownConvertor::HalfBandDecimateBy2(HB39TAP_LENGTH, HB39TAP_H);
            else if (f >= (m_MaxBW / HB43TAP_MAX)) // See if can use Halfband 43 Tap
                m_pDecimators[n++] = new QsDownConvertor::HalfBandDecimateBy2(HB43TAP_LENGTH, HB43TAP_H);
            else if (f >= (m_MaxBW / HB47TAP_MAX)) // See if can use Halfband 47 Tap
                m_pDecimators[n++] = new QsDownConvertor::HalfBandDecimateBy2(HB47TAP_LENGTH, HB47TAP_H);
            else if (f >= (m_MaxBW / HB51TAP_MAX)) // See if can use Halfband 51 Tap
                m_pDecimators[n++] = new QsDownConvertor::HalfBandDecimateBy2(HB51TAP_LENGTH, HB51TAP_H);
            f /= 2.0;
        }
        // m_Mutex.unlock();
        m_OutputRate = f;
    }
    return m_OutputRate;
}

int QsDownConvertor ::process(Cpx *in_cpx, Cpx *out_cpx, int length) {
    int j;

    // Perform decimation of pInData by calling decimate by 2 stages
    // until NULL pointer encountered designating end of chain
    int n = length;
    j = 0;
    // m_Mutex.lock();
    while (m_pDecimators[j]) {
        n = m_pDecimators[j++]->Decimate(in_cpx, in_cpx, n);
    }
    memcpy(out_cpx, in_cpx, sizeof(Cpx) * n);
    // m_Mutex.unlock();

    return n;
}

QsDownConvertor ::HalfBandDecimateBy2 ::HalfBandDecimateBy2(int len, const double *pCoef)
    : m_FirLength(len), m_pCoef(pCoef) {
    // create buffer for FIR implementation
    m_pHBFirBuf = new Cpx[MAX_HALF_BAND_BUFSIZE];
    Cpx CPXZERO(0.0, 0.0);
    for (int i = 0; i < MAX_HALF_BAND_BUFSIZE; i++)
        m_pHBFirBuf[i] = CPXZERO;
}

int QsDownConvertor ::HalfBandDecimateBy2 ::Decimate(Cpx *in_cpx, Cpx *out_cpx, int length) {
    int i;
    int j;
    int numoutsamples = 0;
    if (length < m_FirLength) // safety net to make sure InLength is large enough to process
        return length / 2;
    // copy input samples into buffer starting at position m_FirLength-1
    for (i = 0, j = m_FirLength - 1; i < length; i++)
        m_pHBFirBuf[j++] = in_cpx[i];
    // perform decimation FIR filter on even samples
    for (i = 0; i < length; i += 2) {
        Cpx acc;
        acc.real(m_pHBFirBuf[i].real() * m_pCoef[0]);
        acc.imag(m_pHBFirBuf[i].imag() * m_pCoef[0]);

        for (j = 2; j < m_FirLength; j += 2) { // only use even coefficients since odd are zero (except center point)
            acc.real(acc.real() + (m_pHBFirBuf[i + j].real() * m_pCoef[j]));
            acc.imag(acc.imag() + (m_pHBFirBuf[i + j].imag() * m_pCoef[j]));
        }

        // now multiply the center coefficient
        acc.real(acc.real() + (m_pHBFirBuf[i + (m_FirLength - 1) / 2].real() * m_pCoef[(m_FirLength - 1) / 2]));
        acc.imag(acc.imag() + (m_pHBFirBuf[i + (m_FirLength - 1) / 2].imag() * m_pCoef[(m_FirLength - 1) / 2]));

        out_cpx[numoutsamples++] = acc; // put output buffer
    }
    // need to copy last m_FirLength - 1 input samples in buffer to beginning of buffer
    //  for FIR wrap around management
    for (i = 0, j = length - m_FirLength + 1; i < m_FirLength - 1; i++)
        m_pHBFirBuf[i] = in_cpx[j++];
    return numoutsamples;
}

QsDownConvertor ::HalfBand11TapDecimateBy2 ::HalfBand11TapDecimateBy2() {
    // preload only the taps that are used since evey other one is zero
    // except center tap 5
    H0 = HB11TAP_H[0];
    H2 = HB11TAP_H[2];
    H4 = HB11TAP_H[4];
    H5 = HB11TAP_H[5];
    H6 = HB11TAP_H[6];
    H8 = HB11TAP_H[8];
    H10 = HB11TAP_H[10];
    Cpx CPXZERO(0.0, 0.0);
    d0 = CPXZERO;
    d1 = CPXZERO;
    d2 = CPXZERO;
    d3 = CPXZERO;
    d4 = CPXZERO;
    d5 = CPXZERO;
    d6 = CPXZERO;
    d7 = CPXZERO;
    d8 = CPXZERO;
    d9 = CPXZERO;
}

int QsDownConvertor::HalfBand11TapDecimateBy2::Decimate(Cpx *in_cpx, Cpx *out_cpx, int length) {
    // StartPerformance();
    // First calculate the beginning 10 samples using previous samples in the delay buffer
    Cpx tmpout[9]; // Use temp buffer so outbuf can be the same as inbuf

    tmpout[0] = Cpx(H0 * d0.real() + H2 * d2.real() + H4 * d4.real() + H5 * d5.real() + H6 * d6.real() +
                        H8 * d8.real() + H10 * in_cpx[0].real(),
                    H0 * d0.imag() + H2 * d2.imag() + H4 * d4.imag() + H5 * d5.imag() + H6 * d6.imag() +
                        H8 * d8.imag() + H10 * in_cpx[0].imag());

    tmpout[1] = Cpx(H0 * d2.real() + H2 * d4.real() + H4 * d6.real() + H5 * d7.real() + H6 * d8.real() +
                        H8 * in_cpx[0].real() + H10 * in_cpx[2].real(),
                    H0 * d2.imag() + H2 * d4.imag() + H4 * d6.imag() + H5 * d7.imag() + H6 * d8.imag() +
                        H8 * in_cpx[0].imag() + H10 * in_cpx[2].imag());

    tmpout[2] = Cpx(H0 * d4.real() + H2 * d6.real() + H4 * d8.real() + H5 * d9.real() + H6 * in_cpx[0].real() +
                        H8 * in_cpx[2].real() + H10 * in_cpx[4].real(),
                    H0 * d4.imag() + H2 * d6.imag() + H4 * d8.imag() + H5 * d9.imag() + H6 * in_cpx[0].imag() +
                        H8 * in_cpx[2].imag() + H10 * in_cpx[4].imag());

    tmpout[3] = Cpx(H0 * d6.real() + H2 * d8.real() + H4 * in_cpx[0].real() + H5 * in_cpx[1].real() +
                        H6 * in_cpx[2].real() + H8 * in_cpx[4].real() + H10 * in_cpx[6].real(),
                    H0 * d6.imag() + H2 * d8.imag() + H4 * in_cpx[0].imag() + H5 * in_cpx[1].imag() +
                        H6 * in_cpx[2].imag() + H8 * in_cpx[4].imag() + H10 * in_cpx[6].imag());

    tmpout[4] = Cpx(H0 * d8.real() + H2 * in_cpx[0].real() + H4 * in_cpx[2].real() + H5 * in_cpx[3].real() +
                        H6 * in_cpx[4].real() + H8 * in_cpx[6].real() + H10 * in_cpx[8].real(),
                    H0 * d8.imag() + H2 * in_cpx[0].imag() + H4 * in_cpx[2].imag() + H5 * in_cpx[3].imag() +
                        H6 * in_cpx[4].imag() + H8 * in_cpx[6].imag() + H10 * in_cpx[8].imag());

    tmpout[5] = Cpx(H0 * in_cpx[0].real() + H2 * in_cpx[2].real() + H4 * in_cpx[4].real() + H5 * in_cpx[5].real() +
                        H6 * in_cpx[6].real() + H8 * in_cpx[8].real() + H10 * in_cpx[10].real(),
                    H0 * in_cpx[0].imag() + H2 * in_cpx[2].imag() + H4 * in_cpx[4].imag() + H5 * in_cpx[5].imag() +
                        H6 * in_cpx[6].imag() + H8 * in_cpx[8].imag() + H10 * in_cpx[10].imag());

    tmpout[6] = Cpx(H0 * in_cpx[2].real() + H2 * in_cpx[4].real() + H4 * in_cpx[6].real() + H5 * in_cpx[7].real() +
                        H6 * in_cpx[8].real() + H8 * in_cpx[10].real() + H10 * in_cpx[12].real(),
                    H0 * in_cpx[2].imag() + H2 * in_cpx[4].imag() + H4 * in_cpx[6].imag() + H5 * in_cpx[7].imag() +
                        H6 * in_cpx[8].imag() + H8 * in_cpx[10].imag() + H10 * in_cpx[12].imag());

    tmpout[7] = Cpx(H0 * in_cpx[4].real() + H2 * in_cpx[6].real() + H4 * in_cpx[8].real() + H5 * in_cpx[9].real() +
                        H6 * in_cpx[10].real() + H8 * in_cpx[12].real() + H10 * in_cpx[14].real(),
                    H0 * in_cpx[4].imag() + H2 * in_cpx[6].imag() + H4 * in_cpx[8].imag() + H5 * in_cpx[9].imag() +
                        H6 * in_cpx[10].imag() + H8 * in_cpx[12].imag() + H10 * in_cpx[14].imag());

    tmpout[8] = Cpx(H0 * in_cpx[6].real() + H2 * in_cpx[8].real() + H4 * in_cpx[10].real() + H5 * in_cpx[11].real() +
                        H6 * in_cpx[12].real() + H8 * in_cpx[14].real() + H10 * in_cpx[16].real(),
                    H0 * in_cpx[6].imag() + H2 * in_cpx[8].imag() + H4 * in_cpx[10].imag() + H5 * in_cpx[11].imag() +
                        H6 * in_cpx[12].imag() + H8 * in_cpx[14].imag() + H10 * in_cpx[16].imag());

    // Now loop through remaining input samples
    Cpx *pIn = &in_cpx[8];
    Cpx *pOut = &out_cpx[9];
    for (int i = 0; i < (length - 11 - 6) / 2; i++) {
        *pOut = Cpx(H0 * pIn[0].real() + H2 * pIn[2].real() + H4 * pIn[4].real() + H5 * pIn[5].real() +
                        H6 * pIn[6].real() + H8 * pIn[8].real() + H10 * pIn[10].real(),
                    H0 * pIn[0].imag() + H2 * pIn[2].imag() + H4 * pIn[4].imag() + H5 * pIn[5].imag() +
                        H6 * pIn[6].imag() + H8 * pIn[8].imag() + H10 * pIn[10].imag());
        pOut++;
        pIn += 2; // Skip every second sample for decimation
    }

    // EndPerformance();
    return length; // Adjust this return based on your needs
}

QsDownConvertor::CIC3DecimateBy2::CIC3DecimateBy2() {
    m_Xodd = Cpx(0.0, 0.0);
    m_Xeven = Cpx(0.0, 0.0);
}

int QsDownConvertor::CIC3DecimateBy2::Decimate(Cpx *in_cpx, Cpx *out_cpx, int length) {
    int i, j;
    Cpx even, odd;

    for (i = 0, j = 0; i < length; i += 2, j++) { // mag gn=8
        even = in_cpx[i];
        odd = in_cpx[i + 1];

        out_cpx[j] = Cpx(.125 * (std::real(odd) + std::real(m_Xeven) + 3.0 * (std::real(m_Xodd) + std::real(even))),
                         .125 * (std::imag(odd) + std::imag(m_Xeven) + 3.0 * (std::imag(m_Xodd) + std::imag(even))));

        m_Xodd = odd;
        m_Xeven = even;
    }

    return j;
}
