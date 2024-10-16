#include <cstring>
#include <iomanip>
#include <sstream>
#include <vector>

#include "../headers/ByteArray.h"
#include "../headers/File.h"
#include "../headers/TextStream.h"
#include "../headers/debugloggerclass.h"
#include "../headers/qs_io_libusb.h"
#include <libusb-1.0/libusb.h>

std::string QsIOLib_LibUSB ::printVectorInHex(const std::vector<uint8_t> &ba) {
    std::stringstream ss;
    for (const auto &byte : ba) {
        ss << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(byte) << " ";
    }
    return ss.str();
}

unsigned int QsIOLib_LibUSB ::hexStringToByte(const std::string &hex) {
    unsigned int byte;
    std::stringstream ss;
    ss << std::hex << hex;
    ss >> byte;
    return byte;
}

uint8_t QsIOLib_LibUSB ::calculateChecksum(const std::string &hexLine) {
    if (hexLine[0] != ':') {
        throw std::invalid_argument("Line does not start with a colon");
    }

    int sum = 0;

    // Start at index 1, after the colon, and process two characters at a time
    for (size_t i = 1; i < hexLine.length() - 2; i += 2) {
        std::string byteStr = hexLine.substr(i, 2); // Take two characters (one byte)
        sum += hexStringToByte(byteStr);
    }

    // Return two's complement of the least significant byte of the sum
    uint8_t checksum = static_cast<uint8_t>(-sum);
    return checksum;
}

QsIOLib_LibUSB ::QsIOLib_LibUSB() {
    hdev = nullptr;
    dev_was_found = false;
    usb_dev_count = 0;
    qs1r_device_count = 0;
    int result = libusb_init(&context);

    if (result != LIBUSB_SUCCESS) {
        _debug() << "Libusb init failed!";
    }
}

QsIOLib_LibUSB ::~QsIOLib_LibUSB() {}

int QsIOLib_LibUSB ::clearHalt(libusb_device_handle *hdev, int ep) {
    if (hdev != nullptr)
        return libusb_clear_halt(hdev, ep);
    return -1;
}

int QsIOLib_LibUSB ::open(libusb_device *dev) {
    close();

    if (!dev) {
        _debug() << "Libusb Device is nullptr! ";
        return -1;
    }

    QsIOLib_LibUSB ::hdev = nullptr;

    int result = libusb_open(dev, &hdev);

    if (hdev) {
        if (result = libusb_set_configuration(hdev, 1);
            result != LIBUSB_SUCCESS && result != LIBUSB_TRANSFER_COMPLETED) {
            _debug() << "Could not set configuration 1: " << libusb_error_name(result);
            return -1;
        }
        if (result = libusb_claim_interface(hdev, 0); result != LIBUSB_SUCCESS && result != LIBUSB_TRANSFER_COMPLETED) {
            _debug() << "Could not claim interface 0: " << libusb_error_name(result);
            return -1;
        }
        if (result = libusb_set_interface_alt_setting(hdev, 0, 0);
            result != LIBUSB_SUCCESS && result != LIBUSB_TRANSFER_COMPLETED) {
            _debug() << "Could not set alt interface 0: " << libusb_error_name(result);
            return -1;
        }
        if (result = libusb_clear_halt(hdev, FX2_EP1_IN);
            result != LIBUSB_SUCCESS && result != LIBUSB_TRANSFER_COMPLETED) {
            _debug() << "Could not clear halt on EP1_IN: " << libusb_error_name(result);
        }
        if (result = libusb_clear_halt(hdev, FX2_EP1_OUT);
            result != LIBUSB_SUCCESS && result != LIBUSB_TRANSFER_COMPLETED) {
            _debug() << "Could not clear halt on EP1_OUT: " << libusb_error_name(result);
        }
        if (result = libusb_clear_halt(hdev, FX2_EP2);
            result != LIBUSB_SUCCESS && result != LIBUSB_TRANSFER_COMPLETED) {
            _debug() << "Could not clear halt on EP2: " << libusb_error_name(result);
        }
        if (result = libusb_clear_halt(hdev, FX2_EP4);
            result != LIBUSB_SUCCESS && result != LIBUSB_TRANSFER_COMPLETED) {
            _debug() << "Could not clear halt on EP4: " << libusb_error_name(result);
        }
        if (result = libusb_clear_halt(hdev, FX2_EP6);
            result != LIBUSB_SUCCESS && result != LIBUSB_TRANSFER_COMPLETED) {
            _debug() << "Could not clear halt on EP6: " << libusb_error_name(result);
        }
        if (result = libusb_clear_halt(hdev, FX2_EP8);
            result != LIBUSB_SUCCESS && result != LIBUSB_TRANSFER_COMPLETED) {
            _debug() << "Could not clear halt on EP8: " << libusb_error_name(result);
        }
    } else {
        _debug() << "Libusb device handle is nullptr! " << libusb_error_name(result);
        return -1;
    }
    return 0;
}

