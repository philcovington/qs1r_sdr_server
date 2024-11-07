#include "../include/qs_dsp_proc.hpp"

#include "../include/qs_agc.hpp"
#include "../include/qs_am_demod.hpp"
#include "../include/qs_auto_notch_filter.hpp"
#include "../include/qs_avg_nb.hpp"
#include "../include/qs_blk_nb.hpp"
#include "../include/qs_de_emphasis.hpp"
#include "../include/qs_debugloggerclass.hpp"
#include "../include/qs_defines.hpp"
#include "../include/qs_downcnv.hpp"
#include "../include/qs_fm_demod.hpp"
#include "../include/qs_globals.hpp"
#include "../include/qs_iir_filter.hpp"
#include "../include/qs_io_libusb.hpp"
#include "../include/qs_main_rx_filter.hpp"
#include "../include/qs_nr_filter.hpp"
#include "../include/qs_post_rx_filter.hpp"
#include "../include/qs_resampler.hpp"
#include "../include/qs_sam_demod.hpp"
#include "../include/qs_signalops.hpp"
#include "../include/qs_sleep.hpp"
#include "../include/qs_smeter.hpp"
#include "../include/qs_squelch.hpp"
#include "../include/qs_state.hpp"
#include "../include/qs_test_tone.hpp"
#include "../include/qs_threading.hpp"
#include "../include/qs_tone_gen.hpp"
#include "../include/qs_volume.hpp"
#include <cmath>
#include <pthread.h>
#include <sched.h>

QsDspProcessor::QsDspProcessor()
    : m_rx_num(0), m_bsize(2048), m_bsizeX2(4096), m_thread_go(false), m_is_running(false), m_processing_rate(0) {
    QsSleep sleep;
}

QsDspProcessor::~QsDspProcessor() {}

void QsDspProcessor::init(int rx_num) {
    _debug() << "QsDSPProessor init begin...";
    p_tg0 = std::make_unique<QsToneGenerator>();
    p_anb = std::make_unique<QsAveragingNoiseBlanker>();
    p_bnb = std::make_unique<QsBlockNoiseBlanker>();
    p_tg1 = std::make_unique<QsToneGenerator>();
    p_tg_test = std::make_unique<QsToneGenerator>();
    p_agc = std::make_unique<QsAgc>();
    p_main_filter = std::make_unique<QsMainRxFilter>();
    p_post_filter = std::make_unique<QsMainRxFilter>();
    p_am = std::make_unique<QsAMDemodulator>();
    p_sam = std::make_unique<QsSAMDemodulator>();
    p_fm = std::make_unique<QsFMCombinedDemodulator>();
    p_fm_demph = std::make_unique<DeEmphasis>();
    p_nr = std::make_unique<QsNoiseReductionFilter>();
    p_anf = std::make_unique<QsAutoNotchFilter>();
    p_sm = std::make_unique<QsSMeter>();
    p_sq = std::make_unique<QsSquelch>();
    p_vol = std::make_unique<QsVolume>();
    p_iir0 = std::make_unique<QS_IIR>();
    p_iir1 = std::make_unique<QS_IIR>();
    p_iir2 = std::make_unique<QS_IIR>();
    p_iir3 = std::make_unique<QS_IIR>();
    p_iir4 = std::make_unique<QS_IIR>();
    p_iir5 = std::make_unique<QS_IIR>();
    p_iir6 = std::make_unique<QS_IIR>();
    p_iir7 = std::make_unique<QS_IIR>();    
    p_test_tone = std::make_unique<QsTestTone>();

    m_rx_num = rx_num;
    m_bsize = QsGlobal::g_memory->getReadBlockSize();
    m_bsizeX2 = m_bsize * 2;
    m_processing_rate = QsGlobal::g_memory->getDataProcRate();

    buf_cpx.resize(m_bsize);
    QsSignalOps::Zero(buf_cpx);
    re_f.resize(m_bsize);
    QsSignalOps::Zero(re_f);
    im_f.resize(m_bsize);
    QsSignalOps::Zero(im_f);

    in_re_f.resize(m_bsize);
    QsSignalOps::Zero(in_re_f);
    in_im_f.resize(m_bsize);
    QsSignalOps::Zero(in_im_f);

    in_interleaved_i.resize(m_bsizeX2);
    QsSignalOps::Zero(in_interleaved_i);
    in_interleaved_f.resize(m_bsizeX2);
    QsSignalOps::Zero(in_interleaved_f);
    out_interleaved_f.resize(m_bsizeX2);
    QsSignalOps::Zero(out_interleaved_f);
    rs_interleaved_f.resize(m_bsizeX2);
    QsSignalOps::Zero(rs_interleaved_f);

    out_s.resize(m_bsizeX2);
    QsSignalOps::Zero(out_s);

    m_req_outframes = std::ceil( (double)m_bsize * 48000/50000 );
    m_outframesX2 = m_req_outframes * 2;

    QsGlobal::g_float_rt_ring->init(m_outframesX2 * 4);
    QsGlobal::g_float_rt_ring->setBlockSize(m_outframesX2);
    QsGlobal::g_float_rt_ring->empty();

    p_rs = std::make_unique<Resampler>(50000, 48000);

    m_thread_go = false;

    // #ifdef __NOISE_BLANKERS__
    // ANB
    p_anb->init();

    // BNB
    p_bnb->init();
    // #endif

    // TONE GEN
    p_tg0->init(QsToneGenerator::rateDataRate);

    // SM
    p_sm->init();

    // SQUELCH
    p_sq->init(0.7, 0.7, CtcssTone::TONE_NONE);

    // AGC
    p_agc->init();

    // DEMOD
    p_am->init();
    p_sam->init();
    p_fm->init(NARROW);
    p_fm_demph->init(m_processing_rate);

    // POST FILTER
    p_post_filter->init(m_bsize);

    // MAIN FIR
    p_main_filter->init(m_bsize);

    // ANF
    p_anf->init(m_bsize);

    // NR
    p_nr->init(m_bsize);

    // CW TONE GEN
    p_tg1->init(QsToneGenerator::ratePostDataRate);

#ifdef __IIR_NOTCH__
    // Instantiate 8 manual notch filters
    p_iir0->init(1, QS_IIR::iirBandReject);
    p_iir1->init(2, QS_IIR::iirBandReject);
    p_iir2->init(3, QS_IIR::iirBandReject);
    p_iir3->init(4, QS_IIR::iirBandReject);
    p_iir4->init(5, QS_IIR::iirBandReject);
    p_iir5->init(6, QS_IIR::iirBandReject);
    p_iir6->init(7, QS_IIR::iirBandReject);
    p_iir7->init(8, QS_IIR::iirBandReject);
#endif

    // For testing
    p_test_tone->init(162.2, 0.75, m_processing_rate);
    _debug() << "QsDSPProcessor init end...";
}

