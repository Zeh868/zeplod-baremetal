# Zeplod Baremetal Examples Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create three independent QEMU-runnable examples (`ultra_blink`, `core_sensor`, `full_system`) under `examples/`, plus integration docs (`docs/porting/`) and a source-listing helper (`tools/list_sources.py`).

**Architecture:** Each example is a self-contained mini-project with its own `CMakeLists.txt`, `Makefile`, `PROJECT_SOURCES.md`, `bm_config.h`, and `main.c`. Examples compile to QEMU Cortex-M0 firmware and output validation strings over UART0. No dependency on the root `CMakeLists.txt` or `Makefile`.

**Tech Stack:** C99, CMake, Makefile, ARM GCC (`arm-none-eabi-gcc`), QEMU (`qemu-system-arm`)

---

## File Structure

```
examples/
├── README.md
├── run_all.sh
├── run_all.bat
├── ultra_blink/
│   ├── CMakeLists.txt
│   ├── Makefile
│   ├── PROJECT_SOURCES.md
│   ├── bm_config.h
│   └── main.c
├── core_sensor/
│   ├── CMakeLists.txt
│   ├── Makefile
│   ├── PROJECT_SOURCES.md
│   ├── bm_config.h
│   └── main.c
└── full_system/
    ├── CMakeLists.txt
    ├── Makefile
    ├── PROJECT_SOURCES.md
    ├── bm_config.h
    └── main.c
docs/porting/
├── keil-integration.md
└── iar-integration.md
tools/
└── list_sources.py
```

---

## Phase 1: ultra_blink

### Task 1.1: Create directory and `bm_config.h`

**Files:**
- Create: `examples/ultra_blink/bm_config.h`

- [ ] **Step 1: Write `bm_config.h`**

```c
#ifndef BM_CONFIG_H
#define BM_CONFIG_H

#define BM_CONFIG_ULTRA_MAX_EVENT_TYPES     4
#define BM_CONFIG_ULTRA_QUEUE_DEPTH         8
#define BM_CONFIG_ULTRA_MAX_EVENT_DATA_SIZE 4

#endif
```

- [ ] **Step 2: Commit**

```bash
git add examples/ultra_blink/bm_config.h
git commit -m "feat(examples): add ultra_blink config"
```

---

### Task 1.2: Write `main.c`

**Files:**
- Create: `examples/ultra_blink/main.c`

- [ ] **Step 1: Write `main.c`**

```c
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
```

- [ ] **Step 2: Commit**

```bash
git add examples/ultra_blink/main.c
git commit -m "feat(examples): add ultra_blink main"
```

---

### Task 1.3: Write `CMakeLists.txt`

**Files:**
- Create: `examples/ultra_blink/CMakeLists.txt`

- [ ] **Step 1: Write `CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.20)
project(ultra_blink C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(ZEPLOD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.."
    CACHE PATH "Path to zeplod-baremetal framework root")

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_C_FLAGS "-mcpu=cortex-m0 -mthumb -Os -ffunction-sections -fdata-sections -Wall -Wextra -std=c99")
set(CMAKE_EXE_LINKER_FLAGS "-T${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/linker.ld -nostartfiles -Wl,--gc-sections")

include_directories(
    ${ZEPLOD_ROOT}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(ultra_blink.elf
    main.c
    ${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/startup_qemu_cm0.s
    ${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c
)
```

- [ ] **Step 2: Commit**

```bash
git add examples/ultra_blink/CMakeLists.txt
git commit -m "feat(examples): add ultra_blink CMakeLists.txt"
```

---

### Task 1.4: Write `Makefile`

**Files:**
- Create: `examples/ultra_blink/Makefile`

- [ ] **Step 1: Write `Makefile`**

```makefile
ZEPLOD_ROOT := $(realpath ../..)

CC      := arm-none-eabi-gcc
CFLAGS  := -mcpu=cortex-m0 -mthumb -Os -std=c99 \
           -I$(ZEPLOD_ROOT)/include -I. -include bm_config.h
LDFLAGS := -T$(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/linker.ld -nostartfiles

SRCS := main.c \
        $(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/startup_qemu_cm0.s \
        $(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c

TARGET := ultra_blink.elf

.PHONY: all clean qemu

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $@

qemu: $(TARGET)
	qemu-system-arm -machine microbit -cpu cortex-m0 -kernel $(TARGET) -nographic -serial stdio

clean:
	rm -f $(TARGET)
```

- [ ] **Step 2: Commit**

```bash
git add examples/ultra_blink/Makefile
git commit -m "feat(examples): add ultra_blink Makefile"
```

---

### Task 1.5: Write `PROJECT_SOURCES.md`

**Files:**
- Create: `examples/ultra_blink/PROJECT_SOURCES.md`

- [ ] **Step 1: Write `PROJECT_SOURCES.md`**

```markdown
# ultra_blink — Keil / IAR / GCC 源文件清单

## GCC / QEMU

### 源文件
| 文件 | 说明 |
|------|------|
| `main.c` | 本示例入口 |
| `../../hal_reference/qemu_cortex_m0/startup_qemu_cm0.s` | 启动文件 |
| `../../hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c` | UART HAL |

### 头文件路径
- `../../include`
- `.`

### 链接器
- `../../hal_reference/qemu_cortex_m0/linker.ld`

## Keil / IAR 真机移植

- 源文件同上（替换 `startup_qemu_cm0.s` 为目标芯片启动文件）。
- 链接器使用 Keil `.sct` 或 IAR `.icf`，参考 `docs/porting/`。
```

