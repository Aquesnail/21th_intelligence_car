#include "icm42688.h"
#include "debug_arch.h"
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
/* 辅助函数：写特定 Bank 的寄存器（带读取校验） */
static uint8_t icm_write_reg(ICM42688_t *dev, uint8_t bank, uint8_t reg, uint8_t value) {
    uint8_t read_val;
    uint8_t ret;

    ret = icm_set_bank(dev, bank);
    if (ret != ICM_OK) return ret;

    if (dev->write(reg, value) != 0) return ICM_ERROR;

    // 读取校验：读回刚写入的值，注意读寄存器需置位最高位
    if (dev->read_byte(reg | 0x80, &read_val) != 0) return ICM_ERROR;
    if (read_val != value) {
        return 0x00 | reg;   // 返回带寄存器地址的错误码
    }
    return ICM_OK;
}

/* 辅助函数：读特定 Bank 的寄存器 */
static uint8_t icm_read_regs(ICM42688_t *dev, uint8_t bank, uint8_t reg, uint8_t *dat, uint16_t len) {
    if (icm_set_bank(dev, bank) != ICM_OK) return ICM_ERROR;
   // if (len == 1 && dev->read_byte != NULL) {
   //     return dev->read_byte(reg, dat) == 0 ? ICM_OK : ICM_ERROR;
   // } else {
    reg|=0x80;
        return dev->read(reg, dat, len) == 0 ? ICM_OK : ICM_ERROR;
   // }
}

