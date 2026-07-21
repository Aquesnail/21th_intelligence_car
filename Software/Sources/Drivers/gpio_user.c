
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
    // P0.0-P0.4: ADC, 高阻输入; 其余浮空准双向
    P0M1 = (PIN_0 | PIN_1 | PIN_2 | PIN_3 | PIN_4);
    P0M0 = 0x00;
    P0PU = 0x00;
    P0IE = ~(PIN_0 | PIN_1 | PIN_2 | PIN_3 | PIN_4);
    P0NCS = 0x00;

    // --- P1口配置 ---
    // P1.1: TX, 推挽;  P1.0: RX, 准双向
    // P1.6, P1.7: 保持原配置(高阻);  其余浮空准双向
    P1M1 = (PIN_2);
    P1M0 = (PIN_7 |PIN_1 | PIN_6 );
    P1PU = 0x00;
    P1SR = ~PIN_1;
    P1DR &= ~PIN_1;
    P1IE = ~(PIN_7 | PIN_6);
    P1INTE = 0x00;

    // --- P2口配置 ---
    // P2.3: SPI_MOSI, P2.5: SPI_SCK, P2.2: SPI_CTR_CS — 推挽
    // P2.0: SPI_CTR_INT1, P2.1: SPI_CTR_INT2, P2.4: SPI_MISO — 准双向
    // 其余浮空准双向
    P2M1 = PIN_1 | PIN_0;
    P2M0 = (PIN_2 | PIN_3 | PIN_5 | PIN_7);
    P2SR = ~(PIN_3 | PIN_5);
    P2DR &= ~(PIN_3 | PIN_5);
    P2IM2 = PIN_0;
    SetP2IntPriority(2); // P2.0最高优先级
    
    P2IE = 0xFF;
    //P2INTE = PIN_0;
		
    // --- P3口配置 ---
    // P3.2: ICM_SCK, P3.4: ICM_MOSI, P3.5: ICM_CS — 推挽
    // P3.3: ICM_MISO — 准双向;  其余浮空准双向
    P3M1 = 0x00;
    P3M0 = (PIN_2 | PIN_4 | PIN_5);
    P3SR = ~(PIN_2 | PIN_4);
   // P3DR &= ~(PIN_2 | PIN_4);
    P3IE = 0xFF;

    // --- P4口配置 ---
    // 全浮空准双向
    P4M1 = 0x00;
    P4M0 = 0x00;
    P4PU = 0x00;
    P4IE = 0xFF;
    P4INTE = 0x00;	

    // --- P5口配置 ---
    // P5.0: ICM_INT2, 推挽
    // P5.1: ICM_INT1, 准双向, 上拉开, 中断开(下降沿)
    // 其余浮空准双向
    P5M1 = 0x00;
    P5M0 = PIN_0;
    P5PU = PIN_1;
   // P5SR = ~(PIN_4);
    P5IE = 0xFF;
    P5IM2 &= ~(PIN_1 | PIN_4);
    P5IM1 &= ~(PIN_1 | PIN_4);
    P5IM0 &= ~(PIN_1 | PIN_4);
    P5INTE = PIN_1 | PIN_4;

    // --- P6口配置 ---
    // P6.0: BUZZER, P6.4: LED_PWM — 推挽
    // 其余浮空准双向
    P6M1 = 0x00;
    P6M0 = (PIN_0 | PIN_4);
    P6SR = ~(PIN_0 | PIN_4);
    P6DR &= ~(PIN_0 | PIN_4);
    P6PU = 0x00;
    P6INTE = 0x00;

    // --- P7口配置 ---
    P7M1 = 0x00; P7M0 = 0x00;           // 全准双向
    P7IE = 0xFF;

    // --- P8口配置 ---
    P8M1 = 0x00; P8M0 = 0x00;           // 全准双向
    P8PU = 0x00;

    // --- P9, PA, PB口配置 ---
    P9M1 = 0x00; P9M0 = 0x00;           // 全准双向
    PAM1 = 0x00; PAM0 = 0x00;           // 全准双向
    PBM1 = 0x00; PBM0 = 0x00;           // 全准双向

    // --- 中断优先级全局配置 (最低级00) ---
    PINIPL = 0x00; PINIPH = 0x00;       // P0-P7中断优先级
    PIN2IPL = 0x00; PIN2IPH = 0x00;     // P8-PB中断优先级

}