#include "bm_hrt.h"
#include "bm_hal_timer.h"

#include <string.h>

#ifndef BM_CONFIG_HRT_TICK_US
#define BM_CONFIG_HRT_TICK_US 100u
#endif

#ifndef BM_CONFIG_HRT_MAX_SLOTS
#define BM_CONFIG_HRT_MAX_SLOTS 16u
#endif

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
void bm_hrt_deadline_missed_hook(const bm_hrt_slot_t *slot) {
    (void)slot;
}

static uint32_t hrt_tick_hz(void) {
    return 1000000u / BM_CONFIG_HRT_TICK_US;
}

static void hrt_dispatch(void) {
    uint32_t now = bm_hal_timer_get_ticks();
    uint32_t i;

    for (i = 0u; i < g_slot_count; ++i) {
        bm_hrt_runtime_slot_t *slot = &g_slots[i];

        if (slot->pub.trigger != BM_HRT_TRIGGER_TIMER) {
            continue;
        }
        if ((int32_t)(now - slot->next_tick) < 0) {
            continue;
        }
        if ((uint32_t)(now - slot->next_tick) >= slot->period_ticks) {
            bm_hrt_deadline_missed_hook(&slot->pub);
            slot->next_tick = now + slot->period_ticks;
        } else {
            slot->next_tick += slot->period_ticks;
            if (slot->pub.callback) {
                slot->pub.callback(slot->pub.context);
            }
        }
    }
}

static void hrt_timer_isr(void) {
    hrt_dispatch();
}

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
    return BM_OK;
}

int bm_hrt_init(const bm_hrt_slot_t *slots, uint32_t slot_count) {
    uint32_t i;
    uint32_t now;

    if (!slots && slot_count > 0u) {
        return BM_ERR_INVALID;
    }
    if (slot_count > BM_CONFIG_HRT_MAX_SLOTS) {
        return BM_ERR_OVERFLOW;
    }

    for (i = 0u; i < slot_count; ++i) {
        int rc = validate_slot(&slots[i]);
        if (rc != BM_OK) {
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

    return BM_OK;
}

int bm_hrt_start(void) {
    if (g_slot_count == 0u) {
        return BM_ERR_NOT_INIT;
    }
    if (g_started) {
        return BM_ERR_ALREADY;
    }

    bm_hal_timer_set_callback(hrt_timer_isr);
    if (bm_hal_timer_init(hrt_tick_hz()) != 0) {
        bm_hal_timer_set_callback(NULL);
        return BM_ERR_INVALID;
    }

    g_started = 1;
    return BM_OK;
}

void bm_hrt_stop(void) {
    if (!g_started) {
        return;
    }
    bm_hal_timer_stop();
    bm_hal_timer_set_callback(NULL);
    g_started = 0;
}

void bm_hrt_reset(void) {
    bm_hrt_stop();
    memset(g_slots, 0, sizeof(g_slots));
    g_slot_count = 0u;
}
