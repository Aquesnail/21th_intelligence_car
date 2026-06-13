#include "element.h"
#include "debug_arch.h"
#include "FreeRTOS.h"
#include "task.h"
#include "led_indicator.h"

extern uint8_t manual_control_flag;

/* ================================================================
 * 环岛 ADC 阈值
 *   RingEntrance[x]: ADC 高于此值 → 候选进入环岛
 *   RingExit[x]:     ADC 高于此值 → 候选退出环岛 (与进入阈值独立可调, 形成滞回)
 *   进入/退出之间形成滞回区, 防止边界抖动
 * ================================================================ */
static float RingEntrance[4];
static float RingExit[4];
static float angle_level = 5.0f;   /* 车身水平倾角阈值(度), 重力分量 atan2(|ax|,|az|) */

/* ================================================================
 * 环岛多数投票窗 (Majority Voting Window)
 *   窗长可调 0~100, 0 = 旁路 (即时切换)
 *   统一入口检测窗, 不区分方向:
 *     STRAIGHT: 1 = "任一水平电感超阈值 且 车身水平"
 *     IN_RING:  1 = "环方向对应通道再次尖峰(出环)"
 *   方向由元素顺序表 track_expected 指定, 表禁用时右环优先
 *   状态切换后旧投票自然老化, 提供最短保持时间
 * ================================================================ */
static uint8_t xdata ring_vote_win[100];
static uint8_t ring_win_idx;          /* 窗口写入位置 (循环) */
static uint8_t ring_win_cnt;          /* 窗口内 1 的个数 (增量维护) */

static uint8_t ring_win_len;          /* 有效窗长 0~100, 0=旁路 */
static uint8_t ring_entry_thresh;     /* 进入/退出阈值: 1 的个数 >= 此值 */

/* ================================================================
 * 开环 PASS 阶段参数 (左右环共用, 左环取负值)
 *   进入 ENTRANCE 后立即切入 PASS, 记录起始 yaw, 帧计数
 *   退出条件: yaw 变化量达标 OR 帧数超时
 * ================================================================ */
float    g_ol_ring_angle   = 45.0f;   /* 环岛开环目标角度(绝对值, 度) */
float    g_ol_ring_yawrate = 3.0f;    /* 环岛开环目标角速度(绝对值, °/s) */
static float    g_pass_entry_yaw  = 0.0f;
static uint16_t g_pass_frame_cnt  = 0;
static uint16_t g_pass_timeout_frames = 200;  /* 默认200帧=400ms, 上位机可调 */
static uint8_t  g_ring_side = 0;             /* 环内方向: 0=左环(adc[2]), 1=右环(adc[0]) */

/* 退环 yaw 累计门禁: 入环后 yaw 变化量达标才允许出环 */
static float    g_ring_entry_yaw        = 0.0f;   /* 进入 IN_RING 时的 yaw */
static float    g_ring_exit_yaw_thresh  = 300.0f;  /* 退环所需 yaw 累计角度(绝对值, 度) */

/* ================================================================
 * 赛道元素顺序表 (写死在 flash 中, 按赛道实际顺序排列)
 *   启用后, STRAIGHT 下只对表指定的方向投票, 杜绝方向误判
 *   每完成一个元素退回到 STRAIGHT 后自动推进索引
 *   索引超出表尾后不再约束 (赛道跑完)
 * ================================================================ */
#define TRACK_TABLE_LEN  8  /* 根据实际赛道元素数量修改 */

static const uint8_t g_track_table[TRACK_TABLE_LEN] = {
    /* TODO: 按实际赛道顺序填入非 STRAIGHT 元素 */
    ELEMENT_RING_LEFT_ENTRANCE,
   // ELEMENT_CROSS,
    //ELEMENT_CROSS,
   // ELEMENT_WALL,
   // ELEMENT_BARREL
    /* 继续添加: ELEMENT_WALL, ELEMENT_CROSS, ... */
};

static uint8_t  g_track_table_enabled = 0;   /* 上位机可调: 1=启用, 0=旁路(无约束) */
static uint8_t  g_track_idx = 0;             /* 指向下一个预期元素的索引 */

/* 上墙/滚筒/十字投票窗最大长度 (数组容量, 上位机可在 0..MAX 范围内调窗长) */
#define WALL_WIN_MAX    20
#define BARREL_WIN_MAX  20
#define CROSS_WIN_MAX   20

/* ================================================================
 * 上墙检测多数投票窗 (Majority Voting Window)
 *   窗长可调 0~WALL_WIN_MAX, 0 = 旁路 (即时切换)
 *   进入条件: 倾角 tilt_x 超过 wall_entry_tilt 时投票 1
 *   退出条件: 倾角 tilt_x 低于 wall_exit_tilt  时投票 1
 *   倾角 = atan2(|ax|,|az|), 由重力矢量直接计算, 不依赖欧拉角
 *   仅从 STRAIGHT 进入上墙, 从 WALL 退回 STRAIGHT
 * ================================================================ */
