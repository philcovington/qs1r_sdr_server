#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <thread>

class Thread {
  public:
    Thread() : m_thread(), m_stopFlag(false) {}

    // Start the thread with a callable (function or lambda)
    template <typename Callable> void start(Callable &&func) {
        m_thread = std::thread([this, func]() {
            func();
            // Signal that the thread has finished executing
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stopFlag = true;
            m_cv.notify_all();
        });
    }

    // Wait for the thread to finish execution
    void wait() {
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    // Check if the thread is running
    bool isRunning() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return !m_stopFlag;
    }

    // Stop the thread
    void stop() {
        m_stopFlag = true;
        m_cv.notify_all(); // Notify any waiting thread
    }

    ~Thread() {
        stop(); // Ensure thread is stopped before destruction
        wait(); // Wait for the thread to finish
    }

  private:
    std::thread m_thread;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_stopFlag; // Flag to indicate if the thread should stop
};
