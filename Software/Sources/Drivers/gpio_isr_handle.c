#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

 
extern uint32_t g_key_ids[5];
extern void key_isr_handler(uint32_t key_id);

extern void IMU_INT1_ISR_Handle( BaseType_t* xHigherPriorityTaskWoken);

void P1_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken){
    uint8_t intf;

    intf = CheckP1nFlag(PIN_ALL);
    if(intf & PIN_5){
        IMU_INT1_ISR_Handle(xHigherPriorityTaskWoken);
		}else if(intf & PIN_4){
				IMU_INT1_ISR_Handle(xHigherPriorityTaskWoken);
		}
    ClearP1nFlag(PIN_ALL);       
}

void P4_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken){
    uint8_t intf;

    intf = CheckP4nFlag(PIN_ALL);       //读取中断请求标志
    if(intf & PIN_0){
        key_isr_handler(g_key_ids[0]);
    }
    ClearP4nFlag(PIN_ALL);       
}


void P6_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken){
    uint8_t intf;

    intf = CheckP6nFlag(PIN_ALL);       //读取中断请求标志
    if(intf & PIN_5){
        key_isr_handler(g_key_ids[1]);
    }else if(intf & PIN_7){
        key_isr_handler(g_key_ids[2]);
    }
    ClearP6nFlag(PIN_ALL);       
}


void P8_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken){ 
    uint8_t intf;

    intf = CheckP8nFlag(PIN_ALL);       //读取中断请求标志
    if(intf & PIN_4){
        key_isr_handler(g_key_ids[3]);
    }else if(intf & PIN_6){
        key_isr_handler(g_key_ids[4]);
    }
    ClearP8nFlag(PIN_ALL);       
}


