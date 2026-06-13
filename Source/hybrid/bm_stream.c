/**
 * @file bm_stream.c
 * @brief 静态零拷贝块流实现
 *
 * 实现 bm_stream 生产者/消费者 API、过载策略（drop-newest/drop-oldest）与
 * 运行统计计数。单生产者单消费者，零堆、零拷贝交接。
 *
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-12
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-12       1.0            zeh            正式发布
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */
#include "bm_stream.h"
#include "hal/bm_hal_critical.h"

#include <string.h>

#define BM_CRITICAL_ENTER() bm_hal_critical_enter()
#define BM_CRITICAL_EXIT(state) bm_hal_critical_exit(state)

static int stream_valid(const bm_stream_t *stream) {
    return (stream != NULL &&
            stream->initialized != 0 &&
            stream->blocks != NULL &&
            stream->block_count >= 2u &&
            stream->block_count <= stream->block_capacity &&
            stream->block_capacity <= BM_CONFIG_STREAM_MAX_BLOCKS);
}

static bm_block_t *find_block(bm_stream_t *stream, bm_block_t *block) {
    uint32_t i;

    for (i = 0u; i < stream->block_count; ++i) {
        if (&stream->blocks[i] == block) {
            return &stream->blocks[i];
        }
    }
    return NULL;
}

static bm_block_t *find_state_locked(bm_stream_t *stream,
                                     bm_block_state_t state) {
    uint32_t i;

    for (i = 0u; i < stream->block_count; ++i) {
        if (stream->blocks[i].state == state) {
            return &stream->blocks[i];
        }
    }
    return NULL;
}

static bm_block_t *find_oldest_ready_locked(bm_stream_t *stream) {
    bm_block_t *oldest = NULL;
    uint32_t i;

    for (i = 0u; i < stream->block_count; ++i) {
        bm_block_t *b = &stream->blocks[i];
        if (b->state == BM_BLOCK_STATE_READY &&
            (oldest == NULL ||
             (int32_t)(b->sequence - oldest->sequence) < 0)) {
            oldest = b;
        }
    }
    return oldest;
}

static void drop_oldest_ready_locked(bm_stream_t *stream) {
    bm_block_t *oldest = find_oldest_ready_locked(stream);

    if (oldest != NULL) {
        oldest->state = BM_BLOCK_STATE_FREE;
        oldest->valid_bytes = 0u;
        stream->stats.drop++;
    }
}

int bm_stream_init(bm_stream_t *stream,
                   void *payloads,
                   uint32_t block_count,
                   uint32_t block_bytes) {
    uint32_t i;
    uint32_t descriptor_capacity;
    uint8_t *base;

    if (!stream || !stream->blocks || !payloads || block_count < 2u ||
        block_count > BM_CONFIG_STREAM_MAX_BLOCKS || block_bytes == 0u ||
        block_bytes > (UINT32_MAX / block_count)) {
        return BM_ERR_INVALID;
    }

    descriptor_capacity = stream->block_capacity != 0u
                              ? stream->block_capacity
                              : stream->block_count;
    if (descriptor_capacity < block_count ||
        descriptor_capacity > BM_CONFIG_STREAM_MAX_BLOCKS) {
        return BM_ERR_INVALID;
    }

    base = (uint8_t *)payloads;
    memset(stream->blocks, 0, sizeof(bm_block_t) * block_count);
    for (i = 0u; i < block_count; ++i) {
        stream->blocks[i].data = (void *)(base + (block_bytes * i));
        stream->blocks[i].capacity_bytes = block_bytes;
        stream->blocks[i].state = BM_BLOCK_STATE_FREE;
    }
    stream->block_count = block_count;
    stream->block_capacity = descriptor_capacity;
    stream->next_sequence = 0u;
    stream->initialized = 1;
    return BM_OK;
}

