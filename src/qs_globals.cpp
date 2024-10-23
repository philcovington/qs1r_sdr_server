#include "qs_globals.hpp"

// Define the static members
QS1RServer* QsGlobal::g_server = nullptr;
std::unique_ptr<QsDataReader> QsGlobal::g_data_reader = nullptr;
std::unique_ptr<QsDacWriter> QsGlobal::g_dac_writer = nullptr;
std::unique_ptr<QsCpxVectorCircularBuffer> QsGlobal::g_cpx_readin_ring = nullptr;
std::unique_ptr<QsCpxVectorCircularBuffer> QsGlobal::g_cpx_sd_ring = nullptr;
std::unique_ptr<QsFloatVectorCircularBuffer> QsGlobal::g_float_rt_ring = nullptr;
std::unique_ptr<QsFloatVectorCircularBuffer> QsGlobal::g_float_dac_ring = nullptr;
std::unique_ptr<QsIOLib_LibUSB> QsGlobal::g_io = std::make_unique<QsIOLib_LibUSB>();
std::unique_ptr<QsMemory> QsGlobal::g_memory = std::make_unique<QsMemory>();
bool QsGlobal::g_swap_iq = false;
bool QsGlobal::g_is_hardware_init = false;
