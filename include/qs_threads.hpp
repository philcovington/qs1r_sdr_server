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

    enum class ThreadPriority {
        Low,
        Normal,
        High,
        TimeCritical // Add additional priorities as needed
    };

    template <typename Callable> void start(Callable &&func, ThreadPriority priority = ThreadPriority::Normal) {
        // Create a thread with the provided function
        m_thread = std::thread([this, func]() {
            func();
            // Signal that the thread has finished executing
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stopFlag = true;
            m_cv.notify_all();
        });

        // Set thread priority based on the provided priority
        setThreadPriority(priority);
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

    void setThreadPriority(ThreadPriority priority) {
#ifdef _WIN32
        HANDLE handle = m_thread.native_handle();
        int winPriority;
        switch (priority) {
        case ThreadPriority::Low:
            winPriority = THREAD_PRIORITY_BELOW_NORMAL;
            break;
        case ThreadPriority::Normal:
            winPriority = THREAD_PRIORITY_NORMAL;
            break;
        case ThreadPriority::High:
            winPriority = THREAD_PRIORITY_ABOVE_NORMAL;
            break;
        case ThreadPriority::TimeCritical:
            winPriority = THREAD_PRIORITY_HIGHEST;
            break;
        }
        SetThreadPriority(handle, winPriority);
#else
        int posixPriority;
        switch (priority) {
        case ThreadPriority::Low:
            posixPriority = 19;
            break; // Low priority
        case ThreadPriority::Normal:
            posixPriority = 0;
            break; // Normal priority
        case ThreadPriority::High:
            posixPriority = -1;
            break; // High priority
        case ThreadPriority::TimeCritical:
            posixPriority = -2;
            break; // Time critical
        }
        pthread_t thread = m_thread.native_handle();
        struct sched_param params;
        params.sched_priority = posixPriority;
        pthread_setschedparam(thread, SCHED_FIFO, &params); // Adjust according to your scheduling policy
#endif
    }
};
