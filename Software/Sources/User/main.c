

#include "config.h"
#include "application.h"

#include "gpio_user.h"
//#include "port.h"

#include "clock.h"
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
    //WDT_Clear();
    app_init();
    //WDT_Clear();
    while(1){}
}

/* --- 裸机回环测试（已通过，注释备用）---
void main_loopback_test(void){
    uint8_t rx_buf[64];
    uint16_t rx_available;
    uint16_t rx_read;
    volatile uint32_t delay_cnt;
    const char *start_msg;

    sys_init();
    UART3_Init();

    start_msg = "\r\n=== UART3 Loopback Test (No RTOS) ===\r\n";
    DMA_UART_Write(&uart3_mgr, (const uint8_t *)start_msg, (uint16_t)strlen(start_msg));

    while(1){
        rx_available = DMA_UART_Available(&uart3_mgr);
        if(rx_available > 0){
            if(rx_available > sizeof(rx_buf)){
                rx_available = sizeof(rx_buf);
            }
            rx_read = DMA_UART_Read(&uart3_mgr, rx_buf, rx_available);
            if(rx_read > 0){
                DMA_UART_Write(&uart3_mgr, rx_buf, rx_read);
            }
        }
        for(delay_cnt = 0; delay_cnt < 5000; delay_cnt++){}
    }
}
--- 裸机回环测试结束 --- */

void sys_init(void){
	GPIO_User_Control_On();

	GPIO_Init();
	delay_ms(10);
	CLK_Init();    

	//WDT_SetPrescale256();
	//CLK_Init();  
	WTST = 0x04;
	delay_ms(100);
	
	//GPIO_Init();

	//PWMA_Init();
	//PWME_Init();

	//TIMER10_Init();
	PWME_Init();
	//PWMA_Init();
	TIMER9_Init();
	MATHLIB_Init();
	//ADC2_Init();
	//ADC2_DMA_Init();
	IAP_Init();
//	WS2812_Init();
}