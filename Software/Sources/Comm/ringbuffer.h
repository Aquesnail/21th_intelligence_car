#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "config.h"

/* 用户配置: 数据类型定义 */
typedef uint8_t RB_DataType;

/* RingBuffer 结构体对象 */
typedef struct {
    RB_DataType *pBuffer; // 指向实际数据存储区的指针
    uint32_t size;        // 缓冲区总大小
    volatile uint32_t head; // 写索引 (volatile 确保中断安全)
    volatile uint32_t tail; // 读索引 (volatile 确保中断安全)
} RingBuffer;

/**
 * @brief 初始化 RingBuffer
 * @param rb 指向 RingBuffer 结构体的指针
 * @param buffer 指向用户分配的数组的指针
 * @param size 数组的大小
 */
void rb_init(RingBuffer *rb, RB_DataType *buffer, uint32_t size);

/**
 * @brief 重置/清空缓冲区
 */
void rb_clear(RingBuffer *rb);

/**
 * @brief 写入一个字节
 * @return true 写入成功, false 缓冲区已满
 */
uint8_t rb_push(RingBuffer *rb, RB_DataType dat);

/**
 * @brief 读取一个字节
 * @param dat 用于保存读取数据的指针
 * @return true 读取成功, false 缓冲区为空
 */
uint8_t rb_pop(RingBuffer *rb, RB_DataType *dat);

/**
 * @brief 检查缓冲区是否为空
 */
uint8_t rb_is_empty(const RingBuffer *rb);

/**
 * @brief 检查缓冲区是否已满
 */
uint8_t rb_is_full(const RingBuffer *rb);

/**
 * @brief 获取当前可读取的数据长度
 */
uint32_t rb_available(const RingBuffer *rb);

#ifdef __cplusplus
}
#endif

#endif // RINGBUFFER_H