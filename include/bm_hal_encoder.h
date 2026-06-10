#ifndef BM_HAL_ENCODER_H
#define BM_HAL_ENCODER_H

#include "bm_types.h"

typedef struct bm_hal_encoder bm_hal_encoder_t;

int bm_hal_encoder_read(const bm_hal_encoder_t *enc, int32_t *value);

#endif /* BM_HAL_ENCODER_H */
