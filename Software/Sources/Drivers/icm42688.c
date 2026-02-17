#include "icm42688.h"

/* 辅助函数：切换 Bank */
static uint8_t icm_set_bank(ICM42688_t *dev, uint8_t bank) {
    if (dev->current_bank == bank) return ICM_OK;
    
    // REG_BANK_SEL 位于 Bank 0 (实际上它在所有 Bank 都可以访问)
    if (dev->write(ICM42688_REG_BANK_SEL, bank) != 0) {
        return ICM_ERROR;
    }
    dev->current_bank = bank;
    return ICM_OK;
}

/* 辅助函数：写特定 Bank 的寄存器 */
static uint8_t icm_write_reg(ICM42688_t *dev, uint8_t bank, uint8_t reg, uint8_t value) {
    if (icm_set_bank(dev, bank) != ICM_OK) return ICM_ERROR;
    return dev->write(reg, value) == 0 ? ICM_OK : ICM_ERROR;
}

/* 辅助函数：读特定 Bank 的寄存器 */
static uint8_t icm_read_regs(ICM42688_t *dev, uint8_t bank, uint8_t reg, uint8_t *dat, uint16_t len) {
    if (icm_set_bank(dev, bank) != ICM_OK) return ICM_ERROR;
   // if (len == 1 && dev->read_byte != NULL) {
   //     return dev->read_byte(reg, dat) == 0 ? ICM_OK : ICM_ERROR;
   // } else {
        return dev->read(reg, dat, len) == 0 ? ICM_OK : ICM_ERROR;
   // }
}

static uint8_t icm_read_reg(ICM42688_t *dev, uint8_t bank, uint8_t reg, uint8_t *dat) {
    if (icm_set_bank(dev, bank) != ICM_OK) return ICM_ERROR;
   // if (len == 1 && dev->read_byte != NULL) {
    return dev->read_byte(reg, dat) == 0 ? ICM_OK : ICM_ERROR;
   // } else {
   //     return dev->read(reg, dat, len) == 0 ? ICM_OK : ICM_ERROR;
   // }
}
/* 更新灵敏度辅助函数 */
/*
ACCEL_FS_SEL_
GYRO_FS_SEL_
*/
static void update_sensitivity(ICM42688_t *dev, uint8_t a_scale, uint8_t g_scale) {
    switch(a_scale & ACCEL_FS_SEL_MASK) {
        case ACCEL_FS_SEL_16G: dev->acc_sensitivity = 16.0f / 32768.0f; break;
        case ACCEL_FS_SEL_8G:  dev->acc_sensitivity = 8.0f  / 32768.0f; break;
        case ACCEL_FS_SEL_4G:  dev->acc_sensitivity = 4.0f  / 32768.0f; break;
        case ACCEL_FS_SEL_2G:  dev->acc_sensitivity = 2.0f  / 32768.0f; break;
        default:               dev->acc_sensitivity = 16.0f / 32768.0f; break;
    }

    switch(g_scale & GYRO_FS_SEL_MASK) {
        case GYRO_FS_SEL_2000DPS: dev->gyro_sensitivity = 2000.0f / 32768.0f; break;
        case GYRO_FS_SEL_1000DPS: dev->gyro_sensitivity = 1000.0f / 32768.0f; break;
        case GYRO_FS_SEL_500DPS:  dev->gyro_sensitivity = 500.0f  / 32768.0f; break;
        case GYRO_FS_SEL_250DPS:  dev->gyro_sensitivity = 250.0f  / 32768.0f; break;
        default:                  dev->gyro_sensitivity = 2000.0f / 32768.0f; break;
    }
}

/* AAF 设置封装 */
static uint8_t configure_aaf(ICM42688_t *dev, uint8_t is_accel, uint8_t delt, uint16_t deltsqr, uint8_t bitshift) {
    uint8_t bank = is_accel ? 2 : 1; // Accel 在 Bank 2, Gyro 在 Bank 1
    uint8_t reg_base = is_accel ? ICM42688_ACCEL_CONFIG_STATIC2 : ICM42688_GYRO_CONFIG_STATIC3;
    
    uint8_t deltsqr_low = deltsqr & 0xFF;
    uint8_t deltsqr_high = (deltsqr >> 8) & 0x0F;
    
    // 设置 Delt
    if (icm_write_reg(dev, bank, reg_base, delt << 1) != ICM_OK) return ICM_ERROR;
    // 设置 DeltSqr Low
    if (icm_write_reg(dev, bank, reg_base + 1, deltsqr_low) != ICM_OK) return ICM_ERROR;
    // 设置 DeltSqr High | Bitshift
    if (icm_write_reg(dev, bank, reg_base + 2, (deltsqr_high | (bitshift << 4))) != ICM_OK) return ICM_ERROR;

    return ICM_OK;
}

