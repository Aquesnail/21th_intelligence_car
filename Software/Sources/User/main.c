

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
#include "pwm_user.h"


void sys_init(void);
void main(){
		EnableAccessXFR();                  //??????XFR
    	AccessCodeFastest();                //????????????
    	AccessIXramFastest();               //??????????XDATA
    	IAP_SetTimeBase();                  //??IAP????,??1us??
		EAXFR = 1; //扩展寄存器(XFR)访问使能
		RSTFLAG |= 0x04;   //设置硬件复位后需要检测P3.2的状态选择运行区域，否则硬件复位后进入USB下载模式	
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

	//PWMA_Init();
	//PWME_Init();

	//TIMER10_Init();
	PWMC_Init();
	TIMER9_Init();
	MATHLIB_Init();
	//ADC2_Init();
	//ADC2_DMA_Init();
	IAP_Init();
//	WS2812_Init();
}