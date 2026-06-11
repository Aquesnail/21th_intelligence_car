#include "element.h"
#include "debug_arch.h"

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
 *   同一窗口在不同状态下跟踪不同条件:
 *     STRAIGHT:       1 = "任一通道进入条件成立"
 *     RING_*_ENTRANCE: 1 = "对应通道退出条件成立"
 *   状态切换后旧投票自然老化, 提供最短保持时间
 * ================================================================ */
static uint8_t xdata ring_vote_win[100];
static uint8_t ring_win_idx;          /* 当前写入位置 (循环) */
static uint8_t ring_win_cnt;          /* 窗口内 1 的个数 (增量维护) */
static uint8_t ring_win_len;          /* 有效窗长 0~100, 0=旁路 */
static uint8_t ring_entry_thresh;     /* 进入阈值: 1 的个数 >= 此值 */
static uint8_t ring_exit_thresh;      /* 退出阈值: 1 的个数 >= 此值 */

/* ================================================================
 * 开环 PASS 阶段参数
 *   进入 ENTRANCE 后立即切入 PASS, 记录起始 yaw, 帧计数
 *   退出条件: yaw 变化量达标 OR 帧数超时
 *   目标角度引用自 app_main_control.c, 上位机可调
 * ================================================================ */
extern float g_ol_ring_left_angle;
extern float g_ol_ring_right_angle;

static float    g_pass_entry_yaw = 0.0f;
static uint16_t g_pass_frame_cnt = 0;
static uint16_t g_pass_timeout_frames = 200;  /* 默认200帧=400ms, 上位机可调 */

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

/* ---- 将投票推入环形窗口, 增量更新计数 ---- */
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
    Debug_Register("PassTmOut",  DBG_UINT16 | STATIC_TYPE, &g_pass_timeout_frames);

    /* 上墙 pitch 阈值 */
    Debug_Register("Wall_EnPitch", DBG_FLOAT | STATIC_TYPE, &wall_entry_pitch);
    Debug_Register("Wall_ExPitch", DBG_FLOAT | STATIC_TYPE, &wall_exit_pitch);

    /* 上墙投票窗参数 (上位机可调) */
    Debug_Register("Wall_EntThr",  DBG_UINT8 | STATIC_TYPE, &wall_entry_thresh);
    Debug_Register("Wall_ExtThr",  DBG_UINT8 | STATIC_TYPE, &wall_exit_thresh);

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

    /* 清零窗口缓冲区 (xdata 上电值不确定) */
    for (i = 0; i < 100; i++) {
        ring_vote_win[i] = 0;
    }
    /* 上墙窗口清零 */
    for (i = 0; i < 10; i++) {
        wall_vote_win[i] = 0;
    }
    ring_win_idx = 0;
    ring_win_cnt = 0;
    wall_win_idx = 0;
    wall_win_cnt = 0;
}

/* adc0 是左 */
void Element_Handle(element_t *element, float adc_values[], float pitch, float roll, float yaw)
{
    float   abs_pitch;
    uint8_t vote;
    uint8_t wall_vote;

    abs_pitch = (pitch > 0) ? pitch : -pitch;

    /* ---- 1. 根据当前状态计算本帧投票 ---- */
    switch (*element) {
    case ELEMENT_STRAIGHT:
        /* 左环岛(adc2) 或 右环岛(adc0) 任一满足进入条件, 且车身水平 */
        vote = (((adc_values[0] > RingEntrance[0]) ||
                 (adc_values[2] > RingEntrance[2])) &&
                (abs_pitch < angle_pitch)) ? 1 : 0;
        break;

    case ELEMENT_RING_LEFT_ENTRANCE:
        /* adc2 低于退出阈值 即投票 1 */
        vote = (adc_values[2] < RingExit[2]) ? 1 : 0;
        break;

    case ELEMENT_RING_RIGHT_ENTRANCE:
        /* adc0 低于退出阈值 即投票 1 */
        vote = (adc_values[0] < RingExit[0]) ? 1 : 0;
        break;

    default:
        vote = 0;
        break;
    }

    /* ---- 2. 推入窗口 ---- */
    Ring_Vote_Push(vote);

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

    /* ---- 4. 状态转换判断 ---- */
    switch (*element) {
    case ELEMENT_STRAIGHT:
        if (ring_win_len == 0) {
            /* 旁路: 即时切换, 通道 0 优先判断为右环岛 */
            if ((adc_values[0] > RingEntrance[0]) &&
                (abs_pitch < angle_pitch)) {
                *element = ELEMENT_RING_RIGHT_ENTRANCE;
            } else if ((adc_values[2] > RingEntrance[2]) &&
                       (abs_pitch < angle_pitch)) {
                *element = ELEMENT_RING_LEFT_ENTRANCE;
            }
        } else {
            if (ring_win_cnt >= ring_entry_thresh) {
                /* 两通道同时满足时, 选数值较大的方向 */
                if (adc_values[0] > RingEntrance[0] &&
                    adc_values[0] > adc_values[2]) {
                    *element = ELEMENT_RING_RIGHT_ENTRANCE;
                } else {
                    *element = ELEMENT_RING_LEFT_ENTRANCE;
                }
            }
        }

        /* 上墙检测: 仅从直线状态进入, 且未被环岛抢占 */
        if (*element == ELEMENT_STRAIGHT) {
            if (wall_win_cnt >= wall_entry_thresh) {
                *element = ELEMENT_WALL;
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
            float yaw_delta;
            uint8_t i;
            g_pass_frame_cnt++;
            yaw_delta = yaw - g_pass_entry_yaw;
            /* 左转角度达标(负向) OR 超时 → 回到直线, 清投票窗防重触发 */
            if (yaw_delta <= g_ol_ring_left_angle ||
                g_pass_frame_cnt >= g_pass_timeout_frames) {
                *element = ELEMENT_STRAIGHT;
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
            float yaw_delta;
            uint8_t i;
            g_pass_frame_cnt++;
            yaw_delta = yaw - g_pass_entry_yaw;
            /* 右转角度达标(正向) OR 超时 → 回到直线, 清投票窗防重触发 */
            if (yaw_delta >= g_ol_ring_right_angle ||
                g_pass_frame_cnt >= g_pass_timeout_frames) {
                *element = ELEMENT_STRAIGHT;
                for (i = 0; i < ring_win_len; i++) {
                    ring_vote_win[i] = 0;
                }
                ring_win_cnt = 0;
                ring_win_idx = 0;
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
        break;

    case ELEMENT_WALL:
        if (wall_win_cnt >= wall_exit_thresh) {
            *element = ELEMENT_STRAIGHT;
        }
        break;

    case ELEMENT_CROSS:
        break;

    default:
        break;
    }
}
