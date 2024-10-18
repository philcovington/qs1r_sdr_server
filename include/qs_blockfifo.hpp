/**
 * @file QsBlockFifo.h
 * @brief A class for managing a FIFO (First In, First Out) buffer with support for integer and complex number blocks.
 *
 * The QsBlockFifo class provides a thread-safe implementation of a FIFO queue 
 * for handling blocks of data. It allows enqueuing and dequeuing of integer 
 * and complex number buffers, with configurable block sizes and types.
 *
 * Features:
 * - Two types of buffers: integer and complex number (Cpx).
 * - Thread-safe operations for enqueueing and dequeueing.
 * - Methods to check the current count of items in the FIFO and if it is empty.
 * - Ability to clear the FIFO and trim its size to a maximum limit.
 * - Configurable block size and type for flexible usage scenarios.
 *
 * Usage:
 * To utilize the QsBlockFifo class, you can create an instance and use 
 * its methods as follows:
 *
 *   QsBlockFifo fifo;
 *   int buffer[BSIZE];
 *   fifo.enqueue(buffer); // Enqueue an integer buffer
 *   fifo.dequeue(buffer); // Dequeue into an integer buffer
 *
 *   Cpx cpxBuffer[BSIZE];
 *   fifo.enqueue(cpxBuffer); // Enqueue a complex buffer
 *   fifo.dequeue(cpxBuffer); // Dequeue into a complex buffer
 *
 * This class is useful in scenarios requiring FIFO data processing, such as 
 * in signal processing, data acquisition systems, or any application that 
 * requires buffered data management.
 *
 * Author: [Philip A Covington]
 * Date: [2024-10-16]
 */

#pragma once

#include "../include/qs_dataproc.hpp"
#include "../include/qs_globals.hpp"
#include "../include/qs_wait_condition.hpp"
#include <mutex>
#include <queue>

#define BSIZE 8196 * 2

#pragma pack(push)

#pragma pack(1)

typedef struct fifoblock_t {
    int block[BSIZE];
} FIFOBLOCK;

typedef struct cpxfifoblock_t {
    Cpx block[BSIZE];
} CPXFIFOBLOCK;

#pragma pack(pop)

class QsBlockFifo {

  public:
    QsBlockFifo();
    QsBlockFifo(int blocksize);
    ~QsBlockFifo();

    void enqueue(int *buffer);
    bool dequeue(int *buffer);

    void enqueue(Cpx *buffer);
    bool dequeue(Cpx *buffer);

    int getCount();
    bool isEmpty();
    void empty();

    void setBlockSize(int size);
    int blockSize();

    void setType(int type = 0);
    int type();

    void trimFifo(int maxsize);

  private:
    FIFOBLOCK eq_fifo_block_;
    FIFOBLOCK dq_fifo_block_;
    CPXFIFOBLOCK cpx_eq_fifo_block_;
    CPXFIFOBLOCK cpx_dq_fifo_block_;
    std::queue<FIFOBLOCK> fifo_;
    std::queue<CPXFIFOBLOCK> cpx_fifo_;

    int block_size_;
    int type_;

    std::mutex mutex;
};
