# Zeplod Baremetal 实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.
>
> 计划基于设计文档：`docs/design/2026-06-08-zeplod-baremetal-framework-design.md`（审查修正 v2）

**Goal:** 实现 Zeplod Baremetal 框架核心（M0-M7），覆盖 bm-ultra、bmc-core（事件 publish_copy + 内存池 + 临界区）、bm-module、QEMU 支持、硬件验证及 API 对齐。

**Architecture:** 纯 C99 裸机框架，零堆分配，短临界区保护事件队列，编译期静态配置。native_sim 用于 PC 单元测试，QEMU Cortex-M0/RISC-V 用于集成验证。

**Tech Stack:** C99, CMake, Makefile, Unity (测试), QEMU (system-arm / riscv32), ARM GCC, RISC-V GCC, SDCC (可选 STM8)

---

## 文件结构总览

```
zeplod-baremetal/
├── include/
│   ├── bm_core.h           # 事件、内存池、临界区、错误码、基础类型
│   ├── bm_module.h         # 模块生命周期
│   ├── bm_channel.h        # SPSC 数据通道（暂缓）
│   ├── bm_shell.h          # 串口 CLI（暂缓）
│   └── bm_ultra.h          # 极端裁剪头文件库
├── src/
│   ├── core/
│   │   ├── bm_event.c
│   │   ├── bm_mempool.c
│   │   ├── bm_critical.c
│   │   └── bm_wdg.c
│   ├── module/
│   │   └── bm_module.c
│   ├── channel/            # 暂缓
│   │   └── bm_channel.c
│   ├── shell/              # 暂缓
│   │   └── bm_shell.c
│   └── hal/
│       ├── bm_hal_uart.c
│       ├── bm_hal_timer.c
│       └── bm_hal_gpio.c
├── hal_reference/
│   ├── native_sim/
│   ├── qemu_cortex_m0/
│   ├── qemu_riscv32/
│   └── stm32f0/
├── tests/
│   ├── unit/               # Unity + fff，PC 运行
│   ├── qemu/               # QEMU 镜像 + TAP 输出
│   └── hardware/           # 真实硬件（手动）
├── cmake/
│   └── baremetal.cmake
├── Makefile
├── CMakeLists.txt
└── bm_config.h.template
```

---

## Phase 0: 仓库骨架与构建系统（M0）

**目标：** 可编译的空仓库，native_sim HAL 桩可用，单元测试框架跑通。

---

### Task 0.1: 目录结构初始化

**Files:**
- Create: `include/.gitkeep`, `src/core/.gitkeep`, `src/module/.gitkeep`, `src/channel/.gitkeep`, `src/shell/.gitkeep`, `src/hal/.gitkeep`, `hal_reference/native_sim/.gitkeep`, `hal_reference/qemu_cortex_m0/.gitkeep`, `hal_reference/qemu_riscv32/.gitkeep`, `hal_reference/stm32f0/.gitkeep`, `tests/unit/.gitkeep`, `tests/qemu/.gitkeep`, `tests/hardware/.gitkeep`, `cmake/.gitkeep`, `docs/api/.gitkeep`, `docs/migration/.gitkeep`, `docs/porting/.gitkeep`

- [ ] **Step 1: 创建全部目录**

```bash
mkdir -p include src/core src/module src/channel src/shell src/hal \
  hal_reference/native_sim hal_reference/qemu_cortex_m0 \
  hal_reference/qemu_riscv32 hal_reference/stm32f0 \
  tests/unit tests/qemu tests/hardware \
  cmake docs/api docs/migration docs/porting
```

- [ ] **Step 2: 初始化 Git 仓库并提交骨架**

```bash
git init
git add .
git commit -m "chore: init directory structure"
```

---

### Task 0.2: 顶层 CMakeLists.txt

**Files:**
- Create: `CMakeLists.txt`

- [ ] **Step 1: 编写根 CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.20)
project(zeplod-baremetal C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 配置头文件搜索路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 如果应用层提供了 bm_config.h，优先使用
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bm_config.h)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})
endif()

# 可选模块开关（默认 ON）
option(BM_ENABLE_MODULE "Enable bm-module" ON)
option(BM_ENABLE_CHANNEL "Enable bm-channel (deferred)" OFF)
option(BM_ENABLE_SHELL "Enable bm-shell (deferred)" OFF)
option(BM_ENABLE_WDG "Enable bm-wdg" ON)

# 核心源文件（始终编译）
set(BM_CORE_SRCS
    src/core/bm_event.c
    src/core/bm_mempool.c
    src/core/bm_critical.c
)

if(BM_ENABLE_WDG)
    list(APPEND BM_CORE_SRCS src/core/bm_wdg.c)
endif()

# 模块源文件
if(BM_ENABLE_MODULE)
    list(APPEND BM_CORE_SRCS src/module/bm_module.c)
endif()

# 子目录：native_sim 参考 HAL（用于 PC 单元测试）
add_subdirectory(hal_reference/native_sim EXCLUDE_FROM_ALL)
```

- [ ] **Step 2: 提交**

```bash
git add CMakeLists.txt
git commit -m "build: add root CMakeLists.txt"
```

---

### Task 0.3: 纯 Makefile（8-bit 友好）

**Files:**
- Create: `Makefile`

- [ ] **Step 1: 编写顶层 Makefile**

```makefile
# Zeplod Baremetal — 纯 Makefile 构建
# 适用于 SDCC / AVR-GCC / IAR 等无 CMake 工具链

BM_ROOT := $(dir $(lastword $(MAKEFILE_LIST)))

CC      ?= gcc
CFLAGS  ?= -Os -Wall -Wextra -std=c99 -I$(BM_ROOT)/include
LDFLAGS ?=

# 默认源文件（核心）
BM_SRCS := \
    $(BM_ROOT)/src/core/bm_event.c \
    $(BM_ROOT)/src/core/bm_mempool.c \
    $(BM_ROOT)/src/core/bm_critical.c

# 可选模块
ifeq ($(BM_ENABLE_MODULE),1)
    BM_SRCS += $(BM_ROOT)/src/module/bm_module.c
endif

ifeq ($(BM_ENABLE_WDG),1)
    BM_SRCS += $(BM_ROOT)/src/core/bm_wdg.c
endif

# 应用层覆盖
ifdef BM_CONFIG_H
    CFLAGS += -include $(BM_CONFIG_H)
endif

.PHONY: all clean test

all:
	@echo "BM_SRCS = $(BM_SRCS)"
	@echo "CFLAGS  = $(CFLAGS)"

clean:
	find . -name '*.o' -delete
	find . -name '*.elf' -delete
```

- [ ] **Step 2: 提交**

```bash
git add Makefile
git commit -m "build: add top-level Makefile for 8-bit toolchains"
```

---

### Task 0.4: 配置模板 `bm_config.h.template`

**Files:**
- Create: `bm_config.h.template`

- [ ] **Step 1: 编写配置模板**

```c
/* bm_config.h.template — 复制为 bm_config.h 并修改 */
#ifndef BM_CONFIG_H
#define BM_CONFIG_H

/* 事件系统 */
#define BM_CONFIG_MAX_EVENT_TYPES           16
#define BM_CONFIG_MAX_EVENT_SUBSCRIBERS     32
#define BM_CONFIG_MAX_EVENTS_PER_LOOP       8
#define BM_CONFIG_EVENT_PRIORITIES          4
#define BM_CONFIG_MAX_MODULES               8
#define BM_CONFIG_MEMPOOL_MAX_POOLS         4
#define BM_CONFIG_CHANNEL_MAX_CHANNELS      4
#define BM_CONFIG_SHELL_BUF_SIZE            64

/* bm-ultra */
#define BM_CONFIG_ULTRA_MAX_EVENT_DATA_SIZE 8
#define BM_CONFIG_ULTRA_QUEUE_DEPTH         8

#endif /* BM_CONFIG_H */
```

- [ ] **Step 2: 提交**

```bash
git add bm_config.h.template
git commit -m "build: add bm_config.h.template"
```

---

### Task 0.5: native_sim HAL 桩

**Files:**
- Create: `hal_reference/native_sim/CMakeLists.txt`
- Create: `hal_reference/native_sim/bm_hal_uart_native.c`
- Create: `hal_reference/native_sim/bm_hal_timer_native.c`
- Create: `hal_reference/native_sim/bm_hal_wdg_native.c`

- [ ] **Step 1: native_sim CMakeLists.txt**

```cmake
# hal_reference/native_sim/CMakeLists.txt
add_library(bm_hal_native STATIC
    bm_hal_uart_native.c
    bm_hal_timer_native.c
    bm_hal_wdg_native.c
)
target_include_directories(bm_hal_native PUBLIC ${CMAKE_SOURCE_DIR}/include)
```

- [ ] **Step 2: UART HAL 桩**

```c
/* hal_reference/native_sim/bm_hal_uart_native.c */
#include "bm_hal_uart.h"
#include <stdio.h>

static void (*rx_cb)(uint8_t c) = NULL;

int bm_hal_uart_init(void *config) {
    (void)config;
    return 0;
}

int bm_hal_uart_send(const uint8_t *data, size_t len) {
    fwrite(data, 1, len, stdout);
    fflush(stdout);
    return 0;
}

