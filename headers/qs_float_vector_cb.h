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