static uint8_t xdata wall_vote_win[WALL_WIN_MAX];
static uint8_t wall_win_idx;          /* 当前写入位置 (循环) */
static uint8_t wall_win_cnt;          /* 窗口内 1 的个数 (增量维护) */
static uint8_t wall_win_len;          /* 有效窗长 0~WALL_WIN_MAX, 0=旁路 */
static float   wall_entry_tilt;       /* 进入上墙的倾角阈值(度), 重力 atan2(|ax|,|az|) */
static float   wall_exit_tilt;        /* 退出上墙的倾角阈值(度), 重力 atan2(|ax|,|az|) */
static uint8_t wall_entry_thresh;     /* 进入阈值: 1 的个数 >= 此值 */
static uint8_t wall_exit_thresh;      /* 退出阈值: 1 的个数 >= 此值 */

/* ================================================================
 * 滚筒检测多数投票窗 (Majority Voting Window)
 *   窗长可调 0~BARREL_WIN_MAX, 0 = 旁路 (即时切换)
 *   进入条件: 倾角超过 barrel_entry_tilt 时投票 1
 *   退出条件: 倾角低于 barrel_exit_tilt  时投票 1
 *   仅从 STRAIGHT 进入滚筒, 从 BARREL 退回 STRAIGHT
 *   (滚筒也用前后倾角, 与上墙相同算法, 仅阈值不同)
 * ================================================================ */
static uint8_t xdata barrel_vote_win[BARREL_WIN_MAX];
static uint8_t barrel_win_idx;        /* 当前写入位置 (循环) */
static uint8_t barrel_win_cnt;        /* 窗口内 1 的个数 (增量维护) */
static uint8_t barrel_win_len;        /* 有效窗长 0~BARREL_WIN_MAX, 0=旁路 */
static float   barrel_entry_tilt;     /* 进入滚筒的倾角阈值(度) */
static float   barrel_exit_tilt;      /* 退出滚筒的倾角阈值(度) */
static uint8_t barrel_entry_thresh;   /* 进入阈值: 1 的个数 >= 此值 */
static uint8_t barrel_exit_thresh;    /* 退出阈值: 1 的个数 >= 此值 */

/* ================================================================
 * 十字检测多数投票窗 (Majority Voting Window)
 *   窗长可调 0~CROSS_WIN_MAX, 0 = 旁路 (即时切换)
 *   进入条件: 两侧竖直电感(adc[1]+adc[3])之和超过 cross_entry_sum 时投票 1
 *   退出条件: 两侧竖直电感之和低于 cross_exit_sum  时投票 1
 *   仅从 STRAIGHT 进入十字, 从 CROSS 退回 STRAIGHT
 * ================================================================ */
static uint8_t xdata cross_vote_win[CROSS_WIN_MAX];
static uint8_t cross_win_idx;          /* 当前写入位置 (循环) */
static uint8_t cross_win_cnt;          /* 窗口内 1 的个数 (增量维护) */
static uint8_t cross_win_len;          /* 有效窗长 0~CROSS_WIN_MAX, 0=旁路 */
static float   cross_entry_sum;        /* 进入十字的竖直电感和阈值 */
static float   cross_exit_sum;         /* 退出十字的竖直电感和阈值 */
static uint8_t cross_entry_thresh;     /* 进入阈值: 1 的个数 >= 此值 */
static uint8_t cross_exit_thresh;      /* 退出阈值: 1 的个数 >= 此值 */

/* ================================================================
 * 重力矢量滑动平均窗 (Moving Average, 防加速度计尖峰)
 *   窗长可调 (GRAVITY_MA_WIN_LEN 宏, 默认60帧=120ms)
 *   增量维护累加和, O(1) 无遍历
 * ================================================================ */
#define GRAVITY_MA_WIN_LEN  60

static float xdata g_grav_win_ax[GRAVITY_MA_WIN_LEN];
static float xdata g_grav_win_ay[GRAVITY_MA_WIN_LEN];
static float xdata g_grav_win_az[GRAVITY_MA_WIN_LEN];
static uint8_t g_grav_win_idx;    /* 当前写入位置 (循环) */
static uint8_t g_grav_win_cnt;    /* 已累积样本数 (<= WIN_LEN) */
static float   g_grav_sum_ax;     /* 窗口内累加和 */
static float   g_grav_sum_ay;
static float   g_grav_sum_az;

/* ================================================================
 * 安全停车检测 (Safety Stop)
 *   离赛道超时: 4路ADC全部丢失超过 g_offtrack_timeout_ms → SAFETY_STOP
 * ================================================================ */
static float   g_offtrack_timeout_ms   = 500.0f;  /* 离赛道超时 (ms) */
static uint32_t g_offtrack_start_tick  = 0;       /* 信号丢失起始 tick */
static uint8_t  g_offtrack_active      = 0;       /* 当前是否正在计时 */

/* ---- 将投票推入统一环岛窗口, 增量更新计数 ---- */
static void Ring_Vote_Push(uint8_t vote)
{
    uint8_t old_vote;

    if (ring_win_len == 0) return;
    if (ring_win_idx >= ring_win_len) {
        ring_win_idx = 0;
    }

    old_vote = ring_vote_win[ring_win_idx];
    if (old_vote == 1) {
        ring_win_cnt--;
    }
    ring_vote_win[ring_win_idx] = vote;
    if (vote == 1) {
        ring_win_cnt++;
    }
    /* 防止上位机调小窗长后计数超过窗长, 导致 uint8 减法溢出 */
    if (ring_win_cnt > ring_win_len) {
        ring_win_cnt = ring_win_len;
    }
    ring_win_idx++;
}

