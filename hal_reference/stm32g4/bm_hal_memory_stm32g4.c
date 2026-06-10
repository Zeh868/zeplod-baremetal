/**
 * @file bm_hal_memory_stm32g4.c
 * @brief STM32G4 内存屏障 HAL 实现（DMB/DSB）
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#include "bm_hal_memory.h"

/** Release 语义：数据内存屏障 */
void bm_memory_barrier_release(void) {
    __asm volatile ("dmb" ::: "memory");
}

/** 全序：数据同步屏障 */
void bm_memory_barrier_full(void) {
    __asm volatile ("dsb" ::: "memory");
}