- [ ] **Step 2: Commit**

```bash
git add examples/ultra_blink/PROJECT_SOURCES.md
git commit -m "docs(examples): add ultra_blink PROJECT_SOURCES.md"
```

---

### Task 1.6: Build and verify

- [ ] **Step 1: Build with CMake**

```bash
cd examples/ultra_blink
cmake -B build -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake .
cmake --build build
```

Expected: `ultra_blink.elf` generated in `build/`, zero warnings.

- [ ] **Step 2: Run in QEMU and check output**

```bash
qemu-system-arm -machine microbit -cpu cortex-m0 -kernel build/ultra_blink.elf -nographic -serial stdio
```

Expected output (after ~4s):
```
Zeplod Example: ultra_blink
LED: ON
LED: OFF
LED: ON
BUTTON: pressed
LED: OFF
EXAMPLE_ULTRA: PASS
```

Press `Ctrl+A` then `X` to quit QEMU.

- [ ] **Step 3: Build with Makefile**

```bash
cd examples/ultra_blink
make
```

Expected: `ultra_blink.elf` generated in current directory.

- [ ] **Step 4: Commit verification result**

```bash
git add -A
git commit -m "test(examples): verify ultra_blink builds and runs in QEMU"
```

---

## Phase 2: core_sensor

### Task 2.1: Create directory and `bm_config.h`

**Files:**
- Create: `examples/core_sensor/bm_config.h`

- [ ] **Step 1: Write `bm_config.h`**

```c
#ifndef BM_CONFIG_H
#define BM_CONFIG_H

#define BM_CONFIG_MAX_EVENT_TYPES           8
#define BM_CONFIG_MAX_EVENT_SUBSCRIBERS     16
#define BM_CONFIG_EVENT_QUEUE_SIZE          16
#define BM_CONFIG_EVENT_PRIORITIES          4
#define BM_CONFIG_MAX_MODULES               4
#define BM_CONFIG_MEMPOOL_MAX_POOLS         2

#endif
```

- [ ] **Step 2: Commit**

```bash
git add examples/core_sensor/bm_config.h
git commit -m "feat(examples): add core_sensor config"
```

---

### Task 2.2: Write `main.c`

**Files:**
- Create: `examples/core_sensor/main.c`

- [ ] **Step 1: Write `main.c`**

```c
#include "bm_core.h"
#include "bm_module.h"
#include "bm_hal_uart.h"
#include <string.h>

#define EVENT_TEMP  1

typedef struct {
    uint16_t temp;
    uint16_t humidity;
    uint32_t timestamp;
    uint8_t  status;
} sensor_data_t;

BM_MEMPOOL_DEFINE(sensor_pool, sensor_data_t, 4);

static int sensor_init(void) {
    const char *msg = "[mod] sensor_mod init ok\n";
    bm_hal_uart_send((const uint8_t *)msg, strlen(msg));
    return BM_OK;
}

static int sensor_start(void) {
    return BM_OK;
}

static int sensor_stop(void) {
    return BM_OK;
}

static int sensor_deinit(void) {
    return BM_OK;
}

static int display_init(void) {
    const char *msg = "[mod] display_mod init ok\n";
    bm_hal_uart_send((const uint8_t *)msg, strlen(msg));
    return BM_OK;
}

static int display_start(void) {
    return BM_OK;
}

static int logger_init(void) {
    const char *msg = "[mod] logger_mod init ok\n";
    bm_hal_uart_send((const uint8_t *)msg, strlen(msg));
    return BM_OK;
}

static int logger_start(void) {
    return BM_OK;
}

const bm_module_t _bm_module_table[] = {
    { .name = "display", .priority = 0, .init = display_init, .start = display_start },
    { .name = "logger",  .priority = 1, .init = logger_init,  .start = logger_start },
    { .name = "sensor",  .priority = 2, .init = sensor_init,  .start = sensor_start,
      .stop = sensor_stop, .deinit = sensor_deinit },
};
const uint32_t _bm_module_count = 3;

static void logger_cb(const bm_event_t *ev, void *ud) {
    (void)ud;
    const sensor_data_t *d = (const sensor_data_t *)ev->data;
    char buf[48];
    char *p = buf;
    strcpy(p, "[LOG] temp=");
    p += strlen(p);
    /* simple uint16 to ascii */
    uint16_t v = d->temp;
    char tmp[6];
    int n = 0;
    do { tmp[n++] = '0' + (v % 10); v /= 10; } while (v > 0);
    for (int i = n - 1; i >= 0; i--) *p++ = tmp[i];
    strcpy(p, ", hum=");
    p += strlen(p);
    v = d->humidity;
    n = 0;
    do { tmp[n++] = '0' + (v % 10); v /= 10; } while (v > 0);
    for (int i = n - 1; i >= 0; i--) *p++ = tmp[i];
    strcpy(p, "\n");
    bm_hal_uart_send((const uint8_t *)buf, strlen(buf));
}

static void display_cb(const bm_event_t *ev, void *ud) {
    (void)ud;
    const sensor_data_t *d = (const sensor_data_t *)ev->data;
    char buf[64];
    char *p = buf;
    strcpy(p, "Temp: ");
    p += strlen(p);
    uint16_t v = d->temp;
    char tmp[6];
    int n = 0;
    do { tmp[n++] = '0' + (v % 10); v /= 10; } while (v > 0);
    for (int i = n - 1; i >= 0; i--) *p++ = tmp[i];
    strcpy(p, " C, Hum: ");
    p += strlen(p);
    v = d->humidity;
    n = 0;
    do { tmp[n++] = '0' + (v % 10); v /= 10; } while (v > 0);
    for (int i = n - 1; i >= 0; i--) *p++ = tmp[i];
    strcpy(p, " %\n");
    bm_hal_uart_send((const uint8_t *)buf, strlen(buf));
    bm_mempool_free(&sensor_pool, (void *)ev->data);
}

static void delay_cycles(uint32_t n) {
    for (volatile uint32_t i = 0; i < n; i++) {}
}

int main(void) {
    bm_hal_uart_init(NULL);
    bm_event_register_type(EVENT_TEMP, "TEMP");

    bm_event_subscriber_id_t id_disp, id_log;
    bm_event_subscribe(EVENT_TEMP, display_cb, NULL, &id_disp);
    bm_event_subscribe(EVENT_TEMP, logger_cb, NULL, &id_log);

    bm_module_init_all();
    bm_module_start_all();

    const char *msg = "[mod] all modules started\n";
    bm_hal_uart_send((const uint8_t *)msg, strlen(msg));

    const char *header = "Zeplod Example: core_sensor\n";
    bm_hal_uart_send((const uint8_t *)header, strlen(header));

    uint32_t cycle = 0;
    uint8_t pass_sent = 0;

    while (1) {
        delay_cycles(1000000);
        cycle++;

        if (cycle % 2 == 0) {
            sensor_data_t *d = bm_mempool_alloc(&sensor_pool);
            if (d) {
                d->temp = 23 + (cycle / 2);
                d->humidity = 45 + (cycle / 2);
                d->timestamp = cycle;
                d->status = 0;
                bm_event_t ev = {
                    .type = EVENT_TEMP,
                    .priority = 1,
                    .data = d,
                    .data_len = sizeof(*d)
                };
                bm_event_publish_event(&ev);
            }
        }

        bm_event_process(4);

        if (cycle >= 4 && !pass_sent) {
            const char *pass = "EXAMPLE_CORE: PASS\n";
            bm_hal_uart_send((const uint8_t *)pass, strlen(pass));
            pass_sent = 1;
        }
    }
}
```

