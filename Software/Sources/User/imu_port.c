#include "icm42688.h"
#include "icm42688_defs.h"
#include "spi2_user.h"
#include "delay.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "imu_filter.h"
#include "portasm.h"

static ICM42688_t icm42688_Init_Structure;
static SemaphoreHandle_t xDmaSemaphore;
static SemaphoreHandle_t xFIFOIntSemaphore;
static uint8_t xdata FIFO_Buffer[2048];
static IMU_Filter_t IMU_Filter_Data;
uint8_t IMU_Read_Bytes_DMA_IT(uint8_t reg,uint8_t xdata* dat,uint16_t len){ //这里只能传入Xdata域内的数组，不要传输错了
    
    ICM42688_CS_RESET();
    SPI2_WriteByte(reg | 0x80);
    SPI2_DMA_ReadBytes_IT(dat,len);

    if(xSemaphoreTake(xDmaSemaphore, pdMS_TO_TICKS(10)) == pdTRUE) { //这是用于同步的二值信号量，它是空的，在中断回调Give之前它都是拿不到的，会阻塞在这里直到回调完成
        // 获取成功 (DMA 完成)
    } else {
        // 超时处理 (比如复位 SPI，或者记录错误)
    }

    ICM42688_CS_SET();
    return 0;
}
uint8_t IMU_Read_Byte(uint8_t reg,uint8_t *dat){
    ICM42688_CS_RESET();

    SPI2_WriteByte(reg|0x80);
    *dat = SPI2_ReadByte();

    ICM42688_CS_SET();
    return 0;
}

uint8_t IMU_Write_Byte(uint8_t reg,uint8_t dat){
    ICM42688_CS_RESET();
    SPI2_WriteByte(reg);
    SPI2_WriteByte(dat);
    ICM42688_CS_SET();
    return 0;
}

void IMU_INT_ISR_Handle( BaseType_t* xHigherPriorityTaskWoken){
    
    if(1 /*标志位*/){
        xSemaphoreGiveFromISR(xFIFOIntSemaphore,xHigherPriorityTaskWoken);
        //清除标志位
    }
}

void IMU_DMA_Read_CallBack(BaseType_t* xSPI_DmaSemaphore){
   
    xSemaphoreGiveFromISR(xDmaSemaphore,xSPI_DmaSemaphore);
    //这里会释放信号量
    return;
}

void vIMUProcessTask(void *pvParameters){
    int16_t raw_acc_x;
    int16_t raw_acc_y ;
    int16_t raw_acc_z ;
    int16_t raw_gyro_x ;
    int16_t raw_gyro_y ;
    int16_t raw_gyro_z ;
    uint8_t raw_temp;
    uint16_t raw_timestamp;
    uint16_t i;
    uint8_t xdata* ptr;
    float ax,ay,az,gx,gy,gz;
    ptr=FIFO_Buffer;
	
		(void)pvParameters;
    for(;;){
        if(xSemaphoreTake(xFIFOIntSemaphore,pdMS_TO_TICKS(100)) == pdTRUE){
            ICM42688_ReadFIFO(&icm42688_Init_Structure,FIFO_Buffer);
            for(i=0;i<icm42688_Init_Structure.FIFO_THS;i++){
                raw_acc_x = (int16_t)((ptr[1] << 8) | ptr[2]);
                raw_acc_y = (int16_t)((ptr[3] << 8) | ptr[4]);
                raw_acc_z = (int16_t)((ptr[5] << 8) | ptr[6]);
                raw_gyro_x = (int16_t)((ptr[7] << 8) | ptr[8]);
                raw_gyro_y = (int16_t)((ptr[9] << 8) | ptr[10]);
                raw_gyro_z = (int16_t)((ptr[11] << 8) | ptr[12]);
                raw_temp = ptr[13];
                raw_timestamp = (uint16_t)((ptr[14] << 8) | ptr[15]);

                IMU_Filter_Data.ax = raw_acc_x * icm42688_Init_Structure.acc_sensitivity * 9.80665f;
                IMU_Filter_Data.ay = raw_acc_y * icm42688_Init_Structure.acc_sensitivity * 9.80665f;
                IMU_Filter_Data.az = raw_acc_z * icm42688_Init_Structure.acc_sensitivity * 9.80665f;

                IMU_Filter_Data.gx = raw_gyro_x * icm42688_Init_Structure.gyro_sensitivity;
                IMU_Filter_Data.gy = raw_gyro_y * icm42688_Init_Structure.gyro_sensitivity;
                IMU_Filter_Data.gz = raw_gyro_z * icm42688_Init_Structure.gyro_sensitivity;

                IMU_Filter_Data.time_stamp =  raw_timestamp;
                IMU_Filter_Data.temp = (raw_temp / 2.07f) + 25.0f;
                IMU_Filter_Update(&IMU_Filter_Data);

                ptr+=16;
            }
        }else{
            continue;
        }
    }
}

void IMU_Init(){
    icm42688_Init_Structure.read = IMU_Read_Bytes_DMA_IT;
    icm42688_Init_Structure.read_byte = IMU_Read_Byte;
    icm42688_Init_Structure.write = IMU_Write_Byte;
    icm42688_Init_Structure.delay = delay_ms;
    icm42688_Init_Structure.FIFO_THS = 32;
    xDmaSemaphore = xSemaphoreCreateBinary();
    xFIFOIntSemaphore = xSemaphoreCreateBinary();
    SPI2_Init(IMU_DMA_Read_CallBack);
    xTaskCreate(vIMUProcessTask,"IMU",512,NULL,tskIDLE_PRIORITY+4,NULL);
}

