#ifndef BM_TYPES_H
#define BM_TYPES_H

#include <bm_config.h>

#include <stdint.h>

#define BM_OK                0
#define BM_ERR_NO_MEM       -1
#define BM_ERR_NOT_FOUND    -2
#define BM_ERR_WOULD_BLOCK  -3
#define BM_ERR_INVALID      -4
#define BM_ERR_BUSY         -5
#define BM_ERR_OVERFLOW     -6
#define BM_ERR_NOT_INIT     -7
#define BM_ERR_ALREADY      -8

typedef uint32_t bm_irq_state_t;
typedef volatile uint32_t bm_atomic_t;

#endif /* BM_TYPES_H */
