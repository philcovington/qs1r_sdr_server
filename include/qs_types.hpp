/**
 * @file qs_types.hpp
 * @brief Type definitions for signal processing vectors and complex numbers.
 *
 * This header file contains type definitions used throughout the application
 * for signal processing. It includes definitions for complex numbers and 
 * vectors of various data types, which are essential for handling signals
 * in applications such as communication systems and digital signal processing.
 *
 * Features:
 * - Definition of complex number type using float precision.
 * - Type definitions for vectors of complex numbers, floating-point numbers,
 *   integers, and unsigned 16-bit integers.
 *
 * Usage:
 * Include this header file in any source file where signal processing or 
 * complex number calculations are required. The defined types simplify 
 * the use of complex arithmetic and vector manipulations.
 *
 * Example:
 *   #include "qs_types.hpp"
 *
 *   qs_vect_cpx complexSignals;  // Vector of complex numbers
 *   qs_vect_f floatData;         // Vector of floating-point numbers
 *   qs_vect_i intData;           // Vector of integers
 *
 * Author: [Philip A Covington]
 * Date: [2024-10-16]
 */

#pragma once

#include <complex>
#include <vector>

typedef std::complex<float> Cpx;

typedef std::vector<Cpx> qs_vect_cpx;
typedef std::vector<float> qs_vect_f;
typedef std::vector<int> qs_vect_i;
typedef std::vector<uint16_t> qs_vect_s;
