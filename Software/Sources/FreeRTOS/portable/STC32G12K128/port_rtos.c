/*
 * FreeRTOS Kernel V10.4.6
 * Copyright (C) 2021 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * SPDX-License-Identifier: MIT
 */

/* Standard includes. */
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "portasm.h"

/* Constants required to setup the tick ISR. */
#define portRELOAD_VALUE                    ( 65536 - configCPU_CLOCK_HZ / configTICK_RATE_HZ / 45 )

static StackType_t data xPCL;
static StackType_t data xPCH;
static StackType_t data xPCB;
volatile uint8_t data uxCriticalNesting = 0;

/* Setup the timer to generate the tick interrupts. */
static void prvPortSetupTimerInterrupt( void );

/*-----------------------------------------------------------*/

/*
 * See header file for description.
 */
StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters )
{
    uint32_t ulAddress;
    uint8_t index;

    /* Simulate how the stack would look after a call to the scheduler tick
    ISR. The return address that would have been pushed by the MCU. */
    ulAddress = ( uint32_t ) pxCode;
    *pxTopOfStack = ( StackType_t ) ( ulAddress >> 16 );    /* PC[23:16] */
    pxTopOfStack++;
    *pxTopOfStack = ( StackType_t ) ( ulAddress >> 8 );     /* PC[15:8] */
    pxTopOfStack++;
    *pxTopOfStack = ( StackType_t ) ( ulAddress );          /* PC[7:0] */

    pxTopOfStack++;
    *pxTopOfStack = 0;                                      /* PSW1 */

    /* Next all the registers will have been pushed by portSAVE_CONTEXT(). */
    /* DR56 */
    pxTopOfStack++;
    *pxTopOfStack = 0;                                      /* R56 */
    pxTopOfStack++;
    *pxTopOfStack = 1;                                      /* R57(DPXL) */
    pxTopOfStack++;
    *pxTopOfStack = 0;                                      /* R58(DPH) */
    pxTopOfStack++;
    *pxTopOfStack = 0;                                      /* R59(DPL) */

    /* DR28 DR24 DR20 DR16 DR12 DR8 DR4 */
    for(index = 0; index < 28; index++)
    {
        pxTopOfStack++;
        *pxTopOfStack = 0;
    }

    /* The function parameters will be passed in the DR0 register as
    a four byte generic pointer is used. */
    pxTopOfStack++;
    ulAddress = ( uint32_t ) pvParameters;
    *pxTopOfStack = ( StackType_t ) ( ulAddress >> 24 );    /* R0 */
    pxTopOfStack++;
    *pxTopOfStack = ( StackType_t ) ( ulAddress >> 16 );    /* R1 */
    pxTopOfStack++;
    *pxTopOfStack = ( StackType_t ) ( ulAddress >> 8 );     /* R2 */
    pxTopOfStack++;
    *pxTopOfStack = ( StackType_t ) ( ulAddress );          /* R3 */
    pxTopOfStack++;
    *pxTopOfStack = ( StackType_t ) 0;                      /* PSW */

    /* we return the top of the stack */
    return pxTopOfStack;
}

/*-----------------------------------------------------------*/

BaseType_t xPortStartScheduler( void )
{
    UNUSED( pxCurrentTCB );

    /* Start the timer that generates the tick ISR. */
    prvPortSetupTimerInterrupt();

    /* Start the first task. */
    portRESTORE_SPX();
    portRESTORE_CONTEXT();
    portRETURN_TASK();

    /* Should never get here! */
    return pdTRUE;
}

/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
    /* Not implemented for this port. */
}

/*-----------------------------------------------------------*/

/* Manual context switch. */
void vPortYield( void )
{
    UNUSED( pxCurrentTCB );

    portNORMALIZE_FUNCTION_STACK();
    portSAVE_CONTEXT();

    portEND_SWITCHING_ISR();

    portRESTORE_CONTEXT();
    portRETURN_TASK();
}

/*-----------------------------------------------------------*/

void vTimer0ISR( void )
{
    UNUSED( pxCurrentTCB );

    portNORMALIZE_INTERRUPT_STACK();
    portSAVE_CONTEXT();

#if configUSE_PREEMPTION == 1
    if( xTaskIncrementTick() != pdFALSE )
    {
        portEND_SWITCHING_ISR();
    }
   // WDT_Clear();
#else
    xTaskIncrementTick();
#endif

    portRESTORE_CONTEXT();
    portRELEASE_INTERRUPT();
    portRETURN_TASK();
}

/*-----------------------------------------------------------*/

static void prvPortSetupTimerInterrupt( void )   
{
    /* Timer0 work at 1T mode. */
    AUXR |= 0x80;

    /* Timer0 work at mode0 (16-bit auto reload mode) */
    TMOD &= ~0x0f;

    TM0PS = 45 - 1;
    /* Initial timer0 reload value (1ms per cycle). */
    TL0 = ( uint8_t )( portRELOAD_VALUE );
    TH0 = ( uint8_t )( portRELOAD_VALUE >> 8 );
    
    // 注意：TIMER0_SetIntPriority 需要在外部定义或包含对应头文件
    TIMER0_SetIntPriority(0);
    
    TR0 = 1;
    ET0 = 1;
}

/*-----------------------------------------------------------*/

