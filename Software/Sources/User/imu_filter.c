#include "imu_filter.h"
#include <math.h>

#define TARGET_CALIB_TIME 2.0f
#define GYRO_JITTER_THRESH 0.1f
#define ACCEL_JITTER_THRESH 0.05f
void IMU_Filter_Update(IMU_Filter_t* dat){
    /* =========================================================
     * 1. 局部变量声明 (严格遵循 C89 标准)
     * =========================================================*/
    /* 新增：开机自动校准相关的静态变量 */
    static uint8_t is_calibrated = 0;                        /* 校准完成标志位 */
    static uint32_t calib_count = 0;                         /* 有效静止采样计数 */
    static float last_gx = 0.0f, last_gy = 0.0f, last_gz = 0.0f; /* 上一次的陀螺仪数据 (用于抖动检测) */
    static float last_ax = 0.0f, last_ay = 0.0f, last_az = 0.0f; /* 上一次的加速度计数据 (用于抖动检测) */

    /* 原有：Mahony滤波状态变量 */
    static float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;
    static float eIntX = 0.0f, eIntY = 0.0f, eIntZ = 0.0f;
    
    /* 原有：计算过程临时变量 */
    float norm;
    float vx, vy, vz;
    float ex, ey, ez;
    float qa, qb, qc;
    float gx, gy, gz, ax, ay, az;
    float dt;

    uint32_t target_calib_count;          /* 目标需要达到的静止采样总次数 */

    /* =========================================================
     * 2. 获取输入物理量
     * =========================================================*/
    gx = dat->gx;
    gy = dat->gy;
    gz = dat->gz;
    ax = dat->ax;
    ay = dat->ay;
    az = dat->az;
    dt = (dat->sample_time); 

    if((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f)) {
        return;
    }

    /* =========================================================
     * 3. 核心功能新增：开机静止零偏校准
     * =========================================================*/
    if (is_calibrated == 0) {
        target_calib_count = (uint32_t)(TARGET_CALIB_TIME / dt);

        /* 检测是否发生抖动：通过计算当前值与上一次值的差值绝对值 */
        if (calib_count > 0) {
            /* C89标准中 fabs 返回 double，此处强转为 float 以提高运算效率并避免警告 */
            if ((float)fabs(gx - last_gx) > GYRO_JITTER_THRESH ||
                (float)fabs(gy - last_gy) > GYRO_JITTER_THRESH ||
                (float)fabs(gz - last_gz) > GYRO_JITTER_THRESH ||
                (float)fabs(ax - last_ax) > ACCEL_JITTER_THRESH ||
                (float)fabs(ay - last_ay) > ACCEL_JITTER_THRESH ||
                (float)fabs(az - last_az) > ACCEL_JITTER_THRESH) {
                
                /* 检测到突发抖动，清空累加器，重置校准进程 */
                calib_count = 0;
                dat->bias_gx = 0.0f;
                dat->bias_gy = 0.0f;
                dat->bias_gz = 0.0f;
            }
        }

        /* 更新历史数据用于下一次比较 */
        last_gx = gx; last_gy = gy; last_gz = gz;
        last_ax = ax; last_ay = ay; last_az = az;

        /* 数据平稳，累加陀螺仪当前数据 */
        dat->bias_gx += gx;
        dat->bias_gy += gy;
        dat->bias_gz += gz;
        calib_count++;

        /* 判断是否已经满足所需的纯静止时间 */
        if (calib_count >= target_calib_count) {
            /* 计算平均值，得到真实的静态零漂 */
            dat->bias_gx /= (float)calib_count;
            dat->bias_gy /= (float)calib_count;
            dat->bias_gz /= (float)calib_count;
            is_calibrated = 1; /* 校准完成，锁定基准 */
        }
        
        /* 在校准未完成期间，我们不进行四元数更新，直接返回 */
        /* 注意：此时由于未更新 dat->roll 等数据，上层读取到的角度将保持初始默认值(通常是0) */
        return; 
    }

    /* =========================================================
     * 4. 正常运行阶段：减去静态零偏
     * =========================================================*/
    gx -= dat->bias_gx;
    gy -= dat->bias_gy;
    gz -= dat->bias_gz;

    /* =========================================================
     * 5. 原始的 Mahony 滤波逻辑 (保持不变)
     * =========================================================*/
    
    /* 归一化加速度计数据 */
    norm = (float)sqrt(ax * ax + ay * ay + az * az);
    norm = 1.0f / norm;
    ax *= norm;
    ay *= norm;
    az *= norm;

    /* 根据当前的四元数估计出理论上的重力方向向量 */
    vx = 2.0f * (q1 * q3 - q0 * q2);
    vy = 2.0f * (q0 * q1 + q2 * q3);
    vz = q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3;

    /* 误差叉乘 */
    ex = (ay * vz - az * vy);
    ey = (az * vx - ax * vz);
    ez = (ax * vy - ay * vx);

    /* 积分误差累积 (处理残余的动态零漂) */
    if(dat->Ki > 0.0f) {
        eIntX += ex * dt;
        eIntY += ey * dt;
        eIntZ += ez * dt;
    } else {
        eIntX = 0.0f;
        eIntY = 0.0f;
        eIntZ = 0.0f;
    }

    /* PI 控制 */
    gx = gx + dat->Kp * ex + dat->Ki * eIntX;
    gy = gy + dat->Kp * ey + dat->Ki * eIntY;
    gz = gz + dat->Kp * ez + dat->Ki * eIntZ;

    /* 更新四元数 */
    gx *= (0.5f * dt);
    gy *= (0.5f * dt);
    gz *= (0.5f * dt);
    qa = q0;
    qb = q1;
    qc = q2;
    q0 += (-qb * gx - qc * gy - q3 * gz);
    q1 += (qa * gx + qc * gz - q3 * gy);
    q2 += (qa * gy - qb * gz + q3 * gx);
    q3 += (qa * gz + qb * gy - qc * gx);

    /* 四元数重新归一化 */
    norm = (float)sqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    norm = 1.0f / norm;
    q0 *= norm;
    q1 *= norm;
    q2 *= norm;
    q3 *= norm;

    /* 四元数转欧拉角 */
    dat->roll  = (float)atan2(2.0f * (q0 * q1 + q2 * q3), 1.0f - 2.0f * (q1 * q1 + q2 * q2)) * 57.29578f;
    dat->pitch = (float)asin(2.0f * (q0 * q2 - q3 * q1)) * 57.29578f;
    dat->yaw   = (float)atan2(2.0f * (q0 * q3 + q1 * q2), 1.0f - 2.0f * (q2 * q2 + q3 * q3)) * 57.29578f;
}