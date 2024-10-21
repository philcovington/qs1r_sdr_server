#include "../include/qs_blockfifo.hpp"

QsBlockFifo::QsBlockFifo() {
    block_size_ = BSIZE;
    type_ = 0;
}

QsBlockFifo::QsBlockFifo(int blocksize) {
    block_size_ = blocksize;
    type_ = 0;
}

QsBlockFifo::~QsBlockFifo() {}

void QsBlockFifo::enqueue(int *buffer) {
    mutex.lock();

    QsSignalOps::Copy(buffer, reinterpret_cast<int *>(&eq_fifo_block_), block_size_);

    fifo_.push(eq_fifo_block_);

    mutex.unlock();
}

bool QsBlockFifo::dequeue(int *buffer) {
    if (!fifo_.empty()) {
        if (mutex.try_lock()) {
            dq_fifo_block_ = fifo_.front(); // Get the front element
            fifo_.pop();

            QsSignalOps::Copy(reinterpret_cast<int *>(&dq_fifo_block_), buffer, block_size_);

            mutex.unlock();
        } else
            return false;
    } else {
        return false;
    }
    return true;
}

void QsBlockFifo::enqueue(Cpx *buffer) {
    mutex.lock();

    QsSignalOps::Copy(buffer, reinterpret_cast<Cpx *>(&cpx_eq_fifo_block_), block_size_);

    cpx_fifo_.push(cpx_eq_fifo_block_);

    mutex.unlock();
    
}

bool QsBlockFifo::dequeue(Cpx *buffer) {
    if (!cpx_fifo_.empty()) {
        if (mutex.try_lock()) {
            cpx_dq_fifo_block_ = cpx_fifo_.front();
            cpx_fifo_.pop();

            QsSignalOps::Copy(reinterpret_cast<Cpx *>(&cpx_dq_fifo_block_), buffer, block_size_);

            mutex.unlock();
        } else
            return false;
    } else {
        return false;
    }
    return true;
}

int QsBlockFifo::getCount() {
    if (type_ == 0)
        return fifo_.size();
    else
        return cpx_fifo_.size();
}

bool QsBlockFifo::isEmpty() {
    if (type_ == 0)
        return fifo_.empty();
    else
        return cpx_fifo_.empty();
}

void QsBlockFifo::empty() {
    QsSignalOps::Zero(reinterpret_cast<int *>(&eq_fifo_block_), BSIZE);
    QsSignalOps::Zero(reinterpret_cast<int *>(&dq_fifo_block_), BSIZE);

    QsSignalOps::Zero(reinterpret_cast<Cpx *>(&cpx_eq_fifo_block_), BSIZE);
    QsSignalOps::Zero(reinterpret_cast<Cpx *>(&cpx_dq_fifo_block_), BSIZE);

    std::queue<FIFOBLOCK> empty_fifo;
    std::swap(fifo_, empty_fifo);

    std::queue<CPXFIFOBLOCK> empty_cpxfifo;
    std::swap(cpx_fifo_, empty_cpxfifo);
}

void QsBlockFifo::setBlockSize(int size) {
    if (size > BSIZE)
        size = BSIZE;
    block_size_ = size;
}

int QsBlockFifo::blockSize() { return block_size_; }

void QsBlockFifo::setType(int type) { type_ = type; }

int QsBlockFifo::type() { return type_; }

void QsBlockFifo::trimFifo(int maxsize) {
    while (getCount() > maxsize) {
        if (type_ == 0) {
            fifo_.pop(); // Remove the front element
        } else {
            cpx_fifo_.pop(); // Remove the front element
        }
    }
}
