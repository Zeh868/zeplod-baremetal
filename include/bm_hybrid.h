/**
 * @file bm_hybrid.h
 * @brief Control 层聚合头：HRT / Ticker / 多实例控制 / 同步域
 *
 * 要求 BM_CONFIG_ENABLE_HRT=1。子组件由 BM_CONFIG_ENABLE_* 进一步裁剪。
 */
#ifndef BM_HYBRID_H
#define BM_HYBRID_H

#include "bm_config.h"

#if !BM_CONFIG_ENABLE_HRT
#error "bm_hybrid.h requires BM_CONFIG_ENABLE_HRT"
#endif

#if BM_CONFIG_ENABLE_SYNC && !BM_CONFIG_ENABLE_CTRL_INST
#error "BM_CONFIG_ENABLE_SYNC requires BM_CONFIG_ENABLE_CTRL_INST"
#endif

#if BM_CONFIG_ENABLE_CTRL_INST && !BM_CONFIG_ENABLE_HRT
#error "BM_CONFIG_ENABLE_CTRL_INST requires BM_CONFIG_ENABLE_HRT"
#endif

#include "bm/hybrid/bm_hrt.h"
#include "bm/hybrid/bm_snapshot.h"

#if BM_CONFIG_ENABLE_TICKER
#include "bm/hybrid/bm_ticker.h"
#endif

#if BM_CONFIG_ENABLE_CTRL_INST
#include "bm/hybrid/bm_resource.h"
#include "bm/hybrid/bm_ctrl_inst.h"
#include "bm/hybrid/bm_runtime_model.h"
#endif

#if BM_CONFIG_ENABLE_SYNC
#include "bm/hybrid/bm_sync.h"
#endif

#endif /* BM_HYBRID_H */
