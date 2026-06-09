# Zeplod Baremetal 框架设计文档

> 日期：2026-06-08
> 状态：设计阶段（已根据审查意见修正 v2）
> 范围：独立裸机框架仓库（`zeplod-baremetal`），概念继承 Zeplod，源码独立实现
> 目标：覆盖 Zephyr 无法覆盖的资源受限 MCU，从 <8KB Flash 到 128KB Flash 分级配置；定位为 Zeplod 机器人体系在末端节点（电机板、传感器、执行器）上的运行时延伸

---

## 1. 项目定位与边界

### 1.1 与 Zeplod（Zephyr 版）的关系

| 维度 | Zeplod（Zephyr） | Zeplod Baremetal |
|---|---|---|
| 依赖 | Zephyr RTOS | 无 OS 依赖，纯 C99 |
| 最小 RAM | ~32KB（`prj_tiny.conf`） | ~256B（`bm-ultra`） |
| 多线程 | 支持（`ipc_service`、`data_bus` 多消费者） | 不支持（主循环+中断） |
| 动态模块 | 运行时注册 | 编译期静态或预分配数组 |
| API 目标 | 高度兼容 | 继承 Zeplod 思路的裸机子集；核心事件类型、回调语义、模块生命周期与 Zeplod 一致，允许裸机场景下的必要简化 |

**核心原则**：
1. **概念继承**：事件驱动、模块化、publish-subscribe 等设计哲学不变
2. **API 契约**：`bm_event_publish_copy()` 与 `event_publish_copy()` 语义对齐；回调签名与 Zeplod `event_callback_t` 保持一致（`const bm_event_t *event, void *user_data`），确保迁移时代码可预测
3. **独立演进**：两个仓库无代码共享，避免条件编译地狱；bugfix 通过人工 review 同步；事件 ID 表、诊断码、数据结构 schema 由 Zeplod 主仓库统一定义，baremetal 复用

### 1.2 覆盖资源谱系

| 层级 | Flash | RAM | 目标芯片 | 使用路径 |
|---|---|---|---|---|
| Ultra | <8KB | <1KB | STM8、AVR、8051 | `#include <bm_ultra.h>` 头文件库 |
| Nano | 8-32KB | 1-4KB | CH32V003、STM32F030、GD32E230 | `bm-core` + 可选 `bm-module` |
| Lite | 32-128KB | 4-16KB | STM32F103C8、nRF51822、CH32V307 | `bm-core` + `bm-module` + `bm-channel` + `bm-shell` |

### 1.3 明确不做

- **不支持动态加载/卸载模块**：所有模块在编译期绑定，降低运行时复杂度和内存碎片风险
- **不支持真正的多线程/IPC**：无 `k_thread`、`k_sem` 等 RTOS 概念；模块间通信仅通过事件和 `bm-channel`
- **不替代 Zephyr**：资源足够时（>32KB SRAM），仍推荐使用 Zeplod + Zephyr 以获得完整生态
- **不支持标准 C 库依赖**：除 `stdint.h`、`stddef.h`、`stdbool.h`、`string.h`（有限子集：`memcpy`、`memset`、`memcmp`、`strlen`）外，不依赖 `malloc`、`printf`、`stdio`、`strcpy`、`sprintf` 等

### 1.4 机器人体系三层架构

Baremetal 不是孤立项目，而是 Zeplod 在机器人“末梢控制器”上的延伸。最有价值的协同方式是三层分工：

```
┌─────────────────────────────────────────────────────────────┐
│  ROS 2 / Linux                                              │
│  导航、感知、地图、任务规划、人机交互                          │
├─────────────────────────────────────────────────────────────┤
│  Zeplod + Zephyr                                            │
│  机器人实时控制中枢：多传感器融合、CAN/UART/RS485、状态机、      │
│  Data Bus（多消费者）、模块编排、硬实时协调                      │
├─────────────────────────────────────────────────────────────┤
│  Zeplod Baremetal                                           │
│  低成本 MCU 节点：电机小板、编码器采集、碰撞/急停、灯光、        │
│  舵机、低功耗传感器、简单电源管理                               │
└─────────────────────────────────────────────────────────────┘
```

**跨层共享契约**：
- 事件 ID 表（统一头文件，避免重复定义）
- 模块命名规范与诊断码体系
- 数据结构 schema（传感器帧、控制指令、状态上报）
- 通信协议（CAN/UART 帧格式，裸机侧作为从节点）
- 迁移示例代码（Ultra → Core → Zephyr 的渐进升级路径）

**核心价值**：低端节点（CH32V003、STM8、AVR、8051、低端 STM32F0、小型传感器/执行器板）统一事件、模块生命周期、诊断和迁移路径，比每个板子手写 super-loop 更有长期价值。

---

## 2. 目录结构与模块划分

