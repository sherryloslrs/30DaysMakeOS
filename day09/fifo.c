#include "bootpack.h"

#define FLAGS_OVERRUN       0x0001

void fifo8_init(Fifo8* fifo, int size, unsigned char *buf) {
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size;
    fifo->flags = 0;
    fifo->p = 0;
    fifo->q = 0;
    return;
}

int fifo8_put(Fifo8* fifo, unsigned char data) {
    if(fifo->free == 0) {
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->p] = data;
    fifo->p ++;
    if (fifo->p == fifo->size)
        fifo->p = 0;
    fifo->free --;
    return 0;
}

int fifo8_get(Fifo8* fifo) {
    if(fifo->free == fifo->size)
        return -1;
    int data = fifo->buf[fifo->q];
    fifo->q ++;
    if(fifo->q == fifo->size)
        fifo->q = 0;
    fifo->free++;
    return data;
}

int fifo8_status(Fifo8* fifo) {
    return fifo->size - fifo->free;
}