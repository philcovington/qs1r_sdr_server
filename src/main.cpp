#include <iostream>
#include <sstream>

#include "../include/config.h"
#include "../include/qs1r_server.hpp"
#include "../include/qs_bitstream.hpp"
#include "../include/qs_bytearray.hpp"
#include "../include/qs_debugloggerclass.hpp"
#include "../include/qs_io_libusb.hpp"

int main() {

    // Enable debug logging
    DebugLogger::DEBUG = true;

    QS1RServer qs1r;
    QsSleep sleep;

    int result = qs1r.initQS1RHardware();

    _debug() << "PGA MODE IS: " << qs1r.pgaMode();
    _debug() << "RAND MODE IS: " << qs1r.randMode();
    _debug() << "DITHER MODE IS: " << qs1r.ditherMode();

    result = qs1r.setPgaMode(true);

    _debug() << "setPgaMode result: " << result;

    result = qs1r.setRandMode(true);

    _debug() << "setRandMode result: " << result;

    result = qs1r.setDitherMode(true);

    _debug() << "setDitherMode result: " << result;

    _debug() << "PGA MODE NOW IS: " << qs1r.pgaMode();
    _debug() << "RAND MODE NOW IS: " << qs1r.randMode();
    _debug() << "DITHER MODE NOW IS: " << qs1r.ditherMode();

    _debug() << "QS1R server shutting down...";

    qs1r.shutdown();

    sleep.msleep(500);

    return 0;
}
