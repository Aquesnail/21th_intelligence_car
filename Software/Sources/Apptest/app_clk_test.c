#include "FreeRTOS.h"
#include "task.h"
#include "ws2812.h"
#include "adc_user.h"
#include "debug_arch.h"
#include <stdio.h>

#pragma warning disable = 98
#pragma warning disable = 91

/* VL6180X I2C 7-bit address (左移1位 = 写地址, |1 = 读地址) */
#define VL6180X_WRITE_ADDR   0x52
#define VL6180X_READ_ADDR    0x53

/* VL6180X 识别寄存器地址 (16-bit) */
#define VL6180X_REG_MODEL_ID          0x000
#define VL6180X_REG_MODEL_REV_MAJOR   0x001
#define VL6180X_REG_MODEL_REV_MINOR   0x002
#define VL6180X_REG_MODULE_REV_MAJOR  0x003
#define VL6180X_REG_MODULE_REV_MINOR  0x004
#define VL6180X_REG_DATE_HI           0x006
#define VL6180X_REG_DATE_LO           0x007
#define VL6180X_REG_TIME              0x008

/* ---- 简易 I2C 初始化 (参照官方例程, 不用 DMA) ---- */
static void I2C_SimpleInit(void)
{
    I2C_S1 = 1;         /* P3.2=SCL, P3.3=SDA */
    I2C_S0 = 0;
    I2CCFG = 0xe0;      /* ENI2C=1, MSSL=1(主机), SPEED=0 */
    I2CMSST = 0x00;     /* 清除状态 */
    I2C1_SetClockDivider(1600);
    DMA_I2C_CR = 0x00;  /* 关闭 DMA，纯轮询模式 */
}

/* ---- 等待 I2C 主机命令完成 (直接轮询硬件 MSIF) ---- */
static void I2C_Wait(void)
{
    while ((I2CMSST & 0x40) == 0);  /* 等待 MSIF=1 (命令完成) */
    I2CMSST &= ~0x40;               /* 清除 MSIF */
}

/* ---- 等待 I2C 主机空闲 ---- */
static void I2C_WaitBusy(void)
{
    while (I2CMSST & 0x80);   /* 等待 BUSY=0 */
}

/* ---- 从 VL6180X 16-bit 寄存器读 1 字节 ---- */
static uint8_t VL6180X_ReadByte(uint16_t reg_addr)
{
    uint8_t dat;

    I2C_WaitBusy();

    /* 第1帧: START + 写地址 + 寄存器地址高字节 + 寄存器地址低字节 */
    I2CTXD = VL6180X_WRITE_ADDR;
    I2CMSCR = 0x09;             /* START + 发送 I2CTXD */
    I2C_Wait();

    I2CTXD = (uint8_t)(reg_addr >> 8);   /* 寄存器地址 [15:8] */
    I2CMSCR = 0x0a;             /* 发送数据 (无 START) */
    I2C_Wait();

    I2CTXD = (uint8_t)(reg_addr & 0xFF); /* 寄存器地址 [7:0] */
    I2CMSCR = 0x0a;             /* 发送数据 */
    I2C_Wait();

    /* 第2帧: 重复 START + 读地址 */
    I2CTXD = VL6180X_READ_ADDR;
    I2CMSCR = 0x09;             /* START + 发送 I2CTXD */
    I2C_Wait();

    /* 接收 1 字节 + NAK */
    I2CMSCR = 0x0c;             /* 接收数据 + 回应 NAK */
    I2C_Wait();
    dat = I2CRXD;

    /* 发送 STOP */
    I2CMSCR = 0x06;             /* STOP */
    I2C_Wait();

    return dat;
}

