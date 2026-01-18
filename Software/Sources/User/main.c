

#include "config.h"
#include "application.h"
#include "port.h"

void sys_init(void);
void main(){
	    EAXFR = 1; //扩展寄存器(XFR)访问使能
		CKCON = 0; //提高访问XRAM速
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
}