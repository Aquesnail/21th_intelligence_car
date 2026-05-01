
#ifndef _ICM42688_DEFS_H
#define _ICM42688_DEFS_H

/*Register Map*/

// USER BANK 0 REGISTER MAP
#define ICM42688_DEVICE_CONFIG        0x11 // 17
#define ICM42688_DRIVE_CONFIG         0x13 // 19
#define ICM42688_INT_CONFIG           0x14 // 20
#define ICM42688_FIFO_CONFIG          0x16 // 22
#define ICM42688_TEMP_DATA1           0x1D // 29
#define ICM42688_TEMP_DATA0           0x1E // 30
#define ICM42688_ACCEL_DATA_X1        0x1F // 31
#define ICM42688_ACCEL_DATA_X0        0x20 // 32
#define ICM42688_ACCEL_DATA_Y1        0x21 // 33
#define ICM42688_ACCEL_DATA_Y0        0x22 // 34
#define ICM42688_ACCEL_DATA_Z1        0x23 // 35
#define ICM42688_ACCEL_DATA_Z0        0x24 // 36
#define ICM42688_GYRO_DATA_X1         0x25 // 37
#define ICM42688_GYRO_DATA_X0         0x26 // 38
#define ICM42688_GYRO_DATA_Y1         0x27 // 39
#define ICM42688_GYRO_DATA_Y0         0x28 // 40
#define ICM42688_GYRO_DATA_Z1         0x29 // 41
#define ICM42688_GYRO_DATA_Z0         0x2A // 42
#define ICM42688_TMST_FSYNCH          0x2B // 43
#define ICM42688_TMST_FSYNCL          0x2C // 44
#define ICM42688_INT_STATUS           0x2D // 45
#define ICM42688_FIFO_COUNTH          0x2E // 46
#define ICM42688_FIFO_COUNTL          0x2F // 47
#define ICM42688_FIFO_DATA            0x30 // 48
#define ICM42688_APEX_DATA0           0x31 // 49
#define ICM42688_APEX_DATA1           0x32 // 50
#define ICM42688_APEX_DATA2           0x33 // 51
#define ICM42688_APEX_DATA3           0x34 // 52
#define ICM42688_APEX_DATA4           0x35 // 53
#define ICM42688_APEX_DATA5           0x36 // 54
#define ICM42688_INT_STATUS2          0x37 // 55
#define ICM42688_INT_STATUS3          0x38 // 56
#define ICM42688_SIGNAL_PATH_RESET    0x4B // 75
#define ICM42688_INTF_CONFIG0         0x4C // 76
#define ICM42688_INTF_CONFIG1         0x4D // 77
#define ICM42688_PWR_MGMT0            0x4E // 78
#define ICM42688_GYRO_CONFIG0         0x4F // 79
#define ICM42688_ACCEL_CONFIG0        0x50 // 80
#define ICM42688_GYRO_CONFIG1         0x51 // 81
#define ICM42688_GYRO_ACCEL_CONFIG0   0x52 // 82
#define ICM42688_ACCEL_CONFIG1        0x53 // 83
#define ICM42688_TMST_CONFIG          0x54 // 84
#define ICM42688_APEX_CONFIG0         0x56 // 86
#define ICM42688_SMD_CONFIG           0x57 // 87
#define ICM42688_FIFO_CONFIG1         0x5F // 95
#define ICM42688_FIFO_CONFIG2         0x60 // 96
#define ICM42688_FIFO_CONFIG3         0x61 // 97
#define ICM42688_FSYNC_CONFIG         0x62 // 98
#define ICM42688_INT_CONFIG0          0x63 // 99
#define ICM42688_INT_CONFIG1          0x64 // 100
#define ICM42688_INT_SOURCE0          0x65 // 101
#define ICM42688_INT_SOURCE1          0x66 // 102
#define ICM42688_INT_SOURCE3          0x68 // 104
#define ICM42688_INT_SOURCE4          0x69 // 105
#define ICM42688_FIFO_LOST_PKT0       0x6C // 108
#define ICM42688_FIFO_LOST_PKT1       0x6D // 109
#define ICM42688_SELF_TEST_CONFIG     0x70 // 112
#define ICM42688_WHO_AM_I             0x75 // 117
#define ICM42688_REG_BANK_SEL         0x76 // 118

// USER BANK 1 REGISTER MAP
#define ICM42688_SENSOR_CONFIG0       0x03 // 3
#define ICM42688_GYRO_CONFIG_STATIC2  0x0B // 11
#define ICM42688_GYRO_CONFIG_STATIC3  0x0C // 12
#define ICM42688_GYRO_CONFIG_STATIC4  0x0D // 13
#define ICM42688_GYRO_CONFIG_STATIC5  0x0E // 14
#define ICM42688_GYRO_CONFIG_STATIC6  0x0F // 15
#define ICM42688_GYRO_CONFIG_STATIC7  0x10 // 16
#define ICM42688_GYRO_CONFIG_STATIC8  0x11 // 17
#define ICM42688_GYRO_CONFIG_STATIC9  0x12 // 18
#define ICM42688_GYRO_CONFIG_STATIC10 0x13 // 19
#define ICM42688_XG_ST_DATA           0x5F // 95
#define ICM42688_YG_ST_DATA           0x60 // 96
#define ICM42688_ZG_ST_DATA           0x61 // 97
#define ICM42688_TMSTVAL0             0x62 // 98
#define ICM42688_TMSTVAL1             0x63 // 99
#define ICM42688_TMSTVAL2             0x64 // 100
#define ICM42688_INTF_CONFIG4         0x7A // 122
#define ICM42688_INTF_CONFIG5         0x7B // 123
#define ICM42688_INTF_CONFIG6         0x7C // 124

// USER BANK 2 REGISTER MAP
#define ICM42688_ACCEL_CONFIG_STATIC2 0x03 // 3
#define ICM42688_ACCEL_CONFIG_STATIC3 0x04 // 4
#define ICM42688_ACCEL_CONFIG_STATIC4 0x05 // 5
#define ICM42688_XA_ST_DATA           0x3B // 59
#define ICM42688_YA_ST_DATA           0x3C // 60
#define ICM42688_ZA_ST_DATA           0x3D // 61

