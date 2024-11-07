#pragma once

#include "../include/qs_globals.hpp"
#include "../include/qs_signalops.hpp"
#include "../include/qs_types.hpp"
#include <cmath>
#include <complex>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

class BandPassFilter {
  public:
    BandPassFilter() {}

    // Set or update the filter parameters using lower and upper frequency limits
    void setFilter(double lower_freq, double upper_freq, double sample_rate) {
        // Calculate center frequency and bandwidth from lower and upper frequencies
        double center_freq = (upper_freq + lower_freq) / 2.0;
        double bandwidth = upper_freq - lower_freq;

        double omega = 2.0 * M_PI * center_freq / sample_rate;
        double alpha = std::sin(omega) * std::sinh(std::log(2.0) / 2.0 * bandwidth * omega / std::sin(omega));

        b0 = alpha;
        b1 = 0.0;
        b2 = -alpha;
        a0 = 1.0 + alpha;
        a1 = -2.0 * std::cos(omega);
        a2 = 1.0 - alpha;

        // Normalize coefficients
        b0 /= a0;
        b1 /= a0;
        b2 /= a0;
        a1 /= a0;
        a2 /= a0;
    }

    // Apply the filter to a block of complex samples
    std::vector<std::complex<float>> applyToData(const std::vector<std::complex<float>> &input) {
        std::vector<std::complex<float>> output(input.size());
        for (size_t i = 0; i < input.size(); ++i) {
            double real = input[i].real();
            double imag = input[i].imag();

            // Apply filter to real part
            double filtered_real = b0 * real + b1 * z1_real + b2 * z2_real - a1 * z1_out_real - a2 * z2_out_real;
            z2_real = z1_real;
            z1_real = real;
            z2_out_real = z1_out_real;
            z1_out_real = filtered_real;

            // Apply filter to imaginary part
            double filtered_imag = b0 * imag + b1 * z1_imag + b2 * z2_imag - a1 * z1_out_imag - a2 * z2_out_imag;
            z2_imag = z1_imag;
            z1_imag = imag;
            z2_out_imag = z1_out_imag;
            z1_out_imag = filtered_imag;

            output[i] = std::complex<float>(filtered_real, filtered_imag);
        }
        return output;
    }

  private:
    // Filter coefficients
    double b0, b1, b2, a0, a1, a2;
    // Filter state variables for real and imaginary parts
    double z1_real = 0.0, z2_real = 0.0, z1_out_real = 0.0, z2_out_real = 0.0;
    double z1_imag = 0.0, z2_imag = 0.0, z1_out_imag = 0.0, z2_out_imag = 0.0;
};
