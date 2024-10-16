#include <iostream>
#include <sstream>

#include "../headers/ByteArray.h"
#include "../headers/bitstream.h"
#include "../headers/config.h"
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
    QS1R_server qs1r;
    QsSleep sleep;

    qs1r.showStartupMsg();

    int result = qs1r.initQS1RHardware(QS1R_IDX);

    std::cout << "PGA MODE IS: " << qs1r.pgaMode() << std::endl;
    std::cout << "RAND MODE IS: " << qs1r.randMode() << std::endl;
    std::cout << "DITHER MODE IS: " << qs1r.ditherMode() << std::endl;

    result = qs1r.setPgaMode(true);

    std::cout << "setPgaMode result: " << result << std::endl;

    result = qs1r.setRandMode(true);

    std::cout << "setRandMode result: " << result << std::endl;

    result = qs1r.setDitherMode(true);

    std::cout << "setDitherMode result: " << result << std::endl;

    std::cout << "PGA MODE NOW IS: " << qs1r.pgaMode() << std::endl;
    std::cout << "RAND MODE NOW IS: " << qs1r.randMode() << std::endl;
    std::cout << "DITHER MODE NOW IS: " << qs1r.ditherMode() << std::endl;

    std::cout << "QS1R server shutting down..." << std::endl;

    qs1r.exit();

    sleep.msleep(500);

    return 0;
}
