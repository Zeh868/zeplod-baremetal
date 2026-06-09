# interrupt_demo Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create `examples/interrupt_demo/` demonstrating multi-IRQ event publishing from ISR, plus `run_single` scripts and update existing `run_all` scripts.

**Architecture:** SysTick (10Hz) and TIMER1 (2Hz) both publish events from their ISRs via `bm_event_publish_event_from_isr()`. Main loop is pure `bm_event_process()`. `startup_qemu_cm0.s` vector table is extended to include all IRQ vectors pointing to `Default_Handler` so any example can safely enable interrupts.

**Tech Stack:** C99, ARM GCC, QEMU, PowerShell/Bash, CMake, Makefile

---

## File Structure

```
examples/
├── interrupt_demo/
│   ├── bm_config.h              (new)
│   ├── main.c                   (new)
│   ├── CMakeLists.txt           (new)
│   ├── Makefile                 (new)
│   └── PROJECT_SOURCES.md       (new)
├── run_all.ps1                  (modify)
├── run_all.sh                   (modify)
├── run_all.bat                  (modify)
├── run_single.ps1               (new)
├── run_single.sh                (new)
└── run_single.bat               (new)
hal_reference/qemu_cortex_m0/
    ├── startup_qemu_cm0.s       (modify)
    └── bm_hal_timer_qemu.c      (modify)
```

---

## Task 1: Extend `startup_qemu_cm0.s` vector table

**Files:**
- Modify: `hal_reference/qemu_cortex_m0/startup_qemu_cm0.s`

- [ ] **Step 1: Rewrite startup assembly with full vector table**

Current vector table only has 3 entries after `_estack`. Extend to full Cortex-M0 vector table (16 system exceptions + 32 IRQs), all pointing to `Default_Handler`.

```asm
    .syntax unified
    .cpu cortex-m0
    .thumb

    .section .text.Reset_Handler
    .globl Reset_Handler
    .type Reset_Handler, %function
Reset_Handler:
    ldr r0, =_estack
    mov sp, r0
    bl SystemInit
    bl main
    b .

    .section .text.Default_Handler
    .globl Default_Handler
    .type Default_Handler, %function
Default_Handler:
    b .

    .section .isr_vector
    .globl _vector_table
_vector_table:
    .word _estack
    .word Reset_Handler
    .word Default_Handler /* NMI */
    .word Default_Handler /* HardFault */
    .word 0               /* Reserved */
    .word 0               /* Reserved */
    .word 0               /* Reserved */
    .word 0               /* Reserved */
    .word 0               /* Reserved */
    .word 0               /* Reserved */
    .word 0               /* Reserved */
    .word Default_Handler /* SVC */
    .word 0               /* Reserved */
    .word 0               /* Reserved */
    .word Default_Handler /* PendSV */
    .word Default_Handler /* SysTick */
    /* IRQ0 ~ IRQ31 */
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
    .word Default_Handler
```

- [ ] **Step 2: Verify existing examples still build**

```bash
cd examples/ultra_blink && cmake --build build
cd examples/core_sensor && cmake --build build
cd examples/full_system && cmake --build build
```

Expected: All three build successfully.

- [ ] **Step 3: Commit**

```bash
git add hal_reference/qemu_cortex_m0/startup_qemu_cm0.s
git commit -m "feat(qemu): extend vector table with all IRQ handlers"
```

---

## Task 2: Add user callback to `bm_hal_timer_qemu.c`

**Files:**
- Modify: `hal_reference/qemu_cortex_m0/bm_hal_timer_qemu.c`

- [ ] **Step 1: Add callback pointer and setter**

```c
#include "bm_hal_timer.h"

#define SYSTICK_BASE  0xE000E010
#define SYSTICK_CTRL  (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYSTICK_LOAD  (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYSTICK_VAL   (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))

static volatile uint32_t _ticks = 0;
static void (*_tick_cb)(void) = NULL;

void SysTick_Handler(void) {
    _ticks++;
    if (_tick_cb) {
        _tick_cb();
    }
}

int bm_hal_timer_init(uint32_t freq_hz) {
    uint32_t reload = 1000;
    SYSTICK_LOAD = reload - 1;
    SYSTICK_VAL = 0;
    SYSTICK_CTRL = 0x7;
    (void)freq_hz;
    return 0;
}

uint32_t bm_hal_timer_get_ticks(void) { return _ticks; }
uint32_t bm_hal_timer_get_freq(void) { return 1000; }
void bm_hal_timer_set_callback(void (*cb)(void)) { _tick_cb = cb; }
```

