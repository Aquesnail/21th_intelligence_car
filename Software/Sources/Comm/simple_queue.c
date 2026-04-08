#include "simple_queue.h"
#include <string.h>  // for memcpy
#include <stdint.h>  // for uint8_t, uint16_t
void sQueue_init(sQueue_t *q, void *buffer, uint16_t cap, uint16_t elem_size) {
    // 局部变量声明（本函数无需额外变量）
    q->dat = buffer;
    q->capacity = cap;
    q->element_size = elem_size;
    q->front = 0;
    q->rear = 0;
}

uint8_t sQueue_isEmpty(const sQueue_t *q) {
    // 局部变量声明
    uint8_t empty;

    empty = (q->front == q->rear) ? 1 : 0;
    return empty;
}

uint8_t sQueue_isFull(const sQueue_t *q) {
    // 局部变量声明
    uint8_t full;

    full = ((q->rear + 1) % q->capacity == q->front) ? 1 : 0;
    return full;
}

uint8_t sQueue_enqueue(sQueue_t *q, const void *elem) {
    // 局部变量声明
    uint8_t success;
    uint8_t *byte_ptr;

    if (sQueue_isFull(q)) {
        success = 0;
    } else {
        byte_ptr = (uint8_t*)q->dat + q->rear * q->element_size;
        memcpy(byte_ptr, elem, q->element_size);
        q->rear = (q->rear + 1) % q->capacity;
        success = 1;
    }
    return success;
}

uint8_t sQueue_dequeue(sQueue_t *q, void *elem) {
    // 局部变量声明
    uint8_t success;
    uint8_t *byte_ptr;

    if (sQueue_isEmpty(q)) {
        success = 0;
    } else {
        byte_ptr = (uint8_t*)q->dat + q->front * q->element_size;
        memcpy(elem, byte_ptr, q->element_size);
        q->front = (q->front + 1) % q->capacity;
        success = 1;
    }
    return success;
}

uint8_t sQueue_peek(const sQueue_t *q, void *elem) {
    // 局部变量声明
    uint8_t success;
    const uint8_t *byte_ptr;

    if (sQueue_isEmpty(q)) {
        success = 0;
    } else {
        byte_ptr = (const uint8_t*)q->dat + q->front * q->element_size;
        memcpy(elem, byte_ptr, q->element_size);
        success = 1;
    }
    return success;
}