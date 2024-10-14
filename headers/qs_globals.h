// qs_globals.h
#pragma once

#include <../headers/qs1rserver.h>
#include <../headers/qs_io_libusb.h>

#include <condition_variable>
#include <mutex>

#include "../headers/qs_cpx_vector_circularbuffer.h"
#include "../headers/qs_float_vector_circularbuffer.h"

QWaitCondition WC_FILE_FIFO_WRITE;

namespace QsGlobal {
extern QS1RServer *g_server;
extern auto_ptr<QsDataReader> g_data_reader;
extern auto_ptr<QsIOLib_LibUSB> g_io;
extern auto_ptr<QsMemory> g_memory;
extern auto_ptr<QsCpxVectorCircularBuffer> g_cpx_readin_ring;
extern auto_ptr<QsFloatVectorCircularBuffer> g_float_rt_ring;
}; // namespace QsGlobal
