#include "FreeRTOS.h"
#include "task.h"
#include "debug_arch.h"
#include "storage_port.h"
#include "math.h"

#include "adc_user.h"
#include "spi1_user.h"

extern void IMU_Get_Angle(float* roll,float* pitch,float* yaw);

//以ADC的DMA回调为时间点开始运算

void MOTOR_INT1_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken);
void MOTOR_INT2_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken);
void MOTOR_DMA_Read_CallBack(BaseType_t* xSPI_DmaSemaphore);

float test1,test2,test3;

#define SIGNAL_SPI1_DMA_DONE 1 << 3

uint8_t MOTOR_SPI_TxRx(uint8_t dat) {
    uint8_t rx_dat;
    taskENTER_CRITICAL();
    rx_dat = SPI1_WriteReadByte(dat);
    taskEXIT_CRITICAL();
    return rx_dat;
}

/* SPI DMA批量读取 */
#define SIGNAL_SPI_DMA_DONE    (1 << 1)  // 位 1：DMA 传输完成 //任务通知比信号量快
uint8_t xdata SPI_DMA_Test[16];
void MOTOR_SPI_DmaRead(uint8_t* rx_buf, uint16_t len) {
    uint32_t ulNotifiedValue;
    SPI1_DMA_WriteReadBytes_IT(SPI_DMA_Test, rx_buf, len);
    if (xTaskNotifyWait(0, SIGNAL_SPI1_DMA_DONE, &ulNotifiedValue, pdMS_TO_TICKS(10)) == pdTRUE) {
        return;
    } else {
        // 异常超时处理
    }
}


void vMainControlTask(void* params){
    float num;
    (void)params;
    test2=0;
    for(;;){
        vTaskDelay(2);
        num+=0.01;
        test1 = sin(num);
        test2 += 0.02;

        test3 = MOTOR_SPI_TxRx(0x74);
    }
}

void MainControl_Init(){
    SPI1_Init(MOTOR_DMA_Read_CallBack);
    xTaskCreate(vMainControlTask,"mainTask",1024,NULL,tskIDLE_PRIORITY+3,NULL);
    Debug_Register("test1", DBG_FLOAT | HIGH_FREQ_TYPE, &(test1));
    Debug_Register("test2", DBG_FLOAT | HIGH_FREQ_TYPE, &(test2));
    Debug_Register("test3", DBG_FLOAT | HIGH_FREQ_TYPE, &(test3));
}

void MOTOR_DMA_Read_CallBack(BaseType_t* xSPI_DmaSemaphore){

}