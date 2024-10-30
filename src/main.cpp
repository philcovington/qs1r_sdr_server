#include <iostream>
#include <sstream>

#include "../include/config.h"
#include "../include/qs1r_server.hpp"
#include "../include/qs_bitstream.hpp"
#include "../include/qs_bytearray.hpp"
#include "../include/qs_command.hpp"
#include "../include/qs_debugloggerclass.hpp"
#include "../include/qs_globals.hpp"
#include "../include/qs_io_libusb.hpp"
#include "../include/qs_test_tone.hpp"

int main() {

    // Enable debug logging
    DebugLogger::DEBUG = true;
    std::string cmd;

    QS1RServer qs1r;
    CommandProcessor cmd_proc;
    QsSleep sleep;

    qs1r.initialize();

    cmd_proc.init(&qs1r);
    cmd_proc.process();

    if (qs1r.isDspProcessorRunning()) {
        qs1r.stopIo();
    }

    qs1r.shutdown();

    _debug() << "QS1R server shutting down...";
    sleep.msleep(500);

    return 0;
}