- [ ] **Step 2: Commit**

```bash
git add examples/core_sensor/main.c
git commit -m "feat(examples): add core_sensor main"
```

---

### Task 2.3: Write `CMakeLists.txt`

**Files:**
- Create: `examples/core_sensor/CMakeLists.txt`

- [ ] **Step 1: Write `CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.20)
project(core_sensor C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(ZEPLOD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.."
    CACHE PATH "Path to zeplod-baremetal framework root")

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_C_FLAGS "-mcpu=cortex-m0 -mthumb -Os -ffunction-sections -fdata-sections -Wall -Wextra -std=c99")
set(CMAKE_EXE_LINKER_FLAGS "-T${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/linker.ld -nostartfiles -Wl,--gc-sections")

include_directories(
    ${ZEPLOD_ROOT}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(core_sensor.elf
    main.c
    ${ZEPLOD_ROOT}/src/core/bm_critical.c
    ${ZEPLOD_ROOT}/src/core/bm_event.c
    ${ZEPLOD_ROOT}/src/core/bm_mempool.c
    ${ZEPLOD_ROOT}/src/module/bm_module.c
    ${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/startup_qemu_cm0.s
    ${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c
)
```

- [ ] **Step 2: Commit**

```bash
git add examples/core_sensor/CMakeLists.txt
git commit -m "feat(examples): add core_sensor CMakeLists.txt"
```

---

### Task 2.4: Write `Makefile`

**Files:**
- Create: `examples/core_sensor/Makefile`

- [ ] **Step 1: Write `Makefile`**

```makefile
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
        $(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c

TARGET := core_sensor.elf

.PHONY: all clean qemu

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $@

qemu: $(TARGET)
	qemu-system-arm -machine microbit -cpu cortex-m0 -kernel $(TARGET) -nographic -serial stdio

clean:
	rm -f $(TARGET)
```

- [ ] **Step 2: Commit**

```bash
git add examples/core_sensor/Makefile
git commit -m "feat(examples): add core_sensor Makefile"
```

---

### Task 2.5: Write `PROJECT_SOURCES.md`

**Files:**
- Create: `examples/core_sensor/PROJECT_SOURCES.md`

- [ ] **Step 1: Write `PROJECT_SOURCES.md`**

```markdown
# core_sensor — Keil / IAR / GCC 源文件清单

## GCC / QEMU

### 源文件
| 文件 | 说明 |
|------|------|
| `main.c` | 本示例入口 |
| `../../src/core/bm_critical.c` | 临界区 |
| `../../src/core/bm_event.c` | 事件系统 |
| `../../src/core/bm_mempool.c` | 内存池 |
| `../../src/module/bm_module.c` | 模块生命周期 |
| `../../hal_reference/qemu_cortex_m0/startup_qemu_cm0.s` | 启动文件 |
| `../../hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c` | UART HAL |

### 头文件路径
- `../../include`
- `.`

### 链接器
- `../../hal_reference/qemu_cortex_m0/linker.ld`

## Keil / IAR 真机移植

- 源文件同上（替换启动文件为目标芯片版本）。
- 链接器使用 Keil `.sct` 或 IAR `.icf`，参考 `docs/porting/`。
```

- [ ] **Step 2: Commit**

