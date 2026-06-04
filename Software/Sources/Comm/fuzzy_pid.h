#ifndef FUZZY_PID_H
#define FUZZY_PID_H

#include "lpf.h"

/* 采样频率相关宏：500Hz固定频率，用乘法替代浮点除法 */
#define FUZZY_PID_SAMPLE_FREQ  500.0f   /* 采样频率(Hz)，用于 ec = Δerror * freq */
#define FUZZY_PID_DT           0.002f   /* 采样周期 = 1/500，用于积分累加 */

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
    float err_thresh;             /* 积分分离阈值，|error|>此值时 Ki=0 */
    float kd_err_thresh;          /* Kd积分分离阈值*/
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
    Lpf_t ec_lpf;                 /* 50Hz低通滤波, 用于ec平滑 */

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
float FuzzyPID_Step(FuzzyPID *pid, float measurement);

/* 重置控制器状态 */
void FuzzyPID_Reset(FuzzyPID *pid);

#ifdef __cplusplus
}
#endif

#endif /* FUZZY_PID_H */