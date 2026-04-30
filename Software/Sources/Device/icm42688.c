#include "icm42688.h"
#include "stdio.h"
#include "string.h"

/* =======================================================
 * 外部变量定义（在imu_port.c中定义）
 * ======================================================= */
extern float   icm_acc_sensitivity;
extern float   icm_gyro_sensitivity;
extern uint8_t icm_current_bank;
extern uint16_t icm_fifo_ths;

/* =======================================================
 * 底层硬件包装函数
 * ======================================================= */
static uint8_t hw_write_reg(uint8_t reg, uint8_t value) {
    ICM42688_CS_RESET();
    ICM42688_SPI_TxRx(reg);
    ICM42688_SPI_TxRx(value);
    ICM42688_CS_SET();
    return 0;
}

static uint8_t hw_read_reg(uint8_t reg, uint8_t *value) {
    ICM42688_CS_RESET();
    ICM42688_SPI_TxRx(reg | 0x80);
    *value = ICM42688_SPI_TxRx(0xFF);
    ICM42688_CS_SET();
    return 0;
}

static uint8_t hw_read_regs_dma_trig(uint8_t reg, uint8_t *dat, uint16_t len) {
    ICM42688_CS_RESET();
    ICM42688_SPI_TxRx(reg | 0x80);
    ICM42688_SPI_DmaRead(dat, len);
    ICM42688_CS_SET();
    return 0;
}

/* 切换 Bank */
static uint8_t icm_set_bank(uint8_t bank) {
    if (icm_current_bank == bank) return ICM_OK;
    if (hw_write_reg(ICM42688_REG_BANK_SEL, bank) != 0) return ICM_ERROR;
    icm_current_bank = bank;
    return ICM_OK;
}

/* 写特定 Bank 的寄存器（带读取校验） */
static uint8_t icm_write_reg(uint8_t bank, uint8_t reg, uint8_t value) {
    uint8_t read_val;
    uint8_t ret;

    ret = icm_set_bank(bank);
    if (ret != ICM_OK) return ret;

    if (hw_write_reg(reg, value) != 0) return ICM_ERROR;

    /* 读取校验 */
    if (hw_read_reg(reg, &read_val) != 0) return ICM_ERROR;
    if (read_val != value) return 0x00 | reg;

    return ICM_OK;
}

/* 连续读特定 Bank 的寄存器 */
static uint8_t icm_read_regs(uint8_t bank, uint8_t reg, uint8_t *dat, uint16_t len) {
    if (icm_set_bank(bank) != ICM_OK) return ICM_ERROR;
    return hw_read_regs_dma_trig(reg, dat, len) == 0 ? ICM_OK : ICM_ERROR;
}

/* 单读特定 Bank 的寄存器 */
static uint8_t icm_read_reg(uint8_t bank, uint8_t reg, uint8_t *dat) {
    if (icm_set_bank(bank) != ICM_OK) return ICM_ERROR;
    return hw_read_reg(reg, dat) == 0 ? ICM_OK : ICM_ERROR;
}

/* 更新灵敏度 */
static void update_sensitivity(uint8_t a_scale, uint8_t g_scale) {
    switch(a_scale & ACCEL_FS_SEL_MASK) {
        case ACCEL_FS_SEL_16G: icm_acc_sensitivity = 16.0f / 32768.0f; break;
        case ACCEL_FS_SEL_8G:  icm_acc_sensitivity = 8.0f  / 32768.0f; break;
        case ACCEL_FS_SEL_4G:  icm_acc_sensitivity = 4.0f  / 32768.0f; break;
        case ACCEL_FS_SEL_2G:  icm_acc_sensitivity = 2.0f  / 32768.0f; break;
        default:               icm_acc_sensitivity = 16.0f / 32768.0f; break;
    }

    switch(g_scale & GYRO_FS_SEL_MASK) {
        case GYRO_FS_SEL_2000DPS: icm_gyro_sensitivity = 2000.0f / 32768.0f; break;
        case GYRO_FS_SEL_1000DPS: icm_gyro_sensitivity = 1000.0f / 32768.0f; break;
        case GYRO_FS_SEL_500DPS:  icm_gyro_sensitivity = 500.0f  / 32768.0f; break;
        case GYRO_FS_SEL_250DPS:  icm_gyro_sensitivity = 250.0f  / 32768.0f; break;
        default:                  icm_gyro_sensitivity = 2000.0f / 32768.0f; break;
    }
}

