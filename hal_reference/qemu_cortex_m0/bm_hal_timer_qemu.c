#include "bm_hal_timer.h"

#include <stddef.h>
#include <stdint.h>

/* nRF51 TIMER1 — micro:bit QEMU does not drive Cortex-M SysTick. */
#define TIMER1_BASE             0x40009000U
#define TIMER1_TASKS_START      (*(volatile uint32_t *)(TIMER1_BASE + 0x000U))
#define TIMER1_TASKS_STOP       (*(volatile uint32_t *)(TIMER1_BASE + 0x004U))
#define TIMER1_TASKS_CLEAR      (*(volatile uint32_t *)(TIMER1_BASE + 0x00CU))
#define TIMER1_EVENTS_COMPARE0  (*(volatile uint32_t *)(TIMER1_BASE + 0x140U))
#define TIMER1_MODE             (*(volatile uint32_t *)(TIMER1_BASE + 0x504U))
#define TIMER1_BITMODE          (*(volatile uint32_t *)(TIMER1_BASE + 0x508U))
#define TIMER1_PRESCALER        (*(volatile uint32_t *)(TIMER1_BASE + 0x510U))
#define TIMER1_CC0              (*(volatile uint32_t *)(TIMER1_BASE + 0x540U))
#define TIMER1_INTENSET         (*(volatile uint32_t *)(TIMER1_BASE + 0x304U))
#define TIMER1_INTENCLR         (*(volatile uint32_t *)(TIMER1_BASE + 0x308U))

#define NVIC_ISER               (*(volatile uint32_t *)0xE000E100U)
#define NVIC_ICPR               (*(volatile uint32_t *)0xE000E280U)
#define TIMER1_IRQ_NUMBER       9U

/* 16 MHz CPU with prescaler 4 -> 1 MHz timer counter. */
#define NRF_TIMER_CLK_HZ        1000000u

static volatile uint32_t _ticks;
static void (*_tick_cb)(void);
static uint32_t _tick_freq = 1000u;

void TIMER1_IRQHandler(void) {
    if (TIMER1_EVENTS_COMPARE0 == 0U) {
        return;
    }

    TIMER1_EVENTS_COMPARE0 = 0U;
    TIMER1_TASKS_CLEAR = 1U;
    _ticks++;
    if (_tick_cb) {
        _tick_cb();
    }
    TIMER1_TASKS_START = 1U;
}

static void timer_program_period(void) {
    uint32_t cc = NRF_TIMER_CLK_HZ / _tick_freq;

    if (cc == 0u) {
        cc = 1u;
    }
    TIMER1_CC0 = cc;
}

int bm_hal_timer_init(uint32_t freq_hz) {
    _tick_freq = (freq_hz > 0u) ? freq_hz : 1000u;

    TIMER1_TASKS_STOP = 1U;
    TIMER1_TASKS_CLEAR = 1U;
    TIMER1_MODE = 0U;
    TIMER1_BITMODE = 0U;
    TIMER1_PRESCALER = 4U;
    timer_program_period();
    TIMER1_INTENCLR = 0xFFFFFFFFU;
    TIMER1_INTENSET = (1U << 16);
    TIMER1_EVENTS_COMPARE0 = 0U;
    NVIC_ICPR = (1U << TIMER1_IRQ_NUMBER);
    NVIC_ISER = (1U << TIMER1_IRQ_NUMBER);
    TIMER1_TASKS_START = 1U;
    return 0;
}

void bm_hal_timer_stop(void) {
    TIMER1_TASKS_STOP = 1U;
    TIMER1_INTENCLR = (1U << 16);
    _tick_cb = NULL;
}

uint32_t bm_hal_timer_get_ticks(void) {
    return _ticks;
}

uint32_t bm_hal_timer_get_freq(void) {
    return _tick_freq;
}

void bm_hal_timer_set_callback(void (*cb)(void)) {
    _tick_cb = cb;
}
