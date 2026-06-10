#include "bm_hal_critical.h"

/*
 * Override BM_STM32F0_DEVICE_HEADER when the vendor package uses a different
 * CMSIS device header name.
 */
#ifndef BM_STM32F0_DEVICE_HEADER
#define BM_STM32F0_DEVICE_HEADER "stm32f0xx.h"
#endif

#include BM_STM32F0_DEVICE_HEADER

bm_irq_state_t bm_hal_critical_enter(void) {
    bm_irq_state_t state = __get_PRIMASK();
    __disable_irq();
    return state;
}

void bm_hal_critical_exit(bm_irq_state_t state) {
    __set_PRIMASK(state);
}
