// qs_datareader.h
#pragma once

#include <thread>

#include "../headers/qs_blockfifo.h"
#include "../headers/qs_globals.h"
#include "../headers/qs_types.h"

class QsDataReader {
   public:
    QsDataReader();
    ~QsDataReader();

    QsBlockFifo file_write_fifo;

    void run();
    void start();
    void stop();
    void clearBuffers();
    void reinit();
    void init();

   private:    

    void onQs1rReadFail();
    bool m_thread_go;
    bool m_is_running;
    bool m_qs1r_fail_emitted;

    int m_result;
    int m_channels;
    int m_bsize;
    int m_bsizeX2;
    int m_buffer_min_level;
    int m_circbufsize;

    double m_rec_center_freq;
    double m_samplerate;

    qs_vect_i in_interleaved_i;

    qs_vect_f in_interleaved_f;
    qs_vect_f in_re_f;
    qs_vect_f in_im_f;

    qs_vect_cpx cpx_out;
};