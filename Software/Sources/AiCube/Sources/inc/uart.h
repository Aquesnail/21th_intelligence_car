//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: uart.h
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#ifndef __UART_H__
#define __UART_H__


//<<AICUBE_USER_DEFINE_BEGIN>>
// 在此添加用户宏定义  
//<<AICUBE_USER_DEFINE_END>>


#define UART4_RXSIZE            16      //串口4接收缓冲区大小


void UART4_Init(void);
void UART4_SendByte(uint8_t dat);
void UART4_SendBuffer(uint8_t *dat, uint8_t size);
void UART4_SendString(char *str);
void UART4_RecvPolling(void);

extern uint8_t u8UART4RxCount;
extern uint8_t xdata pu8UART4RxBuffer[UART4_RXSIZE];


//<<AICUBE_USER_EXTERNAL_DECLARE_BEGIN>>
uint8_t UART5_Write(char * str,uint16_t len);  
//<<AICUBE_USER_EXTERNAL_DECLARE_END>>


#endif
