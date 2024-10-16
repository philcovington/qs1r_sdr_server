/*
 * File: qs_globals.hpp
 * Brief: Global definitions and external resources for QS1R system components.
 * 
 * This header file declares global variables and pointers that are shared across 
 * various QS1R system components. These include server communication, memory 
 * management, data handling, and I/O operations. The file uses smart pointers 
 * for automatic memory management and defines external `WaitCondition` objects 
 * for coordinating data flow and write operations.
 *
 * Features:
 * - Externally declared `WaitCondition` variables for data synchronization.
 * - Namespace `QsGlobal` containing smart pointers and flags used across the QS1R system.
 * - Smart pointer usage to manage resources for server, memory, data handling, and I/O.
 *
 * Usage:
 * Include this header in files that need access to the global resources for 
 * handling QS1R data processing, I/O, and synchronization tasks.
 *
 * Notes:
 * - The use of `std::unique_ptr` ensures automatic cleanup of resources when they go out of scope.
 * - `g_swap_iq` is used to control I/Q data swapping.
 * 
 * Author: Philip A Covington
 * Date: 2024-10-16
 */

#pragma once

#include "../include/qs1r_server.hpp"
#include "../include/qs_cpx_vector_cb.hpp"
#include "../include/qs_datareader.hpp"
#include "../include/qs_float_vector_cb.hpp"
#include "../include/qs_io_libusb.hpp"
#include "../include/qs_memory.hpp"
#include "../include/qs_wait_condition.hpp"
#include "../include/qs_mapclass.hpp"

#include <memory>

const double ONE_PI = 3.1415926535897932384626433832795;
const double TWO_PI = 6.283185307179586476925286766559;

extern WaitCondition WC_NEED_MORE_DATA;
extern WaitCondition WC_FILE_FIFO_WRITE;

Map<int, double> SMETERCORRECTMAP;
double SMETERCORRECT = 0.0;

namespace QsGlobal {
extern QS1RServer *g_server;
extern std::unique_ptr<QsDataReader> g_data_reader;
extern std::unique_ptr<QsIOLib_LibUSB> g_io;
extern std::unique_ptr<QsMemory> g_memory;
extern std::unique_ptr<QsCpxVectorCircularBuffer> g_cpx_readin_ring;
extern std::unique_ptr<QsCpxVectorCircularBuffer> g_cpx_ps1_ring;
extern std::unique_ptr<QsCpxVectorCircularBuffer> g_cpx_ps2_ring;
extern std::unique_ptr<QsCpxVectorCircularBuffer> g_cpx_sd_ring;
extern std::unique_ptr<QsFloatVectorCircularBuffer> g_float_rt_ring;
extern std::unique_ptr<QsFloatVectorCircularBuffer> g_float_dac_ring;
extern bool g_swap_iq;
}; // namespace QsGlobal
