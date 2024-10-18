/**
 * @file StringList.h
 * @brief A class for managing a list of strings with additional functionality.
 *
 * The StringList class provides an intuitive way to manage a collection of strings.
 * It allows for adding, inserting, removing, and searching through strings, as well
 * as performing operations such as joining the strings with a specified separator.
 * This class supports both standard `std::string` and custom `String` objects, 
 * enhancing flexibility in string management.
 *
 * Features:
 * - Constructors for initializing from std::vector or an initializer list.
 * - Method to append strings, both from std::string and the String class.
 * - Method to insert a string at a specific index.
 * - Method to remove a string from a specific index.
 * - Method to retrieve a string at a given index safely.
 * - Method to find the index of a string, returning -1 if not found.
 * - Method to get the current size of the list.
 * - Method to check if a string exists within the list.
 * - Method to join the strings into a single string with a specified separator.
 * - Method to clear all strings from the list.
 * - Overloaded operator[] for easy access to elements by index.
 * - Getter method to retrieve the internal vector of strings for further manipulation.
 *
 * Usage:
 * To create and manipulate a StringList instance, you can do the following:
 *
 *   StringList myList;
 *   myList.append("Hello");
 *   myList.append(String("World"));
 *   myList.insert(1, "C++");
 *   int index = myList.indexOf("World"); // Returns 2
 *   std::string joined = myList.join(", "); // "Hello, C++, World"
 *
 * The StringList class enhances string management capabilities while maintaining
 * simplicity and ease of use.
 * 
 * I tried to provide similar functionality to Qt's QStringList class.
 *
 * Author: [Philip A Covington]
 * Date: [2024-10-16]
 */

#pragma once

#include "../include/qs_stringclass.hpp" // Include your String class
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class StringList {
  public:
    // Constructors
    StringList() = default;
    StringList(const std::vector<std::string> &strings) : list(strings) {}
    StringList(const std::initializer_list<std::string> &strings) : list(strings) {}

    // Add a string to the list
    void append(const std::string &str) { list.push_back(str); }

    // Add a String object to the list
    void append(const String &str) {
        list.push_back(str.toStdString()); // Convert String to std::string
    }

    // Add another list of String objects to this list
    void append(const StringList &other) {
        for (const String &s : other.getStrings()) { // Use the new method to get the strings
            append(s);                               // Call the String append method
        }
    }

    // Insert a string at a specific index
    void insert(int index, const std::string &str) {
        if (index >= 0 && index <= size()) {
            list.insert(list.begin() + index, str);
        }
    }

    // Remove a string from the list
    void removeAt(int index) {
        if (index >= 0 && index < size()) {
            list.erase(list.begin() + index);
        }
    }

    // Get string at an index
    std::string at(int index) const {
        if (index >= 0 && index < size()) {
            return list.at(index);
        }
        return "";
    }

    // Find index of a string (-1 if not found)
    int indexOf(const std::string &str) const {
        auto it = std::find(list.begin(), list.end(), str);
        return it != list.end() ? std::distance(list.begin(), it) : -1;
    }

    // Get the size of the list
    int size() const { return static_cast<int>(list.size()); }

    // Check if the list contains a string
    bool contains(const std::string &str) const { return std::find(list.begin(), list.end(), str) != list.end(); }

    // Join the strings with a separator
    std::string join(const std::string &separator) const {
        std::ostringstream os;
        for (size_t i = 0; i < list.size(); ++i) {
            os << list[i];
            if (i != list.size() - 1) {
                os << separator;
            }
        }
        return os.str();
    }

    // Clear the list
    void clear() { list.clear(); }

    // Overload operator[] for access
    std::string &operator[](int index) { return list.at(index); }
    const std::string &operator[](int index) const { return list.at(index); }

    // Getter for the internal string vector (for appending)
    const std::vector<std::string> &getStrings() const { return list; }

  private:
    std::vector<std::string> list;
};
