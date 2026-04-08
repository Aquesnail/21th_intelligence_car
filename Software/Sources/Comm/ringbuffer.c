#include "ringbuffer.h"
#include <stdint.h>
#define false 0
#define true  1

void rb_init(RingBuffer *rb, RB_DataType *buffer, uint32_t size) {
    if (rb == NULL || buffer == NULL || size == 0) {
        return;
    }
    
    rb->pBuffer = buffer;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
}

void rb_clear(RingBuffer *rb) {
    // 为了线程安全，这里可能需要暂时屏蔽中断，取决于具体的使用场景
    rb->head = 0;
    rb->tail = 0;
}

uint8_t rb_push(RingBuffer *rb, RB_DataType dat) {
    // 计算下一个写入位置
    uint32_t next = (rb->head + 1) % rb->size;

    // 如果下一个位置等于 tail，说明满了（留一个空位用于区分空/满）
    if (next == rb->tail) {
        return false;
    }

    rb->pBuffer[rb->head] = dat;
    rb->head = next;
    return true;
}

uint8_t rb_pop(RingBuffer *rb, RB_DataType *dat) {
    // 如果 head == tail，说明是空的
    if (rb->head == rb->tail) {
        return false;
    }

    *dat = rb->pBuffer[rb->tail];
    rb->tail = (rb->tail + 1) % rb->size;
    return true;
}

uint8_t rb_is_empty(const RingBuffer *rb) {
    return (rb->head == rb->tail);
}

uint8_t rb_is_full(const RingBuffer *rb) {
    return ((rb->head + 1) % rb->size) == rb->tail;
}

uint32_t rb_available(const RingBuffer *rb) {
    if (rb->head >= rb->tail) {
        return rb->head - rb->tail;
    } else {
        return rb->size + rb->head - rb->tail;
    }
}