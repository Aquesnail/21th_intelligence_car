#include "element.h"
#include "debug_arch.h"
#include "FreeRTOS.h"
#include "task.h"
#include "led_indicator.h"
extern unsigned char manual_control_flag;

/* ================================================================
 * 环岛 ADC 阈值
 *   RingEntrance[x]: ADC 高于此值 → 候选进入环岛
 *   RingExit[x]:     ADC 高于此值 → 候选退出环岛 (与进入阈值独立可调, 形成滞回)
 *   进入/退出之间形成滞回区, 防止边界抖动
 * ================================================================ */
static float RingEntrance[4];
static float RingExit[4];
static float angle_level = 15.0f;   /* 车身水平倾角阈值(度), 四元数前后倾角 */

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
   /* ELEMENT_WALL,  */
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
 *   线性子状态链: WALL → CLIMB → HORIZONTAL → DESCEND → STRAIGHT
 *   TiltB (tilt_fb): 前后倾角, 四元数版, 有符号
 *   TiltR (tilt_lr): 左右倾角, 四元数版, 有符号
 * ================================================================ */
static unsigned char xdata wall_vote_buf[WALL_WIN_MAX];
static float   wall_entry_tiltb;              /* STRAIGHT→WALL 进入 TiltB 阈值 (默认 -60) */
static float   wall_climb_exit_tiltr;         /* CLIMB→HORIZONTAL |TiltR| 阈值 (默认 70) */
static float   wall_horiz_exit_tiltr;         /* HORIZONTAL→DESCEND |TiltR| 阈值 (默认 70) */
static float   wall_descend_exit_tiltb;       /* DESCEND→STRAIGHT 退出 TiltB 阈值 (默认 60) */
static unsigned short g_ring_timeout_frames   = 3000; /* 环岛总超时 (帧, =6s, 覆盖ENTRANCE+PASS+IN_RING) */
static unsigned short g_wall_timeout_frames   = 2000; /* 上墙总超时 (帧, =4s) */
static unsigned short g_barrel_timeout_frames = 2000; /* 滚筒总超时 (帧, =4s) */
static unsigned short g_cross_timeout_frames  = 1000; /* 十字总超时 (帧, =2s) */
static VoteWindow_t vote_win_wall;

/* ---- 上墙子状态投票窗 ---- */
#define WALL_CLIMB_WIN_MAX    20
#define WALL_HORIZ_WIN_MAX    20
#define WALL_DESCEND_WIN_MAX  20
static unsigned char xdata wall_climb_vote_buf[WALL_CLIMB_WIN_MAX];
static unsigned char xdata wall_horiz_vote_buf[WALL_HORIZ_WIN_MAX];
static unsigned char xdata wall_descend_vote_buf[WALL_DESCEND_WIN_MAX];
static VoteWindow_t vote_win_wall_climb;
static VoteWindow_t vote_win_wall_horiz;
static VoteWindow_t vote_win_wall_descend;

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
 * 直角转弯检测缓冲区 + 投票窗实例
 *   子元素: STRAIGHT → RIGHT_ANGLE_LEFT/RIGHT → STRAIGHT
 *   进入: |err_filtered| > entry_thresh (传感器即将串道)
 *   退出: |err_raw| < exit_thresh (车身回到线上)
 *   左右共用同一投票窗 (方向由赛道表 Track_CheckExpected 区分)
 * ================================================================ */
#define RA_WIN_MAX  20
static unsigned char xdata ra_vote_buf[RA_WIN_MAX];
static VoteWindow_t vote_win_ra;
static float   g_ra_entry_thresh    = 100.0f;   /* |err_filtered| > 此值 → 进入直角 */
static float   g_ra_exit_thresh     = 30.0f;    /* |err_raw| < 此值 → 退出直角 (滞回) */
static unsigned short g_ra_timeout_frames = 500;  /* 直角超时帧数 (默认500帧=1s) */

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
static unsigned char g_debug_push_mode = 0;   /* 推车调试: 1=关电机+关风扇, 元素检测/LED照跑 */