/* ---- 元素表: 完成一个元素后推进索引, 超出表尾后不再推进(赛道跑完) ---- */
static void Track_Advance(void)
{
    if (g_track_table_enabled && g_track_idx < TRACK_TABLE_LEN) {
        g_track_idx++;
    }
}

/* ---- 重力矢量滑动平均: 增量维护累加和, O(1) 无遍历 ---- */
static void Grav_MA_Push(float ax, float ay, float az)
{
    float old_ax, old_ay, old_az;

    if (g_grav_win_cnt >= GRAVITY_MA_WIN_LEN) {
        /* 窗满: 减去最旧值, 加上新值 */
        old_ax = g_grav_win_ax[g_grav_win_idx];
        old_ay = g_grav_win_ay[g_grav_win_idx];
        old_az = g_grav_win_az[g_grav_win_idx];
        g_grav_sum_ax += ax - old_ax;
        g_grav_sum_ay += ay - old_ay;
        g_grav_sum_az += az - old_az;
    } else {
        /* 窗未满: 直接累加 */
        g_grav_sum_ax += ax;
        g_grav_sum_ay += ay;
        g_grav_sum_az += az;
        g_grav_win_cnt++;
    }

    g_grav_win_ax[g_grav_win_idx] = ax;
    g_grav_win_ay[g_grav_win_idx] = ay;
    g_grav_win_az[g_grav_win_idx] = az;

    g_grav_win_idx++;
    if (g_grav_win_idx >= GRAVITY_MA_WIN_LEN) {
        g_grav_win_idx = 0;
    }
}

/* ---- 将上墙投票推入环形窗口, 增量更新计数 ---- */
static void Wall_Vote_Push(uint8_t vote)
{
    uint8_t old_vote;

    if (wall_win_len == 0) return;
    if (wall_win_idx >= wall_win_len) {
        wall_win_idx = 0;
    }

    old_vote = wall_vote_win[wall_win_idx];
    if (old_vote == 1) {
        wall_win_cnt--;
    }
    wall_vote_win[wall_win_idx] = vote;
    if (vote == 1) {
        wall_win_cnt++;
    }
    if (wall_win_cnt > wall_win_len) {
        wall_win_cnt = wall_win_len;
    }
    wall_win_idx++;
}

/* ---- 将滚筒投票推入环形窗口, 增量更新计数 ---- */
static void Barrel_Vote_Push(uint8_t vote)
{
    uint8_t old_vote;

    if (barrel_win_len == 0) return;
    if (barrel_win_idx >= barrel_win_len) {
        barrel_win_idx = 0;
    }

    old_vote = barrel_vote_win[barrel_win_idx];
    if (old_vote == 1) {
        barrel_win_cnt--;
    }
    barrel_vote_win[barrel_win_idx] = vote;
    if (vote == 1) {
        barrel_win_cnt++;
    }
    if (barrel_win_cnt > barrel_win_len) {
        barrel_win_cnt = barrel_win_len;
    }
    barrel_win_idx++;
}

/* ---- 将十字投票推入环形窗口, 增量更新计数 ---- */
static void Cross_Vote_Push(uint8_t vote)
{
    uint8_t old_vote;

    if (cross_win_len == 0) return;
    if (cross_win_idx >= cross_win_len) {
        cross_win_idx = 0;
    }

    old_vote = cross_vote_win[cross_win_idx];
    if (old_vote == 1) {
        cross_win_cnt--;
    }
    cross_vote_win[cross_win_idx] = vote;
    if (vote == 1) {
        cross_win_cnt++;
    }
    if (cross_win_cnt > cross_win_len) {
        cross_win_cnt = cross_win_len;
    }
    cross_win_idx++;
}