```
zeplod-baremetal/
├── include/
│   ├── bm_core.h           # 事件系统、内存池、临界区、看门狗、基础类型
│   ├── bm_module.h         # 模块生命周期（可选）
│   ├── bm_channel.h        # 轻量数据通道 SPSC（可选，非 Zeplod Data Bus）
│   ├── bm_shell.h          # 串口 CLI（可选）
│   └── bm_ultra.h          # 极端裁剪头文件库
├── src/
│   ├── core/
│   │   ├── bm_event.c      # 发布/订阅/分发（短临界区保护队列）
│   │   ├── bm_mempool.c    # 内存池实现
│   │   ├── bm_critical.c   # 临界区默认实现（关中断，保存/恢复状态）
│   │   └── bm_wdg.c        # 看门狗抽象（可选编译）
│   ├── module/
│   │   └── bm_module.c     # 模块注册与生命周期
│   ├── channel/
│   │   └── bm_channel.c    # 轻量 SPSC 数据通道（纯非阻塞）
│   ├── shell/
│   │   └── bm_shell.c      # 串口 CLI 实现
│   └── hal/
│       ├── bm_hal_uart.c   # UART HAL 默认实现（桩/弱符号）
│       ├── bm_hal_timer.c  # 定时器 HAL 默认实现
│       └── bm_hal_gpio.c   # GPIO HAL 默认实现
├── hal_reference/          # 参考 HAL 实现
│   ├── qemu_cortex_m0/     # QEMU Cortex-M0 仿真
│   ├── qemu_riscv32/       # QEMU RISC-V 32 仿真
│   ├── native_sim/         # PC 纯软件模拟（无硬件依赖）
│   └── stm32f0/            # STM32F0 真实硬件参考
├── tests/
│   ├── unit/               # PC 可运行单元测试（Unity 框架）
│   ├── qemu/               # QEMU 仿真冒烟测试
│   └── hardware/           # 真实硬件测试（手动/CI）
├── docs/
│   ├── api/                # API 参考
│   ├── migration/          # Zeplod ↔ Baremetal 迁移指南
│   └── porting/            # HAL 移植指南
├── cmake/
│   └── baremetal.cmake     # CMake 工具链封装
├── Makefile                # 纯 Makefile 构建（8-bit 工具链友好）
├── CMakeLists.txt          # CMake 构建（32-bit 主流）
└── bm_config.h.template    # 应用级配置模板
```

---

## 3. 统一错误码体系

所有公共 API 统一返回 `int`，使用以下错误码。零表示成功，负值表示错误。

```c
#define BM_OK                0   /* 成功 */
#define BM_ERR_NO_MEM       -1   /* 内存/池耗尽 */
#define BM_ERR_NOT_FOUND    -2   /* 事件/模块/通道未找到 */
#define BM_ERR_WOULD_BLOCK  -3   /* 非阻塞操作暂不可行 */
#define BM_ERR_INVALID      -4   /* 参数无效 */
#define BM_ERR_BUSY         -5   /* 资源忙 */
#define BM_ERR_OVERFLOW     -6   /* 队列/缓冲区溢出 */
#define BM_ERR_NOT_INIT     -7   /* 模块未初始化 */
#define BM_ERR_ALREADY      -8   /* 重复操作（如重复订阅） */
```

**语义约定**：
- ISR 安全 API 禁止返回需要调用方重试的错误（ISR 中无法阻塞或轮询），ISR 路径遇错丢弃并可选计 `dropped` 统计
- 主循环 API 返回 `BM_ERR_WOULD_BLOCK` 时，调用方应在下一轮主循环重试

---

## 4. bm-core 设计

`bm-core` 是整个框架的内核，**所有场景必须链接**。设计目标：<2KB Flash，<512B RAM（不含应用数据）。

### 4.1 事件系统

**核心语义**：
- 事件类型用 `uint8_t` 标识（`bm_event_type_t`，与 Zeplod `event_type_t` 范围一致：0–255）
- 支持最多 `BM_CONFIG_MAX_EVENT_TYPES` 种事件（默认 32，可配置到 8）
- 每个事件类型维护一个订阅者链表（静态数组实现的侵入式链表）
- 支持 `publish_copy`（主循环上下文）和 `publish_copy_from_isr`（中断上下文）

**队列设计——短临界区保护的环形缓冲区**：
- 裸机单核场景下，**不采用无锁原子竞争**：仅靠 `atomic_inc(write_idx)` 不足以保证生产者写完之前主循环不读到未完成数据，且满队列时存在超额 reservation 风险
- **采用短临界区（关中断）入队**：`bm_critical_enter()` / `bm_critical_exit()` 保存并恢复中断状态，保证 ISR 与主循环互斥，代码更小、更可靠，也更符合 `<2KB Flash` 的目标
- 队列容量为 2 的幂（默认 16），利用位掩码实现快速环形索引
- 队列元素结构：
  ```c
  typedef struct {
      bm_event_type_t   event_type;   /* uint8_t，与 Zeplod 一致 */
      bm_event_priority_t priority;   /* 0 = 最高 */
      uint8_t           flags;
      const void       *data;         /* publish_copy 时指向队列内联缓冲区；publish_event 时指向调用方提供的 bm_event_t */
      size_t            data_len;
      uint8_t           source_id;    /* 可选：发布源标识，便于诊断 */
  } bm_event_queue_item_t;
  ```

**优先级实现——出队扫描**：
- 不在入队时排序（避免 ISR latency 抖动）
- `bm_event_process(max_events)` 每次从当前可读窗口中扫描最高优先级事件出队
- 复杂度 O(max_events × queue_depth)，裸机主循环对 O(n) 不敏感
- 可通过 `BM_CONFIG_EVENT_PRIORITIES` 配置优先级数量（默认 4 级：HIGH / NORMAL / LOW / IDLE），降低扫描范围

