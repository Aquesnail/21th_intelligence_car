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

}