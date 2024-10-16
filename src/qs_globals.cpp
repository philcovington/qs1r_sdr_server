#include "../headers/qs_globals.h"

WaitCondition WC_NEED_MORE_DATA;
WaitCondition WC_FILE_FIFO_WRITE;

namespace QsGlobal {
QS1R_server *g_server = nullptr;
std::unique_ptr<QsDataReader> g_data_reader(new QsDataReader);
std::unique_ptr<QsIOLib_LibUSB> g_io(new QsIOLib_LibUSB);
std::unique_ptr<QsMemory> g_memory(new QsMemory());
std::unique_ptr<QsCpxVectorCircularBuffer> g_cpx_readin_ring(new QsCpxVectorCircularBuffer);
std::unique_ptr<QsFloatVectorCircularBuffer> g_float_rt_ring(new QsFloatVectorCircularBuffer);
bool g_swap_iq = false; 
} // namespace QsGlobal