**API 概览**：
```c
typedef uint8_t bm_event_type_t;
typedef uint8_t bm_event_priority_t;
typedef uint32_t bm_event_subscriber_id_t;

typedef struct {
    bm_event_type_t   type;
    bm_event_priority_t priority;
    const void       *data;
    size_t            data_len;
    uint8_t           source_id;
} bm_event_t;

/* 回调签名与 Zeplod event_callback_t 语义一致：const event_t * + user_data */
typedef void (*bm_event_callback_t)(const bm_event_t *event, void *user_data);

/* 事件类型注册（命名 + 静态校验） */
int bm_event_register_type(bm_event_type_t type, const char *name);

/* 订阅：输出 subscriber_id，用于精确取消订阅 */
int bm_event_subscribe(bm_event_type_t type, bm_event_callback_t cb,
                       void *user_data, bm_event_subscriber_id_t *id);
int bm_event_unsubscribe(bm_event_type_t type, bm_event_subscriber_id_t id);

/* 推荐 API：publish_copy 语义与 Zeplod event_publish_copy() 对齐 */
int bm_event_publish_copy(bm_event_type_t type, bm_event_priority_t prio,
                          const void *data, size_t len);
int bm_event_publish_copy_from_isr(bm_event_type_t type, bm_event_priority_t prio,
                                   const void *data, size_t len);

/* 高级用法：发布调用方已构造好的 bm_event_t（数据生命周期由调用方保证） */
int bm_event_publish_event(const bm_event_t *event);
int bm_event_publish_event_from_isr(const bm_event_t *event);

/* 主循环调用，返回实际处理事件数 */
int bm_event_process(uint32_t max_events);
```

### 4.2 内存池

**零堆分配原则**：所有内存在编译期或 `bm_init()` 初期分配完毕。

```c
// 编译期定义一个内存池
BM_MEMPOOL_DEFINE(my_pool, struct my_event_data, 16);

// 使用
struct my_event_data *item = bm_mempool_alloc(&my_pool);
if (item) {
    bm_mempool_free(&my_pool, item);
}
```

**实现细节**：
- 每个池是一个固定大小的对象数组 + 空闲位图（bitmap）
- `alloc` 和 `free` 在有 `ffs`/`clz` 指令的平台上为 O(1)，软件 fallback 为 O(n / word_bits)（按字扫描位图）
- 中断安全：`alloc_from_isr` 和 `free_from_isr` 使用无锁位图操作（原子字操作），仅适用于池大小 ≤32 或 ≤64（一个原子字可覆盖）的场景

### 4.3 临界区与原子操作

```c
// 临界区：保存并恢复中断状态，支持嵌套
bm_irq_state_t bm_critical_enter(void);
void bm_critical_exit(bm_irq_state_t state);

// 原子操作（默认实现：关中断包装）
typedef volatile uint32_t bm_atomic_t;
uint32_t bm_atomic_load(bm_atomic_t *v);
void bm_atomic_store(bm_atomic_t *v, uint32_t val);
uint32_t bm_atomic_inc(bm_atomic_t *v);
```

**可覆盖性**：
- 通过 `bm_hal_critical.h` 提供弱符号或宏覆盖点
- Cortex-M 可覆盖为保存/恢复 `PRIMASK`（`__get_PRIMASK()` / `__set_PRIMASK()`），支持嵌套临界区
- RISC-V 可覆盖为保存/恢复 `mstatus.MIE`，支持嵌套
- 8-bit MCU（如 STM8）可覆盖为 `sim` / `rim`，但需用全局/寄存器保存先前状态以支持嵌套

### 4.4 看门狗抽象（可选编译）

裸机主循环必须喂狗。框架提供轻量看门狗封装，**不接管硬件看门狗初始化**（由 HAL 负责），但提供统一喂狗接口和模块级注册。

```c
// bm_wdg.h（可选，不引用则不链入）
int bm_wdg_register(const char *name);      /* 模块注册自己的喂狗需求 */
void bm_wdg_feed(void);                     /* 应用主循环调用，内部喂硬件狗 */
void bm_wdg_feed_module(const char *name);  /* 模块声明自己还活着 */
```

**设计约束**：
- `bm_wdg_feed()` 应在每次 `bm_event_process()` 之后由应用调用
- 若注册了模块看门狗，`bm_wdg_feed()` 会检查所有模块是否在规定时间内调用过 `feed_module`，任一模块未喂则本次不喂硬件狗，触发复位
- 无模块注册时，`bm_wdg_feed()` 直接透传喂硬件狗

### 4.5 低功耗 Hook

裸机事件驱动框架的标准模式：**事件队列为空时进入低功耗睡眠，中断唤醒后继续处理**。

```c
// 应用层主循环模板
while (1) {
    uint32_t processed = bm_event_process(BM_CONFIG_MAX_EVENTS_PER_LOOP);
    if (processed == 0) {
        bm_idle_hook();  /* 应用可覆盖为 __WFI() 或深度睡眠流程 */
    }
    bm_wdg_feed();
}
```

- `bm_idle_hook()` 为弱符号，默认空实现
- 应用覆盖为 `__WFI()`（Cortex-M）或具体芯片的睡眠指令

---

## 5. 可选模块设计

可选模块通过独立的 `.c` 文件提供，**不引用则不链入**。每个模块有自己的 `bm_config.h` 开关。

### 5.1 bm-module（模块生命周期）

**与 Zeplod 的映射**：
- `bm_module_register()` → 静态数组插入（无动态内存）
- `bm_module_init_all()` / `bm_module_start_all()` → 按优先级顺序调用
- `bm_module_stop_all()` / `bm_module_deinit_all()` → 逆序调用

**资源占用**：
- 每个模块占 ~16B RAM（函数指针 × 4 + 状态 + 优先级 + 名称指针）
- 最大模块数 `BM_CONFIG_MAX_MODULES`（默认 16）

**注册方式**：
```c
// 编译期注册（推荐）
BM_MODULE_DEFINE(my_sensor, 10, my_sensor_init, my_sensor_start, my_sensor_stop, my_sensor_deinit);
```

