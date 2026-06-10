#include "bm_hal_comp.h"

struct bm_hal_comp {
    uint32_t id;
};

typedef struct {
    uint16_t threshold;
    int latched;
} comp_sim_state_t;

static comp_sim_state_t g_comp_state[2];

const bm_hal_comp_t BM_HAL_COMP_SIM0 = { 0u };

void bm_hal_comp_sim_set_threshold(const bm_hal_comp_t *comp, uint16_t threshold) {
    if (!comp || comp->id >= 2u) {
        return;
    }
    g_comp_state[comp->id].threshold = threshold;
}

void bm_hal_comp_sim_trip(const bm_hal_comp_t *comp, uint16_t sample) {
    if (!comp || comp->id >= 2u) {
        return;
    }
    if (sample >= g_comp_state[comp->id].threshold) {
        g_comp_state[comp->id].latched = 1;
    }
}

int bm_hal_comp_clear_latch(const bm_hal_comp_t *comp) {
    if (!comp || comp->id >= 2u) {
        return BM_ERR_INVALID;
    }
    g_comp_state[comp->id].latched = 0;
    return BM_OK;
}

int bm_hal_comp_sim_is_latched(const bm_hal_comp_t *comp) {
    if (!comp || comp->id >= 2u) {
        return 0;
    }
    return g_comp_state[comp->id].latched;
}
