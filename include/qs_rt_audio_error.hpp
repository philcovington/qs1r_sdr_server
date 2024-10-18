/*
 * File: qs_rt_audio_error.h
 * Brief: Custom exception class for handling RtAudio-related errors.
 * 
 * This header defines the `RtError` class, a custom exception used to handle 
 * various error types within the RtAudio library. The class extends `std::exception` 
 * and provides additional functionality for categorizing and retrieving error 
 * messages.
 *
 * Features:
 * - Supports multiple error types, including device, memory, system, and thread errors.
 * - Provides a method to print error messages to `stderr`.
 * - Includes functions to retrieve error type, error message string, and a C-style 
 *   error message (`what()`).
 *
 * Usage:
 * Throw an `RtError` object with a specific error message and type when an audio 
 * operation fails. The error can be caught and handled, and the message can be printed 
 * or logged using `printMessage()`. Use `getType()` and `getMessage()` to examine 
 * the error details programmatically.
 *
 * Notes:
 * - The error type defaults to `UNSPECIFIED` if not provided.
 * - The `what()` method provides compatibility with standard exception handling.
 * 
 * Author: Philip A Covington
 * Date: 2024-10-16
 */

#pragma once

#include <exception>
#include <iostream>
#include <string>

class RtError : public std::exception {
  public:
    //! Defined RtError types.
    enum Type {
        WARNING,           /*!< A non-critical error. */
        DEBUG_WARNING,     /*!< A non-critical error which might be useful for debugging. */
        UNSPECIFIED,       /*!< The default, unspecified error type. */
        NO_DEVICES_FOUND,  /*!< No devices found on system. */
        INVALID_DEVICE,    /*!< An invalid device ID was specified. */
        MEMORY_ERROR,      /*!< An error occured during memory allocation. */
        INVALID_PARAMETER, /*!< An invalid parameter was specified to a function. */
        INVALID_USE,       /*!< The function was called incorrectly. */
        DRIVER_ERROR,      /*!< A system driver error occured. */
        SYSTEM_ERROR,      /*!< A system error occured. */
        THREAD_ERROR       /*!< A thread error occured. */
    };

    //! The constructor.
    RtError(const std::string &message, Type type = RtError::UNSPECIFIED) throw() : message_(message), type_(type) {}

    //! The destructor.
    virtual ~RtError(void) throw() {}

    //! Prints thrown error message to stderr.
    virtual void printMessage(void) const throw() { std::cerr << '\n' << message_ << "\n\n"; }

    //! Returns the thrown error message type.
    virtual const Type &getType(void) const throw() { return type_; }

    //! Returns the thrown error message string.
    virtual const std::string &getMessage(void) const throw() { return message_; }

    //! Returns the thrown error message as a c-style string.
    virtual const char *what(void) const throw() { return message_.c_str(); }

  protected:
    std::string message_;
    Type type_;
};
