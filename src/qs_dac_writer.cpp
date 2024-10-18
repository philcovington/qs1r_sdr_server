#include "../include/qs_dac_writer.hpp"
#include "../include/qs1r_server.hpp"
#include "../include/qs_debugloggerclass.hpp"
#include "../include/qs_signalops.hpp"

QsDacWriter::QsDacWriter() : m_bsize(0), m_bsizeX2(0) {}

void QsDacWriter::init() {
    m_bsize = QsGlobal::g_memory->getDACBlockSize();
    m_bsizeX2 = m_bsize * 2;

    out_f.resize(m_bsizeX2);
    QsSignalOps::Zero(out_f);
    out_s.resize(m_bsizeX2);
    QsSignalOps::Zero(out_s);
}

void QsDacWriter::reinit() { init(); }

void QsDacWriter::run() {
    m_thread_go = true;

    QsSignalOps::Zero(out_f);
    QsSignalOps::Zero(out_s);

    while (m_thread_go) {
        if (QsGlobal::g_float_dac_ring->readAvail() >= m_bsizeX2) {
            QsGlobal::g_float_dac_ring->read(out_f, m_bsizeX2);
            QsSignalOps::Convert(out_f, out_s, m_bsizeX2);
        } else {
            QsSignalOps::Zero(out_s);
        }
        // AUDIO DAC takes 16 bit data per channel
        int result = QsGlobal::g_io->writeEP2(reinterpret_cast<unsigned char *>(&out_s[0]), m_bsizeX2 * sizeof(short));
        if (result == -1) {
            // failure
            sleep.msleep(100);
            _debug() << "Failed EP2 write.";
        }
    }

    m_thread_go = false;
    _debug() << "dacwriter thread stopped.";
}

void QsDacWriter::stop() {
    m_thread_go = false; // Set thread running flag to false
    Thread::stop();      // Call base class stop
}
