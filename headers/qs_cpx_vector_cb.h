#pragma once

#include <complex>

static const complex<float> cpx_zero( 0.0, 0.0 );
static const complex<float> cpx_one( 1.0, 1.0 );

class QsCpxVectorCircularBuffer {

private:
	uint32_t _size;
	uint32_t _readPtr;
	uint32_t _writePtr;
	uint32_t _writeAvail;

	std::vector<complex<float>> _cpx;

public: 

	QsCpxVectorCircularBuffer( );

	void init( uint32_t size );

	uint32_t read( qs_vector_cpx & rdata, uint32_t length );
	uint32_t write( qs_vect_cpx & wdata, uint32_t length );
	uint32_t size( );
	uint32_t writeAvail( );
	uint32_t readAvail( );
	void empty( );

};
