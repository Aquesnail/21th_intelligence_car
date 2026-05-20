#include "fuzzy_pid.h"
#include <string.h>   /* memset */

/* 内部辅助函数：计算三角形隶属度 */
static void fuzzify(float x, float x_max, int num_sets,
                    const float centers[], float half_width,
                    float mus[]);

/* 模糊推理，返回输出加权平均结果 */
static float fuzzy_infer(float mus_e[], float mus_ec[],
                         int num_e, int num_ec,
                         const int rule_table[7][5],
                         const float singletons[]);

/**
 * 初始化模糊PID控制器
 * 需传入基础PID参数、输入域限幅值及积分分离阈值。
 * 默认规则表在函数内填写，可根据实车修改。
 */
void FuzzyPID_Init(FuzzyPID *pid,
                   float kp0, float ki0, float kd0,
                   float e_max, float ec_max, float err_thresh)
{
    /* 局部变量必须定义在开头 */
    int i, j;
	int tmp_kp[7][5] = {
        /* ec:         NB        NS        ZO        PS        PB    */
        /* e NB */ { FUZZY_PB, FUZZY_PB, FUZZY_PB, FUZZY_PS, FUZZY_PS },
        /* e NM */ { FUZZY_PB, FUZZY_PS, FUZZY_PS, FUZZY_PS, FUZZY_PM },
        /* e NS */ { FUZZY_PS, FUZZY_PS, FUZZY_PS, FUZZY_PM, FUZZY_NS },
        /* e ZO */ { FUZZY_PS, FUZZY_PM, FUZZY_PM, FUZZY_PM, FUZZY_NS },
        /* e PS */ { FUZZY_NS, FUZZY_PM, FUZZY_PS, FUZZY_PS, FUZZY_PS },
        /* e PM */ { FUZZY_PM, FUZZY_PS, FUZZY_PS, FUZZY_PS, FUZZY_PB },
        /* e PB */ { FUZZY_PS, FUZZY_PS, FUZZY_PB, FUZZY_PB, FUZZY_PB }
    };
	int tmp_kd[7][5] = {
        /* ec:         NB        NS        ZO        PS        PB    */
        /* e NB */ { FUZZY_NS, FUZZY_PM, FUZZY_PS, FUZZY_PM, FUZZY_NS },
        /* e NM */ { FUZZY_NS, FUZZY_PS, FUZZY_PS, FUZZY_PS, FUZZY_NS },
        /* e NS */ { FUZZY_PM, FUZZY_PS, FUZZY_PB, FUZZY_PS, FUZZY_PM },
        /* e ZO */ { FUZZY_PM, FUZZY_PS, FUZZY_PB, FUZZY_PS, FUZZY_PM },
        /* e PS */ { FUZZY_PM, FUZZY_PS, FUZZY_PB, FUZZY_PS, FUZZY_PM },
        /* e PM */ { FUZZY_NS, FUZZY_PS, FUZZY_PS, FUZZY_PS, FUZZY_NS },
        /* e PB */ { FUZZY_NS, FUZZY_PM, FUZZY_PS, FUZZY_PM, FUZZY_NS }
    };
    /* ---- 保存基础参数 ---- */
    pid->Kp0 = kp0;
    pid->Ki0 = ki0;
    pid->Kd0 = kd0;
    pid->e_max = e_max;
    pid->ec_max = ec_max;
    pid->err_thresh = err_thresh;

    /* ---- 输出单点系数（可根据需求修改） ---- */
    /* 对应 FUZZY_NB(0)..FUZZY_PB(6) */
    pid->kp_singletons[FUZZY_NB] = 0.5f;
    pid->kp_singletons[FUZZY_NM] = 0.7f;
    pid->kp_singletons[FUZZY_NS] = 0.9f;
    pid->kp_singletons[FUZZY_ZO] = 1.0f;
    pid->kp_singletons[FUZZY_PS] = 1.1f;
    pid->kp_singletons[FUZZY_PM] = 1.3f;
    pid->kp_singletons[FUZZY_PB] = 1.5f;

    pid->kd_singletons[FUZZY_NB] = 0.3f;
    pid->kd_singletons[FUZZY_NM] = 0.6f;
    pid->kd_singletons[FUZZY_NS] = 0.8f;
    pid->kd_singletons[FUZZY_ZO] = 1.0f;
    pid->kd_singletons[FUZZY_PS] = 1.2f;
    pid->kd_singletons[FUZZY_PM] = 1.5f;
    pid->kd_singletons[FUZZY_PB] = 1.8f;

    /* ---- 填写规则表 (语言值索引 0~6) ---- */
    /* Kp修正系数规则表，行:e 列:ec，ec顺序: NB,NS,ZO,PS,PB */
    /* 原则：大偏差时Kp大(快速消除误差)；小偏差时Kp适中 */
    {

        for (i = 0; i < 7; i++)
            for (j = 0; j < 5; j++)
                pid->rule_kp[i][j] = tmp_kp[i][j];
    }

    /* Kd修正系数规则表，原则：大偏差且变化快时Kd不宜过大防振荡，
       稳态附近若变化快则增大Kd抑制超调 */
    {

        for (i = 0; i < 7; i++)
            for (j = 0; j < 5; j++)
                pid->rule_kd[i][j] = tmp_kd[i][j];
    }

    /* ---- 初始化状态 ---- */
    pid->setpoint = 0.0f;   /* 循迹通常目标偏差为0 */
    pid->prev_error = 0.0f;
    pid->integral = 0.0f;
}

/**
 * 重置控制器历史状态
 */
