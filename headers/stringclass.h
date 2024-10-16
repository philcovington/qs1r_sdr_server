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
