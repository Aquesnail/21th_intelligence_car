#ifndef LPF_H
#define LPF_H

/* ------------------------------------------------------------------
 * 一阶 IIR 低通滤波器 (高效, 每样本一次乘法+两次加法)
 *
 *   公式:  y[n] = y[n-1] + alpha * (x[n] - y[n-1])
 *
 *   alpha 越小 -> 截止频率越低, 响应越平滑
 *   alpha = 1  -> 无滤波 (输出=输入)
 *
 *   从截止频率计算 alpha:
 *     alpha = 1.0f - expf(-2.0f * 3.14159265f * fc * dt)
 *     近似 (fc * dt 很小时):  alpha ≈ 2.0f * 3.14159265f * fc * dt
 * ------------------------------------------------------------------ */

typedef struct {
    float alpha;   /* 滤波系数 (0~1] */
    float output;  /* 当前滤波输出 */
    float freq;    /* 采样频率 (Hz), 仅用于调试显示 */
} Lpf_t;

/*
 * 直接指定 alpha 初始化
 *   alpha: 滤波系数, 范围 (0, 1]
 *          1.0 = 无滤波, 越接近 0 越平滑但响应越慢
 */
void lpf_init(Lpf_t *f, float alpha);

/*
 * 从截止频率和采样周期计算 alpha 并初始化
 *   fc: 截止频率 (Hz)
 *   dt: 采样周期 (秒), 例如 500Hz -> dt=0.002
 */
void lpf_init_fc(Lpf_t *f, float fc, float dt);

/*
 * 输入新采样值, 返回滤波后的值
 */
float lpf_update(Lpf_t *f, float input);

/*
 * 重置滤波器状态
 */
void lpf_reset(Lpf_t *f, float value);

#endif /* LPF_H */
