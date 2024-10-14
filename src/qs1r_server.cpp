#include <iostream>
#include <vector>
#include <string>

#include "../headers/qs1r_server.h"
#include "../headers/config.h"
#include "../headers/qs_sleep.h"
#include "../headers/qs_io_libusb.h"
#include "../headers/Firmware.h"
#include "../headers/bitstream.h"
#include "../headers/ByteArray.h"

QS1R_server :: QS1R_server() {
	QsSleep sleep;
	QsIOLib_LibUSB usb;
}

QS1R_server :: ~QS1R_server() {
	
}

void QS1R_server :: exit( ) {
	usb.exit( );
}

void QS1R_server :: showStartupMsg() {
	std::cout << "This is the QS1R Server version: " << VERSION << std::endl;
}

void QS1R_server :: initSupportedSampleRatesList() {
	m_supported_samplerates.push_back( "50000" );
	m_supported_samplerates.push_back( "125000" );
	m_supported_samplerates.push_back( "250000" );
}

std::vector<std::string> QS1R_server :: getSupportedSampleRates( )
{
	return m_supported_samplerates;
}

libusb_device * QS1R_server :: findQS1RDevice( QsIOLib_LibUSB *usb, u_int16_t vid, uint16_t pid, unsigned int index) {
    libusb_device *device = nullptr;
    device =  usb->findQsDevice( QS1R_VID, QS1R_PID, 0);
    if ( device ) {
        return device;
    } else {
        return nullptr;
    }
}

// -------------------------------------------------------------
// Initializes the QS1R Hardware
// Loads the firmware and the fpga bit stream
// -------------------------------------------------------------
int QS1R_server :: initQS1RHardware( unsigned int index=0 ) {
	if ( m_hardware_is_init ) {
		// close the libusb handle
	}
	m_hardware_is_init = false;

	std::cout << "Trying the libusb driver with index [" << index << "]... wait..." << std::endl;
	
	libusb_device *dev = findQS1RDevice( &usb, QS1R_VID, QS1R_PID, 0);

	if ( dev == nullptr ) {
        std::cerr << "Could not find any QS1R devices!" << std::endl;
        return -1;
    }

    std::cout << "QS1R Device count: " << usb.qs1rDeviceCount( ) << std::endl;
    std::cout << "Device count: " << usb.deviceCount( ) << std::endl;

	int ret = usb.open( dev );

	if ( ret == 0 ) {
	    int fpga_id = 0;
	    int fw_id = 0;
	    std::cout << "Open success!" << std::endl;   
	    std::cout << "FPGA ID returned: " << std::hex << (fpga_id = usb.readMultibusInt( MB_VERSION_REG )) << std::dec << std::endl;        
	    std::cout << "FW S/N: " << (fw_id = usb.readFwSn( )) << std::endl;

		if ( fw_id != ID_FWWR ) {
            std::cout << "Attempting to load firmware..." << std::endl;
            int result = usb.loadFirmware( firmware_hex ); 
            if ( result == 0 ) {
                std::cout << "Firmware load success!" << std::endl;
                usb.close( ); 
                sleep.msleep( 5000 );  
            }
        } else {
            std::cout << "Firmware is already loaded!" << std::endl;
        }

		libusb_device *dev = findQS1RDevice( &usb, QS1R_VID, QS1R_PID, 0);

        if ( dev == nullptr ) {
            std::cerr << "Could not find any QS1R devices!" << std::endl;
            return -1;
        }

        ret = usb.open( dev );

        if ( ret != 0 ) {
            std::cerr << "Open Device Error: " << libusb_error_name( ret ) << std::endl; 
            return -1;   
        }

        std::cout << "FW S/N: " << std::dec << (fw_id = usb.readFwSn( )) << std::endl;

        if ( fpga_id != ID_1RXWR ) {
            std::cout << "Attempting to load FPGA bitstream..." << std::endl;
            int result =usb.loadFpgaFromBitstream( fpga_bitstream, fpga_bitstream_size );
            if ( result == 0 ) {
                std::cout << "FPGA load success!" << std::endl; 
                usb.close( ); 
                sleep.msleep( 1000 );    
            }
        } else {
            std::cout << "FPGA already loaded!" << std::endl;    
        }

        usb.open( dev );

        std::cout << "FPGA ID returned: " << std::hex << (fpga_id = usb.readMultibusInt( MB_VERSION_REG )) << std::dec << std::endl;		

	} else {
        std::cerr << "Error opening device!" << std::endl;
		return -1;
    }

	std::cout << "QS1R index [" << index << "] hardware was successfully initialized!" << std::endl;
	m_hardware_is_init = true;
	return 0;
}

int QS1R_server :: setPgaMode( bool on ) {
	if ( !m_hardware_is_init )
    {
        std::cerr << "Error: Please initialize QS1R Hardware first!" << std::endl;        
        return -1;
    }
    int result = usb.readMultibusInt( MB_CONTRL1 );
    if ( on )
    {
        result |= PGA;
    }
    else
    {
        result &= ~PGA;
    }

    return usb.writeMultibusInt( MB_CONTRL1, (u_int)result );
}

int QS1R_server :: setRandMode( bool on ) {
	if ( !m_hardware_is_init )
    {
        std::cerr << "Error: Please initialize QS1R Hardware first!" << std::endl;        
        return -1;
    }
    int result = usb.readMultibusInt( MB_CONTRL1 );
    if ( on )
    {
        result |= RANDOM;
    }
    else
    {
        result &= ~RANDOM;
    }

    return usb.writeMultibusInt( MB_CONTRL1, (u_int)result );
	
}

int QS1R_server :: setDitherMode( bool on) {
	if ( !m_hardware_is_init )
    {
        std::cerr << "Error: Please initialize QS1R Hardware first!" << std::endl;        
        return -1;
    }
    int result = usb.readMultibusInt( MB_CONTRL1 );
    if ( on )
    {
        result |= DITHER;
    }
    else
    {
        result &= ~DITHER;
    }

    return usb.writeMultibusInt( MB_CONTRL1, (u_int)result );
}

bool QS1R_server :: pgaMode( ) {
	int result = usb.readMultibusInt( MB_CONTRL1 );
	if ( ( result & PGA ) == PGA )
        return true;
    else
        return false;
}

bool QS1R_server :: randMode( ) {
	int result = usb.readMultibusInt( MB_CONTRL1 );
    if ( ( result & RANDOM ) == RANDOM )
        return true;
    else
        return false;
}

bool QS1R_server :: ditherMode( ) {
	int result = usb.readMultibusInt( MB_CONTRL1 );
    if ( ( result & DITHER ) == DITHER )
        return true;
    else
        return false;
}
