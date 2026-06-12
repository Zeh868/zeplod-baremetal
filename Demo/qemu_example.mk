ZEPLOD_ROOT ?= $(realpath ../..)
BM_BOOT_QEMU_CM0 := $(ZEPLOD_ROOT)/portable/boot/qemu_cortex_m0

CC      := arm-none-eabi-gcc
CFLAGS  ?= -mcpu=cortex-m0 -mthumb -Os -ffunction-sections -fdata-sections \
           -Wall -Wextra -std=c99
CFLAGS  += -I$(ZEPLOD_ROOT)/include \
           -I$(ZEPLOD_ROOT)/Demo/common \
           -I. -include bm_config.h
LDFLAGS ?= -T$(BM_BOOT_QEMU_CM0)/linker.ld \
           -nostartfiles -Wl,--gc-sections

QEMU_COMMON_SRCS := \
    $(ZEPLOD_ROOT)/Demo/common/example_support.c \
    $(BM_BOOT_QEMU_CM0)/startup_qemu_cm0.s \
    $(BM_BOOT_QEMU_CM0)/crt0_qemu.c

SRCS := main.c $(QEMU_COMMON_SRCS) $(FRAMEWORK_SRCS) $(HAL_SRCS) $(EXTRA_SRCS)

.PHONY: all clean qemu

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $@

qemu: $(TARGET)
	qemu-system-arm -machine microbit -cpu cortex-m0 -kernel $(TARGET) \
		--semihosting -nographic -serial stdio

clean:
	$(RM) $(TARGET)