void QsIOLib_LibUSB ::close() {
    if (hdev != nullptr) {
        libusb_clear_halt(hdev, FX2_EP1_OUT);
        libusb_clear_halt(hdev, FX2_EP1_IN);
        libusb_clear_halt(hdev, FX2_EP2);
        libusb_clear_halt(hdev, FX2_EP4);
        libusb_clear_halt(hdev, FX2_EP6);
        libusb_clear_halt(hdev, FX2_EP8);
        libusb_release_interface(hdev, 0);
        libusb_close(hdev);
    }
    QsIOLib_LibUSB ::hdev = nullptr;
}

void QsIOLib_LibUSB ::exit() { libusb_exit(context); }

std::string QsIOLib_LibUSB ::get_string_descriptor(libusb_device_handle *device_handle, uint8_t index) {
    unsigned char buffer[256]; // Buffer to hold the string
    int result = libusb_get_string_descriptor_ascii(device_handle, index, buffer, sizeof(buffer));

    if (result < 0) {
        _debug() << "Error getting string descriptor: " << libusb_error_name(result);
        return "";
    }
    return std::string(reinterpret_cast<char *>(buffer), result);
}

int QsIOLib_LibUSB ::findDevices(bool detailed) {
    libusb_device **list;
    libusb_device *found = NULL;
    libusb_device_descriptor desc;

    ssize_t cnt = libusb_get_device_list(context, &list);

    if (cnt < 0) {
        _debug() << "Error finding usb devices!";
        return -1;
    }

    for (ssize_t i = 0; i < cnt; i++) {
        libusb_device *device = list[i];
        int result = libusb_get_device_descriptor(device, &desc);
        if (result == LIBUSB_SUCCESS) {
            // Open device to get manufacturer string
            libusb_device_handle *handle;
            int result = libusb_open(device, &handle);
            unsigned char buffer[256]; // Buffer to hold the manufacturer string
            std::string manufacturer_str;
            std::string product_str;
            std::string serial_str;

            memset(buffer, 0, sizeof(buffer));
            if (result == LIBUSB_SUCCESS) {
                if (desc.iManufacturer > 0) {
                    int ret = libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, buffer, sizeof(buffer));
                    if (ret < 0) {
                        _debug() << "    Unable to retrieve manufacturer string: " << ret << libusb_error_name(ret)
                                 ;
                    }
                } else {
                    const char *unknown = "No manufacturer data...";
                    strncpy(reinterpret_cast<char *>(buffer), unknown, sizeof(buffer));
                    buffer[sizeof(buffer) - 1] = '\0';
                }
                manufacturer_str = (reinterpret_cast<char *>(buffer));

                memset(buffer, 0, sizeof(buffer));
                if (desc.iProduct > 0) {
                    int ret = libusb_get_string_descriptor_ascii(handle, desc.iProduct, buffer, sizeof(buffer));
                    if (ret < 0) {
                        _debug() << "    Unable to retrieve product string: " << ret << libusb_error_name(ret)
                                 ;
                    }
                } else {
                    const char *unknown = "No product string...";
                    strncpy(reinterpret_cast<char *>(buffer), unknown, sizeof(buffer));
                    buffer[sizeof(buffer) - 1] = '\0';
                }
                product_str = (reinterpret_cast<char *>(buffer));

                memset(buffer, 0, sizeof(buffer));
                if (desc.iSerialNumber > 0) {
                    int ret = libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, buffer, sizeof(buffer));
                    if (ret < 0) {
                        _debug() << "    Unable to retrieve serial string: " << ret << libusb_error_name(ret)
                                 ;
                    }
                } else {
                    const char *unknown = "No serial string...";
                    strncpy(reinterpret_cast<char *>(buffer), unknown, sizeof(buffer));
                    buffer[sizeof(buffer) - 1] = '\0';
                }
                serial_str = (reinterpret_cast<char *>(buffer));
                libusb_close(handle);
            } else {
                _debug() << "Unable to open device: " << libusb_error_name(result);
                continue;
            }

            if (detailed) {
                _debug() << "Device " << i + 1 << ": "
                          << "Vendor ID: " << std::hex << desc.idVendor << ", Product ID: " << desc.idProduct
                          << ", Class: " << getDeviceClassString(desc.bDeviceClass)
                          << ", Subclass: " << getDeviceSubClassString(desc.bDeviceClass, desc.bDeviceSubClass)
                          << ", Protocol: "
                          << getDeviceProtocolString(desc.bDeviceClass, desc.bDeviceSubClass, desc.bDeviceProtocol)
                          << ", Max Packet Size: " << static_cast<int>(desc.bMaxPacketSize0)
                          << ", Configurations: " << static_cast<int>(desc.bNumConfigurations)
                          << ", USB Ver: " << static_cast<int>(desc.bcdUSB)
                          << ", Release Number: " << static_cast<int>(desc.bcdDevice)
                          << ", Manufacturer: " << manufacturer_str << ", Product: " << product_str
                          << ", Serial: " << serial_str << std::dec;
            } else {
                _debug() << "Device " << i + 1 << ": "
                          << "Vendor ID: " << std::hex << desc.idVendor << ", Product ID: " << desc.idProduct
                          << ", Class: " << getDeviceClassString(desc.bDeviceClass) << std::dec;
            }

        } else {
            _debug() << "Error: " << libusb_error_name(result);
            return -1;
        }
    }
    return 0;
}

