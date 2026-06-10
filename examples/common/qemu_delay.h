#ifndef QEMU_DELAY_H
#define QEMU_DELAY_H

#include <stdint.h>

/** 让 QEMU 仿真时间推进，使 TIMER1 中断有机会触发 */
static inline void bm_example_qemu_spin(void) {
    for (volatile uint32_t i = 0u; i < 8000u; ++i) {
    }
}

static inline void bm_example_qemu_warmup(void) {
    for (volatile uint32_t i = 0u; i < 400000u; ++i) {
    }
}

#endif /* QEMU_DELAY_H */
