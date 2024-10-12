// // debug.cpp
// #include "../headers/debug.h"
// #include "../headers/config.h"  // For the DEBUG_MODE flag or debugEnabled variable

// // Implement the << operator for manipulators (like std::hex, std::endl)
// DebugStream& DebugStream::operator<<(std::ostream& (*manip)(std::ostream&)) {
// #if DEBUG_MODE
//     std::cout << manip;
//     std::cout.flush();
// #endif
//     return *this;
// }

// // Instantiate the global debugMessage object
// DebugStream debugMessage;

// debug.cpp
#include "debug.h"

// Implement the << operator for manipulators (like std::hex, std::endl)
DebugStream& DebugStream::operator<<(std::ostream& (*manip)(std::ostream&)) {
#if DEBUG_MODE
    std::cout << manip;
#endif
    return *this;
}

// Instantiate the global debugMessage object
DebugStream debugMessage;
