/*!
 * @file qs_data_proc.h
 * @brief Provides a set of utility functions for audio signal processing.
 *
 * The QsDataProc class offers various static methods for performing mathematical 
 * operations on different data types, including floating point numbers, integers, 
 * and complex numbers. These functions are optimized for real-time signal processing 
 * tasks, such as adding values to arrays, rounding, and bounding values.
 *
 * Features:
 * - Max, Min, and Bound functions for clamping values between limits.
 * - Overloaded Add functions for adding constants or arrays to different data types.
 * - Support for both real and complex number operations.
 * - Functions for rounding, absolute value computation, and type conversions.
 * 
 * Usage:
 * - Use the `Add()` methods to apply values to signal arrays.
 * - `qBound()` and `Bound()` can be used to constrain values within specified limits.
 * - `Round()` provides an efficient method for rounding float values.
 *
 * Notes:
 * - The constants `INTTOFLOAT`, `FLOATTOINT`, and others are provided for handling 
 *   type conversions between integers and floating point numbers.
 * - The `qs_vect_f` and `qs_vect_cpx` types are used for arrays of float and complex values.
 * 
 * Author: Philip A Covington
 * Date: 2024-10-16
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstring>
#include <iterator>
#include <memory>
#include <vector>

#include "../headers/qs_types.h"
#include "inttypes.h"

static const Cpx cpx_zero(0.0, 0.0);
static const Cpx cpx_one(1.0, 1.0);

#define INTTOFLOAT 4.656612873077392578125e-10
#define FLOATTOINT 2147483648.0
#define FLOATTOSHORT 32767.0
#define SHORTTOFLOAT 3.0517578125e-5
#define INT24TOFLOAT 1.1920928955078125e-7

class QsDataProc {
  public:
    template <typename T> inline static const T &Max(const T &a, const T &b) {
        if (a < b)
            return b;
        return a;
    }

    template <typename T> inline static const T &Min(const T &a, const T &b) {
        if (a < b)
            return a;
        return b;
    }

    template <typename T> inline static const T qBound(const T &min, const T &val, const T &max) {
        return (val < min) ? min : (val > max) ? max : val;
    }

    template <typename T> inline static const T &Bound(const T &min, const T &val, const T &max) {
        return Max(min, Min(max, val));
    }

    template <typename T> inline static T Abs(const T &t) { return t >= 0 ? t : -t; }

    inline static int Round(float f) { return f >= 0.0 ? int(f + 0.5) : int(f - int(f - 1) + 0.5) + int(f - 1); }

    inline static void Add(short *src_dst, const short val, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            src_dst[i] += val;
        }
    }

    inline static void Add(float *src_dst, const float val, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            src_dst[i] += val;
        }
    }

    inline static void Add(qs_vect_f &src_dst, const float val, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            src_dst[i] += val;
        }
    }

    inline static void Add(Cpx *src_dst, const float val, uint32_t length) {
        Cpx tval;
        tval.real(val);
        tval.imag(val);

        for (uint32_t i = 0; i < length; i++) {
            src_dst[i] += tval;
        }
    }

    inline static void Add(qs_vect_cpx &src_dst, const float val, uint32_t length) {
        Cpx tval;
        tval.real(val);
        tval.imag(val);

        for (uint32_t i = 0; i < length; i++) {
            src_dst[i] += tval;
        }
    }

    inline static void Add(float *src, float *dst, const float val, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[i] = src[i] + val;
        }
    }

    inline static void Add(qs_vect_f &src, qs_vect_f &dst, const float val, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[i] = src[i] + val;
        }
    }

    inline static void Add(Cpx *src1, Cpx *src2, Cpx *dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[i].real(src1[i].real() + src2[i].real());
            dst[i].imag(src1[i].imag() + src2[i].imag());
        }
    }

    inline static void Add(float *src1_re, float *src1_im, float *src2_re, float *src2_im, float *dst_re, float *dst_im,
                           uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst_re[i] = src1_re[i] + src2_re[i];
            dst_im[i] = src1_im[i] + src2_im[i];
        }
    }

    inline static void ComplexToReal(Cpx *src, float *dst_re, float *dst_im, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst_re[i] = static_cast<float>(src[i].real());
            dst_im[i] = static_cast<float>(src[i].imag());
        }
    }

    inline static void ComplexToReal(qs_vect_cpx &src, qs_vect_f &dst_re, qs_vect_f &dst_im, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst_re[i] = static_cast<float>(src[i].real());
            dst_im[i] = static_cast<float>(src[i].imag());
        }
    }

    inline static void RealFromComplex(Cpx *src, float *dst_re, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst_re[i] = static_cast<float>(src[i].real());
        }
    }

    inline static void ImagFromComplex(Cpx *src, float *dst_im, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst_im[i] = static_cast<float>(src[i].imag());
        }
    }

    inline static void Clip(Cpx *src_dst, float clip_level, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            src_dst[i].real(Bound(-clip_level, src_dst[i].real(), clip_level));
            src_dst[i].imag(Bound(-clip_level, src_dst[i].real(), clip_level));
        }
    }

    inline static void Convert(double *src, float *dest, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dest[i] = static_cast<float>(src[i]);
        }
    }

    inline static void Convert(float *src, double *dest, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dest[i] = static_cast<double>(src[i]);
        }
    }

    inline static void Convert(int *src, float *dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[i] = static_cast<float>(src[i]) * INTTOFLOAT;
        }
    }

    inline static void Convert24(int *src, float *dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[i] = static_cast<float>(src[i]) * INT24TOFLOAT;
        }
    }

    inline static void Convert(float *src, int *dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[i] = static_cast<int>(src[i] * FLOATTOINT);
        }
    }

    inline static void Convert(short *src, float *dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[i] = static_cast<float>(src[i]) * SHORTTOFLOAT;
        }
    }

    inline static void Convert(qs_vect_s &src, qs_vect_f &dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[i] = static_cast<float>(src[i]) * SHORTTOFLOAT;
        }
    }

    inline static void Convert(float *src, unsigned char *dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            float x = src[i];
            if (x > 255.0)
                x = 255.0;
            if (x < 0.0)
                x = 0.0;
            dst[i] = static_cast<unsigned char>(x);
        }
    }

    inline static void Convert(qs_vect_f &src, unsigned char *dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            float x = src[i];
            if (x > 255.0)
                x = 255.0;
            if (x < 0.0)
                x = 0.0;
            dst[i] = static_cast<unsigned char>(x);
        }
    }

    inline static void Convert(float *src, short *dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[i] = static_cast<short>(src[i] * FLOATTOSHORT);
        }
    }

    inline static void Convert(qs_vect_f &src, qs_vect_s &dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[i] = static_cast<short>(src[i] * FLOATTOSHORT);
        }
    }

    inline static void Copy(char *src, char *dst, uint32_t length) { memcpy(dst, src, sizeof(char) * length); }

    inline static void Copy(int *src, int *dst, uint32_t length) { memcpy(dst, src, sizeof(int) * length); }

    inline static void Copy(float *src, float *dst, uint32_t length) { memcpy(dst, src, sizeof(float) * length); }

    inline static void Copy(qs_vect_f &src, qs_vect_f &dst, uint32_t length) {
        // memcpy( dst.data(), src.data(), sizeof( float ) * length );
        std::copy(src.begin(), src.begin() + length, dst.begin());
    }

    inline static void Copy(double *src, double *dst, uint32_t length) { memcpy(dst, src, sizeof(double) * length); }

    inline static void Copy(Cpx *src, Cpx *dst, uint32_t length) { memcpy(dst, src, sizeof(Cpx) * length); }

    inline static void Copy(float *src_re, float *src_im, float *dst_re, float *dst_im, uint32_t length) {
        memcpy(dst_re, src_re, sizeof(float) * length);
        memcpy(dst_im, src_im, sizeof(float) * length);
    }

    inline static void Copy(float *src_re, float *src_im, Cpx *dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[i].real(src_re[i]);
            dst[i].imag(src_im[i]);
        }
    }

    inline static void Copy(Cpx *src, float *dst_re, float *dst_im, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst_re[i] = src[i].real();
            dst_im[i] = src[i].imag();
        }
    }

    inline static void CopyRealToImag(qs_vect_cpx &src_dst) {
        qs_vect_cpx::iterator cpx_itr;
        for (cpx_itr = src_dst.begin(); cpx_itr != src_dst.end(); cpx_itr++) {
            (*cpx_itr).imag((*cpx_itr).real());
        }
    }

    inline static void DeInterleave(float *src, Cpx *dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[i].real(src[2 * i]);
            dst[i].imag(src[2 * i + 1]);
        }
    }

    inline static void DeInterleave(qs_vect_f &src, qs_vect_cpx &dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[i].real(src[2 * i]);
            dst[i].imag(src[2 * i + 1]);
        }
    }

    inline static void DeInterleave(float *src, float *dst_re, float *dst_im, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst_re[i] = src[2 * i];
            dst_im[i] = src[2 * i + 1];
        }
    }

    inline static void DeInterleave(qs_vect_f &src, qs_vect_f &dst_re, qs_vect_f &dst_im, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst_re[i] = src[2 * i];
            dst_im[i] = src[2 * i + 1];
        }
    }

    inline static void DeInterleaveIntToFloat(int *src, float *dst_re, float *dst_im, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst_re[i] = static_cast<float>(src[2 * i]) * INTTOFLOAT;
            dst_im[i] = static_cast<float>(src[2 * i + 1]) * INTTOFLOAT;
        }
    }

    inline static void DeInterleaveIntToCpx(int *src, Cpx *dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[i].real(static_cast<float>(src[2 * i]) * INTTOFLOAT);
            dst[i].imag(static_cast<float>(src[2 * i + 1]) * INTTOFLOAT);
        }
    }

    inline static void DuplicateRealIntoImaginary(Cpx *src_dst, uint32_t length) {
        uint32_t i;
        for (i = 0; i < length; i++) {
            src_dst[i].imag(src_dst[i].real());
        }
    }

    inline static void DuplicateImaginaryIntoReal(Cpx *src_dst, uint32_t length) {
        uint32_t i;
        for (i = 0; i < length; i++) {
            src_dst[i].real(src_dst[i].imag());
        }
    }

    inline static void Fill(char *src_dst, char val, uint32_t length) {
        uint32_t i;
        for (i = 0; i < length; i++) {
            src_dst[i] = val;
        }
    }

    inline static void Fill(unsigned char *src_dst, unsigned char val, uint32_t length) {
        uint32_t i;
        for (i = 0; i < length; i++) {
            src_dst[i] = val;
        }
    }

    inline static void Fill(short *src_dst, short val, uint32_t length) {
        uint32_t i;
        for (i = 0; i < length; i++) {
            src_dst[i] = val;
        }
    }

    inline static void Fill(Cpx *src_dst, float val, uint32_t length) {
        uint32_t i;
        for (i = 0; i < length; i++) {
            src_dst[i].real(val);
            src_dst[i].imag(val);
        }
    }

    inline static void Fill(Cpx *src_dst, float val_re, float val_im, uint32_t length) {
        uint32_t i;
        for (i = 0; i < length; i++) {
            src_dst[i].real(val_re);
            src_dst[i].imag(val_im);
        }
    }

    inline static void Flip(float *src, float *dst, uint32_t length) {
        uint32_t i, j;
        for (i = 0, j = (length - 1); i < length; i++, j--) {
            dst[j] = src[i];
        }
    }

    inline static void Flip(double *src, double *dst, uint32_t length) {
        uint32_t i, j;
        for (i = 0, j = (length - 1); i < length; i++, j--) {
            dst[j] = src[i];
        }
    }

    inline static void Flip(int *src, int *dst, uint32_t length) {
        uint32_t i, j;
        for (i = 0, j = (length - 1); i < length; i++, j--) {
            dst[j] = src[i];
        }
    }

    inline static void Flip(Cpx *src, Cpx *dst, uint32_t length) {
        uint32_t i, j;
        for (i = 0, j = (length - 1); i < length; i++, j--) {
            dst[j].real(src[i].real());
            dst[j].imag(src[i].imag());
        }
    }

    inline static void Interleave(float *src1, float *src2, float *dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[2 * i] = src1[i];
            dst[2 * i + 1] = src2[i];
        }
    }

    inline static void Interleave(qs_vect_f &src1, qs_vect_f &src2, qs_vect_f &dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[2 * i] = src1[i];
            dst[2 * i + 1] = src2[i];
        }
    }

    inline static void Interleave(Cpx *src, float *dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[2 * i] = src[i].real();
            dst[2 * i + 1] = src[i].imag();
        }
    }

    inline static void Interleave(qs_vect_cpx &src, qs_vect_f &dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[2 * i] = src[i].real();
            dst[2 * i + 1] = src[i].imag();
        }
    }

    inline static void Interleave(Cpx *src, short *dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[2 * i] = static_cast<short>(src[i].real() * FLOATTOSHORT);
            dst[2 * i + 1] = static_cast<short>(src[i].imag() * FLOATTOSHORT);
        }
    }

    inline static void LimitAndInterleave(Cpx *src, short *dst, int max_value, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[2 * i] = static_cast<short>(src[i].real() * max_value);
            dst[2 * i + 1] = static_cast<short>(src[i].imag() * max_value);
        }
    }

    inline static void LimitAndInterleave(qs_vect_cpx &src, qs_vect_s &dst, int max_value, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[2 * i] = static_cast<short>(src[i].real() * max_value);
            dst[2 * i + 1] = static_cast<short>(src[i].imag() * max_value);
        }
    }

    inline static void InterleaveFloatToInt(float *src1, float *src2, int *dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[2 * i] = static_cast<int>(src1[i] * FLOATTOINT);
            dst[2 * i + 1] = static_cast<int>(src2[i] * FLOATTOINT);
        }
    }

    inline static void Limit(float *src, uint32_t length) {
        double max = 0.0;
        double val = 0.0;
        for (uint32_t i = 0; i < length; i++) {
            val = QsDataProc::Abs(src[i]);
            if (val > max)
                max = val;
        }
        if (max > 1.0) {
            max = 0.95 / max;
            for (uint32_t i = 0; i < length; i++) {
                src[i] *= max;
            }
        }
    }

    inline static void Limit(Cpx *src, uint32_t length) {
        Cpx maxx(0.0, 0.0);
        Cpx val(0.0, 0.0);
        double mmax = 0.0;

        for (uint32_t i = 0; i < length; i++) {
            val.real(QsDataProc::Abs(src[i].real()));
            val.imag(QsDataProc::Abs(src[i].imag()));

            if (val.real() > maxx.real()) {
                maxx.real(val.real());
            }
            if (val.imag() > maxx.imag()) {
                maxx.imag(val.imag());
            }
        }
        mmax = QsDataProc::Max(maxx.real(), maxx.imag());
        if (mmax > 1.0) {
            mmax = 0.95 / mmax;
            for (uint32_t i = 0; i < length; i++) {
                src[i].real(src[i].real() * mmax);
                src[i].imag(src[i].imag() * mmax);
            }
        }
    }

    inline static void Limit(qs_vect_cpx &src, uint32_t length) {
        Cpx maxx(0.0, 0.0);
        Cpx val(0.0, 0.0);
        double mmax = 0.0;
        if (src.size() < length)
            length = src.size();
        for (uint32_t i = 0; i < length; i++) {
            val.real(QsDataProc::Abs(src[i].real()));
            val.imag(QsDataProc::Abs(src[i].imag()));
            if (val.real() > maxx.real())
                maxx.real(val.real());
            if (val.imag() > maxx.imag())
                maxx.imag(val.imag());
        }
        mmax = QsDataProc::Max(maxx.real(), maxx.imag());
        if (mmax > 1.0) {
            mmax = 0.95 / mmax;
            for (uint32_t i = 0; i < length; i++) {
                src[i].real(src[i].real() * mmax);
                src[i].imag(src[i].imag() * mmax);
            }
        }
    }

    inline static void LimitAndScale(float *src, double volume, uint32_t length) {
        double max = 0.0;
        double val = 0.0;
        for (uint32_t i = 0; i < length; i++) {
            src[i] *= volume;
            val = QsDataProc::Abs(src[i]);
            if (val > max)
                max = val;
        }
        if (max > 1.0) {
            max = 0.95 / max;
            for (uint32_t i = 0; i < length; i++) {
                src[i] *= max;
            }
        }
    }

    inline static void LimitAndScale(qs_vect_f &src, double volume, uint32_t length) {
        double max = 0.0;
        double val = 0.0;
        if (src.size() < length)
            length = src.size();
        for (uint32_t i = 0; i < length; i++) {
            src[i] *= volume;
            val = QsDataProc::Abs(src[i]);
            if (val > max)
                max = val;
        }
        if (max > 1.0) {
            max = 0.95 / max;
            for (uint32_t i = 0; i < length; i++) {
                src[i] *= max;
            }
        }
    }

    inline static void Log10(float *src, float *dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[i] = log10(src[i]);
        }
    }

    inline static void x10Log10(float *src, float *dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[i] = 10.0 * log10(src[i]);
        }
    }

    inline static void Magnitude(Cpx *src, float *magn, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            magn[i] = sqrt(src[i].real() * src[i].real() + src[i].imag() * src[i].imag());
        }
    }

    inline static void Magnitude(float *src_re, float *src_im, float *magn, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            magn[i] = sqrt(src_re[i] * src_re[i] + src_im[i] * src_im[i]);
        }
    }

    inline static double MaxReal(Cpx *src, uint32_t length) {
        double max = 0;
        for (uint32_t i = 0; i < length; i++) {
            if (Abs(src[i].real()) >= max) {
                max = Abs(src[i].real());
            }
        }
        return max;
    }

    inline static double MaxReal(qs_vect_cpx &src, uint32_t length) {
        double max = 0;
        for (uint32_t i = 0; i < length; i++) {
            if (Abs(src[i].real()) >= max) {
                max = Abs(src[i].real());
            }
        }
        return max;
    }

    inline static void Mean(float *src, float &mean, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            mean += src[i];
        }
        mean /= static_cast<float>(length);
    }

    inline static void Mean(qs_vect_f &src, float &mean, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            mean += src[i];
        }
        mean /= static_cast<float>(length);
    }

    inline static void Mean(short *src, short *mean, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            *mean += src[i];
        }
        *mean /= static_cast<short>(length);
    }

    inline static void Move(float *src, float *dst, uint32_t length) { memmove(dst, src, length * sizeof(float)); }

    inline static void Multiply(float *src, float *src_dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            src_dst[i] *= src[i];
        }
    }

    inline static void Multiply(double *src, double *src_dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            src_dst[i] *= src[i];
        }
    }

    inline static void Multiply(Cpx *src, Cpx *src_dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            float tmp_real = src_dst[i].real();
            float tmp_imag = src_dst[i].imag();
            src_dst[i].real((src[i].real() * tmp_real) - (src[i].imag() * tmp_imag));
            src_dst[i].imag((src[i].real() * tmp_imag) + (src[i].imag() * tmp_real));
        }
    }

    inline static void Multiply(Cpx *src1, Cpx *src2, Cpx *dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[i].real((src1[i].real() * src2[i].real()) - (src1[i].imag() * src2[i].imag()));
            dst[i].imag((src1[i].real() * src2[i].imag()) + (src1[i].imag() * src2[i].real()));
        }
    }

    inline static void Multiply(qs_vect_cpx &src1, qs_vect_cpx &src2, qs_vect_cpx &dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[i].real((src1[i].real() * src2[i].real()) - (src1[i].imag() * src2[i].imag()));
            dst[i].imag((src1[i].real() * src2[i].imag()) + (src1[i].imag() * src2[i].real()));
        }
    }

    inline static void Multiply(float *src1_re, float *src1_im, float *src2_re, float *src2_im, float *dst_re,
                                float *dst_im, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst_re[i] = (src2_re[i] * src1_re[i]) - (src2_im[i] * src1_im[i]);
            dst_im[i] = (src2_re[i] * src1_im[i]) + (src2_im[i] * src1_re[i]);
        }
    }

    inline static void Multiply(Cpx *src_dst, const float val, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            src_dst[i].real(src_dst[i].real() * val);
            src_dst[i].imag(src_dst[i].imag() * val);
        }
    }

    inline static void Multiply(qs_vect_cpx &src_dst, const float val, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            src_dst[i].real(src_dst[i].real() * val);
            src_dst[i].imag(src_dst[i].imag() * val);
        }
    }

    inline static void Multiply(float *src_dst_re, float *src_dst_im, const float val, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            src_dst_re[i] *= val;
            src_dst_im[i] *= val;
        }
    }

    inline static void Multiply(qs_vect_f &src_dst_re, qs_vect_f &src_dst_im, const float val, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            src_dst_re[i] *= val;
            src_dst_im[i] *= val;
        }
    }

    inline static void Multiply(Cpx *src, Cpx *dst, const float val, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[i].real(src[i].real() * val);
            dst[i].imag(src[i].imag() * val);
        }
    }

    inline static void Multiply(Cpx *src_dst, const float re_val, const float im_val, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            src_dst[i].real(src_dst[i].real() * re_val);
            src_dst[i].imag(src_dst[i].imag() * im_val);
        }
    }

    inline static void PowerSpectrum(Cpx *src, float *dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dst[i] = src[i].real() * src[i].real() + src[i].imag() * src[i].imag();
        }
    }

    inline static void x10Log10PowerSpectrum(Cpx *src, float *dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            float temp = src[i].real() * src[i].real() + src[i].imag() * src[i].imag();
            dst[i] = 10.0 * log10(temp);
        }
    }

    inline static void x10Log10PowerSpectrum(qs_vect_cpx &src, qs_vect_f &dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            float temp = src[i].real() * src[i].real() + src[i].imag() * src[i].imag();
            dst[i] = 10.0 * log10(temp);
        }
    }

    inline static float IntPow(float base, uint32_t exponent) {
        uint32_t i;
        float out = base;
        for (i = 1; i < exponent; i++) {
            out *= base;
        }
        return out;
    }

    inline static void PowerSpectrumSum(Cpx *src, float &sum, uint32_t length) {
        sum = 0.0;
        for (uint32_t i = 0; i < length; i++) {
            sum += src[i].real() * src[i].real() + src[i].imag() * src[i].imag();
        }
    }

    inline static void PowerSpectrumSum(qs_vect_cpx &src, float &sum, uint32_t length) {
        sum = 0.0;
        if (src.size() < length)
            length = src.size();
        for (uint32_t i = 0; i < length; i++) {
            sum += src[i].real() * src[i].real() + src[i].imag() * src[i].imag();
        }
    }

    inline static void PowerSpectrumSum(float *src_re, float *src_im, float &sum, uint32_t length) {
        sum = 0.0;
        for (uint32_t i = 0; i < length; i++) {
            sum += src_re[i] * src_re[i] + src_im[i] * src_im[i];
        }
    }

    inline static void RealToComplex(float *re_src, float *im_src, Cpx *dest, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dest[i].real(re_src[i]);
            dest[i].imag(im_src[i]);
        }
    }

    inline static void RealToComplex(qs_vect_f &re_src, qs_vect_f &im_src, qs_vect_cpx &dest, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dest[i].real(re_src[i]);
            dest[i].imag(im_src[i]);
        }
    }

    inline static void RealToComplex(float *re_src, Cpx *dest, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            dest[i].real(re_src[i]);
            dest[i].imag(0.0);
        }
    }

    inline static void SampleDown(Cpx *src, uint32_t src_length, Cpx *dst, uint32_t &dst_length, uint32_t factor) {
        uint32_t i, j;
        dst_length = 0;
        for (i = 0, j = 0; i < src_length; i += factor, j++) {
            dst[j] = src[i];
            dst_length++;
        }
    }

    inline static void Set(Cpx *src_dst, Cpx &value, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            src_dst[i].real(value.real());
            src_dst[i].imag(value.imag());
        }
    }

    inline static void Set(Cpx *src_dst, float &value, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            src_dst[i].real(value);
            src_dst[i].imag(value);
        }
    }

    inline static void Set(float *src_dst, float &value, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            src_dst[i] = value;
        }
    }

    inline static void Set(double *src_dst, double &value, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            src_dst[i] = value;
        }
    }

    inline static void Set(int *src_dst, int &value, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            src_dst[i] = value;
        }
    }

    inline static void Scale(Cpx *src_dst, float val, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            src_dst[i].real(src_dst[i].real() * val);
            src_dst[i].imag(src_dst[i].imag() * val);
        }
    }

    inline static void Scale(qs_vect_cpx &src_dst, float val, uint32_t length) {
        if (src_dst.size() < length)
            length = src_dst.size();
        for (uint32_t i = 0; i < length; i++) {
            src_dst[i].real(src_dst[i].real() * val);
            src_dst[i].imag(src_dst[i].imag() * val);
        }
    }

    inline static void Scale(float *src_dst_re, float *src_dst_im, float val, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            src_dst_re[i] *= val;
            src_dst_im[i] *= val;
        }
    }

    inline static void Scale(float *src_dst, float val, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            src_dst[i] *= val;
        }
    }

    inline static void Scale(qs_vect_f &src_dst, float val, uint32_t length) {
        if (src_dst.size() < length)
            length = src_dst.size();
        for (uint32_t i = 0; i < length; i++) {
            src_dst[i] *= val;
        }
    }

    inline static void SinCos(double *phase, double *sinval, double *cosval, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            sinval[i] = sin(phase[i]);
            cosval[i] = cos(phase[i]);
        }
    }

    inline static void SinCos(float *phase, float *sinval, float *cosval, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            sinval[i] = static_cast<float>(sin(phase[i]));
            cosval[i] = static_cast<float>(cos(phase[i]));
        }
    }

    inline static void SinCosMultiply(double *phase, Cpx *src_dst, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            double sinval = sin(phase[i]);
            double cosval = cos(phase[i]);
            float tmp_real = src_dst[i].real();
            float tmp_imag = src_dst[i].imag();
            src_dst[i].real((cosval * tmp_real) - (sinval * tmp_imag));
            src_dst[i].imag((cosval * tmp_imag) + (sinval * tmp_real));
        }
    }

    inline static void Sum(float *src, float &sum, uint32_t length) {
        sum = 0.0;
        for (uint32_t i = 0; i < length; i++) {
            sum += src[i];
        }
    }

    inline static void Subtract(float *src_dst, float val, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            src_dst[i] -= val;
        }
    }

    inline static void Subtract(qs_vect_f &src_dst, float val, uint32_t length) {
        for (uint32_t i = 0; i < length; i++) {
            src_dst[i] -= val;
        }
    }

    inline static void Zero(Cpx *src, uint32_t length) { memset(src, 0, sizeof(Cpx) * length); }

    inline static void Zero(qs_vect_cpx &src) {
        Cpx val(0.0, 0.0);
        std::fill(src.begin(), src.end(), val);
    }

    inline static void Zero(float *src, uint32_t length) { memset(src, 0, sizeof(float) * length); }

    inline static void Zero(qs_vect_i &src) { std::fill(src.begin(), src.end(), 0); }

    inline static void Zero(qs_vect_f &src) { std::fill(src.begin(), src.end(), 0.0); }

    inline static void Zero(qs_vect_s &src) { std::fill(src.begin(), src.end(), 0); }

    inline static void Zero(double *src, uint32_t length) { memset(src, 0, sizeof(double) * length); }

    inline static void Zero(int *src, uint32_t length) { memset(src, 0, sizeof(int) * length); }

    inline static void Zero(short *src, uint32_t length) { memset(src, 0, sizeof(short) * length); }

    inline static void Zero(char *src, uint32_t length) { memset(src, 0, sizeof(char) * length); }

    inline static void Zero(unsigned char *src, uint32_t length) { memset(src, 0, sizeof(unsigned char) * length); }

    inline static void Zero(float *src_re, float *src_im, uint32_t length) {
        memset(src_re, 0, sizeof(float) * length);
        memset(src_im, 0, sizeof(float) * length);
    }

    inline static void Zero(int64_t *src, uint32_t length) { memset(src, 0, sizeof(int64_t) * length); }
};