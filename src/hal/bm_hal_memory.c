/**
 * @file bm_hal_memory.c
 * @brief 内存屏障 HAL 默认弱符号桩
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */
#include "bm_hal_memory.h"
#include "bm_hal_weak.h"

BM_HAL_WEAK
void bm_memory_barrier_release(void) {
}

BM_HAL_WEAK
void bm_memory_barrier_full(void) {
}
