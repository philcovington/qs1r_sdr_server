#pragma once

#ifndef SPEEX_VERSION
#define SPEEX_MAJOR_VERSION 1          /**< Major Speex version. */
#define SPEEX_MINOR_VERSION 1          /**< Minor Speex version. */
#define SPEEX_MICRO_VERSION 15         /**< Micro Speex version. */
#define SPEEX_EXTRA_VERSION ""         /**< Extra Speex version. */
#define SPEEX_VERSION "speex-1.2beta3" /**< Speex version string. */
#endif

// #ifndef FLOATING_POINT
// #error You now need to define either FIXED_POINT or FLOATING_POINT
// #define FLOATING_POINT
// #endif

#ifndef OUTSIDE_SPEEX
#include "../headers/speex_types.h"
#endif

#define ABS(x) ((x) < 0 ? (-(x)) : (x))     /**< Absolute integer value. */
#define ABS16(x) ((x) < 0 ? (-(x)) : (x))   /**< Absolute 16-bit value.  */
#define MIN16(a, b) ((a) < (b) ? (a) : (b)) /**< Maximum 16-bit value.   */
#define MAX16(a, b) ((a) > (b) ? (a) : (b)) /**< Maximum 16-bit value.   */
#define ABS32(x) ((x) < 0 ? (-(x)) : (x))   /**< Absolute 32-bit value.  */
#define MIN32(a, b) ((a) < (b) ? (a) : (b)) /**< Maximum 32-bit value.   */
#define MAX32(a, b) ((a) > (b) ? (a) : (b)) /**< Maximum 32-bit value.   */

typedef float spx_mem_t;
typedef float spx_coef_t;
typedef float spx_lsp_t;
typedef float spx_sig_t;
typedef float spx_word16_t;
typedef float spx_word32_t;

#define Q15ONE 1.0f
#define LPC_SCALING 1.f
#define SIG_SCALING 1.f
#define LSP_SCALING 1.f
#define GAMMA_SCALING 1.f
#define GAIN_SCALING 1.f
#define GAIN_SCALING_1 1.f

#define VERY_SMALL 1e-15f
#define VERY_LARGE32 1e15f
#define VERY_LARGE16 1e15f
#define Q15_ONE ((spx_word16_t)1.f)

#define QCONST16(x, bits) (x)
#define QCONST32(x, bits) (x)

#define NEG16(x) (-(x))
#define NEG32(x) (-(x))
#define EXTRACT16(x) (x)
#define EXTEND32(x) (x)
#define SHR16(a, shift) (a)
#define SHL16(a, shift) (a)
#define SHR32(a, shift) (a)
#define SHL32(a, shift) (a)
#define PSHR16(a, shift) (a)
#define PSHR32(a, shift) (a)
#define VSHR32(a, shift) (a)
#define SATURATE16(x, a) (x)
#define SATURATE32(x, a) (x)

#define PSHR(a, shift) (a)
#define SHR(a, shift) (a)
#define SHL(a, shift) (a)
#define SATURATE(x, a) (x)

#define ADD16(a, b) ((a) + (b))
#define SUB16(a, b) ((a) - (b))
#define ADD32(a, b) ((a) + (b))
#define SUB32(a, b) ((a) - (b))
#define MULT16_16_16(a, b) ((a) * (b))
#define MULT16_16(a, b) ((spx_word32_t)(a) * (spx_word32_t)(b))
#define MAC16_16(c, a, b) ((c) + (spx_word32_t)(a) * (spx_word32_t)(b))

#define MULT16_32_Q11(a, b) ((a) * (b))
#define MULT16_32_Q13(a, b) ((a) * (b))
#define MULT16_32_Q14(a, b) ((a) * (b))
#define MULT16_32_Q15(a, b) ((a) * (b))
#define MULT16_32_P15(a, b) ((a) * (b))

#define MAC16_32_Q11(c, a, b) ((c) + (a) * (b))
#define MAC16_32_Q15(c, a, b) ((c) + (a) * (b))

#define MAC16_16_Q11(c, a, b) ((c) + (a) * (b))
#define MAC16_16_Q13(c, a, b) ((c) + (a) * (b))
#define MAC16_16_P13(c, a, b) ((c) + (a) * (b))
#define MULT16_16_Q11_32(a, b) ((a) * (b))
#define MULT16_16_Q13(a, b) ((a) * (b))
#define MULT16_16_Q14(a, b) ((a) * (b))
#define MULT16_16_Q15(a, b) ((a) * (b))
#define MULT16_16_P15(a, b) ((a) * (b))
#define MULT16_16_P13(a, b) ((a) * (b))
#define MULT16_16_P14(a, b) ((a) * (b))

#define DIV32_16(a, b) (((spx_word32_t)(a)) / (spx_word16_t)(b))
#define PDIV32_16(a, b) (((spx_word32_t)(a)) / (spx_word16_t)(b))
#define DIV32(a, b) (((spx_word32_t)(a)) / (spx_word32_t)(b))
#define PDIV32(a, b) (((spx_word32_t)(a)) / (spx_word32_t)(b))

#define BYTES_PER_CHAR 1
#define BITS_PER_CHAR 8
#define LOG2_BITS_PER_CHAR 3
