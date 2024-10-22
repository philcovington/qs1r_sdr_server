#pragma once

#include <samplerate.h>
#include <stdexcept>
#include <vector>

class Resampler {
  public:
    Resampler(int src_rate, int dest_rate);
    ~Resampler();

    void process(const float *input, size_t input_frames, float *output, size_t *output_frames);

  private:
    SRC_STATE *src_state;
    float src_ratio;
};
