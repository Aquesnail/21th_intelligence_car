#include "element.h"
#include "debug_arch.h"
#include "FreeRTOS.h"
#include "task.h"
#include "led_indicator.h"
#include <math.h>

extern unsigned char manual_control_flag;

/* ================================================================
 * 环岛 ADC 阈值
 *   RingEntrance[x]: ADC 高于此值 → 候选进入环岛
 *   RingExit[x]:     ADC 高于此值 → 候选退出环岛 (与进入阈值独立可调, 形成滞回)
 *   进入/退出之间形成滞回区, 防止边界抖动
 * ================================================================ */
static float RingEntrance[4];
static float RingExit[4];
static float angle_level = 15.0f;   /* 车身水平倾角阈值(度), 重力分量 atan2(|ax|,|az|) */

/* ================================================================
 * 环岛多数投票窗缓冲区
 * ================================================================ */
static unsigned char xdata ring_vote_buf[100];

/* 通用投票窗实例 (Phase 1: 指向现有缓冲区, 逐步替代旧变量) */
static VoteWindow_t vote_win_ring;

/* ================================================================
 * 开环 PASS 阶段参数 (左右环共用, 左环取负值)
 *   进入 ENTRANCE 后立即切入 PASS, 记录起始 yaw, 帧计数
 *   退出条件: yaw 变化量达标 OR 帧数超时
 * ================================================================ */
float    g_ol_ring_angle   = 45.0f;   /* 环岛开环目标角度(绝对值, 度) */
float    g_ol_ring_yawrate = 3.0f;    /* 环岛开环目标角速度(绝对值, °/s) */
static unsigned short g_pass_timeout_frames = 150;  /* 默认150帧=300ms, 上位机可调 */

/* 退环 yaw 累计门禁: 入环后 yaw 变化量达标才允许出环 */
static float    g_ring_exit_yaw_thresh  = 200.0f;  /* 退环所需 yaw 累计角度(绝对值, 度) */

/* 出环锁存: 退出环岛后若干帧内禁止再次进入, 防出环尖峰误触发二次进环 */
static unsigned short g_ring_exit_lockout_frames = 1000;  /* 锁存时长(帧), 上位机可调, 默认1000帧=2s */

/* ================================================================
 * 赛道元素顺序表 (写死在 flash 中, 按赛道实际顺序排列)
 *   启用后, STRAIGHT 下只对表指定的方向投票, 杜绝方向误判
 *   每完成一个元素退回到 STRAIGHT 后自动推进索引
 *   索引超出表尾后不再约束 (赛道跑完)
 * ================================================================ */
#define TRACK_TABLE_LEN  8  /* 根据实际赛道元素数量修改 */

static const unsigned char g_track_table[TRACK_TABLE_LEN] = {
    ELEMENT_RING_LEFT_ENTRANCE,
   /* ELEMENT_CROSS, */
   /* ELEMENT_CROSS, */
   /* ELEMENT_WALL, */
   /* ELEMENT_BARREL */
};

static unsigned char  g_track_table_enabled = 1;   /* 上位机可调: 1=启用, 0=旁路(单元素调试) */
static unsigned char  g_track_idx = 0;             /* 指向下一个预期元素的索引 */
static element_t g_single_element_debug = ELEMENT_STRAIGHT; /* 单元素调试: track_table_enabled=0时反复触发 */

/* 上墙/滚筒/十字投票窗最大长度 (数组容量, 上位机可在 0..MAX 范围内调窗长) */
#define WALL_WIN_MAX    20
#define BARREL_WIN_MAX  20
#define CROSS_WIN_MAX   20

/* ================================================================
 * 上墙检测缓冲区 + 投票窗实例
 * ================================================================ */
static unsigned char xdata wall_vote_buf[WALL_WIN_MAX];
static float   wall_entry_tilt;             /* 进入上墙的倾角阈值(度) */
static float   wall_exit_tilt;              /* 退出上墙的倾角阈值(度) */
static VoteWindow_t vote_win_wall;

/* ================================================================
 * 滚筒检测缓冲区 + 投票窗实例
 * ================================================================ */
static unsigned char xdata barrel_vote_buf[BARREL_WIN_MAX];
static float   barrel_entry_tilt;
static float   barrel_exit_tilt;
static VoteWindow_t vote_win_barrel;

/* ================================================================
 * 十字检测缓冲区 + 投票窗实例
 * ================================================================ */
static unsigned char xdata cross_vote_buf[CROSS_WIN_MAX];
static float   cross_entry_sum;
static float   cross_exit_sum;
static VoteWindow_t vote_win_cross;

/* ================================================================
 * 重力矢量滑动平均窗 (Moving Average, 防加速度计尖峰)
 *   窗长可调 (GRAVITY_MA_WIN_LEN 宏, 默认60帧=120ms)
 *   增量维护累加和, O(1) 无遍历
 * ================================================================ */