static float g_speed_cruising     = 50.0f;
static float g_speed_ring         = 50.0f;
static float g_speed_wall         = 45.0f;
static float g_speed_wall_climb   = 45.0f;
static float g_speed_wall_horiz   = 40.0f;
static float g_speed_wall_descend = 50.0f;
static float g_speed_barrel       = 45.0f;
static float g_speed_cross        = 45.0f;

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
    /* [3] ELEMENT_WALL */
    { ELEMENT_WALL, "WALL", &g_speed_wall, 255, 128, 0, CTRL_NORMAL, NULL, 0 },
    /* [4] ELEMENT_CROSS */
    { ELEMENT_CROSS, "CROSS", &g_speed_cross, 0, 255, 0, CTRL_NORMAL, NULL, 0 },
    /* [5] ELEMENT_BARREL */
    { ELEMENT_BARREL, "BARREL", &g_speed_barrel, 255, 0, 255, CTRL_NORMAL, NULL, 0 },
    /* [6] ELEMENT_SKIP */
    { ELEMENT_SKIP, "SKIP", NULL, 0, 0, 0, CTRL_NORMAL, NULL, 0 },
    /* [7] ELEMENT_RING_LEFT_PASS */
    { ELEMENT_RING_LEFT_PASS, "RING_L_PASS", &g_speed_ring, 0, 255, 255, CTRL_OL_YAW, &g_ol_ring_yawrate,  1 },
    /* [8] ELEMENT_RING_RIGHT_PASS */
    { ELEMENT_RING_RIGHT_PASS,"RING_R_PASS",&g_speed_ring, 0, 255, 255, CTRL_OL_YAW, &g_ol_ring_yawrate, -1 },
    /* [9] ELEMENT_IN_RING */
    { ELEMENT_IN_RING, "IN_RING", &g_speed_ring, 255, 255, 0, CTRL_NORMAL, NULL, 0 },
    /* [10] ELEMENT_WALL_CLIMB */
    { ELEMENT_WALL_CLIMB, "WALL_CLIMB", &g_speed_wall_climb, 255, 128, 0, CTRL_NORMAL, NULL, 0 },
    /* [11] ELEMENT_WALL_HORIZONTAL */
    { ELEMENT_WALL_HORIZONTAL, "WALL_HORIZ", &g_speed_wall_horiz, 255, 255, 0, CTRL_NORMAL, NULL, 0 },
    /* [12] ELEMENT_WALL_DESCEND */
    { ELEMENT_WALL_DESCEND, "WALL_DESC", &g_speed_wall_descend, 0, 255, 255, CTRL_NORMAL, NULL, 0 },
    /* [13] ELEMENT_RIGHT_ANGLE_LEFT */
    { ELEMENT_RIGHT_ANGLE_LEFT,  "RT_ANG_L", &g_speed_cruising, 128, 0, 255, CTRL_ERR_FREEZE, NULL, 0 },
    /* [14] ELEMENT_RIGHT_ANGLE_RIGHT */
    { ELEMENT_RIGHT_ANGLE_RIGHT, "RT_ANG_R", &g_speed_cruising, 255, 128, 0, CTRL_ERR_FREEZE, NULL, 0 },
    /* [15] ELEMENT_SAFETY_STOP */
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

/* ---- 获取有效控制策略 (push mode 下全部覆盖为 FREEWHEEL, BRAKE 除外) ---- */
ControlStrategy_t Element_GetEffectiveCtrl(element_t e)
{
    const ElementProp_t *prop;
    ControlStrategy_t ctrl;

    prop = Element_GetProp(e);
    if (prop == NULL) return CTRL_NORMAL;
    ctrl = prop->ctrl;

    if (g_debug_push_mode && ctrl != CTRL_BRAKE) {
        return CTRL_FREEWHEEL;
    }
    return ctrl;
}