### 5.2 bm-channel（轻量数据通道，非 Zeplod Data Bus）

> **命名说明**：此组件**不是** Zeplod Data Bus 的裸机移植。Zeplod Data Bus 的核心语义是：命名 channel、多消费者回调、引用计数、callback 生命周期内数据有效。裸机单核无多线程，无法实现多消费者并发回调，因此本组件定位为 **SPSC 环形数据通道**，API 与 Zeplod Data Bus **不兼容**，名称亦不同，避免用户误用。

**设计约束**：
- 单生产者/单消费者（或轮询读取），无并发消费者问题
- Channel 静态定义：`BM_CHANNEL_DEFINE(temp_sensor, struct temp_data, 4)`
- 支持 `publish` / `publish_from_isr` / `try_read` / `read_latest`
- **纯非阻塞语义**：裸机无阻塞等待概念，所有读取操作要么成功返回数据，要么返回 `BM_ERR_WOULD_BLOCK`
- 数据通过值拷贝进出 channel，无指针所有权歧义

**API 概览**：
```c
int bm_channel_publish(bm_channel_t *ch, const void *data);
int bm_channel_publish_from_isr(bm_channel_t *ch, const void *data);
int bm_channel_try_read(bm_channel_t *ch, void *out_data);      /* 非阻塞 */
int bm_channel_read_latest(bm_channel_t *ch, void *out_data);   /* 返回最新一条，不阻塞 */
```

### 5.3 bm-shell（串口 CLI）

**设计约束**：
- 字符环形缓冲区（大小可配置，默认 64B）
- 命令表静态定义：`BM_SHELL_CMD_DEFINE("status", cmd_status, "Show system status")`
- 非阻塞：`bm_shell_feed(char c)` 在 UART ISR 中调用
- 主循环调用 `bm_shell_process()` 解析并执行命令

**资源占用**：
- ~1KB Flash（含命令表 + 解析器）
- ~64-128B RAM（环形缓冲 + 行缓冲）

---

## 6. bm-ultra 极端裁剪层

当目标资源低于 8KB Flash / 1KB RAM 时，标准 `bm-core` 的函数指针表、模块系统、事件队列都可能成为负担。`bm-ultra` 是一个**纯头文件库**，通过宏和 inline 函数实现零链接开销。

### 6.1 核心差异

| 特性 | bm-core | bm-ultra |
|---|---|---|
| 事件注册 | 运行时 `bm_event_subscribe()` | 编译期 `BM_ULTRA_EVENT_BIND(EVENT_TEMP, on_temp)` |
| 回调类型 | 通用 `void*` user_data | 无 user_data，回调签名固定 |
| 事件队列 | 无锁环形缓冲区 + 优先级扫描 | 简单环形数组（FIFO） |
| 内存池 | 通用 bitmap 池 | 无：事件数据通过值拷贝存储在队列元素内 |
| 模块系统 | 有 | 无：直接在中断和主循环中写逻辑 |

### 6.2 数据拷贝语义（关键设计）

`bm-ultra` **禁止传递指针**，所有事件数据通过**值拷贝**存入队列。队列元素结构：

```c
typedef struct {
    bm_event_type_t event_type;  /* uint8_t，与 bm-core 和 Zeplod 一致 */
    uint8_t  data[BM_CONFIG_ULTRA_MAX_EVENT_DATA_SIZE];  /* 默认 8B，可配置 */
    uint8_t  data_len;
} bm_ultra_queue_item_t;
```

**使用方式**：
```c
#include "bm_ultra.h"

// 编译期绑定事件与回调
BM_ULTRA_EVENT_BIND(EVENT_TEMP_SENSOR, on_temp_sensor);

void on_temp_sensor(const void *data, uint8_t len) {
    uint16_t temp = *(const uint16_t *)data;
    (void)len;  /* 若数据大小固定，len 可忽略 */
    // 处理
}

void main(void) {
    bm_ultra_init();
    while (1) {
        bm_ultra_process();  // 消费事件队列
        if (bm_ultra_event_count() == 0) {
            __WFI();  // 或芯片特定睡眠指令
        }
    }
}

// 在 ADC ISR 中
void adc_isr(void) {
    uint16_t temp = adc_read();
    bm_ultra_publish_from_isr(EVENT_TEMP_SENSOR, &temp, sizeof(temp));
    // bm_ultra_publish_from_isr 内部执行 memcpy 到队列槽位
}
```

> ⚠️ **与 bm-core 的关键差异**：`bm-core` 的 `publish` 传递的是内存池指针（零拷贝），`bm-ultra` 的 `publish_from_isr` 执行数据拷贝。这是资源与安全的权衡。

### 6.3 资源预算（典型 8051 / STM8）

| 组件 | Flash | RAM |
|---|---|---|
| bm-ultra 核心 | ~0.5KB | ~128B |
| 8 个事件类型 | ~0.3KB | ~64B |
| 事件队列（深度 8，数据 8B） | - | ~80B |
| **合计** | **~0.8KB** | **~272B** |

---

## 7. HAL 抽象层与 QEMU 仿真支持

### 7.1 HAL 设计原则

- **接口与实现分离**：`include/bm_hal_*.h` 定义接口，`src/hal/` 提供默认桩，`hal_reference/` 提供真实实现
- **弱符号机制**：应用层可覆盖 HAL 函数，无需修改框架源码
- **最小依赖**：HAL 层不调用任何 `bm-core` 函数，避免循环依赖
- **纯非阻塞语义**：裸机框架下所有 HAL API 均为非阻塞，禁止忙等

