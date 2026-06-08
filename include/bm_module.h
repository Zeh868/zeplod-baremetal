#ifndef BM_MODULE_H
#define BM_MODULE_H

#include "bm_core.h"

#ifndef BM_CONFIG_MAX_MODULES
#define BM_CONFIG_MAX_MODULES 8
#endif

typedef enum {
    BM_MODULE_STATE_UNINIT = 0,
    BM_MODULE_STATE_INITED,
    BM_MODULE_STATE_STARTED,
    BM_MODULE_STATE_STOPPED
} bm_module_state_t;

typedef struct {
    const char        *name;
    uint8_t            priority;
    bm_module_state_t  state;
    int (*init)(void);
    int (*start)(void);
    int (*stop)(void);
    int (*deinit)(void);
} bm_module_t;

/* Compile-time registration macro.
 * For SDCC/STM8 compatibility, we provide an explicit array version.
 * Users define: static const bm_module_t _bm_module_table[] = { ... };
 * and: const uint32_t _bm_module_count = N;
 */

int bm_module_init_all(void);
int bm_module_start_all(void);
int bm_module_stop_all(void);
int bm_module_deinit_all(void);

#endif /* BM_MODULE_H */
