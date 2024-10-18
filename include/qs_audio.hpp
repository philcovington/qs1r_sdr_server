/*
 * File: qs_audio.hpp
 * Brief: Audio interface class for managing real-time audio input/output using RtAudio.
 * 
 * This header defines the `QsAudio` class, which is responsible for initializing and 
 * managing audio streams using the RtAudio library. The class provides methods for 
 * starting, stopping, and interacting with audio devices, as well as managing audio 
 * input/output buffers.
 *
 * Features:
 * - Encapsulates RtAudio functionality for audio device management and streaming.
 * - Provides methods to start and stop audio streams.
 * - Allows retrieval of available audio input and output devices, including default devices.
 * - Validates input and output device IDs for correctness.
 * - Uses a static callback mechanism (`sta_rt_callback`) for handling real-time audio events.
 *
 * Usage:
 * Use the `initAudio()` method to initialize the audio stream with desired parameters. 
 * Start and stop the stream using `startStream()` and `stopStream()`. Audio device 
 * information can be retrieved via `getInputDevices()` and `getOutputDevices()`.
 *
 * Notes:
 * - The class uses `std::unique_ptr` to manage the lifecycle of the RtAudio object.
 * - Callbacks are handled via the static method `sta_rt_callback`, which delegates to 
 *   the instance-specific `RtCallback()` method.
 * 
 * Author: Philip A Covington
 * Date: 2024-10-16
 */

#pragma once

#include "../include/qs_listclass.hpp"
#include "../include/qs_mapclass.hpp"
#include "../include/qs_globals.hpp"
#include "../include/qs_rt_audio.hpp"
#include "../include/qs_stringclass.hpp"
#include "../include/qs_stringlistclass.hpp"
#include <memory>

class QsAudio {

  private:
    std::unique_ptr<RtAudio> p_rta;

    int RtCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime,
                   RtAudioStreamStatus status);

    static int sta_rt_callback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime,
                               RtAudioStreamStatus status, void *userData) {
        return ((QsAudio *)userData)->RtCallback(outputBuffer, inputBuffer, nBufferFrames, streamTime, status);
    }

    Map<int, String> rtaInputDeviceMap;
    Map<int, String> rtaOutputDeviceMap;
    double m_sample_rate;
    int stop_stream_request;

  public:
    QsAudio();

    bool initAudio(int frames, double sample_rate, int in_dev_id, int out_dev_id, bool &ok);

    void startStream() {
        stop_stream_request = 0;
        if (p_rta->isStreamOpen())
            p_rta->startStream();
    }
    void stopStream() {
        stop_stream_request = 1;
        if (p_rta->isStreamRunning())
            p_rta->stopStream();
    }

    StringList getOutputDevices();
    StringList getInputDevices();

    int getDefaultOutputDevice();
    int getDefaultInputDevice();

    bool isOutputDeviceValid(int id, String &descr);
    bool isInputDeviceValid(int id, String &descr);
};
