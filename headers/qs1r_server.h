#pragma once

#include <string>
#include <vector>

#include "../headers/qs_io_libusb.h"
#include "../headers/qs_sleep.h"

#ifndef QS1RSERVER_H
#define QS1RSERVER_H

class QS1R_server {
  public:
    QsIOLib_LibUSB usb;

    QS1R_server();
    ~QS1R_server();

    int setPgaMode(bool on);
    int setRandMode(bool on);
    int setDitherMode(bool on);

    bool pgaMode();
    bool randMode();
    bool ditherMode();

    bool isHardwareInit();

    void showStartupMsg();

    void initSupportedSampleRatesList();
    std::vector<std::string> getSupportedSampleRates();

    int initQS1RHardware(unsigned int index);

    void exit();

  private:
    std::vector<std::string> m_supported_samplerates;

    QsSleep sleep;
    bool m_hardware_is_init = false;

    libusb_device *findQS1RDevice(QsIOLib_LibUSB *usb, u_int16_t vid, uint16_t pid, unsigned int index);
};

#endif