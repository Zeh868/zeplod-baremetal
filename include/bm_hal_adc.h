#ifndef BM_HAL_ADC_H
#define BM_HAL_ADC_H

#include "bm_hal_pwm.h"
#include "bm_types.h"

typedef struct bm_hal_adc bm_hal_adc_t;

int bm_hal_adc_read_injected(const bm_hal_adc_t *adc,
                             uint32_t rank, uint16_t *value);
int bm_hal_adc_bind_complete(const bm_hal_adc_t *adc,
                             const bm_hal_hrt_binding_t *binding);

#endif /* BM_HAL_ADC_H */