libusb_device *QsIOLib_LibUSB ::findQsDevice(uint16_t idVendor, uint16_t idProduct, unsigned int index) {

    libusb_device **list;
    libusb_device *found = NULL;
    libusb_device_descriptor desc;

    ssize_t cnt = libusb_get_device_list(context, &list);

    if (cnt < 0) {
        _debug() << "Error finding usb devices!";
        return NULL;
    }

    unsigned int qs1r_device_count = 0;
    std::vector<libusb_device *> device_list_qs1r; // Vector for dynamic storage

    for (ssize_t i = 0; i < cnt; i++) {
        libusb_device *device = list[i];
        if (libusb_get_device_descriptor(device, &desc) == 0) {
            if ((desc.idVendor == QS1R_VID && desc.idProduct == QS1R_PID) ||
                (desc.idVendor == QS1R_MISSING_EEPROM_VID && desc.idProduct == QS1R_MISSING_EEPROM_PID)) {
                _debug() << "Found VID: " << std::hex << desc.idVendor << ", PID: " << std::hex << desc.idProduct
                             << std::dec;
                device_list_qs1r.push_back(device);
            }
        }
        QsIOLib_LibUSB ::usb_dev_count++;
    }

    if (device_list_qs1r.size() > 0 && index < device_list_qs1r.size()) {
        dev = device_list_qs1r[index];
        dev_was_found = true;
    }

    libusb_free_device_list(list, 1);

    if (dev_was_found) {
        QsIOLib_LibUSB ::qs1r_device_count = device_list_qs1r.size();
        return dev;
    } else {
        return nullptr;
    }
}

int QsIOLib_LibUSB ::deviceCount() { return QsIOLib_LibUSB ::usb_dev_count; }

int QsIOLib_LibUSB ::qs1rDeviceCount() { return QsIOLib_LibUSB ::qs1r_device_count; }

int QsIOLib_LibUSB ::loadFirmware(std::string filename) {
    if (!dev_was_found) {
        _debug() << "Need to call findDevice first.";
        return -1;
    }

    // PUT CPU IN RESET

    u_char value = 1;
    if (!write_cpu_ram(FX2_RAM_RESET, &value, 1)) {
        _debug() << "Could not put CPU in reset.";
        return -1;
    }

    File file(filename);

    if (!file.isValid()) {
        _debug() << "loadFirmware: filename does not exist.";
        return -1;
    }

    TextStream in(&file);

    while (!in.atEnd()) {
        std::string str;

        in >> str;

        if (str.substr(0, 1) != ":") {
            file.close();
            _debug() << "loadFirmware: error, firmware file appears to be corrupted";
            return -1;
        }

        bool ok = false;

        // Convert hex strings to integers
        try {
            int flength = std::stoi(str.substr(1, 2), nullptr, 16);
            int faddr = std::stoi(str.substr(3, 4), nullptr, 16);
            int type = std::stoi(str.substr(7, 2), nullptr, 16);

            if (type == 0) {
                std::string sstr = str.substr(9, flength * 2);
                std::vector<uint8_t> ba;

                for (size_t i = 0; i < sstr.length(); i += 2) {
                    ba.push_back(static_cast<uint8_t>(std::stoi(sstr.substr(i, 2), nullptr, 16)));
                }

                if (!write_cpu_ram(faddr, reinterpret_cast<u_char *>(ba.data()), flength)) {
                    _debug() << "loadFirmware: write failed.";
                    file.close();
                    return -1;
                }
            } else if (type == 0x01) {
                break;
            } else if (type == 0x02) {
                _debug() << "loadFirmware: extended address not supported.";
                file.close();
                return -1;
            }
        } catch (const std::invalid_argument &e) {
            _debug() << "loadFirmware: error in string conversion.";
            return -1;
        } catch (const std::out_of_range &e) {
            _debug() << "loadFirmware: value out of range.";
            return -1;
        }
        return 0;
    }

    return 0;
}

