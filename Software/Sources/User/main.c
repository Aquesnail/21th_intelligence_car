

#include "config.h"
#include "application.h"

#include "gpio_user.h"
//#include "port.h"

#include "clock.h"
#include "pwm.h"
#include "timer.h"
#include "iap_user.h"
#include "ws2812.h"
#include "mathlib.h"
#include "gpio_user.h"
#include "adc_user.h"

void sys_init(void);
void main(){
		EnableAccessXFR();                  //??????XFR
    AccessCodeFastest();                //????????????
    AccessIXramFastest();               //??????????XDATA
    IAP_SetTimeBase();                  //??IAP????,??1us??
		P0M0=0x00;        P0M1=0x00;
		P1M0=0x00;        P1M1=0x00;
		P2M0=0x00;        P2M1=0x00;
		P3M0=0x00;        P3M1=0x00;
		P4M0=0x00;        P4M1=0x00;
		P5M0=0x00;        P5M1=0x00;
		P6M0=0x00;        P6M1=0x00;
		P7M0=0x00;        P7M1=0x00;
		sys_init();
		app_init();
		while(1){}
}

void sys_init(void){
	GPIO_User_Control_On();

	GPIO_Init();
	CLK_Init();    

	//CLK_Init();  
	WTST = 0x03;
	delay_ms(100);
	
	//GPIO_Init();

	PWMA_Init();
	PWME_Init();

	MATHLIB_Init();
	ADC2_Init();
	//ADC2_DMA_Init();
	IAP_Init();
	WS2812_Init();
}