void Element_Init(void)
{
    uint8_t i;

    /* 环岛进入 ADC 阈值 */
    Debug_Register("RingEn0", DBG_FLOAT | STATIC_TYPE, &RingEntrance[0]);
    Debug_Register("RingEn1", DBG_FLOAT | STATIC_TYPE, &RingEntrance[1]);
    Debug_Register("RingEn2", DBG_FLOAT | STATIC_TYPE, &RingEntrance[2]);
    Debug_Register("RingEn3", DBG_FLOAT | STATIC_TYPE, &RingEntrance[3]);

    /* 环岛退出 ADC 阈值 */
    Debug_Register("RingEx0", DBG_FLOAT | STATIC_TYPE, &RingExit[0]);
    Debug_Register("RingEx1", DBG_FLOAT | STATIC_TYPE, &RingExit[1]);
    Debug_Register("RingEx2", DBG_FLOAT | STATIC_TYPE, &RingExit[2]);
    Debug_Register("RingEx3", DBG_FLOAT | STATIC_TYPE, &RingExit[3]);

    /* 环岛角度阈值 */
    Debug_Register("Ring_AngThr", DBG_FLOAT | STATIC_TYPE, &angle_level);

    /* 环岛投票窗参数 (上位机可调) */
    Debug_Register("Ring_WinLen",  DBG_UINT8 | STATIC_TYPE, &ring_win_len);
    Debug_Register("Ring_EntThr",  DBG_UINT8 | STATIC_TYPE, &ring_entry_thresh);

    /* 开环PASS超时帧数 (上位机可调, 1帧=2ms) */
    Debug_Register("OL_Ang",     DBG_FLOAT | STATIC_TYPE, &g_ol_ring_angle);
    Debug_Register("OL_Yaw",     DBG_FLOAT | STATIC_TYPE, &g_ol_ring_yawrate);
    Debug_Register("PassTmOut",  DBG_UINT16 | STATIC_TYPE, &g_pass_timeout_frames);
    Debug_Register("RingSide",   DBG_UINT8 | STATIC_TYPE, &g_ring_side);
    Debug_Register("RingExYaw",  DBG_FLOAT | STATIC_TYPE, &g_ring_exit_yaw_thresh);
    Debug_Register("TblEnable",  DBG_UINT8 | STATIC_TYPE, &g_track_table_enabled);

    Debug_Register("Wall_WinLen",  DBG_UINT8 | STATIC_TYPE, &wall_win_len);
    /* 上墙倾角阈值 */
    Debug_Register("Wall_EnTilt", DBG_FLOAT | STATIC_TYPE, &wall_entry_tilt);
    Debug_Register("Wall_ExTilt", DBG_FLOAT | STATIC_TYPE, &wall_exit_tilt);

    /* 上墙投票窗参数 (上位机可调) */
    Debug_Register("Wall_EntThr",  DBG_UINT8 | STATIC_TYPE, &wall_entry_thresh);
    Debug_Register("Wall_ExtThr",  DBG_UINT8 | STATIC_TYPE, &wall_exit_thresh);

    Debug_Register("Bar_WinLen",   DBG_UINT8 | STATIC_TYPE, &barrel_win_len);
    /* 滚筒倾角阈值 */
    Debug_Register("Bar_EnTilt",  DBG_FLOAT | STATIC_TYPE, &barrel_entry_tilt);
    Debug_Register("Bar_ExTilt",  DBG_FLOAT | STATIC_TYPE, &barrel_exit_tilt);

    /* 滚筒投票窗参数 (上位机可调) */
    Debug_Register("Bar_EntThr",  DBG_UINT8 | STATIC_TYPE, &barrel_entry_thresh);
    Debug_Register("Bar_ExtThr",  DBG_UINT8 | STATIC_TYPE, &barrel_exit_thresh);


    
    Debug_Register("Cross_WinLen", DBG_UINT8 | STATIC_TYPE, &cross_win_len);
    /* 十字竖直电感和阈值 */
    Debug_Register("Cross_EnSum", DBG_FLOAT | STATIC_TYPE, &cross_entry_sum);
    Debug_Register("Cross_ExSum", DBG_FLOAT | STATIC_TYPE, &cross_exit_sum);

    /* 十字投票窗参数 (上位机可调) */
    Debug_Register("Cross_EntThr", DBG_UINT8 | STATIC_TYPE, &cross_entry_thresh);
    Debug_Register("Cross_ExtThr", DBG_UINT8 | STATIC_TYPE, &cross_exit_thresh);

    /* 上墙/滚筒/十字投票窗长度 (上位机可调, 0=旁路即时切换) */
    
    

    /* 默认阈值 */
    RingEntrance[0] = 1200.0f;
    RingEntrance[1] = 0.0f;
    RingEntrance[2] = 1200.0f;
    RingEntrance[3] = 0.0f;

    RingExit[0] = 1200.0f;
    RingExit[1] = 0.0f;
    RingExit[2] = 1200.0f;
    RingExit[3] = 0.0f;

    /* 窗长默认 10 帧 (20ms), 阈值默认 7 (70% 多数) */
    ring_win_len      = 10;
    ring_entry_thresh = 7;

    /* 上墙倾角默认: 进入 >30°, 退出 <20° (滞回) */
    wall_entry_tilt  = 30.0f;
    wall_exit_tilt   = 20.0f;

    /* 上墙投票: 窗长默认 10, 阈值 7 (70% 多数) */
    wall_win_len      = 10;
    wall_entry_thresh = 7;
    wall_exit_thresh  = 7;

    /* 滚筒倾角默认: 进入 >25°, 退出 <15° (滞回) */
    barrel_entry_tilt  = 25.0f;
    barrel_exit_tilt   = 15.0f;

    /* 滚筒投票: 窗长默认 10, 阈值 7 (70% 多数) */
    barrel_win_len      = 10;
    barrel_entry_thresh = 7;
    barrel_exit_thresh  = 7;

    /* 十字竖直电感和默认: 进入 >2000, 退出 <1500 (滞回) */
    cross_entry_sum  = 2000.0f;
    cross_exit_sum   = 1500.0f;

    /* 十字投票: 窗长默认 10, 阈值 7 (70% 多数) */
    cross_win_len      = 10;
    cross_entry_thresh = 7;
    cross_exit_thresh  = 7;

    /* 安全停车参数 (上位机在线调参) */
    Debug_Register("OffTkMs",   DBG_FLOAT | STATIC_TYPE, &g_offtrack_timeout_ms);

    /* 清零窗口缓冲区 (xdata 上电值不确定) */
    for (i = 0; i < 100; i++) {
        ring_vote_win[i] = 0;
    }
    /* 上墙窗口清零 */
    for (i = 0; i < WALL_WIN_MAX; i++) {
        wall_vote_win[i] = 0;
    }
    /* 滚筒窗口清零 */
    for (i = 0; i < BARREL_WIN_MAX; i++) {
        barrel_vote_win[i] = 0;
    }
    /* 十字窗口清零 */
    for (i = 0; i < CROSS_WIN_MAX; i++) {
        cross_vote_win[i] = 0;
    }
    /* 重力 MA 窗口清零 */
    for (i = 0; i < GRAVITY_MA_WIN_LEN; i++) {
        g_grav_win_ax[i] = 0.0f;
        g_grav_win_ay[i] = 0.0f;
        g_grav_win_az[i] = 0.0f;
    }
    ring_win_idx = 0;
    ring_win_cnt = 0;
    wall_win_idx = 0;
    wall_win_cnt = 0;
    barrel_win_idx = 0;
    barrel_win_cnt = 0;
    cross_win_idx = 0;
    cross_win_cnt = 0;
    g_grav_win_idx = 0;
    g_grav_win_cnt = 0;
    g_grav_sum_ax  = 0.0f;
    g_grav_sum_ay  = 0.0f;
    g_grav_sum_az  = 0.0f;

    /* 显式复位元素表索引 (static 变量虽默认0, 此处保证意图清晰) */
    Element_ResetTrackIndex();
}

