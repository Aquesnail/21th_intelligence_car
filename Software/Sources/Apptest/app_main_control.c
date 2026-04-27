#include "FreeRTOS.h"
#include "task.h"
#include "debug_arch.h"
#include "storage_port.h"
#include "math.h"
#include "string.h"

#include "adc_user.h"
#include "spi1_user.h"

extern void IMU_Get_Angle(float* roll,float* pitch,float* yaw);

extern volatile uint8_t dma_spi1_done;
extern uint8_t xdata pu8SPI1DMATxBuffer[1024];

//以ADC的DMA回调为时间点开始运算

void MOTOR_INT1_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken);
void MOTOR_INT2_ISR_Handle(BaseType_t* xHigherPriorityTaskWoken);
void MOTOR_DMA_Read_CallBack(BaseType_t* xSPI_DmaSemaphore);

float test1 = 0.0f, test2 = 0.0f, test3 = 0.0f;

#define SIGNAL_SPI1_DMA_DONE (1u << 3)

static TaskHandle_t xMainControlTaskHandle = NULL;
static uint8_t stream_rx_buf[8];
static volatile uint32_t stream_raw[2] = {0};

enum motor_value_addr
{
    A_adc_vref = 0x10,
    A_vbus_divider_ratio = 0x11,
    A_shunt_resistance = 0x12,
    A_current_sense_gain = 0x13,
    A_pid_d_kp = 0x14,
    A_pid_d_ki = 0x15,
    A_pid_d_kd = 0x16,
    A_pid_q_kp = 0x17,
    A_pid_q_ki = 0x18,
    A_pid_q_kd = 0x19,
    A_adc_full_scale = 0x1A,

    B_adc_vref = 0x20,
    B_vbus_divider_ratio = 0x21,
    B_shunt_resistance = 0x22,
    B_current_sense_gain = 0x23,
    B_pid_d_kp = 0x24,
    B_pid_d_ki = 0x25,
    B_pid_d_kd = 0x26,
    B_pid_q_kp = 0x27,
    B_pid_q_ki = 0x28,
    B_pid_q_kd = 0x29,
    B_adc_full_scale = 0x2A
};

enum spi_decode_cmd
{
    SPI_CMD_EMPTY = 0,
    SPI_CMD_WRITE_ONE = 1,
    SPI_CMD_WRITE_TWO = 2,
    SPI_CMD_READ_ONE = 3,
    SPI_CMD_READ_TWO = 4,
    SPI_CMD_STREAMTRANSFER = 5,
    SPI_CMD_STOP = 6
};

static inline uint32_t spi_decode_pack(uint8_t cmd, uint8_t d1, uint8_t d2)
{
    return 0xAAu | ((uint32_t)cmd << 8) | ((uint32_t)d1 << 16) | ((uint32_t)d2 << 24);
}

static void spi_decode_send_u32(uint32_t val)
{
    MOTOR_SPI_TxRx((uint8_t)(val & 0xFFu));
    MOTOR_SPI_TxRx((uint8_t)((val >> 8) & 0xFFu));
    MOTOR_SPI_TxRx((uint8_t)((val >> 16) & 0xFFu));
    MOTOR_SPI_TxRx((uint8_t)((val >> 24) & 0xFFu));
}

void spi_decode_write_one(uint8_t addr, uint32_t value)
{
    spi_decode_send_u32(spi_decode_pack(SPI_CMD_WRITE_ONE, addr, 0u));
    spi_decode_send_u32(value);
}

void spi_decode_write_two(uint8_t addr1, uint32_t val1, uint8_t addr2, uint32_t val2)
{
    spi_decode_send_u32(spi_decode_pack(SPI_CMD_WRITE_TWO, addr1, addr2));
    spi_decode_send_u32(val1);
    spi_decode_send_u32(val2);
}

uint32_t spi_decode_read_one(uint8_t addr)
{
    uint32_t rx = 0u;
    spi_decode_send_u32(spi_decode_pack(SPI_CMD_READ_ONE, addr, 0u));
    rx  = MOTOR_SPI_TxRx(0u);
    rx |= ((uint32_t)MOTOR_SPI_TxRx(0u) << 8);
    rx |= ((uint32_t)MOTOR_SPI_TxRx(0u) << 16);
    rx |= ((uint32_t)MOTOR_SPI_TxRx(0u) << 24);
    return rx;
}

void spi_decode_read_two(uint8_t addr1, uint32_t* out1, uint8_t addr2, uint32_t* out2)
{
    spi_decode_send_u32(spi_decode_pack(SPI_CMD_READ_TWO, addr1, addr2));
    *out1  = MOTOR_SPI_TxRx(0u);
    *out1 |= ((uint32_t)MOTOR_SPI_TxRx(0u) << 8);
    *out1 |= ((uint32_t)MOTOR_SPI_TxRx(0u) << 16);
    *out1 |= ((uint32_t)MOTOR_SPI_TxRx(0u) << 24);

    *out2  = MOTOR_SPI_TxRx(0u);
    *out2 |= ((uint32_t)MOTOR_SPI_TxRx(0u) << 8);
    *out2 |= ((uint32_t)MOTOR_SPI_TxRx(0u) << 16);
    *out2 |= ((uint32_t)MOTOR_SPI_TxRx(0u) << 24);
}