// USER BANK 4 REGISTER MAP
#define ICM42688_APEX_CONFIG1         0x40 // 64
#define ICM42688_APEX_CONFIG2         0x41 // 65
#define ICM42688_APEX_CONFIG3         0x42 // 66
#define ICM42688_APEX_CONFIG4         0x43 // 67
#define ICM42688_APEX_CONFIG5         0x44 // 68
#define ICM42688_APEX_CONFIG6         0x45 // 69
#define ICM42688_APEX_CONFIG7         0x46 // 70
#define ICM42688_APEX_CONFIG8         0x47 // 71
#define ICM42688_APEX_CONFIG9         0x48 // 72
#define ICM42688_ACCEL_WOM_X_THR      0x4A // 74
#define ICM42688_ACCEL_WOM_Y_THR      0x4B // 75
#define ICM42688_ACCEL_WOM_Z_THR      0x4C // 76
#define ICM42688_INT_SOURCE6          0x4D // 77
#define ICM42688_INT_SOURCE7          0x4E // 78
#define ICM42688_INT_SOURCE8          0x4F // 79
#define ICM42688_INT_SOURCE9          0x50 // 80
#define ICM42688_INT_SOURCE10         0x51 // 81
#define ICM42688_OFFSET_USER0         0x77 // 119
#define ICM42688_OFFSET_USER1         0x78 // 120
#define ICM42688_OFFSET_USER2         0x79 // 121
#define ICM42688_OFFSET_USER3         0x7A // 122
#define ICM42688_OFFSET_USER4         0x7B // 123
#define ICM42688_OFFSET_USER5         0x7C // 124
#define ICM42688_OFFSET_USER6         0x7D // 125
#define ICM42688_OFFSET_USER7         0x7E // 126
#define ICM42688_OFFSET_USER8         0x7F // 127



/*
UI_FILT
*/


/*****************************INT_CONFIG*****************************/

#define INT_CONFIG_ADDR 0x20 // (14h)

// INT_CONFIG 默认复位值
#define INT_CONFIG_RESET_VALUE 0x00

// INT2_MODE 字段掩码
#define INT2_MODE_MASK (0x20) // 0b00100000 (bit 5)

// INT2_MODE 值定义
#define INT2_MODE_PULSED (0x00 << 5) // 脉冲模式
#define INT2_MODE_LATCHED (0x01 << 5) // 锁存模式

// INT2_DRIVE_CIRCUIT 字段掩码
#define INT2_DRIVE_CIRCUIT_MASK (0x10) // 0b00010000 (bit 4)

// INT2_DRIVE_CIRCUIT 值定义
#define INT2_DRIVE_CIRCUIT_OPEN_DRAIN (0x00 << 4) // 开漏驱动
#define INT2_DRIVE_CIRCUIT_PUSH_PULL (0x01 << 4) // 推挽驱动

// INT2_POLARITY 字段掩码
#define INT2_POLARITY_MASK (0x08) // 0b00001000 (bit 3)

// INT2_POLARITY 值定义
#define INT2_POLARITY_ACTIVE_LOW (0x00 << 3) // 活动低电平（默认）
#define INT2_POLARITY_ACTIVE_HIGH (0x01 << 3) // 活动高电平

// INT1_MODE 字段掩码
#define INT1_MODE_MASK (0x04) // 0b00000100 (bit 2)

// INT1_MODE 值定义
#define INT1_MODE_PULSED (0x00 << 2) // 脉冲模式
#define INT1_MODE_LATCHED (0x01 << 2) // 锁存模式

// INT1_DRIVE_CIRCUIT 字段掩码
#define INT1_DRIVE_CIRCUIT_MASK (0x02) // 0b00000010 (bit 1)

// INT1_DRIVE_CIRCUIT 值定义
#define INT1_DRIVE_CIRCUIT_OPEN_DRAIN (0x00 << 1) // 开漏驱动
#define INT1_DRIVE_CIRCUIT_PUSH_PULL (0x01 << 1) // 推挽驱动

// INT1_POLARITY 字段掩码
#define INT1_POLARITY_MASK (0x01) // 0b00000001 (bit 0)

// INT1_POLARITY 值定义
#define INT1_POLARITY_ACTIVE_LOW (0x00) // 活动低电平（默认）
#define INT1_POLARITY_ACTIVE_HIGH (0x01) // 活动高电平



/**************************FIFO_CONFIG***********************/

#define FIFO_CONFIG_ADDR 0x22 // (16h)

// FIFO_CONFIG 默认复位值
#define FIFO_CONFIG_RESET_VALUE 0x00

// FIFO_MODE 字段掩码
#define FIFO_MODE_MASK (0xC0) // 0b11000000 (bits 7:6)

// FIFO_MODE 值定义
#define FIFO_MODE_BYPASS (0x00 << 6) // Bypass Mode (默认)
#define FIFO_MODE_STREAM_TO_FIFO (0x01 << 6) // Stream-to-FIFO Mode
#define FIFO_MODE_STOP_ON_FULL (0x02 << 6) // STOP-on-FULL Mode



/*********************************GYRO_CONFIG0******************************/
#define GYRO_FS_SEL_MASK 0xE0 // 7:5 bits mask for GYRO_FS_SEL （掩码）

// Full scale select for gyroscope UI interface output
#define GYRO_FS_SEL_2000DPS  0x00 // 000: ±2000 dps (default)
#define GYRO_FS_SEL_1000DPS  0x20 // 001: ±1000 dps
#define GYRO_FS_SEL_500DPS   0x40 // 010: ±500 dps
#define GYRO_FS_SEL_250DPS   0x60 // 011: ±250 dps
#define GYRO_FS_SEL_125DPS   0x80 // 100: ±125 dps
#define GYRO_FS_SEL_62_5DPS  0xA0 // 101: ±62.5 dps
#define GYRO_FS_SEL_31_25DPS 0xC0 // 110: ±31.25 dps
#define GYRO_FS_SEL_15_625DPS 0xE0 // 111: ±15.625 dps

#define GYRO_ODR_MASK 0x0F // 3:0 bits mask for GYRO_ODR

// Gyroscope ODR selection for UI interface output

#define GYRO_ODR_32KHZ       0x01 // 0001: 32kHz
#define GYRO_ODR_16KHZ       0x02 // 0010: 16kHz
#define GYRO_ODR_8KHZ        0x03 // 0011: 8kHz
#define GYRO_ODR_4KHZ        0x04 // 0100: 4kHz
#define GYRO_ODR_2KHZ        0x05 // 0101: 2kHz
#define GYRO_ODR_1KHZ        0x06 // 0110: 1kHz (default)
#define GYRO_ODR_200HZ       0x07 // 0111: 200Hz
#define GYRO_ODR_100HZ       0x08 // 1000: 100Hz
#define GYRO_ODR_50HZ        0x09 // 1001: 50Hz
#define GYRO_ODR_25HZ        0x0A // 1010: 25Hz
#define GYRO_ODR_12_5HZ      0x0B // 1011: 12.5Hz
#define GYRO_ODR_500HZ       0x0F // 1111: 500Hz




/************************ ACCEL_CONFIG0 ************************/
#define ACCEL_FS_SEL_MASK 0xE0 // 7:5 bits mask for ACCEL_FS_SEL

// Full scale select for accelerometer UI interface output
#define ACCEL_FS_SEL_16G 0x00 // 000: ±16g (default)
#define ACCEL_FS_SEL_8G  0x20 // 001: ±8g
#define ACCEL_FS_SEL_4G  0x40 // 010: ±4g
#define ACCEL_FS_SEL_2G  0x60 // 011: ±2g
#define ACCEL_FS_SEL_RESERVED_0 0x80 // 100: Reserved
#define ACCEL_FS_SEL_RESERVED_1 0xA0 // 101: Reserved
#define ACCEL_FS_SEL_RESERVED_2 0xC0 // 110: Reserved
#define ACCEL_FS_SEL_RESERVED_3 0xE0 // 111: Reserved