/* 外部处理函数声明 */
//extern void DMA_ADC2_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken);
extern void DMA_SPI2_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken);
extern void DMA_SPI1_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken);
extern void DMA_SPI3_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken);
extern void P1_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken);
extern void P4_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken);
extern void P6_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken);
extern void P8_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken);
extern void P5_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken);
extern void P2_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken);
/* 中断包装宏定义 */
#define INSTALL_ISR_WRAPPER(__NAME, __HANDLE, __CONFIG) \
void __NAME##_ISR(void) { \
    if (__CONFIG == 1) { \
        static BaseType_t xHPTaskWoken; \
        portNORMALIZE_INTERRUPT_STACK(); \
        portSAVE_CONTEXT(); \
        xHPTaskWoken = pdFALSE; \
        __HANDLE(&xHPTaskWoken); \
        if (xHPTaskWoken != pdFALSE) { \
            portEND_SWITCHING_ISR(); \
        } \
        portRESTORE_CONTEXT(); \
        portRELEASE_INTERRUPT(); \
        portRETURN_TASK(); \
    } else { \
        __HANDLE((BaseType_t *)0); \
        portRELEASE_INTERRUPT(); /* 清除中断优先级状态 (如有必要) */ \
        portRETURN_TASK();       /* 核心：执行 ERET/RETI 返回指令 */ \
    } \
}

/* 实例化中断 */
//INSTALL_ISR_WRAPPER(DMA_ADC2, DMA_ADC2_ISR_Handle, configUSE_RTOS_DMA_ADC2)
INSTALL_ISR_WRAPPER(DMA_SPI2, DMA_SPI2_ISR_Handle, configUSE_RTOS_DMA_SPI2)
INSTALL_ISR_WRAPPER(DMA_SPI1, DMA_SPI1_ISR_Handle, configUSE_RTOS_DMA_SPI1)
INSTALL_ISR_WRAPPER(DMA_SPI3, DMA_SPI3_ISR_Handle, configUSE_RTOS_DMA_SPI3)
INSTALL_ISR_WRAPPER(P1,       P1_ISR_Handle,       configUSE_RTOS_P1)
INSTALL_ISR_WRAPPER(P4,       P4_ISR_Handle,       configUSE_RTOS_P4)
INSTALL_ISR_WRAPPER(P6,       P6_ISR_Handle,       configUSE_RTOS_P6)
INSTALL_ISR_WRAPPER(P8,       P8_ISR_Handle,       configUSE_RTOS_P8)
INSTALL_ISR_WRAPPER(P5,       P5_ISR_Handle,       configUSE_RTOS_P5)
INSTALL_ISR_WRAPPER(P2,       P2_ISR_Handle,       configUSE_RTOS_P2)
/*-----------------------------------------------------------*/
/* 硬件中断向量重定向 (汇编部分) */
/*-----------------------------------------------------------*/

// Timer0 中断 (Vector 0BH)
__asm { CSEG AT 0BH }
#if ( configUSE_ROMHUGE == 0 )
    __asm { JMP vTimer0ISR?_ }
#else
    __asm { JMP vTimer0ISR?? }
#endif

// DMA_SPI2 中断 (Vector 027BH)
__asm { CSEG AT 027BH }
#if ( configUSE_ROMHUGE == 0 )
    __asm { JMP DMA_SPI2_ISR?_ }
#else
    __asm { JMP DMA_SPI2_ISR?? }
#endif

// P4 中断 (Vector 014BH)
__asm { CSEG AT 014BH }
#if ( configUSE_ROMHUGE == 0 )
    __asm { JMP P4_ISR?_ }
#else
    __asm { JMP P4_ISR?? }
#endif

// SPI1_DMA 中断 (Vector 018BH)
__asm { CSEG AT 018BH }
#if ( configUSE_ROMHUGE == 0 )
    __asm { JMP DMA_SPI1_ISR?_ }
#else
    __asm { JMP DMA_SPI1_ISR?? }
#endif

__asm { CSEG AT 0283H }
#if ( configUSE_ROMHUGE == 0 )
    __asm { JMP DMA_SPI3_ISR?_ }
#else
    __asm { JMP DMA_SPI3_ISR?? }
#endif

__asm { CSEG AT 0153H }
#if ( configUSE_ROMHUGE == 0 )
    __asm { JMP P5_ISR?_ }
#else
    __asm { JMP P5_ISR?? }
#endif


// P6 中断 (Vector 015BH)
__asm { CSEG AT 015BH }
#if ( configUSE_ROMHUGE == 0 )
    __asm { JMP P6_ISR?_ }
#else
    __asm { JMP P6_ISR?? }
#endif

// P8 中断 (Vector 016BH)
__asm { CSEG AT 016BH }
#if ( configUSE_ROMHUGE == 0 )
    __asm { JMP P8_ISR?_ }
#else
    __asm { JMP P8_ISR?? }
#endif

// P1 中断 (Vector 0133H)
__asm { CSEG AT 0133H }
#if ( configUSE_ROMHUGE == 0 )
    __asm { JMP P1_ISR?_ }
#else
    __asm { JMP P1_ISR?? }
#endif

// P2 中断 (Vector 013BH)
__asm { CSEG AT 013BH }
#if ( configUSE_ROMHUGE == 0 )
    __asm { JMP P2_ISR?_ }
#else
    __asm { JMP P2_ISR?? }
#endif

// // DMA_ADC2 中断 (Vector 0263H)
// __asm { CSEG AT 0263H }
// #if ( configUSE_ROMHUGE == 0 )
//     __asm { JMP DMA_ADC2_ISR?_ }
// #else
//     __asm { JMP DMA_ADC2_ISR?? }
// #endif