#include "bm_hal_memory.h"

void bm_memory_barrier_release(void) {
    __asm volatile ("" ::: "memory");
}

void bm_memory_barrier_full(void) {
    __asm volatile ("" ::: "memory");
}
