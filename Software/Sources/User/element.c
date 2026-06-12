#include "element.h"
#include "debug_arch.h"
#include "FreeRTOS.h"
#include "task.h"

extern uint8_t manual_control_flag;

/* ================================================================
 * 环岛 ADC 阈值
 *   RingEntrance[x]: ADC 高于此值 → 候选进入环岛
 *   RingExit[x]:     ADC 低于此值 → 候选退出环岛
 *   进入/退出之间形成滞回区, 防止边界抖动
 * ================================================================ */
static float RingEntrance[4];
static float RingExit[4];
static float angle_pitch = 5.0f;

/* ================================================================
 * 环岛多数投票窗 (Majority Voting Window)
 *   窗长可调 0~100, 0 = 旁路 (即时切换)
 *   左右环各自独立窗口, 每票含方向判别 (adc[n] > adc[对侧]):
 *     STRAIGHT: 左窗 1 = "adc[2]超阈值 且 大于adc[0] 且 车身水平"
 *              右窗 1 = "adc[0]超阈值 且 大于adc[2] 且 车身水平"
 *     RING_*_ENTRANCE: 1 = "对应通道退出条件成立"
 *     IN_RING:        1 = "环方向对应通道再次尖峰(出环)"
 *   状态切换后旧投票自然老化, 提供最短保持时间
 * ================================================================ */
static uint8_t xdata ring_left_vote_win[100];
static uint8_t ring_left_win_idx;     /* 左环窗口写入位置 (循环) */
static uint8_t ring_left_win_cnt;     /* 左环窗口内 1 的个数 (增量维护) */

static uint8_t xdata ring_right_vote_win[100];
static uint8_t ring_right_win_idx;    /* 右环窗口写入位置 (循环) */
static uint8_t ring_right_win_cnt;    /* 右环窗口内 1 的个数 (增量维护) */

static uint8_t ring_win_len;          /* 有效窗长 0~100, 0=旁路 */
static uint8_t ring_entry_thresh;     /* 进入阈值: 1 的个数 >= 此值 */
static uint8_t ring_exit_thresh;      /* 退出阈值: 1 的个数 >= 此值 */

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
    ELEMENT_RING_RIGHT_ENTRANCE,
    /* 继续添加: ELEMENT_WALL, ELEMENT_CROSS, ... */
};

static uint8_t  g_track_table_enabled = 0;   /* 上位机可调: 1=启用, 0=旁路(无约束) */
static uint8_t  g_track_idx = 0;             /* 指向下一个预期元素的索引 */

/* ================================================================
 * 上墙检测多数投票窗 (Majority Voting Window)
 *   窗长固定 10 cycles
 *   进入条件: pitch 超过 wall_entry_pitch 时投票 1
 *   退出条件: pitch 低于 wall_exit_pitch  时投票 1
 *   仅从 STRAIGHT 进入上墙, 从 WALL 退回 STRAIGHT
 * ================================================================ */
static uint8_t xdata wall_vote_win[10];
static uint8_t wall_win_idx;          /* 当前写入位置 (循环) */
static uint8_t wall_win_cnt;          /* 窗口内 1 的个数 (增量维护) */
static float   wall_entry_pitch;      /* 进入上墙的 pitch 阈值 (度) */
static float   wall_exit_pitch;       /* 退出上墙的 pitch 阈值 (度) */
static uint8_t wall_entry_thresh;     /* 进入阈值: 1 的个数 >= 此值 */
static uint8_t wall_exit_thresh;      /* 退出阈值: 1 的个数 >= 此值 */

/* ================================================================
 * 滚筒检测多数投票窗 (Majority Voting Window)
 *   窗长固定 10 cycles
 *   进入条件: roll 超过 barrel_entry_roll 时投票 1
 *   退出条件: roll 低于 barrel_exit_roll  时投票 1
 *   仅从 STRAIGHT 进入滚筒, 从 BARREL 退回 STRAIGHT
 * ================================================================ */
