#include "bm_hal_timer.h"

/* CMSIS SystemInit stub required by startup assembly */
void SystemInit(void) {}

#define SYSTICK_BASE  0xE000E010
#define SYSTICK_CTRL  (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYSTICK_LOAD  (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYSTICK_VAL   (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))

static volatile uint32_t _ticks = 0;

void SysTick_Handler(void) {
    _ticks++;
}

int bm_hal_timer_init(uint32_t freq_hz) {
    uint32_t reload = 1000;
    SYSTICK_LOAD = reload - 1;
    SYSTICK_VAL = 0;
    SYSTICK_CTRL = 0x7;
    (void)freq_hz;
    return 0;
}

uint32_t bm_hal_timer_get_ticks(void) { return _ticks; }
uint32_t bm_hal_timer_get_freq(void) { return 1000; }
void bm_hal_timer_set_callback(void (*cb)(void)) { (void)cb; }