int QsIOLib_LibUSB ::loadFirmware(const char *firmware) {
    if (!dev_was_found) {
        _debug() << "Need to call findDevice first.";
        return -1;
    }

    // PUT CPU IN RESET
    if (cpuResetControl(true) == 0) {
        _debug() << "CPU reset command was sucessful!";
    } else {
        _debug() << "CPU reset command failed!";
        return -1;
    }

    // Use an istringstream to read from the embedded firmware string
    std::istringstream in(firmware);

    if (!in.good()) {
        _debug() << "Input stream initialization failed.";
        return -1;
    }

    std::string str;

    while (std::getline(in, str)) { // Read line by line from the stringstream
        _debug() << "Read line: [" << str << "]";
        if (str.substr(0, 1) != ":") {
            _debug() << "loadFirmware: error, firmware data appears to be corrupted";
            return -1;
        }

        bool ok = false;

        // Convert hex strings to integers
        try {
            u_int16_t flength = std::stoi(str.substr(1, 2), nullptr, 16);
            u_int16_t faddr = std::stoi(str.substr(3, 4), nullptr, 16);
            u_int16_t type = std::stoi(str.substr(7, 2), nullptr, 16);
            u_int16_t checksum = std::stoi(str.substr(9 + flength * 2, 2), nullptr, 16);

            uint8_t checksum_calc = calculateChecksum(str);

            _debug() << "flength: 0x" << std::hex << flength;
            _debug() << "faddr: 0x" << std::hex << faddr;
            _debug() << "type: 0x" << std::hex << type;
            _debug() << "checksum: 0x" << std::hex << checksum;
            _debug() << "Calculated checksum: 0x" << std::hex << std::uppercase << static_cast<int>(checksum_calc)
                        ;

            if (checksum != checksum_calc) {
                _debug() << "Checksum does not match!";
                return -1;
            } else {
                _debug() << "checksum ok";
            }

            if (type == 0) {
                std::string sstr = str.substr(9, flength * 2);
                std::vector<uint8_t> ba;

                for (size_t i = 0; i < sstr.length(); i += 2) {
                    ba.push_back(static_cast<uint8_t>(std::stoi(sstr.substr(i, 2), nullptr, 16)));
                }

                _debug() << "Data: " << printVectorInHex(ba);

                int result = write_cpu_ram(faddr, reinterpret_cast<u_char *>(ba.data()), flength);

                if (result < 0) {
                    _debug() << "loadFirmware: write failed." << libusb_error_name(result);
                    return -1;
                } else {
                    _debug() << "Write ram: " << result;
                }

            } else if (type == 0x01) {
                break; // End of firmware data
            } else if (type == 0x02) {
                _debug() << "loadFirmware: extended address not supported.";
                return -1;
            }
        } catch (const std::invalid_argument &e) {
            _debug() << "loadFirmware: error in string conversion.";
            return -1;
        } catch (const std::out_of_range &e) {
            _debug() << "loadFirmware: value out of range.";
            return -1;
        }
    }

    // TAKE CPU OUT OF RESET
    if (cpuResetControl(false) == 0) {
        _debug() << "CPU run command was sucessful!";
    } else {
        _debug() << "CPU run command failed!";
        return -1;
    }

    return 0; // Success
}

int QsIOLib_LibUSB ::cpuResetControl(bool reset) {

    u_int8_t data = reset ? 0x01 : 0x00;

    int result = write_cpu_ram(FX2_RAM_RESET, &data, 1);

    if (result <= 0) {
        if (reset) {
            _debug() << "Reset CPU failed: " << libusb_error_name(result);
            return -1;
        } else {
            _debug() << "Run CPU failed: " << libusb_error_name(result);
            return -1;
        }
    } else {
        if (reset) {
            _debug() << "CPU is in reset: " << result;
        } else {
            _debug() << "Run CPU OK: " << result;
        }
    }
    return 0;
}

int QsIOLib_LibUSB ::loadFpga(std::string filename) {
    if (!dev_was_found) {
        _debug() << "Need to call findDevice first.";
        return -1;
    }

    File file(filename);

    if (!file.isValid()) {
        _debug() << "loadFpga: filename does not exist.";
        return -1;
    }

    unsigned long count = 0;

    _debug() << "loadFPGA: Sending FL_BEGIN...";

    count = libusb_control_transfer(hdev, VRT_VENDOR_OUT, VRQ_FPGA_LOAD, 0, FL_BEGIN, 0, 0, USB_TIMEOUT_CONTROL);

    if (count != 0) {
        file.close();
        _debug() << "loadFpga: failed in FL_BEGIN load stage";
        return -1;
    }

    _debug() << "loadFPGA: Transferring FPGA Config...";

    while (!file.atEnd()) {
        std::vector<char> ba = file.read(MAX_EP4_PACKET_SIZE);

        int len = writeEP4((unsigned char *)ba.data(), ba.size());
        if (len != ba.size()) {
            file.close();
            _debug() << "loadFpga: failed in FL_XFER load stage";
            break;
        }
    }

    _debug() << "loadFPGA: Sending FL_END...";

    count = libusb_control_transfer(hdev, VRT_VENDOR_OUT, VRQ_FPGA_LOAD, 0, FL_END, 0, 0, USB_TIMEOUT_CONTROL);

    if (count != 0) {
        file.close();
        _debug() << "loadFpga: failed in FL_END load stage";
        return -1;
    }

    file.close();

    return 0;
}

