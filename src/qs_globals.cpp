#include "qs_globals.hpp"

// Custom deleter for libusb_device
void libusb_device_deleter(libusb_device* dev) {
    // Insert your cleanup logic here if necessary.
    // For example: libusb_unref_device(dev);
}

// Define the static members
std::unique_ptr<QsDataReader> QsGlobal::g_data_reader = nullptr;
std::unique_ptr<QsIOLib_LibUSB> QsGlobal::g_io = nullptr;
std::unique_ptr<libusb_device, void (*)(libusb_device *)> QsGlobal::g_device(nullptr, libusb_device_deleter);
std::unique_ptr<QsMemory> QsGlobal::g_memory = nullptr;
std::unique_ptr<QsCpxVectorCircularBuffer> QsGlobal::g_cpx_readin_ring = nullptr;
std::unique_ptr<QsCpxVectorCircularBuffer> QsGlobal::g_cpx_ps1_ring = nullptr;
std::unique_ptr<QsCpxVectorCircularBuffer> QsGlobal::g_cpx_ps2_ring = nullptr;
std::unique_ptr<QsCpxVectorCircularBuffer> QsGlobal::g_cpx_sd_ring = nullptr;
std::unique_ptr<QsFloatVectorCircularBuffer> QsGlobal::g_float_rt_ring = nullptr;
std::unique_ptr<QsFloatVectorCircularBuffer> QsGlobal::g_float_dac_ring = nullptr;
bool QsGlobal::g_swap_iq = false;
bool QsGlobal::g_is_hardware_init = false;
