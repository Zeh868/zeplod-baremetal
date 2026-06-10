/**
 * @file bm_channel.h
 * @brief 轻量级 SPSC 数据通道
 *
 * 单生产者单消费者环形缓冲区，支持 ISR 安全的非阻塞收发。
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 *
 */
#ifndef BM_CHANNEL_H
#define BM_CHANNEL_H

#include "bm_types.h"

#include <stdbool.h>
#include <stdint.h>

#ifndef BM_CONFIG_CHANNEL_MAX_CHANNELS
#define BM_CONFIG_CHANNEL_MAX_CHANNELS 4
#endif

/** 环形通道控制块 */
typedef struct {
    uint8_t *buf;
    uint32_t elem_size;
    uint32_t capacity;
    volatile uint32_t write_idx;
    volatile uint32_t read_idx;
} bm_channel_t;

/** 静态定义通道实例及 backing buffer
 *  示例：BM_CHANNEL_DEFINE(my_chan, sensor_t, 8); */
#define BM_CHANNEL_DEFINE(name, type, depth) \
    static uint8_t _bm_channel_buf_##name[(depth) * sizeof(type)] = {0}; \
    static bm_channel_t name = { \
        .buf = _bm_channel_buf_##name, \
        .elem_size = sizeof(type), \
        .capacity = (depth), \
        .write_idx = 0, \
        .read_idx = 0 \
    }

/**
 * @brief 重置通道为空
 *
 * @param ch 通道控制块指针
 */
void bm_channel_reset(bm_channel_t *ch);

/**
 * @brief 非阻塞发送单个元素（ISR 安全）
 *
 * @param ch 通道控制块指针
 * @param data 待发送元素数据指针
 * @return BM_OK 成功；BM_ERR_OVERFLOW 通道已满
 */
int bm_channel_send(bm_channel_t *ch, const void *data);

/**
 * @brief 非阻塞接收单个元素（ISR 安全）
 *
 * @param ch 通道控制块指针
 * @param data 接收缓冲区指针
 * @return BM_OK 成功；BM_ERR_WOULD_BLOCK 通道为空
 */
int bm_channel_recv(bm_channel_t *ch, void *data);

/**
 * @brief 查询当前队列中的元素数量
 *
 * @param ch 通道控制块指针
 * @return 当前元素个数
 */
uint32_t bm_channel_count(const bm_channel_t *ch);

/**
 * @brief 查询通道总容量
 *
 * @param ch 通道控制块指针
 * @return 通道容量；ch 为 NULL 时返回 0
 */
static inline uint32_t bm_channel_capacity(const bm_channel_t *ch) {
    return ch ? ch->capacity : 0;
}

/**
 * @brief 判断通道是否为空
 *
 * @param ch 通道控制块指针
 * @return true 为空或 ch 为 NULL；false 非空
 */
static inline bool bm_channel_is_empty(const bm_channel_t *ch) {
    return ch ? (ch->write_idx == ch->read_idx) : true;
}

/**
 * @brief 判断通道是否已满
 *
 * @param ch 通道控制块指针
 * @return true 已满、无效或容量不足；false 仍可写入
 */
static inline bool bm_channel_is_full(const bm_channel_t *ch) {
    if (!ch || ch->capacity < 2U) return true;
    uint32_t next = (ch->write_idx + 1) % ch->capacity;
    return next == ch->read_idx;
}

#endif /* BM_CHANNEL_H */
