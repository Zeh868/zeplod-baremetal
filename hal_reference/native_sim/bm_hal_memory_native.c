/**
 * @file bm_hal_memory_native.c
 * @brief 原生仿真环境内存屏障 HAL 实现
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */

#include "bm_hal_memory.h"

#if defined(_MSC_VER)
#include <intrin.h>
#endif

/** Release 语义内存屏障（写操作对其他核/线程可见） */
void bm_memory_barrier_release(void) {
#if defined(__GNUC__) || defined(__clang__)
    __atomic_thread_fence(__ATOMIC_RELEASE);
#elif defined(_MSC_VER)
    _ReadWriteBarrier();
#else
    volatile int fence = 0;
    (void)fence;
#endif
}

/** 全序内存屏障（读写均有序） */
void bm_memory_barrier_full(void) {
#if defined(__GNUC__) || defined(__clang__)
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
#elif defined(_MSC_VER)
    _ReadWriteBarrier();
#else
    volatile int fence = 0;
    (void)fence;
#endif
}
