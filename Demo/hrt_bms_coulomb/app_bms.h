#ifndef APP_BMS_H
#define APP_BMS_H

#include "bm_ctrl_inst.h"

#define TICKER_INTEGRATE 1u

void app_cell_integrate_step(const bm_ctrl_inst_t *instance);

extern const bm_ctrl_inst_t g_cell_0;

#endif