void bm_stream_reset(bm_stream_t *stream) {
    bm_irq_state_t irq;
    uint32_t i;

    if (!stream || !stream->blocks ||
        stream->block_count > stream->block_capacity ||
        stream->block_capacity > BM_CONFIG_STREAM_MAX_BLOCKS) {
        return;
    }
    irq = BM_CRITICAL_ENTER();
    for (i = 0u; i < stream->block_count; ++i) {
        stream->blocks[i].valid_bytes = 0u;
        stream->blocks[i].sequence = 0u;
        stream->blocks[i].state = BM_BLOCK_STATE_FREE;
    }
    memset(&stream->stats, 0, sizeof(stream->stats));
    stream->next_sequence = 0u;
    BM_CRITICAL_EXIT(irq);
}

void bm_stream_set_policy(bm_stream_t *stream, bm_stream_policy_t policy) {
    bm_irq_state_t irq;

    if (!stream ||
        (policy != BM_STREAM_POLICY_DROP_NEWEST &&
         policy != BM_STREAM_POLICY_DROP_OLDEST)) {
        return;
    }
    irq = BM_CRITICAL_ENTER();
    stream->policy = policy;
    BM_CRITICAL_EXIT(irq);
}

void bm_stream_set_ready_handler(bm_stream_t *stream,
                                 bm_stream_ready_fn_t handler,
                                 void *context) {
    bm_irq_state_t irq;

    if (!stream) {
        return;
    }
    irq = BM_CRITICAL_ENTER();
    stream->on_ready = handler;
    stream->on_ready_context = context;
    BM_CRITICAL_EXIT(irq);
}

const bm_stream_stats_t *bm_stream_stats(const bm_stream_t *stream) {
    if (!stream_valid(stream)) {
        return NULL;
    }
    return &stream->stats;
}

uint32_t bm_stream_ready_count(const bm_stream_t *stream) {
    bm_irq_state_t irq;
    uint32_t count = 0u;
    uint32_t i;

    if (!stream_valid(stream)) {
        return 0u;
    }

    irq = BM_CRITICAL_ENTER();
    for (i = 0u; i < stream->block_count; ++i) {
        if (stream->blocks[i].state == BM_BLOCK_STATE_READY) {
            count++;
        }
    }
    BM_CRITICAL_EXIT(irq);
    return count;
}

int bm_stream_producer_acquire(bm_stream_t *stream, bm_block_t **block) {
    bm_irq_state_t irq;
    bm_block_t *slot;

    if (!stream_valid(stream) || !block) {
        return BM_ERR_INVALID;
    }

    irq = BM_CRITICAL_ENTER();
    slot = find_state_locked(stream, BM_BLOCK_STATE_FREE);
    if (!slot) {
        stream->stats.overrun++;
        if (stream->policy == BM_STREAM_POLICY_DROP_OLDEST) {
            drop_oldest_ready_locked(stream);
            slot = find_state_locked(stream, BM_BLOCK_STATE_FREE);
        }
        if (!slot) {
            stream->stats.drop++;
            BM_CRITICAL_EXIT(irq);
            return BM_ERR_OVERFLOW;
        }
    }

    slot->valid_bytes = 0u;
    slot->state = BM_BLOCK_STATE_DMA_OWNED;
    *block = slot;
    BM_CRITICAL_EXIT(irq);
    return BM_OK;
}

int bm_stream_producer_commit(bm_stream_t *stream,
                              bm_block_t *block,
                              uint32_t valid_bytes,
                              const bm_timestamp_t *timestamp) {
    bm_irq_state_t irq;
    bm_block_t *slot;
    bm_stream_ready_fn_t handler;
    void *handler_context;

    if (!stream_valid(stream) || !block) {
        return BM_ERR_INVALID;
    }

    irq = BM_CRITICAL_ENTER();
    slot = find_block(stream, block);
    if (!slot || slot->state != BM_BLOCK_STATE_DMA_OWNED) {
        stream->stats.corrupt++;
        BM_CRITICAL_EXIT(irq);
        return BM_ERR_INVALID;
    }
    if (valid_bytes > slot->capacity_bytes) {
        stream->stats.corrupt++;
        BM_CRITICAL_EXIT(irq);
        return BM_ERR_INVALID;
    }

    slot->valid_bytes = valid_bytes;
    slot->sequence = ++stream->next_sequence;
    if (timestamp) {
        slot->timestamp = *timestamp;
    }
    slot->state = BM_BLOCK_STATE_READY;
    handler = stream->on_ready;
    handler_context = stream->on_ready_context;
    BM_CRITICAL_EXIT(irq);

    if (handler != NULL) {
        handler(stream, slot, handler_context);
    }
    return BM_OK;
}

