/**
 * @file bm_config.h
 * @brief multi_channel_bms 示例运行时容量配置
 * @author zeh
 * @version 1.0
 * @date 2026-06-10
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 *
 */
#ifndef BM_CONFIG_H
#define BM_CONFIG_H

#define BM_CONFIG_MAX_EVENT_TYPES           32
#define BM_CONFIG_MAX_EVENT_SUBSCRIBERS     64
#define BM_CONFIG_EVENT_QUEUE_SIZE          32
#define BM_CONFIG_EVENT_PRIORITIES          4
#define BM_CONFIG_EVENT_INLINE_DATA_SIZE     8

#define BM_CONFIG_HRT_TICK_US               1000
#define BM_CONFIG_HRT_MAX_SLOTS             16
#define BM_CONFIG_TICKER_MAX_SLOTS          32
#define BM_CONFIG_MAX_CTRL_SLOTS            32
#define BM_CONFIG_MAX_CTRL_INSTANCES        20
#define BM_CONFIG_MAX_RESOURCE_CLAIMS       64

#define BM_CONFIG_ENABLE_PRIORITY_MASK      0

#endif /* BM_CONFIG_H */
