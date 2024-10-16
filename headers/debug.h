// // debug.h
// #pragma once

// #include <iostream>

// class DebugStream {
// public:
//     DebugStream() = default;

//     // Overload the << operator to accept any type of input like std::cout
//     template<typename T>
//     DebugStream& operator<<(const T& value) {
// #if DEBUG_MODE  // For compile-time flag
//         std::cout << value;
// #endif
//         return *this;  // Return *this to allow chaining like std::cout
//     }

//     // Overload the << operator to handle manipulators (e.g., std::hex, std::endl)
//     DebugStream& operator<<(std::ostream& (*manip)(std::ostream&));
// };

// // Declare the global debugMessage object for use in other files
// extern DebugStream debugMessage;

// debug.h
#pragma once

#include "config.h" // For the DEBUG_MODE flag or debugEnabled variable
#include <iostream>

class DebugStream {
  public:
    DebugStream() = default;

    // Overload the << operator to accept any type of input like std::cout
    template <typename T> DebugStream &operator<<(const T &value) {
#if DEBUG_MODE // For compile-time flag
        std::cout << value;
#endif
        return *this; // Return *this to allow chaining like std::cout
    }

    // Overload the << operator to handle manipulators (e.g., std::hex, std::endl)
    DebugStream &operator<<(std::ostream &(*manip)(std::ostream &));
};

// Declare the global debugMessage object for use in other files
extern DebugStream debugMessage;
