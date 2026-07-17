#ifndef FUZZY_PID_H
#define FUZZY_PID_H

/* 模糊语言值枚举 (前缀防混用) */
typedef enum {
    FUZZY_NB = 0,
    FUZZY_NM = 1,
    FUZZY_NS = 2,
    FUZZY_NZ = 3,   /* Negative Zero — 近零负偏差 */
    FUZZY_ZO = 4,
    FUZZY_PZ = 5,   /* Positive Zero — 近零正偏差 */
    FUZZY_PS = 6,
    FUZZY_PM = 7,
    FUZZY_PB = 8
} Fuzzy_LV;

/* 模糊P控制器结构体 */
typedef struct {
    /* ---------- 基础P参数 ---------- */
    float Kp0;                    /* 手动整定的基础值 */

    /* ---------- 模糊域参数 ---------- */
    float e_max;                  /* 误差输入最大幅值 */

    /* ---------- 输出单点(修正系数) ---------- */
    float kp_singletons[9];       /* NB,NM,NS,NZ,ZO,PZ,PS,PM,PB 对应的 Kp 系数 */

    /* ---------- 规则表 (9个e模糊集 → Kp输出) ---------- */
    int rule_kp[9];               /* 每个e模糊集对应的 Kp 输出语言值索引 0~8 */

    /* ---------- 超出部分线性增益 ---------- */
    float Kp_excess;              /* 误差超出e_max部分的固定线性增益 */

    /* ---------- 状态变量 ---------- */
    float setpoint;               /* 目标值(循迹一般设为0) */
} FuzzyPID;

#ifdef __cplusplus
extern "C" {
#endif

/* 初始化控制器，填入默认规则表和参数 */
void FuzzyPID_Init(FuzzyPID *pid, float kp0, float e_max);

/* 执行一次P计算，输入实际值，返回控制量 */
float FuzzyPID_Step(FuzzyPID *pid, float measurement);

/* 重置控制器状态 */
void FuzzyPID_Reset(FuzzyPID *pid);

#ifdef __cplusplus
}
#endif

#endif /* FUZZY_PID_H */