- [ ] **Step 2: Verify existing examples still build**

```bash
cd examples/full_system && cmake --build build
```

Expected: Build succeeds.

- [ ] **Step 3: Commit**

```bash
git add hal_reference/qemu_cortex_m0/bm_hal_timer_qemu.c
git commit -m "feat(qemu/timer): add user tick callback support"
```

---

## Task 3: Create `examples/interrupt_demo/bm_config.h`

**Files:**
- Create: `examples/interrupt_demo/bm_config.h`

- [ ] **Step 1: Write config**

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
git add examples/interrupt_demo/bm_config.h
git commit -m "feat(examples): add interrupt_demo config"
```

---

## Task 4: Create `examples/interrupt_demo/main.c`

**Files:**
- Create: `examples/interrupt_demo/main.c`

- [ ] **Step 1: Write main.c**

```c
#include "bm_core.h"
#include "bm_hal_uart.h"
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
        bm_event_t ev = {
            .type = EVENT_SAMPLE,
            .priority = 1,
            .data = &temp,
            .data_len = sizeof(temp)
        };
        bm_event_publish_event_from_isr(&ev);
        g_sample_count++;
    }
}

/* SysTick user callback */
static void on_systick(void) {
    uint32_t tick = g_tick_count + 1;
    bm_event_t ev = {
        .type = EVENT_TICK,
        .priority = 0,
        .data = &tick,
        .data_len = sizeof(tick)
    };
    bm_event_publish_event_from_isr(&ev);
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

    bm_event_subscriber_id_t id_stats, id_tick;
    bm_event_subscribe(EVENT_TICK, stats_cb, NULL, &id_stats);
    bm_event_subscribe(EVENT_TICK, tick_cb, NULL, &id_tick);
    bm_event_subscribe(EVENT_SAMPLE, sample_cb, NULL, &(bm_event_subscriber_id_t){0});

    bm_hal_timer_set_callback(on_systick);
    bm_hal_timer_init(0);
    timer1_init();

    uart_print("IRQ Demo: interrupt_demo\n");

    uint8_t pass_sent = 0;
    uint8_t stats_sent = 0;

    while (1) {
        bm_event_process(8);

        if (g_tick_count >= 20 && !stats_sent) {
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
```

- [ ] **Step 2: Commit**

```bash
git add examples/interrupt_demo/main.c
git commit -m "feat(examples): add interrupt_demo main"
```

---

## Task 5: Create `examples/interrupt_demo/CMakeLists.txt`

**Files:**
- Create: `examples/interrupt_demo/CMakeLists.txt`

- [ ] **Step 1: Write CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.20)
project(interrupt_demo C ASM)

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

add_executable(interrupt_demo.elf
    main.c
    ${ZEPLOD_ROOT}/src/core/bm_critical.c
    ${ZEPLOD_ROOT}/src/core/bm_event.c
    ${ZEPLOD_ROOT}/src/core/bm_mempool.c
    ${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/startup_qemu_cm0.s
    ${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/crt0_qemu.c
    ${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c
    ${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/bm_hal_timer_qemu.c
)
```

- [ ] **Step 2: Commit**

```bash
git add examples/interrupt_demo/CMakeLists.txt
git commit -m "feat(examples): add interrupt_demo CMakeLists.txt"
```

---

## Task 6: Create `examples/interrupt_demo/Makefile`

**Files:**
- Create: `examples/interrupt_demo/Makefile`

- [ ] **Step 1: Write Makefile**

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
        $(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/startup_qemu_cm0.s \
        $(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/crt0_qemu.c \
        $(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c \
        $(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/bm_hal_timer_qemu.c

TARGET := interrupt_demo.elf

.PHONY: all clean qemu

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $@

qemu: $(TARGET)
	qemu-system-arm -machine microbit -cpu cortex-m0 -kernel $(TARGET) --semihosting -nographic -serial stdio

clean:
	rm -f $(TARGET)
```

- [ ] **Step 2: Commit**

```bash
git add examples/interrupt_demo/Makefile
git commit -m "feat(examples): add interrupt_demo Makefile"
```

---

## Task 7: Create `examples/interrupt_demo/PROJECT_SOURCES.md`

**Files:**
- Create: `examples/interrupt_demo/PROJECT_SOURCES.md`

- [ ] **Step 1: Write PROJECT_SOURCES.md**

```markdown
# interrupt_demo — Keil / IAR / GCC 源文件清单

## GCC / QEMU

### 源文件
| 文件 | 说明 |
|------|------|
| `main.c` | 本示例入口（多中断 + ISR 事件发布） |
| `../../src/core/bm_critical.c` | 临界区 |
| `../../src/core/bm_event.c` | 事件系统 |
| `../../src/core/bm_mempool.c` | 内存池 |
| `../../hal_reference/qemu_cortex_m0/startup_qemu_cm0.s` | 启动文件（含完整向量表） |
| `../../hal_reference/qemu_cortex_m0/crt0_qemu.c` | C runtime init |
| `../../hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c` | UART HAL |
| `../../hal_reference/qemu_cortex_m0/bm_hal_timer_qemu.c` | Timer HAL（含 SysTick 回调） |

### 头文件路径
- `../../include`
- `.`

### 链接器
- `../../hal_reference/qemu_cortex_m0/linker.ld`

## Keil / IAR 真机移植

- 源文件同上（替换启动文件为目标芯片版本，移除 `crt0_qemu.c`）。
- 链接器使用 Keil `.sct` 或 IAR `.icf`，参考 `docs/porting/`。
```

- [ ] **Step 2: Commit**

```bash
git add examples/interrupt_demo/PROJECT_SOURCES.md
git commit -m "docs(examples): add interrupt_demo PROJECT_SOURCES.md"
```

---

## Task 8: Build and verify interrupt_demo

- [ ] **Step 1: Build with CMake**

```bash
cd examples/interrupt_demo
cmake -G "Unix Makefiles" -B build -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake .
cmake --build build
```

Expected: `interrupt_demo.elf` generated, zero warnings.

- [ ] **Step 2: Run in QEMU and check output**

```bash
qemu-system-arm -machine microbit -cpu cortex-m0 -kernel build/interrupt_demo.elf --semihosting -nographic -serial stdio
```

Expected output (after ~3s):
```
IRQ Demo: interrupt_demo
[TICK] count=1
[TICK] count=2
[TICK] count=3
[TICK] count=4
[TICK] count=5
[SAMPLE] temp=25
[TICK] count=6
[TICK] count=7
[TICK] count=8
[TICK] count=9
[TICK] count=10
[SAMPLE] temp=26
[STATS] ticks=20, samples=2
EXAMPLE_IRQ: PASS
```

Press `Ctrl+A` then `X` to quit QEMU.

- [ ] **Step 3: Build with Makefile**

```bash
cd examples/interrupt_demo
make
```

Expected: `interrupt_demo.elf` generated.

- [ ] **Step 4: Commit verification**

```bash
git add -A
git commit -m "test(examples): verify interrupt_demo builds and runs in QEMU"
```

---

## Task 9: Update `run_all` scripts to include interrupt_demo

**Files:**
- Modify: `examples/run_all.ps1`
- Modify: `examples/run_all.sh`
- Modify: `examples/run_all.bat`

- [ ] **Step 1: Update run_all.ps1**

Change line 2 from:
```powershell
$Examples = @('ultra_blink', 'core_sensor', 'full_system')
```
to:
```powershell
$Examples = @('ultra_blink', 'core_sensor', 'full_system', 'interrupt_demo')
```

- [ ] **Step 2: Update run_all.sh**

Change line 4 from:
```bash
EXAMPLES=(ultra_blink core_sensor full_system)
```
to:
```bash
EXAMPLES=(ultra_blink core_sensor full_system interrupt_demo)
```

- [ ] **Step 3: Update run_all.bat**

Change line 3 from:
```batch
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0run_all.ps1"
```
No change needed (run_all.bat delegates to run_all.ps1).

- [ ] **Step 4: Commit**

```bash
git add examples/run_all.ps1 examples/run_all.sh
git commit -m "feat(examples): add interrupt_demo to run_all scripts"
```

---

## Task 10: Create `run_single` scripts

**Files:**
- Create: `examples/run_single.ps1`
- Create: `examples/run_single.sh`
- Create: `examples/run_single.bat`

- [ ] **Step 1: Write run_single.ps1**

```powershell
param([Parameter(Mandatory=$true)][string]$Example)

$ErrorActionPreference = 'Stop'
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$RootDir = Join-Path $ScriptDir '..'

$ExampleDir = Join-Path $ScriptDir $Example
if (-not (Test-Path $ExampleDir)) {
    Write-Error "Example '$Example' not found in $ScriptDir"
    exit 1
}

Set-Location $ExampleDir

Write-Host "=== Building $Example ==="
cmake -G 'Unix Makefiles' -B build -DCMAKE_TOOLCHAIN_FILE="$RootDir\cmake\toolchain-arm-none-eabi.cmake" . 2>&1 | Out-Null
cmake --build build 2>&1 | Out-Null

Write-Host "=== Running $Example in QEMU (interactive) ==="
Write-Host "Press Ctrl+A then X to quit QEMU"
Write-Host ""

$elfPath = Join-Path (Get-Location) "build\$Example.elf"
qemu-system-arm -machine microbit -cpu cortex-m0 -kernel $elfPath --semihosting -nographic -serial stdio
```

- [ ] **Step 2: Write run_single.sh**

```bash
#!/bin/bash
set -e

EXAMPLE="$1"
if [ -z "$EXAMPLE" ]; then
    echo "Usage: $0 <example_name>"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$SCRIPT_DIR/.."

EXAMPLE_DIR="$SCRIPT_DIR/$EXAMPLE"
if [ ! -d "$EXAMPLE_DIR" ]; then
    echo "Example '$EXAMPLE' not found"
    exit 1
fi

cd "$EXAMPLE_DIR"

echo "=== Building $EXAMPLE ==="
cmake -G "Unix Makefiles" -B build -DCMAKE_TOOLCHAIN_FILE="$ROOT_DIR/cmake/toolchain-arm-none-eabi.cmake" . >/dev/null 2>&1
cmake --build build >/dev/null 2>&1

echo "=== Running $EXAMPLE in QEMU (interactive) ==="
echo "Press Ctrl+A then X to quit QEMU"
echo ""

qemu-system-arm -machine microbit -cpu cortex-m0 \
    -kernel "build/${EXAMPLE}.elf" --semihosting -nographic -serial stdio
```

- [ ] **Step 3: Make run_single.sh executable**

```bash
chmod +x examples/run_single.sh
```

- [ ] **Step 4: Write run_single.bat**

```batch
@echo off
if "%~1"=="" (
    echo Usage: run_single.bat ^<example_name^>
    exit /b 1
)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0run_single.ps1" %1
exit /b %errorlevel%
```

- [ ] **Step 5: Commit**

```bash
git add examples/run_single.ps1 examples/run_single.sh examples/run_single.bat
git commit -m "feat(examples): add run_single scripts for interactive single-example launch"
```

---

## Task 11: Final integration test

- [ ] **Step 1: Run all four examples via run_all.ps1**

```bash
cd examples
bash run_all.sh
```

Expected:
```
ultra_blink ... PASS
core_sensor ... PASS
full_system ... PASS
interrupt_demo ... PASS
All examples passed.
```

- [ ] **Step 2: Test run_single.ps1**

```powershell
cd examples
.\run_single.ps1 interrupt_demo
```

Expected: QEMU starts in foreground, outputs `[TICK]` and `[SAMPLE]` lines. Press `Ctrl+A X` to exit.

- [ ] **Step 3: Commit final verification**

```bash
git add -A
git commit -m "test(examples): all four examples pass, run_single works"
```

---

## Self-Review

### Spec coverage

| Spec section | Task |
|---|---|
| SysTick + TIMER1 双中断 | Task 1, 2, 4 |
| ISR 中发布事件 | Task 4 (main.c) |
| 纯事件驱动主循环 | Task 4 (main.c) |
| 扩展向量表 | Task 1 |
| run_all 脚本更新 | Task 9 |
| run_single 脚本 | Task 10 |
| CMake + Makefile | Task 5, 6 |
| PROJECT_SOURCES.md | Task 7 |

### Placeholder scan

- No TBD/TODO.
- All code blocks contain complete code.
- No "similar to Task N" references.

### Type consistency

- `bm_event_publish_event_from_isr` used in ISR contexts.
- `bm_event_publish_event` used in main loop.
- `g_tick_count`, `g_sample_count` types consistent (`volatile uint32_t`).