### 7.2 HAL 接口清单

```c
// bm_hal_uart.h
int bm_hal_uart_init(void *config);
int bm_hal_uart_send(const uint8_t *data, size_t len);
size_t bm_hal_uart_recv(uint8_t *data, size_t max_len);  /* 非阻塞，返回实际读取字节数 */
void bm_hal_uart_set_rx_callback(void (*cb)(uint8_t c));

// bm_hal_timer.h
int bm_hal_timer_init(uint32_t freq_hz);
uint32_t bm_hal_timer_get_ticks(void);
uint32_t bm_hal_timer_get_freq(void);
void bm_hal_timer_set_callback(void (*cb)(void));

// bm_hal_gpio.h
int bm_hal_gpio_init(uint32_t pin, uint32_t mode);
int bm_hal_gpio_write(uint32_t pin, uint32_t value);
int bm_hal_gpio_read(uint32_t pin);
void bm_hal_gpio_set_irq_callback(uint32_t pin, void (*cb)(uint32_t pin));

// bm_hal_wdg.h
int bm_hal_wdg_init(uint32_t timeout_ms);
void bm_hal_wdg_feed(void);
```

### 7.3 QEMU 仿真支持

QEMU 是裸机框架的核心验证手段之一。**所有 `bm-core` 功能必须在 QEMU 上可测试**，无需真实硬件。

**支持的 QEMU 目标**：

| QEMU 板型 | 架构 | 用途 |
|---|---|---|
| `qemu_cortex_m0` | ARM Cortex-M0 | 验证 32-bit 最小目标 |
| `qemu_riscv32` | RISC-V 32-bit | 验证 RISC-V 移植 |
| `native_sim` | x86_64（PC） | 纯软件 CI 测试，无 QEMU 依赖 |

**QEMU 参考实现**：

```
hal_reference/
├── qemu_cortex_m0/
│   ├── bm_hal_uart_qemu.c    # 通过 QEMU semihosting 或 UART 寄存器模拟
│   ├── bm_hal_timer_qemu.c   # 使用 SysTick 或 QEMU 虚拟定时器
│   ├── bm_hal_gpio_qemu.c    # 通过 QEMU 内存映射 GPIO 或 stub
│   ├── bm_hal_wdg_qemu.c     # QEMU 下看门狗为 stub（直接返回）
│   └── startup_qemu_cm0.s    # 启动文件（向量表 + Reset_Handler）
└── qemu_riscv32/
    ├── bm_hal_uart_qemu.c
    ├── bm_hal_timer_qemu.c
    ├── bm_hal_gpio_qemu.c
    ├── bm_hal_wdg_qemu.c
    └── startup_qemu_rv32.s
```

**QEMU 测试集成**：
- 每个 `tests/qemu/` 用例编译为 QEMU 可执行镜像
- 测试框架通过 UART 输出 TAP 格式结果，CI 解析 pass/fail
- 关键测试：事件发布/订阅（含 ISR 路径）、内存池 alloc/free、模块生命周期、Shell 命令回显

**QEMU 仿真构建示例**：
```bash
# Cortex-M0 QEMU
cmake -B build_qemu_cm0 -DBOARD=qemu_cortex_m0 -DHAL_REF=hal_reference/qemu_cortex_m0
cmake --build build_qemu_cm0
qemu-system-arm -M microbit -kernel build_qemu_cm0/zeplod_baremetal.elf -nographic -serial stdio

# RISC-V 32 QEMU
cmake -B build_qemu_rv32 -DBOARD=qemu_riscv32 -DHAL_REF=hal_reference/qemu_riscv32
cmake --build build_qemu_rv32
qemu-system-riscv32 -M sifive_e -kernel build_qemu_rv32/zeplod_baremetal.elf -nographic -serial stdio
```

### 7.4 native_sim（PC 纯软件模拟）

- 完全不依赖 QEMU，在 PC 上编译为本地可执行文件
- UART → `stdin/stdout` 重定向
- Timer → `gettimeofday()` 或 `clock_gettime()`
- GPIO → 内存 stub（可配合 expect 脚本验证写入值）
- WDG → stub（直接返回）
- **用途**：最快反馈的单元测试、CI 基础测试层

---

## 8. 内存模型与资源预算

### 8.1 各层级资源预算表

| 配置 | Flash | RAM | 适用芯片 |
|---|---|---|---|
| `bm-ultra`（仅头文件） | 0.5-1KB | 128-300B | STM8、AVR、8051 |
| `bm-core`（事件+内存池） | 1.5-2.5KB | 256-600B | CH32V003、STM32F030 |
| `bm-core + module` | 2.5-5KB | 512B-1.2KB | STM32F103C8、GD32E230 |
| `bm-core + module + channel` | 4-7KB | 1-2.5KB | nRF51822、CH32V307 |
| `full`（+ shell） | 6-12KB | 1.5-4KB | 任意 32KB+ Flash 芯片 |

> 注：预算含 HAL 参考实现的典型开销。`bm-ultra` 预算基于 SDCC/STM8 实测估算，`core` 及以上基于 ARM GCC `-Os` 估算。每个 release 提供实测资源占用表。

### 8.2 零堆分配原则