static uint8_t xdata barrel_vote_win[10];
static uint8_t barrel_win_idx;        /* 当前写入位置 (循环) */
static uint8_t barrel_win_cnt;        /* 窗口内 1 的个数 (增量维护) */
static float   barrel_entry_roll;     /* 进入滚筒的 roll 阈值 (度) */
static float   barrel_exit_roll;      /* 退出滚筒的 roll 阈值 (度) */
static uint8_t barrel_entry_thresh;   /* 进入阈值: 1 的个数 >= 此值 */
static uint8_t barrel_exit_thresh;    /* 退出阈值: 1 的个数 >= 此值 */

/* ================================================================
 * 安全停车检测 (Safety Stop)
 *   离赛道超时: 4路ADC全部丢失超过 g_offtrack_timeout_ms → SAFETY_STOP
 * ================================================================ */
static float   g_offtrack_timeout_ms   = 500.0f;  /* 离赛道超时 (ms) */
static uint32_t g_offtrack_start_tick  = 0;       /* 信号丢失起始 tick */
static uint8_t  g_offtrack_active      = 0;       /* 当前是否正在计时 */

/* ---- 将投票推入环形窗口, 增量更新计数 (左右环共用) ---- */
static void Ring_Vote_Push(uint8_t vote,
                           uint8_t xdata *win,
                           uint8_t *idx,
                           uint8_t *cnt)
{
    uint8_t old_vote;

    if (ring_win_len == 0) return;
    if (*idx >= ring_win_len) {
        *idx = 0;
    }

    old_vote = win[*idx];
    if (old_vote == 1) {
        (*cnt)--;
    }
    win[*idx] = vote;
    if (vote == 1) {
        (*cnt)++;
    }
    /* 防止上位机调小窗长后计数超过窗长, 导致 uint8 减法溢出 */
    if (*cnt > ring_win_len) {
        *cnt = ring_win_len;
    }
    (*idx)++;
}

/* ---- 元素表: 完成一个元素后推进索引, 超出表尾后不再推进(赛道跑完) ---- */
static void Track_Advance(void)
{
    if (g_track_table_enabled && g_track_idx < TRACK_TABLE_LEN) {
        g_track_idx++;
    }
}

/* ---- 将上墙投票推入环形窗口, 增量更新计数 ---- */
static void Wall_Vote_Push(uint8_t vote)
{
    uint8_t old_vote;

    old_vote = wall_vote_win[wall_win_idx];
    if (old_vote == 1) {
        wall_win_cnt--;
    }
    wall_vote_win[wall_win_idx] = vote;
    if (vote == 1) {
        wall_win_cnt++;
    }
    wall_win_idx++;
    if (wall_win_idx >= 10) {
        wall_win_idx = 0;
    }
}

