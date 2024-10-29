#include <iostream>
#include <sstream>

#include "../include/config.h"
#include "../include/qs1r_server.hpp"
#include "../include/qs_bitstream.hpp"
#include "../include/qs_bytearray.hpp"
#include "../include/qs_debugloggerclass.hpp"
#include "../include/qs_io_libusb.hpp"
#include "../include/qs_test_tone.hpp"

int main() {

    // Enable debug logging
    DebugLogger::DEBUG = true;
    std::string cmd;

    QS1RServer qs1r;
    QsSleep sleep; 
        
    qs1r.initialize(); 
    
    while (cmd != "exit") {
        std::cout << "?:";
        std::cin >> cmd;
        std::cout << cmd << std::endl;
        if (cmd == "start") {
            qs1r.startIo();
        } else if (cmd == "stop") {
            qs1r.stopIo();
        }  
    }  
    
    qs1r.stopIo();   
    
    qs1r.shutdown();

    _debug() << "QS1R server shutting down...";
    sleep.msleep(500);

    return 0;
}
