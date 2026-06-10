/**
 * @file bm_hal_memory_ch32v003.c
 * @brief CH32V003 内存屏障 HAL 参考实现
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */
#include "bm_hal_memory.h"

void bm_memory_barrier_release(void) {
    __asm volatile ("fence rw, rw" ::: "memory");
}

void bm_memory_barrier_full(void) {
    __asm volatile ("fence rw, rw" ::: "memory");
}