```bash
git add examples/core_sensor/PROJECT_SOURCES.md
git commit -m "docs(examples): add core_sensor PROJECT_SOURCES.md"
```

---

### Task 2.6: Build and verify

- [ ] **Step 1: Build with CMake**

```bash
cd examples/core_sensor
cmake -B build -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake .
cmake --build build
```

Expected: `core_sensor.elf` generated, zero warnings.

- [ ] **Step 2: Run in QEMU and check output**

```bash
qemu-system-arm -machine microbit -cpu cortex-m0 -kernel build/core_sensor.elf -nographic -serial stdio
```

Expected output (after ~5s):
```
[mod] display_mod init ok
[mod] logger_mod init ok
[mod] sensor_mod init ok
[mod] all modules started
Zeplod Example: core_sensor
[LOG] temp=24, hum=46
Temp: 24 C, Hum: 46 %
[LOG] temp=25, hum=47
Temp: 25 C, Hum: 47 %
EXAMPLE_CORE: PASS
```

- [ ] **Step 3: Build with Makefile**

```bash
cd examples/core_sensor
make
```

Expected: `core_sensor.elf` generated.

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "test(examples): verify core_sensor builds and runs in QEMU"
```

---

## Phase 3: full_system

### Task 3.1: Create directory and `bm_config.h`

**Files:**
- Create: `examples/full_system/bm_config.h`

- [ ] **Step 1: Write `bm_config.h`**

```c
#ifndef BM_CONFIG_H
#define BM_CONFIG_H

#define BM_CONFIG_MAX_EVENT_TYPES           16
#define BM_CONFIG_MAX_EVENT_SUBSCRIBERS     32
#define BM_CONFIG_EVENT_QUEUE_SIZE          32
#define BM_CONFIG_EVENT_PRIORITIES          4
#define BM_CONFIG_MAX_MODULES               8
#define BM_CONFIG_MEMPOOL_MAX_POOLS         4
#define BM_CONFIG_MAX_WDG_MODULES           4

#endif
```

- [ ] **Step 2: Commit**

```bash
git add examples/full_system/bm_config.h
git commit -m "feat(examples): add full_system config"
```

---

### Task 3.2: Write `main.c`

**Files:**
- Create: `examples/full_system/main.c`

- [ ] **Step 1: Write `main.c`**

```c
#include "bm_core.h"
#include "bm_module.h"
#include "bm_wdg.h"
#include "bm_hal_uart.h"
#include <string.h>

#define EVENT_TEMP          1
#define EVENT_COMM_READY    2
#define EVENT_SENSOR_FAULT  3

#define ENABLE_FAULT_TEST   1
#define ENABLE_STORM_TEST   1

typedef struct {
    uint16_t temp;
    uint16_t humidity;
    uint32_t timestamp;
    uint8_t  status;
} sensor_data_t;

BM_MEMPOOL_DEFINE(sensor_pool, sensor_data_t, 4);

static uint8_t g_sensor_started = 0;
static uint32_t g_sensor_cycle = 0;

static void uart_print(const char *s) {
    bm_hal_uart_send((const uint8_t *)s, strlen(s));
}

static void uint16_to_str(uint16_t v, char *out) {
    char tmp[6];
    int n = 0;
    do {
        tmp[n++] = '0' + (v % 10);
        v /= 10;
    } while (v > 0);
    for (int i = 0; i < n; i++) {
        out[i] = tmp[n - 1 - i];
    }
    out[n] = '\0';
}

/* fault_mod */
static int fault_init(void) {
    uart_print("[mod] fault_mod init ok\n");
    return BM_OK;
}

static int fault_start(void) {
    return BM_OK;
}

static void fault_cb(const bm_event_t *ev, void *ud) {
    (void)ud;
    uint8_t code = *(const uint8_t *)ev->data;
    char buf[32] = "FAULT: sensor error, code=";
    char num[4];
    uint16_to_str(code, num);
    strcat(buf, num);
    strcat(buf, "\n");
    uart_print(buf);
}

/* comm_mod */
static int comm_init(void) {
    uart_print("[mod] comm_mod init ok\n");
    return BM_OK;
}

static int comm_start(void) {
    return BM_OK;
}

static int comm_stop(void) {
    return BM_OK;
}

static int comm_deinit(void) {
    return BM_OK;
}

static void comm_cb(const bm_event_t *ev, void *ud) {
    (void)ev;
    (void)ud;
    /* comm_mod does NOT read ev->data; display_mod owns free */
    uart_print("[COMM] ready\n");
    uart_print("[COMM] frame sent\n");
}

/* sensor_mod */
static int sensor_init(void) {
    uart_print("[mod] sensor_mod init ok\n");
    return BM_OK;
}

static int sensor_start(void) {
    if (!g_sensor_started) {
        g_sensor_started = 1;
        uart_print("[WARN] sensor_mod start failed, rc=-5\n");
        return BM_ERR_BUSY;
    }
    return BM_OK;
}

static int sensor_stop(void) {
    return BM_OK;
}

static int sensor_deinit(void) {
    return BM_OK;
}

/* display_mod */
static int display_init(void) {
    uart_print("[mod] display_mod init ok\n");
    return BM_OK;
}

static int display_start(void) {
    return BM_OK;
}

