#include "bm_module.h"
#include <string.h>

extern const bm_module_t _bm_module_table[];
extern const uint32_t    _bm_module_count;

static bm_module_t _modules[BM_CONFIG_MAX_MODULES];
static uint32_t    _module_count = 0;

static void _sort_modules(void) {
    for (uint32_t i = 0; i < _module_count; i++) {
        for (uint32_t j = i + 1; j < _module_count; j++) {
            if (_modules[i].priority > _modules[j].priority) {
                bm_module_t tmp = _modules[i];
                _modules[i] = _modules[j];
                _modules[j] = tmp;
            }
        }
    }
}

int bm_module_init_all(void) {
    _module_count = (_bm_module_count < BM_CONFIG_MAX_MODULES)
                        ? _bm_module_count : BM_CONFIG_MAX_MODULES;
    memcpy(_modules, _bm_module_table, _module_count * sizeof(bm_module_t));
    _sort_modules();

    int rc = BM_OK;
    for (uint32_t i = 0; i < _module_count; i++) {
        if (_modules[i].init) {
            int r = _modules[i].init();
            if (r == BM_OK) {
                _modules[i].state = BM_MODULE_STATE_INITED;
            } else {
                rc = r;
            }
        }
    }
    return rc;
}

int bm_module_start_all(void) {
    int rc = BM_OK;
    for (uint32_t i = 0; i < _module_count; i++) {
        if (_modules[i].state == BM_MODULE_STATE_INITED && _modules[i].start) {
            int r = _modules[i].start();
            if (r == BM_OK) {
                _modules[i].state = BM_MODULE_STATE_STARTED;
            } else {
                rc = r;
            }
        }
    }
    return rc;
}

int bm_module_stop_all(void) {
    int rc = BM_OK;
    for (int i = (int)_module_count - 1; i >= 0; i--) {
        if (_modules[i].state == BM_MODULE_STATE_STARTED && _modules[i].stop) {
            int r = _modules[i].stop();
            if (r == BM_OK) {
                _modules[i].state = BM_MODULE_STATE_STOPPED;
            } else {
                rc = r;
            }
        }
    }
    return rc;
}

int bm_module_deinit_all(void) {
    for (int i = (int)_module_count - 1; i >= 0; i--) {
        if (_modules[i].deinit) {
            _modules[i].deinit();
        }
        _modules[i].state = BM_MODULE_STATE_UNINIT;
    }
    return BM_OK;
}