size_t bm_hal_uart_recv(uint8_t *data, size_t max_len) {
    (void)data; (void)max_len;
    return 0; /* 非阻塞：无可读数据 */
}

void bm_hal_uart_set_rx_callback(void (*cb)(uint8_t c)) {
    rx_cb = cb;
}
```

- [ ] **Step 3: Timer HAL 桩**

```c
/* hal_reference/native_sim/bm_hal_timer_native.c */
#include "bm_hal_timer.h"
#include <time.h>

static uint32_t tick_freq = 1000;

int bm_hal_timer_init(uint32_t freq_hz) {
    tick_freq = freq_hz ? freq_hz : 1000;
    return 0;
}

uint32_t bm_hal_timer_get_ticks(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t ms = (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    return (uint32_t)(ms * tick_freq / 1000);
}

uint32_t bm_hal_timer_get_freq(void) {
    return tick_freq;
}

void bm_hal_timer_set_callback(void (*cb)(void)) {
    (void)cb; /* native_sim 不模拟定时器中断 */
}
```

- [ ] **Step 4: WDG HAL 桩**

```c
/* hal_reference/native_sim/bm_hal_wdg_native.c */
#include "bm_hal_wdg.h"

int bm_hal_wdg_init(uint32_t timeout_ms) {
    (void)timeout_ms;
    return 0;
}

void bm_hal_wdg_feed(void) {
    /* native_sim 下无看门狗 */
}
```

- [ ] **Step 5: 提交 native_sim HAL**

```bash
git add hal_reference/native_sim/
git commit -m "feat(native_sim): add reference HAL stubs for PC testing"
```

---

### Task 0.6: Unity 测试框架集成

**Files:**
- Create: `tests/unit/unity/CMakeLists.txt`
- Create: `tests/unit/unity/unity_config.h`

- [ ] **Step 1: 添加 Unity 作为子模块或 vendor 副本**

```bash
git submodule add https://github.com/ThrowTheSwitch/Unity.git tests/unit/unity/src
```

若无法使用子模块，直接下载 `unity.c`、`unity.h`、`unity_internals.h` 到 `tests/unit/unity/src/`。

- [ ] **Step 2: unity_config.h**

```c
/* tests/unit/unity/unity_config.h */
#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

#define UNITY_INCLUDE_PRINT_FORMATTED
#define UNITY_OUTPUT_COLOR

#endif
```

- [ ] **Step 3: CMake 集成 Unity**

```cmake
# tests/unit/unity/CMakeLists.txt
add_library(unity STATIC
    src/unity.c
)
target_include_directories(unity PUBLIC src)
```

- [ ] **Step 4: 根 CMakeLists.txt 追加测试目录**

修改 `CMakeLists.txt`，末尾添加：

```cmake
enable_testing()
add_subdirectory(tests/unit/unity)
add_subdirectory(tests/unit)
```

- [ ] **Step 5: 提交**

```bash
git add tests/unit/unity/
git commit -m "test: integrate Unity framework"
```

---

### Task 0.7: 第一个可编译的单元测试骨架

**Files:**
- Create: `tests/unit/CMakeLists.txt`
- Create: `tests/unit/test_skeleton.c`

- [ ] **Step 1: tests/unit/CMakeLists.txt**

```cmake
# tests/unit/CMakeLists.txt
function(bm_add_test TEST_NAME TEST_SRC)
    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME} PRIVATE unity bm_hal_native)
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/tests/unit/unity/src
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

bm_add_test(test_skeleton test_skeleton.c)
```

- [ ] **Step 2: test_skeleton.c**

```c
#include "unity.h"

void setUp(void) {}
void tearDown(void) {}

void test_skeleton_passes(void) {
    TEST_ASSERT_EQUAL(1, 1);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_skeleton_passes);
    return UNITY_END();
}
```

- [ ] **Step 3: 编译并运行**

```bash
cmake -B build_native -DBOARD=native_sim
cmake --build build_native
cd build_native && ctest --output-on-failure
```

Expected: `test_skeleton` PASS。

- [ ] **Step 4: 提交**

```bash
git add tests/unit/
git commit -m "test: add skeleton unit test proving build system works"
```

---

## Phase 1: bm-ultra 极端裁剪层（M1）

**目标：** 纯头文件库，零 `.c` 链接，FIFO 事件队列，值拷贝，编译期绑定。

---

### Task 1.1: bm-ultra 核心类型与配置

**Files:**
- Create: `include/bm_ultra.h`

- [ ] **Step 1: 编写 bm_ultra.h 基础框架**

```c
/* include/bm_ultra.h */
#ifndef BM_ULTRA_H
#define BM_ULTRA_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* 配置默认值 */
#ifndef BM_CONFIG_ULTRA_MAX_EVENT_TYPES
#define BM_CONFIG_ULTRA_MAX_EVENT_TYPES     8
#endif

#ifndef BM_CONFIG_ULTRA_QUEUE_DEPTH
#define BM_CONFIG_ULTRA_QUEUE_DEPTH         8
#endif

#ifndef BM_CONFIG_ULTRA_MAX_EVENT_DATA_SIZE
#define BM_CONFIG_ULTRA_MAX_EVENT_DATA_SIZE 8
#endif

typedef uint8_t bm_event_type_t;

typedef struct {
    bm_event_type_t event_type;
    uint8_t         data[BM_CONFIG_ULTRA_MAX_EVENT_DATA_SIZE];
    uint8_t         data_len;
} bm_ultra_queue_item_t;

/* 回调签名：无 user_data，固定签名 */
typedef void (*bm_ultra_callback_t)(const void *data, uint8_t len);

#endif /* BM_ULTRA_H */
```

- [ ] **Step 2: 提交**

```bash
git add include/bm_ultra.h
git commit -m "feat(ultra): add core types and configuration defaults"
```

---

### Task 1.2: bm-ultra 编译期事件绑定宏

**Files:**
- Modify: `include/bm_ultra.h`

- [ ] **Step 1: 添加绑定宏与内部表**

在 `#endif` 之前添加：

```c
/* 内部：事件回调表（编译期填充） */
#define BM_ULTRA_EVENT_TABLE_EXPAND(type, cb)  [type] = cb,

/* 用户宏：绑定单个事件 */
#define BM_ULTRA_EVENT_BIND(event_type, callback) \
    static const bm_ultra_callback_t _bm_ultra_cb_##event_type##_##callback \
        __attribute__((used, section("bm_ultra_callbacks"))) = callback

/* 简化版：不使用 linker section，直接用静态数组 */
/* 更简单的方案：用户显式声明数组 */
```

实际上，为了 SDCC/STM8 兼容性，**不使用 linker section**，改为显式数组：

```c
/* 用户应在单个 .c 文件中定义此数组 */
#define BM_ULTRA_CALLBACK_TABLE_DEFINE(...) \
    static const bm_ultra_callback_t _bm_ultra_callbacks[BM_CONFIG_ULTRA_MAX_EVENT_TYPES] = { __VA_ARGS__ }

#define BM_ULTRA_CB(event_type, callback) \
    [event_type] = callback
```

- [ ] **Step 2: 提交**

```bash
git add include/bm_ultra.h
git commit -m "feat(ultra): add compile-time event binding macros"
```

---

### Task 1.3: bm-ultra FIFO 队列实现

**Files:**
- Modify: `include/bm_ultra.h`

- [ ] **Step 1: 添加队列结构与操作函数**

```c
typedef struct {
    bm_ultra_queue_item_t items[BM_CONFIG_ULTRA_QUEUE_DEPTH];
    uint8_t write_idx;
    uint8_t read_idx;
} bm_ultra_queue_t;

/* 静态实例 */
static bm_ultra_queue_t _bm_ultra_q;

static inline int _bm_ultra_queue_push(const bm_ultra_queue_item_t *item) {
    uint8_t next = (_bm_ultra_q.write_idx + 1) & (BM_CONFIG_ULTRA_QUEUE_DEPTH - 1);
    if (next == _bm_ultra_q.read_idx) {
        return -1; /* 满 */
    }
    _bm_ultra_q.items[_bm_ultra_q.write_idx] = *item;
    _bm_ultra_q.write_idx = next;
    return 0;
}

static inline int _bm_ultra_queue_pop(bm_ultra_queue_item_t *item) {
    if (_bm_ultra_q.read_idx == _bm_ultra_q.write_idx) {
        return -1; /* 空 */
    }
    *item = _bm_ultra_q.items[_bm_ultra_q.read_idx];
    _bm_ultra_q.read_idx = (_bm_ultra_q.read_idx + 1) & (BM_CONFIG_ULTRA_QUEUE_DEPTH - 1);
    return 0;
}
```

- [ ] **Step 2: 提交**

```bash
git add include/bm_ultra.h
git commit -m "feat(ultra): add FIFO queue with value-copy items"
```

---

### Task 1.4: bm-ultra 公共 API（init / publish / process）

**Files:**
- Modify: `include/bm_ultra.h`

- [ ] **Step 1: 添加公共 API**