/* AAF 设置 */
static uint8_t configure_aaf(uint8_t is_accel, uint8_t delt, uint16_t deltsqr, uint8_t bitshift) {
    uint8_t bank = is_accel ? 2 : 1;
    uint8_t reg_base = is_accel ? ICM42688_ACCEL_CONFIG_STATIC2 : ICM42688_GYRO_CONFIG_STATIC3;
    uint8_t ret;
    uint8_t deltsqr_low = deltsqr & 0xFF;
    uint8_t deltsqr_high = (deltsqr >> 8) & 0x0F;
    uint8_t delt_val = is_accel ? (delt << 1) : delt;

    ret = icm_write_reg(bank, reg_base, delt_val);
    if (ret != ICM_OK) return ret;
    ret = icm_write_reg(bank, (uint8_t)(reg_base + 1), deltsqr_low);
    if (ret != ICM_OK) return ret;
    ret = icm_write_reg(bank, (uint8_t)(reg_base + 2), (uint8_t)(deltsqr_high | (bitshift << 4)));
    return ret;
}

/* =======================================================
 * 初始化
 * ======================================================= */
uint8_t ICM42688_Init(void) {
    uint8_t chip_id = 0;
    uint8_t gyro_cfg;
    uint8_t accel_cfg;
    uint8_t fifo_cfg1;
    uint8_t intf_cfg0;
    uint8_t int_cfg;
    uint8_t pwr_mgmt;
    uint8_t reg_val;
    uint8_t tmst_cfg;
    uint8_t ret;

    const uint8_t accel_delt = 2;
    const uint16_t accel_delt_sqr = 4;
    const uint8_t accel_bitshift = 13;

    if (icm_fifo_ths == 0) icm_fifo_ths = 48;

    icm_current_bank = 0xFF;
    icm_set_bank(0);
    ICM42688_DelayMs(10);

    /* 软复位 */
    hw_write_reg(ICM42688_DEVICE_CONFIG, 0x01);
    ICM42688_DelayMs(2);

    ret = icm_read_reg(0, ICM42688_WHO_AM_I, &chip_id);
    if (ret != ICM_OK) return ret;
    while(chip_id != 0x47){
        ICM42688_DelayMs(3);
        ret = icm_read_reg(0, ICM42688_WHO_AM_I, &chip_id);
    }
    ICM42688_Print("WHO_I_AM %d \r\n", chip_id&0xFF);
    if (chip_id != 0x47) return ICM_ERROR;

    pwr_mgmt = PWR_MGMT0_TEMP_ENABLED | PWR_MGMT0_GYRO_LN | PWR_MGMT0_ACCEL_LN;
    ret = icm_write_reg(0, ICM42688_PWR_MGMT0, 0x0F);
    if (ret != ICM_OK) return ret;

    gyro_cfg = GYRO_FS_SEL_2000DPS | GYRO_ODR_1KHZ;
    ret = icm_write_reg(0, ICM42688_GYRO_CONFIG0, gyro_cfg);
    if (ret != ICM_OK) return ret;

    accel_cfg = ACCEL_FS_SEL_4G | ACCEL_ODR_1KHZ;
    ret = icm_write_reg(0, ICM42688_ACCEL_CONFIG0, accel_cfg);
    if (ret != ICM_OK) return ret;

    update_sensitivity(accel_cfg, gyro_cfg);

    ret = icm_write_reg(0, ICM42688_GYRO_CONFIG1, GYRO_UI_FILT_ORD_2ND | GYRO_DEC2_M2_ORD_3RD);
    if (ret != ICM_OK) return ret;
    ret = icm_write_reg(0, ICM42688_ACCEL_CONFIG1, ACCEL_UI_FILT_ORD_2ND | ACCEL_DEC2_M2_ORD_3RD);
    if (ret != ICM_OK) return ret;
    ret = icm_write_reg(0, ICM42688_GYRO_ACCEL_CONFIG0,
                        GYRO_UI_FILT_BW_LOW_LATENCY_400HZ | ACCEL_UI_FILT_BW_LOW_LATENCY_400HZ);
    if (ret != ICM_OK) return ret;

    ret = configure_aaf(1, accel_delt, accel_delt_sqr, accel_bitshift);
    if (ret != ICM_OK) return ret;
    ret = configure_aaf(0, accel_delt, accel_delt_sqr, accel_bitshift);
    if (ret != ICM_OK) return ret;

    ret = icm_read_reg(1, ICM42688_GYRO_CONFIG_STATIC2, &reg_val);
    if (ret != ICM_OK) return ret;
    reg_val &= ~0x02;
    reg_val |= 0x01;
    ret = icm_write_reg(1, ICM42688_GYRO_CONFIG_STATIC2, reg_val);

    ret = icm_read_reg(2, ICM42688_ACCEL_CONFIG_STATIC2, &reg_val);
    if (ret != ICM_OK) return ret;
    reg_val &= ~0x01;
    ret = icm_write_reg(2, ICM42688_ACCEL_CONFIG_STATIC2, reg_val);
    if (ret != ICM_OK) return ret;

    icm_set_bank(0);
    ICM42688_DelayMs(50);

    tmst_cfg = TMST_EN_ENABLE
             | TMST_RES_1US
             | TMST_DELTA_EN_ENABLE
             | TMST_FSYNC_EN_DISABLE
             | TMST_TO_REGS_EN_DISABLE
             | 0x20;
    icm_write_reg(0, ICM42688_TMST_CONFIG, tmst_cfg);
    if (ret != ICM_OK) return ret;

    intf_cfg0 = FIFO_COUNT_BIG_ENDIAN | SENSOR_DATA_BIG_ENDIAN | FIFO_COUNT_IN_RECORDS | 0x03;
    ret = icm_write_reg(0, ICM42688_INTF_CONFIG0, intf_cfg0);
    if (ret != ICM_OK) return ret;

    fifo_cfg1 = FIFO_GYRO_EN_ENABLE
              | FIFO_ACCEL_EN_ENABLE
              | FIFO_TEMP_EN_ENABLE
              | FIFO_WM_GT_TH_ENABLE
              | FIFO_HIRES_EN_DISABLE
              | FIFO_TMST_FSYNC_EN_ENABLE
              | FIFO_RESUME_PARTIAL_RD_DISABLE;

    ret = icm_write_reg(0, ICM42688_FIFO_CONFIG1, fifo_cfg1);
    if (ret != ICM_OK) return ret;

    ret = icm_write_reg(0, ICM42688_FIFO_CONFIG2, (uint8_t)(icm_fifo_ths));
    if (ret != ICM_OK) return ret;
    ret = icm_write_reg(0, ICM42688_FIFO_CONFIG3, (uint8_t)((icm_fifo_ths) >> 8));
    if (ret != ICM_OK) return ret;

    ret = icm_write_reg(0, ICM42688_INT_CONFIG0, FIFO_THS_INT_CLEAR_ON_BOTH_READS);
    if (ret != ICM_OK) return ret;
    ret = icm_write_reg(0, ICM42688_INT_SOURCE0, FIFO_THS_INT1_EN_ENABLE);
    if (ret != ICM_OK) return ret;

    int_cfg = INT1_MODE_PULSED | INT2_MODE_PULSED |
              INT1_DRIVE_CIRCUIT_OPEN_DRAIN | INT2_DRIVE_CIRCUIT_OPEN_DRAIN |
              INT1_POLARITY_ACTIVE_HIGH | INT2_POLARITY_ACTIVE_LOW;
    ret = icm_write_reg(0, ICM42688_INT_CONFIG, int_cfg);
    if (ret != ICM_OK) return ret;

    ret = icm_write_reg(0, ICM42688_INT_CONFIG1, 0x00);
    if (ret != ICM_OK) return ret;

    hw_write_reg(ICM42688_SIGNAL_PATH_RESET, 0x02);
    ICM42688_DelayMs(2);

    ret = icm_write_reg(0, ICM42688_FIFO_CONFIG, FIFO_MODE_STOP_ON_FULL);
    if (ret != ICM_OK) return ret;

    return ICM_OK;
}

