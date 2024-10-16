#include "../headers/qs1r_server.h"

#include <iostream>
#include <string>
#include <vector>

#include "../headers/ByteArray.h"
#include "../headers/Firmware.h"
#include "../headers/bitstream.h"
#include "../headers/config.h"
#include "../headers/qs_globals.h"
#include "../headers/qs_io_libusb.h"
#include "../headers/qs_sleep.h"
#include "../headers/debugloggerclass.h"

QS1R_server ::QS1R_server() {
    QsGlobal::g_server = this;

    QsSleep sleep;
    QsIOLib_LibUSB usb;
}

QS1R_server ::~QS1R_server() {}

void QS1R_server ::exit() { usb.exit(); }

void QS1R_server ::showStartupMsg() { _debug() << "This is the QS1R Server version: " << VERSION; }

void QS1R_server ::initSupportedSampleRatesList() {
    m_supported_samplerates.push_back("50000");
    m_supported_samplerates.push_back("125000");
    m_supported_samplerates.push_back("250000");
}

std::vector<std::string> QS1R_server ::getSupportedSampleRates() { return m_supported_samplerates; }

libusb_device *QS1R_server ::findQS1RDevice(QsIOLib_LibUSB *usb, u_int16_t vid, uint16_t pid, unsigned int index) {
    libusb_device *device = nullptr;
    device = usb->findQsDevice(QS1R_VID, QS1R_PID, 0);
    if (device) {
        return device;
    } else {
        return nullptr;
    }
}

// -------------------------------------------------------------
// Initializes the QS1R Hardware
// Loads the firmware and the fpga bit stream
// -------------------------------------------------------------
int QS1R_server ::initQS1RHardware(unsigned int index = 0) {
    if (m_hardware_is_init) {
        // close the libusb handle
    }
    m_hardware_is_init = false;

    _debug() << "Trying the libusb driver with index [" << index << "]... wait...";

    libusb_device *dev = findQS1RDevice(&usb, QS1R_VID, QS1R_PID, 0);

    if (dev == nullptr) {
        std::cerr << "Could not find any QS1R devices!";
        return -1;
    }

    _debug() << "QS1R Device count: " << usb.qs1rDeviceCount();
    _debug() << "Device count: " << usb.deviceCount();

    int ret = usb.open(dev);

    if (ret == 0) {
        int fpga_id = 0;
        int fw_id = 0;
        _debug() << "Open success!";
        _debug() << "FPGA ID returned: " << std::hex << (fpga_id = usb.readMultibusInt(MB_VERSION_REG)) << std::dec
                 ;
        _debug() << "FW S/N: " << (fw_id = usb.readFwSn());

        if (fw_id != ID_FWWR) {
            _debug() << "Attempting to load firmware...";
            int result = usb.loadFirmware(firmware_hex);
            if (result == 0) {
                _debug() << "Firmware load success!";
                usb.close();
                sleep.msleep(5000);
            }
        } else {
            _debug() << "Firmware is already loaded!";
        }

        libusb_device *dev = findQS1RDevice(&usb, QS1R_VID, QS1R_PID, 0);

        if (dev == nullptr) {
            std::cerr << "Could not find any QS1R devices!";
            return -1;
        }

        ret = usb.open(dev);

        if (ret != 0) {
            std::cerr << "Open Device Error: " << libusb_error_name(ret);
            return -1;
        }

        _debug() << "FW S/N: " << std::dec << (fw_id = usb.readFwSn());

        if (fpga_id != ID_1RXWR) {
            _debug() << "Attempting to load FPGA bitstream...";
            int result = usb.loadFpgaFromBitstream(fpga_bitstream, fpga_bitstream_size);
            if (result == 0) {
                _debug() << "FPGA load success!";
                usb.close();
                sleep.msleep(1000);
            }
        } else {
            _debug() << "FPGA already loaded!";
        }

        usb.open(dev);

        _debug() << "FPGA ID returned: " << std::hex << (fpga_id = usb.readMultibusInt(MB_VERSION_REG)) << std::dec
                 ;

    } else {
        std::cerr << "Error opening device!";
        return -1;
    }

    _debug() << "QS1R index [" << index << "] hardware was successfully initialized!";
    m_hardware_is_init = true;
    return 0;
}

int QS1R_server ::setPgaMode(bool on) {
    if (!m_hardware_is_init) {
        std::cerr << "Error: Please initialize QS1R Hardware first!";
        return -1;
    }
    int result = usb.readMultibusInt(MB_CONTRL1);
    if (on) {
        result |= PGA;
    } else {
        result &= ~PGA;
    }

    return usb.writeMultibusInt(MB_CONTRL1, (u_int)result);
}

int QS1R_server ::setRandMode(bool on) {
    if (!m_hardware_is_init) {
        std::cerr << "Error: Please initialize QS1R Hardware first!";
        return -1;
    }
    int result = usb.readMultibusInt(MB_CONTRL1);
    if (on) {
        result |= RANDOM;
    } else {
        result &= ~RANDOM;
    }

    return usb.writeMultibusInt(MB_CONTRL1, (u_int)result);
}

int QS1R_server ::setDitherMode(bool on) {
    if (!m_hardware_is_init) {
        std::cerr << "Error: Please initialize QS1R Hardware first!";
        return -1;
    }
    int result = usb.readMultibusInt(MB_CONTRL1);
    if (on) {
        result |= DITHER;
    } else {
        result &= ~DITHER;
    }

    return usb.writeMultibusInt(MB_CONTRL1, (u_int)result);
}

bool QS1R_server ::pgaMode() {
    int result = usb.readMultibusInt(MB_CONTRL1);
    if ((result & PGA) == PGA)
        return true;
    else
        return false;
}

bool QS1R_server ::randMode() {
    int result = usb.readMultibusInt(MB_CONTRL1);
    if ((result & RANDOM) == RANDOM)
        return true;
    else
        return false;
}

bool QS1R_server ::ditherMode() {
    int result = usb.readMultibusInt(MB_CONTRL1);
    if ((result & DITHER) == DITHER)
        return true;
    else
        return false;
}
