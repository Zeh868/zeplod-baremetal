/**
 * @file bm_ticker.c
 * @brief 毫秒级周期事件发布器实现
 *
 * 主循环轮询到期槽，向事件总线发布空载荷事件；统计丢弃次数。
 * @author zeh (china_qzh@163.com)
 * @version 1.3
 * @date 2026-06-11
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 * 2026-06-11       1.1            zeh            SIL-2 队列满时推进周期
 * 2026-06-11       1.2            zeh            init 校验 event 索引与定时器频率
 * 2026-06-11       1.3            zeh            get_dropped 临界区读取
 *
 */
#include "bm_ticker.h"
#include "bm_critical_wrap.h"
#include "bm_hal_timer.h"
#include "bm_log.h"
#include "bm_safety.h"
#include "bm_time.h"

#include <string.h>

/** 运行时槽：公开描述 + 周期与丢弃计数 */
typedef struct {
    bm_ticker_slot_t pub;
    uint32_t period_ticks;
    uint32_t next_tick;
    uint32_t dropped;
} bm_ticker_runtime_slot_t;

static bm_ticker_runtime_slot_t g_slots[BM_CONFIG_TICKER_MAX_SLOTS];
static uint32_t g_slot_count;
static int g_initialized;

/**
 * @brief 使用 uint32_t 模加法计算下次 ticker 触发时刻
 */
static uint32_t ticker_deadline_from(uint32_t base, uint32_t period) {
    return base + period;
}

/**
 * @brief 初始化毫秒级周期事件发布器
 *
 * @param slots 槽描述符数组
 * @param slot_count 槽数量
 * @return BM_OK 成功；BM_ERR_INVALID 参数无效；BM_ERR_OVERFLOW 槽数超限
 */
int bm_ticker_init(const bm_ticker_slot_t *slots, uint32_t slot_count) {
    bm_irq_state_t irq_state;
    uint32_t i;
    uint32_t now;

    if (g_initialized) {
        BM_LOGW("ticker", "init while active");
        return BM_ERR_ALREADY;
    }

    if (!slots && slot_count > 0u) {
        BM_LOGE("ticker", "init invalid slots");
        return BM_ERR_INVALID;
    }
    if (slot_count > BM_CONFIG_TICKER_MAX_SLOTS) {
        BM_LOGE("ticker", "init overflow count=%u", (unsigned)slot_count);
        return BM_ERR_OVERFLOW;
    }

    for (i = 0u; i < slot_count; ++i) {
        if (slots[i].period_ms == 0u) {
            BM_LOGE("ticker", "init slot %u zero period", (unsigned)i);
            return BM_ERR_INVALID;
        }
        if (slots[i].event_type >= BM_CONFIG_MAX_EVENT_TYPES) {
            BM_LOGE("ticker", "init slot %u invalid event type", (unsigned)i);
            return BM_ERR_INVALID;
        }
        if (slots[i].priority >= BM_CONFIG_EVENT_PRIORITIES) {
            BM_LOGE("ticker", "init slot %u invalid priority", (unsigned)i);
            return BM_ERR_INVALID;
        }
    }

    if (bm_hal_timer_get_freq() == 0u) {
        BM_LOGE("ticker", "init timer not configured");
        return BM_ERR_NOT_INIT;
    }

    memset(g_slots, 0, sizeof(g_slots));
    g_slot_count = 0u;
    now = bm_hal_timer_get_ticks();

    for (i = 0u; i < slot_count; ++i) {
        uint32_t period_ticks = 0u;
        int trc = bm_time_ms_to_ticks(slots[i].period_ms, &period_ticks);
        if (trc != BM_OK) {
            BM_LOGE("ticker", "init slot %u tick conversion failed", (unsigned)i);
            memset(g_slots, 0, sizeof(g_slots));
            g_slot_count = 0u;
            g_initialized = 0;
            return BM_ERR_INVALID;
        }
        if (period_ticks > (uint32_t)INT32_MAX) {
            BM_LOGE("ticker", "init slot %u period exceeds timer range",
                    (unsigned)i);
            memset(g_slots, 0, sizeof(g_slots));
            g_slot_count = 0u;
            g_initialized = 0;
            return BM_ERR_INVALID;
        }
        g_slots[i].pub = slots[i];
        g_slots[i].period_ticks = period_ticks;
        g_slots[i].next_tick = ticker_deadline_from(now, period_ticks);
    }

    irq_state = BM_CRITICAL_ENTER();
    g_slot_count = slot_count;
    g_initialized = 1;
    BM_CRITICAL_EXIT(irq_state);
    BM_LOGI("ticker", "init %u slots", (unsigned)slot_count);
    return BM_OK;
}

