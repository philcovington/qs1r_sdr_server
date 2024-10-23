#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
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
        other.m_stopFlag = true;
    }

    Thread &operator=(Thread &&other) noexcept {
        if (this != &other) {
            stop(); // Ensure current thread is stopped before assignment
            wait(); // Wait for the current thread to finish
            m_thread = std::move(other.m_thread);
            m_stopFlag.store(other.m_stopFlag.load()); // Move the atomic flag
            other.m_stopFlag = true;
        }
        return *this;
    }

    enum class ThreadPriority { Low, Normal, High, TimeCritical };

    // Start the thread, no need for template, just call the virtual run()
    void start(ThreadPriority priority = ThreadPriority::Normal) {
        m_thread = std::thread([this]() {
            this->run(); // Automatically call the subclass's run() method
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_stopFlag = true;
                m_cv.notify_all();
            }
        });

        setThreadPriority(priority);
    }

    // Pure virtual function to be implemented by subclasses
    virtual void run() = 0;

    void wait(std::optional<std::chrono::milliseconds> timeout = std::nullopt) {
        if (m_thread.joinable()) {
            if (timeout.has_value()) {
                std::unique_lock<std::mutex> lock(m_mutex);
                if (m_cv.wait_for(lock, timeout.value(), [this]() { return m_stopFlag.load(); })) {
                    m_thread.join();
                } else {
                    throw std::runtime_error("Thread wait timed out.");
                }
            } else {
                m_thread.join();
            }
        }
    }

    bool isRunning() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return !m_stopFlag;
    }

    virtual void stop() {
        m_stopFlag = true;
        m_cv.notify_all();
    }

    ~Thread() {
        stop();
        wait();
    }

  private:
    std::thread m_thread;
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_stopFlag;

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
            break;
        case ThreadPriority::Normal:
            posixPriority = 0;
            break;
        case ThreadPriority::High:
            posixPriority = -1;
            break;
        case ThreadPriority::TimeCritical:
            posixPriority = -2;
            break;
        }
        pthread_t thread = m_thread.native_handle();
        struct sched_param params;
        params.sched_priority = posixPriority;
        pthread_setschedparam(thread, SCHED_FIFO, &params);
#endif
    }
};
