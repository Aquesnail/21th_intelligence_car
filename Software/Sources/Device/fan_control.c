#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include "pwm_user.h"
#include "fan_control.h"

/* ================================================================
 * 全局变量
 * ================================================================ */

/* 风扇占空比默认值 (上位机在线可调, 当元素 fan_d_ptr=NULL 时回退到此值) */
uint16_t fan_D = 375;

/* ================================================================
 * 模块内部状态
 * ================================================================ */

/* 缓启动运行时状态 */
static uint8_t    g_fan_ss_active      = 0;  /* 缓启动进行中                  */
static uint16_t   g_fan_ss_target      = 0;  /* 目标占空比 (已钳位)           */
static TickType_t g_fan_ss_start_tick  = 0;  /* 缓启动起始时刻                */
static TickType_t g_fan_ss_ramp_ticks  = 0;  /* 总 ramp 时长 (tick)           */
static uint16_t   g_fan_output         = 0;  /* 当前 PWM 输出值 (0=停转)      */

/* 内部 PWM 写入封装: 同步追踪当前输出, 用于区分停转/运转 */
static void Fan_PWM_Write(uint16_t duty)
{
    g_fan_output = duty;
    PWMA_Set(duty);
}

void Fan_SetDuty(uint16_t duty);  /* 前向声明, 供回调使用 */

/* ================================================================
 * 调试回调 (上位机读写 fan_D)
 * ================================================================ */

static void Fan_D_Read_Cb(uint32_t peri_addr, void *pData, uint8_t len)
{
    uint16_t val;
    (void)len;
    (void)peri_addr;
    val = fan_D;
    memcpy(pData, &val, sizeof(uint16_t));
}

static void Fan_D_Write_Cb(uint32_t peri_addr, void *pData, uint8_t len)
{
    uint16_t new_val;
    (void)len;
    (void)peri_addr;
    memcpy(&new_val, pData, sizeof(uint16_t));
    fan_D = new_val;
    Fan_SetDuty(new_val);
}

/* 风扇调试外设描述符 */
DebugPeriDesc_t g_fan_d = {0, Fan_D_Read_Cb, Fan_D_Write_Cb};

/* ================================================================
 * 风扇缓启动: 从 FAN_SS_START_DUTY 线性插值到目标, 仅启动阶段使用
 *   Fan_SoftStart_Trigger — 触发缓启动
 *   Fan_SoftStart_Step    — 每 tick 步进 (浮点插值)
 *   Fan_TurnOff           — 关闭 + 清除状态
 * ================================================================ */

void Fan_SoftStart_Trigger(uint16_t target)
{
    float ramp_sec;
    float ramp_ms;

    if (target == 0) {
        g_fan_ss_active = 0;
        Fan_PWM_Write(0);
        return;
    }
    /* 钳位 */
    if (target < FAN_SS_DUTY_MIN) target = FAN_SS_DUTY_MIN;
    if (target > FAN_SS_DUTY_MAX) target = FAN_SS_DUTY_MAX;

    g_fan_ss_target     = target;
    g_fan_ss_start_tick = xTaskGetTickCount();

    /* ramp时长 = 保底时间 + (target - 下限) / 步进值 * 步进时间 */
    ramp_sec = FAN_SS_BASE_TIME_SEC
             + (float)(target - FAN_SS_DUTY_MIN)
             / (float)FAN_SS_STEP_DUTY
             * FAN_SS_STEP_TIME_SEC;
    ramp_ms = ramp_sec * 1000.0f;
    g_fan_ss_ramp_ticks = pdMS_TO_TICKS((TickType_t)ramp_ms);
    g_fan_ss_active     = 1;

    Fan_PWM_Write(FAN_SS_START_DUTY);
}

void Fan_SoftStart_Step(void)
{
    TickType_t now;
    TickType_t elapsed;
    TickType_t hold_ticks;
    float progress;
    float duty_f;
    uint16_t duty;

    if (!g_fan_ss_active) return;

    now     = xTaskGetTickCount();
    elapsed = now - g_fan_ss_start_tick;
    hold_ticks = pdMS_TO_TICKS((TickType_t)(FAN_SS_HOLD_TIME_SEC * 1000.0f));

    if (elapsed < hold_ticks) {
        /* 保持起始占空比, 等待风扇稳定后再开始爬升 */
        Fan_PWM_Write(FAN_SS_START_DUTY);
    } else if (elapsed >= hold_ticks + g_fan_ss_ramp_ticks) {
        /* 缓启动完成, 之后直接写入 */
        Fan_PWM_Write(g_fan_ss_target);
        g_fan_ss_active = 0;
    } else {
        /* 浮点线性插值: 起始 → 目标 (扣除保持时间后按进度计算) */
        progress = (float)(elapsed - hold_ticks) / (float)g_fan_ss_ramp_ticks;
        duty_f   = (float)FAN_SS_START_DUTY
                 + progress * (float)((int)g_fan_ss_target - FAN_SS_START_DUTY);
        duty = (uint16_t)duty_f;
        Fan_PWM_Write(duty);
    }
}

void Fan_TurnOff(void)
{
    g_fan_ss_active = 0;
    Fan_PWM_Write(0);
}

void Fan_SetDuty(uint16_t duty)
{
    if (duty == 0) {
        Fan_TurnOff();
        return;
    }
    /* 钳位 */
    if (duty < FAN_SS_DUTY_MIN) duty = FAN_SS_DUTY_MIN;
    if (duty > FAN_SS_DUTY_MAX) duty = FAN_SS_DUTY_MAX;

    if (g_fan_output == 0) {
        /* 当前停转: 走缓启动 */
        Fan_SoftStart_Trigger(duty);
    } else if (g_fan_ss_active) {
        /* 缓启动进行中: 仅更新目标值, 由 Step 负责步进 */
        g_fan_ss_target = duty;
    } else {
        /* 已运转且缓启动已完成: 直接写入 */
        Fan_PWM_Write(duty);
    }
}

uint8_t Fan_IsSoftStartComplete(void)
{
    return (g_fan_ss_active == 0) ? 1 : 0;
}
