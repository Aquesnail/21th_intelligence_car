#include "FreeRTOS.h"
#include "task.h"

#include "config.h"
#include "adc_user.h"
#include "debug_arch.h"
#include "motor.h"
#include "spi3_user.h"
/* ADC DMA 缓冲区 (xdata 区域) */
// 每个通道: 采样次数 * 2 + 4个硬件计算字节
static uint8_t xdata adc_dma_buf[ADC2_DMASIZE];

/* ADC 各通道采样值 (ADCB0~3) */
static uint16_t adc_values[ADC2_CH_NUM];

#include <stdio.h>



void SPI3_Callback(void *pvParameters){
}

static float s_speed_measure[2] = {0.0f, 0.0f};
static float s_speed_target[2] = {0.0f, 0.0f};
static float s_current_measure[2] = {0.0f, 0.0f};
static float s_current_target[2] = {0.0f, 0.0f};
void Get_Motor_Data(){
    s_current_measure[0]=0.0f;
    s_current_measure[1]=0.0f;
    s_speed_measure[0]=0.0f;
    s_speed_measure[1]=0.0f;
}

#define MOTOR_L 0
#define MOTOR_R 1

static const float SPEED_KP = 0.5f;
static const float SPEED_KI = 2.0f;
static const float SPEED_DT = 0.002f;
static const float I_LIMIT = 5.0f;

static float s_speed_integrals[2] = {0.0f, 0.0f};

float Speed_Outer_Loop(uint8_t id, float target_v, float measure_v) {
    float error;
    float p_term;
    float i_step;
    float target_current;
    float raw_current;
    error = target_v - measure_v;
    p_term = SPEED_KP * error;
    i_step = SPEED_KI * error * SPEED_DT;
    raw_current = p_term + (s_speed_integrals[id] + i_step);
    if (raw_current > I_LIMIT) {
        target_current = I_LIMIT;
        if (error < 0) {
            s_speed_integrals[id] += i_step;
        }
    } else if (raw_current < -I_LIMIT) {
        target_current = -I_LIMIT;
        if (error > 0) {
            s_speed_integrals[id] += i_step;
        }
    } else {
        target_current = raw_current;
        s_speed_integrals[id] += i_step;
    }
    return target_current;
}

void Motor_Test_SendCmd(void)
{
    uint32_t frame;
    uint32_t value;
    uint8_t i;
    frame = motor_pack_head(1, 0x01, 0x01);
    value = 0x12345678;
    MOTOR_CS_RESET();
    motor_write_reg(0x01,0xFFFFFFFF);
		
//for (i = 0; i < 4; i++) {
    //    SPI3_WriteByte((uint8_t)(0xFFu));
   //     frame >>= 8;
  //  }
  //  for (i = 0; i < 4; i++) {
  //      SPI3_WriteByte((uint8_t)(0xAAu));
  //      value >>= 8;
   // }
    MOTOR_CS_SET();
}
TaskHandle_t xMainControlTaskHandle = NULL;
#define SIGNAL_MOTOR_DATA_READY (1 << 2) /* 位 2：电机数据准备好 */
motor_data_t read_motor_test;
uint32_t write_motor_test=0x000000AA;
uint8_t red_test=0x00;

/* 呼吸灯状态 (2ms 周期, 三角波) */
#define BREATH_CYCLE_STEPS  1000u   /* 2ms * 1000 = 2s 完整周期 */
static uint16_t s_breath_phase = 0;

/* 呼吸灯数值计算 (三角波: 0 -> max -> 0) */
static uint32_t Breath_Calc(void)
{
    uint32_t val;
    uint16_t half;
    half = BREATH_CYCLE_STEPS / 2u;
    if (s_breath_phase < half) {
        val = (uint32_t)s_breath_phase * 0xFFFFu / half;
    } else {
        val = (uint32_t)(BREATH_CYCLE_STEPS - s_breath_phase) * 0xFFFFu / half;
    }
    s_breath_phase++;
    if (s_breath_phase >= BREATH_CYCLE_STEPS) {
        s_breath_phase = 0;
    }
    return val;
}
void Main_Control_Task(void *pvParameters) {
    uint32_t ulNotifiedValue;
    uint8_t i;
    (void)pvParameters;
    xMainControlTaskHandle = xTaskGetCurrentTaskHandle();
    SPI3_Init(SPI3_Callback);
    ADC2_DMA_Init(adc_dma_buf);
    ADC2_DMA_Start();
    vTaskDelay(pdMS_TO_TICKS(10)); // 等待 ADC 稳定
    MOTOR_CS_RESET();
    motor_stream_start(0x01, 0x02);
    MOTOR_CS_SET();
    P21=1;
    for (;;) {
       // vTaskDelay(pdMS_TO_TICKS(25));
        if(xTaskNotifyWait(0, SIGNAL_MOTOR_DATA_READY, &ulNotifiedValue, portMAX_DELAY) == pdTRUE) {
           if(ulNotifiedValue & SIGNAL_MOTOR_DATA_READY) {
                //vTaskDelay(pdMS_TO_TICKS(10)); // 等待数据稳定
                
                write_motor_test = Breath_Calc();
                //vTaskDelay(pdMS_TO_TICKS(10));
               // write_motor_test = 2;
                if (ADC2_DMA_IsDone()) {
            ADC2_DMA_ClearFlag();

            for (i = 0; i < ADC2_CH_NUM; i++) {
                adc_values[i] = ADC2_GetAvgValue(i);
            }

            ADC2_DMA_Start();
        }
							MOTOR_CS_RESET();
							read_motor_test = motor_spi_recv_u32();
							MOTOR_CS_SET();
						  //vTaskDelay(pdMS_TO_TICKS(1));
						  NOP33();
						  MOTOR_CS_RESET();
                          
                        motor_write_reg(0x01,2);
             
							MOTOR_CS_SET();

               
               // vTaskDelay(pdMS_TO_TICKS(10));
                // MOTOR_CS_RESET();
                // motor_stream_start(0x01, 0x02);
                // MOTOR_CS_SET();
            }
        }
    }
}



void Motor_Data_Ready_INT2_Handle(BaseType_t* xHigherPriorityTaskWoken){
    xTaskNotifyFromISR(xMainControlTaskHandle, SIGNAL_MOTOR_DATA_READY, eSetBits, xHigherPriorityTaskWoken);
}



void Main_Control_Task_Init(void) {
    /* 注册 ADCB0~3 为高频调试变量 */
    Debug_Register("adc0", DBG_UINT16 | HIGH_FREQ_TYPE, &adc_values[0]);
    Debug_Register("adc1", DBG_UINT16 | HIGH_FREQ_TYPE, &adc_values[1]);
    Debug_Register("adc2", DBG_UINT16 | HIGH_FREQ_TYPE, &adc_values[2]);
    Debug_Register("adc3", DBG_UINT16 | HIGH_FREQ_TYPE, &adc_values[3]);

    xTaskCreate(Main_Control_Task, "motor_test", 1024, NULL, tskIDLE_PRIORITY + 1, NULL);
  //  xTaskCreate(Main_Control_Task, "main_control", 1024, NULL, tskIDLE_PRIORITY + 2, &xMainControlTaskHandle);
}
