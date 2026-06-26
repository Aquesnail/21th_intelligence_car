/**
 ******************************************************************************
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 *
 * Adapted: no-logging stubs + trace_print → Debug_Log bridge for STC32G.
 *          VL53L1_LOG_ENABLE is NOT defined by default.
 *
 * C51-compatible: all macros use fixed-parameter counts (no variadic `...`).
 * trace_print is a real C function (variadic functions are C89 — OK on C51).
 ******************************************************************************
 */

#ifndef _VL53L1_PLATFORM_LOG_H_
#define _VL53L1_PLATFORM_LOG_H_

/* ————— Trace level / module / function constants (always available) —————
 *
 * Defined before any #include so that modules/*.c files can use them in
 * trace_print() calls regardless of VL53L1_LOG_ENABLE.
 * All values are plain hex literals — no types required.
 */

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

/*
 * trace_print(level, fmt, ...) — real variadic function, NOT a macro.
 *
 * Uses `unsigned long` for 'level' — avoids pulling in vl53l1_types.h
 * (which #defines `data`→`_data`, breaking FreeRTOS C51 memory-space
 * keywords in portmacro.h).
 *
 * Forwards fmt + args directly to vDebug_Log() (see debug_arch.h).
 */
void trace_print(unsigned long level, const char *fmt, ...);

#ifdef VL53L1_LOG_ENABLE
    /* ————— Logging enabled ————— */
    #include "vl53l1_types.h"               /* uint32_t etc. — safe: only when explicitly enabled */
    #include "vl53l1_platform_user_config.h"

    extern uint32_t _trace_level;

    void VL53L1_trace_print_module_function(
        uint32_t module, uint32_t level, uint32_t function,
        const char *format, ...);

    uint32_t VL53L1_clock(void);
    uint32_t VL53L1_get_trace_functions(void);
    void VL53L1_set_trace_functions(uint32_t function);

#else
    /* ————— Logging disabled (default) —————
     *
     * C51-compatible stub macros (no variadic `...`).
     *
     * LOG_FUNCTION_* are empty — every modules/*.c has its local
     * definitions commented out; these stubs are the sole definitions.
     *
     * trace_print is a real function (declared above, defined in
     * vl53l1_platform_log.c).  It bridges directly to Debug_Log().
     */

    #define LOG_FUNCTION_START(fmt)
    #define LOG_FUNCTION_END(status)
    #define LOG_FUNCTION_END_FMT(status, fmt)

    #define _LOG_TRACE_PRINT(module, level, function)
    #define _LOG_FUNCTION_START(module, fmt)
    #define _LOG_FUNCTION_END(module, status)
    #define _LOG_FUNCTION_END_FMT(module, status, fmt)
    #define _LOG_GET_TRACE_FUNCTIONS()          0
    #define _LOG_SET_TRACE_FUNCTIONS(functions) ((void)0)
    #define _LOG_STRING_BUFFER(x)               ((void)0)

#endif

#endif /* _VL53L1_PLATFORM_LOG_H_ */
