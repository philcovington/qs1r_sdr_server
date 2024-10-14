// qs_cpx_vector_cb.h
#pragma once

#include "../headers/qs_types.h"
#include <complex>

static const std::complex<float> cpx_zero(0.0, 0.0);
static const std::complex<float> cpx_one(1.0, 1.0);

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