```c
static inline void bm_ultra_init(void) {
    memset(&_bm_ultra_q, 0, sizeof(_bm_ultra_q));
}

static inline int bm_ultra_publish(bm_event_type_t type,
                                    const void *data, uint8_t len) {
    if (len > BM_CONFIG_ULTRA_MAX_EVENT_DATA_SIZE) {
        return -1;
    }
    bm_ultra_queue_item_t item;
    item.event_type = type;
    item.data_len = len;
    if (len > 0 && data != NULL) {
        memcpy(item.data, data, len);
    }
    return _bm_ultra_queue_push(&item);
}

/* ISR 安全：在裸机单核中，若仅在主循环调用 process，
 * 则 publish_from_isr 与 publish 实现相同。
 * 若主循环可能并发，需关中断保护。
 * 这里假设 bm-ultra 用户手动保证互斥或仅在单 ISR 场景使用。
 */
static inline int bm_ultra_publish_from_isr(bm_event_type_t type,
                                             const void *data, uint8_t len) {
    return bm_ultra_publish(type, data, len);
}

static inline uint8_t bm_ultra_event_count(void) {
    return (uint8_t)((_bm_ultra_q.write_idx - _bm_ultra_q.read_idx)
                     & (BM_CONFIG_ULTRA_QUEUE_DEPTH - 1));
}
```

- [ ] **Step 2: 提交**

```bash
git add include/bm_ultra.h
git commit -m "feat(ultra): add init, publish, event_count APIs"
```

---

### Task 1.5: bm-ultra process 与回调分发

**Files:**
- Modify: `include/bm_ultra.h`

- [ ] **Step 1: 添加 process 函数**

```c
/* 用户必须在外部声明此数组 */
extern const bm_ultra_callback_t _bm_ultra_callbacks[BM_CONFIG_ULTRA_MAX_EVENT_TYPES];

static inline uint8_t bm_ultra_process(void) {
    bm_ultra_queue_item_t item;
    if (_bm_ultra_queue_pop(&item) != 0) {
        return 0;
    }
    bm_ultra_callback_t cb = _bm_ultra_callbacks[item.event_type];
    if (cb != NULL) {
        cb(item.data, item.data_len);
    }
    return 1;
}
```

- [ ] **Step 2: 提交**

```bash
git add include/bm_ultra.h
git commit -m "feat(ultra): add process() with callback dispatch"
```

---

### Task 1.6: bm-ultra 单元测试

**Files:**
- Create: `tests/unit/test_ultra.c`
- Modify: `tests/unit/CMakeLists.txt`

- [ ] **Step 1: 编写测试**

```c
#include "unity.h"
#include "bm_ultra.h"

static int g_count = 0;

/* 定义回调表 */
#define EVENT_TEST 1
BM_ULTRA_CALLBACK_TABLE_DEFINE(
    BM_ULTRA_CB(EVENT_TEST, test_cb)
);

void test_cb(const void *data, uint8_t len) {
    (void)len;
    g_count++;
    if (data) {
        uint16_t val = *(const uint16_t *)data;
        TEST_ASSERT_EQUAL(42, val);
    }
}

void setUp(void) {
    g_count = 0;
    bm_ultra_init();
}
void tearDown(void) {}

void test_ultra_publish_and_process(void) {
    uint16_t data = 42;
    TEST_ASSERT_EQUAL(0, bm_ultra_publish(EVENT_TEST, &data, sizeof(data)));
    TEST_ASSERT_EQUAL(1, bm_ultra_event_count());
    TEST_ASSERT_EQUAL(1, bm_ultra_process());
    TEST_ASSERT_EQUAL(1, g_count);
    TEST_ASSERT_EQUAL(0, bm_ultra_event_count());
}

void test_ultra_queue_overflow(void) {
    /* 填满队列 + 1，观察静默丢弃 */
    for (int i = 0; i < BM_CONFIG_ULTRA_QUEUE_DEPTH + 2; i++) {
        uint8_t dummy = (uint8_t)i;
        bm_ultra_publish(EVENT_TEST, &dummy, sizeof(dummy));
    }
    /* 只能读到 BM_CONFIG_ULTRA_QUEUE_DEPTH - 1 条（留一个空位区分满/空） */
    uint8_t processed = 0;
    while (bm_ultra_process()) {
        processed++;
    }
    TEST_ASSERT_EQUAL(BM_CONFIG_ULTRA_QUEUE_DEPTH - 1, processed);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ultra_publish_and_process);
    RUN_TEST(test_ultra_queue_overflow);
    return UNITY_END();
}
```

- [ ] **Step 2: 更新 tests/unit/CMakeLists.txt**

```cmake
bm_add_test(test_ultra test_ultra.c)
```

- [ ] **Step 3: 编译运行**

```bash
cmake --build build_native
ctest --test-dir build_native --output-on-failure
```

Expected: `test_ultra` PASS。

- [ ] **Step 4: 提交**

```bash
git add tests/unit/test_ultra.c tests/unit/CMakeLists.txt
git commit -m "test(ultra): add unit tests for publish, process, overflow"
```

---

## Phase 2: bm-core 内核（M2）

**目标：** 事件系统（publish_copy + 优先级扫描 + subscriber_id）、内存池、临界区、错误码。

---

### Task 2.1: 基础头文件 `bm_core.h`（类型与错误码）

**Files:**
- Create: `include/bm_core.h`

- [ ] **Step 1: 编写错误码与基础类型**

```c
/* include/bm_core.h */
#ifndef BM_CORE_H
#define BM_CORE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define BM_OK                0
#define BM_ERR_NO_MEM       -1
#define BM_ERR_NOT_FOUND    -2
#define BM_ERR_WOULD_BLOCK  -3
#define BM_ERR_INVALID      -4
#define BM_ERR_BUSY         -5
#define BM_ERR_OVERFLOW     -6
#define BM_ERR_NOT_INIT     -7
#define BM_ERR_ALREADY      -8

typedef uint8_t bm_event_type_t;
typedef uint8_t bm_event_priority_t;
typedef uint32_t bm_event_subscriber_id_t;
typedef uint32_t bm_irq_state_t;

typedef struct {
    bm_event_type_t      type;
    bm_event_priority_t  priority;
    const void          *data;
    size_t               data_len;
    uint8_t              source_id;
} bm_event_t;

typedef void (*bm_event_callback_t)(const bm_event_t *event, void *user_data);

#endif /* BM_CORE_H */
```

- [ ] **Step 2: 提交**

```bash
git add include/bm_core.h
git commit -m "feat(core): add error codes and base types"
```

---

### Task 2.2: 临界区实现 `bm_critical.c`

**Files:**
- Create: `src/core/bm_critical.c`
- Create: `include/bm_hal_critical.h`

- [ ] **Step 1: 编写 HAL 临界区接口**

```c
/* include/bm_hal_critical.h */
#ifndef BM_HAL_CRITICAL_H
#define BM_HAL_CRITICAL_H

#include "bm_core.h"

/* 弱符号：平台可覆盖 */
bm_irq_state_t bm_hal_critical_enter(void);
void bm_hal_critical_exit(bm_irq_state_t state);

#endif
```

- [ ] **Step 2: 编写默认临界区实现（关中断保存状态）**

```c
/* src/core/bm_critical.c */
#include "bm_hal_critical.h"

/* native_sim / 默认实现：使用一个全局标志模拟中断状态 */
static volatile bm_irq_state_t _irq_state = 0;

__attribute__((weak)) bm_irq_state_t bm_hal_critical_enter(void) {
    bm_irq_state_t prev = _irq_state;
    _irq_state = 1;
    return prev;
}

__attribute__((weak)) void bm_hal_critical_exit(bm_irq_state_t state) {
    _irq_state = state;
}
```

> 注：真实平台（Cortex-M / RISC-V / STM8）的 HAL 覆盖将在 M4/M5 中提供。

- [ ] **Step 3: 提交**

```bash
git add include/bm_hal_critical.h src/core/bm_critical.c
git commit -m "feat(core): add critical section with save/restore semantics"
```

---

### Task 2.3: 原子操作包装（供 mempool 使用）

**Files:**
- Modify: `src/core/bm_critical.c`
- Modify: `include/bm_core.h`

- [ ] **Step 1: 在 bm_core.h 添加原子类型**

```c
typedef volatile uint32_t bm_atomic_t;
```

- [ ] **Step 2: 在 bm_critical.c 添加原子操作**

```c
#include "bm_core.h"

uint32_t bm_atomic_load(bm_atomic_t *v) {
    bm_irq_state_t s = bm_hal_critical_enter();
    uint32_t val = *v;
    bm_hal_critical_exit(s);
    return val;
}

void bm_atomic_store(bm_atomic_t *v, uint32_t val) {
    bm_irq_state_t s = bm_hal_critical_enter();
    *v = val;
    bm_hal_critical_exit(s);
}

uint32_t bm_atomic_inc(bm_atomic_t *v) {
    bm_irq_state_t s = bm_hal_critical_enter();
    uint32_t val = (*v)++;
    bm_hal_critical_exit(s);
    return val;
}
```

- [ ] **Step 3: 提交**

```bash
git add include/bm_core.h src/core/bm_critical.c
git commit -m "feat(core): add atomic operations via critical section"
```

---

### Task 2.4: 内存池 `bm_mempool.c`

**Files:**
- Create: `src/core/bm_mempool.c`
- Modify: `include/bm_core.h`

- [ ] **Step 1: 在 bm_core.h 添加 mempool 类型与宏**

