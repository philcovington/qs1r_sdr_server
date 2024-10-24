#pragma once

#include "../include/qs_types.hpp" // Complex vector definition
#include <cmath>

enum DemodMode { NARROW, WIDE }; // Enum for demodulator modes

class QsFMCombinedDemodulator {
  public:
    QsFMCombinedDemodulator(); // Constructor

    // Initialize the demodulator with a given mode (NARROW or WIDE)
    void init(DemodMode mode);

    // Process the input vector of complex samples and demodulate them
    void process(qs_vect_cpx &src_dst, DemodMode mode=NARROW);

  private:
    DemodMode m_mode; // Mode selector (NARROW or WIDE)

    // Internal variables for demodulation parameters
    float m_bw;           // Bandwidth
    float m_limit;        // Limit for the NCO frequency
    float m_zeta;         // Damping factor
    float m_norm;         // Normalization factor
    float m_cos;          // NCO cosine component
    float m_sin;          // NCO sine component
    float m_ncoPhase;     // NCO phase
    float m_phaseError;   // Phase error
    float m_ncoFreq;      // NCO frequency
    float m_ncoHighLimit; // Upper limit for the NCO frequency
    float m_ncoLowLimit;  // Lower limit for the NCO frequency
    float m_alpha;        // Loop filter alpha coefficient
    float m_beta;         // Loop filter beta coefficient
    float m_freqDcError;  // DC error for frequency drift compensation
    float m_dc_alpha;     // Alpha coefficient for DC error compensation
    float m_outgain;      // Output gain factor

    // Utility functions could be declared here if needed (e.g., private helper functions)
};