#define ACCEL_ODR_MASK 0x0F // 3:0 bits mask for ACCEL_ODR

// Accelerometer ODR selection for UI interface output
#define ACCEL_ODR_RESERVED_0  0x00 // 0000: Reserved
#define ACCEL_ODR_32KHZ       0x01 // 0001: 32kHz (LN mode)
#define ACCEL_ODR_16KHZ       0x02 // 0010: 16kHz (LN mode)
#define ACCEL_ODR_8KHZ        0x03 // 0011: 8kHz (LN mode)
#define ACCEL_ODR_4KHZ        0x04 // 0100: 4kHz (LN mode)
#define ACCEL_ODR_2KHZ        0x05 // 0101: 2kHz (LN mode)
#define ACCEL_ODR_1KHZ        0x06 // 0110: 1kHz (LN mode) (default)
#define ACCEL_ODR_200HZ       0x07 // 0111: 200Hz (LP or LN mode)
#define ACCEL_ODR_100HZ       0x08 // 1000: 100Hz (LP or LN mode)
#define ACCEL_ODR_50HZ        0x09 // 1001: 50Hz (LP or LN mode)
#define ACCEL_ODR_25HZ        0x0A // 1010: 25Hz (LP or LN mode)
#define ACCEL_ODR_12_5HZ      0x0B // 1011: 12.5Hz (LP or LN mode)
#define ACCEL_ODR_6_25HZ      0x0C // 1100: 6.25Hz (LP mode)
#define ACCEL_ODR_3_125HZ     0x0D // 1101: 3.125Hz (LP mode)
#define ACCEL_ODR_1_5625HZ    0x0E // 1110: 1.5625Hz (LP mode)
#define ACCEL_ODR_500HZ       0x0F // 1111: 500Hz (LP or LN mode)




/************************ GYRO_CONFIG1 ************************/
// GYRO_CONFIG1 默认复位值
#define GYRO_CONFIG1_RESET_VALUE 0x16

// TEMP_FILT_BW 字段掩码
#define TEMP_FILT_BW_MASK   (0xE0) // 0b11100000 (bits 7:5)

// TEMP_FILT_BW 值定义
#define TEMP_FILT_BW_4000HZ (0x00) // DLPF BW = 4000Hz, Latency = 0.125ms (default)
#define TEMP_FILT_BW_170HZ  (0x20) // DLPF BW = 170Hz, Latency = 1ms
#define TEMP_FILT_BW_82HZ   (0x40) // DLPF BW = 82Hz, Latency = 2ms
#define TEMP_FILT_BW_40HZ   (0x60) // DLPF BW = 40Hz, Latency = 4ms
#define TEMP_FILT_BW_20HZ   (0x80) // DLPF BW = 20Hz, Latency = 8ms
#define TEMP_FILT_BW_10HZ   (0xA0) // DLPF BW = 10Hz, Latency = 16ms
#define TEMP_FILT_BW_5HZ    (0xC0) // DLPF BW = 5Hz, Latency = 32ms

#define GYRO_UI_FILT_ORD_MASK   (0x0C) // 0b00001100 (bits 3:2)
#define GYRO_UI_FILT_ORD_1ST    (0x00 << 2) // 1st Order
#define GYRO_UI_FILT_ORD_2ND    (0x01 << 2) // 2nd Order -> 即 0x04
#define GYRO_UI_FILT_ORD_3RD    (0x02 << 2) // 3rd Order -> 即 0x08

#define GYRO_DEC2_M2_ORD_MASK   (0x03) // 0b00000011 (bits 1:0)
#define GYRO_DEC2_M2_ORD_3RD    (0x02) // 3rd Order -> 根据手册 10 代表 3rd order




/***********************************ACCEL_CONFIG1************************************/
// ACCEL_CONFIG1 默认复位值
#define ACCEL_CONFIG1_RESET_VALUE 0x0D

// ACCEL_UI_FILT_ORD 字段掩码
#define ACCEL_UI_FILT_ORD_MASK   (0x18) // 0b00011000 (bits 4:3)

// ACCEL_UI_FILT_ORD 值定义
#define ACCEL_UI_FILT_ORD_1ST    (0x00) // 1st Order
#define ACCEL_UI_FILT_ORD_2ND    (0x08) // 2nd Order
#define ACCEL_UI_FILT_ORD_3RD    (0x10) // 3rd Order

// ACCEL_DEC2_M2_ORD 字段掩码
#define ACCEL_DEC2_M2_ORD_MASK   (0x06) // 0b00000110 (bits 2:1)

// ACCEL_DEC2_M2_ORD 值定义
#define ACCEL_DEC2_M2_ORD_3RD    (0x04) // 3rd Order




/********************************GYRO_ACCEL_CONFIG0**********************************/
// GYRO_ACCEL_CONFIG0 默认复位值
#define GYRO_ACCEL_CONFIG0_RESET_VALUE 0x11

// ACCEL_UI_FILT_BW 字段掩码
#define ACCEL_UI_FILT_BW_MASK (0xF0) // 0b11110000 (bits 7:4)

// ACCEL_UI_FILT_BW 值定义（LN Mode）
#define ACCEL_UI_FILT_BW_ODR_DIV_2      (0x00 << 4) // BW = ODR/2
#define ACCEL_UI_FILT_BW_MAX_400HZ_DIV_4 (0x01 << 4) // BW = max(400Hz, ODR)/4 (default)
#define ACCEL_UI_FILT_BW_MAX_400HZ_DIV_5 (0x02 << 4) // BW = max(400Hz, ODR)/5
#define ACCEL_UI_FILT_BW_MAX_400HZ_DIV_8 (0x03 << 4) // BW = max(400Hz, ODR)/8
#define ACCEL_UI_FILT_BW_MAX_400HZ_DIV_10 (0x04 << 4) // BW = max(400Hz, ODR)/10
#define ACCEL_UI_FILT_BW_MAX_400HZ_DIV_16 (0x05 << 4) // BW = max(400Hz, ODR)/16
#define ACCEL_UI_FILT_BW_MAX_400HZ_DIV_20 (0x06 << 4) // BW = max(400Hz, ODR)/20
#define ACCEL_UI_FILT_BW_MAX_400HZ_DIV_40 (0x07 << 4) // BW = max(400Hz, ODR)/40

// ACCEL_UI_FILT_BW 值定义（Low Latency Option）
#define ACCEL_UI_FILT_BW_LOW_LATENCY_400HZ (0x0E << 4) // Trivial decimation @ ODR of Dec2 filter output. Dec2 runs at max(400Hz, ODR)
#define ACCEL_UI_FILT_BW_LOW_LATENCY_200HZ (0x0F << 4) // Trivial decimation @ ODR of Dec2 filter output. Dec2 runs at max(200Hz, 8*ODR)

