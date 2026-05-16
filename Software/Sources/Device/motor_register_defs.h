#ifndef MOTOR_REGISTER_DEFS_H
#define MOTOR_REGISTER_DEFS_H

#include "config.h"

/* 电机寄存器地址宏定义 */

// 通用控制与状态
#define REG_CMD_COMMAND                 0x01u
#define REG_MECH_STATE                  0x02u
#define REG_LIGHT_COLOR                 0x03u

// 电机 A 控制参数
#define REG_A_SPEED_TARGET              0x31u
#define REG_A_PID_SPEED_KP              0x1Du
#define REG_A_PID_SPEED_KI              0x1Eu
#define REG_A_PID_SPEED_KD              0x1Fu
#define REG_A_PID_SPEED_INTEGRAL_LIMIT  0x30u

// 电机 B 控制参数
#define REG_B_SPEED_TARGET              0x41u
#define REG_B_PID_SPEED_KP              0x2Du
#define REG_B_PID_SPEED_KI              0x2Eu
#define REG_B_PID_SPEED_KD              0x2Fu
#define REG_B_PID_SPEED_INTEGRAL_LIMIT  0x40u

#endif