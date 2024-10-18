#pragma once

#include "../include/qs_sleep.hpp"
#include "../include/qs_threading.hpp"
#include <vector>

class QsIoThread : public Thread {
  public:
    explicit QsIoThread();

    void startThread();
    void stopThread();
    void stop() override;  // Mark stop() as override
    void run();

  private:
    bool m_thread_go;
    QsSleep sleep;

    void decodeMessageId(std::vector<unsigned char> &vector);
    void decodeBitStatus(std::vector<unsigned char> &vector);
};