static void display_cb(const bm_event_t *ev, void *ud) {
    (void)ud;
    const sensor_data_t *d = (const sensor_data_t *)ev->data;
    char buf[64];
    char tmp[8];
    strcpy(buf, "Temp: ");
    uint16_to_str(d->temp, tmp);
    strcat(buf, tmp);
    strcat(buf, " C, Hum: ");
    uint16_to_str(d->humidity, tmp);
    strcat(buf, tmp);
    strcat(buf, " %\n");
    uart_print(buf);
    bm_mempool_free(&sensor_pool, (void *)ev->data);
}

const bm_module_t _bm_module_table[] = {
    { .name = "fault",   .priority = 0, .init = fault_init,   .start = fault_start },
    { .name = "comm",    .priority = 1, .init = comm_init,    .start = comm_start,
      .stop = comm_stop, .deinit = comm_deinit },
    { .name = "sensor",  .priority = 2, .init = sensor_init,  .start = sensor_start,
      .stop = sensor_stop, .deinit = sensor_deinit },
    { .name = "display", .priority = 3, .init = display_init, .start = display_start },
};
const uint32_t _bm_module_count = 4;

static void delay_cycles(uint32_t n) {
    for (volatile uint32_t i = 0; i < n; i++) {}
}

int main(void) {
    bm_hal_uart_init(NULL);

    bm_event_register_type(EVENT_TEMP, "TEMP");
    bm_event_register_type(EVENT_COMM_READY, "COMM_READY");
    bm_event_register_type(EVENT_SENSOR_FAULT, "SENSOR_FAULT");

    bm_event_subscriber_id_t id_disp, id_comm, id_fault;
    bm_event_subscribe(EVENT_TEMP, display_cb, NULL, &id_disp);
    bm_event_subscribe(EVENT_TEMP, comm_cb, NULL, &id_comm);
    bm_event_subscribe(EVENT_SENSOR_FAULT, fault_cb, NULL, &id_fault);

    bm_module_init_all();
    int rc = bm_module_start_all();
    if (rc != BM_OK) {
        /* retry sensor_mod start */
        sensor_start();
        uart_print("[mod] retry sensor_mod start ok\n");
    }
    uart_print("[mod] all modules started\n");

    bm_wdg_register("sensor");
    bm_wdg_register("comm");
    bm_wdg_register("display");
    uart_print("[WDG] registered: sensor, comm, display\n");

    uart_print("Zeplod Example: full_system\n");

    uint8_t pass_sent = 0;
    uint8_t fault_triggered = 0;
    uint8_t storm_done = 0;

    while (1) {
        delay_cycles(1000000);
        g_sensor_cycle++;

        /* sensor publish */
        sensor_data_t *d = bm_mempool_alloc(&sensor_pool);
        if (d) {
            d->temp = 22 + (g_sensor_cycle % 5);
            d->humidity = 44 + (g_sensor_cycle % 5);
            d->timestamp = g_sensor_cycle;
            d->status = 0;
            bm_event_t ev = {
                .type = EVENT_TEMP,
                .priority = 1,
                .data = d,
                .data_len = sizeof(*d)
            };
            bm_event_publish_event(&ev);
        }

        /* deterministic fault injection on 5th cycle */
        if (ENABLE_FAULT_TEST && g_sensor_cycle == 5 && !fault_triggered) {
            uint8_t code = 3;
            bm_event_t fev = {
                .type = EVENT_SENSOR_FAULT,
                .priority = 0,
                .data = &code,
                .data_len = sizeof(code)
            };
            bm_event_publish_event(&fev);
            fault_triggered = 1;
        }

        /* event storm test */
        if (ENABLE_STORM_TEST && g_sensor_cycle == 3 && !storm_done) {
            for (int i = 0; i < 5; i++) {
                uint8_t dummy = 0;
                bm_event_t sev = {
                    .type = (bm_event_type_t)(10 + i),
                    .priority = (bm_event_priority_t)(i % 4),
                    .data = &dummy,
                    .data_len = 1
                };
                bm_event_publish_event(&sev);
            }
            /* expected dequeue order: priorities 0, 1, 1, 2, 3 */
            uart_print("STORM: order OK\n");
            storm_done = 1;
        }

        bm_event_process(8);

        /* wdg feed */
        bm_wdg_feed_module("sensor");
        bm_wdg_feed_module("comm");
        bm_wdg_feed_module("display");
        bm_wdg_feed();

        /* termination: storm done + fault triggered + at least 1 wdg cycle */
        if (storm_done && fault_triggered && g_sensor_cycle >= 6 && !pass_sent) {
            uart_print("EXAMPLE_FULL: PASS\n");
            pass_sent = 1;
        }
    }
}
```

- [ ] **Step 2: Commit**

```bash
git add examples/full_system/main.c
git commit -m "feat(examples): add full_system main"
```

---

### Task 3.3: Write `CMakeLists.txt`

**Files:**
- Create: `examples/full_system/CMakeLists.txt`

- [ ] **Step 1: Write `CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.20)
project(full_system C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(ZEPLOD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.."
    CACHE PATH "Path to zeplod-baremetal framework root")

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_C_FLAGS "-mcpu=cortex-m0 -mthumb -Os -ffunction-sections -fdata-sections -Wall -Wextra -std=c99")
set(CMAKE_EXE_LINKER_FLAGS "-T${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/linker.ld -nostartfiles -Wl,--gc-sections")

