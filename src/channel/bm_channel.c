/**
 * @file bm_channel.c
 * @brief 轻量级 SPSC 数据通道实现
 *
 * 单生产者单消费者环形缓冲，临界区保护索引更新。
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
#include "bm_channel.h"
#include "bm_critical_wrap.h"
#include "bm_log.h"

#include <string.h>

/**
 * @brief 重置 SPSC 通道读写索引
 *
 * @param ch 通道描述符指针
 */
void bm_channel_reset(bm_channel_t *ch) {
    if (!ch) return;
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    ch->write_idx = 0;
    ch->read_idx = 0;
    BM_CRITICAL_EXIT(s);
    BM_LOGD("channel", "reset");
}

/**
 * @brief 向通道写入一个元素（生产者侧）
 *
 * @param ch 通道描述符指针
 * @param data 待发送元素数据指针
 * @return BM_OK 成功；BM_ERR_INVALID 参数无效；BM_ERR_OVERFLOW 通道已满
 */
int bm_channel_send(bm_channel_t *ch, const void *data) {
    if (!ch || !data || !ch->buf || ch->elem_size == 0 || ch->capacity < 2U) {
        BM_LOGE("channel", "send invalid channel");
        return BM_ERR_INVALID;
    }

    bm_irq_state_t s = BM_CRITICAL_ENTER();
    uint32_t next = (ch->write_idx + 1) % ch->capacity;
    if (next == ch->read_idx) {
        BM_CRITICAL_EXIT(s);
        BM_LOGW("channel", "send overflow");
        return BM_ERR_OVERFLOW;
    }

    uint8_t *dst = ch->buf + ch->write_idx * ch->elem_size;
    memcpy(dst, data, ch->elem_size);
    ch->write_idx = next;
    BM_CRITICAL_EXIT(s);
    BM_LOGT("channel", "send ok count=%u", (unsigned)bm_channel_count(ch));
    return BM_OK;
}

/**
 * @brief 从通道读取一个元素（消费者侧）
 *
 * @param ch 通道描述符指针
 * @param data 接收缓冲区指针
 * @return BM_OK 成功；BM_ERR_INVALID 参数无效；BM_ERR_WOULD_BLOCK 通道为空
 */
int bm_channel_recv(bm_channel_t *ch, void *data) {
    if (!ch || !data || !ch->buf || ch->elem_size == 0 || ch->capacity < 2U) {
        BM_LOGE("channel", "recv invalid channel");
        return BM_ERR_INVALID;
    }

    bm_irq_state_t s = BM_CRITICAL_ENTER();
    if (ch->read_idx == ch->write_idx) {
        BM_CRITICAL_EXIT(s);
        return BM_ERR_WOULD_BLOCK;
    }

    const uint8_t *src = ch->buf + ch->read_idx * ch->elem_size;
    memcpy(data, src, ch->elem_size);
    ch->read_idx = (ch->read_idx + 1) % ch->capacity;
    BM_CRITICAL_EXIT(s);
    BM_LOGT("channel", "recv ok");
    return BM_OK;
}

/**
 * @brief 查询通道中待读取的元素数量
 *
 * @param ch 通道描述符指针
 * @return 元素数量；无效通道返回 0
 */
uint32_t bm_channel_count(const bm_channel_t *ch) {
    if (!ch || ch->capacity < 2U) return 0;
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    uint32_t w = ch->write_idx;
    uint32_t r = ch->read_idx;
    BM_CRITICAL_EXIT(s);
    return (w >= r) ? (w - r) : (ch->capacity - r + w);
}
