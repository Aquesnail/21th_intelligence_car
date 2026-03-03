#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "key.h"

#include "debug_arch.h"

/* 全局按键 ID 数组，暴露给外部 ISR 文件使用 (如 gpio_isr_handle.c) */
uint32_t g_key_ids[5];

/* FreeRTOS 定时器句柄 */
static TimerHandle_t xKeyTimer = NULL;

/* ---------------- 引脚读取函数 ---------------- */
uint8_t read_pin0(void* param){
    /* 消除未使用参数警告 */
    (void)param; 
    return P40;
}

uint8_t read_pin1(void* param){
    (void)param;
    return P65;
}

uint8_t read_pin2(void* param){
    (void)param;
    return P67;
}

uint8_t read_pin3(void* param){
    (void)param;
    return (P8IN & PIN_4) ? 1 : 0; /* 归一化为 0 或 1 */
}

uint8_t read_pin4(void* param){
    (void)param;
    return (P8IN & PIN_6) ? 1 : 0; /* 归一化为 0 或 1 */
}

/* ---------------- 事件回调函数 (按要求留空) ---------------- */
void key_event_callback(uint32_t key_id, key_event_t event) {
    /* C89 局部变量声明位置 */
    Debug_Log("Key %u %u \r\n",key_id,event);
    /* TODO: 在此处添加按键业务逻辑 */
}

/* ---------------- 核心移植接口实现 ---------------- */
static void port_enter_critical(void) {
    taskENTER_CRITICAL();
}

static void port_exit_critical(void) {
    taskEXIT_CRITICAL();
}

static void* port_malloc(size_t size) {
    return pvPortMalloc(size);
}

static void port_free(void* ptr) {
    vPortFree(ptr);
}

static void port_timer_start(void* timer_handle, uint32_t timeout_ms) {
    /* C89 局部变量声明 */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    TickType_t xTicks = pdMS_TO_TICKS(timeout_ms);
    
    /* 区分 ISR 上下文与普通任务上下文 */
   
    xTimerChangePeriod((TimerHandle_t)timer_handle, xTicks, 0);
    
}

static void port_timer_stop(void* timer_handle) {
    /* C89 局部变量声明 */
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
   
    xTimerStop((TimerHandle_t)timer_handle, 0);
    
}

/* FreeRTOS 定时器超时回调包装 */
static void vKeyTimerCallbackWrapper(TimerHandle_t xTimer) {
    (void)xTimer;
    /* 驱动核心状态机 */
    key_timer_callback();
}

/* ---------------- 按键系统初始化 ---------------- */
void key_port_init(void) {
    /* C89 局部变量必须全部声明在函数块开头 */
    key_port_t port_cfg;
    key_config_t key_cfg;
    uint8_t active_level = 0; /* 假设低电平按下有效，请根据实际硬件调整 */

    /* 1. 创建 FreeRTOS 软件定时器 */
    xKeyTimer = xTimerCreate(
        "KeyTimer", 
        pdMS_TO_TICKS(20),  
        pdFALSE,            /* 设为单次定时器，由状态机内部自动重启 */
        (void*)0, 
        vKeyTimerCallbackWrapper
    );

    /* 2. 装载移植层接口 */
    port_cfg.timer_handle   = (void*)xKeyTimer;
    port_cfg.timer_start    = port_timer_start;
    port_cfg.timer_stop     = port_timer_stop;
    port_cfg.enter_critical = port_enter_critical;
    port_cfg.exit_critical  = port_exit_critical;
    port_cfg.malloc_fn      = port_malloc;
    port_cfg.free_fn        = port_free;

    /* 初始化按键核心 */
    key_core_init(&port_cfg);

    /* 3. 准备公共配置项 */
    key_cfg.active_level = active_level;
    key_cfg.event_cb     = key_event_callback;
    key_cfg.user_data    = NULL;

    /* 4. 逐个注册按键并保存返回的 key_id */
    
    /* Key 0 (P40) */
    key_cfg.read_pin = read_pin0;
    key_register(&key_cfg, &g_key_ids[0]);

    /* Key 1 (P65) */
    key_cfg.read_pin = read_pin1;
    key_register(&key_cfg, &g_key_ids[1]);

    /* Key 2 (P67) */
    key_cfg.read_pin = read_pin2;
    key_register(&key_cfg, &g_key_ids[2]);

    /* Key 3 (P84) */
    key_cfg.read_pin = read_pin3;
    key_register(&key_cfg, &g_key_ids[3]);

    /* Key 4 (P86) */
    key_cfg.read_pin = read_pin4;
    key_register(&key_cfg, &g_key_ids[4]);
}