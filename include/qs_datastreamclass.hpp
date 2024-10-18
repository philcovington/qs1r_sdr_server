#include "../include/qs_bytearray.hpp"
#include "../include/qs_file.hpp"
#include "../include/qs_uuid.hpp"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class DataStream {
  public:
    // Constructor for writing data
    DataStream(std::ostream &outputStream) : outputStream(&outputStream), inputStream(nullptr) {}

    // Constructor for reading data from ByteArray
    DataStream(const ByteArray &byteArray)
        : outputStream(nullptr), inputStream(new std::istringstream(
                                     std::string(reinterpret_cast<const char *>(byteArray.data()), byteArray.size()))) {
    }

    // Constructor for reading data from a File
    DataStream(File *file) : outputStream(nullptr), inputStream(new std::istringstream()) {
        if (!file || !file->isValid()) {
            throw std::runtime_error("File is not open or invalid.");
        }

        // Temporary buffer for reading
        std::ostringstream tempStream;

        while (!file->atEnd()) {
            std::vector<char> data = file->read(1024);  // Read 1024 bytes
            tempStream.write(data.data(), data.size()); // Write to the temporary output stream
        }

        // Set the contents of the input stream to the temporary stream's content
        inputStream->str(tempStream.str());
        inputStream->seekg(0); // Reset the position of the input stream
    }

    // Destructor
    ~DataStream() {
        delete inputStream; // Clean up allocated input stream
    }

    // Write an integer to the stream
    DataStream &operator<<(int value) {
        outputStream->write(reinterpret_cast<const char *>(&value), sizeof(value));
        return *this;
    }

    // Read an integer from the stream
    DataStream &operator>>(int &value) {
        inputStream->read(reinterpret_cast<char *>(&value), sizeof(value));
        return *this;
    }

    // Write a string to the stream
    DataStream &operator<<(const std::string &value) {
        size_t length = value.size();
        outputStream->write(reinterpret_cast<const char *>(&length), sizeof(length));
        outputStream->write(value.data(), length);
        return *this;
    }

    // Read a string from the stream
    DataStream &operator>>(std::string &value) {
        size_t length;
        inputStream->read(reinterpret_cast<char *>(&length), sizeof(length));
        value.resize(length);
        inputStream->read(&value[0], length);
        return *this;
    }

    // Read a UUID (assumed to be 16 bytes)
    DataStream &operator>>(UUID &uuid) {
        inputStream->read(reinterpret_cast<char *>(&uuid), sizeof(UUID));
        return *this;
    }

  private:
    std::ostream *outputStream;      // Output stream for writing data
    std::istringstream *inputStream; // Input stream for reading data
};