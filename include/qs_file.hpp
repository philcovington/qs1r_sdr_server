/**
 * @file qs_file.hpp
 * @brief A simple file handling utility for binary file operations.
 *
 * The File class provides a convenient interface for reading binary files
 * in C++. It manages file streams using an ifstream object and offers methods
 * to open files, check their validity, read data into a buffer, and close
 * the file when finished.
 *
 * Features:
 * - Constructor to initialize the file with a specified filename.
 * - Method to open a file with a specified mode (defaulting to binary read).
 * - Validity check to ensure the file is successfully opened.
 * - Method to read a specified number of bytes into a vector, returning the
 *   actual bytes read.
 * - Method to check if the end of the file has been reached.
 * - Destructor that ensures the file is closed properly when the object is 
 *   destroyed.
 *
 * Usage:
 * To use this class, create an instance of File with the desired filename.
 * You can then call the methods to perform file operations:
 *
 *   File myFile("example.bin");
 *   if (myFile.isValid()) {
 *       std::vector<char> data = myFile.read(100);
 *       // Process data...
 *   }
 *
 * The class automatically manages the file's lifecycle, closing the file 
 * stream if it is still open upon destruction.
 *
 * @note This class is intended for basic file handling and may need to be
 * extended for more complex file operations or error handling.
 * 
 * I tried to provide the same functionalty as Qt's QFile class.
 *
 * Author: [Philip A Covington]
 * Date: [2024-10-16]
 */

#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

class File {
  public:
    std::ifstream fileStream;

    // Constructor accepts a filename
    File(const std::string &filename) { fileStream.open(filename, std::ios::in | std::ios::binary); }

    // Open a file with the specified mode
    bool open(const std::string &filename, std::ios_base::openmode mode = std::ios::in | std::ios::binary) {
        fileStream.open(filename, mode);
        return isValid(); // Return true if the file is valid (opened successfully)
    }

    // Check if the file is valid
    bool isValid() const { return fileStream.is_open(); }

    // Close the file
    void close() { fileStream.close(); }

    // Read a specified number of bytes into a vector
    std::vector<char> read(size_t length) {
        std::vector<char> buffer(length);
        fileStream.read(buffer.data(), length);

        // Resize the buffer if fewer bytes were read
        buffer.resize(fileStream.gcount());
        return buffer; // Return the read bytes
    }

    // Check if the end of the file is reached
    bool atEnd() const { return fileStream.eof(); }

    // Destructor to close the file if it's still open
    ~File() {
        if (fileStream.is_open()) {
            fileStream.close();
        }
    }
};