/**
 * @brief 轮询到期槽并向事件总线发布事件
 *
 * 非可重入，仅限主循环调用。
 *
 * @return 本次发布的事件数；BM_ERR_NOT_INIT 未初始化
 */
int bm_ticker_poll(void) {
    uint32_t now;
    uint32_t i;
    int published = 0;

    if (!g_initialized) {
        return BM_ERR_NOT_INIT;
    }

    now = bm_hal_timer_get_ticks();

    for (i = 0u; i < g_slot_count; ++i) {
        bm_ticker_runtime_slot_t *slot = &g_slots[i];

        {
            uint32_t catchup = 0u;
            while (bm_time_reached_u32(now, slot->next_tick) &&
                   catchup < BM_CONFIG_TICKER_MAX_CATCHUP) {
                int rc = bm_event_publish_copy(slot->pub.event_type,
                                               slot->pub.priority,
                                               NULL, 0u);
                if (rc == BM_ERR_OVERFLOW) {
                    slot->dropped = bm_u32_saturating_inc(slot->dropped);
                    BM_LOGW("ticker", "slot %u drop event type=%u total=%u",
                            (unsigned)i, (unsigned)slot->pub.event_type,
                            (unsigned)slot->dropped);
                    slot->next_tick = ticker_deadline_from(
                        slot->next_tick, slot->period_ticks);
                    break;
                }
                if (rc != BM_OK) {
                    BM_LOGE("ticker", "slot %u publish failed rc=%d",
                            (unsigned)i, rc);
                    return rc;
                }
                published++;
                catchup++;
                slot->next_tick = ticker_deadline_from(
                    slot->next_tick, slot->period_ticks);
            }
            if (bm_time_reached_u32(now, slot->next_tick)) {
                slot->next_tick =
                    ticker_deadline_from(now, slot->period_ticks);
            }
        }
    }

    BM_LOGT("ticker", "poll published %d", published);
    return published;
}

/**
 * @brief 查询指定槽因队列满而丢弃的事件次数
 *
 * @param slot_index 槽索引
 * @return 丢弃次数；索引无效返回 0
 */
uint32_t bm_ticker_get_dropped(uint32_t slot_index) {
    bm_irq_state_t irq_state = BM_CRITICAL_ENTER();
    uint32_t count = 0u;

    if (bm_index_in_range_u32(slot_index, g_slot_count)) {
        count = g_slots[slot_index].dropped;
    }
    BM_CRITICAL_EXIT(irq_state);
    return count;
}

/**
 * @brief 重置发布器全部内部状态
 */
void bm_ticker_reset(void) {
    bm_irq_state_t irq_state = BM_CRITICAL_ENTER();

    g_initialized = 0;
    g_slot_count = 0u;
    BM_CRITICAL_EXIT(irq_state);
    memset(g_slots, 0, sizeof(g_slots));
    BM_LOGI("ticker", "reset");
}

int bm_ticker_is_initialized(void) {
    bm_irq_state_t irq_state = BM_CRITICAL_ENTER();
    int initialized = g_initialized;

    BM_CRITICAL_EXIT(irq_state);
    return initialized;
}
