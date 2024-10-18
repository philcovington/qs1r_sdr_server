/**
 * @file QsDownConvertor.h
 * @brief Downconversion and decimation class for complex signal processing.
 *
 * This file defines the QsDownConvertor class, which performs downconversion
 * of complex signals using multiple decimation stages. It provides different
 * decimation techniques, such as Half-Band and CIC filters, to reduce the
 * sampling rate while preserving signal integrity.
 *
 * Features:
 * - Supports up to 10 stages of decimation
 * - Includes Half-Band filters and CIC filters for efficient downconversion
 * - Custom decimation filters with varying coefficients
 * - Multi-stage processing for optimized bandwidth and rate control
 * - Thread safety through mutex locks for concurrent access
 *
 * Usage:
 * 1. Create an instance of QsDownConvertor.
 * 2. Use `setRate(double in_rate, double bandwidth)` to set the input rate and bandwidth.
 * 3. Process a signal by calling `process(Cpx *in_cpx, Cpx *out_cpx, int length)`.
 * 4. The class automatically selects the appropriate decimation filters based on the rate and bandwidth.
 *
 * Notes:
 * - Decimation filters like Half-Band and CIC are nested classes within QsDownConvertor.
 * - Memory management is handled within the decimation classes, with buffers being allocated and deleted as needed.
 * - Thread synchronization is provided by QMutex to ensure safe multithreading.
 *
 * Author: Philip A Covington
 * Date: 2024-10-17
 */

#pragma once

#include "../include/qs_signalops.hpp"
#include "../include/qs_defines.hpp"
#include "../include/qs_downcnv_coeff.hpp"
#include "../include/qs_globals.hpp"
#include <mutex>

#define MAX_STAGES 10

class QsDownConvertor {
  public:
    QsDownConvertor();
    ~QsDownConvertor();
    int process(Cpx *in_cpx, Cpx *out_cpx, int length);
    double setRate(double in_rate, double bandwidth);

  private:
    class DecimateBy2 {
      public:
        DecimateBy2() {};
        virtual ~DecimateBy2() {};
        virtual int Decimate(Cpx *in_cpx, Cpx *out_cpx, int length) = 0;
    };

    class HalfBandDecimateBy2 : public DecimateBy2 {
      public:
        HalfBandDecimateBy2(int length, const double *pCoef);
        ~HalfBandDecimateBy2() {
            if (m_pHBFirBuf)
                delete m_pHBFirBuf;
        }
        int Decimate(Cpx *in_cpx, Cpx *out_cpx, int length);

      private:
        Cpx *m_pHBFirBuf;
        int m_FirLength;
        const double *m_pCoef;
    };

    class HalfBand11TapDecimateBy2 : public DecimateBy2 {
      public:
        HalfBand11TapDecimateBy2();
        ~HalfBand11TapDecimateBy2() {}
        int Decimate(Cpx *in_cpx, Cpx *out_cpx, int length);
        double H0; // unwrapped coeeficients
        double H2;
        double H4;
        double H5;
        double H6;
        double H8;
        double H10;
        Cpx d0; // unwrapped delay buffer
        Cpx d1;
        Cpx d2;
        Cpx d3;
        Cpx d4;
        Cpx d5;
        Cpx d6;
        Cpx d7;
        Cpx d8;
        Cpx d9;
    };

    class CIC3DecimateBy2 : public DecimateBy2 {
      public:
        CIC3DecimateBy2();
        ~CIC3DecimateBy2() {}
        int Decimate(Cpx *in_cpx, Cpx *out_cpx, int length);

      private:
        Cpx m_Xodd;
        Cpx m_Xeven;
    };

    void deleteFilters();

    double m_OutputRate;
    double m_InRate;
    double m_MaxBW;
    std::mutex m_Mutex;
    DecimateBy2 *m_pDecimators[MAX_STAGES];
};
