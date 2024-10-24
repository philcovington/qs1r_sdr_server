/*
 * File: qs_datareader.hpp
 * Brief: Data reader class for managing and processing QS1R input data streams.
 *
 * This header defines the `QsDataReader` class, which is responsible for reading
 * and processing data from the QS1R device. It manages input buffers, thread control,
 * and error handling during data acquisition. The class includes methods to start,
 * stop, and reinitialize the data reader as well as manage buffers.
 *
 * Features:
 * - Handles multithreaded data acquisition using internal control flags.
 * - Manages input and output buffers for interleaved and complex data.
 * - Provides methods to clear and reinitialize internal states.
 * - Supports error handling for QS1R read failures.
 *
 * Usage:
 * Create an instance of `QsDataReader` to manage the data acquisition process.
 * Use the `start()` and `stop()` methods to control the acquisition process,
 * and `clearBuffers()` to reset the internal buffers.
 *
 * Notes:
 * - The class internally uses a thread control mechanism (`m_thread_go`) for
 *   starting and stopping the data acquisition loop.
 * - `onQs1rReadFail()` handles errors during data reading from the QS1R.
 *
 * Author: Philip A Covington
 * Date: 2024-10-16
 */

#pragma once

#include "../include/qs_sleep.hpp"
#include "../include/qs_types.hpp"
#include <atomic>
#include <thread>

class QsDataReader {
  public:
    QsDataReader();
    ~QsDataReader();

    void start();        // Method to start the data reader thread
    void stop();         // Method to stop the data reader thread
    void clearBuffers(); // Method to clear the buffers
    void reinit();       // Method to reinitialize the data reader
    void init();         // Method to initialize the data reader
    bool isRunning();

  private:
    void run();            // Method containing the main logic for the thread
    void onQs1rReadFail(); // Method for handling failure

    // Thread control flags
    std::atomic<bool> m_thread_go;
    std::atomic<bool> m_is_running;
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

    QsSleep sleep;

    // The thread object
    std::thread m_thread;
};
