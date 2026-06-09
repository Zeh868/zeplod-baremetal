#include "bm_ultra.h"
#include "bm_hal_uart.h"
#include <string.h>

#define EVENT_TICK          1
#define EVENT_BUTTON_PRESS  2

static uint8_t g_led_state = 0;
static uint32_t g_tick_count = 0;

static void on_tick(const void *data, uint8_t len) {
    (void)data;
    (void)len;
    g_led_state = !g_led_state;
    const char *msg = g_led_state ? "LED: ON\n" : "LED: OFF\n";
    bm_hal_uart_send((const uint8_t *)msg, strlen(msg));
}

static void on_button(const void *data, uint8_t len) {
    (void)data;
    (void)len;
    const char *msg = "BUTTON: pressed\n";
    bm_hal_uart_send((const uint8_t *)msg, strlen(msg));
}

BM_ULTRA_CALLBACK_TABLE_DEFINE(
    BM_ULTRA_CB(EVENT_TICK, on_tick),
    BM_ULTRA_CB(EVENT_BUTTON_PRESS, on_button)
);

static void delay_cycles(uint32_t n) {
    for (volatile uint32_t i = 0; i < n; i++) {}
}

int main(void) {
    bm_hal_uart_init(NULL);
    bm_ultra_init();

    const char *header = "Zeplod Example: ultra_blink\n";
    bm_hal_uart_send((const uint8_t *)header, strlen(header));

    uint32_t button_counter = 0;
    uint8_t pass_sent = 0;

    while (1) {
        delay_cycles(1000000);
        g_tick_count++;
        button_counter++;

        bm_ultra_publish(EVENT_TICK, NULL, 0);

        if (button_counter >= 3) {
            uint8_t btn_id = 0;
            bm_ultra_publish(EVENT_BUTTON_PRESS, &btn_id, sizeof(btn_id));
            button_counter = 0;
        }

        bm_ultra_process();

        if (g_tick_count >= 3 && !pass_sent) {
            const char *pass = "EXAMPLE_ULTRA: PASS\n";
            bm_hal_uart_send((const uint8_t *)pass, strlen(pass));
            pass_sent = 1;
        }
    }
}