- **禁止 `malloc`/`free`**：所有内存在编译期或 `bm_init()` 初期通过静态数组/宏分配
- **事件数据所有权（明确为 publish_copy 语义）**：
  - `bm_event_publish_copy()` / `bm_event_publish_copy_from_isr()`：框架将数据拷贝到队列内联缓冲区或临时槽位，回调接收的 `event->data` 指针仅在回调执行期间有效，回调返回后数据由框架释放。调用方无需管理内存。
  - `bm_event_publish_event()`：调用方构造 `bm_event_t` 并保证其生命周期覆盖整个处理过程（通常用于静态数据或调用方自行管理的内存）。框架不释放 `event->data`。
  - **不存在自动 free**：框架不知道指针来自哪个 pool，也不应猜测。要么框架负责拷贝（`publish_copy`），要么调用方声明生命周期自理（`publish_event`）。
- **栈使用可控**：无递归，无大型栈上数组，所有缓冲区静态分配

### 8.3 配置宏

应用通过 `bm_config.h` 覆盖默认值（复制 `bm_config.h.template`）：

```c
#define BM_CONFIG_MAX_EVENT_TYPES           16
#define BM_CONFIG_MAX_EVENT_SUBSCRIBERS     32
#define BM_CONFIG_MAX_EVENTS_PER_LOOP       8
#define BM_CONFIG_EVENT_PRIORITIES          4
#define BM_CONFIG_MAX_MODULES               8
#define BM_CONFIG_MEMPOOL_MAX_POOLS         4
#define BM_CONFIG_SHELL_BUF_SIZE            64
#define BM_CONFIG_CHANNEL_MAX_CHANNELS      4
#define BM_CONFIG_ULTRA_MAX_EVENT_DATA_SIZE 8
#define BM_CONFIG_ULTRA_QUEUE_DEPTH         8
```

---

## 9. 事件语义对齐与 API 映射

### 9.1 bm-core 与 Zeplod 的 API 映射

| Zeplod API | bm-core API | 差异说明 |
|---|---|---|
| `event_type_t`（`uint8_t`） | `bm_event_type_t`（`uint8_t`） | 范围一致：0–255 |
| `event_callback_t` | `bm_event_callback_t` | 签名一致：`void cb(const bm_event_t *event, void *user_data)` |
| `event_register_type()` | `bm_event_register_type()` | 语义一致 |
| `event_subscribe()` | `bm_event_subscribe()` | 语义一致；bm 版输出 `subscriber_id`，取消订阅按 id 而非回调指针匹配 |
| `event_publish_copy()` / `event_publish_copy_rt()` | `bm_event_publish_copy()` / `bm_event_publish_copy_from_isr()` | 语义一致：显式优先级 + 数据拷贝，调用方无需管理内存 |
| `event_publish_event()` | `bm_event_publish_event()` | 高级用法：调用方保证 `bm_event_t` 生命周期 |
| `event_create()` | `bm_mempool_alloc()` | 显式内存池替代隐式 slab |
| `module_register()` | `BM_MODULE_DEFINE()` | 编译期宏替代运行时注册 |
| `module_start_all()` | `bm_module_start_all()` | 语义一致 |
| `watchdog_feed()` | `bm_wdg_feed()` | 语义一致，新增模块级注册 |
| `data_bus_publish()` / `data_bus_subscribe()` | — | **无映射**：`bm-channel` 是 SPSC 数据通道，不是 Zeplod Data Bus；多消费者、引用计数、retain/release 语义在裸机不实现 |

### 9.2 bm-ultra 与 bm-core 的行为差异

| 行为 | bm-core | bm-ultra |
|---|---|---|
| 事件优先级 | 支持（出队扫描） | 不支持（FIFO） |
| 同一事件多订阅者 | 支持（链表遍历） | 不支持（仅一个回调） |
| 事件数据传递 | `publish_copy`（框架拷贝到队列内联缓冲区，调用方无需管理内存） | 值拷贝（队列内联数组） |
| 取消订阅 | 支持 | 不支持（编译期绑定） |
| 错误码 | 完整 `BM_ERR_*` | 仅 `bm_ultra_init()` 返回状态，publish 无返回值（溢出静默丢弃） |

### 9.3 迁移路径

```
[裸机 Ultra] --升级芯片/资源--> [裸机 Core] --需要模块化--> [裸机 Core+Module]
                                                    |
                                                    v
                                        [Zephyr + Zeplod 完整版]
```

- **Ultra → Core**：将编译期事件绑定改为运行时订阅，引入内存池管理事件数据，获得多订阅者和优先级
- **Core → Core+Module**：将散落的中断/主循环逻辑封装为模块，获得生命周期管理
- **Baremetal → Zephyr**：模块代码基本可复用（模块生命周期 API 对齐），事件系统无缝迁移。`bm-channel` **不是** Zeplod Data Bus，升级到 Zephyr 后需改用 `data_bus` API（命名 channel、多消费者、引用计数）

---

## 10. 构建系统

### 10.1 CMake（32-bit 主流）

```cmake
# 应用 CMakeLists.txt 示例
set(BM_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/zeplod-baremetal/src)
set(BM_HAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/hal_reference/qemu_cortex_m0)

add_executable(app
    main.c
    ${BM_SRC_DIR}/core/bm_event.c
    ${BM_SRC_DIR}/core/bm_mempool.c
    ${BM_SRC_DIR}/core/bm_critical.c
    ${BM_SRC_DIR}/core/bm_wdg.c
    ${BM_SRC_DIR}/module/bm_module.c
    ${BM_HAL_DIR}/bm_hal_uart_qemu.c
    ${BM_HAL_DIR}/bm_hal_timer_qemu.c
    ${BM_HAL_DIR}/bm_hal_wdg_qemu.c
    ${BM_HAL_DIR}/startup_qemu_cm0.s
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/zeplod-baremetal/include
    ${BM_HAL_DIR}
)
```

