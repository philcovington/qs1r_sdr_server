// qs_globals.h
#pragma once

#include "../headers/qs1r_server.h"
#include "../headers/qs_cpx_vector_cb.h"
#include "../headers/qs_datareader.h"
#include "../headers/qs_float_vector_cb.h"
#include "../headers/qs_io_libusb.h"
#include "../headers/qs_memory.h"
#include "../headers/qs_wait_condition.h"

#include <memory>

extern WaitCondition WC_NEED_MORE_DATA;
extern WaitCondition WC_FILE_FIFO_WRITE;

namespace QsGlobal {
extern QS1R_server *g_server;
extern std::unique_ptr<QsDataReader> g_data_reader;
extern std::unique_ptr<QsIOLib_LibUSB> g_io;
extern std::unique_ptr<QsMemory> g_memory;
extern std::unique_ptr<QsCpxVectorCircularBuffer> g_cpx_readin_ring;
extern std::unique_ptr<QsFloatVectorCircularBuffer> g_float_rt_ring;
extern bool g_swap_iq;
}; // namespace QsGlobal
