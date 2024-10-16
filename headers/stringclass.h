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

#include <algorithm>
#include <cctype>
#include <iostream>
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

  private:
    std::string data;
};
