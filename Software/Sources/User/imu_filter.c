#include "imu_filter.h"
#include <math.h>

/* Mahony 滤波器的 PI 控制参数，可根据实际表现进行微调 */

void IMU_Filter_Update(IMU_Filter_t* dat){
    /* C89标准要求：局部变量必须全部在函数/代码块开头声明 */
    static float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f; /* 静态变量保存四元数状态 */
    static float eIntX = 0.0f, eIntY = 0.0f, eIntZ = 0.0f;   /* 静态变量保存积分误差（陀螺仪零偏） */
    
    float norm;
    float vx, vy, vz;
    float ex, ey, ez;
    float qa, qb, qc;
    float gx, gy, gz, ax, ay, az;
    float dt;

    /* 1. 获取输入物理量 */
    /* 注意：传入的陀螺仪数据 (gx, gy, gz) 必须已经转换为 弧度/秒 (rad/s) */
    gx = dat->gx;
    gy = dat->gy;
    gz = dat->gz;
    ax = dat->ax;
    ay = dat->ay;
    az = dat->az;
    dt = dat->sample_time; /* 积分时间周期，单位：秒 (s) */

    /* 防止加速度计数据为0导致除以0引发异常 */
    if((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f)) {
        return;
    }

    /* 2. 将加速度计的三轴数据归一化 */
    norm = (float)sqrt(ax * ax + ay * ay + az * az);
    norm = 1.0f / norm;
    ax *= norm;
    ay *= norm;
    az *= norm;

    /* 3. 根据当前的四元数估计出理论上的重力方向向量 */
    vx = 2.0f * (q1 * q3 - q0 * q2);
    vy = 2.0f * (q0 * q1 + q2 * q3);
    vz = q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3;

    /* 4. 计算加速度计测量的重力向量与理论估计重力向量的误差（叉乘） */
    ex = (ay * vz - az * vy);
    ey = (az * vx - ax * vz);
    ez = (ax * vy - ay * vx);

    /* 5. 误差积分计算 —— 这是自校准功能的核心 */
    /* 积分项 eInt 将不断累积误差，相当于估算并抵消陀螺仪的稳态零偏（Drift） */
    if(dat->Ki > 0.0f) {
        eIntX += ex * dt;
        eIntY += ey * dt;
        eIntZ += ez * dt;
    } else {
        eIntX = 0.0f;
        eIntY = 0.0f;
        eIntZ = 0.0f;
    }

    /* 6. PI 控制器：用比例项和积分项共同修正陀螺仪数据 */
    gx = gx + dat->Kp * ex + dat->Ki * eIntX;
    gy = gy + dat->Kp * ey + dat->Ki * eIntY;
    gz = gz + dat->Kp * ez + dat->Ki * eIntZ;

    /* 7. 使用修正后的陀螺仪数据更新四元数 (一阶龙格-库塔法) */
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

    /* 8. 四元数重新归一化 */
    norm = (float)sqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    norm = 1.0f / norm;
    q0 *= norm;
    q1 *= norm;
    q2 *= norm;
    q3 *= norm;

    /* 9. 四元数转欧拉角 (结果转换为角度制 Degree) */
    /* 乘以 57.29578f 是为了将弧度(rad)转换为度(°)： 180 / π ≈ 57.29578 */
    dat->roll  = (float)atan2(2.0f * (q0 * q1 + q2 * q3), 1.0f - 2.0f * (q1 * q1 + q2 * q2)) * 57.29578f;
    dat->pitch = (float)asin(2.0f * (q0 * q2 - q3 * q1)) * 57.29578f;
    dat->yaw   = (float)atan2(2.0f * (q0 * q3 + q1 * q2), 1.0f - 2.0f * (q2 * q2 + q3 * q3)) * 57.29578f;
}