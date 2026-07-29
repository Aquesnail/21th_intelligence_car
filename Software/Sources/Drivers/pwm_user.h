#ifndef _PWM_USER_H
#define _PWM_USER_H

#include "config.h"

void PWMC_Init(void);
void PWMA_Init(void);
void PWME_Init(void);
void PWMA_Set(uint16_t t);
#endif /* _PWM_USER_H */