/* =======================================================
 * FIFO读取与数据处理
 * ======================================================= */
uint16_t ICM42688_ReadFIFO(uint8_t *buffer) {
    if (icm_read_regs(0, ICM42688_FIFO_DATA, buffer, icm_fifo_ths * 16) != ICM_OK) {
        return 0;
    }
    return 0;
}

void ICM42688_FIFO_Data_Handle(ICM42688_Pack16_t *dat, uint8_t *ptr){
    int16_t raw_acc_x, raw_acc_y, raw_acc_z;
    int16_t raw_gyro_x, raw_gyro_y, raw_gyro_z;
    uint8_t raw_temp;
    uint16_t raw_timestamp;

    raw_acc_x = (int16_t)((ptr[1] << 8) | ptr[2]);
    raw_acc_y = (int16_t)((ptr[3] << 8) | ptr[4]);
    raw_acc_z = (int16_t)((ptr[5] << 8) | ptr[6]);
    raw_gyro_x = (int16_t)((ptr[7] << 8) | ptr[8]);
    raw_gyro_y = (int16_t)((ptr[9] << 8) | ptr[10]);
    raw_gyro_z = (int16_t)((ptr[11] << 8) | ptr[12]);
    raw_temp = ptr[13];
    raw_timestamp = (uint16_t)((ptr[14] << 8) | ptr[15]);

    /* 直接访问全局灵敏度变量，无dev->间接调用 */
    dat->sensor.dat.ax = raw_acc_x * icm_acc_sensitivity * 9.80665f;
    dat->sensor.dat.ay = raw_acc_y * icm_acc_sensitivity * 9.80665f;
    dat->sensor.dat.az = raw_acc_z * icm_acc_sensitivity * 9.80665f;

    dat->sensor.dat.gx = raw_gyro_x * icm_gyro_sensitivity * 0.01745f;
    dat->sensor.dat.gy = raw_gyro_y * icm_gyro_sensitivity * 0.01745f;
    dat->sensor.dat.gz = raw_gyro_z * icm_gyro_sensitivity * 0.01745f;

    dat->timestamp = raw_timestamp;
    dat->sensor.dat.sample_time = (float)(raw_timestamp) / 1000000.0f;
    dat->sensor.dat.temp = (raw_temp / 2.07f) + 25.0f;
}