// ACCEL_UI_FILT_BW 值定义（LP Mode）
#define ACCEL_UI_FILT_BW_LP_1X_AVG (0x01 << 4) // 1x AVG filter (default)
#define ACCEL_UI_FILT_BW_LP_16X_AVG (0x06 << 4) // 16x AVG filter

// GYRO_UI_FILT_BW 字段掩码
#define GYRO_UI_FILT_BW_MASK (0x0F) // 0b00001111 (bits 3:0)

// GYRO_UI_FILT_BW 值定义（LN Mode）
#define GYRO_UI_FILT_BW_ODR_DIV_2      (0x00) // BW = ODR/2
#define GYRO_UI_FILT_BW_MAX_400HZ_DIV_4 (0x01) // BW = max(400Hz, ODR)/4 (default)
#define GYRO_UI_FILT_BW_MAX_400HZ_DIV_5 (0x02) // BW = max(400Hz, ODR)/5
#define GYRO_UI_FILT_BW_MAX_400HZ_DIV_8 (0x03) // BW = max(400Hz, ODR)/8
#define GYRO_UI_FILT_BW_MAX_400HZ_DIV_10 (0x04) // BW = max(400Hz, ODR)/10
#define GYRO_UI_FILT_BW_MAX_400HZ_DIV_16 (0x05) // BW = max(400Hz, ODR)/16
#define GYRO_UI_FILT_BW_MAX_400HZ_DIV_20 (0x06) // BW = max(400Hz, ODR)/20
#define GYRO_UI_FILT_BW_MAX_400HZ_DIV_40 (0x07) // BW = max(400Hz, ODR)/40

// GYRO_UI_FILT_BW 值定义（Low Latency Option）
#define GYRO_UI_FILT_BW_LOW_LATENCY_400HZ (0x0E) // Trivial decimation @ ODR of Dec2 filter output. Dec2 runs at max(400Hz, ODR)
#define GYRO_UI_FILT_BW_LOW_LATENCY_200HZ (0x0F) // Trivial decimation @ ODR of Dec2 filter output. Dec2 runs at max(200Hz, 8*ODR)




/****************************************TMST_CONGIG************************************ */

// TMST_CONFIG 默认复位值
#define TMST_CONFIG_RESET_VALUE 0x23

// TMST_TO_REGS_EN 字段掩码
#define TMST_TO_REGS_EN_MASK (0x10) // 0b00010000 (bit 4)

// TMST_TO_REGS_EN 值定义
#define TMST_TO_REGS_EN_DISABLE (0x00) // TMST_VALUE[19:0] 读取时总是返回 0
#define TMST_TO_REGS_EN_ENABLE  (0x10) // TMST_VALUE[19:0] 读取时返回时间戳值

// TMST_RES 字段掩码
#define TMST_RES_MASK (0x08) // 0b00001000 (bit 3)

// TMST_RES 值定义
#define TMST_RES_1US   (0x00) // 时间戳分辨率为 1 ?s（默认）
#define TMST_RES_16US  (0x08) // 时间戳分辨率为 16 ?s

// TMST_DELTA_EN 字段掩码
#define TMST_DELTA_EN_MASK (0x04) // 0b00000100 (bit 2)

// TMST_DELTA_EN 值定义
#define TMST_DELTA_EN_DISABLE (0x00) // 禁用时间戳增量模式
#define TMST_DELTA_EN_ENABLE  (0x04) // 启用时间戳增量模式

// TMST_FSYNC_EN 字段掩码
#define TMST_FSYNC_EN_MASK (0x02) // 0b00000010 (bit 1)

// TMST_FSYNC_EN 值定义
#define TMST_FSYNC_EN_DISABLE (0x00) // 禁用 FSYNC 时间戳功能
#define TMST_FSYNC_EN_ENABLE  (0x02) // 启用 FSYNC 时间戳功能

// TMST_EN 字段掩码
#define TMST_EN_MASK (0x01) // 0b00000001 (bit 0)

// TMST_EN 值定义
#define TMST_EN_DISABLE (0x00) // 禁用时间戳寄存器
#define TMST_EN_ENABLE  (0x01) // 启用时间戳寄存器（默认）




/**********************************FIFO_CONFIG1***************************************/

// FIFO_CONFIG1 默认复位值
#define FIFO_CONFIG1_RESET_VALUE 0x00

// FIFO_RESUME_PARTIAL_RD 字段掩码
#define FIFO_RESUME_PARTIAL_RD_MASK (0x40) // 0b01000000 (bit 6)

// FIFO_RESUME_PARTIAL_RD 值定义
#define FIFO_RESUME_PARTIAL_RD_DISABLE (0x00) // 禁用部分 FIFO 读取，需要重新读取整个 FIFO
#define FIFO_RESUME_PARTIAL_RD_ENABLE  (0x40) // 启用部分 FIFO 读取，从上次读取点继续

// FIFO_WM_GT_TH 字段掩码
#define FIFO_WM_GT_TH_MASK (0x20) // 0b00100000 (bit 5)

// FIFO_WM_GT_TH 值定义
#define FIFO_WM_GT_TH_DISABLE (0x00) // 禁用水印中断触发
#define FIFO_WM_GT_TH_ENABLE  (0x20) // 每次 ODR（DMA 写入）时，如果 FIFO_COUNT >= FIFO_WM_TH，则触发水印中断

// FIFO_HIRES_EN 字段掩码
#define FIFO_HIRES_EN_MASK (0x10) // 0b00010000 (bit 4)

// FIFO_HIRES_EN 值定义
#define FIFO_HIRES_EN_DISABLE (0x00) // 禁用高分辨率数据存储到 FIFO
#define FIFO_HIRES_EN_ENABLE  (0x10) // 启用高分辨率数据存储到 FIFO

// FIFO_TMST_FSYNC_EN 字段掩码
#define FIFO_TMST_FSYNC_EN_MASK (0x08) // 0b00001000 (bit 3)

// FIFO_TMST_FSYNC_EN 值定义
#define FIFO_TMST_FSYNC_EN_DISABLE (0x00) // 禁用 FSYNC 时间戳存储到 FIFO
#define FIFO_TMST_FSYNC_EN_ENABLE  (0x08) // 启用 FSYNC 时间戳存储到 FIFO

// FIFO_TEMP_EN 字段掩码
#define FIFO_TEMP_EN_MASK (0x04) // 0b00000100 (bit 2)

// FIFO_TEMP_EN 值定义
#define FIFO_TEMP_EN_DISABLE (0x00) // 禁用温度传感器数据存储到 FIFO
#define FIFO_TEMP_EN_ENABLE  (0x04) // 启用温度传感器数据存储到 FIFO

// FIFO_GYRO_EN 字段掩码
#define FIFO_GYRO_EN_MASK (0x02) // 0b00000010 (bit 1)

// FIFO_GYRO_EN 值定义
#define FIFO_GYRO_EN_DISABLE (0x00) // 禁用陀螺仪数据存储到 FIFO
#define FIFO_GYRO_EN_ENABLE  (0x02) // 启用陀螺仪数据存储到 FIFO

// FIFO_ACCEL_EN 字段掩码
#define FIFO_ACCEL_EN_MASK (0x01) // 0b00000001 (bit 0)

