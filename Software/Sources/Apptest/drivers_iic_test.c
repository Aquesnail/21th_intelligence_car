#include "FreeRTOS.h"
#include "task.h"
#include "i2c.h"

/* 假设外部已经定义了 Debug_Log */
extern void Debug_Log(const char *format, ...);

/* SSD1306 I2C 地址 (一般为 0x78 或 0x7A, 0x3C << 1 = 0x78) */
#define OLED_I2C_ADDR 0x78

/* --------------------------------------------------------------------------
 * 基础 I2C 写入函数
 * -------------------------------------------------------------------------- */

/* 向 OLED 写入一条命令 */
void OLED_WriteCmd(uint8_t cmd)
{
    I2C2_MasterStart();
    I2C2_MasterSendByte(OLED_I2C_ADDR); /* 发送器件地址 (写方向) */
    I2C2_MasterSendByte(0x00);          /* 发送控制字节: 0x00 表示接下来的字节是命令 */
    I2C2_MasterSendByte(cmd);           /* 发送命令数据 */
    I2C2_MasterStop();
}

/* 向 OLED 写入一个字节的显存数据 */
void OLED_WriteData(uint8_t dat)
{
    I2C2_MasterStart();
    I2C2_MasterSendByte(OLED_I2C_ADDR); /* 发送器件地址 (写方向) */
    I2C2_MasterSendByte(0x40);          /* 发送控制字节: 0x40 表示接下来的字节是数据 */
    I2C2_MasterSendByte(dat);           /* 发送显存数据 */
    I2C2_MasterStop();
}

/* --------------------------------------------------------------------------
 * OLED 屏幕控制函数
 * -------------------------------------------------------------------------- */

/* 使用指定的数据填充整个 OLED 屏幕 (用于清屏或全显测试) */
void OLED_Fill(uint8_t fill_data)
{
    /* C89 规范：局部变量声明必须在函数最开头 */
    uint8_t i, j;

    for (i = 0; i < 8; i++) 
    {
        OLED_WriteCmd(0xB0 + i); /* 设置页地址 (Page0 ~ Page7) */
        OLED_WriteCmd(0x00);     /* 设置列低地址 */
        OLED_WriteCmd(0x10);     /* 设置列高地址 */
        
        for (j = 0; j < 128; j++) 
        {
            OLED_WriteData(fill_data);
        }
    }
}

/* 初始化 SSD1306 128x64 */
void OLED_Init(void)
{
    OLED_WriteCmd(0xAE); /* 关闭显示 (Display OFF) */

    OLED_WriteCmd(0x20); /* 设置内存寻址模式 */
    OLED_WriteCmd(0x10); /* 00: 水平寻址; 01: 垂直寻址; 10: 页寻址模式 (默认) */

    OLED_WriteCmd(0xB0); /* 设置页寻址模式的起始页地址 0-7 */
    OLED_WriteCmd(0xC8); /* 设置 COM 输出扫描方向 (从 COM63 到 COM0) */

    OLED_WriteCmd(0x00); /* 设置列起始低地址 */
    OLED_WriteCmd(0x10); /* 设置列起始高地址 */

    OLED_WriteCmd(0x40); /* 设置起始行地址 */

    OLED_WriteCmd(0x81); /* 设置对比度控制寄存器 */
    OLED_WriteCmd(0xFF); /* 对比度值 (0x00~0xFF) */

    OLED_WriteCmd(0xA1); /* 设置段重映射 (Segment Re-map) 0 到 127 */
    OLED_WriteCmd(0xA6); /* 设置正常/反转显示 (0xA6: 正常; 0xA7: 反转) */

    OLED_WriteCmd(0xA8); /* 设置复用率 (Multiplex Ratio) */
    OLED_WriteCmd(0x3F); /* 128x64 对应 0x3F (63) */

    OLED_WriteCmd(0xA4); /* 全局显示开启 (0xA4: 输出随 RAM 变化; 0xA5: 全亮不随 RAM 变化) */
    
    OLED_WriteCmd(0xD3); /* 设置显示偏移 (Display Offset) */
    OLED_WriteCmd(0x00); /* 无偏移 */

    OLED_WriteCmd(0xD5); /* 设置显示时钟分频比/振荡器频率 */
    OLED_WriteCmd(0xF0); /* 设置分频比 */

    OLED_WriteCmd(0xD9); /* 设置预充电周期 (Pre-charge Period) */
    OLED_WriteCmd(0x22); 

    OLED_WriteCmd(0xDA); /* 设置 COM 引脚硬件配置 */
    OLED_WriteCmd(0x12); /* 128x64 的默认配置 */

    OLED_WriteCmd(0xDB); /* 设置 VCOMH 取消选择级别 */
    OLED_WriteCmd(0x20); /* 0.77 x Vcc */

    OLED_WriteCmd(0x8D); /* 设置电荷泵使能 (Charge Pump) */
    OLED_WriteCmd(0x14); /* 开启电荷泵 */

    OLED_WriteCmd(0xAF); /* 开启显示 (Display ON) */
}

/* --------------------------------------------------------------------------
 * FreeRTOS 测试任务
 * -------------------------------------------------------------------------- */

/* OLED 闪烁测试任务 */
void vTask_OLED_Test(void *pvParameters)
{
    /* C89 规范：局部变量声明必须在函数最开头 */
    uint8_t loop_count = 0;

    /* 消除未使用参数的编译警告 */
    (void)pvParameters;

    Debug_Log("OLED Test Task is starting...\r\n");

    /* 确保 I2C2 已经初始化完毕。若外部已调用，此处可注释 */
    I2C2_Init(); 

    /* 初始化 OLED 屏幕 */
    OLED_Init();
    Debug_Log("OLED Init done. Target address: 0x%02X\r\n", OLED_I2C_ADDR);

    /* 初始清屏 */
    OLED_Fill(0x00);
    Debug_Log("OLED Screen cleared.\r\n");

    /* 进入任务主循环 */
    while (1)
    {
        loop_count++;
        Debug_Log("OLED Test Loop: %d\r\n", loop_count);

        /* 1. 全亮测试 */
        Debug_Log("-> Fill Screen: All ON\r\n");
        OLED_Fill(0xFF);
        vTaskDelay(pdMS_TO_TICKS(1000));

        /* 2. 斑马线/棋盘格测试 (0x55 = 01010101B) */
        Debug_Log("-> Fill Screen: Pattern (0x55)\r\n");
        OLED_Fill(0x55);
        vTaskDelay(pdMS_TO_TICKS(1000));

        /* 3. 全灭测试 (清屏) */
        Debug_Log("-> Fill Screen: All OFF\r\n");
        OLED_Fill(0x00);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void Task_OLED_Test_Init(void)
{
    xTaskCreate(vTask_OLED_Test, "OLED_Test", 256, NULL, tskIDLE_PRIORITY+10, NULL);
}