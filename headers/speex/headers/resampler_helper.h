#ifndef RESAMPLER_HELPER_H
#define RESAMPLER_HELPER_H

#ifdef _USE_SSE_

#ifdef __cplusplus
extern "C" {
#endif

float inner_product_single(const float *a, const float *b, unsigned int len);
float interpolate_product_single(const float *a, const float *b, unsigned int len, const unsigned int oversample,
                                 float *frac);
double inner_product_double(const float *a, const float *b, unsigned int len);
double interpolate_product_double(const float *a, const float *b, unsigned int len, const unsigned int oversample,
                                  float *frac);

#ifdef __cplusplus
}
#endif
#endif
#endif // RESAMPLER_HELPER_H