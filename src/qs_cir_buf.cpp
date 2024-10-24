#include "../include/qs_circ_buf.hpp"

template <typename T>
QsCircularBuffer<T>::QsCircularBuffer() : _size(0), _readPtr(0), _writePtr(0), _writeAvail(0), m_blocksize(0) {}

template <typename T> void QsCircularBuffer<T>::init(uint32_t size) {
    _size = size;
    _buffer.assign(size, T{}); // Allocate and initialize with default value of T
    _readPtr = 0;
    _writePtr = 0;
    _writeAvail = size;
}

template <typename T> uint32_t QsCircularBuffer<T>::read(std::vector<T> &rdata, uint32_t length) {
    uint32_t availableToRead = readAvail();
    if (length == 0 || length > availableToRead) {
        length = availableToRead;
    }

    rdata.clear();
    rdata.reserve(length);

    uint32_t firstChunk = std::min(length, _size - _readPtr);
    uint32_t secondChunk = length - firstChunk;

    rdata.insert(rdata.end(), _buffer.begin() + _readPtr, _buffer.begin() + _readPtr + firstChunk);
    _readPtr = (_readPtr + firstChunk) % _size;

    if (secondChunk > 0) {
        rdata.insert(rdata.end(), _buffer.begin(), _buffer.begin() + secondChunk);
        _readPtr = secondChunk;
    }

    return length;
}

template <typename T> uint32_t QsCircularBuffer<T>::read(T *rdata, uint32_t length) {
    uint32_t availableToRead = readAvail();
    if (length == 0 || length > availableToRead) {
        length = availableToRead;
    }

    uint32_t firstChunk = std::min(length, _size - _readPtr);
    uint32_t secondChunk = length - firstChunk;

    std::copy(_buffer.begin() + _readPtr, _buffer.begin() + _readPtr + firstChunk, rdata);
    _readPtr = (_readPtr + firstChunk) % _size;

    if (secondChunk > 0) {
        std::copy(_buffer.begin(), _buffer.begin() + secondChunk, rdata + firstChunk);
        _readPtr = secondChunk;
    }

    return length;
}

template <typename T> uint32_t QsCircularBuffer<T>::write(const std::vector<T> &wdata, uint32_t length) {
    uint32_t availableToWrite = writeAvail();
    if (length == 0 || length > wdata.size()) {
        length = wdata.size();
    }
    if (length > availableToWrite) {
        length = availableToWrite;
    }

    uint32_t firstChunk = std::min(length, _size - _writePtr);
    uint32_t secondChunk = length - firstChunk;

    std::copy(wdata.begin(), wdata.begin() + firstChunk, _buffer.begin() + _writePtr);
    _writePtr = (_writePtr + firstChunk) % _size;

    if (secondChunk > 0) {
        std::copy(wdata.begin() + firstChunk, wdata.begin() + length, _buffer.begin());
        _writePtr = secondChunk;
    }

    _writeAvail -= length;

    return length;
}

template <typename T> uint32_t QsCircularBuffer<T>::write(const T *wdata, uint32_t length) {
    uint32_t availableToWrite = writeAvail();
    if (length > availableToWrite) {
        length = availableToWrite;
    }

    uint32_t firstChunk = std::min(length, _size - _writePtr);
    uint32_t secondChunk = length - firstChunk;

    std::copy(wdata, wdata + firstChunk, _buffer.begin() + _writePtr);
    _writePtr = (_writePtr + firstChunk) % _size;

    if (secondChunk > 0) {
        std::copy(wdata + firstChunk, wdata + length, _buffer.begin());
        _writePtr = secondChunk;
    }

    _writeAvail -= length;

    return length;
}

template <typename T> uint32_t QsCircularBuffer<T>::size() { return _size; }

template <typename T> uint32_t QsCircularBuffer<T>::writeAvail() { return _writeAvail; }

template <typename T> uint32_t QsCircularBuffer<T>::readAvail() { return _size - _writeAvail; }

template <typename T> void QsCircularBuffer<T>::empty() {
    _readPtr = 0;
    _writePtr = 0;
    _writeAvail = _size;
}

template <typename T> void QsCircularBuffer<T>::setBlockSize(uint32_t value) { m_blocksize = value; }

template <typename T> uint32_t QsCircularBuffer<T>::blockSize() { return m_blocksize; }
