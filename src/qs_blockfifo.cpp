#include "../headers/qs_blockfifo.h"

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

    QsSpl::Copy(buffer, reinterpret_cast<int *>(&eq_fifo_block_), block_size_);

    fifo_.push(eq_fifo_block_);

    mutex.unlock();

    WC_FILE_FIFO_WRITE.wakeAll();
}

bool QsBlockFifo::dequeue(int *buffer) {
    if (!fifo_.isEmpty()) {
        if (mutex.tryLock(1)) {
            dq_fifo_block_ = fifo_.pop();

            QsSpl::Copy(reinterpret_cast<int *>(&dq_fifo_block_), buffer, block_size_);

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

    QsSpl::Copy(buffer, reinterpret_cast<Cpx *>(&cpx_eq_fifo_block_), block_size_);

    cpx_fifo_.push(cpx_eq_fifo_block_);

    mutex.unlock();

    WC_FILE_FIFO_WRITE.wakeAll();
}

bool QsBlockFifo::dequeue(Cpx *buffer) {
    if (!cpx_fifo_.isEmpty()) {
        if (mutex.tryLock(1)) {
            cpx_dq_fifo_block_ = cpx_fifo_.pop();

            QsSpl::Copy(reinterpret_cast<Cpx *>(&cpx_dq_fifo_block_), buffer, block_size_);

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
        return fifo_.count();
    else
        return cpx_fifo_.count();
}

bool QsBlockFifo::isEmpty() {
    if (type_ == 0)
        return fifo_.isEmpty();
    else
        return cpx_fifo_.isEmpty();
}

void QsBlockFifo::empty() {
    QsSpl::Zero(reinterpret_cast<int *>(&eq_fifo_block_), BSIZE);
    QsSpl::Zero(reinterpret_cast<int *>(&dq_fifo_block_), BSIZE);

    QsSpl::Zero(reinterpret_cast<Cpx *>(&cpx_eq_fifo_block_), BSIZE);
    QsSpl::Zero(reinterpret_cast<Cpx *>(&cpx_dq_fifo_block_), BSIZE);

    fifo_.empty();
    cpx_fifo_.empty();
}

void QsBlockFifo::setBlockSize(int size) {
    if (size > BSIZE) size = BSIZE;
    block_size_ = size;
}

int QsBlockFifo::blockSize() { return block_size_; }

void QsBlockFifo::setType(int type) { type_ = type; }

int QsBlockFifo::type() { return type_; }

void QsBlockFifo::trimFifo(int maxsize) {
    while (getCount() > maxsize) {
        if (type_ == 0)
            fifo_.front();
        else
            cpx_fifo_.front();
    }
}
