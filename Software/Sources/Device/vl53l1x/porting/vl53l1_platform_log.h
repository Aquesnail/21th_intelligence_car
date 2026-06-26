/**
 ******************************************************************************
 * Copyright (c) 2020, STMicroelectronics - All Rights Reserved
 *
 * Adapted: no-logging stubs for STC32G.
 *          VL53L1_LOG_ENABLE is NOT defined by default.
 *
 * When logging is OFF we pre-define LOG_FUNCTION_START / LOG_FUNCTION_END
 * as empty macros so that the local definitions inside every modules/*.c
 * are a harmless no-op redefinition (C251 tolerates identical-parameter
 * redefinitions).
 ******************************************************************************
 */

#ifndef _VL53L1_PLATFORM_LOG_H_
#define _VL53L1_PLATFORM_LOG_H_

#ifdef VL53L1_LOG_ENABLE
    /* ————— Logging enabled ————— */
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
    /* ————— Logging disabled (default) —————
     *
     * C51-compatible stub macros (no variadic `...` — C51 treats it as a
     * literal parameter name, causing C307 at every call site).
     *
     * Every modules/*.c file has its local LOG_FUNCTION_* definitions
     * commented out; these stubs are the sole definitions.
     *
     * To re-enable logging: define VL53L1_LOG_ENABLE, uncomment the local
     * definitions in the relevant .c files, and provide implementations
     * for _LOG_FUNCTION_START / _LOG_FUNCTION_END / trace_print.
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

    /* trace_print is always called as trace_print(LEVEL, "msg") — 2 args */
    #define trace_print(level, fmt)

#endif

#endif /* _VL53L1_PLATFORM_LOG_H_ */
