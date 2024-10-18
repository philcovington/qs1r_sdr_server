#include "../include/qs_cpx_vector_cb.hpp"

#include "../include/qs_types.hpp"

QsCpxVectorCircularBuffer::QsCpxVectorCircularBuffer() : _size(0), _readPtr(0), _writePtr(0), _writeAvail(0) {}

void QsCpxVectorCircularBuffer::init(uint32_t size) {
    _size = size;
    _readPtr = 0;
    _writePtr = 0;
    _writeAvail = size;
    _cpx.resize(size);
    std::fill(_cpx.begin(), _cpx.end(), cpx_zero);
}

uint32_t QsCpxVectorCircularBuffer::read(qs_vect_cpx &rdata, uint32_t length) {
    uint32_t count = 0;

    if (length != 0)
        count = length;
    else
        count = rdata.size();

    uint32_t len = _size - _readPtr;

    if (count > len) {
        memcpy(&rdata[0], &_cpx[0] + _readPtr, sizeof(Cpx) * len);
        memcpy(&rdata[0] + len, &_cpx[0], (count - len) * sizeof(Cpx));
    } else {
        memcpy(&rdata[0], &_cpx[0] + _readPtr, count * sizeof(Cpx));
    }

    _readPtr = (_readPtr + count) % _size;
    _writeAvail += count;

    return count;
}

uint32_t QsCpxVectorCircularBuffer::write(qs_vect_cpx &wdata, uint32_t length) {
    uint32_t count = 0;

    if (length != 0)
        count = length;
    else
        count = wdata.size();

    uint32_t len = _size - _writePtr;

    if (count > len) {
        memcpy(&_cpx[0] + _writePtr, &wdata[0], len * sizeof(Cpx));
        memcpy(&_cpx[0], &wdata[0] + len, (count - len) * sizeof(Cpx));
    } else {
        memcpy(&_cpx[0] + _writePtr, &wdata[0], count * sizeof(Cpx));
    }

    _writePtr = (_writePtr + count) % _size;
    _writeAvail -= count;

    return count;
}

uint32_t QsCpxVectorCircularBuffer::size() { return _size; }

uint32_t QsCpxVectorCircularBuffer::writeAvail() { return _writeAvail; }

uint32_t QsCpxVectorCircularBuffer::readAvail() { return _size - _writeAvail; }

void QsCpxVectorCircularBuffer::empty() {
    _readPtr = 0;
    _writePtr = 0;
    _writeAvail = _size;
    std::fill(_cpx.begin(), _cpx.end(), cpx_zero);
}