// FIFO_ACCEL_EN 值定义
#define FIFO_ACCEL_EN_DISABLE (0x00) // 禁用加速度计数据存储到 FIFO
#define FIFO_ACCEL_EN_ENABLE  (0x01) // 启用加速度计数据存储到 FIFO




/***************************************FIFO_WATERMARK(FIFO_CONFIG2,FIFO_CONFIG3))*****************************************/

// FIFO_CONFIG2 默认复位值
#define FIFO_CONFIG2_RESET_VALUE 0x00

// FIFO_WM[7:0] 字段掩码
#define FIFO_WM_LOWER_MASK (0xFF) // 0b11111111 (bits 7:0)

// FIFO_CONFIG3 默认复位值
#define FIFO_CONFIG3_RESET_VALUE 0x00

// FIFO_WM[11:8] 字段掩码
#define FIFO_WM_UPPER_MASK (0x0F) // 0b00001111 (bits 3:0)



/***********************************************INTF_CONFIG0***************************************************/

// INTF_CONFIG0 默认复位值
#define INTF_CONFIG0_RESET_VALUE 0x30

// FIFO_HOLD_LAST_DATA_EN 字段掩码
#define FIFO_HOLD_LAST_DATA_EN_MASK (0x80) // 0b10000000 (bit 7)

// FIFO_HOLD_LAST_DATA_EN 值定义
#define FIFO_HOLD_LAST_DATA_EN_DISABLE (0x00) // 禁用 FIFO 保留最后一个有效样本
#define FIFO_HOLD_LAST_DATA_EN_ENABLE  (0x80) // 启用 FIFO 保留最后一个有效样本

// FIFO_COUNT_REC 字段掩码
#define FIFO_COUNT_REC_MASK (0x40) // 0b01000000 (bit 6) 

// FIFO_COUNT_REC 值定义
#define FIFO_COUNT_IN_BYTES   (0x00) // FIFO 计数以字节为单位报告
#define FIFO_COUNT_IN_RECORDS (0x40) // FIFO 计数以记录为单位报告（1 记录 = 16 字节或更多）

// FIFO_COUNT_ENDIAN 字段掩码
#define FIFO_COUNT_ENDIAN_MASK (0x20) // 0b00100000 (bit 5)

// FIFO_COUNT_ENDIAN 值定义
#define FIFO_COUNT_LITTLE_ENDIAN (0x00) // FIFO 计数以小端格式报告
#define FIFO_COUNT_BIG_ENDIAN    (0x20) // FIFO 计数以大端格式报告（默认）

// SENSOR_DATA_ENDIAN 字段掩码
#define SENSOR_DATA_ENDIAN_MASK (0x10) // 0b00010000 (bit 4)

// SENSOR_DATA_ENDIAN 值定义
#define SENSOR_DATA_LITTLE_ENDIAN (0x00) // 传感器数据以小端格式报告
#define SENSOR_DATA_BIG_ENDIAN    (0x10) // 传感器数据以大端格式报告（默认）

// UI_SIFS_CFG 字段掩码
#define UI_SIFS_CFG_MASK (0x03) // 0b00000011 (bits 1:0)

// UI_SIFS_CFG 值定义
#define UI_SIFS_CFG_DISABLE_SPI (0x02) // 禁用 SPI
#define UI_SIFS_CFG_DISABLE_I2C (0x03) // 禁用 I2C




/***************************************SIGNAL_PATH_RESET*******************************/

// SIGNAL_PATH_RESET 默认复位值
#define SIGNAL_PATH_RESET_RESET_VALUE 0x00

// DMP_INIT_EN 字段掩码
#define DMP_INIT_EN_MASK (0x40) // 0b01000000 (bit 6)

// DMP_INIT_EN 值定义
#define DMP_INIT_EN_DISABLE (0x00) // 禁用 DMP
#define DMP_INIT_EN_ENABLE  (0x40) // 启用 DMP

// DMP_MEM_RESET_EN 字段掩码
#define DMP_MEM_RESET_EN_MASK (0x20) // 0b00100000 (bit 5)

// DMP_MEM_RESET_EN 值定义
#define DMP_MEM_RESET_EN_DISABLE (0x00) // 不重置 DMP 内存
#define DMP_MEM_RESET_EN_ENABLE  (0x20) // 重置 DMP 内存

// ABORT_AND_RESET 字段掩码
#define ABORT_AND_RESET_MASK (0x08) // 0b00001000 (bit 3)

// ABORT_AND_RESET 值定义
#define ABORT_AND_RESET_DISABLE (0x00) // 不重置信号路径
#define ABORT_AND_RESET_ENABLE  (0x08) // 重置信号路径

// TMST_STROBE 字段掩码
#define TMST_STROBE_MASK (0x04) // 0b00000100 (bit 2)

// TMST_STROBE 值定义
#define TMST_STROBE_CLEAR (0x00) // 清除时间戳寄存器
#define TMST_STROBE_LATCH (0x04) // 将时间戳计数器锁存到时间戳寄存器

// FIFO_FLUSH 字段掩码
#define FIFO_FLUSH_MASK (0x02) // 0b00000010 (bit 1)

// FIFO_FLUSH 值定义
#define FIFO_FLUSH_DISABLE (0x00) // 不清除 FIFO
#define FIFO_FLUSH_ENABLE  (0x02) // 清除 FIFO




/******************************************INT_STATUS*******************************************/

// INT_STATUS 默认复位值
#define INT_STATUS_RESET_VALUE 0x10

// UI_FSYNC_INT 字段掩码
#define UI_FSYNC_INT_MASK (0x40) // 0b01000000 (bit 6)

// UI_FSYNC_INT 值定义
#define UI_FSYNC_INT_NOT_SET (0x00) // 没有生成 UI FSYNC 中断
#define UI_FSYNC_INT_SET     (0x40) // 已生成 UI FSYNC 中断

// PLL_RDY_INT 字段掩码
#define PLL_RDY_INT_MASK (0x20) // 0b00100000 (bit 5)

// PLL_RDY_INT 值定义
#define PLL_RDY_INT_NOT_SET (0x00) // 没有生成 PLL Ready 中断
#define PLL_RDY_INT_SET     (0x20) // 已生成 PLL Ready 中断

// RESET_DONE_INT 字段掩码
#define RESET_DONE_INT_MASK (0x10) // 0b00010000 (bit 4)

// RESET_DONE_INT 值定义
#define RESET_DONE_INT_NOT_SET (0x00) // 软件复位未完成
#define RESET_DONE_INT_SET     (0x10) // 软件复位已完成

// DATA_RDY_INT 字段掩码
#define DATA_RDY_INT_MASK (0x08) // 0b00001000 (bit 3)

// DATA_RDY_INT 值定义
#define DATA_RDY_INT_NOT_SET (0x00) // 没有生成 Data Ready 中断
#define DATA_RDY_INT_SET     (0x08) // 已生成 Data Ready 中断

