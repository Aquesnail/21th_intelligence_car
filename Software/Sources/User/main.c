

#include "config.h"
#include "application.h"
#include "port.h"
#include "clock.h"
#include "pwm.h"
#include "iap_user.h"
#include "ws2812.h"

void sys_init(void);
void main(){
	  EnableAccessXFR();                  //??????XFR
    AccessCodeFastest();                //????????????
    AccessIXramFastest();               //??????????XDATA
    IAP_SetTimeBase();                  //??IAP????,??1us??
	
		sys_init();
		app_init();
		while(1){}
	
}

void sys_init(void){
	PORT0_Init();
	PORT1_Init();
	PORT2_Init();
	PORT3_Init();
	PORT4_Init();
	PORT5_Init();
	PORT6_Init();
	PORT7_Init();
	PORT8_Init();
	PORT9_Init();
	PORTA_Init();
	PORTB_Init();
	CLK_Init();    
	PWMA_Init();
	PWME_Init();
	IAP_Init();
	WS2812_Init();
}