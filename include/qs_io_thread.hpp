#pragma once

#include "../include/qs_thread.hpp"
#include "../include/qs_sleep.hpp"
#include <vector>

class QsIoThread : public Thread {
  public:
    explicit QsIoThread();

    void run();
    void stop();

  private:
    static const char PTT_BIT = 0x1;

    bool m_thread_go;

    void decodeMessageId(std::vector<unsigned char> &vector);
    void decodeBitStatus(std::vector<unsigned char> &vector);

	QsSleep sleep;
};