int bm_stream_consumer_acquire(bm_stream_t *stream, bm_block_t **block) {
    bm_irq_state_t irq;
    bm_block_t *slot;

    if (!stream_valid(stream) || !block) {
        return BM_ERR_INVALID;
    }

    irq = BM_CRITICAL_ENTER();
    slot = find_oldest_ready_locked(stream);
    if (!slot) {
        stream->stats.underrun++;
        BM_CRITICAL_EXIT(irq);
        return BM_ERR_WOULD_BLOCK;
    }

    slot->state = BM_BLOCK_STATE_PROCESSING;
    *block = slot;
    BM_CRITICAL_EXIT(irq);
    return BM_OK;
}

int bm_stream_consumer_release(bm_stream_t *stream, bm_block_t *block) {
    bm_irq_state_t irq;
    bm_block_t *slot;

    if (!stream_valid(stream) || !block) {
        return BM_ERR_INVALID;
    }

    irq = BM_CRITICAL_ENTER();
    slot = find_block(stream, block);
    if (!slot ||
        (slot->state != BM_BLOCK_STATE_PROCESSING &&
         slot->state != BM_BLOCK_STATE_OUTPUT_READY)) {
        stream->stats.corrupt++;
        BM_CRITICAL_EXIT(irq);
        return BM_ERR_INVALID;
    }

    slot->valid_bytes = 0u;
    slot->state = BM_BLOCK_STATE_FREE;
    BM_CRITICAL_EXIT(irq);
    return BM_OK;
}

int bm_stream_output_acquire(bm_stream_t *stream, bm_block_t **block) {
    bm_irq_state_t irq;
    bm_block_t *slot;

    if (!stream_valid(stream) || !block) {
        return BM_ERR_INVALID;
    }

    irq = BM_CRITICAL_ENTER();
    slot = find_state_locked(stream, BM_BLOCK_STATE_FREE);
    if (!slot) {
        stream->stats.overrun++;
        BM_CRITICAL_EXIT(irq);
        return BM_ERR_OVERFLOW;
    }

    slot->valid_bytes = 0u;
    slot->state = BM_BLOCK_STATE_DMA_OWNED;
    *block = slot;
    BM_CRITICAL_EXIT(irq);
    return BM_OK;
}

int bm_stream_output_commit(bm_stream_t *stream,
                            bm_block_t *block,
                            uint32_t valid_bytes,
                            const bm_timestamp_t *timestamp) {
    bm_irq_state_t irq;
    bm_block_t *slot;

    if (!stream_valid(stream) || !block) {
        return BM_ERR_INVALID;
    }

    irq = BM_CRITICAL_ENTER();
    slot = find_block(stream, block);
    if (!slot || slot->state != BM_BLOCK_STATE_DMA_OWNED) {
        stream->stats.corrupt++;
        BM_CRITICAL_EXIT(irq);
        return BM_ERR_INVALID;
    }
    if (valid_bytes > slot->capacity_bytes) {
        stream->stats.corrupt++;
        BM_CRITICAL_EXIT(irq);
        return BM_ERR_INVALID;
    }

    slot->valid_bytes = valid_bytes;
    if (timestamp) {
        slot->timestamp = *timestamp;
    }
    slot->state = BM_BLOCK_STATE_OUTPUT_READY;
    BM_CRITICAL_EXIT(irq);
    return BM_OK;
}
