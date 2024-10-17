#include "../headers/qs_dsp_proc.hpp"
#include "../headers/debugloggerclass.h"
#include "../headers/qs_sleep.h"
#include "../headers/qs_agc.h"
#include "../headers/qs_am_demod.h"
#include "../headers/qs_auto_notch_filter.hpp"
#include "../headers/qs_avg_nb.hpp"
#include "../headers/qs_blk_nb.hpp"
#include "../headers/qs_cpx_vector_cb.h"
#include "../headers/qs_dataproc.h"
#include "../headers/qs_fmn_demod.h"
#include "../headers/qs_fmw_demod.h"
#include "../headers/qs_iir_filter.hpp"
#include "../headers/qs_io_libusb.h"
#include "../headers/qs_main_rx_filter.hpp"
#include "../headers/qs_nr_filter.hpp"
#include "../headers/qs_post_rx_filter.hpp"
#include "../headers/qs_sam_demod.h"
#include "../headers/qs_smeter.h"
#include "../headers/qs_squelch.h"
#include "../headers/qs_state.h"
#include "../headers/qs_tone_gen.h"
#include "../headers/qs_volume.h"
#include "../headers/qs_downcnv.h"
#include <cmath>

QsDspProcessor::QsDspProcessor()
    : m_rx_num(0), m_bsize(0), m_bsizeX2(0), m_sd_buffer_size(0), m_ps_size(0), m_req_outframes(0), m_outframesX2(0),
      m_thread_go(false), m_is_running(false), m_dac_bypass(false), m_rt_audio_bypass(false), m_processing_rate(0),
      m_post_processing_rate(0), m_rs_rate(0), m_rs_quality(4), p_tg0(new QsToneGenerator()),
      p_anb(new QsAveragingNoiseBlanker()), p_bnb(new QsBlockNoiseBlanker()), p_downconv(new QsDownConvertor()),
      p_tg1(new QsToneGenerator()), p_agc(new QsAgc()), p_main_filter(new QsMainRxFilter()),
      p_post_filter(new QsPostRxFilter()), p_am(new QsAMDemodulator()), p_sam(new QsSAMDemodulator()),
      p_fmn(new QsFMNDemodulator()), p_fmw(new QsFMWDemodulator()), p_nr(new QsNoiseReductionFilter()),
      p_anf(new QsAutoNotchFilter()), p_sm(new QsSMeter()), p_sq(new QsSquelch()), p_vol(new QsVolume()),
      p_iir0(new QS_IIR()), p_iir1(new QS_IIR()), p_iir2(new QS_IIR()), p_iir3(new QS_IIR()), p_iir4(new QS_IIR()),
      p_iir5(new QS_IIR()), p_iir6(new QS_IIR()), p_iir7(new QS_IIR()), resampler(NULL), m_rs_output_rate(0),
      m_rs_input_rate(0) { QsSleep sleep; }

QsDspProcessor::~QsDspProcessor() {
    if (resampler != NULL)
        speex_resampler_destroy(resampler);
}

void QsDspProcessor::init(int rx_num) {
    m_rx_num = rx_num;
    m_bsize = QsGlobal::g_memory->getReadBlockSize();
    m_bsizeX2 = m_bsize * 2;
    m_ps_size = m_bsize;
    m_sd_buffer_size = m_bsize * SD_RING_SZ_MULT;
    m_processing_rate = QsGlobal::g_memory->getDataProcRate();
    m_post_processing_rate = QsGlobal::g_memory->getDataPostProcRate();

    m_dac_bypass = QsGlobal::g_memory->getDacBypass();
    m_rt_audio_bypass = QsGlobal::g_memory->getRtAudioBypass();

    m_rs_rate = QsGlobal::g_memory->getResamplerRate();
    m_rs_quality = QsGlobal::g_memory->getResamplerQuality();

    QsGlobal::g_cpx_sd_ring->init(m_sd_buffer_size);
    QsGlobal::g_cpx_ps1_ring->init(m_ps_size * 16);

    in_cpx.resize(m_bsize);
    QsDataProc::Zero(in_cpx);
    rs_cpx.resize(m_bsize);
    QsDataProc::Zero(rs_cpx);
    re_f.resize(m_bsize);
    QsDataProc::Zero(re_f);
    im_f.resize(m_bsize);
    QsDataProc::Zero(im_f);
    rs_cpx_n.resize(m_bsize);
    QsDataProc::Zero(rs_cpx_n);

    m_rs_input_rate = QsGlobal::g_memory->getDataPostProcRate();
    m_rs_output_rate = m_rs_rate;

    m_req_outframes = std::ceil((double)m_bsize * m_rs_output_rate / m_rs_input_rate);
    m_outframesX2 = m_req_outframes * 2;

    QsGlobal::g_float_rt_ring->init(m_outframesX2 * RT_RING_SZ_MULT);
    QsGlobal::g_float_dac_ring->init(m_outframesX2 * DAC_RING_SZ_MULT);
    QsGlobal::g_cpx_ps2_ring->init(m_bsize * 2);

    // ANB
    p_anb->init();

    // BNB
    p_bnb->init();

    // TONE GEN
    p_tg0->init(QsToneGenerator::rateDataRate);

    // Downconvertor
    p_downconv->setRate(m_processing_rate, 20000.00);

    // SM
    p_sm->init();

    // SQUELCH
    p_sq->init();

    // AGC
    p_agc->init();

    // DEMOD
    p_am->init();
    p_sam->init();
    p_fmn->init();
    p_fmw->init();

    // POST FILTER
    p_post_filter->init(m_bsize);

    // MAIN FIR
    p_main_filter->init(m_bsize);

    // ANF
    p_anf->init(m_bsize);

    // NR
    p_nr->init(m_bsize);

    // RESAMPLER
    initResampler(m_bsize);

    // CW TONE GEN
    p_tg1->init(QsToneGenerator::ratePostDataRate);

    // Instantiate 8 manual notch filters
    p_iir0->init(1, QS_IIR::iirBandReject);
    p_iir1->init(2, QS_IIR::iirBandReject);
    p_iir2->init(3, QS_IIR::iirBandReject);
    p_iir3->init(4, QS_IIR::iirBandReject);
    p_iir4->init(5, QS_IIR::iirBandReject);
    p_iir5->init(6, QS_IIR::iirBandReject);
    p_iir6->init(7, QS_IIR::iirBandReject);
    p_iir7->init(8, QS_IIR::iirBandReject);
}

