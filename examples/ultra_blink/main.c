#include "bm_ultra.h"
#include "bm_hal_uart.h"
#include "example_support.h"

#define EVENT_TICK          1
#define EVENT_BUTTON_PRESS  2

static uint8_t g_led_state = 0;
static uint32_t g_tick_count = 0;

static void on_tick(const void *data, uint8_t len) {
    (void)data;
    (void)len;
    g_led_state = !g_led_state;
    example_print(g_led_state ? "LED: ON\n" : "LED: OFF\n");
}

static void on_button(const void *data, uint8_t len) {
    (void)data;
    (void)len;
    example_print("BUTTON: pressed\n");
}

BM_ULTRA_CALLBACK_TABLE_DEFINE(
    BM_ULTRA_CB(EVENT_TICK, on_tick),
    BM_ULTRA_CB(EVENT_BUTTON_PRESS, on_button)
);

int main(void) {
    bm_hal_uart_init(NULL);
    bm_ultra_init();
    example_print("Zeplod Example: ultra_blink\n");

    uint32_t button_counter = 0;
    uint8_t pass_sent = 0;

    while (1) {
        example_delay_cycles(1000000U);
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
            example_print("EXAMPLE_ULTRA: PASS\n");
            pass_sent = 1;
        }
    }
}
