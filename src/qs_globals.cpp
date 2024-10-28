#include "qs_globals.hpp"

// Define the static members
QS1RServer* QsGlobal::g_server = nullptr;
std::unique_ptr<QsMemory> QsGlobal::g_memory = std::make_unique<QsMemory>();
std::unique_ptr<QsDspProcessor> QsGlobal::g_dsp_proc = std::make_unique<QsDspProcessor>();
std::unique_ptr<QsIOLib_LibUSB> QsGlobal::g_io = std::make_unique<QsIOLib_LibUSB>();
bool QsGlobal::g_swap_iq = false;
bool QsGlobal::g_is_hardware_init = false;
