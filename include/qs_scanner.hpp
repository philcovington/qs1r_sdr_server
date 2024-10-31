#pragma once

#include "../include/qs_sleep.hpp"
#include "../include/qs_types.hpp"
#include <atomic>
#include <thread>
#include <vector>

class QsScanner {
  public:
    QsScanner();
    ~QsScanner();

    void start();        // Method to start the thread
    void stop();         // Method to stop the thread
    void reinit();       // Method to reinitialize the scanner
    void init();         // Method to initialize the scanner
    bool isRunning();

  private:
    void run();            // Method containing the main logic for the thread    

    // Thread control flags
    std::atomic<bool> m_thread_go;
    std::atomic<bool> m_is_running;

	std::vector<int> m_frequencies;

	bool m_is_init = false;

    QsSleep sleep;

    // The thread object
    std::thread m_thread;
};
