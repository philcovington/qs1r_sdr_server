#include "../include/qs_io_thread.hpp"
#include "../include/qs_debugloggerclass.hpp"
#include "../include/qs_defaults.hpp"
#include "../include/qs_defines.hpp"
#include "../include/qs_globals.hpp"
#include "../include/qs_signalops.hpp"
#include "../include/qs_sleep.hpp"

QsIoThread::QsIoThread() : m_thread_go(false) {}

void QsIoThread::startThread() {
    start([this]() { this->run(); }, ThreadPriority::Normal);
}

void QsIoThread::stopThread() {
    stop(); // Set m_thread_go to false and stop the base class thread
    wait(); // Wait for the thread to finish
}

void QsIoThread::run() {
    std::vector<unsigned char> buffer(4, 0);
    int counter = 0;

    if (QsGlobal::g_server->isHardwareInit())
        QsGlobal::g_io->sendInterrupt5Gate();

    m_thread_go = true;

    while (m_thread_go) {
        if (QsGlobal::g_server->isHardwareInit()) {
            if (QsGlobal::g_io->readEP1(buffer.data(), buffer.size(), 2000) == buffer.size()) {
                decodeMessageId(buffer);
                _debug() << "-------------------";
                _debug() << "buffer[0]: " << String::number(buffer[0]);
                _debug() << "buffer[1]: " << String::number(buffer[1]);
                _debug() << "buffer[2]: " << String::number(buffer[2]);
                _debug() << "buffer[3]: " << String::number(buffer[3]);
                _debug() << "-------------------";
                _debug() << counter++;
                QsGlobal::g_io->sendInterrupt5Gate();
            } else if (!m_thread_go) {
                break;
            }
        } else if (!m_thread_go) {
            break;
        } else {
            sleep.msleep(500);
        }
    }
}

void QsIoThread::stop() {
    m_thread_go = false; // Set thread running flag to false
    Thread::stop();      // Call base class stop
}

void QsIoThread::decodeMessageId(std::vector<unsigned char> &vector) {
    switch (vector[0]) {
    case 0x1: // IO status message
        decodeBitStatus(vector);
        break;
    default:
        break;
    }
}

void QsIoThread::decodeBitStatus(std::vector<unsigned char> &vector) {
    // Example decoding logic
}
