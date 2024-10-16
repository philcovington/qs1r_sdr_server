#pragma once

#include "../headers/qs_globals.h"
#include "../headers/qs_rt_audio.h"
#include "../headers/stringclass.h"
#include "../headers/stringlistclass.h"
#include "../headers/mapclass.h"
#include "../headers/listclass.h"

class QsAudio {

  private:
    auto_ptr<RtAudio> p_rta;

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