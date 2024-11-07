#pragma once

#include "../include/qs_globals.hpp"
#include "../include/qs_signalops.hpp"
#include "../include/qs_types.hpp"
#include <deque>
#include <string>
#include <unordered_map>

// Butterworth low-pass filter for prefiltering
class ButterworthLowPass {
  private:
    double a0, a1, a2, b1, b2;
    double z1, z2;

  public:
    ButterworthLowPass() : z1(0.0), z2(0.0) {}

    void init(double cutoffFreq, double sampleRate) {
        double omega = 2.0 * M_PI * cutoffFreq / sampleRate;
        double cos_omega = std::cos(omega);
        double sin_omega = std::sin(omega);
        double alpha = sin_omega / std::sqrt(2.0);

        double norm = 1.0 / (1.0 + alpha);
        a0 = (1.0 - cos_omega) * 0.5 * norm;
        a1 = (1.0 - cos_omega) * norm;
        a2 = a0;
        b1 = -2.0 * cos_omega * norm;
        b2 = (1.0 - alpha) * norm;
    }

    double processSample(double sample) {
        double output = a0 * sample + a1 * z1 + a2 * z2 - b1 * z1 - b2 * z2;
        z2 = z1;
        z1 = output;
        return output;
    }

    qs_vect_cpx applyToData(qs_vect_cpx &data) {
        qs_vect_cpx output(data.size());
        for (auto &sample : output) {
            sample.real(processSample(sample.real()));
            sample.imag(processSample(sample.imag()));
        }
        return output;
    }
};