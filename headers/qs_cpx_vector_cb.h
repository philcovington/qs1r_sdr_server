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

    uint32_t read(qs_vect_cpx &rdata, uint32_t length);
    uint32_t write(qs_vect_cpx &wdata, uint32_t length);
    uint32_t size();
    uint32_t writeAvail();
    uint32_t readAvail();
    void empty();
};
