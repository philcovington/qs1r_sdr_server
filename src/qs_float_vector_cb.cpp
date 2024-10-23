// #include "../include/qs_float_vector_cb.hpp"

// #include "../include/qs_types.hpp"
// #include <memory.h>

// QsFloatVectorCircularBuffer::QsFloatVectorCircularBuffer()
//     : _size(0), _readPtr(0), _writePtr(0), _writeAvail(0), m_blocksize(0) {}

// void QsFloatVectorCircularBuffer::init(uint32_t size) {
//     _size = size;
//     _readPtr = 0;
//     _writePtr = 0;
//     _writeAvail = size;
//     _float.resize(size);
//     std::fill(_float.begin(), _float.end(), 0.0);
//     m_blocksize = 0;
// }

// uint32_t QsFloatVectorCircularBuffer::read(qs_vect_f &rdata, uint32_t length) {
//     uint32_t count = 0;

//     if (length != 0)
//         count = length;
//     else
//         count = rdata.size();

//     uint32_t len = _size - _readPtr;

//     if (count > len) {
//         memcpy(&rdata[0], &_float[0] + _readPtr, sizeof(float) * len);
//         memcpy(&rdata[0] + len, &_float[0], (count - len) * sizeof(float));
//     } else {
//         memcpy(&rdata[0], &_float[0] + _readPtr, count * sizeof(float));
//     }

//     _readPtr = (_readPtr + count) % _size;
//     _writeAvail += count;

//     return count;
// }

// uint32_t QsFloatVectorCircularBuffer::read(float *rdata, uint32_t count) {
//     uint32_t len = _size - _readPtr;

//     if (count > len) {
//         memcpy(rdata, &_float[0] + _readPtr, sizeof(float) * len);
//         memcpy(rdata + len, &_float[0], (count - len) * sizeof(float));
//     } else {
//         memcpy(rdata, &_float[0] + _readPtr, count * sizeof(float));
//     }

//     _readPtr = (_readPtr + count) % _size;
//     _writeAvail += count;

//     return count;
// }

// uint32_t QsFloatVectorCircularBuffer::write(qs_vect_f &wdata, uint32_t length) {
//     uint32_t count = 0;

//     if (length != 0)
//         count = length;
//     else
//         count = wdata.size();

//     uint32_t len = _size - _writePtr;

//     if (count > len) {
//         memcpy(&_float[0] + _writePtr, &wdata[0], len * sizeof(float));
//         memcpy(&_float[0], &wdata[0] + len, (count - len) * sizeof(float));
//     } else {
//         memcpy(&_float[0] + _writePtr, &wdata[0], count * sizeof(float));
//     }

//     _writePtr = (_writePtr + count) % _size;
//     _writeAvail -= count;

//     return count;
// }

// uint32_t QsFloatVectorCircularBuffer::write(float *wdata, uint32_t count) {
//     uint32_t len = _size - _writePtr;

//     if (count > len) {
//         memcpy(&_float[0] + _writePtr, wdata, len * sizeof(float));
//         memcpy(&_float[0], wdata + len, (count - len) * sizeof(float));
//     } else {
//         memcpy(&_float[0] + _writePtr, wdata, count * sizeof(float));
//     }

//     _writePtr = (_writePtr + count) % _size;
//     _writeAvail -= count;

//     return count;
// }

// uint32_t QsFloatVectorCircularBuffer::size() { return _size; }

// uint32_t QsFloatVectorCircularBuffer::writeAvail() { return _writeAvail; }

// uint32_t QsFloatVectorCircularBuffer::readAvail() { return _size - _writeAvail; }

// void QsFloatVectorCircularBuffer::empty() {
//     _readPtr = 0;
//     _writePtr = 0;
//     _writeAvail = _size;
//     std::fill(_float.begin(), _float.end(), 0.0);
// }

// void QsFloatVectorCircularBuffer::setBlockSize(uint32_t value) { m_blocksize = value; }

// uint32_t QsFloatVectorCircularBuffer::blockSize() { return m_blocksize; }

#include "../include/qs_float_vector_cb.hpp"
#include <algorithm> // for std::copy

