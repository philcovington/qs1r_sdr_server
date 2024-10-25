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

// #pragma once

// #include <algorithm> // for std::copy
// #include <complex>
// #include <vector>

// template <typename T> class QsCircularBuffer {
//   private:
//     uint32_t _size;
//     uint32_t _readPtr;
//     uint32_t _writePtr;
//     uint32_t _writeAvail;
//     uint32_t m_blocksize;

//     std::vector<T> _buffer;

//   public:
//     QsCircularBuffer() : _size(0), _readPtr(0), _writePtr(0), _writeAvail(0), m_blocksize(0) {}

//     void init(uint32_t size) {
//         _size = size;
//         _buffer.assign(size, T{}); // Allocate and initialize with default value of T
//         _readPtr = 0;
//         _writePtr = 0;
//         _writeAvail = size;
//     }

//     uint32_t read(std::vector<T> &rdata, uint32_t length = 0) {
//         uint32_t availableToRead = readAvail();
//         if (length == 0 || length > availableToRead) {
//             length = availableToRead;
//         }

//         rdata.clear();
//         rdata.reserve(length);

//         uint32_t firstChunk = std::min(length, _size - _readPtr);
//         uint32_t secondChunk = length - firstChunk;

//         rdata.insert(rdata.end(), _buffer.begin() + _readPtr, _buffer.begin() + _readPtr + firstChunk);
//         _readPtr = (_readPtr + firstChunk) % _size;

//         if (secondChunk > 0) {
//             rdata.insert(rdata.end(), _buffer.begin(), _buffer.begin() + secondChunk);
//             _readPtr = secondChunk;
//         }

//         return length;
//     }

//     uint32_t read(T *rdata, uint32_t length = 0) {
//         uint32_t availableToRead = readAvail();
//         if (length == 0 || length > availableToRead) {
//             length = availableToRead;
//         }

//         uint32_t firstChunk = std::min(length, _size - _readPtr);
//         uint32_t secondChunk = length - firstChunk;

//         std::copy(_buffer.begin() + _readPtr, _buffer.begin() + _readPtr + firstChunk, rdata);
//         _readPtr = (_readPtr + firstChunk) % _size;

//         if (secondChunk > 0) {
//             std::copy(_buffer.begin(), _buffer.begin() + secondChunk, rdata + firstChunk);
//             _readPtr = secondChunk;
//         }

//         return length;
//     }

//     uint32_t write(const std::vector<T> &wdata, uint32_t length = 0) {
//         uint32_t availableToWrite = writeAvail();
//         if (length == 0 || length > wdata.size()) {
//             length = wdata.size();
//         }
//         if (length > availableToWrite) {
//             length = availableToWrite;
//         }

//         uint32_t firstChunk = std::min(length, _size - _writePtr);
//         uint32_t secondChunk = length - firstChunk;

//         std::copy(wdata.begin(), wdata.begin() + firstChunk, _buffer.begin() + _writePtr);
//         _writePtr = (_writePtr + firstChunk) % _size;

//         if (secondChunk > 0) {
//             std::copy(wdata.begin() + firstChunk, wdata.begin() + length, _buffer.begin());
//             _writePtr = secondChunk;
//         }

//         _writeAvail -= length;

//         return length;
//     }

//     uint32_t write(const T *wdata, uint32_t length = 0) {
//         uint32_t availableToWrite = writeAvail();
//         if (length > availableToWrite) {
//             length = availableToWrite;
//         }

//         uint32_t firstChunk = std::min(length, _size - _writePtr);
//         uint32_t secondChunk = length - firstChunk;

//         std::copy(wdata, wdata + firstChunk, _buffer.begin() + _writePtr);
//         _writePtr = (_writePtr + firstChunk) % _size;

//         if (secondChunk > 0) {
//             std::copy(wdata + firstChunk, wdata + length, _buffer.begin());
//             _writePtr = secondChunk;
//         }

//         _writeAvail -= length;

//         return length;
//     }

//     uint32_t size() { return _size; }

//     uint32_t writeAvail() { return _writeAvail; }

//     uint32_t readAvail() { return _size - _writeAvail; }

//     void empty() {
//         _readPtr = 0;
//         _writePtr = 0;
//         _writeAvail = _size;
//     }