/* =======================================================
 * 直接读取传感器数据
 * ======================================================= */
static uint8_t xdata raw_data[14] = {0};

uint8_t ICM42688_ReadData(float *acc, float *gyro, float *temp) {
    int16_t raw_acc[3];
    int16_t raw_gyro[3];
    int16_t raw_temp;

    if (icm_read_regs(0, ICM42688_TEMP_DATA1, raw_data, 14) != ICM_OK) return ICM_ERROR;

    raw_temp    = (int16_t)(raw_data[0] << 8 | raw_data[1]);
    raw_acc[0]  = (int16_t)(raw_data[2] << 8 | raw_data[3]);
    raw_acc[1]  = (int16_t)(raw_data[4] << 8 | raw_data[5]);
    raw_acc[2]  = (int16_t)(raw_data[6] << 8 | raw_data[7]);
    raw_gyro[0] = (int16_t)(raw_data[8] << 8 | raw_data[9]);
    raw_gyro[1] = (int16_t)(raw_data[10] << 8 | raw_data[11]);
    raw_gyro[2] = (int16_t)(raw_data[12] << 8 | raw_data[13]);

    if (acc) {
        acc[0] = raw_acc[0] * icm_acc_sensitivity * 9.80665f;
        acc[1] = raw_acc[1] * icm_acc_sensitivity * 9.80665f;
        acc[2] = raw_acc[2] * icm_acc_sensitivity * 9.80665f;
    }
    if (gyro) {
        gyro[0] = raw_gyro[0] * icm_gyro_sensitivity;
        gyro[1] = raw_gyro[1] * icm_gyro_sensitivity;
        gyro[2] = raw_gyro[2] * icm_gyro_sensitivity;
    }
    if (temp) {
        *temp = (raw_temp / 132.48f) + 25.0f;
    }

    return ICM_OK;
}
