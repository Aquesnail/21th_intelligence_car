#ifndef FAN_CONTROL_H
#define FAN_CONTROL_H

#include "debug_arch.h"

/* ================================================================
 * 风扇缓启动 (Fan Soft-Start) 参数宏
 *   启动时从 FAN_SS_START_DUTY 线性插值到目标占空比
 *   总时长(s) = FAN_SS_BASE_TIME + (target - FAN_SS_DUTY_MIN) / FAN_SS_STEP_DUTY * FAN_SS_STEP_TIME
 *   启动完成后直接写入, 不再限斜率 (无感无刷已进闭环)
 * ================================================================ */
#define FAN_SS_DUTY_MIN      250     /* 占空比下限 (钳位)               */
#define FAN_SS_DUTY_MAX      500     /* 占空比上限 (钳位)               */
#define FAN_SS_START_DUTY    275     /* 缓启动起始占空比                */
#define FAN_SS_STEP_DUTY     50      /* 每步占空比增量 (用于计算时长)   */
#define FAN_SS_STEP_TIME_SEC 1.0f    /* 每步对应时间 (秒)               */
#define FAN_SS_BASE_TIME_SEC 2.0f    /* 保底启动时间 (秒)               */

/* 风扇占空比变量 (上位机可读写, 缓启动以此为目标值) */
extern uint16_t fan_D;

/* 风扇调试外设描述符 (注册到 Debug 系统) */
extern DebugPeriDesc_t g_fan_d;

/* ================================================================
 * 风扇控制 API
 *   Fan_SoftStart_Trigger — 触发缓启动 (钳位 + 从起始值开始)
 *   Fan_SoftStart_Step    — 每 tick 调用, 浮点线性插值步进
 *   Fan_TurnOff           — 关闭风扇 + 清除缓启动状态
 * ================================================================ */
void Fan_SoftStart_Trigger(uint16_t target);
void Fan_SoftStart_Step(void);
void Fan_TurnOff(void);
void Fan_SetDuty(uint16_t duty);

#endif /* FAN_CONTROL_H */
