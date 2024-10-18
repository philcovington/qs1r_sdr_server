/**
 * @file qs_bytearray.hpp
 * @brief A class for handling a dynamic array of bytes.
 *
 * The ByteArray class provides a simple interface for managing a dynamic
 * array of bytes, leveraging the capabilities of std::vector to store
 * raw byte data. It includes methods for accessing and manipulating
 * byte data efficiently.
 *
 * Features:
 * - Constructor to initialize a ByteArray from a standard vector of bytes.
 * - Constructor to create a ByteArray of a specific size, initialized to a given value.
 * - Method to retrieve the size of the byte array.
 * - Overloaded subscript operator for easy access to individual bytes.
 * - Functionality to append additional bytes to the end of the array.
 * - Method to retrieve a raw pointer to the underlying byte data.
 *
 * Usage:
 * To utilize the ByteArray class, you can create an instance and use
 * its methods as follows:
 *
 *   ByteArray byteArray(16, 0); // Create a ByteArray of size 16 initialized to 0
 *   size_t size = byteArray.size(); // Get the size
 *   uint8_t byte = byteArray[0]; // Access the first byte
 *
 * This class is useful in scenarios where byte manipulation is needed,
 * such as in communication protocols, file handling, or low-level data
 * processing.
 *
 * Author: Philip A Covington
 * Date: 2024-10-16
 */

#pragma once

#include <cstdint>
#include <vector>

class ByteArray {
  public:
    ByteArray() = default;

    // Construct from a byte vector
    ByteArray(const std::vector<uint8_t> &data) : data_(data) {}

    // Constructor to create a ByteArray of a specific size initialized to a given value
    ByteArray(size_t size, uint8_t fillValue) : data_(size, fillValue) {}

    // Get the size of the byte array
    size_t size() const { return data_.size(); }

    // Access byte at a given index
    uint8_t operator[](size_t index) const { return data_.at(index); }

    // Add a byte to the end
    void append(uint8_t byte) { data_.push_back(byte); }

    // Get raw data pointer
    const uint8_t *data() const { return data_.data(); }

  private:
    std::vector<uint8_t> data_; // Internal storage
};
