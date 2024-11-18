#include <iostream>
#include <sstream>

#include "../include/config.h"
#include "../include/qs1r_server.hpp"
#include "../include/qs_command.hpp"
#include "../include/qs_debugloggerclass.hpp"
#include "../include/qs_globals.hpp"

#include "/usr/local/include/rtaudio/RtAudio.h"
#include <fstream>
#include <unistd.h>

int main() {

    // Enable debug logging
    DebugLogger::DEBUG = true;   

    QS1RServer qs1r;
    CommandProcessor cmd_proc;
    QsSleep sleep;

    if (qs1r.initialize() != 0) {
        std::string result;
        std::cout << "continue? y/n ";
        std::cin >> result;
        if (result == "n") {
            return -1;
        }
    }

    QsGlobal::g_audio->startStream();

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
