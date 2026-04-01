

#include "config.h"
#include "application.h"
<<<<<<< HEAD
=======
#include "gpio_user.h"
//#include "port.h"
>>>>>>> 5df3a5ff6ce8a1d7497c20765d295a76287ed8a5
#include "clock.h"
#include "pwm.h"
#include "timer.h"
#include "iap_user.h"
#include "ws2812.h"
#include "mathlib.h"
#include "gpio_user.h"

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
	GPIO_User_Control_On();
<<<<<<< HEAD
	GPIO_Init();
	CLK_Init();    
=======
	CLK_Init();  
	//delay_ms(1000);
	
	GPIO_Init();
>>>>>>> 5df3a5ff6ce8a1d7497c20765d295a76287ed8a5
	PWMA_Init();
	PWME_Init();

	MATHLIB_Init();
	
	IAP_Init();
	WS2812_Init();
}