static uint8_t icm_read_reg(ICM42688_t *dev, uint8_t bank, uint8_t reg, uint8_t *dat) {
    if (icm_set_bank(dev, bank) != ICM_OK) return ICM_ERROR;
   // if (len == 1 && dev->read_byte != NULL) {
   reg|=0x80;
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
    uint8_t bank = is_accel ? 2 : 1;
    uint8_t reg_base = is_accel ? ICM42688_ACCEL_CONFIG_STATIC2 : ICM42688_GYRO_CONFIG_STATIC3;
    uint8_t ret;
    uint8_t deltsqr_low = deltsqr & 0xFF;
    uint8_t deltsqr_high = (deltsqr >> 8) & 0x0F;
    
    ret = icm_write_reg(dev, bank, reg_base, delt << 1);
    if (ret != ICM_OK) return ret;
    ret = icm_write_reg(dev, bank, (uint8_t)(reg_base + 1), deltsqr_low);
    if (ret != ICM_OK) return ret;
    ret = icm_write_reg(dev, bank, (uint8_t)(reg_base + 2), (uint8_t)(deltsqr_high | (bitshift << 4)));
    return ret;
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
    uint8_t ret;   // 用于暂存返回值

    const uint8_t accel_delt = 2;
    const uint16_t accel_delt_sqr = 4;
    const uint8_t accel_bitshift = 13;
    
    if (!dev || !dev->write || !dev->read || !dev->read_byte || !dev->delay) return ICM_ERROR;

    if (dev->FIFO_THS == 0) dev->FIFO_THS = 48;
    dev->current_bank = 0xFF; // 强制首次切换
    dev->delay(10); // 上电等待

    // 2. 检查 ID
    ret = icm_read_reg(dev, 0, ICM42688_WHO_AM_I, &chip_id);
    if (ret != ICM_OK) return ret;
    Debug_Log("WHO_I_AM %d \r\n", chip_id);
    if (chip_id != 0x47) return ICM_ERROR;
    
    // 1. 软复位
    ret = icm_write_reg(dev, 0, ICM42688_DEVICE_CONFIG, 0x01);
   // if (ret != ICM_OK) return ret;
    dev->delay(10);

    
    
    // 3. 配置 Gyro
    gyro_cfg = GYRO_FS_SEL_500DPS | GYRO_ODR_500HZ;
    ret = icm_write_reg(dev, 0, ICM42688_GYRO_CONFIG0, gyro_cfg);
    if (ret != ICM_OK) return ret;

    // 4. 配置 Accel
    accel_cfg = ACCEL_FS_SEL_4G | ACCEL_ODR_500HZ;
    ret = icm_write_reg(dev, 0, ICM42688_ACCEL_CONFIG0, accel_cfg);
    if (ret != ICM_OK) return ret;

    update_sensitivity(dev, accel_cfg, gyro_cfg);

    // 5. 配置 FIFO
    ret = icm_write_reg(dev, 0, ICM42688_FIFO_CONFIG, FIFO_MODE_STREAM_TO_FIFO);
    if (ret != ICM_OK) return ret;

    fifo_cfg1 = FIFO_GYRO_EN_ENABLE 
              | FIFO_ACCEL_EN_ENABLE
              | FIFO_TEMP_EN_ENABLE  
              | FIFO_WM_GT_TH_DISABLE
              | FIFO_HIRES_EN_DISABLE 
              | FIFO_TMST_FSYNC_EN_ENABLE
              | FIFO_RESUME_PARTIAL_RD_DISABLE;
    ret = icm_write_reg(dev, 0, ICM42688_FIFO_CONFIG1, fifo_cfg1);
    if (ret != ICM_OK) return ret;

    ret = icm_write_reg(dev, 0, ICM42688_TMST_CONFIG,
                        TMST_EN_ENABLE | 
                        TMST_RES_1US |
                        TMST_DELTA_EN_DISABLE |
                        TMST_FSYNC_EN_DISABLE |
                        TMST_TO_REGS_EN_DISABLE);
    if (ret != ICM_OK) return ret;

    // FIFO Watermark
    ret = icm_write_reg(dev, 0, ICM42688_FIFO_CONFIG2, (uint8_t)(dev->FIFO_THS));
    if (ret != ICM_OK) return ret;
    ret = icm_write_reg(dev, 0, ICM42688_FIFO_CONFIG3, (uint8_t)((dev->FIFO_THS)>>8));
    if (ret != ICM_OK) return ret;

    // 6. 接口配置
    intf_cfg0 = FIFO_COUNT_BIG_ENDIAN | SENSOR_DATA_BIG_ENDIAN | FIFO_COUNT_IN_RECORDS;
    ret = icm_write_reg(dev, 0, ICM42688_INTF_CONFIG0, intf_cfg0);
    if (ret != ICM_OK) return ret;

    // 7. 中断配置
    ret = icm_write_reg(dev, 0, ICM42688_INT_SOURCE0, FIFO_THS_INT1_EN_ENABLE);
    if (ret != ICM_OK) return ret;
    ret = icm_write_reg(dev, 0, ICM42688_INT_CONFIG0, FIFO_THS_INT_CLEAR_ON_BOTH_READS);
    if (ret != ICM_OK) return ret;

    int_cfg = INT1_MODE_PULSED | INT2_MODE_PULSED | 
              INT1_DRIVE_CIRCUIT_OPEN_DRAIN | INT2_DRIVE_CIRCUIT_OPEN_DRAIN |
              INT1_POLARITY_ACTIVE_HIGH | INT2_POLARITY_ACTIVE_HIGH;
    ret = icm_write_reg(dev, 0, ICM42688_INT_CONFIG, int_cfg);
    if (ret != ICM_OK) return ret;

    ret = icm_write_reg(dev, 0, ICM42688_INT_CONFIG1, 0x00);
    if (ret != ICM_OK) return ret;

    // 8. UI 滤波器配置
    ret = icm_write_reg(dev, 0, ICM42688_GYRO_CONFIG1, GYRO_UI_FILT_ORD_2ND | GYRO_DEC2_M2_ORD_3RD);
    if (ret != ICM_OK) return ret;
    ret = icm_write_reg(dev, 0, ICM42688_ACCEL_CONFIG1, ACCEL_UI_FILT_ORD_2ND | ACCEL_DEC2_M2_ORD_3RD);
    if (ret != ICM_OK) return ret;
    ret = icm_write_reg(dev, 0, ICM42688_GYRO_ACCEL_CONFIG0, 
                        GYRO_UI_FILT_BW_LOW_LATENCY_400HZ | ACCEL_UI_FILT_BW_LOW_LATENCY_400HZ);
    if (ret != ICM_OK) return ret;

    // 9. AAF 设置
    ret = configure_aaf(dev, 1, accel_delt, accel_delt_sqr, accel_bitshift);
    if (ret != ICM_OK) return ret;
    ret = configure_aaf(dev, 0, accel_delt, accel_delt_sqr, accel_bitshift);
    if (ret != ICM_OK) return ret;

    // Enable Gyro AAF (Bank 1)
    ret = icm_write_reg(dev, 1, ICM42688_GYRO_CONFIG_STATIC2, 0x01);
    if (ret != ICM_OK) return ret;

    // Enable Accel AAF (Bank 2)
    ret = icm_read_reg(dev, 2, ICM42688_ACCEL_CONFIG_STATIC2, &reg_val);
    if (ret != ICM_OK) return ret;
    reg_val &= ~0x01;
    ret = icm_write_reg(dev, 2, ICM42688_ACCEL_CONFIG_STATIC2, reg_val);
    if (ret != ICM_OK) return ret;

    // 10. 开启电源管理
    pwr_mgmt = PWR_MGMT0_TEMP_ENABLED | PWR_MGMT0_GYRO_LN | PWR_MGMT0_ACCEL_LN;
    ret = icm_write_reg(dev, 0, ICM42688_PWR_MGMT0, pwr_mgmt);
    if (ret != ICM_OK) return ret;

    dev->delay(50);
    return ICM_OK;
}
uint8_t ICM42688_ReadFIFO(ICM42688_t *dev, uint8_t *buffer){
    uint16_t fifo_packets;
    if(icm_read_regs(dev,0,ICM42688_FIFO_COUNTH,(uint8_t*)(&fifo_packets),2) != ICM_OK) return ICM_ERROR;
    if(fifo_packets >= 128){
        dev->write(ICM42688_REG_BANK_SEL,0x00);
        dev->write(ICM42688_SIGNAL_PATH_RESET,FIFO_FLUSH_ENABLE);
        return fifo_packets;
    }
    if(icm_read_regs(dev,0,ICM42688_FIFO_DATA,buffer,(dev->FIFO_THS)*16)) return ICM_ERROR; //这里我们是16字节数据包
		return 0;
    
}
/* 读取传感器数据 (查询方式,16位分辨率) */
uint8_t xdata raw_data[14];
uint8_t ICM42688_ReadData(ICM42688_t *dev, float *acc, float *gyro, float *temp) {
     // Temp(2) + Accel(6) + Gyro(6)
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


