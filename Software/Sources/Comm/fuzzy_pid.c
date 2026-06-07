#include "fuzzy_pid.h"

/* 内部辅助函数：计算三角形隶属度 */
static void fuzzify(float x, float x_max, int num_sets,
                    const float centers[], float half_width,
                    float mus[]);

/**
 * 初始化模糊P控制器
 * 需传入基础P参数、输入域限幅值。
 * 默认规则表在函数内填写，可根据实车修改。
 */
void FuzzyPID_Init(FuzzyPID *pid, float kp0, float e_max)
{
    /* 局部变量必须定义在开头 */
    int i;
    int tmp_kp[7] = {
        FUZZY_PB, /* e NB — 大偏差时Kp极大 */
        FUZZY_PM, /* e NM */
        FUZZY_PS, /* e NS */
        FUZZY_ZO, /* e ZO — 中央直道Kp最小 */
        FUZZY_PS, /* e PS */
        FUZZY_PM, /* e PM */
        FUZZY_PB  /* e PB — 大偏差时Kp极大 */
    };

    /* ---- 保存基础参数 ---- */
    pid->Kp0 = kp0;
    pid->e_max = e_max;

    /* ---- 输出单点系数 ---- */
    /* 对应 FUZZY_NB(0)..FUZZY_PB(6) */
    pid->kp_singletons[FUZZY_NB] = 2.2f;
    pid->kp_singletons[FUZZY_NM] = 2.0f;
    pid->kp_singletons[FUZZY_NS] = 1.1f;
    pid->kp_singletons[FUZZY_ZO] = 0.2f;
    pid->kp_singletons[FUZZY_PS] = 1.1f;
    pid->kp_singletons[FUZZY_PM] = 2.0f;
    pid->kp_singletons[FUZZY_PB] = 2.2f;

    /* ---- 填写规则表 (7个e模糊集 → Kp输出语言值) ---- */
    for (i = 0; i < 7; i++)
        pid->rule_kp[i] = tmp_kp[i];

    /* ---- 初始化状态 ---- */
    pid->setpoint = 0.0f;   /* 循迹通常目标偏差为0 */
}

/**
 * 重置控制器历史状态
 */
void FuzzyPID_Reset(FuzzyPID *pid)
{
    /* 纯P控制器无历史状态，预留接口 */
    (void)pid;
}

/**
 * 每一步调用的P计算函数
 * measurement: 当前传感器获得的实际值（如赛道中心偏差）
 * 返回：控制量（如转向PWM）
 */
float FuzzyPID_Step(FuzzyPID *pid, float measurement)
{
    /* 所有局部变量必须在此处声明 */
    float error;
    float kp_coeff;
    float kp;
    float output;
    float mu_e[7];
    float centers_e[7];
    float half_width_e;
    float sum_w;
    float sum_wp;
    float weight;
    float pos;
    int i;

    /* NaN 防护：传感器未就绪时直接返回 0 */
    if (measurement != measurement) {
        return 0.0f;
    }

    /* 1. 计算基本误差 */
    error = pid->setpoint - measurement;

    /* 2. 限幅到模糊域内 */
    if (error > pid->e_max)  error = pid->e_max;
    if (error < -pid->e_max) error = -pid->e_max;

    /* 3. 模糊化：计算 error 的 7 个隶属度 */
    /* e 中心点：均匀分布 [-1,1] */
    for (i = 0; i < 7; i++) {
        centers_e[i] = -1.0f + i * (2.0f / 6.0f);
    }
    half_width_e = 2.0f / 6.0f;   /* 保证相邻隶属度有足够重叠 */
    fuzzify(error, pid->e_max, 7, centers_e, half_width_e, mu_e);

    /* 4. 1D 解模糊：加权平均，单输入仅依赖 e 隶属度 */
    sum_w = 0.0f;
    sum_wp = 0.0f;
    for (i = 0; i < 7; i++) {
        if (mu_e[i] < 1.0e-6f) continue;   /* 小值略过，加速 */
        weight = mu_e[i];
        pos = pid->kp_singletons[pid->rule_kp[i]];
        sum_wp += weight * pos;
        sum_w  += weight;
    }

    /* 若完全没有激活规则，返回中性系数 1.0 */
    if (sum_w < 1.0e-6f) {
        kp_coeff = 1.0f;
    } else {
        kp_coeff = sum_wp / sum_w;
    }

    /* 5. 计算最终P参数并输出 */
    kp = pid->Kp0 * kp_coeff;
    output = kp * error;

    return output;
}

/* ---------- 内部函数实现 ---------- */

/**
 * 三角形隶属度计算
 * x        : 实际值（已限幅在 [-x_max, x_max]）
 * x_max    : 输入最大幅值
 * num_sets : 模糊集个数
 * centers  : 中心点数组（归一化坐标，范围[-1,1]）
 * half_width: 半宽（归一化坐标）
 * mus      : 输出隶属度数组
 */
static void fuzzify(float x, float x_max, int num_sets,
                    const float centers[], float half_width,
                    float mus[])
{
    int i;
    float x_norm;  /* 归一化到 [-1,1] */
    float dist;

    x_norm = x / x_max;

    for (i = 0; i < num_sets; i++) {
        dist = x_norm - centers[i];
        if (dist < 0.0f) dist = -dist;        /* fabsf 替代品 */
        if (dist < half_width) {
            mus[i] = 1.0f - dist / half_width;
        } else {
            mus[i] = 0.0f;
        }
    }
}
