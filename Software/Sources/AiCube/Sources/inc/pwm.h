//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: pwm.h
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#ifndef __PWM_H__
#define __PWM_H__


//<<AICUBE_USER_DEFINE_BEGIN>>
#include "config.h"
// 在此添加用户宏定义  
//<<AICUBE_USER_DEFINE_END>>



#define PWMA_CH1                0x11
#define PWMA_CH2                0x12
#define PWMA_CH3                0x13
#define PWMA_CH4                0x14
#define PWMA_CH5                0x15
#define PWMA_CH6                0x16
#define PWMB_CH5                0x25
#define PWMB_CH6                0x26
#define PWMB_CH7                0x27
#define PWMB_CH8                0x28
#define PWMC_CH1                0x31
#define PWMC_CH2                0x32
#define PWMC_CH3                0x33
#define PWMC_CH4                0x34
#define PWMC_CH5                0x35
#define PWMC_CH6                0x36
#define PWMD_CH5                0x45
#define PWMD_CH6                0x46
#define PWMD_CH7                0x47
#define PWMD_CH8                0x48
#define PWME_CH1                0x51
#define PWME_CH2                0x52
#define PWME_CH3                0x53
#define PWME_CH4                0x54
#define PWME_CH5                0x55
#define PWME_CH6                0x56
#define PWMF_CH5                0x65
#define PWMF_CH6                0x66
#define PWMF_CH7                0x67
#define PWMF_CH8                0x68

#define IS_PWMA(ch)             (((ch) >= PWMA_CH1) && ((ch) <= PWMA_CH6))
#define IS_PWMB(ch)             (((ch) >= PWMB_CH5) && ((ch) <= PWMB_CH8))
#define IS_PWMC(ch)             (((ch) >= PWMC_CH1) && ((ch) <= PWMC_CH6))
#define IS_PWMD(ch)             (((ch) >= PWMD_CH5) && ((ch) <= PWMD_CH8))
#define IS_PWME(ch)             (((ch) >= PWME_CH1) && ((ch) <= PWME_CH6))
#define IS_PWMF(ch)             (((ch) >= PWMF_CH5) && ((ch) <= PWMF_CH8))

#define PWM_MODE1               0x06
#define PWM_MODE2               0x07

void PWMA_Init(void);
void PWME_Init(void);

void PWM_UpdateDuty(uint8_t PWMx, uint16_t nCompare);
void HSPWM_UpdateDuty(uint8_t PWMx, uint16_t nCompare);
uint16_t PWM_ReadCapture(uint8_t PWMx);
uint16_t HSPWM_ReadCapture(uint8_t PWMx);

uint8_t HSPWMA_ReadReg(uint8_t addr);
uint8_t HSPWMB_ReadReg(uint8_t addr);
uint8_t HSPWMC_ReadReg(uint8_t addr);
uint8_t HSPWMD_ReadReg(uint8_t addr);
uint8_t HSPWME_ReadReg(uint8_t addr);
uint8_t HSPWMF_ReadReg(uint8_t addr);
void HSPWMA_WriteReg(uint8_t addr, uint8_t dat);
void HSPWMB_WriteReg(uint8_t addr, uint8_t dat);
void HSPWMC_WriteReg(uint8_t addr, uint8_t dat);
void HSPWMD_WriteReg(uint8_t addr, uint8_t dat);
void HSPWME_WriteReg(uint8_t addr, uint8_t dat);
void HSPWMF_WriteReg(uint8_t addr, uint8_t dat);

extern uint8_t xdata pu8PWMEDMATxBuffer[PWME_DMATXSIZE];


//<<AICUBE_USER_EXTERNAL_DECLARE_BEGIN>>
// 在此添加用户外部函数和外部变量声明  
//<<AICUBE_USER_EXTERNAL_DECLARE_END>>


#endif