// FIFO_THS_INT 字段掩码
#define FIFO_THS_INT_MASK (0x04) // 0b00000100 (bit 2)

// FIFO_THS_INT 值定义
#define FIFO_THS_INT_NOT_SET (0x00) // FIFO 缓冲区未达到阈值
#define FIFO_THS_INT_SET     (0x04) // FIFO 缓冲区已达到阈值

// FIFO_FULL_INT 字段掩码
#define FIFO_FULL_INT_MASK (0x02) // 0b00000010 (bit 1)

// FIFO_FULL_INT 值定义
#define FIFO_FULL_INT_NOT_SET (0x00) // FIFO 缓冲区未满
#define FIFO_FULL_INT_SET     (0x02) // FIFO 缓冲区已满

// AGC_RDY_INT 字段掩码
#define AGC_RDY_INT_MASK (0x01) // 0b00000001 (bit 0)

// AGC_RDY_INT 值定义
#define AGC_RDY_INT_NOT_SET (0x00) // 没有生成 AGC Ready 中断
#define AGC_RDY_INT_SET     (0x01) // 已生成 AGC Ready 中断




/**************************************************INT_CONFIG0***********************************************/

// INT_CONFIG0 默认复位值
#define INT_CONFIG0_RESET_VALUE 0x00

// UI_DRDY_INT_CLEAR 字段掩码
#define UI_DRDY_INT_CLEAR_MASK (0x30) // 0b00110000 (bits 5:4)

// UI_DRDY_INT_CLEAR 值定义
#define UI_DRDY_INT_CLEAR_ON_STATUS_READ_ONLY       (0x00 << 4) // 清除仅在状态位读取时 (默认)
#define UI_DRDY_INT_CLEAR_ON_STATUS_READ            (0x01 << 4) // 清除在状态位读取时
#define UI_DRDY_INT_CLEAR_ON_SENSOR_REGISTER_READ   (0x02 << 4) // 清除在传感器寄存器读取时
#define UI_DRDY_INT_CLEAR_ON_BOTH_READS             (0x03 << 4) // 清除在状态位和传感器寄存器读取时

// FIFO_THS_INT_CLEAR 字段掩码
#define FIFO_THS_INT_CLEAR_MASK (0x0C) // 0b00001100 (bits 3:2)

// FIFO_THS_INT_CLEAR 值定义
#define FIFO_THS_INT_CLEAR_ON_STATUS_READ_ONLY      (0x00 << 2) // 清除仅在状态位读取时 (默认),只有中断为latched mode起作用
#define FIFO_THS_INT_CLEAR_ON_STATUS_READ           (0x01 << 2) // 清除在状态位读取时,只有中断为latched mode起作用
#define FIFO_THS_INT_CLEAR_ON_FIFO_DATA_1BYTE_READ  (0x02 << 2) // 清除在 FIFO 数据 1 字节读取时,只有中断为latched mode起作用
#define FIFO_THS_INT_CLEAR_ON_BOTH_READS            (0x03 << 2) // 清除在状态位和 FIFO 数据 1 字节读取时,只有中断为latched mode起作用

// FIFO_FULL_INT_CLEAR 字段掩码
#define FIFO_FULL_INT_CLEAR_MASK (0x03) // 0b00000011 (bits 1:0)

// FIFO_FULL_INT_CLEAR 值定义
#define FIFO_FULL_INT_CLEAR_ON_STATUS_READ_ONLY     (0x00) // 清除仅在状态位读取时 (默认)
#define FIFO_FULL_INT_CLEAR_ON_STATUS_READ          (0x01) // 清除在状态位读取时
#define FIFO_FULL_INT_CLEAR_ON_FIFO_DATA_1BYTE_READ (0x02) // 清除在 FIFO 数据 1 字节读取时
#define FIFO_FULL_INT_CLEAR_ON_BOTH_READS           (0x03) // 清除在状态位和 FIFO 数据 1 字节读取时

/******************************************INT_SOURCE0******************************************/

// INT_SOURCE0 默认复位值
#define INT_SOURCE0_RESET_VALUE 0x10

// UI_FSYNC_INT1_EN 字段掩码
#define UI_FSYNC_INT1_EN_MASK (0x40) // 0b01000000 (bit 6)

// UI_FSYNC_INT1_EN 值定义
#define UI_FSYNC_INT1_EN_DISABLE (0x00) // 禁用：UI FSYNC 中断不路由到 INT1
#define UI_FSYNC_INT1_EN_ENABLE  (0x40) // 启用：UI FSYNC 中断路由到 INT1

// PLL_RDY_INT1_EN 字段掩码
#define PLL_RDY_INT1_EN_MASK (0x20) // 0b00100000 (bit 5)

// PLL_RDY_INT1_EN 值定义
#define PLL_RDY_INT1_EN_DISABLE (0x00) // 禁用：PLL Ready 中断不路由到 INT1
#define PLL_RDY_INT1_EN_ENABLE  (0x20) // 启用：PLL Ready 中断路由到 INT1

// RESET_DONE_INT1_EN 字段掩码
#define RESET_DONE_INT1_EN_MASK (0x10) // 0b00010000 (bit 4)

// RESET_DONE_INT1_EN 值定义
#define RESET_DONE_INT1_EN_DISABLE (0x00) // 禁用：Reset Done 中断不路由到 INT1
#define RESET_DONE_INT1_EN_ENABLE  (0x10) // 启用：Reset Done 中断路由到 INT1

// UI_DRDY_INT1_EN 字段掩码
#define UI_DRDY_INT1_EN_MASK (0x08) // 0b00001000 (bit 3)

// UI_DRDY_INT1_EN 值定义
#define UI_DRDY_INT1_EN_DISABLE (0x00) // 禁用：UI Data Ready 中断不路由到 INT1
#define UI_DRDY_INT1_EN_ENABLE  (0x08) // 启用：UI Data Ready 中断路由到 INT1

// FIFO_THS_INT1_EN 字段掩码
#define FIFO_THS_INT1_EN_MASK (0x04) // 0b00000100 (bit 2)

// FIFO_THS_INT1_EN 值定义
#define FIFO_THS_INT1_EN_DISABLE (0x00) // 禁用：FIFO Threshold 中断不路由到 INT1
#define FIFO_THS_INT1_EN_ENABLE  (0x04) // 启用：FIFO Threshold 中断路由到 INT1

// FIFO_FULL_INT1_EN 字段掩码
#define FIFO_FULL_INT1_EN_MASK (0x02) // 0b00000010 (bit 1)

// FIFO_FULL_INT1_EN 值定义
#define FIFO_FULL_INT1_EN_DISABLE (0x00) // 禁用：FIFO Full 中断不路由到 INT1
#define FIFO_FULL_INT1_EN_ENABLE  (0x02) // 启用：FIFO Full 中断路由到 INT1

// UI_AGC_RDY_INT1_EN 字段掩码
#define UI_AGC_RDY_INT1_EN_MASK (0x01) // 0b00000001 (bit 0)