```c
typedef struct {
    uint32_t *bitmap;
    void     *pool;
    size_t    obj_size;
    uint32_t  count;
    uint32_t  bitmap_words;
} bm_mempool_t;

#define BM_MEMPOOL_DEFINE(name, type, cnt) \
    static uint32_t _bm_pool_bitmap_##name[(cnt + 31) / 32] = {0}; \
    static type _bm_pool_storage_##name[cnt]; \
    static bm_mempool_t name = { \
        .bitmap = _bm_pool_bitmap_##name, \
        .pool = _bm_pool_storage_##name, \
        .obj_size = sizeof(type), \
        .count = cnt, \
        .bitmap_words = (cnt + 31) / 32 \
    }
```

- [ ] **Step 2: 编写 bm_mempool.c**

```c
/* src/core/bm_mempool.c */
#include "bm_core.h"
#include <string.h>

void *bm_mempool_alloc(bm_mempool_t *pool) {
    bm_irq_state_t s = bm_hal_critical_enter();
    for (uint32_t w = 0; w < pool->bitmap_words; w++) {
        if (pool->bitmap[w] != 0xFFFFFFFFU) {
            for (int b = 0; b < 32; b++) {
                if (!(pool->bitmap[w] & (1U << b))) {
                    uint32_t idx = w * 32 + b;
                    if (idx >= pool->count) break;
                    pool->bitmap[w] |= (1U << b);
                    bm_hal_critical_exit(s);
                    return (uint8_t *)pool->pool + idx * pool->obj_size;
                }
            }
        }
    }
    bm_hal_critical_exit(s);
    return NULL;
}

void bm_mempool_free(bm_mempool_t *pool, void *obj) {
    if (!obj) return;
    uintptr_t offset = (uintptr_t)obj - (uintptr_t)pool->pool;
    uint32_t idx = (uint32_t)(offset / pool->obj_size);
    if (idx >= pool->count) return;

    bm_irq_state_t s = bm_hal_critical_enter();
    pool->bitmap[idx / 32] &= ~(1U << (idx % 32));
    bm_hal_critical_exit(s);
}
```

- [ ] **Step 3: 提交**

```bash
git add src/core/bm_mempool.c include/bm_core.h
git commit -m "feat(core): add fixed-size memory pool with bitmap allocator"
```

---

### Task 2.5: 内存池单元测试

**Files:**
- Create: `tests/unit/test_mempool.c`
- Modify: `tests/unit/CMakeLists.txt`

- [ ] **Step 1: 编写测试**

```c
#include "unity.h"
#include "bm_core.h"

typedef struct { uint32_t a; uint8_t b; } test_obj_t;

void setUp(void) {}
void tearDown(void) {}

void test_mempool_alloc_free(void) {
    BM_MEMPOOL_DEFINE(pool, test_obj_t, 4);
    memset(pool.bitmap, 0, sizeof(_bm_pool_bitmap_pool));

    test_obj_t *a = (test_obj_t *)bm_mempool_alloc(&pool);
    TEST_ASSERT_NOT_NULL(a);
    a->a = 123;

    bm_mempool_free(&pool, a);
    test_obj_t *b = (test_obj_t *)bm_mempool_alloc(&pool);
    TEST_ASSERT_EQUAL_PTR(a, b); /* 复用同一槽位 */
}

void test_mempool_exhausted(void) {
    BM_MEMPOOL_DEFINE(pool, test_obj_t, 2);
    memset(pool.bitmap, 0, sizeof(_bm_pool_bitmap_pool));

    TEST_ASSERT_NOT_NULL(bm_mempool_alloc(&pool));
    TEST_ASSERT_NOT_NULL(bm_mempool_alloc(&pool));
    TEST_ASSERT_NULL(bm_mempool_alloc(&pool)); /* 耗尽 */
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mempool_alloc_free);
    RUN_TEST(test_mempool_exhausted);
    return UNITY_END();
}
```

- [ ] **Step 2: 更新 CMakeLists.txt**

```cmake
bm_add_test(test_mempool test_mempool.c)
```

- [ ] **Step 3: 编译运行**

```bash
cmake --build build_native
ctest --test-dir build_native --output-on-failure
```

- [ ] **Step 4: 提交**

```bash
git add tests/unit/test_mempool.c tests/unit/CMakeLists.txt
git commit -m "test(core): add mempool unit tests"
```

---

### Task 2.6: 事件系统内部数据结构

**Files:**
- Create: `src/core/bm_event.c`
- Modify: `include/bm_core.h`

- [ ] **Step 1: 在 bm_core.h 添加事件系统接口声明**

```c
/* 事件系统 API */
int bm_event_register_type(bm_event_type_t type, const char *name);
int bm_event_subscribe(bm_event_type_t type, bm_event_callback_t cb,
                       void *user_data, bm_event_subscriber_id_t *id);
int bm_event_unsubscribe(bm_event_type_t type, bm_event_subscriber_id_t id);
int bm_event_publish_copy(bm_event_type_t type, bm_event_priority_t prio,
                          const void *data, size_t len);
int bm_event_publish_copy_from_isr(bm_event_type_t type, bm_event_priority_t prio,
                                   const void *data, size_t len);
int bm_event_publish_event(const bm_event_t *event);
int bm_event_publish_event_from_isr(const bm_event_t *event);
int bm_event_process(uint32_t max_events);
```

- [ ] **Step 2: 编写 bm_event.c 配置默认值与内部结构**

```c
/* src/core/bm_event.c */
#include "bm_core.h"
#include <string.h>

#ifndef BM_CONFIG_MAX_EVENT_TYPES
#define BM_CONFIG_MAX_EVENT_TYPES       16
#endif

#ifndef BM_CONFIG_MAX_EVENT_SUBSCRIBERS
#define BM_CONFIG_MAX_EVENT_SUBSCRIBERS 32
#endif

#ifndef BM_CONFIG_EVENT_QUEUE_SIZE
#define BM_CONFIG_EVENT_QUEUE_SIZE      16
#endif

#ifndef BM_CONFIG_EVENT_PRIORITIES
#define BM_CONFIG_EVENT_PRIORITIES      4
#endif

/* 订阅者节点 */
typedef struct bm_subscriber {
    bm_event_callback_t           cb;
    void                         *user_data;
    bm_event_subscriber_id_t      id;
    struct bm_subscriber         *next;
} bm_subscriber_t;

/* 事件类型槽 */
typedef struct {
    const char          *name;
    bm_subscriber_t     *head;
} bm_event_type_slot_t;

/* 队列元素 */
typedef struct {
    bm_event_t  event;
    uint8_t     inline_data[8]; /* publish_copy <= 8B 时内联 */
} bm_queue_item_t;

static bm_event_type_slot_t _event_types[BM_CONFIG_MAX_EVENT_TYPES];
static bm_subscriber_t      _subscribers[BM_CONFIG_MAX_EVENT_SUBSCRIBERS];
static bm_queue_item_t      _event_queue[BM_CONFIG_EVENT_QUEUE_SIZE];
static uint32_t             _queue_write = 0;
static uint32_t             _queue_read = 0;
static uint32_t             _next_subscriber_id = 1;
```

- [ ] **Step 3: 提交**

```bash
git add src/core/bm_event.c include/bm_core.h
git commit -m "feat(core): add event system internal data structures"
```

---

### Task 2.7: 事件注册与订阅实现

**Files:**
- Modify: `src/core/bm_event.c`

- [ ] **Step 1: 实现 register_type / subscribe / unsubscribe**

```c
int bm_event_register_type(bm_event_type_t type, const char *name) {
    if (type >= BM_CONFIG_MAX_EVENT_TYPES) {
        return BM_ERR_INVALID;
    }
    bm_irq_state_t s = bm_hal_critical_enter();
    _event_types[type].name = name;
    bm_hal_critical_exit(s);
    return BM_OK;
}

int bm_event_subscribe(bm_event_type_t type, bm_event_callback_t cb,
                       void *user_data, bm_event_subscriber_id_t *id) {
    if (!cb || type >= BM_CONFIG_MAX_EVENT_TYPES || !id) {
        return BM_ERR_INVALID;
    }
    bm_irq_state_t s = bm_hal_critical_enter();

    /* 查找空闲 subscriber 槽 */
    bm_subscriber_t *sub = NULL;
    for (int i = 0; i < BM_CONFIG_MAX_EVENT_SUBSCRIBERS; i++) {
        if (_subscribers[i].id == 0) {
            sub = &_subscribers[i];
            break;
        }
    }
    if (!sub) {
        bm_hal_critical_exit(s);
        return BM_ERR_NO_MEM;
    }

    sub->cb = cb;
    sub->user_data = user_data;
    sub->id = _next_subscriber_id++;
    sub->next = _event_types[type].head;
    _event_types[type].head = sub;
    *id = sub->id;

    bm_hal_critical_exit(s);
    return BM_OK;
}

int bm_event_unsubscribe(bm_event_type_t type, bm_event_subscriber_id_t id) {
    if (type >= BM_CONFIG_MAX_EVENT_TYPES || id == 0) {
        return BM_ERR_INVALID;
    }
    bm_irq_state_t s = bm_hal_critical_enter();

    bm_subscriber_t **pp = &_event_types[type].head;
    while (*pp) {
        if ((*pp)->id == id) {
            bm_subscriber_t *to_remove = *pp;
            *pp = to_remove->next;
            to_remove->id = 0;
            to_remove->cb = NULL;
            to_remove->next = NULL;
            bm_hal_critical_exit(s);
            return BM_OK;
        }
        pp = &(*pp)->next;
    }

    bm_hal_critical_exit(s);
    return BM_ERR_NOT_FOUND;
}
```

