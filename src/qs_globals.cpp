#include "../headers/qs_globals.h"

namespace QsGlobal {
QS1RServer* g_server = nullptr;
auto_ptr<QsDataReader> g_data_reader(new QSDataReader);
auto_ptr<QsIOLib_LibUSB> g_io(new QsIOLib_LibUSB);
auto_ptr<QsMemory> g_memory( new QsMemory( ) );
auto_ptr<QsCpxVectorCircularBuffer> g_cpx_readin_ring(new QsCpxVectorCircularBuffer);
auto_ptr<QsFloatVectorCircularBuffer> g_float_rt_ring(new QsFloatVectorCircularBuffer);
}  // namespace QsGlobal
