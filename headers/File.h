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
