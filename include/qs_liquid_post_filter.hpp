#pragma once

#include <iostream>
#include <liquid/liquid.h>
#include <vector>
#include <complex>

class DemodPostFilter {
public:
    // Constructor
    DemodPostFilter() : sample_rate_(50000), low_cut_(300), high_cut_(3500) {}

    // Destructor
    ~DemodPostFilter() {
        if (filter_real_) {
            firfilt_rrrf_destroy(filter_real_);
        }
        if (filter_imag_) {
            firfilt_rrrf_destroy(filter_imag_);
        }
    }

    // Apply the filter to an input signal (in-place for std::complex<float>)
    void process(std::vector<std::complex<float>>& cpx_buf) {
        for (size_t i = 0; i < cpx_buf.size(); ++i) {
            float real_filtered, imag_filtered;

            // Process the real part
            firfilt_rrrf_push(filter_real_, cpx_buf[i].real());
            firfilt_rrrf_execute(filter_real_, &real_filtered);

            // Process the imaginary part
            firfilt_rrrf_push(filter_imag_, cpx_buf[i].imag());
            firfilt_rrrf_execute(filter_imag_, &imag_filtered);

            // Store the filtered complex sample back into the buffer
            cpx_buf[i] = std::complex<float>(real_filtered, imag_filtered);
        }
    }

    // Create the filter with the specified parameters
    void create_filter(float sample_rate, float low_cut, float high_cut) {
        // Filter parameters
        unsigned int filter_order = 101; // Higher order for steeper rolloff
        low_cut_ = low_cut;
        high_cut_ = high_cut;
        sample_rate_ = sample_rate;
        float fc_low = low_cut_ / sample_rate_;   // Normalize cutoffs
        float fc_high = high_cut_ / sample_rate_;

        // Design filter taps for a bandpass filter
        std::vector<float> taps(filter_order + 1);
        liquid_firdes_kaiser(filter_order, fc_low, fc_high, 60.0f, taps.data());

        // Create separate FIR filters for the real and imaginary parts
        filter_real_ = firfilt_rrrf_create(taps.data(), taps.size());
        filter_imag_ = firfilt_rrrf_create(taps.data(), taps.size());
    }

    // Disable copy and assignment
    DemodPostFilter(const DemodPostFilter&) = delete;
    DemodPostFilter& operator=(const DemodPostFilter&) = delete;

private:
    float sample_rate_;
    float low_cut_;
    float high_cut_;
    firfilt_rrrf filter_real_ = nullptr;
    firfilt_rrrf filter_imag_ = nullptr;
};
