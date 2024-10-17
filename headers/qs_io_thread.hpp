#pragma once

#include "../headers/qs_threads.h"
#include "../headers/qs_sleep.h"
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
