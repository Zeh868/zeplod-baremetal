ZEPLOD_ROOT := $(realpath ../..)

CC      := arm-none-eabi-gcc
CFLAGS  := -mcpu=cortex-m0 -mthumb -Os -std=c99 \
           -I$(ZEPLOD_ROOT)/include -I. -include bm_config.h
LDFLAGS := -T$(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/linker.ld -nostartfiles

SRCS := main.c \
        $(ZEPLOD_ROOT)/src/core/bm_critical.c \
        $(ZEPLOD_ROOT)/src/core/bm_event.c \
        $(ZEPLOD_ROOT)/src/core/bm_mempool.c \
        $(ZEPLOD_ROOT)/src/module/bm_module.c \
        $(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/startup_qemu_cm0.s \
        $(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/crt0_qemu.c \
        $(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c

TARGET := core_sensor.elf

.PHONY: all clean qemu

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $@

qemu: $(TARGET)
	qemu-system-arm -machine microbit -cpu cortex-m0 -kernel $(TARGET) --semihosting -nographic -serial stdio

clean:
	rm -f $(TARGET)
