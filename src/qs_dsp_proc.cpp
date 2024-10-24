#include "../include/qs_dsp_proc.hpp"

#include <cmath>

QsDspProcessor::QsDspProcessor()
    : m_rx_num(0), m_bsize(0), m_bsizeX2(0), m_sd_buffer_size(0), m_ps_size(0), m_req_outframes(0), m_outframesX2(0),
      m_thread_go(false), m_is_running(false), m_dac_bypass(false), m_rt_audio_bypass(false), m_processing_rate(0),
      m_post_processing_rate(0), m_rs_rate(0), m_rs_quality(4), p_tg0(std::make_unique<QsToneGenerator>()),
      p_anb(std::make_unique<QsAveragingNoiseBlanker>()), p_bnb(std::make_unique<QsBlockNoiseBlanker>()),
      p_downconv(std::make_unique<QsDownConvertor>()), p_tg1(std::make_unique<QsToneGenerator>()),
      p_agc(std::make_unique<QsAgc>()), p_main_filter(std::make_unique<QsMainRxFilter>()),
      p_post_filter(std::make_unique<QsPostRxFilter>()), p_am(std::make_unique<QsAMDemodulator>()),
      p_sam(std::make_unique<QsSAMDemodulator>()), p_fmn(std::make_unique<QsFMNDemodulator>()),
      p_fmw(std::make_unique<QsFMWDemodulator>()), p_nr(std::make_unique<QsNoiseReductionFilter>()),
      p_anf(std::make_unique<QsAutoNotchFilter>()), p_sm(std::make_unique<QsSMeter>()),
      p_sq(std::make_unique<QsSquelch>()), p_vol(std::make_unique<QsVolume>()), p_iir0(std::make_unique<QS_IIR>()),
      p_iir1(std::make_unique<QS_IIR>()), p_iir2(std::make_unique<QS_IIR>()), p_iir3(std::make_unique<QS_IIR>()),
      p_iir4(std::make_unique<QS_IIR>()), p_iir5(std::make_unique<QS_IIR>()), p_iir6(std::make_unique<QS_IIR>()),
      p_iir7(std::make_unique<QS_IIR>()), resampler(nullptr), m_rs_output_rate(0), m_rs_input_rate(0) {
    QsSleep sleep;
}

QsDspProcessor::~QsDspProcessor() {}

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

    in_cpx.resize(m_bsize);
    QsSignalOps::Zero(in_cpx);
    rs_cpx.resize(m_bsize);
    QsSignalOps::Zero(rs_cpx);
    re_f.resize(m_bsize);
    QsSignalOps::Zero(re_f);
    im_f.resize(m_bsize);
    QsSignalOps::Zero(im_f);
    rs_cpx_n.resize(m_bsize);
    QsSignalOps::Zero(rs_cpx_n);

    m_rs_input_rate = QsGlobal::g_memory->getDataPostProcRate();
    m_rs_output_rate = m_rs_rate;

    m_req_outframes = std::ceil((double)m_bsize * m_rs_output_rate / m_rs_input_rate);
    m_outframesX2 = m_req_outframes * 2;

    QsGlobal::g_float_rt_ring->init(m_outframesX2 * RT_RING_SZ_MULT);
    QsGlobal::g_float_dac_ring->init(m_outframesX2 * DAC_RING_SZ_MULT);

#ifdef __NOISE_BLANKERS__
    // ANB
    p_anb->init();

    // BNB
    p_bnb->init();
#endif

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

#ifdef __AUTO_NOTCH__
    // ANF
    p_anf->init(m_bsize);
#endif

    // NR
    p_nr->init(m_bsize);

    // RESAMPLER
    initResampler(m_bsize);

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
}

void QsDspProcessor::reinit() { init(m_rx_num); }

