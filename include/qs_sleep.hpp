/**
 * @file QsSleep.h
 * @brief A utility class for sleeping in various time units.
 *
 * The QsSleep class provides methods to pause execution for specified durations,
 * allowing for controlled timing in applications. It leverages the C++ standard
 * library's threading capabilities to implement sleep functionality in seconds,
 * milliseconds, and microseconds.
 *
 * Features:
 * - Sleep for a specified number of seconds.
 * - Sleep for a specified number of milliseconds.
 * - Sleep for a specified number of microseconds.
 *
 * Usage:
 * To use the QsSleep class, create an instance and call the appropriate sleep method:
 *
 *   QsSleep sleepUtil;
 *   sleepUtil.sleep(1);      // Sleeps for 1 second
 *   sleepUtil.msleep(500);   // Sleeps for 500 milliseconds
 *   sleepUtil.usleep(1000);  // Sleeps for 1000 microseconds
 *
 * This class is useful for implementing time delays in applications such as
 * data processing, device communication, and timing control in embedded systems.
 *
 * Author: [Philip A Covington]
 * Date: [2024-10-16]
 */

#pragma once

#include <chrono>
#include <thread>

class QsSleep {
  public:
    // Sleep for seconds
    void sleep(unsigned long sec) { std::this_thread::sleep_for(std::chrono::seconds(sec)); }

    // Sleep for milliseconds
    void msleep(unsigned long msec) { std::this_thread::sleep_for(std::chrono::milliseconds(msec)); }

    // Sleep for microseconds
    void usleep(unsigned long usec) { std::this_thread::sleep_for(std::chrono::microseconds(usec)); }
};
