/**
 ******************************************************************************
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 *
 * Adapted: C89-compatible no-logging stubs for STC32G.
 *          VL53L1_LOG_ENABLE is NOT defined → all trace macros are empty.
 ******************************************************************************
 */

#ifndef _VL53L1_PLATFORM_LOG_H_
#define _VL53L1_PLATFORM_LOG_H_

/*
 * Logging is disabled by default.
 * To enable, #define VL53L1_LOG_ENABLE and implement the trace callbacks.
 */
#ifdef VL53L1_LOG_ENABLE
    /* ——— Logging enabled ——— */
    #include "vl53l1_platform_user_config.h"
    #include "vl53l1_types.h"

    #define VL53L1_TRACE_LEVEL_NONE       0x00000000
    #define VL53L1_TRACE_LEVEL_ERRORS     0x00000001
    #define VL53L1_TRACE_LEVEL_WARNING    0x00000002
    #define VL53L1_TRACE_LEVEL_INFO       0x00000004
    #define VL53L1_TRACE_LEVEL_DEBUG      0x00000008
    #define VL53L1_TRACE_LEVEL_ALL        0x00000010
    #define VL53L1_TRACE_LEVEL_IGNORE     0x00000020

    #define VL53L1_TRACE_FUNCTION_NONE    0x00000000
    #define VL53L1_TRACE_FUNCTION_I2C     0x00000001
    #define VL53L1_TRACE_FUNCTION_ALL     0x7fffffff

    #define VL53L1_TRACE_MODULE_NONE      0x00000000
    #define VL53L1_TRACE_MODULE_API       0x00000001
    #define VL53L1_TRACE_MODULE_CORE      0x00000002
    #define VL53L1_TRACE_MODULE_PROTECTED 0x00000004
    #define VL53L1_TRACE_MODULE_HISTOGRAM 0x00000008
    #define VL53L1_TRACE_MODULE_REGISTERS 0x00000010
    #define VL53L1_TRACE_MODULE_PLATFORM  0x00000020
    #define VL53L1_TRACE_MODULE_NVM       0x00000040
    #define VL53L1_TRACE_MODULE_ALL       0x7fffffff

    extern uint32_t _trace_level;

    void VL53L1_trace_print_module_function(
        uint32_t module, uint32_t level, uint32_t function,
        const char *format, ...);

    uint32_t VL53L1_clock(void);
    uint32_t VL53L1_get_trace_functions(void);
    void VL53L1_set_trace_functions(uint32_t function);

#else
    /* ——— Logging disabled (default) ———
     * C89-compatible: fixed-parameter macros, no `...` / __VA_ARGS__.
     * Modules/*.c still define LOG_FUNCTION_START(fmt,...) locally,
     * but they resolve to these stubs which discard everything.
     */
    #define _LOG_TRACE_PRINT(module, level, function, dummy)
    #define _LOG_FUNCTION_START(module, fmt, dummy)
    #define _LOG_FUNCTION_END(module, status, dummy)
    #define _LOG_FUNCTION_END_FMT(module, status, fmt, dummy)
    #define _LOG_GET_TRACE_FUNCTIONS()          0
    #define _LOG_SET_TRACE_FUNCTIONS(functions) ((void)0)
    #define _LOG_STRING_BUFFER(x)               ((void)0)
#endif

#endif /* _VL53L1_PLATFORM_LOG_H_ */