/* ---- 重置离赛道超时计时器 (供 Change_Maunal 等外部调用) ---- */
void Element_ResetOfftrackTimer(void)
{
    g_offtrack_active = 0;
}

/* ---- 重置赛道元素表索引 (供手动切自动/冲出赛道恢复时调用) ---- */
void Element_ResetTrackIndex(void)
{
    g_track_idx = 0;
}

/* ---- 元素枚举 → 可读名称 (供 Debug_Log 使用) ---- */
static const char* Element_Name(element_t e)
{
    switch (e) {
    case ELEMENT_STRAIGHT:           return "STRAIGHT";
    case ELEMENT_RING_LEFT_ENTRANCE: return "RING_L_ENT";
    case ELEMENT_RING_RIGHT_ENTRANCE:return "RING_R_ENT";
    case ELEMENT_RING_LEFT_PASS:     return "RING_L_PASS";
    case ELEMENT_RING_RIGHT_PASS:    return "RING_R_PASS";
    case ELEMENT_IN_RING:            return "IN_RING";
    case ELEMENT_RING_LEFT_EXIT:     return "RING_L_EXIT";
    case ELEMENT_RING_RIGHT_EXIT:    return "RING_R_EXIT";
    case ELEMENT_SKIP:               return "SKIP";
    case ELEMENT_BARREL:             return "BARREL";
    case ELEMENT_WALL:               return "WALL";
    case ELEMENT_CROSS:              return "CROSS";
    case ELEMENT_SAFETY_STOP:        return "SAFETY_STOP";
    default:                         return "UNKNOWN";
    }
}

