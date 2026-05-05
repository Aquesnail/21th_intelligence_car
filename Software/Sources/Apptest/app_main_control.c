#include "FreeRTOS.h"
#include "task.h"

#include "config.h"
#include "spi1_user.h"
#include "motor.h"
#include <stdio.h>
#include "debug_arch.h"



/* P2.2 作为电机 SPI 片选 (CS) */
#define MOTOR_CS_SET()   (P22 = 1)
#define MOTOR_CS_RESET() (P22 = 0)

void SPI1_Callback(void *pvParameters){

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

/* --- 电机索引 --- */
#define MOTOR_L 0
#define MOTOR_R 1

/* --- 速度环参数配置 (写死) --- */
static const float SPEED_KP = 0.5f;       // 速度环 P 参数
static const float SPEED_KI = 2.0f;       // 速度环 I 参数
static const float SPEED_DT = 0.002f;      // 采样周期 10ms (通常外环比内环慢)

/* --- 限幅配置 (抗饱和关键) --- */
// 这里限制的是输出电流。例如：电机允许的最大电流是 5.0A，则设为 5.0
static const float I_LIMIT = 5.0f;        

/* --- 状态变量 --- */
static float s_speed_integrals[2] = {0.0f, 0.0f};

/**
 * 速度控制器 (外环)
 * @param id      电机 ID
 * @param target_v 目标速度
 * @param measure_v 反馈速度
 * @return 目标电流 (作为电流内环的输入)
 */
float Speed_Outer_Loop(uint8_t id, float target_v, float measure_v) {
    /* 局部变量定义在开头 */
    float error;
    float p_term;
    float i_step;
    float target_current;
    float raw_current;

    // 1. 计算速度误差
    error = target_v - measure_v;

    // 2. 比例项
    p_term = SPEED_KP * error;

    // 3. 积分增量
    i_step = SPEED_KI * error * SPEED_DT;

    // 4. 计算原始目标电流 (P + 暂存积分 + 本次增量)
    raw_current = p_term + (s_speed_integrals[id] + i_step);

    // 5. 抗饱和与电流限幅逻辑
    // 如果计算出的电流超过了电机/驱动器能承受的最大电流
    if (raw_current > I_LIMIT) {
        target_current = I_LIMIT;
        // 只有当误差为负时（想降速），才允许更新积分
        if (error < 0) {
            s_speed_integrals[id] += i_step;
        }
    } 
    else if (raw_current < -I_LIMIT) {
        target_current = -I_LIMIT;
        // 只有当误差为正时（想增速），才允许更新积分
        if (error > 0) {
            s_speed_integrals[id] += i_step;
        }
    } 
    else {
        // 正常范围内，更新积分
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

    /* 构造 write_one 命令帧: cmd=1, addr=0x10 */
    frame = motor_pack_head(1, 0x10, 0x00);
    value = 0x12345678;

    MOTOR_CS_RESET();

    /* 发送 32bit 帧头 (低字节在前) */
    for (i = 0; i < 4; i++) {
        SPI1_WriteByte((uint8_t)(frame & 0xFFu));
        frame >>= 8;
    }

    /* 发送 32bit 数据 (低字节在前) */
    for (i = 0; i < 4; i++) {
        SPI1_WriteByte((uint8_t)(value & 0xFFu));
        value >>= 8;
    }

    MOTOR_CS_SET();
}

void Main_Control_Task(void *pvParameters){
    uint8_t counter = 0;

    /* 初始化 P2.2 为推挽输出 */
    // P2M1 &= ~0x04;
    // P2M0 |= 0x04;
    MOTOR_CS_SET();

    SPI1_Init(SPI1_Callback);

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

void Main_Control_Task_Init(){
    Debug_Register("Speed_Kp", DBG_FLOAT | STATIC_TYPE, &(SPEED_KP));
    Debug_Register("Speed_Ki", DBG_FLOAT | STATIC_TYPE, &(SPEED_KI));
    xTaskCreate(Main_Control_Task, "main_control", 1024, NULL, tskIDLE_PRIORITY + 4, NULL);
}