int QsIOLib_LibUSB ::loadFpgaFromBitstream(const unsigned char *bitstream, unsigned int bitstream_size) {
    if (!dev_was_found) {
        _debug() << "Need to call findDevice first.";
        return -1;
    }

    if (!hdev) {
        _debug() << "Device handle is nullptr";
        return -1;
    }

    unsigned long count = 0;

    // Send FL_BEGIN signal
    _debug() << "loadFPGA: Sending FL_BEGIN...";
    count = libusb_control_transfer(hdev, VRT_VENDOR_OUT, VRQ_FPGA_LOAD, 0, FL_BEGIN, nullptr, 0, USB_TIMEOUT_CONTROL);

    if (count != 0) {
        _debug() << "loadFpga: failed in FL_BEGIN load stage";
        return -1;
    }

    // Send the FPGA bitstream data in chunks
    _debug() << "loadFPGA: Transferring FPGA Config...";

    const unsigned char *bitstream_ptr = bitstream;
    unsigned int remaining_bytes = bitstream_size;

    while (remaining_bytes > 0) {
        unsigned int chunk_size = (remaining_bytes < MAX_EP4_PACKET_SIZE) ? remaining_bytes : MAX_EP4_PACKET_SIZE;

        int len = writeEP4(const_cast<unsigned char *>(bitstream_ptr), chunk_size);
        if (len != chunk_size) {
            _debug() << "loadFpga: failed in FL_XFER load stage";
            return -1;
        }

        // Move the pointer forward and decrease the remaining bytes
        bitstream_ptr += chunk_size;
        remaining_bytes -= chunk_size;
    }

    // Send FL_END signal
    _debug() << "loadFPGA: Sending FL_END...";
    count = libusb_control_transfer(hdev, VRT_VENDOR_OUT, VRQ_FPGA_LOAD, 0, FL_END, nullptr, 0, USB_TIMEOUT_CONTROL);

    if (count != 0) {
        _debug() << "loadFpga: failed in FL_END load stage";
        return -1;
    }

    return 0;
}

int QsIOLib_LibUSB ::sendControlMessage(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index,
                                        u_char *buf, uint16_t size, unsigned int timeout) {
    return libusb_control_transfer(hdev, request_type, request, value, index, buf, size, timeout);
}

int QsIOLib_LibUSB ::readFwSn() {
    if (!dev_was_found) {
        _debug() << "Need to call findDevice first.";
        return -1;
    }

    if (!hdev) {
        _debug() << "Device handle is nullptr";
        return -1;
    }

    int count = 0;
    u_char buf[4];

    memset(buf, 0, sizeof(buf));

    count = libusb_control_transfer(hdev, VRT_VENDOR_IN, VRQ_SN_READ, 0, 0, buf, sizeof(buf), USB_TIMEOUT_CONTROL);

    if (count != sizeof(buf)) {
        _debug() << "readFwSn: control transfer failed: " << libusb_error_name(count);
        return -1;
    }

    return (int)((u_char)buf[0] + ((u_char)buf[1] << 8) + ((u_char)buf[2] << 16) + ((u_char)buf[3] << 24));
}

int QsIOLib_LibUSB ::read(unsigned int ep, unsigned char *buffer, unsigned int length, unsigned int timeout) {
    if (!dev_was_found) {
        _debug() << "Need to call findDevice first.";
        return -1;
    }

    int chanreg;

    if (ep == 6)
        chanreg = FX2_EP6;
    else if (ep == 8)
        chanreg = FX2_EP8;
    else {
        _debug() << "read: ep is invalid... (6/8 is valid)";
        return -1;
    }

    int transfered;
    int result = libusb_bulk_transfer(hdev, chanreg, (u_char *)buffer, length, &transfered, timeout);

    if (result != LIBUSB_SUCCESS) {
        _debug() << "read: could not read pipe.";
        return -1;
    }

    return transfered;
}

int QsIOLib_LibUSB ::write(unsigned int ep, unsigned char *buffer, unsigned int length, unsigned int timeout) {
    if (!dev_was_found) {
        _debug() << "Need to call findDevice first.";
        return -1;
    }

    int chanreg;
    unsigned long count;

    if (ep == 2)
        chanreg = FX2_EP2;
    else if (ep == 4)
        chanreg = FX2_EP4;
    else {
        _debug() << "write: ep is invalid... (2/4 is valid)";
        return -1;
    }

    int transfered;
    int result = libusb_bulk_transfer(hdev, chanreg, (u_char *)buffer, length, &transfered, timeout);

    if (result != LIBUSB_SUCCESS) {
        _debug() << "write: could not write pipe";
        return -1;
    }

    return transfered;
}

int QsIOLib_LibUSB ::readEP1(unsigned char *buffer, unsigned int length, unsigned int timeout) {
    if (!buffer || !dev_was_found)
        return -1;

    int transfered;
    int result = libusb_bulk_transfer(hdev, FX2_EP1_IN, (u_char *)buffer, length, &transfered, timeout);

    if (result != LIBUSB_SUCCESS) {
        // _debug() <<"read: could not read EP1.";
        if (libusb_clear_halt(hdev, FX2_EP1_IN) != LIBUSB_SUCCESS) {
            _debug() << "Could not clear halt on EP1";
        }
        return -1;
    }
    return transfered;
}

