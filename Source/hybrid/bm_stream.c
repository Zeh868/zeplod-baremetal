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
#include "bm_critical_wrap.h"

#include <string.h>

static int stream_valid(const bm_stream_t *stream) {
    return (stream != NULL &&
            stream->initialized != 0 &&
            stream->blocks != NULL &&
            stream->block_count >= 2u &&
            stream->block_count <= BM_CONFIG_STREAM_MAX_BLOCKS);
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

static bm_block_t *find_state(bm_stream_t *stream, bm_block_state_t state) {
    uint32_t i;

    for (i = 0u; i < stream->block_count; ++i) {
        if (stream->blocks[i].state == state) {
            return &stream->blocks[i];
        }
    }
    return NULL;
}

static void drop_oldest_ready(bm_stream_t *stream) {
    bm_block_t *oldest = NULL;
    uint32_t i;
    uint32_t min_seq = 0xFFFFFFFFu;

    for (i = 0u; i < stream->block_count; ++i) {
        bm_block_t *b = &stream->blocks[i];
        if (b->state == BM_BLOCK_STATE_READY && b->sequence <= min_seq) {
            min_seq = b->sequence;
            oldest = b;
        }
    }
    if (oldest != NULL) {
        oldest->state = BM_BLOCK_STATE_FREE;
        oldest->valid_bytes = 0u;
        stream->stats.drop++;
    }
}

static void notify_ready(bm_stream_t *stream, bm_block_t *block) {
    if (stream->on_ready != NULL) {
        stream->on_ready(stream, block, stream->on_ready_context);
    }
}

int bm_stream_init(bm_stream_t *stream,
                   void *payloads,
                   uint32_t block_count,
                   uint32_t block_bytes) {
    uint32_t i;
    uint8_t *base;

    if (!stream || !payloads || block_count < 2u ||
        block_count > BM_CONFIG_STREAM_MAX_BLOCKS || block_bytes == 0u) {
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
    stream->next_sequence = 0u;
    stream->initialized = 1;
    return BM_OK;
}

void bm_stream_reset(bm_stream_t *stream) {
    uint32_t i;

    if (!stream || !stream->blocks) {
        return;
    }
    for (i = 0u; i < stream->block_count; ++i) {
        stream->blocks[i].valid_bytes = 0u;
        stream->blocks[i].sequence = 0u;
        stream->blocks[i].state = BM_BLOCK_STATE_FREE;
    }
    memset(&stream->stats, 0, sizeof(stream->stats));
}

void bm_stream_set_policy(bm_stream_t *stream, bm_stream_policy_t policy) {
    if (stream) {
        stream->policy = policy;
    }
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

int bm_stream_producer_acquire(bm_stream_t *stream, bm_block_t **block) {
    bm_block_t *slot;

    if (!stream_valid(stream) || !block) {
        return BM_ERR_INVALID;
    }

    slot = find_state(stream, BM_BLOCK_STATE_FREE);
    if (!slot) {
        stream->stats.overrun++;
        if (stream->policy == BM_STREAM_POLICY_DROP_OLDEST) {
            drop_oldest_ready(stream);
            slot = find_state(stream, BM_BLOCK_STATE_FREE);
        }
        if (!slot) {
            stream->stats.drop++;
            return BM_ERR_OVERFLOW;
        }
    }

    slot->valid_bytes = 0u;
    slot->state = BM_BLOCK_STATE_DMA_OWNED;
    *block = slot;
    return BM_OK;
}

int bm_stream_producer_commit(bm_stream_t *stream,
                              bm_block_t *block,
                              uint32_t valid_bytes,
                              const bm_timestamp_t *timestamp) {
    bm_block_t *slot;

    if (!stream_valid(stream) || !block) {
        return BM_ERR_INVALID;
    }

    slot = find_block(stream, block);
    if (!slot || slot->state != BM_BLOCK_STATE_DMA_OWNED) {
        stream->stats.corrupt++;
        return BM_ERR_INVALID;
    }
    if (valid_bytes > slot->capacity_bytes) {
        stream->stats.corrupt++;
        return BM_ERR_INVALID;
    }

    slot->valid_bytes = valid_bytes;
    slot->sequence = ++stream->next_sequence;
    if (timestamp) {
        slot->timestamp = *timestamp;
    }
    slot->state = BM_BLOCK_STATE_READY;
    notify_ready(stream, slot);
    return BM_OK;
}

int bm_stream_consumer_acquire(bm_stream_t *stream, bm_block_t **block) {
    bm_block_t *slot;

    if (!stream_valid(stream) || !block) {
        return BM_ERR_INVALID;
    }

    slot = find_state(stream, BM_BLOCK_STATE_READY);
    if (!slot) {
        stream->stats.underrun++;
        return BM_ERR_WOULD_BLOCK;
    }

    slot->state = BM_BLOCK_STATE_PROCESSING;
    *block = slot;
    return BM_OK;
}

int bm_stream_consumer_release(bm_stream_t *stream, bm_block_t *block) {
    bm_block_t *slot;

    if (!stream_valid(stream) || !block) {
        return BM_ERR_INVALID;
    }

    slot = find_block(stream, block);
    if (!slot ||
        (slot->state != BM_BLOCK_STATE_PROCESSING &&
         slot->state != BM_BLOCK_STATE_OUTPUT_READY)) {
        stream->stats.corrupt++;
        return BM_ERR_INVALID;
    }

    slot->valid_bytes = 0u;
    slot->state = BM_BLOCK_STATE_FREE;
    return BM_OK;
}

int bm_stream_output_acquire(bm_stream_t *stream, bm_block_t **block) {
    bm_block_t *slot;

    if (!stream_valid(stream) || !block) {
        return BM_ERR_INVALID;
    }

    slot = find_state(stream, BM_BLOCK_STATE_FREE);
    if (!slot) {
        stream->stats.overrun++;
        return BM_ERR_OVERFLOW;
    }

    slot->state = BM_BLOCK_STATE_DMA_OWNED;
    *block = slot;
    return BM_OK;
}

int bm_stream_output_commit(bm_stream_t *stream,
                            bm_block_t *block,
                            uint32_t valid_bytes,
                            const bm_timestamp_t *timestamp) {
    bm_block_t *slot;

    if (!stream_valid(stream) || !block) {
        return BM_ERR_INVALID;
    }

    slot = find_block(stream, block);
    if (!slot || slot->state != BM_BLOCK_STATE_DMA_OWNED) {
        stream->stats.corrupt++;
        return BM_ERR_INVALID;
    }
    if (valid_bytes > slot->capacity_bytes) {
        stream->stats.corrupt++;
        return BM_ERR_INVALID;
    }

    slot->valid_bytes = valid_bytes;
    if (timestamp) {
        slot->timestamp = *timestamp;
    }
    slot->state = BM_BLOCK_STATE_OUTPUT_READY;
    return BM_OK;
}
