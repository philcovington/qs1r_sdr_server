/**
 * @file QsIOLib_LibUSB.h
 * @brief A class for interfacing with USB devices using the LibUSB library.
 *
 * The QsIOLib_LibUSB class provides an interface for communicating with USB devices,
 * particularly those adhering to the QS1R specifications. It encapsulates the LibUSB
 * functionalities required for device enumeration, data transfer, and firmware management.
 *
 * Features:
 * - Support for USB device discovery and communication.
 * - Functions to read from and write to various endpoints of the USB device.
 * - Capabilities for loading firmware and FPGA bitstreams into the device.
 * - Methods for I2C and EEPROM communication.
 * - Control over device resets and other control messages.
 * - Utilities for obtaining device class and subclass strings, facilitating better
 *   understanding of device capabilities.
 *
 * Usage:
 * To utilize the QsIOLib_LibUSB class, instantiate the class and call its methods
 * to interact with USB devices. For example:
 *
 *   QsIOLib_LibUSB usbInterface;
 *   usbInterface.findDevices(); // Find and enumerate devices
 *
 *   unsigned char buffer[64];
 *   usbInterface.readEP1(buffer, sizeof(buffer)); // Read data from endpoint 1
 *
 * The class is designed for flexibility and ease of use in applications that require
 * communication with USB devices, such as DSP systems, data acquisition systems, and
 * custom hardware interfacing.
 *
 * Author: Philip A Covington
 * Date: 2024-10-16
 */

#pragma once

#ifndef QSIO_H
#define QSIO_H

#define MAX_BUFFER_SZ 16384 * 8

#define QS1R_GUID L"{1491A52C-73EC-4596-8638-C458CFB91A17}"
#define QS1R_VID 0xfffe
#define QS1R_PID 0x8
#define QS1R_MISSING_EEPROM_VID 0x4b4
#define QS1R_MISSING_EEPROM_PID 0x8613

#define MAX_EP0_PACKET_SIZE 64
#define MAX_EP4_PACKET_SIZE 512

#define QS1R_DAC_EP 0x02
#define QS1R_CH0_EP 0x86
#define QS1R_CH1_EP 0x88

#define FX2_RAM_RESET 0xE600
#define FX2_WRITE_RAM_REQ 0xA0

/* Vendor Request Types */
#define VRT_VENDOR_IN 0xC0
#define VRT_VENDOR_OUT 0x40

/* Vendor In Commands */
#define VRQ_I2C_READ 0x81 // wValueL: i2c address; length: how much to read
#define VRQ_SPI_READ 0x82 // wValue: optional header bytes
// wIndexH:	enables
// wIndexL:	format
// len: how much to read

#define VRQ_SN_READ 0x83

#define VRQ_EEPROM_TYPE_READ 0x84
#define VRQ_I2C_SPEED_READ 0x85
#define VRQ_MULTI_READ 0x86
#define VRQ_DEBUG_READ 0x87

/* Vendor Out Commands */
#define VRQ_FPGA_LOAD 0x02
#define FL_BEGIN 0
#define FL_XFER 1
#define FL_END 2

#define VRQ_FPGA_SET_RESET 0x04 // wValueL: {0,1}
#define VRQ_MULTI_WRITE 0x05
#define VRQ_REQ_I2C_WRITE 0x08 // wValueL: i2c address; data: data
#define VRQ_REQ_SPI_WRITE 0x09 // wValue: optional header bytes
// wIndexH:	enables
// wIndexL:	format
// len: how much to write

#define VRQ_I2C_SPEED_SET 0x0B // wValueL: {0,1}
#define VRQ_CPU_SPEED_SET 0x0C // wValueL: {0, 1, 2}
#define VRQ_EP_RESET 0x0D
#define VRQ_ECHO_TO_EP1IN 0x0E //
#define VRQ_INT5_READY 0x0F    //

#define DEFAULT_VID 0xfffe
#define DEFAULT_PID 0x8
#define QS1R_EEPROM_ADDR 0x51
#define QS1E_PDAC_ADDR 0x60

#define MB_VERSION_REG 0
#define MB_CONTRL0 1
#define MB_CONTRL1 2
#define MB_SAMPLERATE 3
#define MB_CW_SETTINGS_REG 4
#define MB_CW_SIDETONE_FREQ 5
#define MB_RFB_CNTRL 6
#define MB_RFB_IO 7
#define MB_TX_FREQ 8
#define MB_STATUS_MSG 9
#define MB_FREQRX0_REG 10
#define MB_FREQRX1_REG 11
#define MB_FREQRX2_REG 12
#define MB_FREQRX3_REG 13

#define MB_CONTRL0_BIT0 0x1
#define MB_CONTRL0_BIT1 0x2
#define MB_CONTRL0_BIT2 0x4
#define MB_CONTRL0_BIT3 0x8
#define MB_CONTRL0_BIT4 0x10
#define MB_CONTRL0_BIT5 0x20
#define MB_CONTRL0_BIT6 0x40
#define MB_CONTRL0_BIT7 0x80
#define MB_CONTRL0_BIT31 0x80000000

