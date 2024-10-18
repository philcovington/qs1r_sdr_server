/**
 * @file qs_fft.hpp
 * @brief Provides a class for performing Fast Fourier Transform (FFT) operations.
 * 
 * This class implements both forward and inverse Discrete Fourier Transform (DFT) using various input types. 
 * It supports normalization and can work with complex and real-valued vectors.
 * 
 * Features:
 * - Resize functionality for DFT operations
 * - Multiple overloads for DFT forward and inverse calculations
 * - Utilizes a working area for efficient computations
 * 
 * Usage:
 * To use this class, create an instance of QsFFT and call the desired DFT function.
 * 
 * Notes:
 * Ensure to call resize() before performing FFT operations to allocate the necessary memory.
 * 
 * Author: Philip A Covington
 * Date: 2024-10-17
 */

#pragma once

#include "../include/qs_signalops.hpp"
#include "../include/qs_types.hpp"

class QsFFT {

  public:
    explicit QsFFT();

    void resize(int size);

    void doDFTForward(qs_vect_cpx &src_dst, int length, float normalize_val = 1.0);
    void doDFTForward(qs_vect_cpx &src, qs_vect_cpx &dst, int length, float normalize_val = 1.0);
    void doDFTForward(qs_vect_f &src_re, qs_vect_f &src_im, qs_vect_f &dst_re, qs_vect_f &dst_im, int length,
                      float normalize_val = 1.0);
    void doDFTInverse(qs_vect_cpx &src_dst, int length, float normalize_val = 1.0);
    void doDFTInverse(qs_vect_cpx &src, qs_vect_cpx &dst, int length, float normalize_val = 1.0);
    void doDFTInverse(qs_vect_f &src_re, qs_vect_f &src_im, qs_vect_f &dst_re, qs_vect_f &dst_im, int length,
                      float normalize_val = 1.0);

  private:
    int size;
    int half_sz;

    qs_vect_i p_ip_workarea;
    qs_vect_f p_w_cossintable;
    qs_vect_f p_tmp_buffer;
    qs_vect_f p_tmp_re;
    qs_vect_f p_tmp_im;

    float GetPower(float re, float im);

    void cdft(int n, int isgn, qs_vect_f &a, qs_vect_i &ip, qs_vect_f &w);
    void cdft_fwd(int n, qs_vect_f &a, qs_vect_i &ip, qs_vect_f &w);
    void cdft_rev(int n, qs_vect_f &a, qs_vect_i &ip, qs_vect_f &w);
    void rdft(int n, int isgn, qs_vect_f &a, qs_vect_i &ip, qs_vect_f &w);

    void makewt(int nw, qs_vect_i &ip, qs_vect_f &w);
    void makect(int nc, qs_vect_i &ip, float *c);

    void bitrv2(int n, int *fp, qs_vect_f &a);
    void bitrv2conj(int n, int *fp, qs_vect_f &a);
    void cftfsub(int n, qs_vect_f &a, qs_vect_f &w);
    void cftbsub(int n, qs_vect_f &a, qs_vect_f &w);
    void cft1st(int n, qs_vect_f &a, qs_vect_f &w);
    void cftmdl(int n, int l, qs_vect_f &a, qs_vect_f &w);
    void rftfsub(int n, qs_vect_f &a, int nc, float *c);
    void rftbsub(int n, qs_vect_f &a, int nc, float *c);
};
