#ifndef BM_HAL_COMP_SIM_H
#define BM_HAL_COMP_SIM_H

#include "bm_hal_comp.h"

extern const bm_hal_comp_t BM_HAL_COMP_SIM0;

void bm_hal_comp_sim_set_threshold(const bm_hal_comp_t *comp, uint16_t threshold);
void bm_hal_comp_sim_trip(const bm_hal_comp_t *comp, uint16_t sample);
int bm_hal_comp_sim_is_latched(const bm_hal_comp_t *comp);

#endif /* BM_HAL_COMP_SIM_H */
