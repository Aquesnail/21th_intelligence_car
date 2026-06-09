#include "element.h"
#include "debug_arch.h"

static float Ring[4] = {0.0f}, angle_pitch = 5.0f;

/* ================================================================
 * 环岛多数投票窗 (Majority Voting Window)
 *   窗长可调 0~100, 0 = 旁路 (即时切换)
 *   一个窗口同时服务进入和退出:
 *     - 进入: 窗口中"条件成立(1)"的个数 >= ring_entry_thresh
 *     - 退出: 窗口中"条件不成立(0)"的个数 >= ring_exit_thresh
 *   ring_win_cnt 增量维护, 避免每帧遍历求和
 * ================================================================ */
static uint8_t xdata ring_vote_win[100];
static uint8_t ring_win_idx;          /* 当前写入位置 (循环) */
static uint8_t ring_win_cnt;          /* 窗口内 1 的个数 */
static uint8_t ring_win_len;          /* 有效窗长 0~100, 0=旁路 */
static uint8_t ring_entry_thresh;     /* 进入阈值: 1 的个数 >= 此值 */
static uint8_t ring_exit_thresh;      /* 退出阈值: 0 的个数 >= 此值 */

/* ---- 环岛入口条件判断 ---- */
static uint8_t Ring_Condition_Met(float adc_values[], float abs_pitch)
{
    if (adc_values[0] > Ring[0] &&
        adc_values[2] > Ring[2] &&
        adc_values[0] > Ring[2] &&
        (abs_pitch < angle_pitch)) {
        return 1;
    }
    return 0;
}

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
    /* 防止上位机调小窗长后计数超过窗长, 导致 zero_cnt 溢出 */
    if (ring_win_cnt > ring_win_len) {
        ring_win_cnt = ring_win_len;
    }
    ring_win_idx++;
}

void Element_Init(void)
{
    uint8_t i;

    /* 环岛 ADC 阈值 */
    Debug_Register("Ring1", DBG_FLOAT | STATIC_TYPE, &Ring[0]);
    Debug_Register("Ring2", DBG_FLOAT | STATIC_TYPE, &Ring[1]);
    Debug_Register("Ring3", DBG_FLOAT | STATIC_TYPE, &Ring[2]);
    Debug_Register("Ring4", DBG_FLOAT | STATIC_TYPE, &Ring[3]);

    /* 环岛角度阈值 */
    Debug_Register("Ring_AngThr", DBG_FLOAT | STATIC_TYPE, &angle_pitch);

    /* 环岛投票窗参数 (上位机可调) */
    Debug_Register("Ring_WinLen",  DBG_UINT8 | STATIC_TYPE, &ring_win_len);
    Debug_Register("Ring_EntThr",  DBG_UINT8 | STATIC_TYPE, &ring_entry_thresh);
    Debug_Register("Ring_ExtThr",  DBG_UINT8 | STATIC_TYPE, &ring_exit_thresh);

    /* 窗长默认 10 帧 (20ms), 阈值默认 7 (70% 多数) */
    ring_win_len      = 10;
    ring_entry_thresh = 7;
    ring_exit_thresh  = 7;

    /* 清零窗口缓冲区 (xdata 上电值不确定) */
    for (i = 0; i < 100; i++) {
        ring_vote_win[i] = 0;
    }
    ring_win_idx = 0;
    ring_win_cnt = 0;
}

/* adc0 是左 */
void Element_Handle(element_t *element, float adc_values[], float pitch, float roll, float yaw)
{
    float   abs_pitch;
    uint8_t vote;
    uint8_t zero_cnt;

    abs_pitch = (pitch > 0) ? pitch : -pitch;
    vote      = Ring_Condition_Met(adc_values, abs_pitch);
    Ring_Vote_Push(vote);

    switch (*element) {
    case ELEMENT_STRAIGHT:
        if (ring_win_len == 0) {
            /* 旁路: 即时切换 */
            if (vote) {
                *element = ELEMENT_RING_LEFT_ENTRANCE;
            }
        } else {
            if (ring_win_cnt >= ring_entry_thresh) {
                *element = ELEMENT_RING_LEFT_ENTRANCE;
            }
        }
        break;

    case ELEMENT_RING_LEFT_ENTRANCE:
        if (ring_win_len == 0) {
            if (!vote) {
                *element = ELEMENT_STRAIGHT;
            }
        } else {
            zero_cnt = ring_win_len - ring_win_cnt;
            if (zero_cnt >= ring_exit_thresh) {
                *element = ELEMENT_STRAIGHT;
            }
        }
        break;

    case ELEMENT_RING_RIGHT_ENTRANCE:
        if (ring_win_len == 0) {
            if (!vote) {
                *element = ELEMENT_STRAIGHT;
            }
        } else {
            zero_cnt = ring_win_len - ring_win_cnt;
            if (zero_cnt >= ring_exit_thresh) {
                *element = ELEMENT_STRAIGHT;
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
        break;

    case ELEMENT_CROSS:
        break;

    default:
        break;
    }
}
