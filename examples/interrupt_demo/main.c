#include "bm_core.h"
#include "bm_hal_uart.h"
#include "bm_hal_timer.h"
#include <string.h>

#define EVENT_TICK      1
#define EVENT_SAMPLE    2
#define EVENT_STATS     3

#define SYSTICK_FREQ_HZ     10      /* 10Hz = 100ms */
#define TIMER1_FREQ_HZ      2       /* 2Hz = 500ms */

/* nRF51 TIMER1 registers */
#define TIMER1_BASE         0x40009000
#define TIMER1_TASKS_START  (*(volatile uint32_t *)(TIMER1_BASE + 0x000))
#define TIMER1_TASKS_STOP   (*(volatile uint32_t *)(TIMER1_BASE + 0x004))
#define TIMER1_TASKS_CLEAR  (*(volatile uint32_t *)(TIMER1_BASE + 0x00C))
#define TIMER1_EVENTS_COMPARE0 (*(volatile uint32_t *)(TIMER1_BASE + 0x140))
#define TIMER1_INTENSET     (*(volatile uint32_t *)(TIMER1_BASE + 0x304))
#define TIMER1_INTENCLR     (*(volatile uint32_t *)(TIMER1_BASE + 0x308))
#define TIMER1_CC0          (*(volatile uint32_t *)(TIMER1_BASE + 0x540))
#define TIMER1_PRESCALER    (*(volatile uint32_t *)(TIMER1_BASE + 0x510))
#define TIMER1_MODE         (*(volatile uint32_t *)(TIMER1_BASE + 0x504))
#define TIMER1_BITMODE      (*(volatile uint32_t *)(TIMER1_BASE + 0x508))

/* nRF51 NVIC */
#define NVIC_ISER           (*(volatile uint32_t *)0xE000E100)
#define NVIC_ICPR           (*(volatile uint32_t *)0xE000E280)
#define TIMER1_IRQn         9

static volatile uint32_t g_tick_count = 0;
static volatile uint32_t g_sample_count = 0;
static volatile uint16_t g_next_temp = 25;

static void uart_print(const char *s) {
    bm_hal_uart_send((const uint8_t *)s, strlen(s));
}

/* TIMER1 IRQ handler */
void TIMER1_IRQHandler(void) {
    if (TIMER1_EVENTS_COMPARE0) {
        TIMER1_EVENTS_COMPARE0 = 0;
        TIMER1_TASKS_CLEAR = 1;
        TIMER1_TASKS_START = 1;

        uint16_t temp = g_next_temp++;
        bm_event_publish_copy_from_isr(EVENT_SAMPLE, 1, &temp, sizeof(temp));
        g_sample_count++;
    }
}

/* SysTick user callback */
static void on_systick(void) {
    uint32_t tick = g_tick_count + 1;
    bm_event_publish_copy_from_isr(EVENT_TICK, 0, &tick, sizeof(tick));
    g_tick_count = tick;
}

static void tick_cb(const bm_event_t *ev, void *ud) {
    (void)ud;
    const uint32_t *t = (const uint32_t *)ev->data;
    char buf[32];
    char *p = buf;
    strcpy(p, "[TICK] count=");
    p += strlen(p);
    uint32_t v = *t;
    char tmp[12];
    int n = 0;
    do { tmp[n++] = '0' + (v % 10); v /= 10; } while (v > 0);
    for (int i = n - 1; i >= 0; i--) *p++ = tmp[i];
    strcpy(p, "\n");
    uart_print(buf);
}

static void sample_cb(const bm_event_t *ev, void *ud) {
    (void)ud;
    const uint16_t *t = (const uint16_t *)ev->data;
    char buf[32];
    char *p = buf;
    strcpy(p, "[SAMPLE] temp=");
    p += strlen(p);
    uint16_t v = *t;
    char tmp[6];
    int n = 0;
    do { tmp[n++] = '0' + (v % 10); v /= 10; } while (v > 0);
    for (int i = n - 1; i >= 0; i--) *p++ = tmp[i];
    strcpy(p, "\n");
    uart_print(buf);
}

/* LIFO demo: subscribed first on EVENT_TICK, runs after tick_cb */
static void tick_lifo_cb(const bm_event_t *ev, void *ud) {
    (void)ev;
    (void)ud;
}

static void stats_cb(const bm_event_t *ev, void *ud) {
    (void)ev;
    (void)ud;
    char buf[48];
    char *p = buf;
    strcpy(p, "[STATS] ticks=");
    p += strlen(p);
    uint32_t v = g_tick_count;
    char tmp[12];
    int n = 0;
    do { tmp[n++] = '0' + (v % 10); v /= 10; } while (v > 0);
    for (int i = n - 1; i >= 0; i--) *p++ = tmp[i];
    strcpy(p, ", samples=");
    p += strlen(p);
    v = g_sample_count;
    n = 0;
    do { tmp[n++] = '0' + (v % 10); v /= 10; } while (v > 0);
    for (int i = n - 1; i >= 0; i--) *p++ = tmp[i];
    strcpy(p, "\n");
    uart_print(buf);
}

static void timer1_init(void) {
    (void)TIMER1_FREQ_HZ;
    TIMER1_TASKS_STOP = 1;
    TIMER1_TASKS_CLEAR = 1;
    TIMER1_MODE = 0;          /* Timer mode */
    TIMER1_BITMODE = 0;       /* 16-bit */
    TIMER1_PRESCALER = 4;     /* 16 MHz / 2^4 = 1 MHz */
    TIMER1_CC0 = 500000;      /* 500 ms @ 1 MHz */
    TIMER1_INTENCLR = 0xFFFFFFFF;
    TIMER1_INTENSET = (1 << 16); /* COMPARE0 interrupt */
    TIMER1_EVENTS_COMPARE0 = 0;
    NVIC_ICPR = (1 << TIMER1_IRQn);
    NVIC_ISER = (1 << TIMER1_IRQn);
    TIMER1_TASKS_START = 1;
}

int main(void) {
    bm_hal_uart_init(NULL);
    bm_event_register_type(EVENT_TICK, "TICK");
    bm_event_register_type(EVENT_SAMPLE, "SAMPLE");
    bm_event_register_type(EVENT_STATS, "STATS");

    bm_event_subscriber_id_t id_lifo, id_tick, id_sample, id_stats;
    bm_event_subscribe(EVENT_TICK, tick_lifo_cb, NULL, &id_lifo);
    bm_event_subscribe(EVENT_TICK, tick_cb, NULL, &id_tick);
    bm_event_subscribe(EVENT_SAMPLE, sample_cb, NULL, &id_sample);
    bm_event_subscribe(EVENT_STATS, stats_cb, NULL, &id_stats);
    (void)id_lifo;
    (void)id_sample;
    (void)id_stats;

    bm_hal_timer_set_callback(on_systick);
    bm_hal_timer_init(SYSTICK_FREQ_HZ);
    timer1_init();

    uart_print("IRQ Demo: interrupt_demo\n");

    uint8_t pass_sent = 0;
    uint8_t stats_sent = 0;

    while (1) {
        bm_event_process(8);

        if (g_tick_count >= 20 && g_sample_count >= 2 && !stats_sent) {
            bm_event_t ev = { .type = EVENT_STATS, .priority = 2, .data = NULL, .data_len = 0 };
            bm_event_publish_event(&ev);
            stats_sent = 1;
        }

        if (g_sample_count >= 2 && stats_sent && !pass_sent) {
            uart_print("EXAMPLE_IRQ: PASS\n");
            pass_sent = 1;
        }
    }
}
