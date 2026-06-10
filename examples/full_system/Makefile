TARGET := full_system.elf
FRAMEWORK_SRCS = \
    $(ZEPLOD_ROOT)/src/core/bm_critical.c \
    $(ZEPLOD_ROOT)/src/core/bm_event.c \
    $(ZEPLOD_ROOT)/src/core/bm_mempool.c \
    $(ZEPLOD_ROOT)/src/module/bm_module.c \
    $(ZEPLOD_ROOT)/src/core/bm_wdg.c
HAL_SRCS = \
    $(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c \
    $(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/bm_hal_timer_qemu.c \
    $(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/bm_hal_wdg_qemu.c

include ../qemu_example.mk