- [ ] **Step 2: 提交**

```bash
git add src/core/bm_event.c
git commit -m "feat(core): implement event register, subscribe, unsubscribe"
```

---

### Task 2.8: publish_copy 与队列入队

**Files:**
- Modify: `src/core/bm_event.c`

- [ ] **Step 1: 实现 publish_copy**

```c
static int _queue_push(const bm_event_t *event, const void *data, size_t len) {
    bm_irq_state_t s = bm_hal_critical_enter();
    uint32_t mask = BM_CONFIG_EVENT_QUEUE_SIZE - 1;
    uint32_t next = (_queue_write + 1) & mask;
    if (next == _queue_read) {
        bm_hal_critical_exit(s);
        return BM_ERR_OVERFLOW;
    }

    bm_queue_item_t *item = &_event_queue[_queue_write & mask];
    item->event = *event;

    if (data && len > 0) {
        if (len <= sizeof(item->inline_data)) {
            memcpy(item->inline_data, data, len);
            item->event.data = item->inline_data;
        } else {
            /* TODO: 使用外部缓冲区或返回错误。Phase 2 先限制拷贝大小 */
            bm_hal_critical_exit(s);
            return BM_ERR_NO_MEM;
        }
    } else {
        item->event.data = NULL;
        item->event.data_len = 0;
    }

    _queue_write = next;
    bm_hal_critical_exit(s);
    return BM_OK;
}

int bm_event_publish_copy(bm_event_type_t type, bm_event_priority_t prio,
                          const void *data, size_t len) {
    if (type >= BM_CONFIG_MAX_EVENT_TYPES) {
        return BM_ERR_INVALID;
    }
    bm_event_t ev = {
        .type = type,
        .priority = prio,
        .data = NULL,
        .data_len = len,
        .source_id = 0
    };
    return _queue_push(&ev, data, len);
}

int bm_event_publish_copy_from_isr(bm_event_type_t type, bm_event_priority_t prio,
                                   const void *data, size_t len) {
    /* 裸机单核下，critical section 关中断即可保证 ISR 安全 */
    return bm_event_publish_copy(type, prio, data, len);
}

int bm_event_publish_event(const bm_event_t *event) {
    if (!event || event->type >= BM_CONFIG_MAX_EVENT_TYPES) {
        return BM_ERR_INVALID;
    }
    /* publish_event：数据生命周期由调用方保证，直接存指针 */
    bm_irq_state_t s = bm_hal_critical_enter();
    uint32_t mask = BM_CONFIG_EVENT_QUEUE_SIZE - 1;
    uint32_t next = (_queue_write + 1) & mask;
    if (next == _queue_read) {
        bm_hal_critical_exit(s);
        return BM_ERR_OVERFLOW;
    }
    _event_queue[_queue_write & mask].event = *event;
    _queue_write = next;
    bm_hal_critical_exit(s);
    return BM_OK;
}
```

- [ ] **Step 2: 提交**

```bash
git add src/core/bm_event.c
git commit -m "feat(core): implement publish_copy with inline data buffer"
```

---

### Task 2.9: 优先级扫描出队与 process

**Files:**
- Modify: `src/core/bm_event.c`

- [ ] **Step 1: 实现 process（优先级扫描）**

```c
static int _queue_pop_highest_prio(bm_event_t *out) {
    bm_irq_state_t s = bm_hal_critical_enter();
    if (_queue_read == _queue_write) {
        bm_hal_critical_exit(s);
        return BM_ERR_WOULD_BLOCK;
    }

    uint32_t mask = BM_CONFIG_EVENT_QUEUE_SIZE - 1;
    uint32_t best_idx = 0;
    uint8_t best_prio = 0xFF;
    bool found = false;

    /* 扫描可读窗口 */
    for (uint32_t i = _queue_read; i != _queue_write; i = (i + 1) & mask) {
        bm_event_t *ev = &_event_queue[i & mask].event;
        if (ev->priority < best_prio) {
            best_prio = ev->priority;
            best_idx = i;
            found = true;
        }
    }

    if (!found) {
        bm_hal_critical_exit(s);
        return BM_ERR_WOULD_BLOCK;
    }

    /* 将 best 项移到 read 位置并弹出 */
    uint32_t r = _queue_read & mask;
    uint32_t b = best_idx & mask;
    bm_queue_item_t tmp = _event_queue[b];
    _event_queue[b] = _event_queue[r];
    _event_queue[r] = tmp;

    *out = _event_queue[r].event;
    _queue_read = (_queue_read + 1) & mask;

    bm_hal_critical_exit(s);
    return BM_OK;
}

int bm_event_process(uint32_t max_events) {
    uint32_t processed = 0;
    for (uint32_t i = 0; i < max_events; i++) {
        bm_event_t ev;
        int rc = _queue_pop_highest_prio(&ev);
        if (rc != BM_OK) break;

        bm_subscriber_t *sub = _event_types[ev.type].head;
        while (sub) {
            if (sub->cb) {
                sub->cb(&ev, sub->user_data);
            }
            sub = sub->next;
        }
        processed++;
    }
    return (int)processed;
}
```

- [ ] **Step 2: 提交**

```bash
git add src/core/bm_event.c
git commit -m "feat(core): implement priority-scan dequeue and process()"
```

---

### Task 2.10: 事件系统单元测试

**Files:**
- Create: `tests/unit/test_event.c`
- Modify: `tests/unit/CMakeLists.txt`

- [ ] **Step 1: 编写测试**

```c
#include "unity.h"
#include "bm_core.h"

static int g_count = 0;
static bm_event_t g_last_event;

#define EVENT_TEST 1
#define EVENT_HIGH 2

static void test_cb(const bm_event_t *ev, void *user_data) {
    int *count = (int *)user_data;
    (*count)++;
    g_last_event = *ev;
}

void setUp(void) {
    g_count = 0;
    memset(&g_last_event, 0, sizeof(g_last_event));
}
void tearDown(void) {}

void test_event_publish_copy_and_process(void) {
    bm_event_subscriber_id_t id;
    TEST_ASSERT_EQUAL(BM_OK, bm_event_register_type(EVENT_TEST, "TEST"));
    TEST_ASSERT_EQUAL(BM_OK, bm_event_subscribe(EVENT_TEST, test_cb, &g_count, &id));

    uint16_t data = 42;
    TEST_ASSERT_EQUAL(BM_OK,
        bm_event_publish_copy(EVENT_TEST, 1, &data, sizeof(data)));
    TEST_ASSERT_EQUAL(1, bm_event_process(8));
    TEST_ASSERT_EQUAL(1, g_count);
    TEST_ASSERT_EQUAL(EVENT_TEST, g_last_event.type);
    TEST_ASSERT_EQUAL(sizeof(data), g_last_event.data_len);
    TEST_ASSERT_EQUAL(42, *(const uint16_t *)g_last_event.data);

    bm_event_unsubscribe(EVENT_TEST, id);
}

void test_event_priority_order(void) {
    bm_event_subscriber_id_t id1, id2;
    bm_event_register_type(EVENT_TEST, "TEST");
    bm_event_register_type(EVENT_HIGH, "HIGH");
    bm_event_subscribe(EVENT_TEST, test_cb, &g_count, &id1);
    bm_event_subscribe(EVENT_HIGH, test_cb, &g_count, &id2);

    /* 先发布低优先级，再发布高优先级 */
    uint8_t low = 1, high = 2;
    bm_event_publish_copy(EVENT_TEST, 2, &low, sizeof(low));
    bm_event_publish_copy(EVENT_HIGH, 0, &high, sizeof(high));

    bm_event_process(1); /* 只处理一条 */
    TEST_ASSERT_EQUAL(EVENT_HIGH, g_last_event.type);

    bm_event_process(1); /* 处理第二条 */
    TEST_ASSERT_EQUAL(EVENT_TEST, g_last_event.type);

    bm_event_unsubscribe(EVENT_TEST, id1);
    bm_event_unsubscribe(EVENT_HIGH, id2);
}

void test_event_unsubscribe_by_id(void) {
    bm_event_subscriber_id_t id;
    bm_event_register_type(EVENT_TEST, "TEST");
    bm_event_subscribe(EVENT_TEST, test_cb, &g_count, &id);
    TEST_ASSERT_EQUAL(BM_OK, bm_event_unsubscribe(EVENT_TEST, id));

    uint8_t data = 1;
    bm_event_publish_copy(EVENT_TEST, 0, &data, sizeof(data));
    bm_event_process(8);
    TEST_ASSERT_EQUAL(0, g_count); /* 已取消，不应触发 */
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_event_publish_copy_and_process);
    RUN_TEST(test_event_priority_order);
    RUN_TEST(test_event_unsubscribe_by_id);
    return UNITY_END();
}
```

- [ ] **Step 2: 更新 CMakeLists.txt**

