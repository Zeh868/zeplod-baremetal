/**
 * @file bm_hal_dma_stream.h
 * @brief DMA 块流 HAL 契约（提交缓冲、half/full 回调）
 *
 * 抽象 RX/TX 方向的块缓冲提交与半满/全满事件，供厂商 DMA 驱动或 native_sim
 * 后端实现。与 bm_stream 配合完成 DMA_OWNED ↔ READY 所有权交接。
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
 */#ifndef BM_HAL_DMA_STREAM_H
#define BM_HAL_DMA_STREAM_H

#include "bm/hybrid/bm_block.h"
#include "bm/common/bm_types.h"

typedef struct bm_hal_dma_stream bm_hal_dma_stream_t;

typedef void (*bm_hal_dma_stream_block_fn_t)(const bm_hal_dma_stream_t *stream,
                                             bm_block_t *block,
                                             void *context);

typedef struct {
    bm_hal_dma_stream_block_fn_t on_half;
    bm_hal_dma_stream_block_fn_t on_full;
    void *context;
} bm_hal_dma_stream_binding_t;

int bm_hal_dma_stream_bind_rx(const bm_hal_dma_stream_t *stream,
                              const bm_hal_dma_stream_binding_t *binding);

int bm_hal_dma_stream_submit_rx(const bm_hal_dma_stream_t *stream,
                                bm_block_t *block);

void bm_hal_dma_stream_stop(const bm_hal_dma_stream_t *stream);

#endif /* BM_HAL_DMA_STREAM_H */
