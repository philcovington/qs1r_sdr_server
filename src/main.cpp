#include <iostream>
#include <sstream>

#include "../include/config.h"
#include "../include/qs1r_server.hpp"
#include "../include/qs_command.hpp"
#include "../include/qs_debugloggerclass.hpp"
#include "../include/qs_globals.hpp"

#include "/usr/include/rtaudio/RtAudio.h"
#include <fstream>
#include <unistd.h>

int main() {

    // Enable debug logging
    DebugLogger::DEBUG = true;
    std::string cmd;

    QS1RServer qs1r;
    CommandProcessor cmd_proc;    
    QsSleep sleep;

    // RtAudio audio(RtAudio::LINUX_ALSA);

    // freopen("/dev/null", "w", stderr);

    // int cnt = audio.getDeviceCount();

    // std::cout << cnt << std::endl;

    // RtAudio::DeviceInfo info;
    // for (unsigned int i = 0; i < cnt; i++) {
    //     info = audio.getDeviceInfo(i);
    //     if (info.outputChannels > 0) {
    //         std::cout << info.name << std::endl;
    //         std::cout << "Supported sample rates: ";
    //         for (unsigned int j = 0; j < info.sampleRates.size(); ++j) {
    //             std::cout << info.sampleRates[j] << " ";
    //         }
    //         std::cout << std::endl;
    //     } else if (info.inputChannels > 0) {
    //         std::cout << info.name << std::endl;
    //         // rtaInputDeviceMap[i] = String::fromStdString(info.name);
    //         // rates = List<unsigned int>::fromVector(info.sampleRates);
    //     }
    // }

    // freopen("/dev/tty", "w", stderr);
    
    // return 0;

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
