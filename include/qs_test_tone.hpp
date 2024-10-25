
/**
 * @file QsTestTone.hpp
 * @brief Tone generation class supporting float, integer, and complex types.
 * 
 * This class generates a test tone of a specified frequency, amplitude, and sample rate.
 * It is templated to support multiple data types, including float, integer, and std::complex<float>.
 * For complex types, the output is a quadrature signal, with sine in the real part and cosine in the imaginary part.
 * 
 * @tparam T Data type for the tone buffer (float, int, or std::complex<float>).
 * 
 * ## Features
 * - Generates a sine wave (or quadrature signal for complex).
 * - Supports various data types.
 * - Adjustable frequency, amplitude, and sample rate.
 * 
 * ## Usage
 * ```cpp
 * QsTestTone<float> toneGenerator(1024); // Buffer size 1024, float type
 * toneGenerator.setFrequency(1000.0f);
 * toneGenerator.setAmplitude(0.5f);
 * toneGenerator.setSampleRate(44100);
 * const auto& buffer = toneGenerator.generateTone();
 * ```
 * 
 * @note Phase is internally managed to ensure continuity across successive calls.
 * 
 * @author Philip A Covington
 * @date 2024-10-25
 */

#pragma once

#include <cmath>
#include <complex>
#include <type_traits>
#include <vector>

template <typename T> class QsTestTone {
  public:
    QsTestTone(size_t bufferSize = 1024)
        : m_bufferSize(bufferSize), frequency(1000.0f), amplitude(0.25f), sampleRate(48000), outbuffer(bufferSize) {}

    // Function to generate the tone and return the buffer
    const std::vector<T> &generateTone() {
        float phaseIncrement = 2.0f * M_PI * frequency / sampleRate;

        for (size_t i = 0; i < m_bufferSize; ++i) {
            if constexpr (std::is_same<T, float>::value) {
                outbuffer[i] = amplitude * std::sin(phase);
            } else if constexpr (std::is_integral<T>::value) {
                outbuffer[i] = static_cast<T>(amplitude * std::sin(phase));
            } else if constexpr (std::is_same<T, std::complex<float>>::value) {
                outbuffer[i] = std::complex<float>(amplitude * std::sin(phase), amplitude * std::cos(phase));
            }

            phase += phaseIncrement;
            if (phase > 2.0f * M_PI) {
                phase -= 2.0f * M_PI;
            }
        }
        return outbuffer;
    }

    // Setters to change parameters
    void setFrequency(float freq) { frequency = freq; }
    void setAmplitude(float amp) { amplitude = amp; }
    void setSampleRate(int rate) { sampleRate = rate; }

    // Getters for parameters
    float getFrequency() const { return frequency; }
    float getAmplitude() const { return amplitude; }
    int getSampleRate() const { return sampleRate; }
    size_t getBufferSize() const { return m_bufferSize; }

  private:
    float frequency;
    float amplitude;
    int sampleRate;
    size_t m_bufferSize;
    float phase = 0.0f;
    std::vector<T> outbuffer;
};