/* ---- 将滚筒投票推入环形窗口, 增量更新计数 ---- */
static void Barrel_Vote_Push(uint8_t vote)
{
    uint8_t old_vote;

    old_vote = barrel_vote_win[barrel_win_idx];
    if (old_vote == 1) {
        barrel_win_cnt--;
    }
    barrel_vote_win[barrel_win_idx] = vote;
    if (vote == 1) {
        barrel_win_cnt++;
    }
    barrel_win_idx++;
    if (barrel_win_idx >= 10) {
        barrel_win_idx = 0;
    }
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
    Debug_Register("Ring_AngThr", DBG_FLOAT | STATIC_TYPE, &angle_pitch);

    /* 环岛投票窗参数 (上位机可调) */
    Debug_Register("Ring_WinLen",  DBG_UINT8 | STATIC_TYPE, &ring_win_len);
    Debug_Register("Ring_EntThr",  DBG_UINT8 | STATIC_TYPE, &ring_entry_thresh);
    Debug_Register("Ring_ExtThr",  DBG_UINT8 | STATIC_TYPE, &ring_exit_thresh);

    /* 开环PASS超时帧数 (上位机可调, 1帧=2ms) */
    Debug_Register("OL_Ang",     DBG_FLOAT | STATIC_TYPE, &g_ol_ring_angle);
    Debug_Register("OL_Yaw",     DBG_FLOAT | STATIC_TYPE, &g_ol_ring_yawrate);
    Debug_Register("PassTmOut",  DBG_UINT16 | STATIC_TYPE, &g_pass_timeout_frames);
    Debug_Register("RingSide",   DBG_UINT8 | STATIC_TYPE, &g_ring_side);
    Debug_Register("RingExYaw",  DBG_FLOAT | STATIC_TYPE, &g_ring_exit_yaw_thresh);
    Debug_Register("TblEnable",  DBG_UINT8 | STATIC_TYPE, &g_track_table_enabled);

    /* 上墙 pitch 阈值 */
    Debug_Register("Wall_EnPitch", DBG_FLOAT | STATIC_TYPE, &wall_entry_pitch);
    Debug_Register("Wall_ExPitch", DBG_FLOAT | STATIC_TYPE, &wall_exit_pitch);

    /* 上墙投票窗参数 (上位机可调) */
    Debug_Register("Wall_EntThr",  DBG_UINT8 | STATIC_TYPE, &wall_entry_thresh);
    Debug_Register("Wall_ExtThr",  DBG_UINT8 | STATIC_TYPE, &wall_exit_thresh);

    /* 滚筒 roll 阈值 */
    Debug_Register("Bar_EnRoll",  DBG_FLOAT | STATIC_TYPE, &barrel_entry_roll);
    Debug_Register("Bar_ExRoll",  DBG_FLOAT | STATIC_TYPE, &barrel_exit_roll);

    /* 滚筒投票窗参数 (上位机可调) */
    Debug_Register("Bar_EntThr",  DBG_UINT8 | STATIC_TYPE, &barrel_entry_thresh);
    Debug_Register("Bar_ExtThr",  DBG_UINT8 | STATIC_TYPE, &barrel_exit_thresh);

    /* 默认阈值 */
    RingEntrance[0] = 2500.0f;
    RingEntrance[1] = 0.0f;
    RingEntrance[2] = 2500.0f;
    RingEntrance[3] = 0.0f;

    RingExit[0] = 700.0f;
    RingExit[1] = 0.0f;
    RingExit[2] = 700.0f;
    RingExit[3] = 0.0f;

    /* 窗长默认 10 帧 (20ms), 阈值默认 7 (70% 多数) */
    ring_win_len      = 10;
    ring_entry_thresh = 7;
    ring_exit_thresh  = 7;

    /* 上墙 pitch 默认: 进入 >30°, 退出 <20° (滞回) */
    wall_entry_pitch  = 30.0f;
    wall_exit_pitch   = 20.0f;

    /* 上墙投票: 窗长固定 10 cycles, 阈值 7 (70% 多数) */
    wall_entry_thresh = 7;
    wall_exit_thresh  = 7;

    /* 滚筒 roll 默认: 进入 >25°, 退出 <15° (滞回) */
    barrel_entry_roll  = 25.0f;
    barrel_exit_roll   = 15.0f;

    /* 滚筒投票: 窗长固定 10 cycles, 阈值 7 (70% 多数) */
    barrel_entry_thresh = 7;
    barrel_exit_thresh  = 7;

    /* 安全停车参数 (上位机在线调参) */
    Debug_Register("OffTkMs",   DBG_FLOAT | STATIC_TYPE, &g_offtrack_timeout_ms);

    /* 清零窗口缓冲区 (xdata 上电值不确定) */
    for (i = 0; i < 100; i++) {
        ring_left_vote_win[i] = 0;
        ring_right_vote_win[i] = 0;
    }
    /* 上墙窗口清零 */
    for (i = 0; i < 10; i++) {
        wall_vote_win[i] = 0;
    }
    /* 滚筒窗口清零 */
    for (i = 0; i < 10; i++) {
        barrel_vote_win[i] = 0;
    }
    ring_left_win_idx  = 0;
    ring_left_win_cnt  = 0;
    ring_right_win_idx = 0;
    ring_right_win_cnt = 0;
    wall_win_idx = 0;
    wall_win_cnt = 0;
    barrel_win_idx = 0;
    barrel_win_cnt = 0;
}

