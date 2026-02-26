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
#include "storage.h"

#pragma warning disable = 138
#pragma warning disable = 151

#define FIFO_Packets 32

static ICM42688_t IMU_Init_Structure;
SemaphoreHandle_t xDmaSemaphore;
static SemaphoreHandle_t xFIFOIntSemaphore;
static uint8_t xdata FIFO_RxBuffer[FIFO_Packets*16 +1];
static uint8_t xdata FIFO_TxBuffer[FIFO_Packets*16 +1];
IMU_Filter_t IMU_Filter_Data;
static ICM42688_Pack16_t IMU_Dat;


extern SemaphoreHandle_t xDebugReadySingal;
void IMU_Structure_Init(void);

/* --- 极简传入函数 1：SPI 单字节纯净收发 --- */
uint8_t IMU_SPI_TxRx_Byte(uint8_t dat){
    uint8_t rx_dat;
    // icm42688.c 已经帮你控制好了 CS 拉低拉高，这里只需干活
    taskENTER_CRITICAL(); // 保留你的原逻辑，防止单点传输被打断
    rx_dat = SPI2_WriteReadByte(dat);
    taskEXIT_CRITICAL();
    return rx_dat;
}

/* --- 极简传入函数 2：SPI DMA 批量读取 --- */
void IMU_SPI_Dma_Read(uint8_t* rx_buf, uint16_t len){ 
    // icm42688.c 已经发过了 寄存器地址，并拉低了 CS
    // 此时直接开启 DMA 收取数据
    ICM42688_CS_RESET();
    SPI2_DMA_WriteReadBytes_IT(FIFO_TxBuffer, rx_buf, len);
    
    if(xSemaphoreTake(xDmaSemaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
        // 获取成功 (DMA 完成)
        ICM42688_CS_SET();
    } else {
        // 超时处理
    }
    // 函数退出后，icm42688.c 会自动拉高 CS
}

// -------------------------------------------------------------

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
    return;
}

float acc_t[3];
float gyro_t[3];
float temp_t;

void vIMUProcessTask(void *pvParameters){
    uint8_t xdata* ptr;
    uint16_t i,j;
    
    ptr = FIFO_RxBuffer;
    j = 0;
    (void)pvParameters;
    
    xDmaSemaphore = xSemaphoreCreateBinary();
    xFIFOIntSemaphore = xSemaphoreCreateBinary();
    
    if(xDebugReadySingal != NULL){
        xSemaphoreTake(xDebugReadySingal,portMAX_DELAY);
        xSemaphoreGive(xDebugReadySingal);
    }
    memset(FIFO_TxBuffer,0xff,sizeof(FIFO_TxBuffer));
    PORT4_Init();
    vTaskDelay(pdMS_TO_TICKS(2));
    
    IMU_Structure_Init();
    i = ICM42688_Init(&IMU_Init_Structure);
    if(i!=ICM_OK){
        Debug_Log("reg %d \r\n",i);
    }
    
   // IMU_Filter_Data.sample_time = 1/500.0f;
    Debug_Register("ax",DBG_FLOAT| HIGH_FREQ_TYPE,&(IMU_Filter_Data.gz));
    Debug_Register("ay",DBG_FLOAT | HIGH_FREQ_TYPE,&(IMU_Filter_Data.gy));
    Debug_Register("az",DBG_FLOAT| HIGH_FREQ_TYPE,&(IMU_Filter_Data.gz));
    Debug_Register("pitch",DBG_FLOAT| HIGH_FREQ_TYPE,&(IMU_Filter_Data.pitch));
    Debug_Register("roll",DBG_FLOAT| HIGH_FREQ_TYPE,&(IMU_Filter_Data.roll));
    Debug_Register("yaw",DBG_FLOAT| HIGH_FREQ_TYPE,&(IMU_Filter_Data.yaw));
     Debug_Register("Kp",DBG_FLOAT,&(IMU_Filter_Data.Kp));
    Debug_Register("Ki",DBG_FLOAT,&(IMU_Filter_Data.Ki));
    
    IMU_Filter_Data.Kp = 0.8;
    IMU_Filter_Data.Ki = 0.002;
    IMU_Filter_Data.bias_gx=0.0f;
    IMU_Filter_Data.bias_gy=0.0f;
    IMU_Filter_Data.bias_gz=0.0f;
    IMU_Filter_Data.is_calibrated = 0;
    
    if(Storage_Data_Writed(&(IMU_Filter_Data.bias_gx))==0){
        IMU_Filter_Data.is_calibrated = 1;
        Storage_Load(&(IMU_Filter_Data.bias_gx));
        Storage_Load(&(IMU_Filter_Data.bias_gy));
        Storage_Load(&(IMU_Filter_Data.bias_gz));
    }else{
        Debug_Log("Bias Not In Flash");
    }
    if(Storage_Data_Writed(&(IMU_Filter_Data.Kp))==0){
        Storage_Load(&(IMU_Filter_Data.Kp));
    }else{
        Debug_Log("Kp Not In Flash");
    }
    if(Storage_Data_Writed(&(IMU_Filter_Data.Kp))==0){
        Storage_Load(&(IMU_Filter_Data.Ki));
    }else{
        Debug_Log("Ki Not In Falsh");
    }
    
    for(;;){
        if(xSemaphoreTake(xFIFOIntSemaphore,pdMS_TO_TICKS(100)) == pdTRUE){
           // Debug_Log("sample %f \r\n",100*IMU_Filter_Data.sample_time);
            j = ICM42688_ReadFIFO(&IMU_Init_Structure,FIFO_RxBuffer);
            if(j!=ICM_ERROR && j!= 0){
                Debug_Log("Pcakets: %u \r\n",j);
            }
            P41=0;
            ptr = FIFO_RxBuffer;
            if(IMU_Filter_Data.is_calibrated){

            }
            for(i=0;i<IMU_Init_Structure.FIFO_THS;i++){
                
                ICM42688_FIFO_Data_Handle(&IMU_Init_Structure,&IMU_Dat,ptr);
                IMU_Filter_Data.ax = IMU_Dat.ax;
                IMU_Filter_Data.ay = IMU_Dat.ay;
                IMU_Filter_Data.az = IMU_Dat.az;

                IMU_Filter_Data.gx = IMU_Dat.gx;
                IMU_Filter_Data.gy = IMU_Dat.gy;
                IMU_Filter_Data.gz = IMU_Dat.gz;

                IMU_Filter_Data.time_stamp =  IMU_Dat.timestamp;
                IMU_Filter_Data.sample_time =  IMU_Dat.timestamp / 1000000.0f;//1 / 500.0f;
                IMU_Filter_Data.temp = IMU_Dat.temp;
                IMU_Filter_Update(&IMU_Filter_Data);

                ptr+=16;
            }
            P41=1;
            ICM42688_ReadData(&IMU_Init_Structure,acc_t,gyro_t,&temp_t);
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
    
    /* 仅仅只传两个最极简的 SPI 函数给它 */
    IMU_Init_Structure.spi_tx_rx = IMU_SPI_TxRx_Byte;
    IMU_Init_Structure.spi_dma_read = IMU_SPI_Dma_Read;
    IMU_Init_Structure.print = Debug_Log;
    IMU_Init_Structure.delay = task_delay_ms;
    IMU_Init_Structure.FIFO_THS = FIFO_Packets;
    IMU_Init_Structure.delay(2);
}