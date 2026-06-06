#ifndef FUZZY_PID_H
#define FUZZY_PID_H

/* 采样频率相关宏：500Hz固定频率，用乘法替代浮点除法 */
#define FUZZY_PID_SAMPLE_FREQ  500.0f   /* 采样频率(Hz)，用于 ec = Δerror * freq */
#define FUZZY_PID_DT           0.002f   /* 采样周期 = 1/500，用于积分累加 */

/* EC 移动平均滤波最大窗口 */
#define FUZZY_EC_MA_MAX        50

/* 模糊语言值枚举 (前缀防混用) */
typedef enum {
    FUZZY_NB = 0,
    FUZZY_NM = 1,
    FUZZY_NS = 2,
    FUZZY_ZO = 3,
    FUZZY_PS = 4,
    FUZZY_PM = 5,
    FUZZY_PB = 6
} Fuzzy_LV;

/* 模糊PID控制器结构体 */
typedef struct {
    /* ---------- 基础PID参数 ---------- */
    float Kp0, Ki0, Kd0;          /* 手动整定的基础值 */

    /* ---------- 积分分离 ---------- */
    float err_thresh;             /* 积分分离阈值，|error|>此值时积分清零 */
    float err_clear_thresh;       /* 低误差清零阈值，|error|<此值时积分清零，防止低频振荡 */
    float kd_err_thresh_l;        /* 角速度阻尼外阈值(左/负向)，|error|>此值时关闭gyro阻尼，用kd*ec */
    float kd_err_thresh_r;        /* 角速度阻尼外阈值(右/正向) */
    float kd_gyro_thresh_l;       /* 角速度阻尼内阈值(左/负向)，|error|<此值时用高阻尼系数 */
    float kd_gyro_thresh_r;       /* 角速度阻尼内阈值(右/正向) */
    /* ---------- 模糊域参数 ---------- */
    float e_max;                  /* 误差输入最大幅值 */
    float ec_max;                 /* 误差变化率最大幅值 */
    float ec_dead_zone;              /* 误差变化率死区，|ec|<此值视为0 */
    /* ---------- 输出单点(修正系数) ---------- */
    float kp_singletons[7];       /* NB,NM,NS,ZO,PS,PM,PB 对应的 Kp 系数 */
    float kd_singletons[7];       /* Kd 系数 */

    /* ---------- 规则表 (7行 e, 5列 ec) ---------- */
    int rule_kp[7][5];            /* Kp 修正系数的语言值索引 0~6 */
    int rule_kd[7][5];            /* Kd 修正系数的语言值索引 0~6 */

    /* ---------- 状态变量 ---------- */
    float setpoint;               /* 目标值(循迹一般设为0) */
    float prev_error;             /* 上一次误差 */
    float integral;               /* 积分项 */
    float limit;                  /* 积分限幅 */
    float ec_ma_buf[FUZZY_EC_MA_MAX]; /* EC 移动平均环形缓冲区 */
    float ec_ma_sum;              /* 环形缓冲区和 (O(1) 更新) */
    unsigned char ec_ma_index;    /* 当前写入位置 */
    unsigned char ec_ma_filled;   /* 已填充样本数 (启动阶段用) */
    char ec_count;                /* 移动平均窗口大小 (1~20, 上位机可调) */
    float K_gyro;                 /* 角速度→微分项增益 (中偏差时低阻尼) */
    float K_gyro_high;            /* 角速度→微分项增益 (小偏差时高阻尼) */
    float temp_ec;
} FuzzyPID;

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化控制器，填入默认规则表和参数 */
void FuzzyPID_Init(FuzzyPID *pid,
                   float kp0, float ki0, float kd0,
                   float e_max, float ec_max, float err_thresh);

/* 执行一次PID计算，输入实际值，返回控制量 */
float FuzzyPID_Step(FuzzyPID *pid, float measurement, float gyro_z);

/* 重置控制器状态 */
void FuzzyPID_Reset(FuzzyPID *pid);

#ifdef __cplusplus
}
#endif

#endif /* FUZZY_PID_H */