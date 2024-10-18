/**
 * @file qs_stringlist.hpp
 * @brief Command Processor Class
 *
 * This class processes commands with read and write operations.
 * It supports storing various data types, including strings and numeric values.
 *
 * Features:
 * - Processes commands with read and write capabilities.
 * - Stores command values and types for processing.
 * - Allows for enumeration of command result types.
 *
 * Usage:
 * Create an instance of the CMD class, then call the `processCMD` method
 * with a command string to process the command.
 *
 * Example:
 * CMD commandProcessor;
 * CMD result = commandProcessor.processCMD("your_command_here");
 *
 * Author: Philip A Covington
 * Date: 2024-10-17
 */

#pragma once

#include "../include/qs_stringclass.hpp"
#include "../include/qs_stringlistclass.hpp"

class CMD {

  public:
    typedef enum rw_t { cmd_error, cmd_read, cmd_write } CMDRW;

    CMDRW RW;
    String cmd;
    String svalue;
    StringList slist;
    int ivalue;
    double dvalue;
    CMD();
    CMD processCMD(String command);
};


