/**
 * @file bm_hal_dma_stream.c
 * @brief DMA 流 HAL 分发层
 *
 * 将 bm_hal_dma_stream 契约分发至平台 driver API；未链接后端时返回
 * BM_ERR_NOT_INIT。量产后由 portable 后端或 bm_port 对接厂商 DMA。
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
#include "bm_hal_dma_stream.h"

int bm_hal_dma_stream_bind_rx(const bm_hal_dma_stream_t *stream,
                              const bm_hal_dma_stream_binding_t *binding) {
    (void)stream;
    (void)binding;
    return BM_ERR_NOT_INIT;
}

int bm_hal_dma_stream_submit_rx(const bm_hal_dma_stream_t *stream,
                                  bm_block_t *block) {
    (void)stream;
    (void)block;
    return BM_ERR_NOT_INIT;
}

void bm_hal_dma_stream_stop(const bm_hal_dma_stream_t *stream) {
    (void)stream;
}
