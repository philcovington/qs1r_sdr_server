#include "../include/qs_resampler.hpp" // Include the header file for the class

Resampler::Resampler(int src_rate, int dest_rate) {
    int error;
    src_state = src_new(SRC_SINC_BEST_QUALITY, 1, &error); // Mono
    if (!src_state) {
        throw std::runtime_error("Error initializing resampler: " + std::string(src_strerror(error)));
    }
    src_ratio = static_cast<float>(dest_rate) / src_rate;
}

Resampler::~Resampler() {
    if (src_state) {
        src_delete(src_state);
    }
}

void Resampler::process(const float *input, size_t input_frames, float *output, size_t *output_frames) {
    SRC_DATA src_data;
    src_data.data_in = input;                // Pointer to input data
    src_data.input_frames = input_frames;    // Total number of input frames
    src_data.data_out = output;              // Pointer to output data
    src_data.output_frames = *output_frames; // Maximum number of output frames
    src_data.src_ratio = src_ratio;          // Resampling ratio

    int error = src_process(src_state, &src_data);
    if (error != 0) {
        throw std::runtime_error("Error during resampling: " + std::string(src_strerror(error)));
    }

    *output_frames = src_data.output_frames_gen; // Update output frames count
}
