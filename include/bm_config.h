#ifndef BM_CONFIG_H
#define BM_CONFIG_H

/* Core event system */
#define BM_CONFIG_MAX_EVENT_TYPES           16
#define BM_CONFIG_MAX_EVENT_SUBSCRIBERS     32
#define BM_CONFIG_EVENT_QUEUE_SIZE          16
#define BM_CONFIG_EVENT_PRIORITIES          4
#define BM_CONFIG_EVENT_INLINE_DATA_SIZE     8

/* Optional components */
#define BM_CONFIG_MAX_MODULES                8
#define BM_CONFIG_SHELL_BUF_SIZE            64
#define BM_CONFIG_SHELL_MAX_ARGS             4
#define BM_CONFIG_SHELL_MAX_CMDS             8
#define BM_CONFIG_MAX_WDG_MODULES            4

/* Header-only ultra profile */
#define BM_CONFIG_ULTRA_MAX_EVENT_TYPES      8
#define BM_CONFIG_ULTRA_QUEUE_DEPTH          8
#define BM_CONFIG_ULTRA_MAX_EVENT_DATA_SIZE  8

/* Hybrid domain (optional) */
#define BM_CONFIG_ENABLE_PRIORITY_MASK       0
#define BM_CONFIG_HRT_PRIORITY_THRESHOLD     4
#define BM_CONFIG_HRT_TICK_US                100
#define BM_CONFIG_HRT_MAX_SLOTS              16
#define BM_CONFIG_TICKER_MAX_SLOTS           8
#define BM_CONFIG_MAX_CTRL_SLOTS             32
#define BM_CONFIG_MAX_CTRL_INSTANCES         16
#define BM_CONFIG_MAX_RESOURCE_CLAIMS        64

#endif /* BM_CONFIG_H */
