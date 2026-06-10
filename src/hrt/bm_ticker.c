#include "bm_ticker.h"
#include "bm_hal_timer.h"

#include <string.h>

#ifndef BM_CONFIG_TICKER_MAX_SLOTS
#define BM_CONFIG_TICKER_MAX_SLOTS 8u
#endif

typedef struct {
    bm_ticker_slot_t pub;
    uint32_t period_ticks;
    uint32_t next_tick;
    uint32_t dropped;
} bm_ticker_runtime_slot_t;

static bm_ticker_runtime_slot_t g_slots[BM_CONFIG_TICKER_MAX_SLOTS];
static uint32_t g_slot_count;
static int g_initialized;

static uint32_t ms_to_ticks(uint32_t period_ms) {
    uint32_t freq = bm_hal_timer_get_freq();
    if (freq == 0u) {
        return 0u;
    }
    return (period_ms * freq) / 1000u;
}

int bm_ticker_init(const bm_ticker_slot_t *slots, uint32_t slot_count) {
    uint32_t i;
    uint32_t now;

    if (!slots && slot_count > 0u) {
        return BM_ERR_INVALID;
    }
    if (slot_count > BM_CONFIG_TICKER_MAX_SLOTS) {
        return BM_ERR_OVERFLOW;
    }

    for (i = 0u; i < slot_count; ++i) {
        if (slots[i].period_ms == 0u) {
            return BM_ERR_INVALID;
        }
    }

    memset(g_slots, 0, sizeof(g_slots));
    g_slot_count = slot_count;
    now = bm_hal_timer_get_ticks();

    for (i = 0u; i < slot_count; ++i) {
        g_slots[i].pub = slots[i];
        g_slots[i].period_ticks = ms_to_ticks(slots[i].period_ms);
        if (g_slots[i].period_ticks == 0u) {
            return BM_ERR_INVALID;
        }
        g_slots[i].next_tick = now + g_slots[i].period_ticks;
    }

    g_initialized = 1;
    return BM_OK;
}

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

        while ((int32_t)(now - slot->next_tick) >= 0) {
            int rc = bm_event_publish_copy(slot->pub.event_type,
                                           slot->pub.priority,
                                           NULL, 0u);
            if (rc != BM_OK) {
                slot->dropped++;
                break;
            }
            published++;
            slot->next_tick += slot->period_ticks;
            if ((uint32_t)(now - slot->next_tick) >= slot->period_ticks) {
                slot->next_tick = now + slot->period_ticks;
                break;
            }
        }
    }

    return published;
}

uint32_t bm_ticker_get_dropped(uint32_t slot_index) {
    if (slot_index >= g_slot_count) {
        return 0u;
    }
    return g_slots[slot_index].dropped;
}

void bm_ticker_reset(void) {
    memset(g_slots, 0, sizeof(g_slots));
    g_slot_count = 0u;
    g_initialized = 0;
}
