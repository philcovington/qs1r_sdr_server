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
    
    qs1r.startDACWriter();

    qs1r.shutdown();

    _debug() << "QS1R server shutting down...";
    sleep.msleep(500);

    return 0;
}