void FuzzyPID_Reset(FuzzyPID *pid)
{
    if (pid != NULL) {
        pid->prev_error = 0.0f;
        pid->integral = 0.0f;
    }
}

/**
 * 每一步调用的PID计算函数
 * measurement: 当前传感器获得的实际值（如赛道中心偏差）
 * dt:         距离上次调用的时间间隔(>0)
 * 返回：控制量（如转向PWM）
 */
float FuzzyPID_Step(FuzzyPID *pid, float measurement, float dt)
{
    /* 所有局部变量必须在此处声明 */
    float error, ec;
    float kp_coeff, kd_coeff;
    float kp, ki, kd;
    float output;
    int num_e_sets = 7;
    int num_ec_sets = 5;
    float mu_e[7];
    float mu_ec[5];
    float centers_e[7];
    float centers_ec[5];
    float half_width_e;
    float half_width_ec;
    int i;

    /* 1. 计算基本误差及变化率 */
    error = pid->setpoint - measurement;               /* 循迹一般 setpoint=0 */

    /* 防除零 */
    if (dt <= 1.0e-6f) {
        ec = 0.0f;
    } else {
        ec = (error - pid->prev_error) / dt;
    }
    pid->prev_error = error;

    /* 2. 限幅到模糊域内 */
    if (error > pid->e_max)  error = pid->e_max;
    if (error < -pid->e_max) error = -pid->e_max;
    if (ec > pid->ec_max)    ec = pid->ec_max;
    if (ec < -pid->ec_max)   ec = -pid->ec_max;

    /* 3. 模糊化：计算 error 的 7 个隶属度 */
    /* e 中心点：均匀分布 [-1,1] 映射到归一化 */
    for (i = 0; i < 7; i++) {
        centers_e[i] = -1.0f + i * (2.0f / 6.0f);
    }
    half_width_e = 2.0f / 3.0f;   /* 保证相邻隶属度有足够重叠 */
    fuzzify(error, pid->e_max, 7, centers_e, half_width_e, mu_e);

    /* 4. 模糊化：计算 ec 的 5 个隶属度 */
    for (i = 0; i < 5; i++) {
        centers_ec[i] = -1.0f + i * (2.0f / 4.0f);
    }
    half_width_ec = 1.0f;         /* 5个集合，半宽=间距保证重叠 */
    fuzzify(ec, pid->ec_max, 5, centers_ec, half_width_ec, mu_ec);

    /* 5. 模糊推理得到 Kp 和 Kd 的修正系数 */
    kp_coeff = fuzzy_infer(mu_e, mu_ec, 7, 5,
                           pid->rule_kp, pid->kp_singletons);
    kd_coeff = fuzzy_infer(mu_e, mu_ec, 7, 5,
                           pid->rule_kd, pid->kd_singletons);

    /* 6. 计算最终PID参数 */
    kp = pid->Kp0 * kp_coeff;
    kd = pid->Kd0 * kd_coeff;

    /* 积分分离：大偏差时冻结积分，防止过冲和饱和 */
    if (error > pid->err_thresh || error < -pid->err_thresh) {
        ki = 0.0f;
        /* 也可选清零积分项，保留上次值可能导致突变，这里选择冻结 */
        pid->integral = 0.0f;  /* 如需强制清零可取消注释 */
    } else {
        ki = pid->Ki0;
        pid->integral += error * dt;
        /* 积分限幅 (可选) */
         if (pid->integral >  pid->limit) pid->integral =  pid->limit;
    }

    /* 7. 位置式PID输出 */
    output = kp * error + ki * pid->integral + kd * ec;

    /* 8. 输出限幅（根据实际需求调整） */
    /* if (output >  1000.0f) output =  1000.0f; */
    /* if (output < -1000.0f) output = -1000.0f; */

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

    /* 边界处理：若输入超出归一化范围，强制最外侧隶属度为1 */
    if (x_norm <= -1.0f) {
        for (i = 0; i < num_sets; i++) mus[i] = 0.0f;
        mus[FUZZY_NB] = 1.0f;
    } else if (x_norm >= 1.0f) {
        for (i = 0; i < num_sets; i++) mus[i] = 0.0f;
        mus[num_sets-1] = 1.0f;  /* FUZZY_PB */
    }
}

/**
 * 模糊推理：取最小激活度，加权平均解模糊（单点输出）
 * mus_e, mus_ec : 输入隶属度
 * num_e, num_ec : 集合数量
 * rule_table    : 规则表，行列对应e和ec，值为输出语言值索引
 * singletons    : 各输出语言值对应的精确数值
 * 返回：加权平均后的清晰值
 */
static float fuzzy_infer(float mus_e[], float mus_ec[],
                         int num_e, int num_ec,
                         const int rule_table[7][5],
                         const float singletons[])
{
    int i, j;
    float weight, pos;
    float sum_w = 0.0f;
    float sum_wp = 0.0f;

    for (i = 0; i < num_e; i++) {
        if (mus_e[i] < 1.0e-6f) continue;   /* 小值略过，加速 */
        for (j = 0; j < num_ec; j++) {
            if (mus_ec[j] < 1.0e-6f) continue;
            /* 取小运算 */
            weight = (mus_e[i] < mus_ec[j]) ? mus_e[i] : mus_ec[j];
            pos = singletons[rule_table[i][j]];
            sum_wp += weight * pos;
            sum_w  += weight;
        }
    }

    /* 若完全没有激活规则，返回中性系数 1.0 */
    if (sum_w < 1.0e-6f) {
        return 1.0f;
    }
    return sum_wp / sum_w;
}
