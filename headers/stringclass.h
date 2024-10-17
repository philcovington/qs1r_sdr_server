/**
 * @file String.h
 * @brief A wrapper class for std::string that provides additional functionality.
 *
 * The String class is designed to simplify string manipulation by wrapping
 * around std::string and offering a more intuitive interface for common
 * operations. This class supports constructing from standard strings or C-style
 * strings, as well as converting back to std::string. It includes methods for
 * appending, checking for substrings, and converting various numeric types
 * into string representations.
 *
 * Features:
 * - Constructors for default, std::string, and C-style string initialization.
 * - Static method to create a String object from std::string.
 * - Method to convert the String object back to std::string.
 * - Support for appending both std::string and other String objects.
 * - Method to check if the string contains a specific substring.
 * - Overloaded operator+ for concatenating two String objects.
 * - Method to clear the internal string data.
 * - Static methods to convert integers, unsigned integers, floats, and doubles
 *   into String objects with optional precision control for floating-point values.
 *
 * Usage:
 * To create and manipulate a String instance, you can do the following:
 *
 *   String myString("Hello, ");
 *   myString.append("World!");
 *   String concatenated = myString + String(" Welcome!"); // "Hello, World! Welcome!"
 *   bool hasSubstring = myString.contains("World"); // true
 *
 * The String class enhances the standard string functionality while
 * maintaining simplicity and ease of use.
 *
 * I tried to provide similar functionality to Qt's QString class.
 *
 * Author: [Philip A Covington]
 * Date: [2024-10-16]
 */

#pragma once

#include "../headers/stringlistclass.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

class String {
  public:
    // Constructors
    String() = default;
    String(const std::string &str) : data(str) {}
    String(const char *str) : data(str) {}

    // Static method to create a String object from std::string
    static String fromStdString(const std::string &str) { return String(str); }

    // Convert the String object to std::string
    std::string toStdString() const { return data; }

    // Append another std::string
    void append(const std::string &str) { data.append(str); }

    // Append another String object
    void append(const String &other) { data += other.data; }

    // Check if the string contains a substring
    bool contains(const std::string &str) const { return data.find(str) != std::string::npos; }

    // Overload the `+` operator for concatenation of another String
    friend String operator+(const String &lhs, const String &rhs) { return String(lhs.data + rhs.data); }

    // Clear the string
    void clear() { data.clear(); }

    // Static method to convert int to String
    static String number(int num) { return String(std::to_string(num)); }

    // Static method to convert unsigned int to String
    static String number(unsigned int num) { return String(std::to_string(num)); }

    // Static method to convert double to String (with optional precision)
    static String number(double num, int precision = 6) {
        std::ostringstream out;
        out.precision(precision);
        out << std::fixed << num;
        return String(out.str());
    }

    // Static method to convert float to String (with optional precision)
    static String number(float num, int precision = 6) {
        std::ostringstream out;
        out.precision(precision);
        out << std::fixed << num;
        return String(out.str());
    }

    int toInt(bool *ok = nullptr) const {
        if (ok) {
            *ok = false; // Assume failure by default
        }

        // Check if the string is empty or only contains whitespace
        if (data.empty() || std::all_of(data.begin(), data.end(), isspace)) {
            return 0; // Invalid input
        }

        // Check for leading sign
        size_t startIndex = 0;
        if (data[0] == '-' || data[0] == '+') {
            startIndex = 1; // Move past the sign
        }

        // Validate each character in the string
        for (size_t i = startIndex; i < data.length(); ++i) {
            if (!isdigit(data[i])) {
                return 0; // Invalid character found
            }
        }

        // Convert the string to an integer
        int result = std::stoi(data); // We assume input is valid after checks

        // Check for out of range
        if (result == std::numeric_limits<int>::max() || result == std::numeric_limits<int>::min()) {
            return 0; // Indicate failure (or handle it differently based on your requirements)
        }

        if (ok) {
            *ok = true; // Set ok to true if conversion is successful
        }

        return result;
    }

    float toFloat(bool *ok = nullptr) const {
        if (ok) {
            *ok = false; // Assume failure by default
        }

        // Check if the string is empty or only contains whitespace
        if (data.empty() || std::all_of(data.begin(), data.end(), isspace)) {
            return 0.0f; // Invalid input
        }

        // Check for leading sign
        size_t startIndex = 0;
        if (data[0] == '-' || data[0] == '+') {
            startIndex = 1; // Move past the sign
        }

        bool decimalFound = false; // Flag to check if we found a decimal point
        for (size_t i = startIndex; i < data.length(); ++i) {
            if (data[i] == '.') {
                if (decimalFound) {
                    return 0.0f; // Invalid format: multiple decimal points
                }
                decimalFound = true; // Found the decimal point
            } else if (!isdigit(data[i])) {
                return 0.0f; // Invalid character found
            }
        }

        // Convert the string to a float
        float result = std::strtof(data.c_str(), nullptr);

        // Check for out of range
        if (result == std::numeric_limits<float>::max() || result == std::numeric_limits<float>::min()) {
            return 0.0f; // Indicate failure (or handle it differently)
        }

        if (ok) {
            *ok = true; // Set ok to true if conversion is successful
        }

        return result;
    }

    double toDouble(bool *ok = nullptr) const {
        if (ok) {
            *ok = false; // Assume failure by default
        }

        // Check if the string is empty or only contains whitespace
        if (data.empty() || std::all_of(data.begin(), data.end(), isspace)) {
            return 0.0; // Invalid input
        }

        // Check for leading sign
        size_t startIndex = 0;
        if (data[0] == '-' || data[0] == '+') {
            startIndex = 1; // Move past the sign
        }

        bool decimalFound = false; // Flag to check if we found a decimal point
        for (size_t i = startIndex; i < data.length(); ++i) {
            if (data[i] == '.') {
                if (decimalFound) {
                    return 0.0; // Invalid format: multiple decimal points
                }
                decimalFound = true; // Found the decimal point
            } else if (!isdigit(data[i])) {
                return 0.0; // Invalid character found
            }
        }

        // Convert the string to a double
        double result = std::strtod(data.c_str(), nullptr);

        // Check for out of range
        if (result == std::numeric_limits<double>::max() || result == std::numeric_limits<double>::min()) {
            return 0.0; // Indicate failure (or handle it differently)
        }

        if (ok) {
            *ok = true; // Set ok to true if conversion is successful
        }

        return result;
    }

    bool startsWith(const std::string &prefix) const {
        return data.rfind(prefix, 0) == 0; // Check if the prefix matches the beginning of the string
    }

    int indexOf(const std::string &str, size_t start = 0) const {
        if (start >= data.length()) {
            return -1; // Start position is out of bounds
        }
        size_t pos = data.find(str, start);
        return (pos != std::string::npos) ? static_cast<int>(pos) : -1;
    }

    size_t length() const { return data.length(); }

    String mid(size_t start, size_t length) const {
        if (start >= data.length()) {
            return String(); // Return an empty String if start is out of bounds
        }
        return String(data.substr(start, length));
    }

    StringList split(const std::string &delimiter) const {
        StringList result;
        size_t start = 0;
        size_t end = data.find(delimiter);

        while (end != std::string::npos) {
            result.append(data.substr(start, end - start)); // Add the substring to the result
            start = end + delimiter.length();
            end = data.find(delimiter, start);
        }

        result.append(data.substr(start, end)); // Add the last substring
        return result;
    }

  private:
    std::string data;
};