#define GRAVITY_MA_WIN_LEN  60

static float xdata g_grav_win_ax[GRAVITY_MA_WIN_LEN];
static float xdata g_grav_win_ay[GRAVITY_MA_WIN_LEN];
static float xdata g_grav_win_az[GRAVITY_MA_WIN_LEN];
static unsigned char g_grav_win_idx;    /* 当前写入位置 (循环) */
static unsigned char g_grav_win_cnt;    /* 已累积样本数 (<= WIN_LEN) */
static float   g_grav_sum_ax;           /* 窗口内累加和 */
static float   g_grav_sum_ay;
static float   g_grav_sum_az;

/* ================================================================
 * 安全停车检测 (Safety Stop)
 *   离赛道超时: 4路ADC全部丢失超过 g_offtrack_timeout_ms → SAFETY_STOP
 * ================================================================ */
static float   g_offtrack_timeout_ms   = 300.0f;  /* 离赛道超时 (ms) */
static unsigned long g_offtrack_start_tick  = 0;  /* 信号丢失起始 tick */
static unsigned char  g_offtrack_active      = 0; /* 当前是否正在计时 */

/* ================================================================
 * 速度变量 (供元素属性表 speed_ptr 指向 + Debug_Register 在线调参)
 *   Phase 2: 从 app_main_control.c 迁移过来, 作为单一真相来源
 * ================================================================ */
static float g_speed_cruising = 50.0f;
static float g_speed_ring     = 50.0f;
static float g_speed_wall     = 45.0f;
static float g_speed_barrel   = 45.0f;
static float g_speed_cross    = 45.0f;

/* ================================================================
 * 元素属性表 (Flash 常驻, 按 ELEMENT_ACTIVE_COUNT 索引)
 *   Phase 2: 替代分散在 app_main_control.c 和 led_indicator.c 中的 switch-case
 * ================================================================ */
static const ElementProp_t g_element_props[ELEMENT_ACTIVE_COUNT] = {
    /* [0] ELEMENT_STRAIGHT */
    { ELEMENT_STRAIGHT, "STRAIGHT", &g_speed_cruising, 255, 255, 255, CTRL_NORMAL, NULL, 0 },
    /* [1] ELEMENT_RING_LEFT_ENTRANCE */
    { ELEMENT_RING_LEFT_ENTRANCE, "RING_L_ENT", &g_speed_ring, 0, 0, 255, CTRL_NORMAL, NULL, 0 },
    /* [2] ELEMENT_RING_RIGHT_ENTRANCE */
    { ELEMENT_RING_RIGHT_ENTRANCE,"RING_R_ENT",&g_speed_ring, 0, 0, 255, CTRL_NORMAL, NULL, 0 },
    /* [3] ELEMENT_RING_LEFT_PASS */
    { ELEMENT_RING_LEFT_PASS, "RING_L_PASS", &g_speed_ring, 0, 255, 255, CTRL_OL_YAW, &g_ol_ring_yawrate,  1 },
    /* [4] ELEMENT_RING_RIGHT_PASS */
    { ELEMENT_RING_RIGHT_PASS,"RING_R_PASS",&g_speed_ring, 0, 255, 255, CTRL_OL_YAW, &g_ol_ring_yawrate, -1 },
    /* [5] ELEMENT_IN_RING */
    { ELEMENT_IN_RING, "IN_RING", &g_speed_ring, 255, 255, 0, CTRL_NORMAL, NULL, 0 },
    /* [6] ELEMENT_BARREL */
    { ELEMENT_BARREL, "BARREL", &g_speed_barrel, 255, 0, 255, CTRL_NORMAL, NULL, 0 },
    /* [7] ELEMENT_WALL */
    { ELEMENT_WALL, "WALL", &g_speed_wall, 255, 128, 0, CTRL_NORMAL, NULL, 0 },
    /* [8] ELEMENT_CROSS */
    { ELEMENT_CROSS, "CROSS", &g_speed_cross, 0, 255, 0, CTRL_NORMAL, NULL, 0 },
    /* [9] ELEMENT_SAFETY_STOP */
    { ELEMENT_SAFETY_STOP, "SAFETY_STOP", NULL, 255, 0, 0, CTRL_BRAKE, NULL, 0 },
};

/* ================================================================
 * 通用投票窗 API (Phase 1)
 * ================================================================ */

/* ---- 初始化投票窗: 绑定缓冲区, 清零状态 ---- */
void VoteWin_Init(VoteWindow_t *w, unsigned char xdata *buf, unsigned char max_len)
{
    unsigned char i;

    w->buf     = buf;
    w->max_len = max_len;
    w->idx     = 0;
    w->cnt     = 0;
    w->len     = 10;          /* 默认窗长 10 帧 */
    w->entry_thresh = 7;      /* 默认 70% 多数 */
    w->exit_thresh  = 7;

    /* 清零缓冲区 */
    for (i = 0; i < max_len; i++) {
        buf[i] = 0;
    }
}

