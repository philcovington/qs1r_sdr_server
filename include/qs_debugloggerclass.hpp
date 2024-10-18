/**
 * @file DebugLogger.h
 * @brief A logging utility class for formatted output with timestamps.
 *
 * The DebugLogger class provides a flexible and convenient way to log messages
 * with support for various data types, including standard types and custom
 * types (e.g., the String class). Each log entry includes a timestamp with
 * seconds and milliseconds precision, allowing for precise tracking of events.
 *
 * Features:
 * - Overloaded operator<< for logging different types, including vectors.
 * - Automatic timestamp formatting in the log output, showing both time
 *   and milliseconds, controlled by a DEBUG flag.
 * - Specialization for custom String class to ensure proper logging.
 *
 * Usage:
 * To log messages, create an instance of DebugLogger using the _debug()
 * helper function. Messages can be logged using the operator<<:
 *
 *   _debug() << "Log message: " << someValue << std::vector<int>{1, 2, 3};
 *
 * This will output:
 *   [YYYY-MM-DD HH:MM:SS.mmm] Log message: someValue [1, 2, 3]
 *
 * If the DEBUG flag is not set, the output will be:
 *   Log message: someValue [1, 2, 3]
 *
 * The class automatically handles the destruction of the logger instance,
 * flushing the output with the formatted timestamp if DEBUG is enabled.
 *
 * @note This class is intended for debugging purposes and may not be suitable
 * for production logging due to performance considerations.
 *
 * I have tried to make it provide similar functionality to Qt's qDebug().
 *
 * Author: [Philip A Covington]
 * Date: [2024-10-16]
 */

#pragma once

#include "../include/qs_stringclass.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class DebugLogger {
  public:
    // Static flag to enable or disable debug logging
    static inline bool DEBUG = false; // Default to false

    // Log function for basic types and strings
    template <typename T> inline DebugLogger &operator<<(const T &value) {
        stream << value;
        return *this;
    }

    // Function to log a vector of values
    template <typename T> inline DebugLogger &operator<<(const std::vector<T> &vec) {
        stream << "[";
        for (size_t i = 0; i < vec.size(); ++i) {
            stream << vec[i];
            if (i < vec.size() - 1) {
                stream << ", ";
            }
        }
        stream << "]";
        return *this;
    }

    // Flush the log with timestamp and output
    ~DebugLogger() {
        if (DEBUG) {
            // Get current time with milliseconds precision
            auto now = std::chrono::system_clock::now();
            std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> ms =
                std::chrono::time_point_cast<std::chrono::milliseconds>(now);
            auto in_ms = ms.time_since_epoch();

            // Get seconds and milliseconds
            auto seconds = std::chrono::duration_cast<std::chrono::seconds>(in_ms);
            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(in_ms) % 1000;

            // Convert to time_t for formatting
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::tm *local_time = std::localtime(&now_c);

            // Output formatted time with milliseconds
            std::cout << std::put_time(local_time, "[%Y-%m-%d %H:%M:%S.") << std::setw(3) << std::setfill('0')
                      << milliseconds.count() << "] " << stream.str() << std::endl;
        } else {
            // Print only the message if DEBUG is not enabled
            std::cout << stream.str() << std::endl;
        }
    }

  private:
    std::ostringstream stream;
};

// Specialization for the String class (defined outside the class)
template <> inline DebugLogger &DebugLogger::operator<<(const String &value) {
    stream << value.toStdString(); // Convert to std::string for logging
    return *this;
}

// Helper function to create a logger instance
inline DebugLogger _debug() { return DebugLogger(); }
