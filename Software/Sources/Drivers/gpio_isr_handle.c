#include "config.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "portasm.h"
 
extern void IMU_INT_ISR_Handle( BaseType_t* xHigherPriorityTaskWoken);

void P4_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken){
    uint8_t intf;

    intf = CheckP4nFlag(PIN_ALL);       //读取中断请求标志
    if(intf & PIN_6){
        IMU_INT_ISR_Handle(xHigherPriorityTaskWoken);
    }else if(intf & PIN_0){

    }
    ClearP4nFlag(PIN_ALL);       
}


