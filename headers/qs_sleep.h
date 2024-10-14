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