/* ---- 推入一票, O(1) 增量维护 ---- */
void VoteWin_Push(VoteWindow_t *w, unsigned char vote)
{
    unsigned char old_vote;

    if (w->len == 0) return;
    if (w->idx >= w->len) {
        w->idx = 0;
    }

    old_vote = w->buf[w->idx];
    if (old_vote == 1) {
        w->cnt--;
    }
    w->buf[w->idx] = vote;
    if (vote == 1) {
        w->cnt++;
    }
    /* 防止上位机调小窗长后计数超过窗长, 导致 unsigned char 减法溢出 */
    if (w->cnt > w->len) {
        w->cnt = w->len;
    }
    w->idx++;
}

/* ---- 查询投票是否达标 ---- */
unsigned char VoteWin_Reached(VoteWindow_t *w, unsigned char thresh)
{
    return (w->cnt >= thresh) ? 1 : 0;
}

/* ---- 清零窗口 (状态切换时冲刷旧票) ---- */
void VoteWin_Reset(VoteWindow_t *w)
{
    unsigned char i;

    for (i = 0; i < w->len; i++) {
        w->buf[i] = 0;
    }
    w->cnt = 0;
    w->idx = 0;
}

/* ---- 调整窗长 (上位机在线调参) ---- */
void VoteWin_SetLen(VoteWindow_t *w, unsigned char len)
{
    if (len > w->max_len) {
        len = w->max_len;
    }
    w->len = len;
    /* 窗长变化后需清零, 避免残留旧票 */
    VoteWin_Reset(w);
}

/* ---- 元素表: 完成一个元素后推进索引, 超出表尾后不再推进(赛道跑完) ---- */
static void Track_Advance(void)
{
    if (g_track_table_enabled && g_track_idx < TRACK_TABLE_LEN) {
        g_track_idx++;
    }
}

/* ---- 统一查询: 下一个期望元素 (正式模式→查顺序表, 调试模式→返回单元素) ---- */
static element_t Track_GetExpected(ElementCtx_t *ctx)
{
    (void)ctx;

    if (g_track_table_enabled) {
        /* 正式模式: 查赛道顺序表 */
        if (g_track_idx < TRACK_TABLE_LEN) {
            return (element_t)g_track_table[g_track_idx];
        }
        return ELEMENT_STRAIGHT;  /* 表跑完, 无约束 */
    } else {
        /* 单元素调试模式: 反复触发同一个元素 */
        return g_single_element_debug;
    }
}

