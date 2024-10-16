#pragma once

#include "../headers/qs_dataproc.h"
#include "../headers/qs_globals.h"
#include "../headers/qs_wait_condition.h"
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
