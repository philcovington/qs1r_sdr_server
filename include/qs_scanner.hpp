#pragma once

#include "../include/qs_sleep.hpp"
#include "../include/qs_types.hpp"
#include <atomic>
#include <thread>
#include <vector>
#include <unordered_map>
#include <string>

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

	std::unordered_map<int, std::string> m_frequencies;

	bool m_is_init = false;
	int m_holdTime = 2000;
	int m_settleTime = 100;

    QsSleep sleep;

    // The thread object
    std::thread m_thread;
};
