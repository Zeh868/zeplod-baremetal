#ifndef BM_HRT_H
#define BM_HRT_H

#include "bm_types.h"

typedef void (*bm_hrt_callback_t)(void *context);

typedef enum {
    BM_HRT_TRIGGER_TIMER,
    BM_HRT_TRIGGER_PWM_UPDATE,
    BM_HRT_TRIGGER_ADC_COMPLETE
} bm_hrt_trigger_t;

typedef struct {
    uint32_t period_us;
    bm_hrt_trigger_t trigger;
    bm_hrt_callback_t callback;
    void *context;
    const char *name;
} bm_hrt_slot_t;

int bm_hrt_init(const bm_hrt_slot_t *slots, uint32_t slot_count);
int bm_hrt_start(void);
void bm_hrt_stop(void);
void bm_hrt_reset(void);

void bm_hrt_deadline_missed_hook(const bm_hrt_slot_t *slot);

#endif /* BM_HRT_H */
