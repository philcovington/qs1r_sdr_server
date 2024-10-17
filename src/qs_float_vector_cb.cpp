#include "../headers/qs_float_vector_cb.h"

#include "../headers/qs_types.h"
#include <memory.h>

QsFloatVectorCircularBuffer::QsFloatVectorCircularBuffer()
    : _size(0), _readPtr(0), _writePtr(0), _writeAvail(0), m_blocksize(0) {}

void QsFloatVectorCircularBuffer::init(uint32_t size) {
    _size = size;
    _readPtr = 0;
    _writePtr = 0;
    _writeAvail = size;
    _float.resize(size);
    std::fill(_float.begin(), _float.end(), 0.0);
    m_blocksize = 0;
}

uint32_t QsFloatVectorCircularBuffer::read(qs_vect_f &rdata, uint32_t length) {
    uint32_t count = 0;

    if (length != 0)
        count = length;
    else
        count = rdata.size();

    uint32_t len = _size - _readPtr;

    if (count > len) {
        memcpy(&rdata[0], &_float[0] + _readPtr, sizeof(float) * len);
        memcpy(&rdata[0] + len, &_float[0], (count - len) * sizeof(float));
    } else {
        memcpy(&rdata[0], &_float[0] + _readPtr, count * sizeof(float));
    }

    _readPtr = (_readPtr + count) % _size;
    _writeAvail += count;

    return count;
}

uint32_t QsFloatVectorCircularBuffer::read(float *rdata, uint32_t count) {
    uint32_t len = _size - _readPtr;

    if (count > len) {
        memcpy(rdata, &_float[0] + _readPtr, sizeof(float) * len);
        memcpy(rdata + len, &_float[0], (count - len) * sizeof(float));
    } else {
        memcpy(rdata, &_float[0] + _readPtr, count * sizeof(float));
    }

    _readPtr = (_readPtr + count) % _size;
    _writeAvail += count;

    return count;
}

uint32_t QsFloatVectorCircularBuffer::write(qs_vect_f &wdata, uint32_t length) {
    uint32_t count = 0;

    if (length != 0)
        count = length;
    else
        count = wdata.size();

    uint32_t len = _size - _writePtr;

    if (count > len) {
        memcpy(&_float[0] + _writePtr, &wdata[0], len * sizeof(float));
        memcpy(&_float[0], &wdata[0] + len, (count - len) * sizeof(float));
    } else {
        memcpy(&_float[0] + _writePtr, &wdata[0], count * sizeof(float));
    }

    _writePtr = (_writePtr + count) % _size;
    _writeAvail -= count;

    return count;
}

uint32_t QsFloatVectorCircularBuffer::write(float *wdata, uint32_t count) {
    uint32_t len = _size - _writePtr;

    if (count > len) {
        memcpy(&_float[0] + _writePtr, wdata, len * sizeof(float));
        memcpy(&_float[0], wdata + len, (count - len) * sizeof(float));
    } else {
        memcpy(&_float[0] + _writePtr, wdata, count * sizeof(float));
    }

    _writePtr = (_writePtr + count) % _size;
    _writeAvail -= count;

    return count;
}

uint32_t QsFloatVectorCircularBuffer::size() { return _size; }

uint32_t QsFloatVectorCircularBuffer::writeAvail() { return _writeAvail; }

uint32_t QsFloatVectorCircularBuffer::readAvail() { return _size - _writeAvail; }

void QsFloatVectorCircularBuffer::empty() {
    _readPtr = 0;
    _writePtr = 0;
    _writeAvail = _size;
    std::fill(_float.begin(), _float.end(), 0.0);
}

void QsFloatVectorCircularBuffer::setBlockSize(uint32_t value) { m_blocksize = value; }

uint32_t QsFloatVectorCircularBuffer::blockSize() { return m_blocksize; }
