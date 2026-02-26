#ifndef SQUEUE_H
#define SQUEUE_H

#include <stdint.h>  // for uint8_t, uint16_t
#include "config.h"
/**
 * 通用循环队列结构体
 * 可存储任意类型元素，通过 void* 缓冲区和元素大小实现通用性
 */
typedef struct {
    void *dat;             // 缓冲区起始地址（按字节寻址）
    uint16_t capacity;      // 队列最大元素个数
    uint16_t element_size;  // 每个元素占用的字节数
    uint16_t front;         // 队头索引（指向第一个有效元素）
    uint16_t rear;          // 队尾索引（指向下一个空闲位置）
} sQueue_t;

/**
 * 初始化队列
 * @param q          队列对象指针
 * @param buffer     用户提供的缓冲区（大小至少为 capacity * element_size 字节）
 * @param cap        队列容量（元素个数，实际最多存储 cap-1 个元素）
 * @param elem_size  单个元素大小（字节）
 */
void sQueue_init(sQueue_t *q, void *buffer, uint16_t cap, uint16_t elem_size);

/**
 * 判断队列是否为空
 * @param q 队列对象指针
 * @return  1（空）或 0（非空）
 */
uint8_t sQueue_isEmpty(const sQueue_t *q);

/**
 * 判断队列是否已满
 * @param q 队列对象指针
 * @return  1（满）或 0（未满）
 */
uint8_t sQueue_isFull(const sQueue_t *q);

/**
 * 入队：将元素 elem 复制到队尾
 * @param q    队列对象指针
 * @param elem 指向待入队元素的指针
 * @return     1（成功）或 0（队列满，失败）
 */
uint8_t sQueue_enqueue(sQueue_t *q, const void *elem);

/**
 * 出队：将队首元素复制到 elem 指向的内存，并移除队首
 * @param q    队列对象指针
 * @param elem 接收元素的内存指针（需足够容纳一个元素）
 * @return     1（成功）或 0（队列空，失败）
 */
uint8_t sQueue_dequeue(sQueue_t *q, void *elem);

/**
 * 查看队首元素（不出队）
 * @param q    队列对象指针
 * @param elem 接收元素的内存指针
 * @return     1（成功）或 0（队列空，失败）
 */
uint8_t sQueue_peek(const sQueue_t *q, void *elem);

#endif // SQUEUE_H