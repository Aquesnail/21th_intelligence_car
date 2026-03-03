#ifndef _KEY_H
#define _KEY_H

#include "config.h"
#include <stdint.h>
#include <stddef.h>

/* 定义按键事件枚举 */
typedef enum {
    KEY_EVENT_SINGLE_CLICK,
    KEY_EVENT_DOUBLE_CLICK,
    KEY_EVENT_LONG_PRESS
} key_event_t;

/* 函数指针类型定义 */
typedef void (*key_event_cb_t)(uint32_t key_id, key_event_t event);
typedef uint8_t (*key_read_pin_t)(void* user_data);
typedef void (*key_timer_start_t)(void* timer_handle, uint32_t timeout_ms);
typedef void (*key_timer_stop_t)(void* timer_handle);
typedef void (*key_critical_t)(void);
typedef void* (*key_malloc_t)(size_t size);
typedef void (*key_free_t)(void* ptr);

/* 核心移植层接口 (全局仅需配置一次) */
typedef struct {
    void* timer_handle;              
    key_timer_start_t timer_start;   
    key_timer_stop_t timer_stop;     
    key_critical_t enter_critical;   
    key_critical_t exit_critical;    
    key_malloc_t malloc_fn;          
    key_free_t free_fn;              
} key_port_t;

/* 单个按键的业务配置 */
typedef struct {
    uint8_t active_level;            
    key_read_pin_t read_pin;         
    key_event_cb_t event_cb;         
    void* user_data;
} key_config_t;

/* 系统初始化：注入 RTOS 依赖 */
int key_core_init(const key_port_t* port_config);

/* 注册具体的按键 (动态分配 ID) */
int key_register(const key_config_t* config, uint32_t* out_key_id);

/* 注销按键，释放内存 */
int key_unregister(uint32_t key_id);

/* 外部中断处理：唤醒全局定时器 */
void key_isr_handler(uint32_t key_id);

/* 全局定时器回调：自动遍历并处理所有按键状态 */
void key_timer_callback(void);

#endif /* _KEY_H */