/* ---- 重置离赛道超时计时器 (供 Change_Maunal 等外部调用) ---- */
void Element_ResetOfftrackTimer(void)
{
    g_offtrack_active = 0;
}

/* adc0 是左 */
void Element_Handle(element_t *element, float adc_values[], float pitch, float roll, float yaw)
{
    /* C89: 所有局部变量声明在开头 */
    float   abs_pitch;
    float   abs_roll;
    uint8_t left_vote;   /* 左环岛(adc[2])本帧投票 */
    uint8_t right_vote;  /* 右环岛(adc[0])本帧投票 */
    uint8_t wall_vote;
    uint8_t barrel_vote;
    float   yaw_delta;   /* PASS 阶段 yaw 变化量 */
    uint8_t i;           /* 循环/清零用 */

    /* 安全检测用局部变量 */
    uint8_t all_adc_low;
    uint32_t elapsed_ms;
    uint32_t now_tick;

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

    /* ---- 0.5 元素表: 禁用时复位索引 ---- */
    if (!g_track_table_enabled) {
        g_track_idx = 0;
    }

    /* ---- 1. 根据当前状态计算本帧投票 ---- */
    abs_pitch = (pitch > 0) ? pitch : -pitch;
    switch (*element) {
    case ELEMENT_STRAIGHT:
        {/* C89: 块作用域声明 */
            uint8_t track_expected;
            /* 左环: adc[2]超阈值 且 大于右通道(方向判别) 且 车身水平 */
            left_vote = ((adc_values[2] > RingEntrance[2]) &&
                         (adc_values[2] > adc_values[0]) &&
                         (abs_pitch < angle_pitch)) ? 1 : 0;
            /* 右环: adc[0]超阈值 且 大于左通道(方向判别) 且 车身水平 */
            right_vote = ((adc_values[0] > RingEntrance[0]) &&
                          (adc_values[0] > adc_values[2]) &&
                          (abs_pitch < angle_pitch)) ? 1 : 0;

            /* 元素表约束: 非预期方向强制不投票 */
            if (g_track_table_enabled) {
                if (g_track_idx < TRACK_TABLE_LEN) {
                    track_expected = g_track_table[g_track_idx];
                } else {
                    track_expected = ELEMENT_STRAIGHT;  /* 超出表尾, 不再放行任何环岛 */
                }
                if (track_expected != ELEMENT_RING_LEFT_ENTRANCE)  left_vote  = 0;
                if (track_expected != ELEMENT_RING_RIGHT_ENTRANCE) right_vote = 0;
            }
        }
        break;

    case ELEMENT_RING_LEFT_ENTRANCE:
        left_vote  = (adc_values[2] < RingExit[2]) ? 1 : 0;
        right_vote = 0;
        break;

    case ELEMENT_RING_RIGHT_ENTRANCE:
        left_vote  = 0;
        right_vote = (adc_values[0] < RingExit[0]) ? 1 : 0;
        break;

    case ELEMENT_IN_RING:
        /* 环内等待出环: 对应通道再次出现尖峰(大于进入阈值)即投票1 */
        if (g_ring_side == 0) {
            left_vote  = (adc_values[2] > RingEntrance[2]) ? 1 : 0;
            right_vote = 0;
        } else {
            left_vote  = 0;
            right_vote = (adc_values[0] > RingEntrance[0]) ? 1 : 0;
        }
        break;

    default:
        left_vote  = 0;
        right_vote = 0;
        break;
    }

    /* ---- 2. 推入对应窗口 ---- */
    Ring_Vote_Push(left_vote,  ring_left_vote_win,  &ring_left_win_idx,  &ring_left_win_cnt);
    Ring_Vote_Push(right_vote, ring_right_vote_win, &ring_right_win_idx, &ring_right_win_cnt);

    /* ---- 3. 上墙投票 ---- */
    switch (*element) {
    case ELEMENT_STRAIGHT:
        /* 在直线状态下, pitch 超过进入阈值即投票 1 */
        wall_vote = (abs_pitch > wall_entry_pitch) ? 1 : 0;
        break;

    case ELEMENT_WALL:
        /* 在上墙状态下, pitch 低于退出阈值即投票 1 */
        wall_vote = (abs_pitch < wall_exit_pitch) ? 1 : 0;
        break;

    default:
        /* 其他状态不关心上墙, 投票 0 自然老化 */
        wall_vote = 0;
        break;
    }
    Wall_Vote_Push(wall_vote);

    /* ---- 3.5 滚筒投票 ---- */
    abs_roll = (roll > 0) ? roll : -roll;
    switch (*element) {
    case ELEMENT_STRAIGHT:
        /* 在直线状态下, roll 超过进入阈值即投票 1 */
        barrel_vote = (abs_roll > barrel_entry_roll) ? 1 : 0;
        break;

    case ELEMENT_BARREL:
        /* 在滚筒状态下, roll 低于退出阈值即投票 1 */
        barrel_vote = (abs_roll < barrel_exit_roll) ? 1 : 0;
        break;

    default:
        /* 其他状态不关心滚筒, 投票 0 自然老化 */
        barrel_vote = 0;
        break;
    }
    Barrel_Vote_Push(barrel_vote);

    /* ---- 4. 状态转换判断 ---- */
    switch (*element) {
    case ELEMENT_STRAIGHT:
        if (ring_win_len == 0) {
            /* 旁路: 即时切换, 加入方向判别, 右环优先 */
            if ((adc_values[0] > RingEntrance[0]) &&
                (adc_values[0] > adc_values[2]) &&
                (abs_pitch < angle_pitch)) {
                *element = ELEMENT_RING_RIGHT_ENTRANCE;
            } else if ((adc_values[2] > RingEntrance[2]) &&
                       (adc_values[2] > adc_values[0]) &&
                       (abs_pitch < angle_pitch)) {
                *element = ELEMENT_RING_LEFT_ENTRANCE;
            }
        } else {
            /* 多数投票: 左右独立判断, 右环优先 */
            if (ring_right_win_cnt >= ring_entry_thresh) {
                *element = ELEMENT_RING_RIGHT_ENTRANCE;
            } else if (ring_left_win_cnt >= ring_entry_thresh) {
                *element = ELEMENT_RING_LEFT_ENTRANCE;
            }
        }

        /* 上墙检测: 仅从直线状态进入, 且未被环岛抢占 */
        if (*element == ELEMENT_STRAIGHT) {
            if (wall_win_cnt >= wall_entry_thresh) {
                *element = ELEMENT_WALL;
            }
        }

        /* 滚筒检测: 仅从直线状态进入, 且未被环岛/上墙抢占 */
        if (*element == ELEMENT_STRAIGHT) {
            if (barrel_win_cnt >= barrel_entry_thresh) {
                *element = ELEMENT_BARREL;
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
            /* 左转角度达标(负向) OR 超时 → 进入环内, 清两边投票窗 */
            if (yaw_delta <= -g_ol_ring_angle ||
                g_pass_frame_cnt >= g_pass_timeout_frames) {
                *element = ELEMENT_IN_RING;
                g_ring_side = 0;  /* 左环, 出环检测看 adc[2] */
                g_ring_entry_yaw = yaw;  /* 记录入环 yaw, 用于退环门禁 */
                for (i = 0; i < ring_win_len; i++) {
                    ring_left_vote_win[i] = 0;
                    ring_right_vote_win[i] = 0;
                }
                ring_left_win_cnt  = 0;
                ring_left_win_idx  = 0;
                ring_right_win_cnt = 0;
                ring_right_win_idx = 0;
            }
        }
        break;

    case ELEMENT_RING_RIGHT_PASS:
        {
            g_pass_frame_cnt++;
            yaw_delta = yaw - g_pass_entry_yaw;
            /* 右转角度达标(正向) OR 超时 → 进入环内, 清两边投票窗 */
            if (yaw_delta >= g_ol_ring_angle ||
                g_pass_frame_cnt >= g_pass_timeout_frames) {
                *element = ELEMENT_IN_RING;
                g_ring_side = 1;  /* 右环, 出环检测看 adc[0] */
                g_ring_entry_yaw = yaw;  /* 记录入环 yaw, 用于退环门禁 */
                for (i = 0; i < ring_win_len; i++) {
                    ring_left_vote_win[i] = 0;
                    ring_right_vote_win[i] = 0;
                }
                ring_left_win_cnt  = 0;
                ring_left_win_idx  = 0;
                ring_right_win_cnt = 0;
                ring_right_win_idx = 0;
            }
        }
        break;

    case ELEMENT_IN_RING:
        /* 环内正常循迹, 等待对应通道第二个 ADC 尖峰(出环)
         * 前置门禁: yaw 累计变化量达标才允许出环
         *   向左转 yaw 增加 → 左环看正向, 右环看负向 */
        yaw_delta = yaw - g_ring_entry_yaw;
        if (g_ring_side == 0) {
            /* 左环: 需要 yaw 正向累计 >= 阈值 */
            if (yaw_delta < g_ring_exit_yaw_thresh) {
                break;
            }
        } else {
            /* 右环: 需要 yaw 负向累计 <= -阈值 */
            if (yaw_delta > -g_ring_exit_yaw_thresh) {
                break;
            }
        }

        if (ring_win_len == 0) {
            /* 旁路: 即时判断 */
            if (g_ring_side == 0) {
                if (adc_values[2] > RingEntrance[2]) {
                    *element = ELEMENT_STRAIGHT;
                    Track_Advance();
                }
            } else {
                if (adc_values[0] > RingEntrance[0]) {
                    *element = ELEMENT_STRAIGHT;
                    Track_Advance();
                }
            }
        } else {
            /* 多数投票: 仅看当前环方向对应窗口的尖峰确认数 */
            if (g_ring_side == 0) {
                if (ring_left_win_cnt >= ring_entry_thresh) {
                    *element = ELEMENT_STRAIGHT;
                    Track_Advance();
                }
            } else {
                if (ring_right_win_cnt >= ring_entry_thresh) {
                    *element = ELEMENT_STRAIGHT;
                    Track_Advance();
                }
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
        /* 滚筒状态下, roll 低于退出阈值投票达标 → 退回直线 */
        if (barrel_win_cnt >= barrel_exit_thresh) {
            *element = ELEMENT_STRAIGHT;
            Track_Advance();
        }
        break;

    case ELEMENT_WALL:
        if (wall_win_cnt >= wall_exit_thresh) {
            *element = ELEMENT_STRAIGHT;
            Track_Advance();
        }
        break;

    case ELEMENT_CROSS:
        break;

    case ELEMENT_SAFETY_STOP:
        /* 任一通道恢复电感值 → 退出安全停车, 切回手动模式 */
        if (adc_values[0] > 10.0f || adc_values[1] > 10.0f
            || adc_values[2] > 10.0f || adc_values[3] > 10.0f) {
            *element = ELEMENT_STRAIGHT;
            manual_control_flag = 1;
            g_offtrack_active = 0;  /* 确保离赛道定时器复位, 防止切回自动模式时误触发 */
        }
        break;

    default:
        break;
    }
}
