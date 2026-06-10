#ifndef BM_HAL_ADC_SIM_H
#define BM_HAL_ADC_SIM_H

#include "bm_hal_adc.h"

extern const bm_hal_adc_t BM_HAL_ADC_SIM0;
extern const bm_hal_adc_t BM_HAL_ADC_SIM1;

void bm_hal_adc_sim_set_rank(const bm_hal_adc_t *adc, uint32_t rank,
                             uint16_t value);
void bm_hal_adc_sim_fire_complete(const bm_hal_adc_t *adc);

#endif /* BM_HAL_ADC_SIM_H */
