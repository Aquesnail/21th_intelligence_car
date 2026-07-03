#ifndef ELEMENT_H
#define ELEMENT_H

#include "config.h"

typedef enum ElementType {
    ELEMENT_STRAIGHT = 0,
    ELEMENT_RING_LEFT_ENTRANCE,
    ELEMENT_RING_RIGHT_ENTRANCE,
    ELEMENT_WALL,
    ELEMENT_CROSS,
    ELEMENT_BARREL,
    ELEMENT_SKIP,
    
    ELEMENT_RING_LEFT_PASS,       /* 左环岛开环通过中 */
    ELEMENT_RING_RIGHT_PASS,      /* 右环岛开环通过中 */
    ELEMENT_IN_RING,              /* 环内行驶, 等待出环检测 */
    
    ELEMENT_WALL_CLIMB,           /* 上墙子状态1: 上爬, TiltB < -60 进入, |TiltR| > 70 退出 */
    ELEMENT_WALL_HORIZONTAL,      /* 上墙子状态2: 墙上水平, |TiltR| > 70 进入, |TiltR| < 70 退出 */
    ELEMENT_WALL_DESCEND,         /* 上墙子状态3: 下行冲刺, |TiltR| < 70 进入, TiltB > 60 退出 */

    ELEMENT_RIGHT_ANGLE_LEFT,     /* 左直角转弯: 传感器极性反转屏蔽 */
    ELEMENT_RIGHT_ANGLE_RIGHT,    /* 右直角转弯: 传感器极性反转屏蔽 */

    ELEMENT_SAFETY_STOP,          /* 安全停车: 翻车/离赛道超时 (最后一个活跃元素) */
    /* 以下保留，暂未使用 */
    ELEMENT_RING_LEFT_EXIT,
    ELEMENT_RING_RIGHT_EXIT,
    
  //  ELEMENT_TIMER,                  /* 定时器元素: 每次开始起跑时清零，以秒为单位计时，计时到了若发现倾角正常则自动停车*/
} element_t;

#define ELEMENT_ACTIVE_COUNT  (ELEMENT_SAFETY_STOP + 1)  /* 14 个活跃元素 (含 WALL 3 子状态) */

/* ================================================================
 * 控制策略枚举
 * ================================================================ */
typedef enum {
    CTRL_NORMAL,     /* 正常 PID 循迹 → 写电机 */
    CTRL_OL_YAW,     /* 开环角速度覆盖（PASS 阶段） → 写电机 */
    CTRL_BRAKE,      /* 紧急刹车（SAFETY_STOP） → 写0 */
    CTRL_FREEWHEEL,  /* 自由滑行（推车调试） → 写0, 元素检测/LED照跑 */
    CTRL_ERR_FREEZE, /* 冻结 Err_Angle（直角转弯 → 屏蔽传感器极性反转） */
} ControlStrategy_t;

/* ================================================================
 * 通用投票窗 (Majority Voting Window)
 *   环形缓冲 + 增量计数, O(1) 推入和查询
 * ================================================================ */
typedef struct {
    unsigned char xdata *buf;       /* 环形缓冲区指针 */
    unsigned char max_len;          /* 缓冲区容量（数组长度上限） */
    unsigned char idx;              /* 当前写入位置 */
    unsigned char cnt;              /* 窗口内 1 的个数（增量维护） */
    unsigned char len;              /* 有效窗长 0~max_len，0=旁路 */
    unsigned char entry_thresh;     /* 进入阈值：cnt >= 此值则触发 */
    unsigned char exit_thresh;      /* 退出阈值：cnt >= 此值则触发 */
} VoteWindow_t;

/* ================================================================
 * 元素属性 (flash 常驻, 只读)
 *   每个元素的所有属性集中定义, 替代分散在多个文件中的 switch-case
 * ================================================================ */
