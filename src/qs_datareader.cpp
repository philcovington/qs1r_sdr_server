
#include "../include/qs_datareader.hpp"
#include "../include/qs_debugloggerclass.hpp"
#include "../include/qs_cpx_vector_cb.hpp"
#include "../include/qs_globals.hpp"
#include "../include/qs_types.hpp"

QsDataReader::QsDataReader()
    : m_thread_go(false), m_is_running(false), m_qs1r_fail_emitted(false), m_result(-1), m_channels(1), m_bsize(0),
      m_bsizeX2(0), m_buffer_min_level(0), m_circbufsize(0), m_rec_center_freq(0), m_samplerate(50000.0) {}

QsDataReader::~QsDataReader() {}

void QsDataReader::reinit() { init(); }

void QsDataReader::init() {
    m_bsize = QsGlobal::g_memory->getReadBlockSize();
    m_bsizeX2 = m_bsize * 2;

    m_buffer_min_level = m_bsize;
    m_circbufsize = m_bsize * CPX_RING_SZ_MULT;

    m_samplerate = QsGlobal::g_memory->getDataProcRate();
   
    in_interleaved_i.resize(m_bsizeX2);
    QsDataProc::Zero(in_interleaved_i);
    in_interleaved_f.resize(m_bsizeX2);
    QsDataProc::Zero(in_interleaved_f);
    in_re_f.resize(m_bsize);
    QsDataProc::Zero(in_re_f);
    in_im_f.resize(m_bsize);
    QsDataProc::Zero(in_im_f);
    cpx_out.resize(m_bsize);
    QsDataProc::Zero(cpx_out);
}

void QsDataReader::run() {

    QsDataProc::Zero(in_interleaved_i);
    QsDataProc::Zero(in_interleaved_f);
    QsDataProc::Zero(in_re_f);
    QsDataProc::Zero(in_im_f);
    QsDataProc::Zero(cpx_out);

    m_circbufsize = m_bsize * CPX_RING_SZ_MULT;
    QsGlobal::g_cpx_readin_ring->init(m_circbufsize);
    QsGlobal::g_cpx_readin_ring->empty();

    m_is_running = true;
    m_thread_go = true;
    m_qs1r_fail_emitted = false;

    while (m_thread_go) {
        if (QsGlobal::g_io->readEP6(reinterpret_cast<unsigned char *>(&in_interleaved_i[0]), m_bsizeX2 * sizeof(int)) ==
            -1) {

            if (m_qs1r_fail_emitted) {
                _debug() << "QS1R read failed!";
                m_qs1r_fail_emitted = true;
            }
            // return zeroed out buffers
            QsDataProc::Zero(&in_interleaved_i[0], m_bsizeX2);
            m_thread_go = false;
            sleep.usleep(1000);
        }

        // convert interleaved integers into floats
        QsDataProc::Convert(&in_interleaved_i[0], &in_interleaved_f[0], m_bsizeX2);

        // deinterleave into in_re_f and in_im_f
        if (!QsGlobal::g_swap_iq) {
            QsDataProc::DeInterleave(&in_interleaved_f[0], &in_re_f[0], &in_im_f[0], m_bsize);
        } else {
            QsDataProc::DeInterleave(&in_interleaved_f[0], &in_im_f[0], &in_re_f[0], m_bsize);
        }

        // Convert in_re_f and in_im_f to Complex
        QsDataProc::RealToComplex(&in_re_f[0], &in_im_f[0], &cpx_out[0], m_bsize);

        QsGlobal::g_cpx_readin_ring->write(cpx_out, m_bsize);
    }

    m_is_running = false;
    _debug() << "datareader thread stopped.";
}

void QsDataReader::stop() {
    m_thread_go = false;
    WC_NEED_MORE_DATA.wakeAll();
}

void QsDataReader::clearBuffers() { QsGlobal::g_cpx_readin_ring->empty(); }