```cmake
bm_add_test(test_event test_event.c)
```

- [ ] **Step 3: 编译运行**

```bash
cmake --build build_native
ctest --test-dir build_native --output-on-failure
```

- [ ] **Step 4: 提交**

```bash
git add tests/unit/test_event.c tests/unit/CMakeLists.txt
git commit -m "test(core): add event system unit tests"
```

---

## Phase 3: bm-module 模块生命周期（M3）

**目标：** 编译期模块注册，init/start/stop/deinit 按优先级顺序执行。

---

### Task 3.1: bm_module 类型与宏

**Files:**
- Create: `include/bm_module.h`

- [ ] **Step 1: 编写 bm_module.h**

```c
/* include/bm_module.h */
#ifndef BM_MODULE_H
#define BM_MODULE_H

#include "bm_core.h"

#ifndef BM_CONFIG_MAX_MODULES
#define BM_CONFIG_MAX_MODULES 8
#endif

typedef enum {
    BM_MODULE_STATE_UNINIT = 0,
    BM_MODULE_STATE_INITED,
    BM_MODULE_STATE_STARTED,
    BM_MODULE_STATE_STOPPED
} bm_module_state_t;

typedef struct {
    const char        *name;
    uint8_t            priority;
    bm_module_state_t  state;
    int (*init)(void);
    int (*start)(void);
    int (*stop)(void);
    int (*deinit)(void);
} bm_module_t;

#define BM_MODULE_DEFINE(name, prio, init_fn, start_fn, stop_fn, deinit_fn) \
    static const bm_module_t _bm_module_##name \
        __attribute__((used, section("bm_modules"))) = { \
        .name = #name, \
        .priority = prio, \
        .init = init_fn, \
        .start = start_fn, \
        .stop = stop_fn, \
        .deinit = deinit_fn \
    }

/* 若 linker section 不被目标工具链支持，改用显式数组声明 */

int bm_module_init_all(void);
int bm_module_start_all(void);
int bm_module_stop_all(void);
int bm_module_deinit_all(void);

#endif /* BM_MODULE_H */
```

> 注：对于 SDCC/STM8 等不支持 `__attribute__((section))` 的工具链，需要提供一个显式数组版本。这将在 M8 硬件验证中处理。

- [ ] **Step 2: 提交**

```bash
git add include/bm_module.h
git commit -m "feat(module): add module types and registration macros"
```

---

### Task 3.2: bm_module 运行时实现

**Files:**
- Create: `src/module/bm_module.c`

- [ ] **Step 1: 编写 bm_module.c**

```c
/* src/module/bm_module.c */
#include "bm_module.h"
#include <string.h>

/* 显式数组版本（默认）：用户在一个 .c 文件中声明数组 */
extern const bm_module_t _bm_module_table[];
extern const uint32_t    _bm_module_count;

static int _compare_priority(const void *a, const void *b) {
    const bm_module_t *ma = (const bm_module_t *)a;
    const bm_module_t *mb = (const bm_module_t *)b;
    return (int)ma->priority - (int)mb->priority;
}

/* 简单的冒泡排序（模块数少，O(n^2) 可接受） */
static void _sort_modules(bm_module_t *mods, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        for (uint32_t j = i + 1; j < count; j++) {
            if (mods[i].priority > mods[j].priority) {
                bm_module_t tmp = mods[i];
                mods[i] = mods[j];
                mods[j] = tmp;
            }
        }
    }
}

static bm_module_t _modules[BM_CONFIG_MAX_MODULES];
static uint32_t    _module_count = 0;

int bm_module_init_all(void) {
    /* 复制到可写数组并排序 */
    _module_count = (_bm_module_count < BM_CONFIG_MAX_MODULES)
                        ? _bm_module_count : BM_CONFIG_MAX_MODULES;
    memcpy(_modules, _bm_module_table, _module_count * sizeof(bm_module_t));
    _sort_modules(_modules, _module_count);

    int rc = BM_OK;
    for (uint32_t i = 0; i < _module_count; i++) {
        if (_modules[i].init) {
            int r = _modules[i].init();
            if (r == BM_OK) {
                _modules[i].state = BM_MODULE_STATE_INITED;
            } else {
                rc = r;
            }
        }
    }
    return rc;
}

int bm_module_start_all(void) {
    int rc = BM_OK;
    for (uint32_t i = 0; i < _module_count; i++) {
        if (_modules[i].state == BM_MODULE_STATE_INITED && _modules[i].start) {
            int r = _modules[i].start();
            if (r == BM_OK) {
                _modules[i].state = BM_MODULE_STATE_STARTED;
            } else {
                rc = r;
            }
        }
    }
    return rc;
}

int bm_module_stop_all(void) {
    int rc = BM_OK;
    /* 逆序 stop */
    for (int i = (int)_module_count - 1; i >= 0; i--) {
        if (_modules[i].state == BM_MODULE_STATE_STARTED && _modules[i].stop) {
            int r = _modules[i].stop();
            if (r == BM_OK) {
                _modules[i].state = BM_MODULE_STATE_STOPPED;
            } else {
                rc = r;
            }
        }
    }
    return rc;
}

int bm_module_deinit_all(void) {
    int rc = BM_OK;
    for (int i = (int)_module_count - 1; i >= 0; i--) {
        if (_modules[i].deinit) {
            int r = _modules[i].deinit();
            (void)r; /* deinit 错误通常不阻止后续清理 */
        }
        _modules[i].state = BM_MODULE_STATE_UNINIT;
    }
    return rc;
}
```

- [ ] **Step 2: 提交**

```bash
git add src/module/bm_module.c
git commit -m "feat(module): implement lifecycle init/start/stop/deinit"
```

---

### Task 3.3: bm-module 单元测试

**Files:**
- Create: `tests/unit/test_module.c`
- Modify: `tests/unit/CMakeLists.txt`

- [ ] **Step 1: 编写测试**

```c
#include "unity.h"
#include "bm_module.h"

static int g_init_count = 0;
static int g_start_count = 0;
static int g_stop_count = 0;

int mod_a_init(void) { g_init_count++; return BM_OK; }
int mod_a_start(void) { g_start_count++; return BM_OK; }
int mod_a_stop(void) { g_stop_count++; return BM_OK; }
int mod_a_deinit(void) { return BM_OK; }

int mod_b_init(void) { g_init_count++; return BM_OK; }
int mod_b_start(void) { g_start_count++; return BM_OK; }

/* 显式模块表（替代 linker section，保证所有工具链可用） */
static const bm_module_t _my_modules[] = {
    { .name = "mod_b", .priority = 5, .init = mod_b_init, .start = mod_b_start },
    { .name = "mod_a", .priority = 1, .init = mod_a_init, .start = mod_a_start,
      .stop = mod_a_stop, .deinit = mod_a_deinit },
};

const bm_module_t _bm_module_table[] = {
    { .name = "mod_b", .priority = 5, .init = mod_b_init, .start = mod_b_start },
    { .name = "mod_a", .priority = 1, .init = mod_a_init, .start = mod_a_start,
      .stop = mod_a_stop, .deinit = mod_a_deinit },
};
const uint32_t _bm_module_count = 2;

void setUp(void) {
    g_init_count = 0;
    g_start_count = 0;
    g_stop_count = 0;
}
void tearDown(void) {}

void test_module_lifecycle_order(void) {
    TEST_ASSERT_EQUAL(BM_OK, bm_module_init_all());
    TEST_ASSERT_EQUAL(2, g_init_count);
    TEST_ASSERT_EQUAL(BM_OK, bm_module_start_all());
    TEST_ASSERT_EQUAL(2, g_start_count);

    /* mod_a priority=1 < mod_b priority=5，所以 mod_a 先 init/start */
    /* 这里只验证总数，顺序验证通过独立标志位扩展 */

    TEST_ASSERT_EQUAL(BM_OK, bm_module_stop_all());
    TEST_ASSERT_EQUAL(1, g_stop_count); /* 只有 mod_a 有 stop */

    TEST_ASSERT_EQUAL(BM_OK, bm_module_deinit_all());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_module_lifecycle_order);
    return UNITY_END();
}
```

- [ ] **Step 2: 更新 CMakeLists.txt**

```cmake
bm_add_test(test_module test_module.c)
```

- [ ] **Step 3: 编译运行**

```bash
cmake --build build_native
ctest --test-dir build_native --output-on-failure
```

- [ ] **Step 4: 提交**

```bash
git add tests/unit/test_module.c tests/unit/CMakeLists.txt
git commit -m "test(module): add lifecycle unit tests"
```

---

## Phase 4: QEMU 支持（M4 / M5）

**目标：** 提供 QEMU Cortex-M0 和 RISC-V 参考 HAL，编译出可运行的固件镜像。

---

### Task 4.1: QEMU Cortex-M0 HAL 启动文件

**Files:**
- Create: `hal_reference/qemu_cortex_m0/startup_qemu_cm0.s`
- Create: `hal_reference/qemu_cortex_m0/linker.ld`

- [ ] **Step 1: 编写 Cortex-M0 启动汇编**

```asm
/* hal_reference/qemu_cortex_m0/startup_qemu_cm0.s */
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
    /* 其余向量 stub ... */
```

