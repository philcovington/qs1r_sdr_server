#include "../headers/qs_audio.h"
#include "../headers/qs1rserver.h"
#include "../headers/qs_globals.h"
#include "../headers/qs_dataproc.h"

QsAudio ::QsAudio() : p_rta(new RtAudio()), stop_stream_request(0) {}

bool QsAudio ::initAudio(int frames, double sample_rate, int in_dev_id, int out_dev_id, bool &ok) {
    stop_stream_request = 0;

    if (p_rta->isStreamOpen())
        p_rta->closeStream();

    ok = false;

    unsigned int devcount = p_rta->getDeviceCount();

    if (devcount < 1) {
        qDebug() << "Info: No sound devices found!";
        QsGlobal::g_server->setStatusText("Info: No sound devices found!");
        return ok;
    }

    rtaOutputDeviceMap.clear();
    rtaInputDeviceMap.clear();

    RtAudio::DeviceInfo info;
    QList<unsigned int> rates;

    for (unsigned int i = 0; i < devcount; i++) {
        info = p_rta->getDeviceInfo(i);
        if (info.outputChannels > 0) {
            rtaOutputDeviceMap[i] = QString::fromStdString(info.name);
            rates = QList<unsigned int>::fromVector(QVector<unsigned int>::fromStdVector(info.sampleRates));
        } else if (info.inputChannels > 0) {
            rtaInputDeviceMap[i] = QString::fromStdString(info.name);
            rates = QList<unsigned int>::fromVector(QVector<unsigned int>::fromStdVector(info.sampleRates));
        }
    }

    RtAudio::StreamParameters out_rta_parameters;
    RtAudio::StreamParameters in_rta_parameters;

    if ((in_dev_id == -1) | (in_dev_id > devcount - 1)) {
        in_dev_id = p_rta->getDefaultInputDevice();
    } else {
        in_rta_parameters.deviceId = in_dev_id;
    }
    if ((out_dev_id == -1) | (out_dev_id > devcount - 1)) {
        out_dev_id = p_rta->getDefaultOutputDevice();
    } else {
        out_rta_parameters.deviceId = out_dev_id;
    }

    out_rta_parameters.nChannels = 2;
    out_rta_parameters.firstChannel = 0;
    in_rta_parameters.nChannels = 2;
    in_rta_parameters.firstChannel = 0;

    RtAudio::StreamOptions rta_options;
#ifdef Q_OS_WIN
    rta_options.flags |= RTAUDIO_MINIMIZE_LATENCY;
#endif
    rta_options.flags |= RTAUDIO_SCHEDULE_REALTIME;
    rta_options.priority = 31;
    rta_options.numberOfBuffers = 2;

    unsigned int frames_ = frames;
    unsigned int rate = (unsigned int)sample_rate;

    try {
        if (QsTx::g_qs1e_present == true) {
            p_rta->openStream(&out_rta_parameters, &in_rta_parameters, RTAUDIO_FLOAT32, rate, &frames_, sta_rt_callback,
                              this, &rta_options);
        } else {
            p_rta->openStream(&out_rta_parameters, NULL, RTAUDIO_FLOAT32, rate, &frames_, sta_rt_callback, this,
                              &rta_options);
        }
        m_sample_rate = rate;
        // qDebug() << "frames: " << frames_;
    } catch (RtError &e) {
        if (QString(e.what()).contains("does not support input")) {
            try {
                QsTx::g_qs1e_present = false;
                p_rta->openStream(&out_rta_parameters, NULL, RTAUDIO_FLOAT32, rate, &frames_, sta_rt_callback, this,
                                  &rta_options);
                m_sample_rate = rate;
                QsGlobal::g_server->setStatusText("Audio Error: No valid audio input device. QS1E bypassed.");
            } catch (RtError &e) {
                qDebug() << "Audio Error: " + QString(e.what());
                QsGlobal::g_server->setStatusText("Audio Error: " + QString(e.what()));
                return ok;
            }
        } else {
            qDebug() << "Audio Error: " + QString(e.what());
            QsGlobal::g_server->setStatusText("Audio Error: " + QString(e.what()));
            return ok;
        }
    }

    ok = true;
    return ok;
}

// ------------------------------------------------------------
// This is the RtAudio callback
// ------------------------------------------------------------
int QsAudio ::RtCallback(void *outputBuffer, void *inputBuffer, unsigned int nBufferFrames, double streamTime,
                         RtAudioStreamStatus status) {
    int size = nBufferFrames * 2;

    if (QsGlobal::g_float_rt_ring->readAvail() >= size) {
        QsGlobal::g_float_rt_ring->read((float *)outputBuffer, size);
    } else {
        QsSpl::Zero((float *)outputBuffer, size);
    }

    if (QsTx::g_qs1e_present == true) {
        if (QsGlobal::g_float_tx_ring->writeAvail() >= size) {
            QsGlobal::g_float_tx_ring->write((float *)inputBuffer, size);
        }
    }

    return stop_stream_request;
}

QStringList QsAudio ::getOutputDevices() {
    unsigned int devcount = p_rta->getDeviceCount();
    RtAudio::DeviceInfo info;
    QStringList list;

    list.clear();

    if (devcount < 1) {
        list.append("No audio devices found.");
    } else {
        rtaOutputDeviceMap.clear();
        for (unsigned int i = 0; i < devcount; i++) {
            info = p_rta->getDeviceInfo(i);
            if (info.outputChannels > 1) {
                rtaOutputDeviceMap[i] = QString::fromStdString(info.name);
                list.append("id: " + QString::number(i) + " -> " + QString::fromStdString(info.name));
            }
        }
    }
    return list;
}

QStringList QsAudio ::getInputDevices() {
    unsigned int devcount = p_rta->getDeviceCount();
    RtAudio::DeviceInfo info;
    QStringList list;

    list.clear();

    if (devcount < 1) {
        list.append("No audio devices found.");
    } else {
        rtaInputDeviceMap.clear();
        for (unsigned int i = 0; i < devcount; i++) {
            info = p_rta->getDeviceInfo(i);
            if (info.inputChannels > 1) {
                rtaInputDeviceMap[i] = QString::fromStdString(info.name);
                list.append("id: " + QString::number(i) + " -> " + QString::fromStdString(info.name));
            }
        }
    }
    return list;
}

bool QsAudio ::isOutputDeviceValid(int id, QString &descr) {
    bool result = false;
    descr.clear();
    if (rtaOutputDeviceMap.contains(id)) {
        result = true;
        descr.append(QString(rtaOutputDeviceMap[id]));
    }
    return result;
}

bool QsAudio ::isInputDeviceValid(int id, QString &descr) {
    bool result = false;
    descr.clear();
    if (rtaInputDeviceMap.contains(id)) {
        result = true;
        descr.append(QString(rtaInputDeviceMap[id]));
    }
    return result;
}

int QsAudio ::getDefaultOutputDevice() { return p_rta->getDefaultOutputDevice(); }

int QsAudio ::getDefaultInputDevice() { return p_rta->getDefaultInputDevice(); }
