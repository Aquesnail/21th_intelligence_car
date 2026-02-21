#include "icm42688.h"
#include "icm42688_defs.h"
#include "spi2_user.h"
#include "delay.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "imu_filter.h"
#include "debug_arch.h"
#include "port.h"

#pragma warning disable = 138
#pragma warning disable = 151

static ICM42688_t icm42688_Init_Structure;
SemaphoreHandle_t xDmaSemaphore;
static SemaphoreHandle_t xFIFOIntSemaphore;
static uint8_t xdata FIFO_Buffer[2048];
static IMU_Filter_t IMU_Filter_Data;

extern SemaphoreHandle_t xDebugReadySingal;
void IMU_Structure_Init(void);
uint8_t IMU_Read_Bytes_DMA_IT(uint8_t reg,uint8_t* dat,uint16_t len){ //这里只能传入Xdata域内的数组，不要传输错了
    
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
    taskENTER_CRITICAL();
    SPI2_WriteReadByte(reg|0x80);
    *dat = SPI2_WriteReadByte(0xFF);
    taskEXIT_CRITICAL();
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
 uint8_t intf;

void IMU_INT_ISR_Handle( BaseType_t* xHigherPriorityTaskWoken){
    uint8_t intf;

    intf = CheckP4nFlag(PIN_ALL);       //读取中断请求标志
    if(intf & PIN_6){
        xSemaphoreGiveFromISR(xFIFOIntSemaphore,xHigherPriorityTaskWoken);
       
    }
    ClearP4nFlag(PIN_ALL);              //清除中断请求标志
}

void IMU_DMA_Read_CallBack(BaseType_t* xSPI_DmaSemaphore){
   
    xSemaphoreGiveFromISR(xDmaSemaphore,xSPI_DmaSemaphore);
    //这里会释放信号量
    return;
}
float acc_t[3];
float gyro_t[3];
float temp_t;
void vIMUProcessTask(void *pvParameters){
    int16_t raw_acc_x;
    int16_t raw_acc_y ;
    int16_t raw_acc_z ;
    int16_t raw_gyro_x ;
    int16_t raw_gyro_y ;
    int16_t raw_gyro_z ;
    uint8_t raw_temp;
    uint16_t raw_timestamp;
    uint8_t xdata* ptr;
		uint16_t i,j;
    ptr=FIFO_Buffer;
		j=0;
	(void)pvParameters;
    xDmaSemaphore = xSemaphoreCreateBinary();
    xFIFOIntSemaphore = xSemaphoreCreateBinary();
    if(xDebugReadySingal != NULL){
        xSemaphoreTake(xDebugReadySingal,portMAX_DELAY);
        xSemaphoreGive(xDebugReadySingal);
    }
    PORT4_Init();
    vTaskDelay(pdMS_TO_TICKS(2));
    IMU_Structure_Init();
    i = ICM42688_Init(&icm42688_Init_Structure);
    if(i!=ICM_OK){
        Debug_Log("reg %d \r\n",i);
    }
    IMU_Filter_Data.sample_time = 1/500.0f;
    Debug_Register("ax",DBG_FLOAT| HIGH_FREQ_TYPE,&(IMU_Filter_Data.ax));
    Debug_Register("ay",DBG_FLOAT | HIGH_FREQ_TYPE,&(IMU_Filter_Data.ay));
    Debug_Register("az",DBG_FLOAT| HIGH_FREQ_TYPE,&(IMU_Filter_Data.az));
    Debug_Register("ax",DBG_FLOAT| HIGH_FREQ_TYPE,&(acc_t[0]));
    Debug_Register("ay",DBG_FLOAT | HIGH_FREQ_TYPE,&(acc_t[1]));
    Debug_Register("az",DBG_FLOAT| HIGH_FREQ_TYPE,&(acc_t[2]));
    IMU_Filter_Data.Kp = 1.0;
    IMU_Filter_Data.Ki = 0.01;
    Debug_Register("Kp",DBG_FLOAT,&(IMU_Filter_Data.Kp));
    Debug_Register("Ki",DBG_FLOAT,&(IMU_Filter_Data.Ki));
    for(;;){
        // vTaskDelay(pdMS_TO_TICKS(1000));
        // ICM42688_ReadData(&icm42688_Init_Structure,acc_t,gyro_t,&temp_t);
        // Debug_Log("%.2f %.2f %.2f ", acc_t[0],acc_t[1],acc_t[2]);
        if(xSemaphoreTake(xFIFOIntSemaphore,pdMS_TO_TICKS(100)) == pdTRUE){
            ICM42688_ReadFIFO(&icm42688_Init_Structure,FIFO_Buffer);
            P41=0;
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
                IMU_Filter_Data.sample_time = raw_timestamp * 0.000001f;
                IMU_Filter_Data.temp = (raw_temp / 2.07f) + 25.0f;
                IMU_Filter_Update(&IMU_Filter_Data);

                ptr+=16;
            }
            P41=1;
            //Debug_Log("p %.2f r %.2f y %.2f \r\n",IMU_Filter_Data.pitch,IMU_Filter_Data.roll,IMU_Filter_Data.yaw);
            ICM42688_ReadData(&icm42688_Init_Structure,acc_t,gyro_t,&temp_t);

        }else{
            continue;
        }
    }
}

static void task_delay_ms(uint32_t t){
    vTaskDelay(pdMS_TO_TICKS(t));
    return;
}
void IMU_Init(){
    xTaskCreate(vIMUProcessTask,"IMU",1024,NULL,tskIDLE_PRIORITY+4,NULL);
}
void IMU_Structure_Init(){
    SPI2_Init(IMU_DMA_Read_CallBack);
    icm42688_Init_Structure.read = IMU_Read_Bytes_DMA_IT;
    icm42688_Init_Structure.read_byte = IMU_Read_Byte;
    icm42688_Init_Structure.write = IMU_Write_Byte;
    icm42688_Init_Structure.delay = task_delay_ms;
    icm42688_Init_Structure.FIFO_THS = 32;
    icm42688_Init_Structure.delay(2);
   
    
}