### 10.2 纯 Makefile（8-bit 工具链友好）

```makefile
# Makefile 示例（SDCC / IAR / AVR-GCC）
BM_ROOT = ../zeplod-baremetal

SRCS = main.c \
       $(BM_ROOT)/src/core/bm_event.c \
       $(BM_ROOT)/src/core/bm_mempool.c \
       $(BM_ROOT)/src/hal/bm_hal_uart_stub.c

INCS = -I$(BM_ROOT)/include

# 仅编译 bm-ultra 时，不需要任何 .c 文件：
# SRCS = main.c
# INCS = -I$(BM_ROOT)/include
# CFLAGS += -DBM_ULTRA_MODE
```

### 10.3 配置方式

- **方式一**：应用层提供 `bm_config.h`，放在 include path 最前面，覆盖默认值
- **方式二**：通过 `-D` 编译标志定义（适合 Makefile 场景）
- **方式三**：CMake 选项（`-DBM_ENABLE_SHELL=ON` 等）

---

## 11. 测试策略

### 11.1 三层测试金字塔

| 层级 | 目标 | 工具 | 运行环境 |
|---|---|---|---|
| **单元测试** | 算法、数据结构、状态机 | Unity + fff（mock） | PC（native_sim） |
| **QEMU 冒烟** | HAL 集成、中断上下文、启动流 | 自定义测试框架（UART 输出 TAP） | QEMU Cortex-M0 / RISC-V |
| **硬件在环** | 真实时序、功耗、外设 | 手动 + 自动化脚本 | 参考开发板 |

### 11.2 单元测试设计（native_sim）

```c
// tests/unit/test_event.c
static void test_callback(const bm_event_t *event, void *user_data) {
    int *count = (int *)user_data;
    (*count)++;
    (void)event;
}

void test_event_publish_and_subscribe(void) {
    int count = 0;
    bm_event_subscriber_id_t id;
    TEST_ASSERT_EQUAL(BM_OK, bm_event_register_type(EVENT_TEST, "TEST"));
    TEST_ASSERT_EQUAL(BM_OK, bm_event_subscribe(EVENT_TEST, test_callback, &count, &id));
    TEST_ASSERT_EQUAL(BM_OK, bm_event_publish_copy(EVENT_TEST, BM_EVENT_PRIO_NORMAL, NULL, 0));
    TEST_ASSERT_EQUAL(1, bm_event_process(1));
    TEST_ASSERT_EQUAL(1, count);
    TEST_ASSERT_EQUAL(BM_OK, bm_event_unsubscribe(EVENT_TEST, id));
}

void test_event_priority_order(void) {
    // 验证高优先级事件先被处理
}

void test_mempool_alloc_free(void) {
    BM_MEMPOOL_DEFINE(pool, struct test_obj, 4);
    struct test_obj *a = bm_mempool_alloc(&pool);
    TEST_ASSERT_NOT_NULL(a);
    bm_mempool_free(&pool, a);
    struct test_obj *b = bm_mempool_alloc(&pool);
    TEST_ASSERT_EQUAL_PTR(a, b);  // 复用同一槽位
}

void test_channel_would_block(void) {
    BM_CHANNEL_DEFINE(ch, uint32_t, 1);
    uint32_t data = 42;
    TEST_ASSERT_EQUAL(BM_OK, bm_channel_publish(&ch, &data));
    TEST_ASSERT_EQUAL(BM_ERR_WOULD_BLOCK, bm_channel_try_read(&ch, &data));
    // 已读走一条，再次读取应返回 WOULD_BLOCK
}
```

### 11.3 QEMU 冒烟测试

每个 QEMU 测试是一个独立的固件镜像，启动后执行预设测试序列，通过 UART 输出：

```
TAP version 13
1..5
ok 1 - event_publish_from_isr
ok 2 - mempool_alloc_under_interrupt_load
ok 3 - module_lifecycle_init_start_stop
ok 4 - shell_echo_command
ok 5 - watchdog_module_timeout
```

CI 解析 TAP 输出判断结果。

### 11.4 测试矩阵

| 测试 / 配置 | ultra | core | core+module | full |
|---|---|---|---|---|
| 单元测试（native_sim） | ✓ | ✓ | ✓ | ✓ |
| QEMU Cortex-M0 | — | ✓ | ✓ | ✓ |
| QEMU RISC-V | — | ✓ | ✓ | ✓ |
| 硬件 STM32F0 | — | ✓ | ✓ | ✓ |
| 硬件 CH32V003 | ✓ | ✓ | — | — |

> 注：`bm-ultra` 的单元测试在 `native_sim` 下编译为普通 PC 程序验证逻辑。`bm-ultra` 暂无对应 QEMU 目标（STM8/8051 无主流 QEMU 支持），需通过真实硬件验证。

---

## 12. 版本管理与发布

### 12.1 版本号策略

与 Zeplod 解耦，独立发版：
- `v0.1.0`：bm-ultra + bm-core（事件 `publish_copy` + 内存池 + 临界区）+ bm-module 最小可用
- `v0.2.0`：+ QEMU 支持（Cortex-M0 / RISC-V）+ 硬件验证 + 资源实测表
- `v0.3.0`：+ bm-channel（轻量 SPSC 数据通道）—— 若资源实测后 Lite 级有余量
- `v0.4.0`：+ bm-shell + 完整 QEMU 支持 —— 同理，资源确认后投入
- `v1.0.0`：API 稳定，与 Zeplod v2.x API 对齐

### 12.2 发布产物