int QsIOLib_LibUSB ::writeEP1(unsigned char *buffer, unsigned int length, unsigned int timeout) {
    if (!buffer || !dev_was_found)
        return -1;

    int transfered;
    int result = libusb_bulk_transfer(hdev, FX2_EP1_OUT, buffer, length, &transfered, timeout);

    if (result != LIBUSB_SUCCESS) {
        // _debug() <<"write: could not write EP1";
        if (libusb_clear_halt(hdev, FX2_EP1_OUT) != LIBUSB_SUCCESS) {
            _debug() << "Could not clear halt on EP1";
        }
        return -1;
    }
    return transfered;
}

int QsIOLib_LibUSB ::writeEP2(unsigned char *buffer, unsigned int length, unsigned int timeout) {
    if (!buffer || !dev_was_found)
        return -1;

    int transfered;
    int result = libusb_bulk_transfer(hdev, FX2_EP2, buffer, length, &transfered, timeout);

    if (result != LIBUSB_SUCCESS) {
        _debug() << "write: could not write EP2";
        if (libusb_clear_halt(hdev, FX2_EP2) != LIBUSB_SUCCESS) {
            _debug() << "Could not clear halt on EP2";
        }
        return -1;
    }
    return transfered;
}

int QsIOLib_LibUSB ::writeEP4(unsigned char *buffer, unsigned int length, unsigned int timeout) {
    if (!buffer || !dev_was_found)
        return -1;

    int transfered;
    int result = libusb_bulk_transfer(hdev, FX2_EP4, buffer, length, &transfered, timeout);

    if (result != LIBUSB_SUCCESS) {
        _debug() << "write: could not write EP4";
        if (libusb_clear_halt(hdev, FX2_EP4) != LIBUSB_SUCCESS) {
            _debug() << "Could not clear halt on EP4";
        }
        return -1;
    }
    return transfered;
}

int QsIOLib_LibUSB ::readEP6(unsigned char *buffer, unsigned int length, unsigned int timeout) {
    if (!buffer || !dev_was_found)
        return -1;

    int transfered;
    int result = libusb_bulk_transfer(hdev, FX2_EP6, (u_char *)buffer, length, &transfered, timeout);

    if (result != LIBUSB_SUCCESS) {
        _debug() << "read: could not read EP6.";
        if (libusb_clear_halt(hdev, FX2_EP6) != 0) {
            _debug() << "Could not clear halt on EP6";
            return -1;
        }
        return -1;
    }
    return transfered;
}

int QsIOLib_LibUSB ::readEP8(unsigned char *buffer, unsigned int length, unsigned int timeout) {
    if (!buffer || !dev_was_found)
        return -1;

    int transfered;
    int result = libusb_bulk_transfer(hdev, FX2_EP8, (u_char *)buffer, length, &transfered, timeout);

    if (result != LIBUSB_SUCCESS) {
        _debug() << "read: could not read EP8.";
        if (libusb_clear_halt(hdev, FX2_EP8) != 0) {
            _debug() << "Could not clear halt on EP8";
            return -1;
        }
        return -1;
    }
    return transfered;
}

int QsIOLib_LibUSB ::readEEPROM(unsigned address, unsigned offset, unsigned char *buffer, unsigned int length) {
    if (!dev_was_found) {
        _debug() << "Need to call findDevice first.";
        return -1;
    }

    unsigned char cmd[2];

    if (length < 1)
        return -1;
    if (buffer == 0)
        return -1;

    cmd[0] = (char)((0xFF00 & offset) >> 8); // high byte address
    cmd[1] = (char)(0xFF & offset);          // low byte address

    // set address pointer in EEPROM
    if (writeI2C(address, cmd, 2) != 2) {
        _debug() << "readEEPROM: Could not set EEPROM address";
        return -1;
    }

    // now read from the address
    if (readI2C(address, buffer, length) != length) {
        _debug() << "readEEPROM: Could not read EEPROM device";
        return -1;
    }

    return length;
}

int QsIOLib_LibUSB ::writeEEPROM(unsigned int address, unsigned int offset, unsigned char *buffer,
                                 unsigned int length) {
    if (!dev_was_found) {
        _debug() << "Need to call findDevice first.";
        return -1;
    }

    unsigned char cmd[3];
    int i;

    if (length < 1)
        return -1;
    if (buffer == 0)
        return -1;

    for (i = 0; i < length; i++) {
        cmd[0] = (char)((0xFF00 & offset) >> 8); // high byte address
        cmd[1] = (char)(0xFF & offset);          // low byte address
        cmd[2] = (char)buffer[i];                // value to write
        // set address pointer in EEPROM
        if (writeI2C(address, cmd, 3) != 3) {
            _debug() << "writeEEPROM: Could not write EEPROM";
            return -1;
        }
        offset++;
        qssleep.msleep(10);
    }

    return length;
}

int QsIOLib_LibUSB ::readI2C(unsigned int address, unsigned char *buffer, unsigned int length) {
    if (!dev_was_found) {
        _debug() << "Need to call findDevice first.";
        return -1;
    }

    unsigned long count = 0;

    count = libusb_control_transfer(hdev, VRT_VENDOR_IN, VRQ_I2C_READ, address, 0, (u_char *)buffer, length,
                                    USB_TIMEOUT_CONTROL);

    if (count != (unsigned int)length) {
        _debug() << "readI2C: control transfer failed.";
        return -1;
    }

    return count;
}

