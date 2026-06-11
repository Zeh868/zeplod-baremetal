/**
 * @file bm_hrt.c
 * @brief 高分辨率定时（HRT）调度器实现
 *
 * 基于 HAL 定时器 ISR 按周期触发回调；支持 deadline 错过弱钩子。
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 *
 */
#include "bm_hrt.h"
#include "bm_hal_timer.h"
#include "bm_log.h"

#include <string.h>

#ifndef BM_CONFIG_HRT_TICK_US
#define BM_CONFIG_HRT_TICK_US 100u
#endif

#if BM_CONFIG_HRT_TICK_US == 0u || (1000000u % BM_CONFIG_HRT_TICK_US) != 0u
#error "BM_CONFIG_HRT_TICK_US must divide 1000000 evenly and be non-zero"
#endif

#ifndef BM_CONFIG_HRT_MAX_SLOTS
#define BM_CONFIG_HRT_MAX_SLOTS 16u
#endif

/** 运行时槽：公开描述 + 周期 tick 与下次触发时刻 */
typedef struct {
    bm_hrt_slot_t pub;
    uint32_t period_ticks;
    uint32_t next_tick;
} bm_hrt_runtime_slot_t;

static bm_hrt_runtime_slot_t g_slots[BM_CONFIG_HRT_MAX_SLOTS];
static uint32_t g_slot_count;
static int g_started;

#if defined(__GNUC__) || defined(__clang__)
__attribute__((weak))
#endif
/**
 * @brief HRT 槽错过 deadline 时的弱钩子（默认空实现）
 *
 * @param slot 触发 deadline 错过的槽描述符
 */
void bm_hrt_deadline_missed_hook(const bm_hrt_slot_t *slot) {
    (void)slot;
}

/**
 * @brief 根据配置计算 HRT 定时器频率（Hz）
 *
 * @return 定时器 tick 频率
 */
static uint32_t hrt_tick_hz(void) {
    return 1000000u / BM_CONFIG_HRT_TICK_US;
}

/**
 * @brief 扫描所有槽并触发到期回调
 */
static void hrt_dispatch(void) {
    uint32_t now = bm_hal_timer_get_ticks();
    uint32_t i;

    for (i = 0u; i < g_slot_count; ++i) {
        bm_hrt_runtime_slot_t *slot = &g_slots[i];

        if (slot->pub.trigger != BM_HRT_TRIGGER_TIMER) {
            continue;
        }
        if (slot->period_ticks == 0u) {
            continue;
        }
        if ((int32_t)(now - slot->next_tick) < 0) {
            continue;
        }
        if ((uint32_t)(now - slot->next_tick) >= slot->period_ticks) {
            BM_LOGW("hrt", "deadline missed slot '%s'",
                    slot->pub.name ? slot->pub.name : "(null)");
            bm_hrt_deadline_missed_hook(&slot->pub);
            slot->next_tick = now + slot->period_ticks;
            continue;
        }
        if (slot->pub.callback) {
            slot->pub.callback(slot->pub.context);
        }
        slot->next_tick += slot->period_ticks;
    }
}

/**
 * @brief HAL 定时器 ISR 入口，分派 HRT 回调
 */
static void hrt_timer_isr(void) {
    hrt_dispatch();
}

/**
 * @brief 校验 HRT 槽描述符字段合法性
 *
 * @param slot 槽描述符指针
 * @return BM_OK 有效；BM_ERR_INVALID 字段无效
 */
static int validate_slot(const bm_hrt_slot_t *slot) {
    if (!slot || !slot->callback) {
        return BM_ERR_INVALID;
    }
    if (slot->trigger != BM_HRT_TRIGGER_TIMER) {
        return BM_ERR_INVALID;
    }
    if (slot->period_us == 0u ||
        (slot->period_us % BM_CONFIG_HRT_TICK_US) != 0u) {
        return BM_ERR_INVALID;
    }
    if ((slot->period_us / BM_CONFIG_HRT_TICK_US) == 0u) {
        return BM_ERR_INVALID;
    }
    return BM_OK;
}

/**
 * @brief 初始化 HRT 调度器槽表
 *
 * @param slots 槽描述符数组
 * @param slot_count 槽数量
 * @return BM_OK 成功；BM_ERR_INVALID 参数无效；BM_ERR_OVERFLOW 槽数超限
 */
int bm_hrt_init(const bm_hrt_slot_t *slots, uint32_t slot_count) {
    uint32_t i;
    uint32_t now;

    if (g_started) {
        BM_LOGE("hrt", "init while started");
        return BM_ERR_ALREADY;
    }

    if (!slots && slot_count > 0u) {
        BM_LOGE("hrt", "init invalid slots pointer");
        return BM_ERR_INVALID;
    }
    if (slot_count > BM_CONFIG_HRT_MAX_SLOTS) {
        BM_LOGE("hrt", "init slot overflow count=%u", (unsigned)slot_count);
        return BM_ERR_OVERFLOW;
    }

    for (i = 0u; i < slot_count; ++i) {
        int rc = validate_slot(&slots[i]);
        if (rc != BM_OK) {
            BM_LOGE("hrt", "init slot %u validation failed", (unsigned)i);
            return rc;
        }
    }

    memset(g_slots, 0, sizeof(g_slots));
    g_slot_count = slot_count;
    now = bm_hal_timer_get_ticks();

    for (i = 0u; i < slot_count; ++i) {
        g_slots[i].pub = slots[i];
        g_slots[i].period_ticks = slots[i].period_us / BM_CONFIG_HRT_TICK_US;
        g_slots[i].next_tick = now + g_slots[i].period_ticks;
    }

    BM_LOGI("hrt", "init %u slots tick_us=%u", (unsigned)slot_count,
            (unsigned)BM_CONFIG_HRT_TICK_US);
    return BM_OK;
}

/**
 * @brief 启动 HRT 定时器并注册 ISR 回调
 *
 * @return BM_OK 成功；BM_ERR_ALREADY 已启动；BM_ERR_INVALID HAL 初始化失败
 */
int bm_hrt_start(void) {
    if (g_started) {
        BM_LOGW("hrt", "already started");
        return BM_ERR_ALREADY;
    }
    if (g_slot_count == 0u) {
        BM_LOGI("hrt", "start with zero slots");
        return BM_OK;
    }

    if (bm_hal_timer_init(hrt_tick_hz()) != 0) {
        BM_LOGE("hrt", "hal timer init failed");
        return BM_ERR_INVALID;
    }
    bm_hal_timer_set_callback(hrt_timer_isr);

    {
        uint32_t now = bm_hal_timer_get_ticks();
        uint32_t i;

        for (i = 0u; i < g_slot_count; ++i) {
            g_slots[i].next_tick = now + g_slots[i].period_ticks;
        }
    }

    g_started = 1;
    BM_LOGI("hrt", "started %u slots at %u Hz",
            (unsigned)g_slot_count, (unsigned)hrt_tick_hz());
    return BM_OK;
}

/**
 * @brief 停止 HRT 定时器并注销 ISR 回调
 */
void bm_hrt_stop(void) {
    if (!g_started) {
        return;
    }
    bm_hal_timer_stop();
    bm_hal_timer_set_callback(NULL);
    g_started = 0;
    BM_LOGI("hrt", "stopped");
}

/**
 * @brief 停止调度器并清空全部槽位状态
 */
void bm_hrt_reset(void) {
    bm_hrt_stop();
    memset(g_slots, 0, sizeof(g_slots));
    g_slot_count = 0u;
    BM_LOGI("hrt", "reset");
}
