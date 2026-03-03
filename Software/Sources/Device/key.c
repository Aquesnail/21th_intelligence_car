#include "key.h"

#define KEY_POLL_INTERVAL_MS   20
#define KEY_LONG_PRESS_MS      500
#define KEY_DOUBLE_WAIT_MS     300

#define KEY_LONG_PRESS_TICKS   (KEY_LONG_PRESS_MS / KEY_POLL_INTERVAL_MS)
#define KEY_DOUBLE_WAIT_TICKS  (KEY_DOUBLE_WAIT_MS / KEY_POLL_INTERVAL_MS)

/* 定义系统最大支持的按键数量，用于 O(1) 数组映射 */
#ifndef KEY_MAX_NUM
#define KEY_MAX_NUM            16
#endif

typedef enum {
    KEY_STATE_IDLE,
    KEY_STATE_DEBOUNCE_PRESS,
    KEY_STATE_PRESSED,
    KEY_STATE_WAIT_DOUBLE,
    KEY_STATE_DEBOUNCE_DOUBLE,
    KEY_STATE_WAIT_RELEASE
} key_state_t;

typedef struct key_node_s {
    uint32_t key_id;                 
    key_config_t config;             
    volatile key_state_t state;      /* 增加 volatile，因为在中断和任务中都会访问 */
    uint32_t state_ticks;    
    uint8_t release_filter;        
} key_node_t;

/* 全局变量：使用指针数组替代链表，实现 O(1) 访问 */
static key_node_t* g_key_map[KEY_MAX_NUM] = {NULL};
static key_port_t g_port = {0};
static uint8_t g_is_initialized = 0;

/* 初始化全局 RTOS 接口 */
int key_core_init(const key_port_t* port_config) {
    if (port_config == NULL || port_config->malloc_fn == NULL || port_config->timer_start == NULL) {
        return 1;
    }
    g_port = *port_config;
    g_is_initialized = 1;
    return 0;
}

/* 注册具体按键 */
int key_register(const key_config_t* config, uint32_t* out_key_id) {
    int free_idx = -1;
		uint8_t i;
    key_node_t* new_node;

    if (!g_is_initialized || config == NULL || config->read_pin == NULL) {
        return 1; 
    }

    if (g_port.enter_critical) g_port.enter_critical();

    /* 寻找空闲的 ID 槽位 */
    for (i = 0; i < KEY_MAX_NUM; i++) {
        if (g_key_map[i] == NULL) {
            free_idx = i;
            break;
        }
    }

    if (free_idx == -1) {
        if (g_port.exit_critical) g_port.exit_critical();
        return 2; /* 达到最大按键数量限制 */
    }

    new_node = (key_node_t*)g_port.malloc_fn(sizeof(key_node_t));
    if (new_node == NULL) {
        if (g_port.exit_critical) g_port.exit_critical();
        return 3; /* 内存分配失败 */
    }

    new_node->key_id = free_idx; 
    new_node->config = *config;
    new_node->state = KEY_STATE_IDLE;
    new_node->state_ticks = 0;
    
    g_key_map[free_idx] = new_node;

    if (out_key_id != NULL) {
        *out_key_id = free_idx;
    }

    if (g_port.exit_critical) g_port.exit_critical();
    return 0;
}

/* 注销按键 */
int key_unregister(uint32_t key_id) {
    if (!g_is_initialized || key_id >= KEY_MAX_NUM) return 1;

    if (g_port.enter_critical) g_port.enter_critical();
    
    if (g_key_map[key_id] != NULL) {
        if (g_port.free_fn) {
            g_port.free_fn(g_key_map[key_id]);
        }
        g_key_map[key_id] = NULL;
    }

    if (g_port.exit_critical) g_port.exit_critical();
    return 0;
}

/* 中断触发：O(1) 查表并启动全局定时器 */
void key_isr_handler(uint32_t key_id) {
    key_node_t* node;

    if (!g_is_initialized || key_id >= KEY_MAX_NUM) return;

    node = g_key_map[key_id];
    if (node != NULL) {
        if (node->state == KEY_STATE_IDLE) {
            node->state = KEY_STATE_DEBOUNCE_PRESS;
            node->state_ticks = 0;
            
            /* 注意：此处的 timer_start 必须是 RTOS 支持在 ISR 中调用的 API */
            g_port.timer_start(g_port.timer_handle, KEY_POLL_INTERVAL_MS);
        }
    }
}

