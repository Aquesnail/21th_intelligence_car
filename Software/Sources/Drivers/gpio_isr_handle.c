#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"


extern uint32_t g_key_ids[5];
extern void key_isr_handler(uint32_t key_id);
volatile uint8_t g_p54_key_flag = 0;

extern void IMU_INT1_ISR_Handle( BaseType_t* xHigherPriorityTaskWoken);
extern void Motor_Data_Ready_INT2_Handle(BaseType_t* xHigherPriorityTaskWoken);

// ============================================================
// P3 中断: IMU + MOTOR 中断输入集中处理
// P3.3: SPI_IMU_INT2    P3.4: SPI_IMU_INT1
// P3.6: SPI_MOTOR_INT2  P3.7: SPI_MOTOR_INT1
// ============================================================
void P3_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken){
    uint8_t intf;

    intf = CheckP3nFlag(PIN_ALL);
    if(intf & PIN_3){                       // P3.3: SPI_IMU_INT2
        IMU_INT1_ISR_Handle(xHigherPriorityTaskWoken);
    }
    if(intf & PIN_4){                       // P3.4: SPI_IMU_INT1
        IMU_INT1_ISR_Handle(xHigherPriorityTaskWoken);
    }
    if(intf & PIN_6){                       // P3.6: SPI_MOTOR_INT2
        Motor_Data_Ready_INT2_Handle(xHigherPriorityTaskWoken);
    }
    if(intf & PIN_7){                       // P3.7: SPI_MOTOR_INT1
        // TODO: 添加 MOTOR_INT1 处理函数
    }
    ClearP3nFlag(PIN_ALL);
}

// ============================================================
// P1 中断: 全部高阻输入，保留框架
// ============================================================
void P1_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken){
    uint8_t intf;

    intf = CheckP1nFlag(PIN_ALL);
    ClearP1nFlag(PIN_ALL);
}

// ============================================================
// P2 中断: 全部高阻输入，保留框架
// ============================================================
void P2_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken){
    uint8_t intf;

    intf = CheckP2nFlag(PIN_ALL);
    ClearP2nFlag(PIN_ALL);
}

// ============================================================
// P4 中断: 全部高阻输入，保留框架
// ============================================================
void P4_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken){
    uint8_t intf;

    intf = CheckP4nFlag(PIN_ALL);
    ClearP4nFlag(PIN_ALL);
}

// ============================================================
// P5 中断: P5.4 按键
// ============================================================
void P5_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken){
    uint8_t intf;

    intf = CheckP5nFlag(PIN_ALL);
    if(intf & PIN_4){
        g_p54_key_flag = 1;
    }
    ClearP5nFlag(PIN_ALL);
}

// ============================================================
// P6 中断: 全部高阻输入 (SPI功能不产生中断)，保留框架
// ============================================================
void P6_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken){
    uint8_t intf;

    intf = CheckP6nFlag(PIN_ALL);
    ClearP6nFlag(PIN_ALL);
}

// ============================================================
// P8 中断: 全部高阻输入，保留框架
// ============================================================
void P8_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken){
    uint8_t intf;

    intf = CheckP8nFlag(PIN_ALL);
    ClearP8nFlag(PIN_ALL);
}