int QsIOLib_LibUSB ::writeI2C(unsigned int address, unsigned char *buffer, unsigned int length) {
    if (!dev_was_found) {
        _debug() << "Need to call findDevice first.";
        return -1;
    }

    unsigned long count = 0;

    count = libusb_control_transfer(hdev, VRT_VENDOR_OUT, VRQ_REQ_I2C_WRITE, address, 0, (u_char *)buffer, length,
                                    USB_TIMEOUT_CONTROL);

    if (count != (unsigned int)length) {
        _debug() << "writeI2C: control transfer failed.";
        return -1;
    }

    return count;
}

int QsIOLib_LibUSB ::readMultibusInt(u_int16_t index) {
    if (!dev_was_found) {
        _debug() << "Need to call findDevice first.";
        return -1;
    }

    if (!hdev) {
        _debug() << "Device handle is nullptr";
        return -1;
    }

    u_char buf[4];

    memset(buf, 0, sizeof(buf));

    int count = libusb_control_transfer(hdev, VRT_VENDOR_IN, VRQ_MULTI_READ, index, 0, (u_char *)buf, sizeof(buf),
                                        USB_TIMEOUT_CONTROL);

    if (count != sizeof(buf)) {
        _debug() << "readMultibus: control transfer failed.";
        return -1;
    }

    return (int)((u_char)buf[0] + ((u_char)buf[1] << 8) + ((u_char)buf[2] << 16) + ((u_char)buf[3] << 24));
}

int QsIOLib_LibUSB ::readMultibusBuf(unsigned index, unsigned char *buffer, unsigned length) {
    if (!dev_was_found) {
        _debug() << "Need to call findDevice first.";
        return -1;
    }

    if (length != 4) {
        _debug() << "readMultibus: buffer must be length 4.";
        return -1;
    }

    unsigned long count = 0;
    unsigned long nsize = 4;

    count = libusb_control_transfer(hdev, VRT_VENDOR_IN, VRQ_MULTI_READ, index, 0, (u_char *)buffer, nsize,
                                    USB_TIMEOUT_CONTROL);

    if (count != nsize) {
        _debug() << "readMultibus: control transfer failed.";
        return -1;
    }

    return nsize;
}

int QsIOLib_LibUSB ::writeMultibusInt(unsigned int index, unsigned int value) {
    if (!dev_was_found) {
        _debug() << "Need to call findDevice first.";
        return -1;
    }

    unsigned long count = 0;
    unsigned char buf[4];
    unsigned long nsize = 4;

    buf[0] = (value >> 0) & 0xff;
    buf[1] = (value >> 8) & 0xff;
    buf[2] = (value >> 16) & 0xff;
    buf[3] = (value >> 24) & 0xff;

    count = libusb_control_transfer(hdev, VRT_VENDOR_OUT, VRQ_MULTI_WRITE, index, 0, (u_char *)buf, nsize,
                                    USB_TIMEOUT_CONTROL);

    if (count != nsize) {
        _debug() << "writeMultibus: control transfer failed.";
        return -1;
    }

    return count;
}

int QsIOLib_LibUSB ::writeMultibusBuf(unsigned index, unsigned char *buffer, unsigned length) {
    if (!dev_was_found) {
        _debug() << "Need to call findDevice first.";
        return -1;
    }

    if (length != 4) {
        _debug() << "writeMultibus: buffer must be length 4.";
        return -1;
    }

    unsigned long count = 0;
    unsigned long nsize = 4;

    count = libusb_control_transfer(hdev, VRT_VENDOR_OUT, VRQ_MULTI_WRITE, index, 0, (u_char *)buffer, nsize,
                                    USB_TIMEOUT_CONTROL);

    if (count != nsize) {
        _debug() << "writeMultibus: control transfer failed.";
        return -1;
    }

    return nsize;
}

int QsIOLib_LibUSB ::sendInterrupt5Gate() {
    if (hdev == 0)
        return -1;
    unsigned long count = 0;
    count = libusb_control_transfer(hdev, VRT_VENDOR_OUT, VRQ_INT5_READY, 0, 0, 0, 0, USB_TIMEOUT_CONTROL);
    return count;
}

int QsIOLib_LibUSB ::resetDevice() {
    if (!hdev)
        return -1;
    int count = libusb_control_transfer(hdev, VRT_VENDOR_OUT, VRQ_EP_RESET, 0, 0, 0, 0, USB_TIMEOUT_CONTROL);

    if (count < 0) {
        _debug() << "Result of device reset was: " << libusb_error_name(count);
        return -1;
    }
    return count;
}

int QsIOLib_LibUSB ::deviceWasFound() {
    if (QsIOLib_LibUSB ::dev_was_found)
        return 1;
    else
        return 0;
}