void QsDspProcessor::run() {
    size_t sz = 0;
    size_t outframes = 0;

    m_sd_buffer_size = m_bsize * SD_RING_SZ_MULT;
    m_ps_size = m_bsize;

    QsSignalOps::Zero(in_cpx);
    QsSignalOps::Zero(rs_cpx);

    QsGlobal::g_cpx_sd_ring->init(m_sd_buffer_size);

    QsGlobal::g_cpx_sd_ring->empty();

    int dstlen = 0;

    m_rs_input_rate = QsGlobal::g_memory->getDataPostProcRate();
    m_rs_output_rate = m_rs_rate;

    m_req_outframes = std::ceil((double)m_bsize * m_rs_output_rate / m_rs_input_rate);
    m_outframesX2 = m_req_outframes * 2;

    QsSignalOps::Zero(re_f);
    QsSignalOps::Zero(im_f);
    QsSignalOps::Zero(rs_cpx_n);

    QsGlobal::g_float_rt_ring->init(m_outframesX2 * RT_RING_SZ_MULT);
    QsGlobal::g_float_dac_ring->init(m_outframesX2 * DAC_RING_SZ_MULT);

    m_is_running = true;
    m_thread_go = true;

    while (m_thread_go) {
        
        // read data from reader ring buffer
        while (QsGlobal::g_cpx_readin_ring->readAvail() >= m_bsize & m_thread_go == true) {
            
            QsGlobal::g_cpx_readin_ring->read(in_cpx);

#ifdef __NOISE_BLANKERS__            
            // Do noiseblankers
            // ======== <AVERAGING NOISE BLANKER> ===========
            p_anb->process(in_cpx);
            // ======== </AVERAGING NOISE BLANKER> ===========

            // ======== <BLOCK NOISE BLANKER> ===========
            p_bnb->process(in_cpx);
            // ======== </BLOCK NOISE BLANKER> ===========
#endif
            // apply LO
            // ======== <TONE GENERATOR> ===========
            p_tg0->process(in_cpx);
            // ======== </TONE GENERATOR> ===========

            // DOWNSAMPLER
            dstlen = p_downconv->process(&in_cpx[0], &rs_cpx[0], m_bsize);
            QsGlobal::g_cpx_sd_ring->write(rs_cpx, dstlen);
        }
       
        while (QsGlobal::g_cpx_sd_ring->readAvail() >= m_bsize & m_thread_go == true) {
            // read data from integer resample buffer
            QsGlobal::g_cpx_sd_ring->read(rs_cpx_n, m_bsize);

            // main filter
            // ======== <MAIN FIR> ========
            p_main_filter->process(rs_cpx_n);
            // ======== </MAIN FIR> ========

#ifdef __IIR_NOTCH__
            p_iir0->process(rs_cpx_n);
            p_iir1->process(rs_cpx_n);
            p_iir2->process(rs_cpx_n);
            p_iir3->process(rs_cpx_n);
            p_iir4->process(rs_cpx_n);
            p_iir5->process(rs_cpx_n);
            p_iir6->process(rs_cpx_n);
            p_iir7->process(rs_cpx_n);
#endif

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

            QsSignalOps::Limit(rs_cpx_n, m_bsize);

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

#ifdef __BINAURAL__
            // ======== <BINAURAL> =============
            if (!QsGlobal::g_memory->getBinauralMode()) {
                QsSignalOps::CopyRealToImag(rs_cpx_n);
            }
            // ======== </BINAURAL> =============
#endif
#ifdef __AUTO_NOTCH__
            // ======== <AUTO NOTCH FILTER> =============
            p_anf->process(rs_cpx_n);
            // ======== </AUTO NOTCH FILTER> =============
#endif
            // ======== <NOISE REDUCTION FILTER> =============
            p_nr->process(rs_cpx_n);
            // ======== </NOISE REDUCTION FILTER> =============

            // ======== <SQUELCH> ===========
            p_sq->process(rs_cpx_n);
            // ======== </SQUELCH> ===========

            QsSignalOps::Interleave(rs_cpx_n, rs_in_interleaved, m_bsize);

            // do fractional resampler to port audio rate
            // ======== <RESAMPLER> ==========
            sz = m_bsize;
            outframes = m_req_outframes;
            resampler->process(&rs_in_interleaved[0], sz, &rs_out_interleaved[0], &outframes);
            m_outframesX2 = outframes * 2;

            // ======== </RESAMPLER> ==========

            // volume
            // ======== <VOLUME WITH LIMITER> ===========
            p_vol->process(rs_out_interleaved);
            // ======== </VOLUME WITH LIMITER> ===========

#ifdef __SOUND_OUT__
            if (QsGlobal::g_float_rt_ring->writeAvail() >= m_outframesX2) {
                    QsGlobal::g_float_rt_ring->write(rs_out_interleaved, m_outframesX2);
            }            
#endif
#ifdef __DAC_OUT__
            if (QsGlobal::g_float_dac_ring->writeAvail() >= m_outframesX2) {
                    QsGlobal::g_float_dac_ring->write(rs_out_interleaved, m_outframesX2);
            }            
#endif
        }
        sleep.usleep(1);
    }
    m_is_running = false;
    _debug() << "dspproc thread stopped.";
}

void QsDspProcessor::start() {
    // Start the thread only if it isn't already running
    if (!m_is_running && !m_thread_go) {
        m_thread_go = true;
        m_thread = std::thread(&QsDspProcessor::run, this); // Launch the run() method in a new thread
    }
}

void QsDspProcessor::stop() {
    m_thread_go = false; // Signal the thread to stop
    if (m_thread.joinable()) {
        m_thread.join(); // Wait for the thread to finish
    }
}

bool QsDspProcessor::isRunning() { return m_thread_go; }

void QsDspProcessor::clearBuffers() { QsGlobal::g_cpx_sd_ring->empty(); }

// RESAMPLER

void QsDspProcessor::initResampler(int size) {
    rs_in_interleaved.resize(size * 8);
    rs_out_interleaved.resize(size * 8);
    resampler = std::make_unique<Resampler>(std::round(m_rs_input_rate), std::round(m_rs_output_rate));
}
