#include "FreeRTOS.h"
#include "task.h"

#include "config.h"
#include "adc_user.h"
#include "debug_arch.h"

/* ADC DMA 缓冲区 (xdata 区域) */
uint8_t xdata adc_dma_buf[ADC2_CH_NUM * ADC2_BUF_SIZE * 2];

/* ADC 各通道采样值 (ADCB0~3) */
static uint16_t adc_values[ADC2_CH_NUM];

#include <stdio.h>

#define MOTOR_CS_SET()   (P22 = 1)
#define MOTOR_CS_RESET() (P22 = 0)

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
    frame = motor_pack_head(1, 0x10, 0x00);
    value = 0x12345678;
    MOTOR_CS_RESET();
    for (i = 0; i < 4; i++) {
        SPI3_WriteByte((uint8_t)(frame & 0xFFu));
        frame >>= 8;
    }
    for (i = 0; i < 4; i++) {
        SPI3_WriteByte((uint8_t)(value & 0xFFu));
        value >>= 8;
    }
    MOTOR_CS_SET();
}

void Motor_Test_Task(void *pvParameters){
    uint8_t counter = 0;
    MOTOR_CS_SET();
    SPI3_Init(SPI3_Callback);
    for(;;){
        vTaskDelay(pdMS_TO_TICKS(2));
        Get_Motor_Data();
        counter++;
        if (counter % 50 == 0) {
            Motor_Test_SendCmd();
            counter = 0;
        }
    }
}


/* ===================================================================
 * ADC 测试任务
 * 采集 ADCB0~3 (P0.0~P0.3) 四通道 ADC 值，注册为高频调试变量
 * =================================================================== */
void ADC_Test_Task(void *pvParameters) {
    uint8_t i;

    //ADC2_Init();
    ADC2_DMA_Init(adc_dma_buf);
    ADC2_DMA_Start();
    vTaskDelay(pdMS_TO_TICKS(10)); // 等待 ADC 稳定
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10));

        if (ADC2_DMA_IsDone()) {
            ADC2_DMA_ClearFlag();

            for (i = 0; i < ADC2_CH_NUM; i++) {
                adc_values[i] = ADC2_GetAvgValue(i);
            }

            ADC2_DMA_Start();
        }
    }
}

void Main_Control_Task_Init(void) {
    /* 注册 ADCB0~3 为高频调试变量 */
    Debug_Register("adc0", DBG_UINT16 | HIGH_FREQ_TYPE, &adc_values[0]);
    Debug_Register("adc1", DBG_UINT16 | HIGH_FREQ_TYPE, &adc_values[1]);
    Debug_Register("adc2", DBG_UINT16 | HIGH_FREQ_TYPE, &adc_values[2]);
    Debug_Register("adc3", DBG_UINT16 | HIGH_FREQ_TYPE, &adc_values[3]);

    xTaskCreate(Motor_Test_Task, "motor_test", 1024, NULL, tskIDLE_PRIORITY + 1, NULL);
}
