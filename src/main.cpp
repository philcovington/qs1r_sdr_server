#include <iostream>
#include <sstream>

#include "../headers/ByteArray.h"
#include "../headers/bitstream.h"
#include "../headers/config.h"
#include "../headers/debugloggerclass.h"
#include "../headers/qs1r_server.h"
#include "../headers/qs_io_libusb.h"

libusb_device *findQS1RDevice(QsIOLib_LibUSB *usb, u_int16_t vid, uint16_t pid, unsigned int index) {
    libusb_device *device = nullptr;
    device = usb->findQsDevice(QS1R_VID, QS1R_PID, 0);
    if (device) {
        return device;
    } else {
        return nullptr;
    }
}

int main() {

    // Enable debug logging
    DebugLogger::DEBUG = true;

    QS1R_server qs1r;
    QsSleep sleep;

    qs1r.showStartupMsg();

    int result = qs1r.initQS1RHardware(QS1R_IDX);

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

    qs1r.exit();

    sleep.msleep(500);

    return 0;
}