//     void setBlockSize(uint32_t value) { m_blocksize = value; }

//     uint32_t blockSize() { return m_blocksize; }
// };

#pragma once

#include <algorithm>
#include <cstring>
#include <mutex>
#include <vector>

template <typename T> class QsCircularBuffer {
  public:
    QsCircularBuffer() : _size(0), _readPtr(0), _writePtr(0), _writeAvail(0), m_blocksize(0) {}

    void init(unsigned int size) {
        MX.lock();
        _size = size;
        _readPtr = 0;
        _writePtr = 0;
        _writeAvail = size;
        _buffer.resize(size);
        std::fill(_buffer.begin(), _buffer.end(), T());
        m_blocksize = 0;
        MX.unlock();
    }

    unsigned int read(std::vector<T> &rdata, unsigned int length) {
        unsigned int count = (length != static_cast<unsigned int>(-1)) ? length : rdata.size();
        unsigned int len = _size - _readPtr;

        MX.lock();

        if (count > len) {
            std::memcpy(&rdata[0], &_buffer[0] + _readPtr, sizeof(T) * len);
            std::memcpy(&rdata[0] + len, &_buffer[0], (count - len) * sizeof(T));
        } else {
            std::memcpy(&rdata[0], &_buffer[0] + _readPtr, count * sizeof(T));
        }

        _readPtr = (_readPtr + count) % _size;
        _writeAvail += count;

        MX.unlock();

        return count;
    }

    unsigned int read(T *rdata, unsigned int count) {
        unsigned int len = _size - _readPtr;

        MX.lock();

        if (count > len) {
            std::memcpy(rdata, &_buffer[0] + _readPtr, sizeof(T) * len);
            std::memcpy(rdata + len, &_buffer[0], (count - len) * sizeof(T));
        } else {
            std::memcpy(rdata, &_buffer[0] + _readPtr, count * sizeof(T));
        }

        _readPtr = (_readPtr + count) % _size;
        _writeAvail += count;

        MX.unlock();

        return count;
    }

    unsigned int write(const std::vector<T> &wdata, unsigned int length) {
        unsigned int count = (length != static_cast<unsigned int>(-1)) ? length : wdata.size();
        unsigned int len = _size - _writePtr;

        MX.lock();

        if (count > len) {
            std::memcpy(&_buffer[0] + _writePtr, &wdata[0], len * sizeof(T));
            std::memcpy(&_buffer[0], &wdata[0] + len, (count - len) * sizeof(T));
        } else {
            std::memcpy(&_buffer[0] + _writePtr, &wdata[0], count * sizeof(T));
        }

        _writePtr = (_writePtr + count) % _size;
        _writeAvail -= count;

        MX.unlock();

        return count;
    }

    unsigned int write(const T *wdata, unsigned int count) {
        unsigned int len = _size - _writePtr;

        MX.lock();

        if (count > len) {
            std::memcpy(&_buffer[0] + _writePtr, wdata, len * sizeof(T));
            std::memcpy(&_buffer[0], wdata + len, (count - len) * sizeof(T));
        } else {
            std::memcpy(&_buffer[0] + _writePtr, wdata, count * sizeof(T));
        }

        _writePtr = (_writePtr + count) % _size;
        _writeAvail -= count;

        MX.unlock();

        return count;
    }

    unsigned int size() const { return _size; }

    unsigned int writeAvail() const { return _writeAvail; }

    unsigned int readAvail() const { return _size - _writeAvail; }

    void empty() {
        MX.lock();
        _readPtr = 0;
        _writePtr = 0;
        _writeAvail = _size;
        std::fill(_buffer.begin(), _buffer.end(), T());
        MX.unlock();
    }

    void setBlockSize(unsigned int value) { m_blocksize = value; }

    unsigned int blockSize() const { return m_blocksize; }

  private:
    unsigned int _size;
    unsigned int _readPtr;
    unsigned int _writePtr;
    unsigned int _writeAvail;
    unsigned int m_blocksize;
    std::vector<T> _buffer;
    std::mutex MX;
};

// Example usage:
// QsVectorCircularBuffer<float> floatBuffer;
// QsVectorCircularBuffer<std::complex<float>> complexBuffer;
