#include "../include/qs_datareader.hpp"
#include "../include/qs_cpx_vector_cb.hpp"
#include "../include/qs_debugloggerclass.hpp"
#include "../include/qs_globals.hpp"
#include "../include/qs_types.hpp"

QsDataReader::QsDataReader()
    : m_thread_go(false), m_is_running(false), m_qs1r_fail_emitted(false), m_result(-1), m_channels(1), m_bsize(0),
      m_bsizeX2(0), m_buffer_min_level(0), m_circbufsize(0), m_rec_center_freq(0), m_samplerate(50000.0) {}

QsDataReader::~QsDataReader() {
    stop(); // Ensure the thread is stopped before destruction
}

void QsDataReader::reinit() { init(); }

void QsDataReader::init() {
    m_bsize = QsGlobal::g_memory->getReadBlockSize();
    m_bsizeX2 = m_bsize * 2;

    m_buffer_min_level = m_bsize;
    m_circbufsize = m_bsize * CPX_RING_SZ_MULT;

    m_samplerate = QsGlobal::g_memory->getDataProcRate();

    in_interleaved_i.resize(m_bsizeX2);
    QsSignalOps::Zero(in_interleaved_i);
    in_interleaved_f.resize(m_bsizeX2);
    QsSignalOps::Zero(in_interleaved_f);
    in_re_f.resize(m_bsize);
    QsSignalOps::Zero(in_re_f);
    in_im_f.resize(m_bsize);
    QsSignalOps::Zero(in_im_f);
    cpx_out.resize(m_bsize);
    QsSignalOps::Zero(cpx_out);
}

void QsDataReader::run() {
    QsSignalOps::Zero(in_interleaved_i);
    QsSignalOps::Zero(in_interleaved_f);
    QsSignalOps::Zero(in_re_f);
    QsSignalOps::Zero(in_im_f);
    QsSignalOps::Zero(cpx_out);

    m_circbufsize = m_bsize * CPX_RING_SZ_MULT;
    QsGlobal::g_cpx_readin_ring->init(m_circbufsize);
    QsGlobal::g_cpx_readin_ring->empty();

    m_is_running = true;
    m_thread_go = true;
    m_qs1r_fail_emitted = false;

    while (m_thread_go) {
        if (QsGlobal::g_io->readEP6(reinterpret_cast<unsigned char *>(&in_interleaved_i[0]), m_bsizeX2 * sizeof(int)) ==
            -1) {
            if (!m_qs1r_fail_emitted) {
                _debug() << "QS1R read failed!";
                m_qs1r_fail_emitted = true;
            }
            // Return zeroed out buffers
            QsSignalOps::Zero(&in_interleaved_i[0], m_bsizeX2);
            m_thread_go = false;
            sleep.usleep(1000);
        }

        // Convert interleaved integers into floats
        QsSignalOps::Convert(&in_interleaved_i[0], &in_interleaved_f[0], m_bsizeX2);

        // Deinterleave into in_re_f and in_im_f
        if (!QsGlobal::g_swap_iq) {
            QsSignalOps::DeInterleave(&in_interleaved_f[0], &in_re_f[0], &in_im_f[0], m_bsize);
        } else {
            QsSignalOps::DeInterleave(&in_interleaved_f[0], &in_im_f[0], &in_re_f[0], m_bsize);
        }

        // Convert in_re_f and in_im_f to Complex
        QsSignalOps::RealToComplex(&in_re_f[0], &in_im_f[0], &cpx_out[0], m_bsize);

        QsGlobal::g_cpx_readin_ring->write(cpx_out, m_bsize);
    }

    m_is_running = false;
    _debug() << "DataReader thread stopped.";
}

void QsDataReader::stop() {
    m_thread_go = false;
    WC_NEED_MORE_DATA.wakeAll(); // Notify the condition variable in case of waiting threads
    Thread::stop();              // Stop the base class thread
}

void QsDataReader::clearBuffers() { QsGlobal::g_cpx_readin_ring->empty(); }