uint8_t ICM42688_Init(ICM42688_t *dev) {
    uint8_t chip_id = 0;
    uint8_t gyro_cfg;
    uint8_t accel_cfg;
    uint8_t fifo_cfg1;
    uint8_t intf_cfg0;
    uint8_t int_cfg;
    uint8_t pwr_mgmt;
    uint8_t reg_val;
    const uint8_t accel_delt = 2;
    const uint16_t accel_delt_sqr = 4;
    const uint8_t accel_bitshift = 13;
    
    if (!dev || !dev->write || !dev->read || !dev->read_byte || !dev->delay) return ICM_ERROR;

    if (dev->FIFO_THS == 0) dev->FIFO_THS = 48;
    dev->current_bank = 0xFF; // 强制首次切换
    dev->delay(10); // 上电等待

    // 1. 软复位 (可选，建议加上)
    icm_write_reg(dev, 0, ICM42688_DEVICE_CONFIG, 0x01);
    dev->delay(10); // 等待复位完成

    // 2. 检查 ID
    if (icm_read_reg(dev, 0, ICM42688_WHO_AM_I, &chip_id, 1) != ICM_OK) return ICM_ERROR;
    if (chip_id != 0x47) return ICM_ERROR;

    // 3. 配置 Power Management (先开启传感器，进入低噪声模式)
    // 注意：手册建议配置其他寄存器前先开启时钟，或者保持 IDLE
    // 这里按照你原来的逻辑，先配参数最后开，也可以，但需确保 CLK 正常
    
    // 4. 配置 Gyro (Bank 0)
    // 500dps, 500Hz ODR
    gyro_cfg = GYRO_FS_SEL_500DPS | GYRO_ODR_500HZ;
    if (icm_write_reg(dev, 0, ICM42688_GYRO_CONFIG0, gyro_cfg) != ICM_OK) return ICM_ERROR;

    // 5. 配置 Accel (Bank 0)
    // 4G, 500Hz ODR
    accel_cfg = ACCEL_FS_SEL_4G | ACCEL_ODR_500HZ;
    if (icm_write_reg(dev, 0, ICM42688_ACCEL_CONFIG0, accel_cfg) != ICM_OK) return ICM_ERROR;

    // 更新结构体中的灵敏度
    update_sensitivity(dev, accel_cfg, gyro_cfg);

    // 6. 配置 FIFO (Bank 0)
    if (icm_write_reg(dev, 0, ICM42688_FIFO_CONFIG, FIFO_MODE_STREAM_TO_FIFO) != ICM_OK) return ICM_ERROR;

    // FIFO Config 1: Enable Accel, Gyro, Temp, Partial Read
    fifo_cfg1 =           FIFO_GYRO_EN_ENABLE 
                        | FIFO_ACCEL_EN_ENABLE
                        | FIFO_TEMP_EN_ENABLE  
                        | FIFO_WM_GT_TH_ENABLE 
                        | FIFO_HIRES_EN_DISABLE 
                        | FIFO_TMST_FSYNC_EN_ENABLE
                        | FIFO_RESUME_PARTIAL_RD_DISABLE; //由于我们是DMA读的，单个包内不会有中断，故关闭部分读保证每一次读取都是完整包(16字节)
    if (icm_write_reg(dev, 0, ICM42688_FIFO_CONFIG1, fifo_cfg1) != ICM_OK) return ICM_ERROR;

    if (
        icm_write_reg(dev, 0, ICM42688_TMST_CONFIG,
            TMST_EN_ENABLE | 
            TMST_RES_1US |
            TMST_DELTA_EN_DISABLE| //时间戳里面是一个很大的绝对时间
            TMST_FSYNC_EN_DISABLE |
            TMST_TO_REGS_EN_DISABLE
        ) != ICM_OK
    )return ICM_ERROR;
    // FIFO Watermark (48 bytes)
    if (icm_write_reg(dev, 0, ICM42688_FIFO_CONFIG2, (uint8_t)(dev->FIFO_THS)) != ICM_OK) return ICM_ERROR; // Low byte
    if (icm_write_reg(dev, 0, ICM42688_FIFO_CONFIG3, (uint8_t)((dev->FIFO_THS)>>8)) != ICM_OK) return ICM_ERROR;  // High byte

    // 7. 接口配置 INTF_CONFIG0 (Bank 0)
    // Big Endian
    intf_cfg0 = FIFO_COUNT_BIG_ENDIAN | SENSOR_DATA_BIG_ENDIAN 
    | FIFO_COUNT_IN_RECORDS; // 这里以数据包数量为单位//FIFO_COUNT_LITTLE_ENDIAN | SENSOR_DATA_LITTLE_ENDIAN | FIFO_COUNT_IN_BYTES;
    if (icm_write_reg(dev, 0, ICM42688_INTF_CONFIG0, intf_cfg0) != ICM_OK) return ICM_ERROR;

    // 8. 中断配置 (Bank 0)
    // INT_SOURCE3: FIFO Threshold Interrupt
    // if (icm_write_reg(dev, 0, ICM42688_INT_SOURCE3, 0x04) != ICM_OK) return ICM_ERROR;
    if (icm_write_reg(dev,0,ICM42688_INT_SOURCE0,FIFO_THS_INT1_EN_ENABLE) != ICM_OK) return ICM_ERROR;
    // INT_CONFIG0: Clear on both reads
    if (icm_write_reg(dev, 0, ICM42688_INT_CONFIG0, FIFO_THS_INT_CLEAR_ON_BOTH_READS) != ICM_OK) return ICM_ERROR;

    // INT_CONFIG: Push-Pull/Open-Drain, Pulsed (按照你原来的 0x20 | ... 设置)
    // 你的旧代码设置了 INT1_MODE_PULSED (0) 和 INT2_MODE_PULSED (0)
    // 但原代码 reg_value |= INT1_POLARITY_ACTIVE_HIGH... 
    // 这里保持你原有的逻辑
    int_cfg = INT1_MODE_PULSED | INT2_MODE_PULSED | 
                      INT1_DRIVE_CIRCUIT_OPEN_DRAIN | INT2_DRIVE_CIRCUIT_OPEN_DRAIN |
                      INT1_POLARITY_ACTIVE_HIGH | INT2_POLARITY_ACTIVE_HIGH;
    if (icm_write_reg(dev, 0, ICM42688_INT_CONFIG, int_cfg) != ICM_OK) return ICM_ERROR;

    if (icm_write_reg(dev, 0, ICM42688_INT_CONFIG1, 0x00) != ICM_OK) return ICM_ERROR; 
    // 9. UI 滤波器配置 (Bank 0)
    // GYRO: 2nd Order, 3rd Order Dec2
    if (icm_write_reg(dev, 0, ICM42688_GYRO_CONFIG1, GYRO_UI_FILT_ORD_2ND | GYRO_DEC2_M2_ORD_3RD) != ICM_OK) return ICM_ERROR;
    // ACCEL: 2nd Order
    if (icm_write_reg(dev, 0, ICM42688_ACCEL_CONFIG1, ACCEL_UI_FILT_ORD_2ND | ACCEL_DEC2_M2_ORD_3RD) != ICM_OK) return ICM_ERROR;
    // BW: Low Latency 400Hz
    if (icm_write_reg(dev, 0, ICM42688_GYRO_ACCEL_CONFIG0, GYRO_UI_FILT_BW_LOW_LATENCY_400HZ | ACCEL_UI_FILT_BW_LOW_LATENCY_400HZ) != ICM_OK) return ICM_ERROR;

    // 10. AAF (Anti-Aliasing Filter) 设置
    // Accel AAF (Bank 2)
    if (configure_aaf(dev, 1, accel_delt, accel_delt_sqr, accel_bitshift) != ICM_OK) return ICM_ERROR;
    
    // Gyro AAF (Bank 1) - 使用相同的参数
    if (configure_aaf(dev, 0, accel_delt, accel_delt_sqr, accel_bitshift) != ICM_OK) return ICM_ERROR;

    // Enable AAF (Bank 1)
    // GYRO_CONFIG_STATIC2: bit 1 (GYRO_AAF_DIS) = 0 (Enable), bit 0 (NF_DIS) = 1 (Disable Notch)
    // 你的代码写的是 1<<3 (0x08)，但 BIT3 在 Datasheet 是保留位，可能是笔误？
    // 根据你的注释 "enable aaf for gyro and disable the notch filter"，
    // 应该是 GYRO_NF_DIS (Bit 0) = 1, GYRO_AAF_DIS (Bit 1) = 0.
    // 但为了兼容你的旧代码逻辑，我保留你原代码的意图，这里假设你是要操作 Bank 1 的配置。
    // *修正*: 你的原代码写 1<<3 (XG_DISABLE?) 看起来是在 SENSOR_CONFIG0 (Bank 1, 0x03) 的位置，
    // 但你说的是 GYRO_CONFIG_STATIC2 (Bank 1, 0x0B)。
    // 标准配置：Enable AAF, Disable Notch -> 写入 0x01 (即 Bit 0 置 1)
    if (icm_write_reg(dev, 1, ICM42688_GYRO_CONFIG_STATIC2, 0x01) != ICM_OK) return ICM_ERROR;

    // Enable Accel AAF (Bank 2)
    // ACCEL_CONFIG_STATIC2 bit 0 is ACCEL_AAF_DIS. 0 = Enable.
    // 你的旧代码逻辑：ICM42688_ReadBytes(ACCEL_AAF_DELT...);
    // 这里做修正：读取 Bank 2 的配置并确保 Bit 0 为 0
    if (icm_read_reg(dev, 2, ICM42688_ACCEL_CONFIG_STATIC2, &reg_val) != ICM_OK) return ICM_ERROR;
    reg_val &= ~0x01; // Clear Bit 0 to enable AAF
    if (icm_write_reg(dev, 2, ICM42688_ACCEL_CONFIG_STATIC2, reg_val) != ICM_OK) return ICM_ERROR;

    // 11. 最终开启 Power Management (Bank 0)
    pwr_mgmt = PWR_MGMT0_TEMP_ENABLED | PWR_MGMT0_GYRO_LN | PWR_MGMT0_ACCEL_LN;
    if (icm_write_reg(dev, 0, ICM42688_PWR_MGMT0, pwr_mgmt) != ICM_OK) return ICM_ERROR;

    dev->delay(50); // 等待传感器稳定
    return ICM_OK;
}
uint8_t ICM42688_ReadFIFO(ICM42688_t *dev, uint8_t *buffer){
    uint16_t fifo_packets;
    if(icm_read_regs(dev,0,ICM42688_FIFO_COUNTH,(uint8_t*)(&fifo_packets),2) != ICM_OK) return ICM_ERROR;
    if(fifo_packets >= 2048 / 16) return 0;
    if(icm_read_regs(dev,0,ICM42688_FIFO_DATA,buffer,(dev->FIFO_THS)*16)) return ICM_ERROR; //这里我们是16字节数据包
    
}
/* 读取传感器数据 (查询方式,16位分辨率) */
uint8_t ICM42688_ReadData(ICM42688_t *dev, float *acc, float *gyro, float *temp) {
    uint8_t raw_data[14]; // Temp(2) + Accel(6) + Gyro(6)
    int16_t raw_acc[3];
    int16_t raw_gyro[3];
    int16_t raw_temp;

    // 数据寄存器从 0x1D (TEMP_DATA1) 开始
    if (icm_read_regs(dev, 0, ICM42688_TEMP_DATA1, raw_data, 14) != ICM_OK) return ICM_ERROR;

    raw_temp    = (int16_t)(raw_data[0] << 8 | raw_data[1]);
    raw_acc[0]  = (int16_t)(raw_data[2] << 8 | raw_data[3]);
    raw_acc[1]  = (int16_t)(raw_data[4] << 8 | raw_data[5]);
    raw_acc[2]  = (int16_t)(raw_data[6] << 8 | raw_data[7]);
    raw_gyro[0] = (int16_t)(raw_data[8] << 8 | raw_data[9]);
    raw_gyro[1] = (int16_t)(raw_data[10] << 8 | raw_data[11]);
    raw_gyro[2] = (int16_t)(raw_data[12] << 8 | raw_data[13]);

    // 转换物理量
    if (acc) {
        acc[0] = raw_acc[0] * dev->acc_sensitivity * 9.80665f;
        acc[1] = raw_acc[1] * dev->acc_sensitivity * 9.80665f;
        acc[2] = raw_acc[2] * dev->acc_sensitivity * 9.80665f;
    }
    if (gyro) {
        gyro[0] = raw_gyro[0] * dev->gyro_sensitivity;
        gyro[1] = raw_gyro[1] * dev->gyro_sensitivity;
        gyro[2] = raw_gyro[2] * dev->gyro_sensitivity;
    }
    if (temp) {
        *temp = (raw_temp / 132.48f) + 25.0f; // 典型公式
    }

    return ICM_OK;
}