// UI_AGC_RDY_INT1_EN 值定义
#define UI_AGC_RDY_INT1_EN_DISABLE (0x00) // 禁用：UI AGC Ready 中断不路由到 INT1
#define UI_AGC_RDY_INT1_EN_ENABLE  (0x01) // 启用：UI AGC Ready 中断路由到 INT1

/***************************INT_SOURCE3************************** */

// INT_SOURCE3 默认复位值
#define INT_SOURCE3_RESET_VALUE 0x00

// UI_FSYNC_INT2_EN 字段掩码
#define UI_FSYNC_INT2_EN_MASK (0x40) // 0b01000000 (bit 6)

// UI_FSYNC_INT2_EN 值定义
#define UI_FSYNC_INT2_EN_DISABLE (0x00) // 禁用：UI FSYNC 中断不路由到 INT2
#define UI_FSYNC_INT2_EN_ENABLE  (0x40) // 启用：UI FSYNC 中断路由到 INT2

// PLL_RDY_INT2_EN 字段掩码
#define PLL_RDY_INT2_EN_MASK (0x20) // 0b00100000 (bit 5)

// PLL_RDY_INT2_EN 值定义
#define PLL_RDY_INT2_EN_DISABLE (0x00) // 禁用：PLL Ready 中断不路由到 INT2
#define PLL_RDY_INT2_EN_ENABLE  (0x20) // 启用：PLL Ready 中断路由到 INT2

// RESET_DONE_INT2_EN 字段掩码
#define RESET_DONE_INT2_EN_MASK (0x10) // 0b00010000 (bit 4)

// RESET_DONE_INT2_EN 值定义
#define RESET_DONE_INT2_EN_DISABLE (0x00) // 禁用：Reset Done 中断不路由到 INT2
#define RESET_DONE_INT2_EN_ENABLE  (0x10) // 启用：Reset Done 中断路由到 INT2

// UI_DRDY_INT2_EN 字段掩码
#define UI_DRDY_INT2_EN_MASK (0x08) // 0b00001000 (bit 3)

// UI_DRDY_INT2_EN 值定义
#define UI_DRDY_INT2_EN_DISABLE (0x00) // 禁用：UI Data Ready 中断不路由到 INT2
#define UI_DRDY_INT2_EN_ENABLE  (0x08) // 启用：UI Data Ready 中断路由到 INT2

// FIFO_THS_INT2_EN 字段掩码
#define FIFO_THS_INT2_EN_MASK (0x04) // 0b00000100 (bit 2)

// FIFO_THS_INT2_EN 值定义
#define FIFO_THS_INT2_EN_DISABLE (0x00) // 禁用：FIFO Threshold 中断不路由到 INT2
#define FIFO_THS_INT2_EN_ENABLE  (0x04) // 启用：FIFO Threshold 中断路由到 INT2

// FIFO_FULL_INT2_EN 字段掩码
#define FIFO_FULL_INT2_EN_MASK (0x02) // 0b00000010 (bit 1)

// FIFO_FULL_INT2_EN 值定义
#define FIFO_FULL_INT2_EN_DISABLE (0x00) // 禁用：FIFO Full 中断不路由到 INT2
#define FIFO_FULL_INT2_EN_ENABLE  (0x02) // 启用：FIFO Full 中断路由到 INT2

// UI_AGC_RDY_INT2_EN 字段掩码
#define UI_AGC_RDY_INT2_EN_MASK (0x01) // 0b00000001 (bit 0)

// UI_AGC_RDY_INT2_EN 值定义
#define UI_AGC_RDY_INT2_EN_DISABLE (0x00) // 禁用：UI AGC Ready 中断不路由到 INT2
#define UI_AGC_RDY_INT2_EN_ENABLE  (0x01) // 启用：UI AGC Ready 中断路由到 INT2


/***************************INT_SOURCE4************************** */

// INT_SOURCE4 默认复位值
#define INT_SOURCE4_RESET_VALUE 0x00

// I3C_PROTOCOL_ERROR_IN_T2_EN 字段掩码
#define I3C_PROTOCOL_ERROR_IN_T2_EN_MASK (0x40) // 0b01000000 (bit 6)

// I3C_PROTOCOL_ERROR_IN_T2_EN 值定义
#define I3C_PROTOCOL_ERROR_IN_T2_EN_DISABLE (0x00) // 禁用：I3C协议错误中断不路由到 INT2
#define I3C_PROTOCOL_ERROR_IN_T2_EN_ENABLE  (0x40) // 启用：I3C协议错误中断路由到 INT2

// SMD_INT2_EN 字段掩码
#define SMD_INT2_EN_MASK (0x08) // 0b00001000 (bit 3)

// SMD_INT2_EN 值定义
#define SMD_INT2_EN_DISABLE (0x00) // 禁用：SMD中断不路由到 INT2
#define SMD_INT2_EN_ENABLE  (0x08) // 启用：SMD中断路由到 INT2

// WOM_Z_INT2_EN 字段掩码
#define WOM_Z_INT2_EN_MASK (0x04) // 0b00000100 (bit 2)

// WOM_Z_INT2_EN 值定义
#define WOM_Z_INT2_EN_DISABLE (0x00) // 禁用：Z轴WOM中断不路由到 INT2
#define WOM_Z_INT2_EN_ENABLE  (0x04) // 启用：Z轴WOM中断路由到 INT2

// WOM_Y_INT2_EN 字段掩码
#define WOM_Y_INT2_EN_MASK (0x02) // 0b00000010 (bit 1)

// WOM_Y_INT2_EN 值定义
#define WOM_Y_INT2_EN_DISABLE (0x00) // 禁用：Y轴WOM中断不路由到 INT2
#define WOM_Y_INT2_EN_ENABLE  (0x02) // 启用：Y轴WOM中断路由到 INT2

// WOM_X_INT2_EN 字段掩码
#define WOM_X_INT2_EN_MASK (0x01) // 0b00000001 (bit 0)

// WOM_X_INT2_EN 值定义
#define WOM_X_INT2_EN_DISABLE (0x00) // 禁用：X轴WOM中断不路由到 INT2
#define WOM_X_INT2_EN_ENABLE  (0x01) // 启用：X轴WOM中断路由到 INT2


/*************************** SENSOR_CONFIG0 ************************/
#define SENSOR_CONFIG0_RESET_VALUE 0x80
#define SENSOR_CONFIG0_ZG_DISABLE (1 << 5) // 0: Z gyroscope is on, 1: Z gyroscope is disabled
#define SENSOR_CONFIG0_YG_DISABLE (1 << 4) // 0: Y gyroscope is on, 1: Y gyroscope is disabled
#define SENSOR_CONFIG0_XG_DISABLE (1 << 3) // 0: X gyroscope is on, 1: X gyroscope is disabled
#define SENSOR_CONFIG0_ZA_DISABLE (1 << 2) // 0: Z accelerometer is on, 1: Z accelerometer is disabled
#define SENSOR_CONFIG0_YA_DISABLE (1 << 1) // 0: Y accelerometer is on, 1: Y accelerometer is disabled
#define SENSOR_CONFIG0_XA_DISABLE (1 << 0) // 0: X accelerometer is on, 1: X accelerometer is disabled