void QsDspProcessor::reinit() { init(m_rx_num); }

void QsDspProcessor::run() {
    _debug() << "QsDSPProcessor process begin...";
    m_thread_go = true;
    QsSignalOps::Zero(buf_cpx);
    QsSignalOps::Zero(in_interleaved_i);
    QsSignalOps::Zero(in_interleaved_f);
    QsSignalOps::Zero(out_interleaved_f);

    int dstlen = 0;

    QsSignalOps::Zero(re_f);
    QsSignalOps::Zero(im_f);

    QsSignalOps::Zero(in_re_f);
    QsSignalOps::Zero(in_im_f);

    QsSignalOps::Zero(out_s);    

    m_is_running = true;
    m_thread_go = true;

    while (m_thread_go) {
        if (QsGlobal::g_io->readEP6(reinterpret_cast<unsigned char *>(&in_interleaved_i[0]), m_bsizeX2 * sizeof(int)) >
            0) {

            // Convert interleaved integers into floats
            QsSignalOps::Convert(in_interleaved_i, in_interleaved_f, m_bsizeX2);

            // Deinterleave into in_re_f and in_im_f
            if (!QsGlobal::g_swap_iq) {
                QsSignalOps::DeInterleave(in_interleaved_f, in_re_f, in_im_f, m_bsize);
            } else {
                QsSignalOps::DeInterleave(in_interleaved_f, in_im_f, in_re_f, m_bsize);
            }

            // Convert in_re_f and in_im_f to Complex
            QsSignalOps::RealToComplex(in_re_f, in_im_f, buf_cpx, m_bsize);

            // #ifdef __NOISE_BLANKERS__
            // Do noiseblankers
            // ======== <AVERAGING NOISE BLANKER> ===========
            p_anb->process(buf_cpx);
            // ======== </AVERAGING NOISE BLANKER> ===========

            // ======== <BLOCK NOISE BLANKER> ===========
            p_bnb->process(buf_cpx);
            // ======== </BLOCK NOISE BLANKER> ===========
            // #endif
            // apply LO
            // ======== <TONE GENERATOR> ===========
            p_tg0->process(buf_cpx);
            // ======== </TONE GENERATOR> ===========

            // main filter
            // ======== <MAIN FIR> ========
            p_main_filter->process(buf_cpx);
            // ======== </MAIN FIR> ========

            // #ifdef __IIR_NOTCH__
            //             p_iir0->process(buf_cpx);
            //             p_iir1->process(buf_cpx);
            //             p_iir2->process(buf_cpx);
            //             p_iir3->process(buf_cpx);
            //             p_iir4->process(buf_cpx);
            //             p_iir5->process(buf_cpx);
            //             p_iir6->process(buf_cpx);
            //             p_iir7->process(buf_cpx);
            // #endif

            if (QsGlobal::g_memory->getDemodMode() == dmCW) {
                // ======== <CW TONE GENERATOR> ===========
                p_tg1->process(buf_cpx);
                // ======== </CW TONE GENERATOR> ===========
            }

            // process through s meter
            // ======== <S METER> ===========
            p_sm->process(buf_cpx);
            // ======== </S METER> ===========

            // Do AGC
            p_agc->process(buf_cpx);

            QsSignalOps::Limit(buf_cpx, m_bsize);

            // ======== <DEMODULATORS> ===========

            switch (QsGlobal::g_memory->getDemodMode()) {
            case dmAM:
                p_am->process(buf_cpx);
                // p_post_filter->process(buf_cpx);
                break;
            case dmSAM:
                p_sam->process(buf_cpx);
                // p_post_filter->process(buf_cpx);
                break;
            case dmFMN:
                p_fm->process(buf_cpx, NARROW);
                p_fm_demph->process(buf_cpx);
                // p_post_filter->process(buf_cpx);
                break;
            case dmFMW:
                p_fm->process(buf_cpx, WIDE);
                p_fm_demph->process(buf_cpx);
                // p_post_filter->process(buf_cpx);
                break;
            default:
                break;
            }

            // ======== </DEMODULATORS> ===========

            // ======== <BINAURAL> =============
            if (!QsGlobal::g_memory->getBinauralMode()) {
                QsSignalOps::CopyRealToImag(buf_cpx);
            }
            // ======== </BINAURAL> =============

            // ======== <AUTO NOTCH FILTER> =============
            p_anf->process(buf_cpx);
            // ======== </AUTO NOTCH FILTER> =============

            // ======== <NOISE REDUCTION FILTER> =============
            p_nr->process(buf_cpx);
            // ======== </NOISE REDUCTION FILTER> =============

            // p_test_tone->process(buf_cpx);

            // ======== <SQUELCH> ===========
            p_sq->process(buf_cpx);
            // ======== </SQUELCH> ===========

            QsSignalOps::Interleave(buf_cpx, out_interleaved_f, m_bsize);

            // volume
            // ======== <VOLUME WITH LIMITER> ===========
            p_vol->process(out_interleaved_f);
            // ======== </VOLUME WITH LIMITER> ===========

            size_t out_frames = m_req_outframes;
            p_rs->process(&out_interleaved_f[0], m_bsize, &rs_interleaved_f[0], &out_frames);
            m_outframesX2 = out_frames * 2;

            if (QsGlobal::g_float_rt_ring->writeAvail() >= m_outframesX2) {
                QsGlobal::g_float_rt_ring->write(rs_interleaved_f, m_outframesX2);
            }

            // ======== <WRITE TO DAC> ===========
            QsSignalOps::Convert(out_interleaved_f, out_s, m_bsizeX2);
            int result =
                QsGlobal::g_io->writeEP2(reinterpret_cast<unsigned char *>(&out_s[0]), m_bsizeX2 * sizeof(short));
            if (result == -1) {
                // Failure handling
                sleep.msleep(100);
                _debug() << "Failed EP2 write.";
            }
            // ======== </WRITE TO DAC> ===========
        }
    }
    m_is_running = false;
    _debug() << "dspproc thread stopped.";
    _debug() << "QsDSPProcessor process end...";
}

void QsDspProcessor::start() {
    // Start the thread only if it isn't already running
    if (!m_is_running && !m_thread_go) {
        m_thread_go = true;
        m_thread = std::thread(&QsDspProcessor::run, this); // Launch the run() method in a new thread

        // Set the thread priority
        struct sched_param sch_params;
        sch_params.sched_priority = sched_get_priority_max(SCHED_FIFO); // Set priority (range depends on policy)

        pthread_t pthread = m_thread.native_handle();

        // Apply real-time scheduling policy (SCHED_FIFO, SCHED_RR)
        if (pthread_setschedparam(pthread, SCHED_FIFO, &sch_params)) {
            std::cerr << "Failed to set thread scheduling: " << strerror(errno) << '\n';
        }
    }
}

void QsDspProcessor::stop() {
    m_thread_go = false; // Signal the thread to stop
    if (m_thread.joinable()) {
        m_thread.join(); // Wait for the thread to finish
    }
}

bool QsDspProcessor::isRunning() { return m_thread_go; }

void QsDspProcessor::clearBuffers() {}
