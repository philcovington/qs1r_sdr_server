/**
 * @file WaitCondition.h
 * @brief A class for managing thread synchronization using condition variables.
 *
 * The WaitCondition class encapsulates a mutex and a condition variable to 
 * facilitate synchronization between threads. It provides methods for 
 * waiting on a condition and notifying waiting threads, enabling 
 * safe and efficient communication between concurrent tasks.
 *
 * Features:
 * - Ability to wait until a condition is notified, ensuring threads can 
 *   block until they are explicitly signaled.
 * - Support for waiting with a timeout, allowing threads to proceed 
 *   if the condition is not met within a specified duration.
 * - Methods to notify one or all waiting threads, facilitating 
 *   thread wake-up mechanisms.
 * - Wrapper function to simplify waking all waiting threads.
 *
 * Usage:
 * To utilize the WaitCondition class for thread synchronization, you can 
 * do the following:
 *
 *   WaitCondition waitCondition;
 *
 *   // In one thread:
 *   waitCondition.wait(); // Wait until notified
 *
 *   // In another thread:
 *   waitCondition.notify_one(); // Notify one waiting thread
 *
 * This class is particularly useful in scenarios where threads need to 
 * wait for specific conditions to be met before proceeding, such as in 
 * producer-consumer situations or when coordinating access to shared 
 * resources.
 * 
 * I tried to provide similar functionality to Qt's QWaitCondition class.
 *
 * Author: Philip A Covington
 * Date: 2024-10-16
 */

#pragma once

#include <condition_variable>
#include <mutex>

class WaitCondition {
  public:
    WaitCondition() = default;

    // Lock the mutex and wait until the condition is notified
    void wait() {
        std::unique_lock<std::mutex> lock(mutex);
        cond_var.wait(lock);
    }

    // Wait with a timeout
    template <class Rep, class Period> bool wait_for(const std::chrono::duration<Rep, Period> &timeout_duration) {
        std::unique_lock<std::mutex> lock(mutex);
        return cond_var.wait_for(lock, timeout_duration) == std::cv_status::no_timeout;
    }

    // Notify one waiting thread
    void notify_one() {
        std::lock_guard<std::mutex> lock(mutex);
        cond_var.notify_one();
    }

    // Notify all waiting threads
    void notify_all() {
        std::lock_guard<std::mutex> lock(mutex);
        cond_var.notify_all();
    }

    // Wake all waiting threads (wrapper for notify_all)
    void wakeAll() {
        notify_all(); // Simply call notify_all here
    }

  private:
    std::mutex mutex;
    std::condition_variable cond_var;
};