QsFloatVectorCircularBuffer::QsFloatVectorCircularBuffer()
    : _size(0), _readPtr(0), _writePtr(0), _writeAvail(0), m_blocksize(0) {}

void QsFloatVectorCircularBuffer::init(uint32_t size) {
    _size = size;
    _float.assign(size, 0.0f); // Allocate and zero-initialize
    _readPtr = 0;
    _writePtr = 0;
    _writeAvail = size;
}

uint32_t QsFloatVectorCircularBuffer::read(qs_vect_f &rdata, uint32_t length) {
    uint32_t availableToRead = readAvail();
    if (length == 0 || length > availableToRead) {
        length = availableToRead;
    }

    rdata.clear();
    rdata.reserve(length);

    // Handle wrap-around
    uint32_t firstChunk = std::min(length, _size - _readPtr);
    uint32_t secondChunk = length - firstChunk;

    rdata.insert(rdata.end(), _float.begin() + _readPtr, _float.begin() + _readPtr + firstChunk);
    _readPtr = (_readPtr + firstChunk) % _size;

    if (secondChunk > 0) {
        rdata.insert(rdata.end(), _float.begin(), _float.begin() + secondChunk);
        _readPtr = secondChunk;
    }

    return length;
}

uint32_t QsFloatVectorCircularBuffer::read(float *rdata, uint32_t length) {
    uint32_t availableToRead = readAvail();
    if (length == 0 || length > availableToRead) {
        length = availableToRead;
    }

    uint32_t firstChunk = std::min(length, _size - _readPtr);
    uint32_t secondChunk = length - firstChunk;

    std::copy(_float.begin() + _readPtr, _float.begin() + _readPtr + firstChunk, rdata);
    _readPtr = (_readPtr + firstChunk) % _size;

    if (secondChunk > 0) {
        std::copy(_float.begin(), _float.begin() + secondChunk, rdata + firstChunk);
        _readPtr = secondChunk;
    }

    return length;
}

uint32_t QsFloatVectorCircularBuffer::write(qs_vect_f &wdata, uint32_t length) {
    uint32_t availableToWrite = writeAvail();
    if (length == 0 || length > wdata.size()) {
        length = wdata.size();
    }
    if (length > availableToWrite) {
        length = availableToWrite;
    }

    // Handle wrap-around
    uint32_t firstChunk = std::min(length, _size - _writePtr);
    uint32_t secondChunk = length - firstChunk;

    std::copy(wdata.begin(), wdata.begin() + firstChunk, _float.begin() + _writePtr);
    _writePtr = (_writePtr + firstChunk) % _size;

    if (secondChunk > 0) {
        std::copy(wdata.begin() + firstChunk, wdata.begin() + length, _float.begin());
        _writePtr = secondChunk;
    }

    _writeAvail -= length;

    return length;
}

uint32_t QsFloatVectorCircularBuffer::write(float *wdata, uint32_t length) {
    uint32_t availableToWrite = writeAvail();
    if (length > availableToWrite) {
        length = availableToWrite;
    }

    uint32_t firstChunk = std::min(length, _size - _writePtr);
    uint32_t secondChunk = length - firstChunk;

    std::copy(wdata, wdata + firstChunk, _float.begin() + _writePtr);
    _writePtr = (_writePtr + firstChunk) % _size;

    if (secondChunk > 0) {
        std::copy(wdata + firstChunk, wdata + length, _float.begin());
        _writePtr = secondChunk;
    }

    _writeAvail -= length;

    return length;
}

uint32_t QsFloatVectorCircularBuffer::size() { return _size; }

uint32_t QsFloatVectorCircularBuffer::writeAvail() { return _writeAvail; }

uint32_t QsFloatVectorCircularBuffer::readAvail() { return _size - _writeAvail; }

void QsFloatVectorCircularBuffer::empty() {
    _readPtr = 0;
    _writePtr = 0;
    _writeAvail = _size;
}

void QsFloatVectorCircularBuffer::setBlockSize(uint32_t value) { m_blocksize = value; }

uint32_t QsFloatVectorCircularBuffer::blockSize() { return m_blocksize; }
