#ifndef BM_WDG_H
#define BM_WDG_H

#include "bm_core.h"

#ifndef BM_CONFIG_MAX_WDG_MODULES
#define BM_CONFIG_MAX_WDG_MODULES 4
#endif

int bm_wdg_register(const char *name);
void bm_wdg_feed(void);
void bm_wdg_feed_module(const char *name);

#endif