/* adc0 是左 */
void Element_Handle(element_t *element, float adc_values[], float ax, float ay, float az, float yaw)
{
    /* C89: 所有局部变量声明在开头 */
    float   fax, fay, faz;  /* MA 滤波后的重力矢量 */
    float   tilt_x;         /* 前后倾角(度): atan2(|fax|,|faz|), 替代 pitch */
    float   g_mag;          /* 重力矢量模长 */
    uint8_t ring_vote;   /* 环岛统一入口投票 (方向由元素表指定) */
    uint8_t wall_vote;
    uint8_t barrel_vote;
    uint8_t cross_vote;
    float   cross_adc_sum;  /* adc[1] + adc[3] 竖直电感和 */
    float   yaw_delta;   /* PASS 阶段 yaw 变化量 */
    uint8_t i;           /* 循环/清零用 */

    /* 安全检测用局部变量 */
    uint8_t all_adc_low;
    uint32_t elapsed_ms;
    uint32_t now_tick;
    uint8_t track_expected;

    /* 状态切换日志用 */
    element_t element_prev;
    /* ================================================================
     * 0. 安全检测层 (最高优先级, 可覆盖任意状态)
     *    离赛道超时: 4路ADC全部丢失超过阈值 → SAFETY_STOP
     * ================================================================ */

    all_adc_low = (adc_values[0] < 10.0f && adc_values[1] < 10.0f
                && adc_values[2] < 10.0f && adc_values[3] < 10.0f) ? 1 : 0;

    if (all_adc_low) {
        if (g_offtrack_active == 0) {
            g_offtrack_active = 1;
            g_offtrack_start_tick = xTaskGetTickCount();
        } else {
            now_tick = xTaskGetTickCount();
            elapsed_ms = (now_tick - g_offtrack_start_tick) * portTICK_PERIOD_MS;
            if (elapsed_ms >= (uint32_t)g_offtrack_timeout_ms) {
                *element = ELEMENT_SAFETY_STOP;
                Debug_Log("SAFETY_STOP:OFF_TRACK");
                g_offtrack_active = 0;
                return;
            }
        }
    } else {
        /* 任一 ADC 恢复 → 取消计时 */
        g_offtrack_active = 0;
    }

    /* ---- 0.5 元素表: 禁用时每周期复位索引, 确保重新启用时总是从表头开始 ---- */
    if (!g_track_table_enabled) {
        g_track_idx = 0;
    }

    /* ---- 1. 根据当前状态计算本帧投票 ---- */
    /* 重力矢量滑动平均 → 前后倾角: 防加速度计尖峰, 替代 Mahony 滤波 pitch */
    Grav_MA_Push(ax, ay, az);
    if (g_grav_win_cnt > 0) {
        fax = g_grav_sum_ax / (float)g_grav_win_cnt;
        fay = g_grav_sum_ay / (float)g_grav_win_cnt;
        faz = g_grav_sum_az / (float)g_grav_win_cnt;
    } else {
        fax = ax; fay = ay; faz = az;  /* 第一帧直接使用 */
    }
    g_mag = (float)sqrt(fax * fax + fay * fay + faz * faz);
    if (g_mag < 0.01f) { g_mag = 0.01f; }  /* 防除零 */
    tilt_x = (float)atan2((fax > 0 ? fax : -fax), (faz > 0 ? faz : -faz)) * 57.29578f;

    /* ---- 0.6 元素表: 预查当前期望元素 (所有投票类型共用) ---- */
    if (g_track_table_enabled) {
        if (g_track_idx < TRACK_TABLE_LEN) {
            track_expected = g_track_table[g_track_idx];
        } else {
            track_expected = ELEMENT_STRAIGHT;  /* 超出表尾, 不再放行任何元素 */
        }
    } else {
        track_expected = ELEMENT_STRAIGHT;  /* 未启用, 不约束 */
    }

    switch (*element) {
    case ELEMENT_STRAIGHT:
        {/* C89: 块作用域声明 */

            /* 统一环岛入口检测: 任一水平电感超阈值 且 车身水平, 方向由元素表指定 */
            ring_vote = (((adc_values[0] > RingEntrance[0]) ||
                          (adc_values[2] > RingEntrance[2])) &&
                         (tilt_x < angle_level)) ? 1 : 0;

            /* 元素表约束: 非环岛入口期强制不投票 (表禁用时不约束, 自由检测) */
            if (g_track_table_enabled) {
                if (track_expected != ELEMENT_RING_LEFT_ENTRANCE &&
                    track_expected != ELEMENT_RING_RIGHT_ENTRANCE) {
                    ring_vote = 0;
                }
            }
        }
        break;

    case ELEMENT_IN_RING:
        /* 环内等待出环: 对应通道再次出现尖峰(大于退出阈值, 与进入阈值可独立调谐形成滞回) */
        if (g_ring_side == 0) {
            ring_vote = (adc_values[2] > RingExit[2]) ? 1 : 0;
        } else {
            ring_vote = (adc_values[0] > RingExit[0]) ? 1 : 0;
        }
        break;

    default:
        ring_vote = 0;
        break;
    }

    /* ---- 2. 推入统一窗口 ---- */
    Ring_Vote_Push(ring_vote);

    /* ---- 3. 上墙投票 ---- */
    switch (*element) {
    case ELEMENT_STRAIGHT:
        /* 在直线状态下, 倾角超过进入阈值即投票 1 */
        wall_vote = (tilt_x > wall_entry_tilt) ? 1 : 0;
        /* 元素表约束: 非预期元素强制不投票 */
        if (g_track_table_enabled && track_expected != ELEMENT_WALL) wall_vote = 0;
        break;

    case ELEMENT_WALL:
        /* 在上墙状态下, 倾角低于退出阈值即投票 1 */
        wall_vote = (tilt_x < wall_exit_tilt) ? 1 : 0;
        break;

    default:
        /* 其他状态不关心上墙, 投票 0 自然老化 */
        wall_vote = 0;
        break;
    }
    Wall_Vote_Push(wall_vote);

    /* ---- 3.5 滚筒投票 (倾角 = 重力 atan2(|ax|,|az|), 角度) ---- */
    switch (*element) {
    case ELEMENT_STRAIGHT:
        /* 在直线状态下, 倾角超过进入阈值即投票 1 */
        barrel_vote = (tilt_x > barrel_entry_tilt) ? 1 : 0;
        /* 元素表约束: 非预期元素强制不投票 */
        if (g_track_table_enabled && track_expected != ELEMENT_BARREL) barrel_vote = 0;
        break;

    case ELEMENT_BARREL:
        /* 在滚筒状态下, 倾角低于退出阈值即投票 1 */
        barrel_vote = (tilt_x < barrel_exit_tilt) ? 1 : 0;
        break;

    default:
        /* 其他状态不关心滚筒, 投票 0 自然老化 */
        barrel_vote = 0;
        break;
    }
    Barrel_Vote_Push(barrel_vote);

    /* ---- 3.6 十字投票 (竖直电感 adc[1]+adc[3] 之和) ---- */
    cross_adc_sum = adc_values[1] + adc_values[3];
    switch (*element) {
    case ELEMENT_STRAIGHT:
        /* 在直线状态下, 竖直电感和超过进入阈值即投票 1 */
        cross_vote = (cross_adc_sum > cross_entry_sum) ? 1 : 0;
        /* 元素表约束: 非预期元素强制不投票 */
        if (g_track_table_enabled && track_expected != ELEMENT_CROSS) cross_vote = 0;
        break;

    case ELEMENT_CROSS:
        /* 在十字状态下, 竖直电感和低于退出阈值即投票 1 */
        cross_vote = (cross_adc_sum < cross_exit_sum) ? 1 : 0;
        break;

    default:
        /* 其他状态不关心十字, 投票 0 自然老化 */
        cross_vote = 0;
        break;
    }
    Cross_Vote_Push(cross_vote);

    /* ---- 4. 状态转换判断 ---- */
    element_prev = *element;
    switch (*element) {
    case ELEMENT_STRAIGHT:
        if (ring_win_len == 0) {
            /* 旁路: 即时切换, 方向由元素表指定 (表启用但非环岛期则忽略; 表禁用时右环优先) */
            if (((adc_values[0] > RingEntrance[0]) || (adc_values[2] > RingEntrance[2])) &&
                (tilt_x < angle_level)) {
                if (g_track_table_enabled) {
                    if (track_expected == ELEMENT_RING_RIGHT_ENTRANCE) {
                        *element = ELEMENT_RING_RIGHT_ENTRANCE;
                    } else if (track_expected == ELEMENT_RING_LEFT_ENTRANCE) {
                        *element = ELEMENT_RING_LEFT_ENTRANCE;
                    }
                    /* else: 表指示非环岛期, 忽略此尖峰 */
                } else {
                    /* 表禁用 fallback: 右环优先 */
                    if (adc_values[0] > RingEntrance[0]) {
                        *element = ELEMENT_RING_RIGHT_ENTRANCE;
                    } else {
                        *element = ELEMENT_RING_LEFT_ENTRANCE;
                    }
                }
            }
        } else {
            /* 多数投票: 统一窗票数达标, 方向由元素表指定 (表禁用时右环优先) */
            if (ring_win_cnt >= ring_entry_thresh) {
                if (g_track_table_enabled) {
                    if (track_expected == ELEMENT_RING_RIGHT_ENTRANCE) {
                        *element = ELEMENT_RING_RIGHT_ENTRANCE;
                    } else {
                        *element = ELEMENT_RING_LEFT_ENTRANCE;
                    }
                } else {
                    if (adc_values[0] > RingEntrance[0]) {
                        *element = ELEMENT_RING_RIGHT_ENTRANCE;
                    } else {
                        *element = ELEMENT_RING_LEFT_ENTRANCE;
                    }
                }
            }
        }

        /* 上墙检测: 仅从直线状态进入, 且未被环岛抢占 */
        if (*element == ELEMENT_STRAIGHT) {
            if (wall_win_len == 0) {
                /* 旁路: 即时判断倾角 */
                if (tilt_x > wall_entry_tilt) {
                    *element = ELEMENT_WALL;
                }
            } else if (wall_win_cnt >= wall_entry_thresh) {
                *element = ELEMENT_WALL;
            }
        }

        /* 滚筒检测: 仅从直线状态进入, 且未被环岛/上墙抢占 */
        if (*element == ELEMENT_STRAIGHT) {
            if (barrel_win_len == 0) {
                /* 旁路: 即时判断倾角 */
                if (tilt_x > barrel_entry_tilt) {
                    *element = ELEMENT_BARREL;
                }
            } else if (barrel_win_cnt >= barrel_entry_thresh) {
                *element = ELEMENT_BARREL;
            }
        }

        /* 十字检测: 仅从直线状态进入, 且未被环岛/上墙/滚筒抢占 */
        if (*element == ELEMENT_STRAIGHT) {
            if (cross_win_len == 0) {
                /* 旁路: 即时判断竖直电感和 */
                if (cross_adc_sum > cross_entry_sum) {
                    *element = ELEMENT_CROSS;
                }
            } else if (cross_win_cnt >= cross_entry_thresh) {
                *element = ELEMENT_CROSS;
            }
        }
        break;

    case ELEMENT_RING_LEFT_ENTRANCE:
        /* 确认进入环岛, 立即切入开环PASS阶段 */
        g_pass_entry_yaw = yaw;
        g_pass_frame_cnt = 0;
        *element = ELEMENT_RING_LEFT_PASS;
        break;

    case ELEMENT_RING_RIGHT_ENTRANCE:
        /* 确认进入环岛, 立即切入开环PASS阶段 */
        g_pass_entry_yaw = yaw;
        g_pass_frame_cnt = 0;
        *element = ELEMENT_RING_RIGHT_PASS;
        break;

    case ELEMENT_RING_LEFT_PASS:
        {
            g_pass_frame_cnt++;
            yaw_delta = yaw - g_pass_entry_yaw;
            /* 左转角度达标(负向) OR 超时 → 进入环内, 清投票窗 */
            if (yaw_delta <= -g_ol_ring_angle ||
                g_pass_frame_cnt >= g_pass_timeout_frames) {
                *element = ELEMENT_IN_RING;
                g_ring_side = 0;  /* 左环, 出环检测看 adc[2] */
                g_ring_entry_yaw = yaw;  /* 记录入环 yaw, 用于退环门禁 */
                for (i = 0; i < ring_win_len; i++) {
                    ring_vote_win[i] = 0;
                }
                ring_win_cnt = 0;
                ring_win_idx = 0;
            }
        }
        break;

    case ELEMENT_RING_RIGHT_PASS:
        {
            g_pass_frame_cnt++;
            yaw_delta = yaw - g_pass_entry_yaw;
            /* 右转角度达标(正向) OR 超时 → 进入环内, 清投票窗 */
            if (yaw_delta >= g_ol_ring_angle ||
                g_pass_frame_cnt >= g_pass_timeout_frames) {
                *element = ELEMENT_IN_RING;
                g_ring_side = 1;  /* 右环, 出环检测看 adc[0] */
                g_ring_entry_yaw = yaw;  /* 记录入环 yaw, 用于退环门禁 */
                for (i = 0; i < ring_win_len; i++) {
                    ring_vote_win[i] = 0;
                }
                ring_win_cnt = 0;
                ring_win_idx = 0;
            }
        }
        break;

    case ELEMENT_IN_RING:
        /* 环内正常循迹, 等待对应通道第二个 ADC 尖峰(出环)
         * 前置门禁: yaw 累计变化量达标才允许出环
         *   左转 yaw 减小(负向) → 左环看负向; 右转 yaw 增加(正向) → 右环看正向 */
        yaw_delta = yaw - g_ring_entry_yaw;
        if (g_ring_side == 0) {
            /* 左环: 左转 yaw 减小, yaw_delta 为负, 累计左转足够 → yaw_delta <= -阈值 */
            if (yaw_delta > -g_ring_exit_yaw_thresh) {
                break;
            }
        } else {
            /* 右环: 右转 yaw 增加, yaw_delta 为正, 累计右转足够 → yaw_delta >= 阈值 */
            if (yaw_delta < g_ring_exit_yaw_thresh) {
                break;
            }
        }

        if (ring_win_len == 0) {
            /* 旁路: 即时判断, 使用退出阈值(与进入阈值独立) */
            if (g_ring_side == 0) {
                if (adc_values[2] > RingExit[2]) {
                    *element = ELEMENT_STRAIGHT;
                    Track_Advance();
                }
            } else {
                if (adc_values[0] > RingExit[0]) {
                    *element = ELEMENT_STRAIGHT;
                    Track_Advance();
                }
            }
        } else {
            /* 多数投票: 统一窗票数达标即出环 (出环方向已在投票生成时由 g_ring_side 区分) */
            if (ring_win_cnt >= ring_entry_thresh) {
                *element = ELEMENT_STRAIGHT;
                Track_Advance();
            }
        }
        break;

    case ELEMENT_RING_LEFT_EXIT:
        break;

    case ELEMENT_RING_RIGHT_EXIT:
        break;

    case ELEMENT_SKIP:
        break;

    case ELEMENT_BARREL:
        /* 滚筒状态下, 倾角低于退出阈值 → 退回直线 */
        if (barrel_win_len == 0) {
            /* 旁路: 即时判断倾角 */
            if (tilt_x < barrel_exit_tilt) {
                *element = ELEMENT_STRAIGHT;
                Track_Advance();
            }
        } else if (barrel_win_cnt >= barrel_exit_thresh) {
            *element = ELEMENT_STRAIGHT;
            Track_Advance();
        }
        break;

    case ELEMENT_WALL:
        /* 上墙状态下, 倾角低于退出阈值 → 退回直线 */
        if (wall_win_len == 0) {
            /* 旁路: 即时判断倾角 */
            if (tilt_x < wall_exit_tilt) {
                *element = ELEMENT_STRAIGHT;
                Track_Advance();
            }
        } else if (wall_win_cnt >= wall_exit_thresh) {
            *element = ELEMENT_STRAIGHT;
            Track_Advance();
        }
        break;

    case ELEMENT_CROSS:
        /* 十字状态下, 竖直电感和低于退出阈值 → 退回直线 */
        if (cross_win_len == 0) {
            /* 旁路: 即时判断竖直电感和 */
            if (cross_adc_sum < cross_exit_sum) {
                *element = ELEMENT_STRAIGHT;
                Track_Advance();
            }
        } else if (cross_win_cnt >= cross_exit_thresh) {
            *element = ELEMENT_STRAIGHT;
            Track_Advance();
        }
        break;

    case ELEMENT_SAFETY_STOP:
        /* 任一通道恢复电感值 → 退出安全停车, 切回手动模式 */
        if (adc_values[0] > 10.0f || adc_values[1] > 10.0f
            || adc_values[2] > 10.0f || adc_values[3] > 10.0f) {
            *element = ELEMENT_STRAIGHT;
            manual_control_flag = 1;
            LED_RequestBreathing();          /* 冲出赛道恢复 → 呼吸灯 */
            g_offtrack_active = 0;  /* 确保离赛道定时器复位, 防止切回自动模式时误触发 */
            Element_ResetTrackIndex();  /* 冲出赛道后从表头重新开始 */
        }
        break;

    default:
        break;
    }

    /* ---- 5. 状态切换日志 ---- */
    if (*element != element_prev) {
        Debug_Log(Element_Name(*element));
    }
}