void QsDspProcessor::reinit() { init(m_rx_num); }

void QsDspProcessor::run() {
    unsigned int sz = 0;
    unsigned int outframes = 0;

    m_sd_buffer_size = m_bsize * SD_RING_SZ_MULT;
    m_ps_size = m_bsize;

    QsDataProc::Zero(in_cpx);
    QsDataProc::Zero(rs_cpx);

    QsGlobal::g_cpx_sd_ring->init(m_sd_buffer_size);
    QsGlobal::g_cpx_ps1_ring->init(m_ps_size * 8);

    QsGlobal::g_cpx_sd_ring->empty();
    QsGlobal::g_cpx_ps1_ring->empty();

    int dstlen = 0;

    m_rs_input_rate = QsGlobal::g_memory->getDataPostProcRate();
    m_rs_output_rate = m_rs_rate;

    m_req_outframes = std::ceil((double)m_bsize * m_rs_output_rate / m_rs_input_rate);
    m_outframesX2 = m_req_outframes * 2;

    QsDataProc::Zero(re_f);
    QsDataProc::Zero(im_f);
    QsDataProc::Zero(rs_cpx_n);

    QsGlobal::g_float_rt_ring->init(m_outframesX2 * RT_RING_SZ_MULT);
    QsGlobal::g_float_dac_ring->init(m_outframesX2 * DAC_RING_SZ_MULT);
    QsGlobal::g_cpx_ps2_ring->init(m_bsize * 16);

    m_is_running = true;
    m_thread_go = true;

    while (m_thread_go) {
        // read data from reader ring buffer
        while (QsGlobal::g_cpx_readin_ring->readAvail() >= m_bsize) {
            QsGlobal::g_cpx_readin_ring->read(in_cpx);

            // Do noiseblankers
            // ======== <AVERAGING NOISE BLANKER> ===========
            p_anb->process(in_cpx);
            // ======== </AVERAGING NOISE BLANKER> ===========

            // ======== <BLOCK NOISE BLANKER> ===========
            p_bnb->process(in_cpx);
            // ======== </BLOCK NOISE BLANKER> ===========

            // power spectrum
            // =================<POWER SPECTRUM>==================
            if (QsGlobal::g_cpx_ps1_ring->writeAvail() >= m_bsize) {
                QsGlobal::g_cpx_ps1_ring->write(in_cpx);
            }
            // =================</POWER SPECTRUM>==================

            // apply LO
            // ======== <TONE GENERATOR> ===========
            p_tg0->process(in_cpx);
            // ======== </TONE GENERATOR> ===========

            // DOWNSAMPLER
            dstlen = p_downconv->process(&in_cpx[0], &rs_cpx[0], m_bsize);
            QsGlobal::g_cpx_sd_ring->write(rs_cpx, dstlen);
        }

        while (QsGlobal::g_cpx_sd_ring->readAvail() >= m_bsize) {
            // read data from integer resample buffer
            QsGlobal::g_cpx_sd_ring->read(rs_cpx_n, m_bsize);

            // main filter
            // ======== <MAIN FIR> ========
            p_main_filter->process(rs_cpx_n);
            // ======== </MAIN FIR> ========

            p_iir0->process(rs_cpx_n);
            p_iir1->process(rs_cpx_n);
            p_iir2->process(rs_cpx_n);
            p_iir3->process(rs_cpx_n);
            p_iir4->process(rs_cpx_n);
            p_iir5->process(rs_cpx_n);
            p_iir6->process(rs_cpx_n);
            p_iir7->process(rs_cpx_n);

            // =================<POWER SPECTRUM>==================
            if (QsGlobal::g_cpx_ps2_ring->writeAvail() >= m_bsize) {
                QsGlobal::g_cpx_ps2_ring->write(rs_cpx_n, m_bsize);
            }
            // =================</POWER SPECTRUM>==================

            if (QsGlobal::g_memory->getDemodMode() == dmCW) {
                // ======== <CW TONE GENERATOR> ===========
                p_tg1->process(rs_cpx_n);
                // ======== </CW TONE GENERATOR> ===========
            }

            // process through s meter
            // ======== <S METER> ===========
            p_sm->process(rs_cpx_n);
            // ======== </S METER> ===========

            // Do AGC
            p_agc->process(rs_cpx_n);

            QsDataProc::Limit(rs_cpx_n, m_bsize);

            // ======== <DEMODULATORS> ===========

            switch (QsGlobal::g_memory->getDemodMode()) {
            case dmAM:
                p_am->process(rs_cpx_n);
                p_post_filter->process(rs_cpx_n);
                break;
            case dmSAM:
                p_sam->process(rs_cpx_n);
                p_post_filter->process(rs_cpx_n);
                break;
            case dmFMN:
                p_fmn->process(rs_cpx_n);
                p_post_filter->process(rs_cpx_n);
                break;
            case dmFMW:
                p_fmw->process(rs_cpx_n);
                p_post_filter->process(rs_cpx_n);
                break;
            default:
                break;
            }

            // ======== </DEMODULATORS> ===========

            // ======== <BINAURAL> =============
            if (!QsGlobal::g_memory->getBinauralMode()) {
                QsDataProc::CopyRealToImag(rs_cpx_n);
            }
            // ======== </BINAURAL> =============

            // ======== <AUTO NOTCH FILTER> =============
            p_anf->process(rs_cpx_n);
            // ======== </AUTO NOTCH FILTER> =============

            // ======== <NOISE REDUCTION FILTER> =============
            p_nr->process(rs_cpx_n);
            // ======== </NOISE REDUCTION FILTER> =============

            // ======== <SQUELCH> ===========
            p_sq->process(rs_cpx_n);
            // ======== </SQUELCH> ===========

            QsDataProc::Interleave(rs_cpx_n, rs_in_interleaved, m_bsize);

            // do fractional resampler to port audio rate
            // ======== <RESAMPLER> ==========
            sz = m_bsize;
            outframes = m_req_outframes;
            speex_resampler_process_interleaved_float(resampler, &rs_in_interleaved[0], &sz, &rs_out_interleaved[0],
                                                      &outframes);
            m_outframesX2 = outframes * 2;

            // ======== </RESAMPLER> ==========

            // volume
            // ======== <VOLUME WITH LIMITER> ===========
            p_vol->process(rs_out_interleaved);
            // ======== </VOLUME WITH LIMITER> ===========

            if (!m_rt_audio_bypass) {
                // write resampled data to audio ring
                if (QsGlobal::g_float_rt_ring->writeAvail() >= m_outframesX2) {
                    QsGlobal::g_float_rt_ring->write(rs_out_interleaved, m_outframesX2);
                }
            }

            if (!m_dac_bypass) {
                // write resampled data to dac ring
                if (QsGlobal::g_float_dac_ring->writeAvail() >= m_outframesX2) {
                    QsGlobal::g_float_dac_ring->write(rs_out_interleaved, m_outframesX2);
                }
            }
        }
        sleep.usleep(1);
    }

    m_is_running = false;
    _debug() << "dspproc thread stopped.";
}

void QsDspProcessor::stop() { m_thread_go = false; }

void QsDspProcessor::clearBuffers() {
    QsGlobal::g_cpx_sd_ring->empty();
    QsGlobal::g_cpx_ps1_ring->empty();
}

// RESAMPLER

void QsDspProcessor::initResampler(int size) {
    rs_in_interleaved.resize(size * 8);
    rs_out_interleaved.resize(size * 8);

    if (resampler != 0)
        speex_resampler_destroy(resampler);
    resampler = 0;
    int err = 0;
    resampler = speex_resampler_init(2, std::round(m_rs_input_rate), std::round(m_rs_output_rate), 3, &err);
    speex_resampler_skip_zeros(resampler);
    speex_resampler_set_input_stride(resampler, 2);
    speex_resampler_set_output_stride(resampler, 2);
}
