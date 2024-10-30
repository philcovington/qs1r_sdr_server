#pragma once

#include "../include/qs_types.hpp"
#include <cmath>
#include <vector>

class DeEmphasis {
  public:
    DeEmphasis();
    ~DeEmphasis();

    void init(uint32_t sample_rate, double deemphasis = 75e-6f);
    void process(qs_vect_cpx &src_dst);

  private:
    uint32_t m_samplerate;
    double m_de_emphasis_time_const;
    float m_alpha;
    float m_prev_output;
	bool m_is_init = false;
};
