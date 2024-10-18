/**
 * @file    qs_dacwriter.hpp
 * @brief   Digital to Analog Converter (DAC) writer class.
 *
 * This class handles the process of writing audio data to the DAC using 
 * a separate thread for real-time processing. It supports initialization,
 * reinitialization, and control over starting and stopping the thread.
 *
 * Features:
 * - Initializes and reinitializes the DAC output.
 * - Provides real-time audio data processing through multi-threading.
 * - Manages buffer sizes for float and short output formats.
 * - Contains functionality to control the thread's execution state.
 *
 * Usage:
 * - Call `init()` to initialize the DAC writer.
 * - Call `run()` to start the audio output process.
 * - Use `stop()` to halt the process and clean up resources.
 *
 * Notes:
 * - The thread execution is controlled using the `m_thread_go` flag.
 * - `out_f` stores float data, and `out_s` stores short integer data.
 *
 * Author: Philip A Covington
 * Date: 2024-10-17
 */

#pragma once

#include "../include/qs_defines.hpp"
#include "../include/qs_globals.hpp"
#include "../include/qs_memory.hpp"
#include "../include/qs_threading.hpp"
#include "../include/qs_sleep.hpp"
#include <atomic>

class QsDacWriter : public Thread {
  public:
    QsDacWriter();

    void init();
    void reinit();

    void run() override; // Override the run method from the Thread class
    void stop() override; // Override the stop method from the Thread class

  private:

    std::atomic<bool> m_thread_go;
    std::atomic<bool> m_is_running;

    int m_bsize;
    int m_bsizeX2;    

    qs_vect_f out_f;
    qs_vect_s out_s;

	QsSleep sleep;
};
