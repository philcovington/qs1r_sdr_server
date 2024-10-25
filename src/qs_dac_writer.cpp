#include "../include/qs_dac_writer.hpp"
#include "../include/qs1r_server.hpp"
#include "../include/qs_debugloggerclass.hpp"
#include "../include/qs_signalops.hpp"

QsDacWriter::QsDacWriter() : m_bsize(0), m_bsizeX2(0), m_thread_go(false), m_is_running(false) {}

void QsDacWriter::init(bool test_mode) {
    m_testMode = test_mode;
    m_bsize = QsGlobal::g_memory->getDACBlockSize();
    m_bsizeX2 = m_bsize * 2;

    out_f.resize(m_bsizeX2);
    QsSignalOps::Zero(out_f);
    out_s.resize(m_bsizeX2);
    QsSignalOps::Zero(out_s);

    if (m_testMode) {
        tone.init(QsToneGenerator::QSDSPPOS::rate48000);
        tone.setAmplitude(0.01f);
        tone.setFrequency(1000.0);
    }
}

void QsDacWriter::reinit() { init(); }

void QsDacWriter::start() {
    // Start the thread only if it isn't already running
    if (!m_is_running && !m_thread_go) {
        m_thread_go = true;
        m_thread = std::thread(&QsDacWriter::run, this); // Launch the run() method in a new thread
    }
}

void QsDacWriter::run() {
    m_is_running = true;

    QsSignalOps::Zero(out_f);
    QsSignalOps::Zero(out_s);

    while (m_thread_go) {
        if (m_testMode) {
            tone.process(out_f);
            QsSignalOps::Convert(out_f, out_s, m_bsizeX2);
        } else if (QsGlobal::g_float_dac_ring->readAvail() >= m_bsizeX2) {
            QsGlobal::g_float_dac_ring->read(out_f, m_bsizeX2);
            QsSignalOps::Convert(out_f, out_s, m_bsizeX2);
        } else {
            QsSignalOps::Zero(out_s);
        }

        // AUDIO DAC takes 16-bit data per channel
        int result = QsGlobal::g_io->writeEP2(reinterpret_cast<unsigned char *>(&out_s[0]), m_bsizeX2 * sizeof(short));
        if (result == -1) {
            // Failure handling
            sleep.msleep(100);
            _debug() << "Failed EP2 write.";
        }
    }

    m_is_running = false;
    _debug() << "DAC writer thread stopped.";
}

void QsDacWriter::stop() {
    m_thread_go = false; // Signal the thread to stop
    if (m_thread.joinable()) {
        m_thread.join(); // Wait for the thread to finish
    }
}

bool QsDacWriter::isRunning() { return m_thread_go; }

void QsDacWriter::setTestModeParams(float frequency, float amplitude, u_int samplerate) {
    if (m_testMode) {
        m_toneFrequency = frequency;
        m_toneAmplitude = amplitude;

        switch (samplerate) {
        case 24000:
            tone.init(QsToneGenerator::QSDSPPOS::rate24000);
            break;
        case 48000:
            tone.init(QsToneGenerator::QSDSPPOS::rate48000);
            break;
        case 50000:
            tone.init(QsToneGenerator::QSDSPPOS::rate50000);
            break;
        default:
            break;
        }

        tone.setAmplitude(m_toneAmplitude);
        tone.setFrequency(m_toneFrequency);
    }
}
