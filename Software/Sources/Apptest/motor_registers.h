#ifndef __MOTOR_REGISTERS_H__
#define __MOTOR_REGISTERS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 电机控制寄存器地址定义 (Motor Control Register Map)
 * 基地址: 
 *   - Group A: 0x10 ~ 0x1A
 *   - Group B: 0x20 ~ 0x2A
 * ============================================================================ */

/* =============================================================================
 * Group A 寄存器 (Motor A / Channel A)
 * 地址范围: 0x10 ~ 0x1A
 * ============================================================================ */
#define Motor_A_adc_vref            0x10u    /* ADC参考电压 */
#define Motor_A_vbus_divider_ratio  0x11u    /* 母线电压分压比 */
#define Motor_A_shunt_resistance    0x12u    /* 采样电阻值 */
#define Motor_A_current_sense_gain  0x13u    /* 电流检测增益 */
#define Motor_A_pid_d_kp            0x14u    /* D轴PID比例系数 */
#define Motor_A_pid_d_ki            0x15u    /* D轴PID积分系数 */
#define Motor_A_pid_d_kd            0x16u    /* D轴PID微分系数 */
#define Motor_A_pid_q_kp            0x17u    /* Q轴PID比例系数 */
#define Motor_A_pid_q_ki            0x18u    /* Q轴PID积分系数 */
#define Motor_A_pid_q_kd            0x19u    /* Q轴PID微分系数 */
#define Motor_A_adc_full_scale      0x1Au    /* ADC满量程值 */

/* =============================================================================
 * Group B 寄存器 (Motor B / Channel B)
 * 地址范围: 0x20 ~ 0x2A
 * ============================================================================ */
#define Motor_B_adc_vref            0x20u    /* ADC参考电压 */
#define Motor_B_vbus_divider_ratio  0x21u    /* 母线电压分压比 */
#define Motor_B_shunt_resistance    0x22u    /* 采样电阻值 */
#define Motor_B_current_sense_gain  0x23u    /* 电流检测增益 */
#define Motor_B_pid_d_kp            0x24u    /* D轴PID比例系数 */
#define Motor_B_pid_d_ki            0x25u    /* D轴PID积分系数 */
#define Motor_B_pid_d_kd            0x26u    /* D轴PID微分系数 */
#define Motor_B_pid_q_kp            0x27u    /* Q轴PID比例系数 */
#define Motor_B_pid_q_ki            0x28u    /* Q轴PID积分系数 */
#define Motor_B_pid_q_kd            0x29u    /* Q轴PID微分系数 */
#define Motor_B_adc_full_scale      0x2Au    /* ADC满量程值 */

/* =============================================================================
 * 寄存器组基地址 (用于偏移计算)
 * ============================================================================ */
#define MOTOR_A_BASE_ADDR           0x10u
#define MOTOR_B_BASE_ADDR           0x20u
#define MOTOR_REG_OFFSET            0x10u    /* A/B组间距 */

/* =============================================================================
 * 辅助宏 (寄存器访问)
 * ============================================================================ */
/* 通过通道号(0=A, 1=B)和偏移量计算寄存器地址 */
#define MOTOR_REG_ADDR(ch, offset)  (((ch) == 0) ? ((0x10u) + (offset)) : ((0x20u) + (offset)))

/* 寄存器偏移量定义 (相对于基地址) */
#define MOTOR_REG_OFFSET_adc_vref           0x00u
#define MOTOR_REG_OFFSET_vbus_divider_ratio 0x01u
#define MOTOR_REG_OFFSET_shunt_resistance   0x02u
#define MOTOR_REG_OFFSET_current_sense_gain 0x03u
#define MOTOR_REG_OFFSET_pid_d_kp           0x04u
#define MOTOR_REG_OFFSET_pid_d_ki           0x05u
#define MOTOR_REG_OFFSET_pid_d_kd           0x06u
#define MOTOR_REG_OFFSET_pid_q_kp           0x07u
#define MOTOR_REG_OFFSET_pid_q_ki           0x08u
#define MOTOR_REG_OFFSET_pid_q_kd           0x09u
#define MOTOR_REG_OFFSET_adc_full_scale     0x0Au

#ifdef __cplusplus
}
#endif

#endif /* __MOTOR_REGISTERS_H__ */