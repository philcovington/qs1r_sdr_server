#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>

class Thread {
  public:
    Thread() : m_thread(), m_stopFlag(false) {}

    // Delete copy constructor and assignment operator
    Thread(const Thread &) = delete;
    Thread &operator=(const Thread &) = delete;

    // Allow move constructor and assignment
    Thread(Thread &&other) noexcept : m_thread(std::move(other.m_thread)), m_stopFlag(other.m_stopFlag.load()) {
        other.m_stopFlag = true; // Set the moved-from object's stop flag
    }

    Thread &operator=(Thread &&other) noexcept {
        if (this != &other) {
            stop(); // Ensure current thread is stopped before assignment
            wait(); // Wait for the current thread to finish
            m_thread = std::move(other.m_thread);
            m_stopFlag.store(other.m_stopFlag.load()); // Move the atomic flag
            other.m_stopFlag = true;                   // Avoid joining the moved-from thread
        }
        return *this;
    }

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
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_stopFlag = true;
                m_cv.notify_all();
            }
        });

        // Set thread priority based on the provided priority
        setThreadPriority(priority);
    }

    // Make run virtual to allow overriding
    virtual void run() = 0; // Pure virtual function

    // Wait for the thread to finish execution with an optional timeout
    void wait(std::optional<std::chrono::milliseconds> timeout = std::nullopt) {
        if (m_thread.joinable()) {
            if (timeout.has_value()) {
                std::unique_lock<std::mutex> lock(m_mutex);
                // Capture m_stopFlag by reference in the lambda
                if (m_cv.wait_for(lock, timeout.value(), [this]() { return m_stopFlag.load(); })) {
                    // Thread finished within the timeout
                    m_thread.join();
                } else {
                    // Timeout occurred
                    throw std::runtime_error("Thread wait timed out.");
                }
            } else {
                // No timeout provided, just join
                m_thread.join();
            }
        }
    }

    // Check if the thread is running
    bool isRunning() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return !m_stopFlag;
    }

    virtual void stop() { // Make this virtual
        m_stopFlag = true;
        m_cv.notify_all(); // Notify any waiting threads
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
