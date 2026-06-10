#include "bm_hal_adc.h"

#define BM_SIM_ADC_RANKS      4u
#define BM_SIM_ADC_INSTANCES  2u

struct bm_hal_adc {
    uint32_t id;
};

typedef struct {
    uint16_t waveform[BM_SIM_ADC_RANKS];
    bm_hal_hrt_binding_t complete_binding;
} adc_sim_state_t;

static adc_sim_state_t g_adc_state[BM_SIM_ADC_INSTANCES];

static adc_sim_state_t *adc_state_for(const bm_hal_adc_t *adc) {
    if (!adc || adc->id >= BM_SIM_ADC_INSTANCES) {
        return NULL;
    }
    return &g_adc_state[adc->id];
}

const bm_hal_adc_t BM_HAL_ADC_SIM0 = { 0u };
const bm_hal_adc_t BM_HAL_ADC_SIM1 = { 1u };

void bm_hal_adc_sim_set_rank(const bm_hal_adc_t *adc, uint32_t rank,
                             uint16_t value) {
    adc_sim_state_t *state = adc_state_for(adc);
    if (!state || rank >= BM_SIM_ADC_RANKS) {
        return;
    }
    state->waveform[rank] = value;
}

int bm_hal_adc_read_injected(const bm_hal_adc_t *adc,
                             uint32_t rank, uint16_t *value) {
    adc_sim_state_t *state = adc_state_for(adc);
    if (!state || !value || rank >= BM_SIM_ADC_RANKS) {
        return BM_ERR_INVALID;
    }
    *value = state->waveform[rank];
    return BM_OK;
}

int bm_hal_adc_bind_complete(const bm_hal_adc_t *adc,
                             const bm_hal_hrt_binding_t *binding) {
    adc_sim_state_t *state = adc_state_for(adc);
    if (!state) {
        return BM_ERR_INVALID;
    }
    if (!binding) {
        state->complete_binding.callback = NULL;
        state->complete_binding.context = NULL;
        return BM_OK;
    }
    state->complete_binding = *binding;
    return BM_OK;
}

void bm_hal_adc_sim_fire_complete(const bm_hal_adc_t *adc) {
    adc_sim_state_t *state = adc_state_for(adc);
    if (!state || !state->complete_binding.callback) {
        return;
    }
    state->complete_binding.callback(state->complete_binding.context);
}
