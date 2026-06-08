#include "bm_wdg.h"
#include "bm_hal_wdg.h"
#include "bm_hal_timer.h"
#include <string.h>

typedef struct {
    const char *name;
    uint32_t    last_feed_ticks;
} bm_wdg_module_t;

static bm_wdg_module_t _wdg_modules[BM_CONFIG_MAX_WDG_MODULES];
static uint32_t        _wdg_module_count = 0;

int bm_wdg_register(const char *name) {
    if (_wdg_module_count >= BM_CONFIG_MAX_WDG_MODULES) {
        return BM_ERR_NO_MEM;
    }
    _wdg_modules[_wdg_module_count].name = name;
    _wdg_modules[_wdg_module_count].last_feed_ticks = 0;
    _wdg_module_count++;
    return BM_OK;
}

void bm_wdg_feed_module(const char *name) {
    for (uint32_t i = 0; i < _wdg_module_count; i++) {
        if (_wdg_modules[i].name && strcmp(_wdg_modules[i].name, name) == 0) {
            _wdg_modules[i].last_feed_ticks = bm_hal_timer_get_ticks();
            return;
        }
    }
}

void bm_wdg_feed(void) {
    if (_wdg_module_count == 0) {
        bm_hal_wdg_feed();
        return;
    }
    for (uint32_t i = 0; i < _wdg_module_count; i++) {
        if (_wdg_modules[i].last_feed_ticks == 0) {
            return;
        }
    }
    bm_hal_wdg_feed();
}
