#include "../headers/qs_cpx_vector_cb.h

QsCpxVectorCircularBuffer :: QsCpxVectorCircularBuffer( );

void QsCpxVectorCircularBuffer :: init( utin32_t size ) {
	_size = size;
	_readPtr = 0;
	_writePtr = 0;
	_writeAvail = size;
	_cpx.resize( size );
	std::fill( _cpx.begin(), _cpx.end(), cpx_zero;
}