void Element_Init(ElementCtx_t *ctx)
{
    unsigned char i;

    /* Phase 3: 初始化上下文 (NULL = 兼容旧接口, 仅初始化静态变量) */
    if (ctx != NULL) {
        ctx->ring_side        = 0;
        ctx->ring_frame_cnt   = 0;
        ctx->elem_frame_cnt   = 0;
        ctx->ring_entry_yaw   = 0.0f;
        ctx->exit_lockout_cnt = 0;
        ctx->track_idx        = 0;
        ctx->track_table_enabled = 1;
        ctx->offtrack_active     = 0;
        ctx->offtrack_start_tick = 0;
        ctx->current = ELEMENT_STRAIGHT;
        ctx->prev    = ELEMENT_STRAIGHT;
        ctx->tilt_fb   = 0.0f;
        ctx->tilt_lr   = 0.0f;
        ctx->frozen_err       = 0.0f;
        ctx->right_angle_side = 0;
        ctx->err_raw          = 0.0f;
        ctx->err_filtered     = 0.0f;

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
    Debug_Register("Ring_Timeout", DBG_UINT16 | STATIC_TYPE, &g_ring_timeout_frames);
    Debug_Register("TblEnable",  DBG_UINT8 | STATIC_TYPE, &g_track_table_enabled);
    Debug_Register("DbgElem",    DBG_UINT8 | STATIC_TYPE, &g_single_element_debug);
    Debug_Register("PushMode",   DBG_UINT8 | STATIC_TYPE, &g_debug_push_mode);

    Debug_Register("Wall_WinLen",  DBG_UINT8 | STATIC_TYPE, &vote_win_wall.len);
    /* 上墙入口阈值: STRAIGHT→WALL TiltB */
    Debug_Register("Wall_EntTiltB", DBG_FLOAT | STATIC_TYPE, &wall_entry_tiltb);
    /* 上爬→水平: |TiltR| 阈值 */
    Debug_Register("Wall_ClimbTR", DBG_FLOAT | STATIC_TYPE, &wall_climb_exit_tiltr);
    /* 水平→下行: |TiltR| 阈值 */
    Debug_Register("Wall_HorizTR", DBG_FLOAT | STATIC_TYPE, &wall_horiz_exit_tiltr);
    /* 下行→STRAIGHT: TiltB 阈值 */
    Debug_Register("Wall_DescTiltB",DBG_FLOAT | STATIC_TYPE, &wall_descend_exit_tiltb);
    /* 上墙总超时 (帧数) */
    Debug_Register("Wall_Timeout", DBG_UINT16 | STATIC_TYPE, &g_wall_timeout_frames);

    /* 上墙投票窗参数 (上位机可调) */
   // Debug_Register("Wall_EntThr",  DBG_UINT8 | STATIC_TYPE, &vote_win_wall.entry_thresh);
  //  Debug_Register("Wall_ExtThr",  DBG_UINT8 | STATIC_TYPE, &vote_win_wall.exit_thresh);

  //  Debug_Register("Bar_WinLen",   DBG_UINT8 | STATIC_TYPE, &vote_win_barrel.len);
    /* 滚筒倾角阈值 */
    Debug_Register("Bar_EnTilt",  DBG_FLOAT | STATIC_TYPE, &barrel_entry_tilt);
    Debug_Register("Bar_ExTilt",  DBG_FLOAT | STATIC_TYPE, &barrel_exit_tilt);
    /* 滚筒超时 (帧数) */
    Debug_Register("Bar_Timeout", DBG_UINT16 | STATIC_TYPE, &g_barrel_timeout_frames);

    /* 滚筒投票窗参数 (上位机可调) */
   // Debug_Register("Bar_EntThr",  DBG_UINT8 | STATIC_TYPE, &vote_win_barrel.entry_thresh);
  //  Debug_Register("Bar_ExtThr",  DBG_UINT8 | STATIC_TYPE, &vote_win_barrel.exit_thresh);

  //  Debug_Register("Cross_WinLen", DBG_UINT8 | STATIC_TYPE, &vote_win_cross.len);
    /* 十字竖直电感和阈值 */
    Debug_Register("Cross_EnSum", DBG_FLOAT | STATIC_TYPE, &cross_entry_sum);
    Debug_Register("Cross_ExSum", DBG_FLOAT | STATIC_TYPE, &cross_exit_sum);
    /* 十字超时 (帧数) */
    Debug_Register("Cross_Timeout", DBG_UINT16 | STATIC_TYPE, &g_cross_timeout_frames);

    /* 十字投票窗参数 (上位机可调) */
    Debug_Register("Cross_EntThr", DBG_UINT8 | STATIC_TYPE, &vote_win_cross.entry_thresh);
    Debug_Register("Cross_ExtThr", DBG_UINT8 | STATIC_TYPE, &vote_win_cross.exit_thresh);

    /* 直角转弯参数 (上位机在线调参) */
    Debug_Register("RA_EnThr",   DBG_FLOAT  | STATIC_TYPE, &g_ra_entry_thresh);
    Debug_Register("RA_ExThr",   DBG_FLOAT  | STATIC_TYPE, &g_ra_exit_thresh);
    Debug_Register("RA_Timeout", DBG_UINT16 | STATIC_TYPE, &g_ra_timeout_frames);
    Debug_Register("RA_WinLen",  DBG_UINT8  | STATIC_TYPE, &vote_win_ra.len);
    Debug_Register("RA_WinEnTh", DBG_UINT8  | STATIC_TYPE, &vote_win_ra.entry_thresh);
    Debug_Register("RA_WinExTh", DBG_UINT8  | STATIC_TYPE, &vote_win_ra.exit_thresh);

    /* 安全停车参数 (上位机在线调参) */
    Debug_Register("OffTkMs",   DBG_FLOAT | STATIC_TYPE, &g_offtrack_timeout_ms);

    /* 速度变量 (上位机在线调参) */
    Debug_Register("Cruising", DBG_FLOAT | STATIC_TYPE, &g_speed_cruising);
    Debug_Register("RingSpd",  DBG_FLOAT | STATIC_TYPE, &g_speed_ring);
    Debug_Register("WallSpd",  DBG_FLOAT | STATIC_TYPE, &g_speed_wall);
    Debug_Register("WClimbSpd",DBG_FLOAT | STATIC_TYPE, &g_speed_wall_climb);
    Debug_Register("WHorizSpd",DBG_FLOAT | STATIC_TYPE, &g_speed_wall_horiz);
    Debug_Register("WDescSpd", DBG_FLOAT | STATIC_TYPE, &g_speed_wall_descend);
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
    VoteWin_Init(&vote_win_ring,         ring_vote_buf,          100);
    VoteWin_Init(&vote_win_wall,         wall_vote_buf,          WALL_WIN_MAX);
    VoteWin_Init(&vote_win_wall_climb,   wall_climb_vote_buf,    WALL_CLIMB_WIN_MAX);
    VoteWin_Init(&vote_win_wall_horiz,   wall_horiz_vote_buf,    WALL_HORIZ_WIN_MAX);
    VoteWin_Init(&vote_win_wall_descend, wall_descend_vote_buf,  WALL_DESCEND_WIN_MAX);
    VoteWin_Init(&vote_win_barrel,       barrel_vote_buf,        BARREL_WIN_MAX);
    VoteWin_Init(&vote_win_cross,        cross_vote_buf,         CROSS_WIN_MAX);
    VoteWin_Init(&vote_win_ra,           ra_vote_buf,             RA_WIN_MAX);

    /* 覆盖各元素特定默认值 */
    /* 环岛: 窗长 10, 进入阈值 3 */
    vote_win_ring.len          = 10;
    vote_win_ring.entry_thresh = 3;

    /* 上墙入口: TiltB < -60 进入 (四元数前后倾角) */
    wall_entry_tiltb  = -60.0f;
    vote_win_wall.entry_thresh = 7;
    vote_win_wall.exit_thresh  = 7;
    vote_win_wall.len          = 10;

    /* 上爬→水平: |TiltR| > 70 */
    wall_climb_exit_tiltr  = 70.0f;
    vote_win_wall_climb.len          = 10;
    vote_win_wall_climb.entry_thresh = 7;
    vote_win_wall_climb.exit_thresh  = 7;

    /* 水平→下行: |TiltR| < 70 */
    wall_horiz_exit_tiltr  = 70.0f;
    vote_win_wall_horiz.len          = 10;
    vote_win_wall_horiz.entry_thresh = 7;
    vote_win_wall_horiz.exit_thresh  = 7;

    /* 下行→STRAIGHT: TiltB > 60 */
    wall_descend_exit_tiltb = 60.0f;
    vote_win_wall_descend.len          = 10;
    vote_win_wall_descend.entry_thresh = 7;
    vote_win_wall_descend.exit_thresh  = 7;

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

    /* 直角转弯默认: |err_filtered|>100 进入, |err_raw|<30 退出, 窗长5帧 */
    vote_win_ra.len          = 5;
    vote_win_ra.entry_thresh = 3;
    vote_win_ra.exit_thresh  = 3;

    /* 绑定投票窗到上下文 (必须在 VoteWin_Init + 参数设置之后, 确保 buf/len/thresh 有效) */
    if (ctx != NULL) {
        ctx->vote_ring         = vote_win_ring;
        ctx->vote_wall         = vote_win_wall;
        ctx->vote_wall_climb   = vote_win_wall_climb;
        ctx->vote_wall_horiz   = vote_win_wall_horiz;
        ctx->vote_wall_descend = vote_win_wall_descend;
        ctx->vote_barrel       = vote_win_barrel;
        ctx->vote_cross        = vote_win_cross;
        ctx->vote_right_angle  = vote_win_ra;
    }

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
                && ((ctx->tilt_fb > 0.0f ? ctx->tilt_fb : -ctx->tilt_fb) < angle_level)) ? 1 : 0;
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

/* ---- 上墙入口: 赛道表把关 + TiltB 检测 (四元数前后倾角) ---- */
static unsigned char Check_WallEntry(ElementCtx_t *ctx)
{
    if (!Track_CheckExpected(ctx, ELEMENT_WALL)) return 0;

    if (ctx->vote_wall.len == 0) { //如果窗长为零就忽略后面的VoteWin_Reached，等于取消了时间滤波
        return (ctx->tilt_fb < wall_entry_tiltb) ? 1 : 0;
    }
    return VoteWin_Reached(&ctx->vote_wall, ctx->vote_wall.entry_thresh);
}

/* ---- 上爬→水平: |TiltR| > 阈值 (四元数侧倾角) ---- */
static unsigned char Check_WallClimbToHoriz(ElementCtx_t *ctx)
{
    float tilt_lr_abs;
    tilt_lr_abs = (ctx->tilt_lr > 0.0f) ? ctx->tilt_lr : -ctx->tilt_lr;

    if (ctx->vote_wall_climb.len == 0) {
        return (tilt_lr_abs > wall_climb_exit_tiltr) ? 1 : 0;
    }
    return VoteWin_Reached(&ctx->vote_wall_climb, ctx->vote_wall_climb.entry_thresh);
}

/* ---- 水平→下行: |TiltR| < 阈值 (四元数侧倾角) ---- */
static unsigned char Check_WallHorizToDescend(ElementCtx_t *ctx)
{
    float tilt_lr_abs;
    tilt_lr_abs = (ctx->tilt_lr > 0.0f) ? ctx->tilt_lr : -ctx->tilt_lr;

    if (ctx->vote_wall_horiz.len == 0) {
        return (tilt_lr_abs < wall_horiz_exit_tiltr) ? 1 : 0;
    }
    return VoteWin_Reached(&ctx->vote_wall_horiz, ctx->vote_wall_horiz.entry_thresh);
}

/* ---- 下行→STRAIGHT: TiltB > 阈值 (四元数前后倾角) ---- */
static unsigned char Check_WallDescendExit(ElementCtx_t *ctx)
{
    if (ctx->vote_wall_descend.len == 0) { 
        return (ctx->tilt_fb > wall_descend_exit_tiltb) ? 1 : 0;
    }
    return VoteWin_Reached(&ctx->vote_wall_descend, ctx->vote_wall_descend.entry_thresh);
}

/* ---- 滚筒进入: 赛道表把关 + 倾角检测 ---- */
static unsigned char Check_BarrelEntry(ElementCtx_t *ctx)
{
    if (!Track_CheckExpected(ctx, ELEMENT_BARREL)) return 0;

    if (ctx->vote_barrel.len == 0) {
        return ((ctx->tilt_fb > 0.0f ? ctx->tilt_fb : -ctx->tilt_fb) > barrel_entry_tilt) ? 1 : 0;
    }
    return VoteWin_Reached(&ctx->vote_barrel, ctx->vote_barrel.entry_thresh);
}

/* ---- 滚筒退出 ---- */
static unsigned char Check_BarrelExit(ElementCtx_t *ctx)
{
    if (ctx->vote_barrel.len == 0) {
        return ((ctx->tilt_fb > 0.0f ? ctx->tilt_fb : -ctx->tilt_fb) < barrel_exit_tilt) ? 1 : 0;
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
    return (ctx->elem_frame_cnt >= g_pass_timeout_frames) ? 1 : 0;
}

/* ---- 环岛退出 (yaw 门禁 + 任一外侧电感超过退出阈值) ---- */
static unsigned char Check_RingExit(ElementCtx_t *ctx)
{
    float yaw_delta;

    /* yaw 门禁: 累计转向不足则不允许出环 */
    yaw_delta = ctx->yaw - ctx->ring_entry_yaw;
    if (ctx->ring_side == 0) {
        /* 左环: 左转 yaw 增加 */
        if (yaw_delta < g_ring_exit_yaw_thresh) return 0;
    } else {
        /* 右环: 右转 yaw 减小 */
        if (yaw_delta > -g_ring_exit_yaw_thresh) return 0; //票仍然在票仓里，只是这里返回0不通过
    }

    /* 投票窗判断 (窗口内 OR 两侧电感) */
    return VoteWin_Reached(&ctx->vote_ring, ctx->vote_ring.exit_thresh);//条件都满足了才会开始检查票数是否足够
}

/* ================================================================
 * 进入动作函数
 * ================================================================ */

/* ---- 进入环岛 PASS: 重置帧计数 ---- */
static void Enter_RingPass(ElementCtx_t *ctx)
{
    ctx->elem_frame_cnt = 0;
    ctx->ring_frame_cnt = 0;
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

/* ---- 进入 IN_RING: 记录 yaw, 重置投票窗 (开始攒出口票) ---- */
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

/* ---- 退出滚筒: 清空投票窗, 推进元素表 ---- */
static void Enter_BarrelExit(ElementCtx_t *ctx)
{
    VoteWin_Reset(&ctx->vote_barrel);
    Enter_ExitToStraight(ctx);
}

/* ---- 退出十字: 清空投票窗, 推进元素表 ---- */
static void Enter_CrossExit(ElementCtx_t *ctx)
{
    VoteWin_Reset(&ctx->vote_cross);
    Enter_ExitToStraight(ctx);
}

/* ---- 进入上墙: 复位帧计数, 清空全部上墙投票窗 ---- */
static void Enter_Wall(ElementCtx_t *ctx)
{
    ctx->elem_frame_cnt  = 0;
    VoteWin_Reset(&ctx->vote_wall);
    VoteWin_Reset(&ctx->vote_wall_climb);
    VoteWin_Reset(&ctx->vote_wall_horiz);
    VoteWin_Reset(&ctx->vote_wall_descend);
}

/* ---- 进入滚筒: 复位帧计数, 清空投票窗 ---- */
static void Enter_Barrel(ElementCtx_t *ctx)
{
    ctx->elem_frame_cnt = 0;
    VoteWin_Reset(&ctx->vote_barrel);
}

/* ---- 进入十字: 复位帧计数, 清空投票窗 ---- */
static void Enter_Cross(ElementCtx_t *ctx)
{
    ctx->elem_frame_cnt = 0;
    VoteWin_Reset(&ctx->vote_cross);
}

/* ---- 上墙超时: 检查帧计数 ---- */
static unsigned char Check_WallTimeout(ElementCtx_t *ctx)
{
    return (ctx->elem_frame_cnt >= g_wall_timeout_frames) ? 1 : 0;
}

/* ---- 上墙超时: 清空投票窗, 推进元素表 ---- */
static void Enter_WallTimeout(ElementCtx_t *ctx)
{
    VoteWin_Reset(&ctx->vote_wall);
    VoteWin_Reset(&ctx->vote_wall_climb);
    VoteWin_Reset(&ctx->vote_wall_horiz);
    VoteWin_Reset(&ctx->vote_wall_descend);
    Enter_ExitToStraight(ctx);
}

/* ---- 滚筒超时: 检查帧计数 ---- */
static unsigned char Check_BarrelTimeout(ElementCtx_t *ctx)
{
    return (ctx->elem_frame_cnt >= g_barrel_timeout_frames) ? 1 : 0;
}

/* ---- 滚筒超时: 清空投票窗, 推进元素表 ---- */
static void Enter_BarrelTimeout(ElementCtx_t *ctx)
{
    VoteWin_Reset(&ctx->vote_barrel);
    Enter_ExitToStraight(ctx);
}

/* ---- 十字超时: 检查帧计数 ---- */
static unsigned char Check_CrossTimeout(ElementCtx_t *ctx)
{
    return (ctx->elem_frame_cnt >= g_cross_timeout_frames) ? 1 : 0;
}

/* ---- 十字超时: 清空投票窗, 推进元素表 ---- */
static void Enter_CrossTimeout(ElementCtx_t *ctx)
{
    VoteWin_Reset(&ctx->vote_cross);
    Enter_ExitToStraight(ctx);
}

static unsigned char Check_RingTimeout(ElementCtx_t *ctx)
{
    return (ctx->ring_frame_cnt >= g_ring_timeout_frames) ? 1 : 0;
}

static void Enter_RingTimeout(ElementCtx_t *ctx)
{
    ctx->ring_frame_cnt = 0;
    VoteWin_Reset(&ctx->vote_ring);
    Enter_ExitToStraight(ctx);
}

/* ---- 进入上爬: 清空上爬投票窗 ---- */
static void Enter_WallClimb(ElementCtx_t *ctx)
{
    VoteWin_Reset(&ctx->vote_wall_climb);
}

/* ---- 进入水平: 清空水平投票窗 ---- */
static void Enter_WallHoriz(ElementCtx_t *ctx)
{
    VoteWin_Reset(&ctx->vote_wall_horiz);
}

/* ---- 进入下行: 清空下行投票窗 ---- */
static void Enter_WallDescend(ElementCtx_t *ctx)
{
    VoteWin_Reset(&ctx->vote_wall_descend);
}

/* ---- 直角转弯进入: 记录冻结 err, 清空投票窗 ---- */
static void Enter_RightAngle(ElementCtx_t *ctx)
{
    ctx->frozen_err       = ctx->err_raw;
    ctx->right_angle_side = (ctx->current == ELEMENT_RIGHT_ANGLE_LEFT) ? 0 : 1;
    ctx->elem_frame_cnt   = 0;
    VoteWin_Reset(&ctx->vote_right_angle);
}

/* ---- 直角转弯退出: 清空投票窗 (管线 seed 由 OnCtrlTransition 处理) ---- */
static void Enter_RightAngleExit(ElementCtx_t *ctx)
{
    VoteWin_Reset(&ctx->vote_right_angle);
}

/* ---- 直角转弯进入条件 (左): 无赛道表约束 + 投票达标 + err 为负 ---- */
static unsigned char Check_RightAngleLeftEntry(ElementCtx_t *ctx)
{
    if (!VoteWin_Reached(&ctx->vote_right_angle, ctx->vote_right_angle.entry_thresh))
        return 0;
    return (ctx->err_raw < -g_ra_entry_thresh) ? 1 : 0;
}

/* ---- 直角转弯进入条件 (右): 无赛道表约束 + 投票达标 + err 为正 ---- */
static unsigned char Check_RightAngleRightEntry(ElementCtx_t *ctx)
{
    if (!VoteWin_Reached(&ctx->vote_right_angle, ctx->vote_right_angle.entry_thresh))
        return 0;
    return (ctx->err_raw > g_ra_entry_thresh) ? 1 : 0;
}

/* ---- 直角转弯退出条件: |raw_err| < exit_thresh ---- */
static unsigned char Check_RightAngleExit(ElementCtx_t *ctx)
{
    float err_raw_abs;
    err_raw_abs = (ctx->err_raw > 0.0f) ? ctx->err_raw : -ctx->err_raw;

    if (ctx->vote_right_angle.len == 0) {
        return (err_raw_abs < g_ra_exit_thresh) ? 1 : 0;
    }
    return VoteWin_Reached(&ctx->vote_right_angle, ctx->vote_right_angle.exit_thresh);
}

/* ---- 直角转弯超时 ---- */
static unsigned char Check_RATimeout(ElementCtx_t *ctx)
{
    return (ctx->elem_frame_cnt >= g_ra_timeout_frames) ? 1 : 0;
}

/* ================================================================
 * 转移表 (按优先级排列, 同 from 内先匹配先触发)
 * ================================================================ */
static const Transition_t g_transitions[] = {
    /* 无条件即时转移 */
    { ELEMENT_RING_LEFT_ENTRANCE,  ELEMENT_RING_LEFT_PASS,  0, NULL, Enter_RingPass },
    { ELEMENT_RING_RIGHT_ENTRANCE, ELEMENT_RING_RIGHT_PASS, 0, NULL, Enter_RingPass },

    /* STRAIGHT → 各元素 (优先级: 环岛=1 > 上墙=2 > 滚筒=3 > 十字=4 > 直角=5) */
    { ELEMENT_STRAIGHT, ELEMENT_RING_LEFT_ENTRANCE,  1, Check_RingLeftEntry,  Enter_RingEntry },
    { ELEMENT_STRAIGHT, ELEMENT_RING_RIGHT_ENTRANCE, 1, Check_RingRightEntry, Enter_RingEntry },
    { ELEMENT_STRAIGHT, ELEMENT_WALL,    2, Check_WallEntry,    Enter_Wall },
    { ELEMENT_STRAIGHT, ELEMENT_BARREL,  3, Check_BarrelEntry,  Enter_Barrel },
    { ELEMENT_STRAIGHT, ELEMENT_CROSS,   4, Check_CrossEntry,   Enter_Cross },
    { ELEMENT_STRAIGHT, ELEMENT_RIGHT_ANGLE_LEFT,  5, Check_RightAngleLeftEntry,  Enter_RightAngle },
    { ELEMENT_STRAIGHT, ELEMENT_RIGHT_ANGLE_RIGHT, 5, Check_RightAngleRightEntry, Enter_RightAngle },

    /* PASS → IN_RING (帧超时) */
    { ELEMENT_RING_LEFT_PASS,  ELEMENT_IN_RING, 0, Check_PassTimeout, Enter_InRing },
    { ELEMENT_RING_RIGHT_PASS, ELEMENT_IN_RING, 0, Check_PassTimeout, Enter_InRing },
    /* IN_RING → STRAIGHT (出环) */
    { ELEMENT_IN_RING, ELEMENT_STRAIGHT, 0, Check_RingExit, Enter_RingExit },
    /* IN_RING 超时兜底 (prio=1: 正常出环优先) */
    { ELEMENT_IN_RING, ELEMENT_STRAIGHT, 1, Check_RingTimeout, Enter_RingTimeout },

    /* 上墙线性子状态链: WALL → CLIMB → HORIZONTAL → DESCEND → STRAIGHT */
    { ELEMENT_WALL,       ELEMENT_WALL_CLIMB,      0, NULL,                     Enter_WallClimb },
    { ELEMENT_WALL_CLIMB, ELEMENT_WALL_HORIZONTAL, 0, Check_WallClimbToHoriz,   Enter_WallHoriz },
    { ELEMENT_WALL_HORIZONTAL, ELEMENT_WALL_DESCEND, 0, Check_WallHorizToDescend, Enter_WallDescend },
    { ELEMENT_WALL_DESCEND,    ELEMENT_STRAIGHT,      0, Check_WallDescendExit,    Enter_ExitToStraight },

    /* 上墙超时兜底 (prio=1: 正常子状态转移优先, 超时才触发) */
    { ELEMENT_WALL_CLIMB,    ELEMENT_STRAIGHT, 1, Check_WallTimeout,    Enter_WallTimeout },
    { ELEMENT_WALL_HORIZONTAL, ELEMENT_STRAIGHT, 1, Check_WallTimeout,    Enter_WallTimeout },
    { ELEMENT_WALL_DESCEND,  ELEMENT_STRAIGHT, 1, Check_WallTimeout,    Enter_WallTimeout },

    /* 退出各元素 → STRAIGHT */
    { ELEMENT_BARREL, ELEMENT_STRAIGHT, 0, Check_BarrelExit, Enter_BarrelExit },
    { ELEMENT_BARREL, ELEMENT_STRAIGHT, 1, Check_BarrelTimeout, Enter_BarrelTimeout },
    { ELEMENT_CROSS,  ELEMENT_STRAIGHT, 0, Check_CrossExit,  Enter_CrossExit },
    { ELEMENT_CROSS,  ELEMENT_STRAIGHT, 1, Check_CrossTimeout,  Enter_CrossTimeout },

    /* 直角转弯退出 */
    { ELEMENT_RIGHT_ANGLE_LEFT,  ELEMENT_STRAIGHT, 0, Check_RightAngleExit, Enter_RightAngleExit },
    { ELEMENT_RIGHT_ANGLE_LEFT,  ELEMENT_STRAIGHT, 1, Check_RATimeout,      Enter_RightAngleExit },
    { ELEMENT_RIGHT_ANGLE_RIGHT, ELEMENT_STRAIGHT, 0, Check_RightAngleExit, Enter_RightAngleExit },
    { ELEMENT_RIGHT_ANGLE_RIGHT, ELEMENT_STRAIGHT, 1, Check_RATimeout,      Enter_RightAngleExit },
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

        if (t->check == NULL || t->check(ctx)) {//如果t0>check(ctx)发现投票满足条件
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
//投票在这里push进去，由Check决定是否放行
element_t Element_Update(ElementCtx_t *ctx)
{
    unsigned char all_adc_low;
    unsigned long elapsed_ms;
    unsigned long now_tick;
    float cross_adc_sum;
    float tilt_fb_abs;
    unsigned char ring_vote;
    unsigned char wall_vote;
    unsigned char wall_climb_vote;
    unsigned char wall_horiz_vote;
    unsigned char wall_descend_vote;
    unsigned char barrel_vote;
    unsigned char cross_vote;
    unsigned char ra_vote;
    float err_abs;
    float tilt_lr_abs;

    /* ---- 0. 安全检测层 (最高优先级) ---- */
    all_adc_low = (ctx->adc[0] < 10.0f && ctx->adc[1] < 10.0f
                && ctx->adc[2] < 10.0f && ctx->adc[3] < 10.0f) ? 1 : 0;

    if (all_adc_low && ctx->current != ELEMENT_SAFETY_STOP) {
        if (g_offtrack_active == 0) {
            g_offtrack_active = 1;
            g_offtrack_start_tick = xTaskGetTickCount();
        } else {
            now_tick = xTaskGetTickCount();
            elapsed_ms = (now_tick - g_offtrack_start_tick) * portTICK_PERIOD_MS;
            if (elapsed_ms >= (unsigned long)g_offtrack_timeout_ms) {
                ctx->current = ELEMENT_SAFETY_STOP;
                Debug_Log(Element_GetProp(ELEMENT_SAFETY_STOP)->name);
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
            Debug_Log(Element_GetProp(ELEMENT_STRAIGHT)->name);
            return ctx->current;
        }
        return ctx->current;
    }

    /* ---- 0.5 元素表 ---- */
    if (!g_track_table_enabled) {
        g_track_idx = 0;
    }

    /* ---- 0.6 通用帧计数 (各元素超时检测共用) ---- */
    ctx->elem_frame_cnt++;

    /* 环岛链帧计数 (ENTRANCE/PASS/IN_RING 超时兜底) */
    if (ctx->current == ELEMENT_RING_LEFT_ENTRANCE
        || ctx->current == ELEMENT_RING_RIGHT_ENTRANCE
        || ctx->current == ELEMENT_RING_LEFT_PASS
        || ctx->current == ELEMENT_RING_RIGHT_PASS
        || ctx->current == ELEMENT_IN_RING) {
        ctx->ring_frame_cnt++;
    }

    /* ---- 1. 倾角绝对值 (四元数前后倾角, 供投票使用) ---- */
    tilt_fb_abs = (ctx->tilt_fb > 0.0f ? ctx->tilt_fb : -ctx->tilt_fb);

    /* ---- 2. 投票生成 (所有元素类型的投票在此统一计算) ---- */

    /* 环岛投票 (入口 + 出口共用 vote_ring 窗口) */
    if (ctx->current == ELEMENT_STRAIGHT) {
        /* 入口票: 两侧电感 OR + 车身水平 */
        ring_vote = (((ctx->adc[0] > RingEntrance[0]) ||
                      (ctx->adc[2] > RingEntrance[2])) &&
                     (tilt_fb_abs < angle_level)) ? 1 : 0;
    } else if (ctx->current == ELEMENT_IN_RING) {
        /* 出口票: 两侧电感 OR (不区分左右环) */
        ring_vote = (ctx->adc[2] > RingExit[2] || ctx->adc[0] > RingExit[0]) ? 1 : 0;
    } else {
        ring_vote = 0;
    }
    VoteWin_Push(&ctx->vote_ring, ring_vote);

    /* 上墙入口投票: STRAIGHT → WALL, TiltB < 阈值 */
    if (ctx->current == ELEMENT_STRAIGHT) {
        wall_vote = (ctx->tilt_fb < wall_entry_tiltb) ? 1 : 0;
    } else {
        wall_vote = 0;
    }
    VoteWin_Push(&ctx->vote_wall, wall_vote);

    /* 上爬→水平投票: WALL_CLIMB → HORIZONTAL, |TiltR| > 阈值 */
    tilt_lr_abs = (ctx->tilt_lr > 0.0f) ? ctx->tilt_lr : -ctx->tilt_lr;
    if (ctx->current == ELEMENT_WALL_CLIMB) {
        wall_climb_vote = (tilt_lr_abs > wall_climb_exit_tiltr) ? 1 : 0;
    } else {
        wall_climb_vote = 0;
    }
    VoteWin_Push(&ctx->vote_wall_climb, wall_climb_vote);

    /* 水平→下行投票: WALL_HORIZONTAL → DESCEND, |TiltR| < 阈值 */
    if (ctx->current == ELEMENT_WALL_HORIZONTAL) {
        wall_horiz_vote = (tilt_lr_abs < wall_horiz_exit_tiltr) ? 1 : 0;
    } else {
        wall_horiz_vote = 0;
    }
    VoteWin_Push(&ctx->vote_wall_horiz, wall_horiz_vote);

    /* 下行→STRAIGHT投票: WALL_DESCEND → STRAIGHT, TiltB > 阈值 */
    if (ctx->current == ELEMENT_WALL_DESCEND) {
        wall_descend_vote = (ctx->tilt_fb > wall_descend_exit_tiltb) ? 1 : 0;
    } else {
        wall_descend_vote = 0;
    }
    VoteWin_Push(&ctx->vote_wall_descend, wall_descend_vote);

    /* 滚筒投票 */
    if (ctx->current == ELEMENT_STRAIGHT) {
        barrel_vote = (tilt_fb_abs > barrel_entry_tilt) ? 1 : 0;
    } else if (ctx->current == ELEMENT_BARREL) {
        barrel_vote = (tilt_fb_abs < barrel_exit_tilt) ? 1 : 0;
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

    /* 直角转弯投票: STRAIGHT 下 |err_filtered| > entry_thresh 则投票进入 */
    if (ctx->current == ELEMENT_STRAIGHT) {
        err_abs = (ctx->err_raw > 0.0f) ? ctx->err_raw : -ctx->err_raw;
        ra_vote = (err_abs > g_ra_entry_thresh) ? 1 : 0;
    } else if (ctx->current == ELEMENT_RIGHT_ANGLE_LEFT
            || ctx->current == ELEMENT_RIGHT_ANGLE_RIGHT) {
        err_abs = (ctx->err_raw > 0.0f) ? ctx->err_raw : -ctx->err_raw;
        ra_vote = (err_abs < g_ra_exit_thresh) ? 1 : 0;
    } else {
        ra_vote = 0;
    }
    VoteWin_Push(&ctx->vote_right_angle, ra_vote);

    /* ---- 4. 状态转移评估 (遍历转移表) ---- */
    Element_EvaluateTransitions(ctx);

    return ctx->current;
}



/* ================================================================
 * SelectTargetYaw — 统一目标角速度选择器
 *   替代原来在 PID 输出之后手动覆盖 target_yaw 的"劫持"模式
 *   CTRL_NORMAL:  使用外环 PID 输出
 *   CTRL_OL_YAW:  使用属性表中的开环角速度 (符号 × 幅值)
 *   CTRL_BRAKE / CTRL_FREEWHEEL: 返回 0
 * ================================================================ */
float SelectTargetYaw(ControlStrategy_t ctrl, const ElementProp_t *prop,ElementCtx_t *ctx)
{
    if (ctrl == CTRL_OL_YAW && prop != NULL && prop->ol_yawrate_ptr != NULL) {
        return (float)(prop->ol_yaw_sign) * (*prop->ol_yawrate_ptr) + ctx->yawrate;
    }
    if (ctrl == CTRL_BRAKE || ctrl == CTRL_FREEWHEEL) {
        return 0.0f;
    }
    return ctx->yawrate;
}


/* ================================================================
 * SelectTargetErr — 统一目标误差值选择器
 *   替代原来在 差比和差 输出之后手动覆盖 Err 的"劫持"模式
 *   CTRL_NORMAL:  使用原本Err
 *   CTRL_ERR_FREEZE:  使用冻结 Err (直角转弯)
 * ================================================================ */

float SelectTargetErr(ControlStrategy_t ctrl, const ElementProp_t *prop,ElementCtx_t *ctx)
{
    if (ctrl == CTRL_ERR_FREEZE && prop != NULL && ctx != NULL) {
        return (ctx->frozen_err);
    }
    return ctx->err_raw;

}