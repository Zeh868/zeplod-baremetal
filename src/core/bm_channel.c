/**
 * @file bm_channel.c
 * @brief 轻量级 SPSC 数据通道实现
 *
 * 单生产者单消费者环形缓冲，临界区保护索引更新。
 * @author zeh (china_qzh@163.com)
 * @version 1.1
 * @date 2026-06-10
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 * 2026-06-10       1.1            zeh            SIL-2 防御性校验与查询 API 临界区
 *
 */
#include "bm_channel.h"
#include "bm_critical_wrap.h"
#include "bm_log.h"
#include "bm_safety.h"

#include <string.h>

/**
 * @brief 校验通道控制块与缓冲区配置（fail-stop，不修改控制块）
 */
static int channel_validate_config(const bm_channel_t *ch) {
    if (!ch || !ch->buf || ch->elem_size == 0u || ch->capacity < 2U) {
        return BM_ERR_INVALID;
    }
    {
        uint32_t bytes = 0u;
        if (bm_mul_u32_overflow(ch->elem_size, ch->capacity, &bytes)) {
            return BM_ERR_INVALID;
        }
    }
    return BM_OK;
}

/**
 * @brief 校验读写索引在合法范围内（fail-stop）
 */
static int channel_validate_indices(const bm_channel_t *ch) {
    if (!bm_index_in_range_u32(ch->read_idx, ch->capacity) ||
        !bm_index_in_range_u32(ch->write_idx, ch->capacity)) {
        return BM_ERR_INVALID;
    }
    return BM_OK;
}

/**
 * @brief 计算槽位字节偏移，溢出则失败
 */
static int channel_slot_offset(const bm_channel_t *ch, uint32_t index,
                               uint32_t *offset_out) {
    uint32_t offset = 0u;

    if (!bm_index_in_range_u32(index, ch->capacity)) {
        return BM_ERR_INVALID;
    }
    if (bm_mul_u32_overflow(index, ch->elem_size, &offset)) {
        return BM_ERR_INVALID;
    }
    *offset_out = offset;
    return BM_OK;
}

/**
 * @brief 在临界区内计算待读取元素数量
 */
static uint32_t channel_count_locked(const bm_channel_t *ch) {
    uint32_t w = ch->write_idx;
    uint32_t r = ch->read_idx;
    return (w >= r) ? (w - r) : (ch->capacity - r + w);
}

/** 逻辑占用量上界：保留槽区分满/空，最大可存 capacity-1 */
static int channel_logical_count_valid(const bm_channel_t *ch, uint32_t count) {
    return count < ch->capacity ? BM_OK : BM_ERR_INVALID;
}

/**
 * @brief 重置 SPSC 通道读写索引
 *
 * @param ch 通道描述符指针
 */
void bm_channel_reset(bm_channel_t *ch) {
    if (!ch) {
        return;
    }
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
    int rc = channel_validate_config(ch);

    if (rc != BM_OK || !data) {
        BM_LOGE("channel", "send invalid channel");
        return BM_ERR_INVALID;
    }
    if (channel_validate_indices(ch) != BM_OK) {
        BM_LOGE("channel", "send corrupt indices");
        return BM_ERR_INVALID;
    }

    bm_irq_state_t s = BM_CRITICAL_ENTER();
    if (channel_validate_indices(ch) != BM_OK) {
        BM_CRITICAL_EXIT(s);
        return BM_ERR_INVALID;
    }
    if (channel_logical_count_valid(ch, channel_count_locked(ch)) != BM_OK) {
        BM_CRITICAL_EXIT(s);
        BM_LOGE("channel", "send corrupt occupancy");
        return BM_ERR_INVALID;
    }

    uint32_t next = (ch->write_idx + 1u) % ch->capacity;
    if (next == ch->read_idx) {
        BM_CRITICAL_EXIT(s);
        BM_LOGW("channel", "send overflow");
        return BM_ERR_OVERFLOW;
    }

    {
        uint32_t offset = 0u;
        if (channel_slot_offset(ch, ch->write_idx, &offset) != BM_OK) {
            BM_CRITICAL_EXIT(s);
            return BM_ERR_INVALID;
        }
        memcpy(ch->buf + offset, data, ch->elem_size);
    }
    ch->write_idx = next;
    BM_CRITICAL_EXIT(s);
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
    int rc = channel_validate_config(ch);

    if (rc != BM_OK || !data) {
        BM_LOGE("channel", "recv invalid channel");
        return BM_ERR_INVALID;
    }
    if (channel_validate_indices(ch) != BM_OK) {
        BM_LOGE("channel", "recv corrupt indices");
        return BM_ERR_INVALID;
    }

    bm_irq_state_t s = BM_CRITICAL_ENTER();
    if (channel_validate_indices(ch) != BM_OK) {
        BM_CRITICAL_EXIT(s);
        return BM_ERR_INVALID;
    }
    if (ch->read_idx == ch->write_idx) {
        BM_CRITICAL_EXIT(s);
        return BM_ERR_WOULD_BLOCK;
    }
    if (channel_logical_count_valid(ch, channel_count_locked(ch)) != BM_OK) {
        BM_CRITICAL_EXIT(s);
        BM_LOGE("channel", "recv corrupt occupancy");
        return BM_ERR_INVALID;
    }

    {
        uint32_t offset = 0u;
        if (channel_slot_offset(ch, ch->read_idx, &offset) != BM_OK) {
            BM_CRITICAL_EXIT(s);
            return BM_ERR_INVALID;
        }
        memcpy(data, ch->buf + offset, ch->elem_size);
    }
    ch->read_idx = (ch->read_idx + 1u) % ch->capacity;
    BM_CRITICAL_EXIT(s);
    return BM_OK;
}

/**
 * @brief 查询通道中待读取的元素数量
 *
 * @param ch 通道描述符指针
 * @return 元素数量；无效通道返回 0
 */
uint32_t bm_channel_count(const bm_channel_t *ch) {
    if (channel_validate_config(ch) != BM_OK) {
        return 0u;
    }
    if (channel_validate_indices(ch) != BM_OK) {
        return 0u;
    }
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    uint32_t count = channel_count_locked(ch);

    if (channel_logical_count_valid(ch, count) != BM_OK) {
        BM_CRITICAL_EXIT(s);
        return 0u;
    }
    BM_CRITICAL_EXIT(s);
    return count;
}

/**
 * @brief 判断通道是否为空
 *
 * @param ch 通道描述符指针
 * @return true 为空或 ch 无效；false 非空
 */
bool bm_channel_is_empty(const bm_channel_t *ch) {
    return bm_channel_count(ch) == 0u;
}

/**
 * @brief 判断通道是否已满
 *
 * @param ch 通道描述符指针
 * @return true 已满或无效；false 仍可写入
 */
bool bm_channel_is_full(const bm_channel_t *ch) {
    if (channel_validate_config(ch) != BM_OK) {
        return true;
    }
    if (channel_validate_indices(ch) != BM_OK) {
        return true;
    }

    bm_irq_state_t s = BM_CRITICAL_ENTER();
    uint32_t count = channel_count_locked(ch);
    uint32_t next = (ch->write_idx + 1u) % ch->capacity;
    bool full = (next == ch->read_idx);

    if (channel_logical_count_valid(ch, count) != BM_OK) {
        BM_CRITICAL_EXIT(s);
        return true;
    }
    BM_CRITICAL_EXIT(s);
    return full;
}
