#include "../include/qs_defines.hpp"
#include <string>

std::string modeIntToString(int mode) {
    switch (mode) {
    case dmAM:
        return "AM";
    case dmSAM:
        return "SAM";
    case dmFMN:
        return "FMN";
    case dmFMW:
        return "FMW";
    case dmDSB:
        return "DSB";
    case dmLSB:
        return "LSB";
    case dmUSB:
        return "USB";
    case dmCW:
        return "CW";
    case dmDIG:
        return "DIG";
    default:
        return "";
    }
}

int modeStringToInt(std::string mode) {
    if (mode == "AM")
        return (int)dmAM;
    else if (mode == "SAM")
        return (int)dmSAM;
    else if (mode == "FMN")
        return (int)dmFMN;
    else if (mode == "FMW")
        return (int)dmFMW;
    else if (mode == "DSB")
        return (int)dmDSB;
    else if (mode == "LSB")
        return (int)dmLSB;
    else if (mode == "USB")
        return (int)dmUSB;
    else if (mode == "CW")
        return (int)dmCW;
    else if (mode == "DIG")
        return (int)dmDIG;
    else
        return -1;
}