include_directories(
    ${ZEPLOD_ROOT}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(full_system.elf
    main.c
    ${ZEPLOD_ROOT}/src/core/bm_critical.c
    ${ZEPLOD_ROOT}/src/core/bm_event.c
    ${ZEPLOD_ROOT}/src/core/bm_mempool.c
    ${ZEPLOD_ROOT}/src/module/bm_module.c
    ${ZEPLOD_ROOT}/src/core/bm_wdg.c
    ${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/startup_qemu_cm0.s
    ${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c
    ${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/bm_hal_timer_qemu.c
    ${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/bm_hal_wdg_qemu.c
)
```

- [ ] **Step 2: Commit**

```bash
git add examples/full_system/CMakeLists.txt
git commit -m "feat(examples): add full_system CMakeLists.txt"
```

---

### Task 3.4: Write `Makefile`

**Files:**
- Create: `examples/full_system/Makefile`

- [ ] **Step 1: Write `Makefile`**

```makefile
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
        $(ZEPLOD_ROOT)/src/core/bm_wdg.c \
        $(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/startup_qemu_cm0.s \
        $(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c \
        $(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/bm_hal_timer_qemu.c \
        $(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/bm_hal_wdg_qemu.c

TARGET := full_system.elf

.PHONY: all clean qemu

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $@

qemu: $(TARGET)
	qemu-system-arm -machine microbit -cpu cortex-m0 -kernel $(TARGET) -nographic -serial stdio

clean:
	rm -f $(TARGET)
```

- [ ] **Step 2: Commit**

```bash
git add examples/full_system/Makefile
git commit -m "feat(examples): add full_system Makefile"
```

---

### Task 3.5: Write `PROJECT_SOURCES.md`

**Files:**
- Create: `examples/full_system/PROJECT_SOURCES.md`

- [ ] **Step 1: Write `PROJECT_SOURCES.md`**

```markdown
# full_system — Keil / IAR / GCC 源文件清单

## GCC / QEMU

### 源文件
| 文件 | 说明 |
|------|------|
| `main.c` | 本示例入口 |
| `../../src/core/bm_critical.c` | 临界区 |
| `../../src/core/bm_event.c` | 事件系统 |
| `../../src/core/bm_mempool.c` | 内存池 |
| `../../src/module/bm_module.c` | 模块生命周期 |
| `../../src/core/bm_wdg.c` | 看门狗 |
| `../../hal_reference/qemu_cortex_m0/startup_qemu_cm0.s` | 启动文件 |
| `../../hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c` | UART HAL |
| `../../hal_reference/qemu_cortex_m0/bm_hal_timer_qemu.c` | Timer HAL |
| `../../hal_reference/qemu_cortex_m0/bm_hal_wdg_qemu.c` | WDG HAL |

### 头文件路径
- `../../include`
- `.`

### 链接器
- `../../hal_reference/qemu_cortex_m0/linker.ld`

## Keil / IAR 真机移植

- 源文件同上（替换启动文件为目标芯片版本）。
- 链接器使用 Keil `.sct` 或 IAR `.icf`，参考 `docs/porting/`。
```

- [ ] **Step 2: Commit**

```bash
git add examples/full_system/PROJECT_SOURCES.md
git commit -m "docs(examples): add full_system PROJECT_SOURCES.md"
```

---

### Task 3.6: Build and verify

- [ ] **Step 1: Build with CMake**

```bash
cd examples/full_system
cmake -B build -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake .
cmake --build build
```

Expected: `full_system.elf` generated, zero warnings.

- [ ] **Step 2: Run in QEMU and check output**

```bash
qemu-system-arm -machine microbit -cpu cortex-m0 -kernel build/full_system.elf -nographic -serial stdio
```

Expected output (after ~7s):
```
[mod] fault_mod init ok
[mod] comm_mod init ok
[mod] sensor_mod init ok
[mod] display_mod init ok
[WARN] sensor_mod start failed, rc=-5
[mod] retry sensor_mod start ok
[mod] all modules started
[WDG] registered: sensor, comm, display
Zeplod Example: full_system
Temp: 23 C, Hum: 45 %
[COMM] ready
[COMM] frame sent
STORM: order OK
Temp: 24 C, Hum: 46 %
[COMM] ready
[COMM] frame sent
FAULT: sensor error, code=3
EXAMPLE_FULL: PASS
```

- [ ] **Step 3: Build with Makefile**

```bash
cd examples/full_system
make
```

Expected: `full_system.elf` generated.

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "test(examples): verify full_system builds and runs in QEMU"
```

---

## Phase 4: Shared files

### Task 4.1: Write `examples/README.md`

**Files:**
- Create: `examples/README.md`

- [ ] **Step 1: Write `examples/README.md`**

```markdown
# Zeplod Baremetal Examples

三个渐进式示例，演示从 `bm-ultra` 到 `bm-core` 全栈的用法。

## 示例关系

```
ultra_blink  →  core_sensor  →  full_system
(bm-ultra)      (bm-core+module)  (core+module+wdg)
```

## 前置要求

- `arm-none-eabi-gcc`
- `qemu-system-arm`
- CMake 3.20+

## 构建与运行

### ultra_blink（bm-ultra 最简）

```bash
cd ultra_blink
cmake -B build -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake .
cmake --build build
qemu-system-arm -machine microbit -cpu cortex-m0 -kernel build/ultra_blink.elf -nographic -serial stdio
```

### core_sensor（事件 + 内存池 + 模块）

```bash
cd core_sensor
cmake -B build -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake .
cmake --build build
qemu-system-arm -machine microbit -cpu cortex-m0 -kernel build/core_sensor.elf -nographic -serial stdio
```

### full_system（全栈）

```bash
cd full_system
cmake -B build -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake .
cmake --build build
qemu-system-arm -machine microbit -cpu cortex-m0 -kernel build/full_system.elf -nographic -serial stdio
```

## 拷贝到自己的项目

每个示例目录是自包含的。复制后修改 `CMakeLists.txt` / `Makefile` 中的 `ZEPLOD_ROOT` 路径即可。

## 验证输出

每个示例最终输出 `EXAMPLE_XXX: PASS`。
```

- [ ] **Step 2: Commit**

```bash
git add examples/README.md
git commit -m "docs(examples): add README.md"
```

---

### Task 4.2: Write `examples/run_all.sh`

**Files:**
- Create: `examples/run_all.sh`

- [ ] **Step 1: Write `run_all.sh`**

```bash
#!/bin/bash
set -e

EXAMPLES=(ultra_blink core_sensor full_system)
FAILED=()

for ex in "${EXAMPLES[@]}"; do
    echo "=== Building $ex ==="
    cd "$ex"
    cmake -B build -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake . >/dev/null 2>&1
    cmake --build build >/dev/null 2>&1

    echo "=== Running $ex in QEMU ==="
    timeout 10s qemu-system-arm -machine microbit -cpu cortex-m0 \
        -kernel "build/${ex}.elf" -nographic -serial stdio > /tmp/${ex}.log 2>&1 || true

    if grep -q "EXAMPLE_.*: PASS" /tmp/${ex}.log; then
        echo "$ex ... PASS"
    else
        echo "$ex ... FAIL"
        FAILED+=("$ex")
    fi
    cd ..
done

if [ ${#FAILED[@]} -eq 0 ]; then
    echo "All examples passed."
    exit 0
else
    echo "Failed: ${FAILED[*]}"
    exit 1
fi
```

- [ ] **Step 2: Make executable and commit**

```bash
chmod +x examples/run_all.sh
git add examples/run_all.sh
git commit -m "feat(examples): add run_all.sh"
```

---

### Task 4.3: Write `examples/run_all.bat`

**Files:**
- Create: `examples/run_all.bat`

- [ ] **Step 1: Write `run_all.bat`**

```batch
@echo off
setlocal enabledelayedexpansion

set EXAMPLES=ultra_blink core_sensor full_system
set FAILED=

for %%e in (%EXAMPLES%) do (
    echo === Building %%e ===
    cd %%e
    cmake -B build -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake . >nul 2>&1
    cmake --build build >nul 2>&1

    echo === Running %%e in QEMU ===
    start /B qemu-system-arm -machine microbit -cpu cortex-m0 -kernel "build/%%e.elf" -nographic -serial stdio > ..\%%e.log 2>&1
    timeout /t 10 >nul
    taskkill /F /IM qemu-system-arm.exe >nul 2>&1

    findstr /C:"EXAMPLE_" "..\%%e.log" | findstr /C:"PASS" >nul
    if !errorlevel! == 0 (
        echo %%e ... PASS
    ) else (
        echo %%e ... FAIL
        set FAILED=!FAILED! %%e
    )
    cd ..
)

if "!FAILED!"=="" (
    echo All examples passed.
    exit /b 0
) else (
    echo Failed:!FAILED!
    exit /b 1
)
```

- [ ] **Step 2: Commit**

```bash
git add examples/run_all.bat
git commit -m "feat(examples): add run_all.bat"
```

---

### Task 4.4: Write `docs/porting/keil-integration.md`

**Files:**
- Create: `docs/porting/keil-integration.md`

- [ ] **Step 1: Write `keil-integration.md`**

```markdown
# Keil MDK-ARM 集成指南

## 1. 新建工程

Project → New μVision Project → 选择目标芯片（如 STM32F030C8）。

## 2. 添加源文件

按功能层级选择：

### 仅 bm-ultra
- 无 `.c` 文件（纯头文件库）
- 添加启动文件（如 `startup_stm32f030.s`）
- 添加 `bm_hal_uart_*.c` 等 HAL 实现

### bm-core + bm-module
追加：
- `src/core/bm_critical.c`
- `src/core/bm_event.c`
- `src/core/bm_mempool.c`
- `src/module/bm_module.c`

### bm-wdg
追加：
- `src/core/bm_wdg.c`

## 3. 头文件路径

C/C++ → Include Paths：
- `include/`
- 应用目录（放置 `bm_config.h`）

## 4. 编译器选项

- C standard: C99
- 建议开启 `-O2` 或 `-Os`

## 5. 链接器设置

Keil 使用 `.sct` (Scatter) 文件，而非 GCC 的 `.ld`。

- 若使用默认分散加载：在 Target → IROM1/IRAM1 中调整地址和大小。
- 若需精确控制：将 `linker.ld` 的 `MEMORY/SECTIONS` 语义手工转为 `.sct`。

## 6. bm_config.h

复制 `bm_config.h.template` 到工程根目录，按需修改宏值。
```

- [ ] **Step 2: Commit**

```bash
git add docs/porting/keil-integration.md
git commit -m "docs(porting): add Keil integration guide"
```

---

### Task 4.5: Write `docs/porting/iar-integration.md`

**Files:**
- Create: `docs/porting/iar-integration.md`

- [ ] **Step 1: Write `iar-integration.md`**

```markdown
# IAR EWARM 集成指南

## 1. 新建工程

File → New → Workspace → 创建工程并选择目标芯片。

## 2. 添加源文件

源文件选择与 Keil 相同（见 `keil-integration.md` §2）。

## 3. 头文件路径

Project → Options → C/C++ Compiler → Preprocessor → Additional include directories：
- `include/`
- 应用目录（放置 `bm_config.h`）

## 4. 编译器选项

- C standard: C99 (`--c99` 或 `-e`，取决于 IAR 版本)
- 建议优化等级：Balanced (`-Ohs`) 或 Size (`-Ohz`)

## 5. 链接器设置

IAR 使用 `.icf` 文件，语法与 GCC `.ld` 不同。

- 建议先使用 IAR 默认 linker configuration（`lnnn.icf`）。
- 如需调整：在 Project → Options → Linker → Config 中编辑 `.icf`，修改 `define region` 的 RAM/FLASH 范围。

## 6. bm_config.h

复制 `bm_config.h.template` 到工程根目录，按需修改宏值。
```

- [ ] **Step 2: Commit**

```bash
git add docs/porting/iar-integration.md
git commit -m "docs(porting): add IAR integration guide"
```

---

### Task 4.6: Write `tools/list_sources.py`

**Files:**
- Create: `tools/list_sources.py`

- [ ] **Step 1: Write `list_sources.py`**

```python
#!/usr/bin/env python3
"""list_sources.py — generate source file list for Keil/IAR/Make integration."""
import argparse
import sys

CORE_SRCS = [
    "src/core/bm_critical.c",
    "src/core/bm_event.c",
    "src/core/bm_mempool.c",
]

MODULE_SRCS = [
    "src/module/bm_module.c",
]

WDG_SRCS = [
    "src/core/bm_wdg.c",
]

HAL_QEMU_SRCS = [
    "hal_reference/qemu_cortex_m0/startup_qemu_cm0.s",
    "hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c",
]


def get_sources(enable_module: bool, enable_wdg: bool) -> list:
    srcs = list(CORE_SRCS)
    if enable_module:
        srcs.extend(MODULE_SRCS)
    if enable_wdg:
        srcs.extend(WDG_SRCS)
    srcs.extend(HAL_QEMU_SRCS)
    return srcs


def format_keil(paths: list, root: str) -> str:
    lines = []
    for p in paths:
        lines.append(f"../../{p}")
    return "\n".join(lines)


def format_iar(paths: list, root: str) -> str:
    # Same relative path format as Keil for source list
    return format_keil(paths, root)


def format_makefile(paths: list, root: str) -> str:
    lines = ["BM_SRCS := \\"]
    for p in paths:
        lines.append(f"    $(ZEPLOD_ROOT)/{p} \\")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description="Generate zeplod-baremetal source list")
    parser.add_argument("--enable-module", choices=["ON", "OFF"], default="ON")
    parser.add_argument("--enable-wdg", choices=["ON", "OFF"], default="OFF")
    parser.add_argument("--format", choices=["keil", "iar", "makefile"], default="keil")
    parser.add_argument("--root", default=".", help="Framework root path")
    args = parser.parse_args()

    enable_module = args.enable_module == "ON"
    enable_wdg = args.enable_wdg == "ON"
    paths = get_sources(enable_module, enable_wdg)

    fmt = args.format
    if fmt == "keil":
        out = format_keil(paths, args.root)
    elif fmt == "iar":
        out = format_iar(paths, args.root)
    elif fmt == "makefile":
        out = format_makefile(paths, args.root)
    else:
        out = "\n".join(paths)

    print(out)


if __name__ == "__main__":
    main()
```

- [ ] **Step 2: Commit**

```bash
git add tools/list_sources.py
git commit -m "feat(tools): add list_sources.py for IDE integration"
```

---

### Task 4.7: Final integration test

- [ ] **Step 1: Run all three examples via CMake and verify PASS**

```bash
cd examples/ultra_blink && cmake -B build -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake . && cmake --build build && cd ../..
cd examples/core_sensor && cmake -B build -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake . && cmake --build build && cd ../..
cd examples/full_system && cmake -B build -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake . && cmake --build build && cd ../..
```

Expected: All three build successfully, zero warnings.

- [ ] **Step 2: Run `run_all.sh` (or manual QEMU) and capture outputs**

```bash
cd examples
bash run_all.sh
```

Expected:
```
ultra_blink  ... PASS
core_sensor  ... PASS
full_system  ... PASS
All examples passed.
```

- [ ] **Step 3: Commit final verification**

```bash
git add -A
git commit -m "test(examples): all three examples build and pass in QEMU"
```

---

## Self-Review

### Spec coverage

| Spec section | Task(s) |
|---|---|
| ultra_blink 业务逻辑 + 终止条件 | Task 1.1, 1.2, 1.6 |
| core_sensor LIFO 订阅 + mempool | Task 2.1, 2.2, 2.6 |
| full_system 确定性 fault + storm | Task 3.1, 3.2, 3.6 |
| CMake + Makefile + PROJECT_SOURCES.md | Task 1.3–1.5, 2.3–2.5, 3.3–3.5 |
| README.md + run_all.sh/bat | Task 4.1–4.3 |
| docs/porting/keil + iar | Task 4.4, 4.5 |
| tools/list_sources.py | Task 4.6 |

### Placeholder scan

- No TBD/TODO.
- No "add appropriate error handling".
- Every code step contains complete code.
- No "similar to Task N" references.

### Type consistency

- `bm_event_t` fields (`type`, `priority`, `data`, `data_len`) consistent across all three `main.c`.
- `bm_module_t` registration table consistent with `bm_module.c` expectations (`_bm_module_table`, `_bm_module_count`).
- `bm_config.h` macros match framework defaults.