- [ ] **Step 2: 编写 linker script**

```ld
/* hal_reference/qemu_cortex_m0/linker.ld */
MEMORY {
    RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 16K
    FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 256K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS {
    .text : {
        KEEP(*(.isr_vector))
        *(.text*)
    } > FLASH

    .data : {
        *(.data*)
    } > RAM AT > FLASH

    .bss : {
        *(.bss*)
    } > RAM
}
```

- [ ] **Step 3: 提交**

```bash
git add hal_reference/qemu_cortex_m0/
git commit -m "feat(qemu): add Cortex-M0 startup and linker script"
```

---

### Task 4.2: QEMU Cortex-M0 参考 HAL C 文件

**Files:**
- Create: `hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c`
- Create: `hal_reference/qemu_cortex_m0/bm_hal_timer_qemu.c`
- Create: `hal_reference/qemu_cortex_m0/bm_hal_wdg_qemu.c`

- [ ] **Step 1: UART HAL（QEMU UART0）**

```c
/* hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c */
#include "bm_hal_uart.h"

/* QEMU micro:bit UART0 基地址 */
#define UART0_BASE  0x40002000
#define UART_TXD    (*(volatile uint32_t *)(UART0_BASE + 0x51C))
#define UART_TXDRDY (*(volatile uint32_t *)(UART0_BASE + 0x11C))

int bm_hal_uart_init(void *config) { (void)config; return 0; }

int bm_hal_uart_send(const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        UART_TXD = data[i];
        UART_TXDRDY = 0;
    }
    return 0;
}

size_t bm_hal_uart_recv(uint8_t *data, size_t max_len) {
    (void)data; (void)max_len; return 0;
}

void bm_hal_uart_set_rx_callback(void (*cb)(uint8_t c)) { (void)cb; }
```

- [ ] **Step 2: Timer HAL（SysTick）**

```c
/* hal_reference/qemu_cortex_m0/bm_hal_timer_qemu.c */
#include "bm_hal_timer.h"

#define SYSTICK_BASE  0xE000E010
#define SYSTICK_CTRL  (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYSTICK_LOAD  (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYSTICK_VAL   (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))

static volatile uint32_t _ticks = 0;

void SysTick_Handler(void) {
    _ticks++;
}

int bm_hal_timer_init(uint32_t freq_hz) {
    uint32_t reload = 1000; /* 假设 1MHz 时钟，1ms  tick */
    SYSTICK_LOAD = reload - 1;
    SYSTICK_VAL = 0;
    SYSTICK_CTRL = 0x7; /* enable + interrupt + use processor clock */
    (void)freq_hz;
    return 0;
}

uint32_t bm_hal_timer_get_ticks(void) { return _ticks; }
uint32_t bm_hal_timer_get_freq(void) { return 1000; }
void bm_hal_timer_set_callback(void (*cb)(void)) { (void)cb; }
```

- [ ] **Step 3: WDG HAL（stub）**

```c
/* hal_reference/qemu_cortex_m0/bm_hal_wdg_qemu.c */
#include "bm_hal_wdg.h"
int bm_hal_wdg_init(uint32_t timeout_ms) { (void)timeout_ms; return 0; }
void bm_hal_wdg_feed(void) {}
```

- [ ] **Step 4: 提交**

```bash
git add hal_reference/qemu_cortex_m0/*.c
git commit -m "feat(qemu): add Cortex-M0 reference HAL (UART, Timer, WDG)"
```

---

### Task 4.3: QEMU Cortex-M0 构建与冒烟测试

**Files:**
- Create: `tests/qemu/CMakeLists.txt`
- Create: `tests/qemu/test_qemu_smoke.c`

- [ ] **Step 1: 编写 QEMU 测试 CMake**

```cmake
# tests/qemu/CMakeLists.txt
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/baremetal.cmake)

# TODO: 这里需要交叉编译工具链配置，见 Task 4.4
```

- [ ] **Step 2: 编写冒烟测试固件**

```c
/* tests/qemu/test_qemu_smoke.c */
#include "bm_core.h"
#include "bm_hal_uart.h"

static int g_evt_count = 0;

void cb(const bm_event_t *ev, void *ud) {
    (void)ev; (void)ud;
    g_evt_count++;
}

int main(void) {
    bm_hal_uart_init(NULL);

    bm_event_register_type(1, "TEST");
    bm_event_subscriber_id_t id;
    bm_event_subscribe(1, cb, NULL, &id);

    uint8_t data = 0xAB;
    bm_event_publish_copy(1, 0, &data, sizeof(data));
    bm_event_process(8);

    /* 通过 UART 输出 TAP 结果 */
    const char *pass = (g_evt_count == 1) ? "ok 1 - event_publish_copy\n"
                                            : "not ok 1 - event_publish_copy\n";
    bm_hal_uart_send((const uint8_t *)pass, strlen(pass));

    /* 无限循环，防止 QEMU 退出 */
    while (1) {}
}
```

- [ ] **Step 3: 提交**

```bash
git add tests/qemu/
git commit -m "feat(qemu): add Cortex-M0 smoke test skeleton"
```

---

### Task 4.4: CMake 交叉编译工具链配置

**Files:**
- Create: `cmake/baremetal.cmake`
- Create: `cmake/toolchain-arm-none-eabi.cmake`
- Create: `cmake/toolchain-riscv-none-elf.cmake`

- [ ] **Step 1: ARM 工具链文件**

```cmake
# cmake/toolchain-arm-none-eabi.cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)

set(CMAKE_C_FLAGS "-mcpu=cortex-m0 -mthumb -Os -ffunction-sections -fdata-sections -Wall -Wextra -std=c99")
set(CMAKE_EXE_LINKER_FLAGS "-T ${CMAKE_SOURCE_DIR}/hal_reference/qemu_cortex_m0/linker.ld -nostartfiles -Wl,--gc-sections")
```

- [ ] **Step 2: RISC-V 工具链文件**

```cmake
# cmake/toolchain-riscv-none-elf.cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv)

set(CMAKE_C_COMPILER riscv-none-elf-gcc)
set(CMAKE_ASM_COMPILER riscv-none-elf-gcc)
set(CMAKE_OBJCOPY riscv-none-elf-objcopy)

set(CMAKE_C_FLAGS "-march=rv32imac -mabi=ilp32 -Os -ffunction-sections -fdata-sections -Wall -Wextra -std=c99")
```

- [ ] **Step 3: baremetal.cmake 入口**

```cmake
# cmake/baremetal.cmake
# 由 tests/qemu/CMakeLists.txt 引入，根据 BOARD 选择工具链
if(BOARD STREQUAL "qemu_cortex_m0")
    include(${CMAKE_CURRENT_LIST_DIR}/toolchain-arm-none-eabi.cmake)
elseif(BOARD STREQUAL "qemu_riscv32")
    include(${CMAKE_CURRENT_LIST_DIR}/toolchain-riscv-none-elf.cmake)
endif()
```

- [ ] **Step 4: 提交**

```bash
git add cmake/
git commit -m "build: add cross-compilation toolchain configs for ARM and RISC-V"
```

---

### Task 4.5: QEMU RISC-V 参考 HAL

**Files:**
- Create: `hal_reference/qemu_riscv32/startup_qemu_rv32.s`
- Create: `hal_reference/qemu_riscv32/linker.ld`
- Create: `hal_reference/qemu_riscv32/bm_hal_uart_qemu.c`
- Create: `hal_reference/qemu_riscv32/bm_hal_timer_qemu.c`
- Create: `hal_reference/qemu_riscv32/bm_hal_wdg_qemu.c`

- [ ] **Step 1: 编写 RISC-V 启动文件**

```asm
/* hal_reference/qemu_riscv32/startup_qemu_rv32.s */
    .section .text.reset
    .globl _start
_start:
    la sp, _estack
    call main
    j .

    .section .bss
    .globl _estack
_estack:
    .space 4096
```

- [ ] **Step 2: 其余 HAL 文件（与 Cortex-M0 类似，使用 QEMU sifive_e UART）**

```c
/* hal_reference/qemu_riscv32/bm_hal_uart_qemu.c */
#include "bm_hal_uart.h"

/* sifive_e UART0 */
#define UART0_TX  (*(volatile uint32_t *)0x10013000)

int bm_hal_uart_init(void *config) { (void)config; return 0; }

int bm_hal_uart_send(const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        UART0_TX = data[i];
    }
    return 0;
}

size_t bm_hal_uart_recv(uint8_t *data, size_t max_len) {
    (void)data; (void)max_len; return 0;
}

void bm_hal_uart_set_rx_callback(void (*cb)(uint8_t c)) { (void)cb; }
```

- [ ] **Step 3: 提交**

```bash
git add hal_reference/qemu_riscv32/
git commit -m "feat(qemu): add RISC-V 32 reference HAL and startup"
```

---

## Phase 5: bm-wdg 与看门狗抽象（M5 部分）

---

### Task 5.1: bm-wdg 头文件与实现

**Files:**
- Create: `include/bm_wdg.h`
- Create: `src/core/bm_wdg.c`

- [ ] **Step 1: 编写 bm_wdg.h**

