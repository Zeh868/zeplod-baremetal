#include "example_support.h"

#include "bm_hal_uart.h"

#include <stddef.h>
#include <string.h>

void example_print(const char *text) {
    if (text) {
        bm_hal_uart_send((const uint8_t *)text, strlen(text));
    }
}

void example_print_u32(uint32_t value) {
    char reversed[10];
    char output[11];
    size_t count = 0;

    do {
        reversed[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    } while (value > 0U);

    for (size_t i = 0; i < count; i++) {
        output[i] = reversed[count - 1U - i];
    }
    output[count] = '\0';
    bm_hal_uart_send((const uint8_t *)output, count);
}

void example_delay_cycles(uint32_t cycles) {
    for (volatile uint32_t i = 0; i < cycles; i++) {
    }
}
