#ifndef _DEBUG_ARCH_H
#define _DEBUG_ARCH_H

#include "stc32g144k246.h"  // 包含硬件寄存器定义
#include "dma_uart3.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "vartype.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#define DataType_Msk    0x0F
#define FreqType_Msk    0x10
#define StaticType_Msk  0x20
#define PeriType_Msk    0x40

/* ── 变量语义分类 (1字节) ── */
#define VAR_CAT_ADC_ERR     0x00  /* ADC误差权重  */
#define VAR_CAT_OUTER_LOOP  0x01  /* 外环控制     */
#define VAR_CAT_INNER_PI    0x02  /* 内环PI       */
#define VAR_CAT_SPEED_OUT   0x03  /* 速度输出     */
#define VAR_CAT_FUZZY_PID   0x04  /* 模糊PID      */
#define VAR_CAT_ELEMENT     0x05  /* 元素行为     */
#define VAR_CAT_SYSTEM      0x06  /* 系统杂项     */
#define VAR_CAT_OBSERVE     0x07  /* 纯观测变量   */
#define VAR_CAT_MOTOR_PERI  0x08  /* 电机外设     */
#define VAR_CAT_UNCATED     0xFF  /* 未分类(兼容) */

/* ── 元素类型上下文 (1字节) ── */
#define VAR_ELEM_GLOBAL     0x00  /* 全局/无特定元素 */
#define VAR_ELEM_STRAIGHT   0x01  /* 直线(默认)      */
#define VAR_ELEM_CROSS      0x02  /* 十字             */
#define VAR_ELEM_RING       0x03  /* 环岛             */
#define VAR_ELEM_WALL       0x04  /* 墙面             */

#define HIGH_FREQ_TYPE 0x10
#define LOW_FREQ_TYPE  0x00
#define STATIC_TYPE    0x20
#define PERI_TYPE      0x40

/* 外设变量描述符与回调 ---------------------------------------------------- */
typedef void (*DebugPeriReadCb_t)(uint32_t peri_addr, void* pData, uint8_t len);
typedef void (*DebugPeriWriteCb_t)(uint32_t peri_addr, void* pData, uint8_t len);

typedef struct {
    uint32_t peri_addr;
    DebugPeriReadCb_t read_cb;
    DebugPeriWriteCb_t write_cb;
} DebugPeriDesc_t;

/* 静态变量表定义宏 -------------------------------------------------------- */
#define DEBUG_VAR_TABLE_BEGIN()  \
    typedef struct { char name[17]; uint8_t type; uint8_t category; uint8_t element; void* addr; } _DebugVarEntry_t; \
    static const _DebugVarEntry_t _debugVarTable[] = {

#define DEBUG_VAR(name, type_flags, cat, elem, var_addr)  \
    { name, type_flags, cat, elem, (void*)var_addr }

#define DEBUG_VAR_PERI(name, type_flags, cat, elem, desc_ptr)  \
    { name, (type_flags) | PERI_TYPE, cat, elem, (void*)desc_ptr }

#define DEBUG_VAR_TABLE_END()  \
    }; \
    extern uint8_t debugVarCount; \
    extern void* debugVarTablePtr; \
    static void _Debug_RegisterStaticVars(void) { \
        extern void Debug_RegisterFromTable(const void* table, uint8_t count); \
        Debug_RegisterFromTable(_debugVarTable, sizeof(_debugVarTable)/sizeof(_DebugVarEntry_t)); \
    }

/* 简化静态注册宏（一键完成） */
#define DEBUG_REGISTER_STATIC_VARS()  _Debug_RegisterStaticVars()

typedef void    (*debug_write_t)(uint8_t *pData,uint16_t len);
typedef uint16_t    (*debug_read_t)(uint8_t *pData,uint16_t len);

typedef struct{
    debug_read_t debug_read;
    debug_write_t debug_write;
}debug_port_t;

extern SemaphoreHandle_t xDebugReadySignal;

void vDebugTask(void *pvParameters);
void Debug_Register(char* name, uint8_t type, uint8_t category, uint8_t element, void* addr);
void Debug_RegisterFromTable(const void* table, uint8_t count);
void Debug_Log(const char* fmt,...);
void vDebug_Log(const char* fmt, va_list args);
void Debug_LogLongString(char *buffer);
void Debug_Init(debug_port_t* debug_port_init_structure);
#endif