/* 定时器状态机轮询 */
void key_timer_callback(void) {
    key_node_t* node;
    uint8_t pin_level;
    uint8_t is_active;
    uint8_t any_key_active = 0;
	uint8_t i;
    key_event_t event_to_emit;
    int emit_event;

    if (!g_is_initialized) return;

    for (i = 0; i < KEY_MAX_NUM; i++) {
        node = g_key_map[i];
        if (node == NULL || node->state == KEY_STATE_IDLE) continue;

        emit_event = 0;
        event_to_emit = KEY_EVENT_SINGLE_CLICK;

        /* 读取硬件电平 */
        pin_level = node->config.read_pin(node->config.user_data);
        is_active = (pin_level == node->config.active_level);

        node->state_ticks++;

        /* 完整状态机逻辑 */
        switch (node->state) {
            case KEY_STATE_DEBOUNCE_PRESS:
                if (is_active) {
                    node->state = KEY_STATE_PRESSED;
                    node->state_ticks = 0;
                } else {
                    node->state = KEY_STATE_IDLE; // 抖动，回到空闲
                }
                break;

            case KEY_STATE_PRESSED:
                /* 如果检测到按下，清除松手滤波计数，并继续累加长按时间 */
                if (is_active) {
                    node->release_filter = 0; /* 只要有一次接触良好，就重置松手计数 */

                    if (node->state_ticks >= KEY_LONG_PRESS_TICKS) {
                        event_to_emit = KEY_EVENT_LONG_PRESS;
                        emit_event = 1;
                        node->state = KEY_STATE_WAIT_RELEASE; 
                    }
                } 
                /* 检测到松开：不立即跳转，而是先累加“松手确认计数器” */
                else {
                    node->release_filter++;
                    
                    /* 只有连续 3 次 (3 * 20ms = 60ms) 检测到松开，才认定为真松手 */
                    /* 你可以根据损坏程度调整这个阈值，3-5 之间通常比较合适 */
                    if (node->release_filter >= 3) {
                        node->state = KEY_STATE_WAIT_DOUBLE;
                        node->state_ticks = 0;
                        node->release_filter = 0;
                    }
                    /* 如果没达到阈值，说明是接触不良造成的毛刺，忽略它，保持 PRESSED 状态 */
                }
                break;

            case KEY_STATE_WAIT_DOUBLE:
                if (is_active) {
                    // 在等待双击期间再次按下，进入双击消抖
                    node->state = KEY_STATE_DEBOUNCE_DOUBLE;
                    node->state_ticks = 0;
                } else {
                    if (node->state_ticks >= KEY_DOUBLE_WAIT_TICKS) {
                        // 等待双击超时，确认为单击
                        event_to_emit = KEY_EVENT_SINGLE_CLICK;
                        emit_event = 1;
                        node->state = KEY_STATE_IDLE;
                    }
                }
                break;

            case KEY_STATE_DEBOUNCE_DOUBLE:
                if (is_active) {
                    // 确认双击
                    event_to_emit = KEY_EVENT_DOUBLE_CLICK;
                    emit_event = 1;
                    node->state = KEY_STATE_WAIT_RELEASE;
                } else {
                    // 抖动，退回等待双击
                    node->state = KEY_STATE_WAIT_DOUBLE;
                }
                break;

            case KEY_STATE_WAIT_RELEASE:
                if (!is_active) {
                    // 物理松开后回到空闲
                    node->state = KEY_STATE_IDLE;
                }
                break;

            default:
                node->state = KEY_STATE_IDLE;
                break;
        }

        /* 触发回调 */
        if (emit_event && node->config.event_cb != NULL) {
            node->config.event_cb(node->key_id, event_to_emit);
        }
    }

    /* --- 修复竞态条件：进入临界区检查是否可以停止定时器 --- */
    if (g_port.enter_critical) g_port.enter_critical();
    
    any_key_active = 0;
    for (i = 0; i < KEY_MAX_NUM; i++) {
        if (g_key_map[i] != NULL && g_key_map[i]->state != KEY_STATE_IDLE) {
            any_key_active = 1;
            break;
        }
    }

    if (any_key_active) {
        g_port.timer_start(g_port.timer_handle, KEY_POLL_INTERVAL_MS);
    } else {
        if (g_port.timer_stop != NULL) {
            g_port.timer_stop(g_port.timer_handle);
        }
    }

    if (g_port.exit_critical) g_port.exit_critical();
}