#define DAC_BYPASS MB_CONTRL0_BIT0
#define DAC_EXT_MUTE_EN MB_CONTRL0_BIT1
#define WB_BYPASS MB_CONTRL0_BIT2
#define DAC_CLK_SEL MB_CONTRL0_BIT3
#define MASTER_RESET MB_CONTRL0_BIT31

#define MB_CONTRL1_BIT0 0x1
#define MB_CONTRL1_BIT1 0x2
#define MB_CONTRL1_BIT2 0x4

#define PGA MB_CONTRL1_BIT0
#define RANDOM MB_CONTRL1_BIT1
#define DITHER MB_CONTRL1_BIT2

#define TX_PTT 0x1
#define CW_ENABLE 0x2

#define ID_2RX 0x20000000
#define ID_1RXWR 0x02102012
#define ID_FWWR 3032011

#define USB_TIMEOUT_CONTROL 500
#define USB_TIMEOUT_BULK 1000

#define WB_BLOCK_SIZE 32768

// OUT
#define FX2_EP1_OUT 0x01
#define FX2_EP2 0x02
#define FX2_EP4 0x04

// IN
#define FX2_EP1_IN 0x81
#define FX2_EP6 0x86
#define FX2_EP8 0x88

#include "../include/qs_sleep.hpp"
#include <libusb-1.0/libusb.h>
#include <string>

class QsIOLib_LibUSB {
  public:
    QsIOLib_LibUSB();
    ~QsIOLib_LibUSB();

    std::string getDeviceClassString(uint8_t deviceClass);
    std::string getDeviceSubClassString(uint8_t deviceClass, uint8_t deviceSubClass);
    std::string getDeviceProtocolString(uint8_t deviceClass, uint8_t deviceSubClass, uint8_t deviceProtocol);
    std::string get_string_descriptor(libusb_device_handle *device_handle, uint8_t index);

    int open(libusb_device *dev);
    void close();
    void exit();
    libusb_device *findQsDevice(uint16_t idVendor, uint16_t idProduct, unsigned int index = 0);
    int findDevices(bool detailed = false);
    int deviceCount();
    int qs1rDeviceCount();
    int loadFirmware(std::string filename);
    int loadFirmware(const char *firmware_hex);
    int loadFpga(std::string filename);
    int loadFpgaFromBitstream(const unsigned char *bitstream, unsigned int bitstream_size);
    int readFwSn();
    int read(unsigned int ep, unsigned char *buffer, unsigned int length, unsigned int timeout = USB_TIMEOUT_BULK);
    int write(unsigned int ep, unsigned char *buffer, unsigned int length, unsigned int timeout = USB_TIMEOUT_BULK);
    int writeEP1(unsigned char *buffer, unsigned int length, unsigned int timeout = USB_TIMEOUT_BULK);
    int writeEP2(unsigned char *buffer, unsigned int length, unsigned int timeout = USB_TIMEOUT_BULK);
    int writeEP4(unsigned char *buffer, unsigned int length, unsigned int timeout = USB_TIMEOUT_BULK);
    int readEP1(unsigned char *buffer, unsigned int length, unsigned int timeout = USB_TIMEOUT_BULK);
    int readEP6(unsigned char *buffer, unsigned int length, unsigned int timeout = USB_TIMEOUT_BULK);
    int readEP8(unsigned char *buffer, unsigned int length, unsigned int timeout = USB_TIMEOUT_BULK);
    int readEEPROM(unsigned int address, unsigned int offset, unsigned char *buffer, unsigned int length);
    int writeEEPROM(unsigned int address, unsigned int offset, unsigned char *buffer, unsigned int length);
    int readI2C(unsigned int address, unsigned char *buffer, unsigned int length);
    int writeI2C(unsigned int address, unsigned char *buffer, unsigned int length);
    int readMultibusInt(u_int16_t index);
    int readMultibusBuf(unsigned int index, unsigned char *buffer, unsigned int length);
    int writeMultibusInt(unsigned int index, unsigned int value);
    int writeMultibusBuf(unsigned int index, unsigned char *buffer, unsigned int length);
    int resetDevice();
    int cpuResetControl(bool reset = 1);
    int deviceWasFound();

    int clearHalt(libusb_device_handle *hdev, int ep);
    int sendInterrupt5Gate();

    int sendControlMessage(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, u_char *buf,
                           uint16_t size, unsigned int timeout = USB_TIMEOUT_CONTROL);

  private:
    std::string printVectorInHex(const std::vector<uint8_t> &ba);
    unsigned int hexStringToByte(const std::string &hex);
    uint8_t calculateChecksum(const std::string &hexLine);

    libusb_context *context = nullptr;
    libusb_device *dev = nullptr;
    libusb_device_handle *hdev = nullptr;

    int write_cpu_ram(u_int16_t startaddr, u_char *buffer, u_int16_t length);

    std::string device_path;

    bool dev_was_found;
    unsigned int usb_dev_count;
    unsigned int qs1r_device_count;

    QsSleep qssleep;
};

#endif