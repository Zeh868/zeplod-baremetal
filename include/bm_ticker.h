#ifndef BM_TICKER_H
#define BM_TICKER_H

#include "bm_event.h"
#include "bm_types.h"

typedef struct {
    uint32_t period_ms;
    bm_event_type_t event_type;
    bm_event_priority_t priority;
    const char *name;
} bm_ticker_slot_t;

int bm_ticker_init(const bm_ticker_slot_t *slots, uint32_t slot_count);
int bm_ticker_poll(void);
uint32_t bm_ticker_get_dropped(uint32_t slot_index);
void bm_ticker_reset(void);

#endif /* BM_TICKER_H */
