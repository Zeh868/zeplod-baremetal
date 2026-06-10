TARGET := interrupt_demo.elf
FRAMEWORK_SRCS = \
    $(ZEPLOD_ROOT)/src/core/bm_critical.c \
    $(ZEPLOD_ROOT)/src/core/bm_event.c \
    $(ZEPLOD_ROOT)/src/core/bm_mempool.c
HAL_SRCS = \
    $(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c \
    $(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/bm_hal_timer_qemu.c
EXTRA_SRCS = interrupt_timer.c

include ../qemu_example.mk