/* ---- 统一入口把关: 查赛道表是否允许进入指定元素, 封装 Track_GetExpected + 比较 ---- */
static unsigned char Track_CheckExpected(ElementCtx_t *ctx, element_t expected)
{
    element_t exp;
    exp = Track_GetExpected(ctx);
    return (exp == expected) ? 1 : 0;
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

/* ---- 元素枚举 → 可读名称 (供 Debug_Log 使用) ---- */
static const char* Element_Name(element_t e)
{
    if (e < ELEMENT_ACTIVE_COUNT) {
        return g_element_props[e].name;
    }
    switch (e) {
    case ELEMENT_RING_LEFT_EXIT:     return "RING_L_EXIT";
    case ELEMENT_RING_RIGHT_EXIT:    return "RING_R_EXIT";
    case ELEMENT_SKIP:               return "SKIP";
    default:                         return "UNKNOWN";
    }
}

/* ---- 元素属性查询 ---- */
const ElementProp_t* Element_GetProp(element_t e)
{
    if (e >= ELEMENT_ACTIVE_COUNT) return NULL;
    return &g_element_props[e];
}

void Element_Init(ElementCtx_t *ctx)
{
    unsigned char i;

    /* Phase 3: 初始化上下文 (NULL = 兼容旧接口, 仅初始化静态变量) */
    if (ctx != NULL) {
        ctx->ring_side        = 0;
        ctx->pass_frame_cnt   = 0;
        ctx->ring_entry_yaw   = 0.0f;
        ctx->exit_lockout_cnt = 0;
        ctx->track_idx        = 0;
        ctx->track_table_enabled = 1;
        ctx->offtrack_active     = 0;
        ctx->offtrack_start_tick = 0;
        ctx->current = ELEMENT_STRAIGHT;
        ctx->prev    = ELEMENT_STRAIGHT;
        ctx->frame_cnt = 0;
        ctx->tilt_x    = 0.0f;

        /* 绑定投票窗: 共享底层缓冲区 */
        ctx->vote_ring   = vote_win_ring;
        ctx->vote_wall   = vote_win_wall;
        ctx->vote_barrel = vote_win_barrel;
        ctx->vote_cross  = vote_win_cross;
    }

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
    Debug_Register("Ring_WinLen",  DBG_UINT8 | STATIC_TYPE, &vote_win_ring.len);
    Debug_Register("Ring_EntThr",  DBG_UINT8 | STATIC_TYPE, &vote_win_ring.entry_thresh);

    /* 开环PASS超时帧数 (上位机可调, 1帧=2ms) */
    Debug_Register("OL_Yaw",     DBG_FLOAT | STATIC_TYPE, &g_ol_ring_yawrate);
    Debug_Register("PassTmOut",  DBG_UINT16 | STATIC_TYPE, &g_pass_timeout_frames);
    Debug_Register("RingExYaw",  DBG_FLOAT | STATIC_TYPE, &g_ring_exit_yaw_thresh);
    Debug_Register("RingExLock", DBG_UINT16 | STATIC_TYPE, &g_ring_exit_lockout_frames);
    Debug_Register("TblEnable",  DBG_UINT8 | STATIC_TYPE, &g_track_table_enabled);
    Debug_Register("DbgElem",    DBG_UINT8 | STATIC_TYPE, &g_single_element_debug);

    Debug_Register("Wall_WinLen",  DBG_UINT8 | STATIC_TYPE, &vote_win_wall.len);
    /* 上墙倾角阈值 */
    Debug_Register("Wall_EnTilt", DBG_FLOAT | STATIC_TYPE, &wall_entry_tilt);
    Debug_Register("Wall_ExTilt", DBG_FLOAT | STATIC_TYPE, &wall_exit_tilt);

    /* 上墙投票窗参数 (上位机可调) */
    Debug_Register("Wall_EntThr",  DBG_UINT8 | STATIC_TYPE, &vote_win_wall.entry_thresh);
    Debug_Register("Wall_ExtThr",  DBG_UINT8 | STATIC_TYPE, &vote_win_wall.exit_thresh);

    Debug_Register("Bar_WinLen",   DBG_UINT8 | STATIC_TYPE, &vote_win_barrel.len);
    /* 滚筒倾角阈值 */
    Debug_Register("Bar_EnTilt",  DBG_FLOAT | STATIC_TYPE, &barrel_entry_tilt);
    Debug_Register("Bar_ExTilt",  DBG_FLOAT | STATIC_TYPE, &barrel_exit_tilt);

    /* 滚筒投票窗参数 (上位机可调) */
    Debug_Register("Bar_EntThr",  DBG_UINT8 | STATIC_TYPE, &vote_win_barrel.entry_thresh);
    Debug_Register("Bar_ExtThr",  DBG_UINT8 | STATIC_TYPE, &vote_win_barrel.exit_thresh);

    Debug_Register("Cross_WinLen", DBG_UINT8 | STATIC_TYPE, &vote_win_cross.len);
    /* 十字竖直电感和阈值 */
    Debug_Register("Cross_EnSum", DBG_FLOAT | STATIC_TYPE, &cross_entry_sum);
    Debug_Register("Cross_ExSum", DBG_FLOAT | STATIC_TYPE, &cross_exit_sum);

    /* 十字投票窗参数 (上位机可调) */
    Debug_Register("Cross_EntThr", DBG_UINT8 | STATIC_TYPE, &vote_win_cross.entry_thresh);
    Debug_Register("Cross_ExtThr", DBG_UINT8 | STATIC_TYPE, &vote_win_cross.exit_thresh);

    /* 安全停车参数 (上位机在线调参) */
    Debug_Register("OffTkMs",   DBG_FLOAT | STATIC_TYPE, &g_offtrack_timeout_ms);

    /* 速度变量 (上位机在线调参) */
    Debug_Register("Cruising", DBG_FLOAT | STATIC_TYPE, &g_speed_cruising);
    Debug_Register("RingSpd",  DBG_FLOAT | STATIC_TYPE, &g_speed_ring);
    Debug_Register("WallSpd",  DBG_FLOAT | STATIC_TYPE, &g_speed_wall);
    Debug_Register("BarrelSpd",DBG_FLOAT | STATIC_TYPE, &g_speed_barrel);
    Debug_Register("CrossSpd", DBG_FLOAT | STATIC_TYPE, &g_speed_cross);

    /* 默认阈值 */
    RingEntrance[0] = 1200.0f;
    RingEntrance[1] = 0.0f;
    RingEntrance[2] = 1200.0f;
    RingEntrance[3] = 0.0f;

    RingExit[0] = 1000.0f;
    RingExit[1] = 1000.0f;
    RingExit[2] = 1000.0f;
    RingExit[3] = 1000.0f;

    /* 初始化投票窗: 绑定缓冲区 */
    VoteWin_Init(&vote_win_ring,   ring_vote_buf,   100);
    VoteWin_Init(&vote_win_wall,   wall_vote_buf,   WALL_WIN_MAX);
    VoteWin_Init(&vote_win_barrel, barrel_vote_buf, BARREL_WIN_MAX);
    VoteWin_Init(&vote_win_cross,  cross_vote_buf,  CROSS_WIN_MAX);

    /* 覆盖各元素特定默认值 */
    /* 环岛: 窗长 10, 进入阈值 3 */
    vote_win_ring.len          = 10;
    vote_win_ring.entry_thresh = 3;

    /* 上墙倾角默认: 进入 >30°, 退出 <20° (滞回) */
    wall_entry_tilt  = 30.0f;
    wall_exit_tilt   = 20.0f;
    vote_win_wall.entry_thresh = 7;
    vote_win_wall.exit_thresh  = 7;

    /* 滚筒倾角默认: 进入 >25°, 退出 <15° (滞回) */
    barrel_entry_tilt  = 25.0f;
    barrel_exit_tilt   = 15.0f;
    vote_win_barrel.entry_thresh = 7;
    vote_win_barrel.exit_thresh  = 7;

    /* 十字竖直电感和默认: 进入 >5020000, 退出 <20000 (滞回) */
    cross_entry_sum  = 5020000.0f;
    cross_exit_sum   = 20000.0f;
    vote_win_cross.entry_thresh = 7;
    vote_win_cross.exit_thresh  = 7;

    /* 重力 MA 窗口清零 */
    for (i = 0; i < GRAVITY_MA_WIN_LEN; i++) {
        g_grav_win_ax[i] = 0.0f;
        g_grav_win_ay[i] = 0.0f;
        g_grav_win_az[i] = 0.0f;
    }
    g_grav_win_idx = 0;
    g_grav_win_cnt = 0;
    g_grav_sum_ax  = 0.0f;
    g_grav_sum_ay  = 0.0f;
    g_grav_sum_az  = 0.0f;

    /* 显式复位元素表索引 */
    Element_ResetTrackIndex(ctx);
}

/* ---- 重置离赛道超时计时器 (供 Change_Maunal 等外部调用) ---- */
void Element_ResetOfftrackTimer(ElementCtx_t *ctx)
{
    (void)ctx;
    g_offtrack_active = 0;
}

/* ---- 重置赛道元素表索引 (供手动切自动/冲出赛道恢复时调用) ---- */
void Element_ResetTrackIndex(ElementCtx_t *ctx)
{
    (void)ctx;
    g_track_idx = 0;
}



/* ================================================================
 * Element_Update — 基于转移表的统一状态机
 * ================================================================ */

/* ---- 转移条件/动作函数签名 ---- */
typedef unsigned char (*TransitionCheck)(ElementCtx_t *ctx);
typedef void (*TransitionAction)(ElementCtx_t *ctx);

typedef struct {
    element_t        from;
    element_t        to;
    unsigned char    priority;
    TransitionCheck  check;
    TransitionAction on_enter;
} Transition_t;

/* ================================================================
 * 条件函数
 * ================================================================ */

/* ---- 环岛入口统一物理检测: 任意水平电感 + 车身水平 + 退出锁存 + 投票达标 ---- */
static unsigned char Check_RingEntry_Physical(ElementCtx_t *ctx)
{
    if (ctx->exit_lockout_cnt > 0) {
        ctx->exit_lockout_cnt--;
        return 0;
    }
    if (ctx->vote_ring.len == 0) {
        return ((ctx->adc[0] > RingEntrance[0] || ctx->adc[2] > RingEntrance[2])
                && ctx->tilt_x < angle_level) ? 1 : 0;
    }
    return VoteWin_Reached(&ctx->vote_ring, ctx->vote_ring.entry_thresh);
}

/* ---- 环岛左入口: 赛道表把关 + 统一物理检测 ---- */
static unsigned char Check_RingLeftEntry(ElementCtx_t *ctx)
{
    if (!Track_CheckExpected(ctx, ELEMENT_RING_LEFT_ENTRANCE)) return 0;
    return Check_RingEntry_Physical(ctx);
}

/* ---- 环岛右入口: 赛道表把关 + 统一物理检测 ---- */
static unsigned char Check_RingRightEntry(ElementCtx_t *ctx)
{
    if (!Track_CheckExpected(ctx, ELEMENT_RING_RIGHT_ENTRANCE)) return 0;
    return Check_RingEntry_Physical(ctx);
}

/* ---- 上墙进入: 赛道表把关 + 倾角检测 ---- */
static unsigned char Check_WallEntry(ElementCtx_t *ctx)
{
    if (!Track_CheckExpected(ctx, ELEMENT_WALL)) return 0;

    if (ctx->vote_wall.len == 0) {
        return (ctx->tilt_x > wall_entry_tilt) ? 1 : 0;
    }
    return VoteWin_Reached(&ctx->vote_wall, ctx->vote_wall.entry_thresh);
}

/* ---- 上墙退出 ---- */
static unsigned char Check_WallExit(ElementCtx_t *ctx)
{
    if (ctx->vote_wall.len == 0) {
        return (ctx->tilt_x < wall_exit_tilt) ? 1 : 0;
    }
    return VoteWin_Reached(&ctx->vote_wall, ctx->vote_wall.exit_thresh);
}

/* ---- 滚筒进入: 赛道表把关 + 倾角检测 ---- */
static unsigned char Check_BarrelEntry(ElementCtx_t *ctx)
{
    if (!Track_CheckExpected(ctx, ELEMENT_BARREL)) return 0;

    if (ctx->vote_barrel.len == 0) {
        return (ctx->tilt_x > barrel_entry_tilt) ? 1 : 0;
    }
    return VoteWin_Reached(&ctx->vote_barrel, ctx->vote_barrel.entry_thresh);
}

/* ---- 滚筒退出 ---- */
static unsigned char Check_BarrelExit(ElementCtx_t *ctx)
{
    if (ctx->vote_barrel.len == 0) {
        return (ctx->tilt_x < barrel_exit_tilt) ? 1 : 0;
    }
    return VoteWin_Reached(&ctx->vote_barrel, ctx->vote_barrel.exit_thresh);
}

/* ---- 十字进入: 赛道表把关 + 竖直电感和检测 ---- */
static unsigned char Check_CrossEntry(ElementCtx_t *ctx)
{
    float cross_adc_sum;

    if (!Track_CheckExpected(ctx, ELEMENT_CROSS)) return 0;

    cross_adc_sum = ctx->adc[1] + ctx->adc[3];
    if (ctx->vote_cross.len == 0) {
        return (cross_adc_sum > cross_entry_sum) ? 1 : 0;
    }
    return VoteWin_Reached(&ctx->vote_cross, ctx->vote_cross.entry_thresh);
}

/* ---- 十字退出 ---- */
static unsigned char Check_CrossExit(ElementCtx_t *ctx)
{
    float cross_adc_sum;
    cross_adc_sum = ctx->adc[1] + ctx->adc[3];
    if (ctx->vote_cross.len == 0) {
        return (cross_adc_sum < cross_exit_sum) ? 1 : 0;
    }
    return VoteWin_Reached(&ctx->vote_cross, ctx->vote_cross.exit_thresh);
}

/* ---- PASS 帧超时 ---- */
static unsigned char Check_PassTimeout(ElementCtx_t *ctx)
{
    ctx->pass_frame_cnt++;
    return (ctx->pass_frame_cnt >= g_pass_timeout_frames) ? 1 : 0;
}

/* ---- 环岛退出 (yaw 门禁 + 投票) ---- */
static unsigned char Check_RingExit(ElementCtx_t *ctx)
{
    float yaw_delta;
    unsigned char ring_vote;

    /* yaw 门禁: 累计转向不足则不允许出环 */
    yaw_delta = ctx->yaw - ctx->ring_entry_yaw;
    if (ctx->ring_side == 0) {
        /* 左环: 左转 yaw 增加 */
        if (yaw_delta < g_ring_exit_yaw_thresh) return 0;
    } else {
        /* 右环: 右转 yaw 减小 */
        if (yaw_delta > -g_ring_exit_yaw_thresh) return 0;
    }

    /* 投票判断 */
    if (ctx->vote_ring.len == 0) {
        if (ctx->ring_side == 0) {
            ring_vote = (ctx->adc[2] > RingExit[2]) ? 1 : 0;
        } else {
            ring_vote = (ctx->adc[0] > RingExit[0]) ? 1 : 0;
        }
        return ring_vote;
    }
    return VoteWin_Reached(&ctx->vote_ring, ctx->vote_ring.entry_thresh);
}

/* ================================================================
 * 进入动作函数
 * ================================================================ */

/* ---- 进入环岛 PASS: 重置帧计数 ---- */
static void Enter_RingPass(ElementCtx_t *ctx)
{
    ctx->pass_frame_cnt = 0;
}

/* ---- 进入环岛 ENTRANCE: 设置 ring_side ---- */
static void Enter_RingEntry(ElementCtx_t *ctx)
{
    /* ring_side 由转移目标决定: LEFT_ENTRANCE → 0, RIGHT_ENTRANCE → 1 */
    if (ctx->current == ELEMENT_RING_LEFT_ENTRANCE ||
        ctx->current == ELEMENT_RING_LEFT_PASS) {
        ctx->ring_side = 0;
    } else {
        ctx->ring_side = 1;
    }
}

/* ---- 进入 IN_RING: 记录 yaw, 重置投票窗 ---- */
static void Enter_InRing(ElementCtx_t *ctx)
{
    ctx->ring_entry_yaw = ctx->yaw;
    VoteWin_Reset(&ctx->vote_ring);
}

/* ---- 统一退出到 STRAIGHT: 推进赛道元素表 (所有元素退出共用) ---- */
static void Enter_ExitToStraight(ElementCtx_t *ctx)
{
    (void)ctx;
    Track_Advance();
}

/* ---- 退出环岛: 启动锁存, 推进元素表, 清空投票窗 ---- */
static void Enter_RingExit(ElementCtx_t *ctx)
{
    ctx->exit_lockout_cnt = g_ring_exit_lockout_frames;
    VoteWin_Reset(&ctx->vote_ring);
    Enter_ExitToStraight(ctx);
}

/* ================================================================
 * 转移表 (按优先级排列, 同 from 内先匹配先触发)
 * ================================================================ */
static const Transition_t g_transitions[] = {
    /* 无条件即时转移 */
    { ELEMENT_RING_LEFT_ENTRANCE,  ELEMENT_RING_LEFT_PASS,  0, NULL, Enter_RingPass },
    { ELEMENT_RING_RIGHT_ENTRANCE, ELEMENT_RING_RIGHT_PASS, 0, NULL, Enter_RingPass },

    /* STRAIGHT → 各元素 (优先级: 环岛=1 > 上墙=2 > 滚筒=3 > 十字=4) */
    { ELEMENT_STRAIGHT, ELEMENT_RING_LEFT_ENTRANCE,  1, Check_RingLeftEntry,  Enter_RingEntry },
    { ELEMENT_STRAIGHT, ELEMENT_RING_RIGHT_ENTRANCE, 1, Check_RingRightEntry, Enter_RingEntry },
    { ELEMENT_STRAIGHT, ELEMENT_WALL,    2, Check_WallEntry,    NULL },
    { ELEMENT_STRAIGHT, ELEMENT_BARREL,  3, Check_BarrelEntry,  NULL },
    { ELEMENT_STRAIGHT, ELEMENT_CROSS,   4, Check_CrossEntry,   NULL },

    /* PASS → IN_RING (帧超时) */
    { ELEMENT_RING_LEFT_PASS,  ELEMENT_IN_RING, 0, Check_PassTimeout, Enter_InRing },
    { ELEMENT_RING_RIGHT_PASS, ELEMENT_IN_RING, 0, Check_PassTimeout, Enter_InRing },

    /* IN_RING → STRAIGHT (出环) */
    { ELEMENT_IN_RING, ELEMENT_STRAIGHT, 0, Check_RingExit, Enter_RingExit },

    /* 退出各元素 → STRAIGHT */
    { ELEMENT_WALL,   ELEMENT_STRAIGHT, 0, Check_WallExit,   Enter_ExitToStraight },
    { ELEMENT_BARREL, ELEMENT_STRAIGHT, 0, Check_BarrelExit, Enter_ExitToStraight },
    { ELEMENT_CROSS,  ELEMENT_STRAIGHT, 0, Check_CrossExit,  Enter_ExitToStraight },
};

#define TRANSITION_COUNT  (sizeof(g_transitions) / sizeof(g_transitions[0]))

/* ================================================================
 * Element_EvaluateTransitions — 遍历转移表, 找到第一条匹配即触发
 * ================================================================ */
static void Element_EvaluateTransitions(ElementCtx_t *ctx)
{
    unsigned char i;
    const Transition_t *t;

    for (i = 0; i < TRANSITION_COUNT; i++) {
        t = &g_transitions[i];
        if (t->from != ctx->current) continue;

        if (t->check == NULL || t->check(ctx)) {
            ctx->prev = ctx->current;
            ctx->current = t->to;

            if (t->on_enter != NULL) {
                t->on_enter(ctx);
            }

            Debug_Log(Element_GetProp(t->to)->name);
            break;  /* 第一条匹配即停止 (优先级由表顺序保证) */
        }
    }
}

/* ================================================================
 * Element_Update — 统一入口
 *   每帧调用一次 (2ms 周期)
 *   1. 安全检测层
 *   2. 重力矢量 MA + 倾角计算
 *   3. 投票生成 + 推入窗口
 *   4. 状态转移评估
 * ================================================================ */
element_t Element_Update(ElementCtx_t *ctx)
{
    unsigned char all_adc_low;
    unsigned long elapsed_ms;
    unsigned long now_tick;
    float fax, fay, faz;
    float g_mag;
    float cross_adc_sum;
    unsigned char ring_vote;
    unsigned char wall_vote;
    unsigned char barrel_vote;
    unsigned char cross_vote;

    /* ---- 0. 安全检测层 (最高优先级) ---- */
    all_adc_low = (ctx->adc[0] < 10.0f && ctx->adc[1] < 10.0f
                && ctx->adc[2] < 10.0f && ctx->adc[3] < 10.0f) ? 1 : 0;

    if (all_adc_low) {
        if (g_offtrack_active == 0) {
            g_offtrack_active = 1;
            g_offtrack_start_tick = xTaskGetTickCount();
        } else {
            now_tick = xTaskGetTickCount();
            elapsed_ms = (now_tick - g_offtrack_start_tick) * portTICK_PERIOD_MS;
            if (elapsed_ms >= (unsigned long)g_offtrack_timeout_ms) {
                ctx->current = ELEMENT_SAFETY_STOP;
                Debug_Log("SAFETY_STOP:OFF_TRACK");
                g_offtrack_active = 0;
                return ctx->current;
            }
        }
    } else {
        g_offtrack_active = 0;
    }

    /* SAFETY_STOP 退出: 任一 ADC 恢复 → 切手动模式 */
    if (ctx->current == ELEMENT_SAFETY_STOP) {
        if (ctx->adc[0] > 10.0f || ctx->adc[1] > 10.0f
            || ctx->adc[2] > 10.0f || ctx->adc[3] > 10.0f) {
            ctx->current = ELEMENT_STRAIGHT;
            manual_control_flag = 1;
            LED_RequestBreathing();
            g_offtrack_active = 0;
            Element_ResetTrackIndex(ctx);
            return ctx->current;
        }
        return ctx->current;
    }

    /* ---- 0.5 元素表 ---- */
    if (!g_track_table_enabled) {
        g_track_idx = 0;
    }

    /* ---- 1. 重力矢量 MA + 倾角计算 ---- */
    Grav_MA_Push(ctx->ax, ctx->ay, ctx->az);
    if (g_grav_win_cnt > 0) {
        fax = g_grav_sum_ax / (float)g_grav_win_cnt;
        fay = g_grav_sum_ay / (float)g_grav_win_cnt;
        faz = g_grav_sum_az / (float)g_grav_win_cnt;
    } else {
        fax = ctx->ax; fay = ctx->ay; faz = ctx->az;
    }
    g_mag = (float)sqrt(fax * fax + fay * fay + faz * faz);
    if (g_mag < 0.01f) { g_mag = 0.01f; }
    ctx->tilt_x = (float)atan2((fax > 0 ? fax : -fax), (faz > 0 ? faz : -faz)) * 57.29578f;

    /* ---- 2. 投票生成 (所有元素类型的投票在此统一计算) ---- */

    /* 环岛投票 */
    if (ctx->current == ELEMENT_STRAIGHT) {
        ring_vote = (((ctx->adc[0] > RingEntrance[0]) ||
                      (ctx->adc[2] > RingEntrance[2])) &&
                     (ctx->tilt_x < angle_level)) ? 1 : 0;
    } else if (ctx->current == ELEMENT_IN_RING) {
        if (ctx->ring_side == 0) {
            ring_vote = (ctx->adc[2] > RingExit[2]) ? 1 : 0;
        } else {
            ring_vote = (ctx->adc[0] > RingExit[0]) ? 1 : 0;
        }
    } else {
        ring_vote = 0;
    }
    VoteWin_Push(&ctx->vote_ring, ring_vote);

    /* 上墙投票 */
    if (ctx->current == ELEMENT_STRAIGHT) {
        wall_vote = (ctx->tilt_x > wall_entry_tilt) ? 1 : 0;
    } else if (ctx->current == ELEMENT_WALL) {
        wall_vote = (ctx->tilt_x < wall_exit_tilt) ? 1 : 0;
    } else {
        wall_vote = 0;
    }
    VoteWin_Push(&ctx->vote_wall, wall_vote);

    /* 滚筒投票 */
    if (ctx->current == ELEMENT_STRAIGHT) {
        barrel_vote = (ctx->tilt_x > barrel_entry_tilt) ? 1 : 0;
    } else if (ctx->current == ELEMENT_BARREL) {
        barrel_vote = (ctx->tilt_x < barrel_exit_tilt) ? 1 : 0;
    } else {
        barrel_vote = 0;
    }
    VoteWin_Push(&ctx->vote_barrel, barrel_vote);

    /* 十字投票 */
    cross_adc_sum = ctx->adc[1] + ctx->adc[3];
    if (ctx->current == ELEMENT_STRAIGHT) {
        cross_vote = (cross_adc_sum > cross_entry_sum) ? 1 : 0;
    } else if (ctx->current == ELEMENT_CROSS) {
        cross_vote = (cross_adc_sum < cross_exit_sum) ? 1 : 0;
    } else {
        cross_vote = 0;
    }
    VoteWin_Push(&ctx->vote_cross, cross_vote);

    /* ---- 4. 状态转移评估 (遍历转移表) ---- */
    Element_EvaluateTransitions(ctx);

    return ctx->current;
}
