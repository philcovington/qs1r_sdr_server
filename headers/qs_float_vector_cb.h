/**
 * @file QsFloatVectorCircularBuffer.h
 * @brief A class for managing a circular buffer for floating-point numbers.
 *
 * The QsFloatVectorCircularBuffer class provides a circular buffer implementation
 * designed specifically for handling vectors of floating-point numbers. It supports
 * efficient reading and writing operations, making it suitable for real-time data
 * processing applications where floating-point data needs to be buffered.
 *
 * Features:
 * - Initialization of buffer size for dynamic memory management.
 * - Methods to read and write floating-point number vectors with circular buffer
 *   logic, allowing for efficient data handling.
 * - Support for reading and writing data using both vector and raw pointer forms.
 * - Functions to check the available read and write space in the buffer.
 * - Ability to clear the buffer and set block sizes as needed.
 *
 * Usage:
 * To use the QsFloatVectorCircularBuffer class, create an instance and initialize
 * it with the desired buffer size:
 *
 *   QsFloatVectorCircularBuffer buffer;
 *   buffer.init(1024); // Initialize with a size of 1024 floating-point numbers
 *
 * You can then read from and write to the buffer as follows:
 *
 *   qs_vect_f writeData; // Assume this is initialized with floating-point numbers
 *   buffer.write(writeData, writeData.size()); // Write floating-point numbers to buffer
 *
 *   qs_vect_f readData; // Buffer to hold read data
 *   buffer.read(readData, length); // Read floating-point numbers from buffer
 *
 * This class is particularly useful in signal processing, numerical analysis, and
 * other applications where floating-point data needs to be efficiently managed in a
 * circular buffer format.
 *
 * Author: [Philip A Covington]
 * Date: [2024-10-16]
 */

#pragma once

#include "../headers/qs_types.h"

class QsFloatVectorCircularBuffer {
  private:
    uint32_t _size;
    uint32_t _readPtr;
    uint32_t _writePtr;
    uint32_t _writeAvail;
    uint32_t m_blocksize;

    qs_vect_f _float;

  public:
    QsFloatVectorCircularBuffer();

    void init(uint32_t size);

    uint32_t read(qs_vect_f &rdata, uint32_t length);
    uint32_t read(float *rdata, uint32_t length);
    uint32_t write(qs_vect_f &wdata, uint32_t length);
    uint32_t write(float *wdata, uint32_t length);
    uint32_t size();
    uint32_t writeAvail();
    uint32_t readAvail();
    void empty();
    void setBlockSize(uint32_t value);
    uint32_t blockSize();
};