int QsIOLib_LibUSB ::write_cpu_ram(u_int16_t startaddr, u_char *buffer, u_int16_t length) {
    u_int pkt_size = MAX_EP0_PACKET_SIZE;
    u_int16_t addr = 0;
    int nsize = 0;
    int count = 0;

    for (addr = startaddr; addr < startaddr + length; addr += pkt_size) {
        nsize = length + startaddr - addr;
        if (nsize > pkt_size)
            nsize = pkt_size;

        count = libusb_control_transfer(hdev, VRT_VENDOR_OUT, FX2_WRITE_RAM_REQ, addr, 0,
                                        (u_char *)(buffer + (addr - startaddr)), (u_int16_t)nsize, USB_TIMEOUT_CONTROL);

        if (count != nsize) {
            _debug() << "write_cpu_ram error!" << libusb_error_name(count);
            return -1;
        }
    }
    return count;
}

std::string QsIOLib_LibUSB ::getDeviceClassString(uint8_t deviceClass) {
    switch (deviceClass) {
    case 0x00:
        return "Defined by the USB Specification";
    case 0x01:
        return "Audio";
    case 0x02:
        return "Communications and CDC Control";
    case 0x03:
        return "HID (Human Interface Device)";
    case 0x05:
        return "Physical Interface Device";
    case 0x06:
        return "Storage";
    case 0x07:
        return "Printer";
    case 0x08:
        return "Image";
    case 0x09:
        return "Media";
    case 0x0A:
        return "Smart Card";
    case 0x0B:
        return "Content Security";
    case 0x0D:
        return "Video";
    case 0x0E:
        return "Personal Healthcare";
    case 0x0F:
        return "Audio/Video Devices";
    case 0x10:
        return "Billboard Device Class";
    case 0x11:
        return "USB Type-C Bridge";
    // Add more cases as needed based on the USB specification
    default:
        return "Unknown Class";
    }
}

std::string QsIOLib_LibUSB ::getDeviceSubClassString(uint8_t deviceClass, uint8_t deviceSubClass) {
    if (deviceClass == 0x01) { // Audio
        switch (deviceSubClass) {
        case 0x01:
            return "Audio Control";
        case 0x02:
            return "Audio Streaming";
        case 0x03:
            return "MIDI Streaming";
        default:
            return "Unknown Audio Subclass";
        }
    } else if (deviceClass == 0x02) { // Communications
        switch (deviceSubClass) {
        case 0x01:
            return "Direct Line Connection";
        case 0x02:
            return "Abstract Control Model";
        case 0x03:
            return "Telephone Device Class";
        default:
            return "Unknown Communications Subclass";
        }
    } else if (deviceClass == 0x03) { // HID
        switch (deviceSubClass) {
        case 0x00:
            return "No Subclass";
        case 0x01:
            return "Boot Interface Subclass";
        default:
            return "Unknown HID Subclass";
        }
    } else if (deviceClass == 0x06) { // Mass Storage
        switch (deviceSubClass) {
        case 0x00:
            return "No Subclass";
        case 0x01:
            return "SCSI Transparent Command Set";
        case 0x02:
            return "UFI Command Set";
        case 0x03:
            return "SCSI Command Set";
        default:
            return "Unknown Mass Storage Subclass";
        }
    }
    // Add more classes and subclasses as needed based on the USB specification
    return "Unknown Subclass";
}

std::string QsIOLib_LibUSB ::getDeviceProtocolString(uint8_t deviceClass, uint8_t deviceSubClass,
                                                     uint8_t deviceProtocol) {
    if (deviceClass == 0x01) { // Audio
        switch (deviceSubClass) {
        case 0x01: // Audio Control
            switch (deviceProtocol) {
            case 0x00:
                return "None";
            case 0x01:
                return "Streaming";
            default:
                return "Unknown Audio Control Protocol";
            }
        case 0x02: // Audio Streaming
            return (deviceProtocol == 0x00) ? "General" : "Unknown Audio Streaming Protocol";
        case 0x03: // MIDI Streaming
            return (deviceProtocol == 0x00) ? "General" : "Unknown MIDI Protocol";
        default:
            return "Unknown Audio Subclass Protocol";
        }
    } else if (deviceClass == 0x02) { // Communications
        switch (deviceSubClass) {
        case 0x01: // Direct Line Connection
        case 0x02: // Abstract Control Model
            return (deviceProtocol == 0x00) ? "Common AT Commands" : "Unknown Communication Protocol";
        default:
            return "Unknown Communications Protocol";
        }
    } else if (deviceClass == 0x03) { // HID
        switch (deviceProtocol) {
        case 0x00:
            return "None";
        case 0x01:
            return "Keyboard";
        case 0x02:
            return "Mouse";
        default:
            return "Unknown HID Protocol";
        }
    } else if (deviceClass == 0x06) { // Mass Storage
        switch (deviceProtocol) {
        case 0x50:
            return "Bulk-Only Transport";
        default:
            return "Unknown Mass Storage Protocol";
        }
    }
    // Add more classes, subclasses, and protocols as needed based on the USB specification
    return "Unknown Protocol";
}
