<<<<<<< HEAD
#include "config.h"

// 强制所有端口进入用户手动控制模式
void GPIO_User_Control_On(void) {
    P0BP = 0xFF; P1BP = 0xFF; P2BP = 0xFF; P3BP = 0xFF;
    P4BP = 0xFF; P5BP = 0xFF; P6BP = 0xFF; P7BP = 0xFF;
    P8BP = 0xFF; P9BP = 0xFF; PABP = 0xFF; PBBP = 0xFF;
}

void GPIO_Init(void) {
    GPIO_User_Control_On();

    // --- P0口配置 ---
    P0M1 = 0x00; P0M0 = 0x00;           // 全准双向口
    P0PU = 0x10;                        // P0.4使能上拉
    P0IE = 0xFF;                        // 全数字输入使能
    P0NCS = 0x00;                       // 全施密特触发使能

    // --- P1口配置 ---
    // P1.7, P1.6, P1.3 高阻(10), 其余准双向(00)
    P1M1 = (PIN_7 | PIN_6 | PIN_3); 
    P1M0 = 0x00;
    P1PU = (PIN_5 | PIN_4);             // P1.5, P1.4上拉
    P1IE = ~(PIN_7 | PIN_6 | PIN_3);    // 模拟输入位关闭数字使能
    // P1中断配置: P1.5, P1.4 上升沿(001)
    P1IM2 &= ~(PIN_5 | PIN_4); 
    P1IM1 &= ~(PIN_5 | PIN_4); 
    P1IM0 |= (PIN_5 | PIN_4); 
    P1INTE |= (PIN_5 | PIN_4);          // 中断使能

    // --- P2口配置 ---
    // P2.7, P2.6开漏(11), P2.1, P2.0推挽(01), 其余准双向(00)
    P2M1 = (PIN_7 | PIN_6);
    P2M0 = (PIN_7 | PIN_6 | PIN_1 | PIN_0);
    P2IE = 0xFF;

    // --- P3口配置 ---
    // P3.1, P3.0高阻(10), 其余准双向(00)
    P3M1 = (PIN_1 | PIN_0);
    P3M0 = 0x00;
    P3IE = 0xFF;

    // --- P4口配置 ---
    // P4.5, P4.1推挽(01), 其余准双向(00)
    P4M1 = 0x00;
    P4M0 = (PIN_5 | PIN_1);
    P4PU = (PIN_6 | PIN_5);
    P4DR &= ~PIN_5;                     // P4.5增强驱动(0)
    // P4中断配置: P4.6, P4.0 上升沿(001)
    P4IM2 &= ~(PIN_6 | PIN_0);
    P4IM1 &= ~(PIN_6 | PIN_0);
    P4IM0 |= (PIN_6 | PIN_0);
    P4INTE |= (PIN_6 | PIN_0);

    // --- P5口配置 ---
    // P5.3推挽(01), P5.2高阻(10), 其余准双向(00)
    P5M1 = PIN_2;
    P5M0 = PIN_3;
    P5IE = 0xFF;

    // --- P6口配置 ---
    // P6.6, P6.4推挽(01), 其余准双向(00)
    P6M1 = 0x00;
    P6M0 = (PIN_6 | PIN_4);
    P6PU = PIN_1;
    // P6中断配置: P6.1, P6.5, P6.7 下降沿(000)
    P6IM2 &= ~(PIN_1 | PIN_5 | PIN_7);
    P6IM1 &= ~(PIN_1 | PIN_5 | PIN_7);
    P6IM0 &= ~(PIN_1 | PIN_5 | PIN_7);
    P6INTE |= (PIN_1 | PIN_5 | PIN_7);

    // --- P7口配置 ---
    P7M1 = 0x00; P7M0 = 0x00;           // 全准双向

    // --- P8口配置 ---
    // P8.3, P8.1, P8.0推挽(01), 其余准双向(00)
    P8M1 = 0x00;
    P8M0 = (PIN_3 | PIN_1 | PIN_0);
    P8PU = PIN_2;
    P8DR &= ~(PIN_3 | PIN_2 | PIN_1);   // 增强驱动

    // --- P9, PA, PB口配置 ---
    P9M1 = 0x00; P9M0 = PIN_5;          // P9.5推挽
    PAM1 = 0x00; PAM0 = 0x00;           // 全准双向
    PBM1 = 0x00; PBM0 = 0x00;           // 全准双向

    // --- 中断优先级全局配置 (最低级00) ---
    PINIPL = 0x00; PINIPH = 0x00;       // P0-P7中断优先级
    PIN2IPL = 0x00; PIN2IPH = 0x00;     // P8-PB中断优先级
=======
#include "gpio_user.h"

void GPIO_User_Control_On(void){
    P0BP = 0xFF;
    P1BP = ~(PIN_6 | PIN_7);
    P2BP = 0xFF;
    P3BP = 0xFF;
    P4BP = 0xFF;
    P5BP = 0xFF;
    P6BP = 0xFF;
    P7BP = 0xFF;
    P8BP = 0xFF;
    P9BP = 0xFF;
    PABP = 0xFF;
    PBBP = 0xFF;

    //全部手动控制，我不信任它的外设自动控制
}

void GPIO_Init(){
    

    /*P0*/
    P0M0 = 0x00; P0M1 = 0xff;  //全部高阻态模式

    /*P1*/
    P1M0 = 0x00;
    P1M1 = PIN_0 | PIN_1 | PIN_2 | PIN_3 | PIN_6 | PIN_7;
    P1PU = PIN_4 | PIN_5; //开启上拉电阻
    P1PD = 0x00;
    //P1NCS = 0x00; //全部使能施密特触发
    P1IE = PIN_4 | PIN_5;  //数字输入口必开数字输入

    P1INTE = PIN_4 | PIN_5;

    /*P2*/
    P2M0 = 0x00; 
    P2M1 = PIN_0 | PIN_1 | PIN_2 | PIN_3 | PIN_4 | PIN_5 | PIN_6 | PIN_7; //全部高阻态

    /*P3*/
    P3M0 = 0x00;
    P3M1 = 0x00; //全部准双向

    /*P4*/
    P4M0 = 0x00;
    P4M1 = PIN_0 | PIN_1 | PIN_2 | PIN_3 | PIN_4 | PIN_5 | PIN_6 | PIN_7; //全部高阻态

    /*P5*/
    P5M0 = PIN_0 | PIN_1         | PIN_3 | PIN_4 | PIN_5 | PIN_6 | PIN_7; 
    P5M1 = PIN_0 | PIN_1                         | PIN_5 | PIN_6 | PIN_7; //00双向,//10推挽,//11开漏,//01高阻
    P5PU = 0x00;
    P5PD = 0x00;
    //P5NCS = 0x00;
    P5IE = PIN_2;
   //P5SR = ~(PIN_4 | PIN_3);

    /*P6*/
    P6M0 = 0x00                  | PIN_4 | PIN_6;
    P6M1 = PIN_0 | PIN_1 | PIN_2;
    P6PU = 0x00;
    P6PD = 0x00;
   // P6NCS = 0x00;
    P6IE = 0x00;

    /*P7*/
    P7M0 = 0x00; P7M1 = 0x00; 

    /*P8*/
    P8M0 = PIN_1 | PIN_3       ;
    P8M1 =               PIN_2;
    P8PU = PIN_2 | PIN_0;
    P8PD = 0x00;
    //P8NCS = 0x00;
    P8IE = PIN_2;
    P8SR = ~(PIN_1 | PIN_3);

    /*P9*/
    P9M0 = 0x00;
    P9M1 = 0x00;

    /*PA*/
    PAM0 = 0x00;
    PAM1 = 0x00;

    /*PB*/
    PAM0 = 0x00;
    PAM1 = 0x00;


    // P0M0 = 0x00; P0M1 = 0x00; 
    // P1M0 = 0x00; P1M1 = 0x00; 
    // P2M0 = 0x00; P2M1 = 0x00; 
    // P3M0 = 0x00; P3M1 = 0x00; 
    // P4M0 = 0x00; P4M1 = 0x00; 
    // P5M0 = 0x08; P5M1 = 0x00; 
    // P6M0 = 0x00; P6M1 = 0x00; 
    // P7M0 = 0x00; P7M1 = 0x00; 

>>>>>>> 5df3a5ff6ce8a1d7497c20765d295a76287ed8a5
}