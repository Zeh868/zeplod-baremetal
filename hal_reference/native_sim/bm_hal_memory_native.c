#include "bm_hal_memory.h"

#if defined(_MSC_VER)
#include <intrin.h>
#endif

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