| 产物 | 说明 |
|---|---|
| `zeplod-baremetal-x.y.z.zip` | 完整源码包 |
| `bm-ultra-single.h` | 单文件头文件库（由 `tools/amalgamate_ultra.py` 自动生成，方便直接拖入老项目） |
| `reference_hal_qemu_cm0.tar.gz` | QEMU Cortex-M0 参考 HAL |
| `reference_hal_qemu_riscv32.tar.gz` | QEMU RISC-V 参考 HAL |

---

## 13. 风险与缓解

| 风险 | 表现 | 缓解 |
|---|---|---|
| **双仓库维护成本** | Zeplod bugfix 忘记同步到 baremetal | 建立同步 checklist：每次 Zeplod core 变更时 review 是否影响 baremetal；CI 中增加 API 签名 diff |
| **ultra 与 core 语义漂移** | 用户从 ultra 迁移到 core 时发现事件行为不一致 | 文档中显式列出差异清单（第 9.2 节）；提供迁移示例代码 |
| **8-bit MCU 编译器兼容性** | SDCC、IAR 对 C99 支持不完整，宏语法差异 | ultra 层仅使用 C89 子集 + 常见扩展；CI 中用 SDCC 编译验证；`bm-ultra-single.h` 发布前经 SDCC 编译检查 |
| **QEMU 与真实硬件行为差异** | QEMU 中断时序过于理想，掩盖真实竞态 | 硬件测试覆盖中断密集型场景；QEMU 测试中通过 `bm_hal_timer` 注入随机延迟模拟抖动 |
| **Flash/RAM 预算估不准** | 某芯片声称 32KB Flash，实际编译后超预算 | 每个 release 提供实测资源占用表（按配置矩阵）；预留 20% 缓冲 |
| **HAL 抽象过度** | 为兼容所有架构引入函数指针，导致 8-bit 代码膨胀 | ultra 层无 HAL 抽象，直接操作寄存器；core HAL 用宏内联 + 弱符号，函数指针仅在未内联时生效 |

---

## 14. 里程碑与推荐开发顺序

| 里程碑 | 内容 | 预估工作量 | 优先级 |
|---|---|---|---|
| **M0** | 仓库初始化、目录结构、CMake/Makefile 骨架、native_sim HAL 桩 | 1-2 天 | 必须 |
| **M1** | `bm-ultra` 头文件库 + native_sim 单元测试 + 文档 | 3-5 天 | 必须 |
| **M2** | `bm-core`（事件 `publish_copy` + 内存池 + 临界区 + 错误码）+ native_sim 单元测试 | 5-7 天 | 必须 |
| **M3** | `bm-module`（最小生命周期）+ 生命周期测试 | 3-4 天 | 必须 |
| **M4** | QEMU Cortex-M0 支持 + 冒烟测试 | 3-5 天 | 必须 |
| **M5** | `bm-wdg` + QEMU RISC-V 支持 + 参考 HAL 完善 | 3-5 天 | 必须 |
| **M6** | 硬件验证（STM32F0 / CH32V003）+ 性能基准 + **资源实测** | 7-10 天 | 必须 |
| **M7** | 与 Zeplod API 对齐审查 + 迁移指南 + `v0.2.0` 发布 | 3-5 天 | 必须 |
| **M8** | `bm-channel` + SPSC 通道测试 | 3-4 天 | **暂缓**：等资源实测确认各配置层级 Flash/RAM 余量后再决定是否投入 |
| **M9** | `bm-shell` + Shell 测试 | 3-4 天 | **暂缓**：同理，若 Lite 级实测后余量不足，可推迟到 `v0.4.0` |
| **M10** | `v1.0.0`：API 稳定，与 Zeplod v2.x 完成对齐审查 | 3-5 天 | 最终目标 |

---

## 15. 完成定义（Definition of Done）

本设计文档的实现阶段可视为完成的条件：

1. `bm-ultra` 在 SDCC/STM8 或 AVR-GCC 下编译通过，事件 publish/subscribe 主循环测试通过
2. `bm-core` 在 native_sim 下通过全部单元测试（事件 `publish_copy`、内存池、临界区）
3. QEMU Cortex-M0 和 QEMU RISC-V 下通过冒烟测试（事件 ISR 发布、模块生命周期）
4. 至少一块真实硬件（STM32F0 或 CH32V003）验证通过，并提供实测 Flash/RAM 占用
5. 提供与 Zeplod 的 API 映射表和迁移示例
6. 优先配置层级（ultra / core / core+module）提供实测 Flash/RAM 占用表；`bm-channel` 和 `bm-shell` 的资源占用在投入开发前通过实测确认余量
7. 文档完整：API 参考、HAL 移植指南、配置说明、测试运行手册

---

## 16. 附录：命名规范

- 公共 API：`bm_<module>_<action>()`（如 `bm_event_publish_copy`）
- 内部函数：`_bm_<module>_<action>()` 或 `bm_<module>_<action>_internal()`
- 宏：`BM_<MODULE>_<NAME>`（如 `BM_MEMPOOL_DEFINE`）
- 配置宏：`BM_CONFIG_<NAME>`（如 `BM_CONFIG_MAX_EVENT_TYPES`）
- 类型：`bm_<name>_t`（如 `bm_event_callback_t`）
- HAL 函数：`bm_hal_<periph>_<action>()`（如 `bm_hal_uart_send`）
- 错误码：`BM_ERR_<NAME>`（如 `BM_ERR_WOULD_BLOCK`）

---

*本文档由 Zeplod 架构讨论产生，遵循 Zeplod 设计哲学：静态优先、显式契约、可测试性、渐进升级。*
