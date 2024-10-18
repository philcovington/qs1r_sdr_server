// qs_defines.hpp
#pragma once

#include <string>

enum QSDEMODMODE { dmAM = 0, dmSAM = 1, dmFMN = 2, dmFMW = 3, dmDSB = 4, dmLSB = 5, dmUSB = 6, dmCW = 7, dmDIG = 8 };

extern std::string modeIntToString(int mode);
extern int modeStringToInt(std::string mode);

enum QSTXVFOMODE { txFollowRXVfo = 0, txFollowTXVfo = 1 };

#define NUMBER_OF_RECEIVERS 1
#define MAX_RECEIVERS 2

#define SDRMAXV_VERSION "5.0.0.9"

#define SPEC_OFFSET_VALUE 200.0

#define FIRMWARE_FILENAME "qs1r_firmware_11022011.hex.hex"
#define FPGA_FILENAME "QS1R_MASTER_RXTX.rbf"
#define RESOURCE_FIRMWARE_FILENAME ":/QS1RServer/Resources/qs1r_firmware_11022011.hex"
#define RESOURCE_FPGA_MASTER ":/QS1RServer/Resources/QS1R_MASTER_RXTX.rbf"

#define LOAD_DELAY_COUNT 250

#define MAX_CHANNELS 2
#define CPX_RING_SZ_MULT 4
#define SD_RING_SZ_MULT 4
#define RT_RING_SZ_MULT 4
#define DAC_RING_SZ_MULT 4

#define MAX_MAN_NOTCHES 8