typedef struct {
    element_t          id;              /* 元素枚举值 */
    const char        *name;            /* 可读名称（Debug_Log 用） */
    float             *speed_ptr;       /* 指向对应速度变量（NULL = 不改变速度） */
    unsigned char      led_r;           /* LED 颜色 R */
    unsigned char      led_g;           /* LED 颜色 G */
    unsigned char      led_b;           /* LED 颜色 B */
    ControlStrategy_t  ctrl;            /* 控制策略 */
    float             *ol_yawrate_ptr;  /* 指向开环角速度变量（仅 CTRL_OL_YAW 使用） */
    signed char        ol_yaw_sign;     /* 角速度符号：+1 左转，-1 右转 */
} ElementProp_t;

/* ================================================================
 * 元素运行时上下文 (替代散乱的 static 变量)
 * ================================================================ */
typedef struct {
    /* ---- 传感器输入（由调用方填充） ---- */
    float adc[4];
    float ax, ay, az;
    float yaw;
    float err_raw;                /* 原始 Err_Angle (未滤波), 供退出检测用 */
    float err_filtered;           /* 滤波后 Err_Angle, 供进入检测用 */
    float yawrate;               /* 角速度 (deg/s) */

    /* ---- 计算中间量 ---- */
    float tilt_fb;                /* 前后倾角(度), 四元数重力向量版 */
    float tilt_lr;                /* 侧倾角(度),   四元数重力向量版 */

    /* ---- 投票窗（4 个实例共享同一 VoteWin_Push 逻辑） ---- */
    VoteWindow_t vote_ring;
    VoteWindow_t vote_wall;
    VoteWindow_t vote_wall_climb;
    VoteWindow_t vote_wall_horiz;
    VoteWindow_t vote_wall_descend;
    VoteWindow_t vote_barrel;
    VoteWindow_t vote_cross;
    VoteWindow_t vote_right_angle;  /* 直角转弯投票窗 */

    /* ---- 环岛专用状态 ---- */
    unsigned char  ring_side;           /* 0=左环, 1=右环 */
    float          ring_entry_yaw;
    unsigned short exit_lockout_cnt;    /* 出环锁存倒计时 */
    unsigned short ring_frame_cnt;      /* 环岛链总帧计数 (ENTRANCE→PASS→IN_RING 超时兜底) */

    /* ---- 直角转弯专用状态 ---- */
    float          frozen_err;           /* 进入冻结时记录的 err_angle_filtered */
    unsigned char  right_angle_side;     /* 0=左直角, 1=右直角 */

    /* ---- 通用元素帧计数 (在 element Enter 时归零, Element_Update 开头累加) ---- */
    unsigned short elem_frame_cnt;

    unsigned long  total_ticks_start;  /* 总计时器起始 tick (xTaskGetTickCount) */
    /* ---- 元素表 ---- */
    unsigned char track_idx;
    unsigned char track_table_enabled;  /* 上位机可调 */

    /* ---- 离赛道超时 ---- */
    unsigned char  offtrack_active;
    unsigned long  offtrack_start_tick;

    
    /* ---- 当前/前一状态 ---- */
    element_t current;
    element_t prev;
} ElementCtx_t;

/* ================================================================
 * 投票窗 API
 * ================================================================ */
void VoteWin_Init(VoteWindow_t *w, unsigned char xdata *buf, unsigned char max_len);
void VoteWin_Push(VoteWindow_t *w, unsigned char vote);
unsigned char VoteWin_Reached(VoteWindow_t *w, unsigned char thresh);
void VoteWin_Reset(VoteWindow_t *w);
void VoteWin_SetLen(VoteWindow_t *w, unsigned char len);

/* ================================================================
 * 元素 API
 * ================================================================ */
void Element_Init(ElementCtx_t *ctx);
element_t Element_Update(ElementCtx_t *ctx);
const ElementProp_t* Element_GetProp(element_t e);
ControlStrategy_t Element_GetEffectiveCtrl(element_t e);
void Element_ResetOfftrackTimer(ElementCtx_t *ctx);
void Element_ResetTrackIndex(ElementCtx_t *ctx);
float SelectTargetYaw(ControlStrategy_t ctrl, const ElementProp_t *prop,ElementCtx_t *ctx);
float SelectTargetErr(ControlStrategy_t ctrl, const ElementProp_t *prop,ElementCtx_t *ctx);
#endif /* ELEMENT_H */