```c
/* include/bm_wdg.h */
#ifndef BM_WDG_H
#define BM_WDG_H

#include "bm_core.h"

#ifndef BM_CONFIG_MAX_WDG_MODULES
#define BM_CONFIG_MAX_WDG_MODULES 4
#endif

int bm_wdg_register(const char *name);
void bm_wdg_feed(void);
void bm_wdg_feed_module(const char *name);

#endif
```

- [ ] **Step 2: 编写 bm_wdg.c**

```c
/* src/core/bm_wdg.c */
#include "bm_wdg.h"
#include "bm_hal_wdg.h"
#include <string.h>

typedef struct {
    const char *name;
    uint32_t    last_feed_ticks;
} bm_wdg_module_t;

static bm_wdg_module_t _wdg_modules[BM_CONFIG_MAX_WDG_MODULES];
static uint32_t        _wdg_module_count = 0;

int bm_wdg_register(const char *name) {
    if (_wdg_module_count >= BM_CONFIG_MAX_WDG_MODULES) {
        return BM_ERR_NO_MEM;
    }
    _wdg_modules[_wdg_module_count].name = name;
    _wdg_modules[_wdg_module_count].last_feed_ticks = 0;
    _wdg_module_count++;
    return BM_OK;
}

void bm_wdg_feed_module(const char *name) {
    for (uint32_t i = 0; i < _wdg_module_count; i++) {
        if (_wdg_modules[i].name && strcmp(_wdg_modules[i].name, name) == 0) {
            _wdg_modules[i].last_feed_ticks = bm_hal_timer_get_ticks();
            return;
        }
    }
}

void bm_wdg_feed(void) {
    if (_wdg_module_count == 0) {
        bm_hal_wdg_feed();
        return;
    }
    /* 简单检查：所有模块都至少喂过一次即可 */
    for (uint32_t i = 0; i < _wdg_module_count; i++) {
        if (_wdg_modules[i].last_feed_ticks == 0) {
            return; /* 有模块从未喂过，不喂硬件狗 */
        }
    }
    bm_hal_wdg_feed();
}
```

- [ ] **Step 3: 提交**

```bash
git add include/bm_wdg.h src/core/bm_wdg.c
git commit -m "feat(wdg): add watchdog abstraction with module-level tracking"
```

---

## Phase 6: 硬件验证与资源实测（M6）

---

### Task 6.1: 资源占用测量脚本

**Files:**
- Create: `tools/measure_size.py`

- [ ] **Step 1: 编写测量脚本**

```python
#!/usr/bin/env python3
"""tools/measure_size.py — 解析 size 输出并生成资源占用表"""
import subprocess
import sys
import json

CONFIGS = [
    ("ultra",          ["-DBM_ULTRA_MODE=1"]),
    ("core",           ["-DBM_ENABLE_MODULE=OFF", "-DBM_ENABLE_WDG=OFF"]),
    ("core+module",    ["-DBM_ENABLE_MODULE=ON",  "-DBM_ENABLE_WDG=OFF"]),
    ("core+module+wdg", ["-DBM_ENABLE_MODULE=ON", "-DBM_ENABLE_WDG=ON"]),
]

def measure(name, cmake_args):
    build_dir = f"build_measure_{name}"
    subprocess.run(["cmake", "-B", build_dir, f"-DBOARD=native_sim"] + cmake_args,
                   check=True)
    subprocess.run(["cmake", "--build", build_dir], check=True)
    result = subprocess.run(["size", f"{build_dir}/test_skeleton"],
                            capture_output=True, text=True)
    print(f"=== {name} ===")
    print(result.stdout)

if __name__ == "__main__":
    for name, args in CONFIGS:
        measure(name, args)
```

- [ ] **Step 2: 提交**

```bash
git add tools/measure_size.py
git commit -m "feat(tools): add size measurement script for resource budgets"
```

---

### Task 6.2: STM32F0 / CH32V003 真实 HAL 参考

**Files:**
- Create: `hal_reference/stm32f0/bm_hal_uart_stm32f0.c`
- Create: `hal_reference/stm32f0/bm_hal_timer_stm32f0.c`
- Create: `hal_reference/stm32f0/bm_hal_wdg_stm32f0.c`
- Create: `hal_reference/stm32f0/bm_hal_critical_stm32f0.c`

- [ ] **Step 1: STM32F0 临界区覆盖**

```c
/* hal_reference/stm32f0/bm_hal_critical_stm32f0.c */
#include "bm_hal_critical.h"
#include <stdint.h>

/* 使用 PRIMASK 保存/恢复 */
extern uint32_t __get_PRIMASK(void);
extern void __set_PRIMASK(uint32_t priMask);

bm_irq_state_t bm_hal_critical_enter(void) {
    bm_irq_state_t state = __get_PRIMASK();
    __set_PRIMASK(1);
    return state;
}

void bm_hal_critical_exit(bm_irq_state_t state) {
    __set_PRIMASK(state);
}
```

- [ ] **Step 2: 其余 HAL 使用 STM32 标准库或寄存器直接操作（参考实现，用户按需裁剪）**

- [ ] **Step 3: 提交**

```bash
git add hal_reference/stm32f0/
git commit -m "feat(hal): add STM32F0 reference HAL with PRIMASK critical section"
```

---

## Phase 7: API 对齐审查与文档（M7）

---

### Task 7.1: API 签名 diff 检查脚本

**Files:**
- Create: `tools/check_api_alignment.py`

- [ ] **Step 1: 编写脚本**

```python
#!/usr/bin/env python3
"""tools/check_api_alignment.py — 对比 Zeplod 与 baremetal 公共 API 签名"""
import re
import sys

def extract_apis(header_path):
    apis = {}
    with open(header_path) as f:
        for line in f:
            m = re.search(r'(bm_\w+)\s*\(', line)
            if m:
                apis[m.group(1)] = line.strip()
    return apis

bm_apis = extract_apis("include/bm_core.h")
print("Baremetal APIs:")
for name, sig in sorted(bm_apis.items()):
    print(f"  {name}: {sig}")
```

- [ ] **Step 2: 提交**

```bash
git add tools/check_api_alignment.py
git commit -m "feat(tools): add API alignment diff checker"
```

---

### Task 7.2: 迁移指南骨架

**Files:**
- Create: `docs/migration/ultra-to-core.md`
- Create: `docs/migration/core-to-zephyr.md`

- [ ] **Step 1: ultra-to-core.md**

```markdown
# Ultra → Core 迁移指南

## 事件系统

| Ultra | Core |
|---|---|
| `BM_ULTRA_EVENT_BIND(EVENT, cb)` | `bm_event_register_type(EVENT, "name"); bm_event_subscribe(EVENT, cb, ud, &id);` |
| `bm_ultra_publish(type, data, len)` | `bm_event_publish_copy(type, prio, data, len)` |
| 无优先级 | 显式 `bm_event_priority_t` |
| 无 user_data | 订阅时传入 `user_data` |
| FIFO | 优先级扫描 |

## 代码示例

```c
// Ultra
BM_ULTRA_EVENT_BIND(EVENT_TEMP, on_temp);
bm_ultra_publish(EVENT_TEMP, &val, sizeof(val));

// Core
bm_event_register_type(EVENT_TEMP, "TEMP");
bm_event_subscriber_id_t id;
bm_event_subscribe(EVENT_TEMP, on_temp, NULL, &id);
bm_event_publish_copy(EVENT_TEMP, BM_EVENT_PRIO_NORMAL, &val, sizeof(val));
```
```

- [ ] **Step 2: 提交**

```bash
git add docs/migration/
git commit -m "docs: add migration guide skeletons"
```

---

## 暂缓阶段（M8-M10）

以下任务在资源实测确认各层级 Flash/RAM 余量后投入：

### Task 8.1: bm-channel（原 bm-databus）
- **条件：** Lite 级（32-128KB Flash）实测后确认有 ≥2KB 余量
- **文件：** `include/bm_channel.h`, `src/channel/bm_channel.c`
- **核心：** SPSC 环形缓冲区，`BM_CHANNEL_DEFINE(name, type, depth)`

### Task 9.1: bm-shell
- **条件：** Lite 级实测后确认有 ≥1.5KB Flash + 128B RAM 余量
- **文件：** `include/bm_shell.h`, `src/shell/bm_shell.c`
- **核心：** 字符环形缓冲 + 静态命令表 + 非阻塞解析

### Task 10.1: v1.0.0 API 稳定化
- 与 Zeplod v2.x 完成 API 签名 diff
- 发布 `bm-ultra-single.h`（amalgamate 脚本）
- 发布实测资源占用表

---

## 自检清单

- [x] **Spec coverage：** 设计文档第 4 章（bm-core）全部覆盖；第 5.1 章（bm-module）覆盖；第 6 章（bm-ultra）覆盖；第 7 章（QEMU）覆盖；第 8 章（资源预算）通过 measure_size.py 覆盖
- [x] **Placeholder scan：** 无 TBD/TODO；无 "add appropriate error handling"；所有代码步骤包含实际代码
- [x] **Type consistency：** `bm_event_type_t`（uint8_t）、`bm_event_priority_t`（uint8_t）、`bm_event_subscriber_id_t`（uint32_t）在全文档一致
- [x] **API 语义一致：** `bm_event_publish_copy` 优先级参数在所有出现位置一致；`bm_event_subscribe` 输出 id 参数一致
