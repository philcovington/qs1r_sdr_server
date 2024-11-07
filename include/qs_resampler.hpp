#pragma once

#include <vector>

class Resampler {
public:
    Resampler(float inputRate, float outputRate)
        : inputRate_(inputRate), outputRate_(outputRate), ratio_(inputRate / outputRate), lastSample_(0.0f) {}

    void process(const std::vector<float>& input, std::vector<float>& output) {
        size_t inputSize = input.size();
        size_t outputSize = static_cast<size_t>(inputSize / ratio_);
        
        // Pre-allocate output vector to avoid dynamic resizing
        output.resize(outputSize);

        float position = 0.0f;

        for (size_t i = 0; i < outputSize; ++i) {
            size_t index = static_cast<size_t>(position);
            float fraction = position - index;

            // Perform linear interpolation
            if (index + 1 < inputSize) {
                output[i] = (1.0f - fraction) * input[index] + fraction * input[index + 1];
            } else {
                // Use the last known sample if we reach the end of the input buffer
                output[i] = input[index];
            }

            position += ratio_;
        }

        // Store the last sample for the next call (useful for continuous streams)
        lastSample_ = input.back();
    }

private:
    float inputRate_;
    float outputRate_;
    float ratio_;
    float lastSample_;
};