/* ---- 从 VL6180X 16-bit 寄存器读 2 字节 (大端) ---- */
static uint16_t VL6180X_ReadWord(uint16_t reg_addr)
{
    uint16_t result;
    uint8_t hi;
    uint8_t lo;

    I2C_WaitBusy();

    /* 第1帧: START + 写地址 + 寄存器地址高字节 + 寄存器地址低字节 */
    I2CTXD = VL6180X_WRITE_ADDR;
    I2CMSCR = 0x09;
    I2C_Wait();

    I2CTXD = (uint8_t)(reg_addr >> 8);
    I2CMSCR = 0x0a;
    I2C_Wait();

    I2CTXD = (uint8_t)(reg_addr & 0xFF);
    I2CMSCR = 0x0a;
    I2C_Wait();

    /* 第2帧: 重复 START + 读地址 */
    I2CTXD = VL6180X_READ_ADDR;
    I2CMSCR = 0x09;
    I2C_Wait();

    /* 接收 2 字节: 第1字节 ACK, 第2字节 NAK */
    I2CMSCR = 0x0b;             /* 接收数据 + 回应 ACK (还有后续) */
    I2C_Wait();
    hi = I2CRXD;

    I2CMSCR = 0x0c;             /* 接收数据 + 回应 NAK (最后一个) */
    I2C_Wait();
    lo = I2CRXD;

    /* STOP */
    I2CMSCR = 0x06;
    I2C_Wait();

    result = ((uint16_t)hi << 8) | (uint16_t)lo;
    return result;
}

void app_clk_test(void)
{
    uint8_t model_id;
    uint8_t rev_major;
    uint8_t rev_minor;
    uint8_t module_rev_major;
    uint8_t module_rev_minor;
    uint8_t date_hi;
    uint8_t date_lo;
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t phase;
    uint16_t time_val;
    uint8_t led_on;
    char log_buf[128];

    led_on = 0;

    /* 任务启动标记 */
    WS2812_Clear();
    WS2812_SetColor(0, 0, 0, 50);  /* 蓝色 */
    WS2812_Show();
    vTaskDelay(pdMS_TO_TICKS(500));

    Debug_Log("VL6180X Task Started\r\n");

    for (;;) {
        /* ---- 1. LED 闪烁 ---- */
        WS2812_Clear();
        if (led_on) {
            WS2812_SetColor(0, 50, 50, 0);  /* 黄色 */
        }
        WS2812_Show();
        led_on = !led_on;

        /* ---- 2. 读取 VL6180X ---- */
        Debug_Log("R:MODEL_ID...\r\n");
        model_id = VL6180X_ReadByte(VL6180X_REG_MODEL_ID);

        Debug_Log("R:REV_MAJOR...\r\n");
        rev_major = VL6180X_ReadByte(VL6180X_REG_MODEL_REV_MAJOR) & 0x07;

        Debug_Log("R:REV_MINOR...\r\n");
        rev_minor = VL6180X_ReadByte(VL6180X_REG_MODEL_REV_MINOR) & 0x07;

        Debug_Log("R:MODULE_MAJOR...\r\n");
        module_rev_major = VL6180X_ReadByte(VL6180X_REG_MODULE_REV_MAJOR) & 0x07;

        Debug_Log("R:MODULE_MINOR...\r\n");
        module_rev_minor = VL6180X_ReadByte(VL6180X_REG_MODULE_REV_MINOR) & 0x07;

        Debug_Log("R:DATE_HI...\r\n");
        date_hi = VL6180X_ReadByte(VL6180X_REG_DATE_HI);

        Debug_Log("R:DATE_LO...\r\n");
        date_lo = VL6180X_ReadByte(VL6180X_REG_DATE_LO);

        Debug_Log("R:TIME...\r\n");
        time_val = VL6180X_ReadWord(VL6180X_REG_TIME);

        /* 解析日期 */
        year  = (date_hi >> 4) & 0x0F;
        month = date_hi & 0x0F;
        day   = (date_lo >> 3) & 0x1F;
        phase = date_lo & 0x07;

        /* ---- 3. 汇总打印 ---- */
        sprintf(log_buf,
            "VL6180X: ID=0x%02X R%u.%u M%u.%u "
            "D=20%02u-%02u-%02u Ph%u T=%u\r\n",
            model_id,
            rev_major, rev_minor,
            module_rev_major, module_rev_minor,
            year, month, day, phase,
            time_val);
        Debug_Log(log_buf);

        /* 500ms 周期 */
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void APP_CLK_TEST_Init(void)
{
    I2C_SimpleInit();
    xTaskCreate(app_clk_test, "AppClkTest", 512, NULL, tskIDLE_PRIORITY + 1, NULL);
}
