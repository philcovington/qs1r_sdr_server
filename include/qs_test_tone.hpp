#pragma once

#include "../include/qs_globals.hpp"
#include "../include/qs_signalops.hpp"
#include "../include/qs_types.hpp"

class QsTestTone {
  public:
    void init(float frequency=1000.0f, float amplitude=0.1f, uint32_t samplerate=50000);
    void setFrequency(float frequency);
    void setAmplitude(float amplitude);
    void process(qs_vect_f &in_out, size_t blocksize);

  private:
    float m_tt_amplitude = 0.1f;
    float m_tt_frequency = 1000.0f;
    float m_tt_phase = 0;
    float m_tt_phaseIncrement = 0;
};