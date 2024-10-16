/**
 * @file TextStream.h
 * @brief A class for handling text file streams for reading data.
 *
 * The TextStream class provides a convenient interface for reading data from 
 * text files. It allows for reading strings and integers from an underlying 
 * file stream, enabling easy integration with file handling in C++. 
 * The class checks for end-of-file conditions and facilitates data 
 * extraction using the overloaded extraction operators.
 *
 * Features:
 * - Constructor that initializes the class with a pointer to a File object,
 *   allowing seamless access to the file stream.
 * - Overloaded operator>> for reading strings from the file stream.
 * - Overloaded operator>> for reading integers from the file stream.
 * - Method to check if the end of the file has been reached.
 *
 * Usage:
 * To create and utilize a TextStream instance, you can do the following:
 *
 *   File myFile("data.txt"); // Assuming a File class exists
 *   TextStream textStream(&myFile);
 *   std::string line;
 *   while (!textStream.atEnd()) {
 *       textStream >> line;
 *       std::cout << line << std::endl;
 *   }
 *
 * This class abstracts the complexity of file reading operations, providing 
 * a straightforward method for extracting data from text files in C++.
 *
 * Author: [Philip A Covington]
 * Date: [2024-10-16]
 */

#pragma once

#include <fstream>
#include <iostream>
#include <string>

class TextStream {
  private:
    std::ifstream *fileStream;

  public:
    // Constructor that accepts a pointer to a File object
    TextStream(File *file) { fileStream = &file->fileStream; }

    // Overload >> operator for reading from the file stream
    TextStream &operator>>(std::string &output) {
        if (fileStream && *fileStream) {
            *fileStream >> output;
        }
        return *this;
    }

    // Overload for reading integers (you can add more as needed)
    TextStream &operator>>(int &output) {
        if (fileStream && *fileStream) {
            *fileStream >> output;
        }
        return *this;
    }

    // Check if at the end of file
    bool atEnd() const { return fileStream->eof(); }
};