/*************************** GYRO_CONFIG_STATIC2 ************************/
#define GYRO_CONFIG_STATIC2_RESET_VALUE 0xA0
#define GYRO_CONFIG_STATIC2_GYRO_AAF_DIS (1 << 1) // 0: Enable gyroscope anti-aliasing filter, 1: Disable
#define GYRO_CONFIG_STATIC2_GYRO_NF_DIS (1 << 0)  // 0: Enable Notch Filter, 1: Disable

/*************************** GYRO_CONFIG_STATIC3 ************************/
#define GYRO_CONFIG_STATIC3_RESET_VALUE 0x0D
#define GYRO_CONFIG_STATIC3_GYRO_AAF_DELT_MASK 0x3F // Controls bandwidth of the gyroscope anti-alias filter

/*************************** GYRO_CONFIG_STATIC4 ************************/
#define GYRO_CONFIG_STATIC4_RESET_VALUE 0xAA
#define GYRO_CONFIG_STATIC4_GYRO_AAF_DELTSQR_MASK 0xFF // Controls bandwidth of the gyroscope anti-alias filter

/*************************** GYRO_CONFIG_STATIC5 ************************/
#define GYRO_CONFIG_STATIC5_RESET_VALUE 0x80
#define GYRO_CONFIG_STATIC5_GYRO_AAF_BITSHIFT_MASK 0xF0 // Controls bandwidth of the gyroscope anti-alias filter
#define GYRO_CONFIG_STATIC5_GYRO_AAF_DELTSQR_HIGH_MASK 0x0F // Controls bandwidth of the gyroscope anti-alias filter

/*************************** GYRO_CONFIG_STATIC6 ************************/
//#define GYRO_CONFIG_STATIC6_RESET_VALUE 0xXX // Factory trimmed
#define GYRO_CONFIG_STATIC6_GYRO_X_NF_COSWZ_MASK 0xFF // Used for gyroscope X-axis notch filter frequency selection

/*************************** GYRO_CONFIG_STATIC7 ************************/
//#define GYRO_CONFIG_STATIC7_RESET_VALUE 0xXX // Factory trimmed
#define GYRO_CONFIG_STATIC7_GYRO_Y_NF_COSWZ_MASK 0xFF // Used for gyroscope Y-axis notch filter frequency selection

/*************************** GYRO_CONFIG_STATIC8 ************************/
//#define GYRO_CONFIG_STATIC8_RESET_VALUE 0xXX // Factory trimmed
#define GYRO_CONFIG_STATIC8_GYRO_Z_NF_COSWZ_MASK 0xFF // Used for gyroscope Z-axis notch filter frequency selection

/*************************** GYRO_CONFIG_STATIC9 ************************/
//#define GYRO_CONFIG_STATIC9_RESET_VALUE 0xXX // Factory trimmed
#define GYRO_CONFIG_STATIC9_GYRO_Z_NF_COSWZ_SEL_0 (1 << 5) // Used for gyroscope Z-axis notch filter frequency selection
#define GYRO_CONFIG_STATIC9_GYRO_Y_NF_COSWZ_SEL_0 (1 << 4) // Used for gyroscope Y-axis notch filter frequency selection
#define GYRO_CONFIG_STATIC9_GYRO_X_NF_COSWZ_SEL_0 (1 << 3) // Used for gyroscope X-axis notch filter frequency selection
#define GYRO_CONFIG_STATIC9_GYRO_Z_NF_COSWZ_8 (1 << 2)     // Used for gyroscope Z-axis notch filter frequency selection
#define GYRO_CONFIG_STATIC9_GYRO_Y_NF_COSWZ_8 (1 << 1)     // Used for gyroscope Y-axis notch filter frequency selection
#define GYRO_CONFIG_STATIC9_GYRO_X_NF_COSWZ_8 (1 << 0)     // Used for gyroscope X-axis notch filter frequency selection

/*************************** GYRO_CONFIG_STATIC10 ************************/
#define GYRO_CONFIG_STATIC10_RESET_VALUE 0x11
#define GYRO_CONFIG_STATIC10_GYRO_NF_BW_SEL_MASK 0x70 // Selects bandwidth for gyroscope notch filter


/***************************************PWR_MGMT0****************************************/

// 默认复位值
#define PWR_MGMT0_RESET_VALUE    0x00

// BIT NAME 宏定义
// Bits 7:6 - Reserved
#define PWR_MGMT0_TEMP_DIS_BIT   5       // 温度传感器使能位
#define PWR_MGMT0_IDLE_BIT       4       // IDLE 位
#define PWR_MGMT0_GYRO_MODE_BITS 3       // Gyro 模式控制位 (Bit 3:2)
#define PWR_MGMT0_ACCEL_MODE_BITS 1      // Accel 模式控制位 (Bit 1:0)

// TEMP_DIS 宏定义
#define PWR_MGMT0_TEMP_ENABLED   0x00    // 温度传感器启用
#define PWR_MGMT0_TEMP_DISABLED  (1 << PWR_MGMT0_TEMP_DIS_BIT) // 温度传感器禁用

// IDLE 宏定义
#define PWR_MGMT0_IDLE_OFF       0x00    // RC 振荡器随加速度计和陀螺仪关闭
#define PWR_MGMT0_IDLE_ON        (1 << PWR_MGMT0_IDLE_BIT)     // RC 振荡器始终保持开启

// GYRO_MODE 宏定义
#define PWR_MGMT0_GYRO_OFF       0x00    // 关闭陀螺仪
#define PWR_MGMT0_GYRO_STANDBY   (0x1 << PWR_MGMT0_GYRO_MODE_BITS) // Standby 模式
#define PWR_MGMT0_GYRO_RESERVED  (0x2 << PWR_MGMT0_GYRO_MODE_BITS) // 保留模式
#define PWR_MGMT0_GYRO_LN        (0x3 << PWR_MGMT0_GYRO_MODE_BITS) // Low Noise 模式

// ACCEL_MODE 宏定义
#define PWR_MGMT0_ACCEL_OFF      0x00    // 关闭加速度计
#define PWR_MGMT0_ACCEL_LP       (0x2 << PWR_MGMT0_ACCEL_MODE_BITS) // Low Power 模式
#define PWR_MGMT0_ACCEL_LN       (0x3 << PWR_MGMT0_ACCEL_MODE_BITS) // Low Noise 模式

// 过渡状态延迟时间
#define GYRO_TRANSITION_DELAY_US 200     // 陀螺仪模式切换时的延迟 (200?s)
#define ACCEL_TRANSITION_DELAY_US 200    // 加速度计模式切换时的延迟 (200?s)
#define GYRO_MIN_ON_TIME_MS      45      // 陀螺仪最小开启时间 (45ms)

#define PIN9_FUNCTION_INT2  0x00 << 1 //pin9作为int2
#define PIN9_FUNCTION_FSYNC 0x01 << 1
#define PIN9_FUNCTION_CLKIN 0x02 << 1

#endif