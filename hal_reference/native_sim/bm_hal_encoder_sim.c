#include "bm_hal_encoder.h"

struct bm_hal_encoder {
    uint32_t id;
};

static int32_t g_encoder_count[2];

const bm_hal_encoder_t BM_HAL_ENC_SIM0 = { 0u };

void bm_hal_encoder_sim_set_count(const bm_hal_encoder_t *enc, int32_t value) {
    if (!enc || enc->id >= 2u) {
        return;
    }
    g_encoder_count[enc->id] = value;
}

int bm_hal_encoder_read(const bm_hal_encoder_t *enc, int32_t *value) {
    if (!enc || !value || enc->id >= 2u) {
        return BM_ERR_INVALID;
    }
    *value = g_encoder_count[enc->id];
    return BM_OK;
}
