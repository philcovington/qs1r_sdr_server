#include <iostream>
#include <fstream>
#include <string>

class TextStream {
private:
    std::ifstream* fileStream;

public:
    // Constructor that accepts a pointer to a File object
    TextStream(File* file) {
        fileStream = &file->fileStream;
    }

    // Overload >> operator for reading from the file stream
    TextStream& operator>>(std::string& output) {
        if (fileStream && *fileStream) {
            *fileStream >> output;
        }
        return *this;
    }

    // Overload for reading integers (you can add more as needed)
    TextStream& operator>>(int& output) {
        if (fileStream && *fileStream) {
            *fileStream >> output;
        }
        return *this;
    }

    // Check if at the end of file
    bool atEnd() const {
        return fileStream->eof();
    }
};
