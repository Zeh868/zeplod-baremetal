#include "interrupt_timer.h"

#include <stdint.h>

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

static interrupt_timer_callback_t timer_callback;

void TIMER1_IRQHandler(void) {
    if (TIMER1_EVENTS_COMPARE0 == 0U) {
        return;
    }

    TIMER1_EVENTS_COMPARE0 = 0U;
    TIMER1_TASKS_CLEAR = 1U;
    TIMER1_TASKS_START = 1U;
    if (timer_callback) {
        timer_callback();
    }
}

void interrupt_timer_init(interrupt_timer_callback_t callback) {
    timer_callback = callback;

    TIMER1_TASKS_STOP = 1U;
    TIMER1_TASKS_CLEAR = 1U;
    TIMER1_MODE = 0U;
    TIMER1_BITMODE = 0U;
    TIMER1_PRESCALER = 4U;
    TIMER1_CC0 = 500000U;
    TIMER1_INTENCLR = 0xFFFFFFFFU;
    TIMER1_INTENSET = (1U << 16);
    TIMER1_EVENTS_COMPARE0 = 0U;
    NVIC_ICPR = (1U << TIMER1_IRQ_NUMBER);
    NVIC_ISER = (1U << TIMER1_IRQ_NUMBER);
    TIMER1_TASKS_START = 1U;
}
