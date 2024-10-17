/**
 * @file QsCpxVectorCircularBuffer.h
 * @brief A class for managing a circular buffer for complex numbers.
 *
 * The QsCpxVectorCircularBuffer class provides a circular buffer implementation 
 * designed specifically for handling vectors of complex numbers. It supports 
 * efficient reading and writing operations, making it suitable for real-time 
 * data processing applications where data needs to be buffered.
 *
 * Features:
 * - Initialization of buffer size for dynamic memory management.
 * - Methods to read and write complex number vectors with circular buffer 
 *   logic, allowing for efficient data handling.
 * - Functions to check the available read and write space in the buffer.
 * - Ability to clear the buffer when needed.
 *
 * Usage:
 * To use the QsCpxVectorCircularBuffer class, create an instance and initialize 
 * it with the desired buffer size:
 *
 *   QsCpxVectorCircularBuffer buffer;
 *   buffer.init(1024); // Initialize with a size of 1024 complex numbers
 *
 * You can then read from and write to the buffer as follows:
 *
 *   qs_vect_cpx writeData; // Assume this is initialized with complex numbers
 *   buffer.write(writeData, writeData.size()); // Write complex numbers to buffer
 *
 *   qs_vect_cpx readData; // Buffer to hold read data
 *   buffer.read(readData, length); // Read complex numbers from buffer
 *
 * This class is particularly useful in signal processing, communications, and 
 * other applications where complex number data needs to be efficiently managed 
 * in a circular buffer format.
 *
 * Author: [Philip A Covington]
 * Date: [2024-10-16]
 */

#pragma once

#include "../headers/qs_dataproc.h"
#include "../headers/qs_types.h"
#include <complex>

class QsCpxVectorCircularBuffer {

  private:
    uint32_t _size;
    uint32_t _readPtr;
    uint32_t _writePtr;
    uint32_t _writeAvail;

    qs_vect_cpx _cpx;

  public:
    QsCpxVectorCircularBuffer();

    void init(uint32_t size);

    uint32_t read(qs_vect_cpx &rdata, uint32_t length=0);
    uint32_t write(qs_vect_cpx &wdata, uint32_t length=0);
    uint32_t size();
    uint32_t writeAvail();
    uint32_t readAvail();
    void empty();
};
