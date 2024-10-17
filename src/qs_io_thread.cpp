#include "../headers/qs_io_thread.hpp"
#include "../headers/debugloggerclass.h"
#include "../headers/qs_dataproc.h"
#include "../headers/qs_defaults.h"
#include "../headers/qs_defines.h"
#include "../headers/qs_globals.h"
#include "../headers/qs_sleep.h"
#include "../headers/stringclass.h"

QsIoThread ::QsIoThread() : m_thread_go(false) {}

void QsIoThread ::run() {
    std::vector<unsigned char> buffer;
    buffer.resize(4);
    std::fill(buffer.begin(), buffer.end(), 0);

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

void QsIoThread ::stop() { m_thread_go = false; }

//
// message ID 0x1 is IO status message with 24 bits of io info
// message[0] = message ID
// message[1] = bits 0 through 7
// message[2] = bits 8 through 15
// message[3] = bits 16 through 23
//
void QsIoThread ::decodeMessageId(std::vector<unsigned char> &vector) {
    switch ((int)vector[0]) {
    case 0x1: // io status message
        decodeBitStatus(vector);
        break;
    default:
        break;
    }
}

void QsIoThread ::decodeBitStatus(std::vector<unsigned char> &vector) {
    // check ptt bit status
    // if ((vector[1] & PTT_BIT) == PTT_BIT) {
    //     _debug() << "ext ptt on";
    //     if (!QsTx::g_soft_ptt) {
    //         QsGlobal::g_server->setTxPttDirect(true);
    //     }
    // } else {
    //     _debug() << "ext ptt off";
    //     if (!QsTx::g_soft_ptt) {
    //         QsGlobal::g_server->setTxPttDirect(false);
    //     }
    // }
}
