/**
 * @file    QsCircularBuffer.hpp
 * @brief   Template-based Circular Buffer for various data types.
 * 
 * This header defines a template class `QsCircularBuffer<T>` for creating a circular 
 * buffer that can store and handle different types of data, such as `float`, 
 * `double`, and `std::complex<float>`. The buffer supports reading and writing 
 * operations with automatic handling of wrap-around when the end of the buffer is 
 * reached. 
 * 
 * Features:
 * - Supports arbitrary data types via templating.
 * - Efficient read and write operations using block copying.
 * - Handles circular buffer wrap-around internally.
 * - Provides methods for checking available space for reading and writing.
 * - Can be configured with a custom block size.
 * 
 * Usage:
 * ```
 * QsCircularBuffer<float> floatBuffer;
 * floatBuffer.init(100);
 * 
 * QsCircularBuffer<std::complex<float>> complexBuffer;
 * complexBuffer.init(100);
 * ```
 * 
 * Notes:
 * - The buffer size must be initialized before performing read or write operations.
 * - The template allows the buffer to work with both fundamental types (e.g., `float`, 
 *   `double`) and complex types (e.g., `std::complex<float>`).
 * 
 * @author  Philip A Covington
 * @date    2024-10-24
 */

#pragma once

#include <vector>
#include <complex>
#include <algorithm> // for std::copy

template <typename T>
class QsCircularBuffer {
  private:
    uint32_t _size;
    uint32_t _readPtr;
    uint32_t _writePtr;
    uint32_t _writeAvail;
    uint32_t m_blocksize;

    std::vector<T> _buffer;

  public:
    QsCircularBuffer();

    void init(uint32_t size);

    uint32_t read(std::vector<T> &rdata, uint32_t length=0);
    uint32_t read(T *rdata, uint32_t length=0);
    uint32_t write(const std::vector<T> &wdata, uint32_t length=0);
    uint32_t write(const T *wdata, uint32_t length=0);
    uint32_t size();
    uint32_t writeAvail();
    uint32_t readAvail();
    void empty();
    void setBlockSize(uint32_t value);
    uint32_t blockSize();
};