void spi_decode_write_float(uint8_t addr, float value)
{
    uint32_t bits = 0u;
    memcpy(&bits, &value, sizeof(bits));
    spi_decode_write_one(addr, bits);
}

float spi_decode_read_float(uint8_t addr)
{
    uint32_t bits = spi_decode_read_one(addr);
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

void spi_decode_stream_start(uint8_t addr1, uint8_t addr2)
{
    spi_decode_send_u32(spi_decode_pack(SPI_CMD_STREAMTRANSFER, addr1, addr2));
}

void spi_decode_stream_stop(void)
{
    spi_decode_send_u32(spi_decode_pack(SPI_CMD_STOP, 0u, 0u));
}

void spi_decode_stream_dma_start(void)
{
    SPI1_DMA_WriteReadBytes_IT(pu8SPI1DMATxBuffer, stream_rx_buf, 8u);
}

uint16_t spi_decode_stream_u16(uint8_t index)
{
    if (index > 1u) return 0u;
    return (uint16_t)(stream_raw[index] & 0xFFFFu);
}

float spi_decode_stream_float(uint8_t index)
{
    if (index > 1u) return 0.0f;
    float val = 0.0f;
    uint32_t raw = stream_raw[index];
    memcpy(&val, &raw, sizeof(val));
    return val;
}

uint8_t MOTOR_SPI_TxRx(uint8_t dat) {
    uint8_t rx_dat;
    taskENTER_CRITICAL();
    rx_dat = SPI1_WriteReadByte(dat);
    taskEXIT_CRITICAL();
    return rx_dat;
}

/* SPI DMA批量读取 */
#define SIGNAL_SPI_DMA_DONE    (1 << 1)  // 位 1：DMA 传输完成 //任务通知比信号量快
uint8_t xdata SPI_DMA_Test[16] = {0};
void MOTOR_SPI_DmaRead(uint8_t* rx_buf, uint16_t len) {
    TickType_t xStart;
    xStart = xTaskGetTickCount();
    dma_spi1_done = 0;
    SPI1_DMA_WriteReadBytes_IT(SPI_DMA_Test, rx_buf, len);
    while (!dma_spi1_done) {
        if ((xTaskGetTickCount() - xStart) > pdMS_TO_TICKS(10)) {
            // 异常超时处理
            break;
        }
        taskYIELD();
    }
}


void vMainControlTask(void* params){
    float num;
    uint32_t ulNotifiedValue;
    (void)params;
    test2=0;
    for(;;){
        P32=~P32;

        if (xTaskNotifyWait(0, SIGNAL_SPI1_DMA_DONE, &ulNotifiedValue, pdMS_TO_TICKS(2)) == pdTRUE) {
            if (ulNotifiedValue & SIGNAL_SPI1_DMA_DONE) {
                stream_raw[0] = (uint32_t)stream_rx_buf[0]
                              | ((uint32_t)stream_rx_buf[1] << 8)
                              | ((uint32_t)stream_rx_buf[2] << 16)
                              | ((uint32_t)stream_rx_buf[3] << 24);
                stream_raw[1] = (uint32_t)stream_rx_buf[4]
                              | ((uint32_t)stream_rx_buf[5] << 8)
                              | ((uint32_t)stream_rx_buf[6] << 16)
                              | ((uint32_t)stream_rx_buf[7] << 24);
                spi_decode_stream_dma_start();
            }
        }

        num+=0.01;
        test1 = sin(num);
        test2 += 0.02;

        test3 = MOTOR_SPI_TxRx(0x74);
    }
}

void MainControl_Init(){
    SPI1_Init(MOTOR_DMA_Read_CallBack);
    xTaskCreate(vMainControlTask,"mainTask",1024,NULL,tskIDLE_PRIORITY+3,&xMainControlTaskHandle);
    Debug_Register("test1", DBG_FLOAT | HIGH_FREQ_TYPE, &(test1));
    Debug_Register("test2", DBG_FLOAT | HIGH_FREQ_TYPE, &(test2));
    Debug_Register("test3", DBG_FLOAT | HIGH_FREQ_TYPE, &(test3));
}

void MOTOR_DMA_Read_CallBack(BaseType_t* xHigherPriorityTaskWoken){
    if (xMainControlTaskHandle != NULL) {
        xTaskNotifyFromISR(xMainControlTaskHandle, SIGNAL_SPI1_DMA_DONE, eSetBits, xHigherPriorityTaskWoken);
    }
}