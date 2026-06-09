# Zeplod Baremetal 使用示例设计规格

> **日期**: 2026-06-09  
> **关联计划**: `docs/plan/2026-06-08-zeplod-baremetal-implementation-plan.md` (M0–M7 已完成)  
> **目标**: 提供 3 个独立、可拷贝、QEMU 可运行的渐进式示例，覆盖 bm-ultra → bm-core → 全栈 API。

---

## 1. 设计原则

1. **渐进式学习**: 示例复杂度严格递增，每个示例只引入新的一层概念。
2. **独立可复制**: 每个示例目录是一个"迷你项目"，用户可 `cp -r` 到自己的仓库起步。
3. **真机可验证**: 所有示例编译为 QEMU Cortex-M0 固件，通过 UART0 输出验证结果。
4. **自动化友好**: 每个示例输出固定的 `EXAMPLE_XXX: PASS` 魔法字符串，支持脚本化验证。

---

## 2. 目录结构

```
examples/
├── README.md                 # 示例总览、构建命令、预期输出
├── run_all.sh                # 一键构建+运行+验证脚本（POSIX）
├── run_all.bat               # Windows 批处理版本
│
├── ultra_blink/              # Example 1: bm-ultra 最简
│   ├── CMakeLists.txt
│   ├── Makefile              # 纯 Makefile 备选
│   ├── PROJECT_SOURCES.md    # Keil/IAR 手动导入指南
│   ├── bm_config.h
│   └── main.c
│
├── core_sensor/              # Example 2: bm-core + bm-module + mempool
│   ├── CMakeLists.txt
│   ├── Makefile
│   ├── PROJECT_SOURCES.md
│   ├── bm_config.h
│   └── main.c
│
└── full_system/              # Example 3: 全栈（含 wdg、错误处理、事件风暴）
    ├── CMakeLists.txt
    ├── Makefile
    ├── PROJECT_SOURCES.md
    ├── bm_config.h
    └── main.c
```

---

## 3. 构建系统（ZEPLOD_ROOT 方案）

每个示例的 `CMakeLists.txt` 顶部定义：

```cmake
set(ZEPLOD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.."
    CACHE PATH "Path to zeplod-baremetal framework root")
```

后续所有框架路径均通过 `${ZEPLOD_ROOT}` 引用：
- 头文件：`${ZEPLOD_ROOT}/include`
- 核心源文件：`${ZEPLOD_ROOT}/src/core/bm_*.c`
- 模块源文件：`${ZEPLOD_ROOT}/src/module/bm_module.c`
- HAL 启动文件：`${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/startup_qemu_cm0.s`
- 链接脚本：`${ZEPLOD_ROOT}/hal_reference/qemu_cortex_m0/linker.ld`
- 工具链文件：`${ZEPLOD_ROOT}/cmake/toolchain-arm-none-eabi.cmake`

**拷贝适配**: 用户将示例复制到独立项目后，仅需修改 `ZEPLOD_ROOT` 一行即可指向自己的框架副本。

---

## 4. Example 1: ultra_blink

### 4.1 目标
展示 `bm-ultra` 纯头文件 API：事件发布、回调表绑定、FIFO 队列处理。

### 4.2 业务逻辑
- **初始化**: `bm_ultra_init()`
- **主循环**:
  - 软件延时计数模拟 1s 周期，发布 `EVENT_TICK`（无数据）。
  - 每隔 3 秒额外发布 `EVENT_BUTTON_PRESS`（数据: `uint8_t btn_id = 0`）。
  - 调用 `bm_ultra_process()` 消费队列。
- **回调**:
  - `on_tick`: 切换虚拟 LED 状态，通过 UART 输出 `LED: ON` / `LED: OFF`。
  - `on_button`: 输出 `BUTTON: pressed`。

### 4.3 依赖声明
代码注释中必须明确区分：
```c
/* === 业务逻辑仅使用 bm-ultra API === */
/* 以下 startup / linker / hal_uart 是任何裸机固件的最小基础设施，
 * 不属于 bm-ultra 框架本身。 */
```

### 4.4 配置 (`bm_config.h`)
```c
#define BM_CONFIG_ULTRA_MAX_EVENT_TYPES     4
#define BM_CONFIG_ULTRA_QUEUE_DEPTH         8
#define BM_CONFIG_ULTRA_MAX_EVENT_DATA_SIZE 4
```

### 4.5 验证输出与终止条件

**终止条件**: 3 次 `EVENT_TICK` + 1 次 `EVENT_BUTTON_PRESS` 处理完毕后输出 `EXAMPLE_ULTRA: PASS`，随后进入 `while(1)` 空转。

```
Zeplod Example: ultra_blink
LED: ON
LED: OFF
LED: ON
BUTTON: pressed
LED: OFF
EXAMPLE_ULTRA: PASS
```

---

## 5. Example 2: core_sensor

### 5.1 目标
展示 `bm-core`（事件系统 + 内存池 + 优先级扫描）与 `bm-module`（生命周期管理）。

### 5.2 业务逻辑
- **模块划分**:
  | 模块 | 模块优先级 | init | start | stop | deinit | 说明 |
  |------|-----------|------|-------|------|--------|------|
  | `display_mod` | 0 | ✅ | ✅ | — | — | 打印温度，**负责 `mempool_free`** |
  | `logger_mod` | 1 | ✅ | ✅ | — | — | 打印 `[LOG]` |
  | `sensor_mod` | 2 | ✅ | ✅ | ✅ | ✅ | 发布事件 |

- **数据流**:
  1. `sensor_mod.start()` 启动后，每 2 秒生成一个 `sensor_data_t`：
     ```c
     typedef struct {
         uint16_t temp;      /* 2B */
         uint16_t humidity;  /* 2B */
         uint32_t timestamp; /* 4B */
         uint8_t  status;    /* 1B */
     } sensor_data_t;         /* 总计 9B */
     ```
  2. 由于 `9B > 8B`（`publish_copy` 内联上限），示例必须使用内存池：
     ```c
     sensor_data_t *d = bm_mempool_alloc(&sensor_pool);
     *d = reading;
     bm_event_t ev = {
         .type = EVENT_TEMP,
         .priority = 1, /* NORMAL: 0=HIGH, 1=NORMAL, 2=LOW */
         .data = d,
         .data_len = sizeof(*d)
     };
     bm_event_publish_event(&ev);
     ```
  3. `display_mod` 与 `logger_mod` 均在 `init()` 中订阅 `EVENT_TEMP`。
     - 模块按 priority 升序 init → `display_mod(0)` 先 init 并订阅 → `logger_mod(1)` 后 init 并订阅。
     - 订阅链表为 LIFO（后订阅的先执行）→ **`logger_mod` 先执行**，`display_mod` 后执行。
  4. `logger_mod` 回调打印 `[LOG] temp=XX`；`display_mod` 回调打印 `Temp: XX C, Hum: YY %`。
  5. `bm_event_process(4)` 按事件优先级扫描出队（此处仅一种事件类型，出队顺序即入队顺序）。
  6. `display_mod` 后执行，**负责 `bm_mempool_free(&sensor_pool, ev->data)`**。

> **关键澄清**: 事件优先级（`ev.priority`）控制的是**不同事件类型之间的出队顺序**；同一事件类型的多个订阅者之间无优先级，按 LIFO 顺序执行。

### 5.3 模块生命周期
```c
bm_module_init_all();   // 排序后依次 init
bm_module_start_all();  // 排序后依次 start
// ... 主循环 ...
bm_module_stop_all();   // 逆序 stop
bm_module_deinit_all(); // 逆序 deinit
```

### 5.4 配置 (`bm_config.h`)
```c
#define BM_CONFIG_MAX_EVENT_TYPES           8
#define BM_CONFIG_MAX_EVENT_SUBSCRIBERS     16
#define BM_CONFIG_EVENT_QUEUE_SIZE          16
#define BM_CONFIG_EVENT_PRIORITIES          4
#define BM_CONFIG_MAX_MODULES               4
#define BM_CONFIG_MEMPOOL_MAX_POOLS         2
```

### 5.5 验证输出与终止条件

**终止条件**: 2 次 `EVENT_TEMP` 完整处理（sensor → publish → process → logger 回调 → display 回调 → mempool free）后输出 `EXAMPLE_CORE: PASS`。

```
Zeplod Example: core_sensor
[mod] display_mod init ok
[mod] logger_mod init ok
[mod] sensor_mod init ok
[mod] all modules started
[LOG] temp=23, hum=45
Temp: 23 C, Hum: 45 %
[LOG] temp=24, hum=46
Temp: 24 C, Hum: 46 %
EXAMPLE_CORE: PASS
```

**关键验证点**:
1. `[LOG]` 行始终紧邻出现在 `Temp:` 行之前（验证 LIFO 订阅者顺序：logger 后订阅、先执行）。
2. `Temp:` 行出现后无崩溃（验证 display_mod 正确执行了 `mempool_free`）。

---

## 6. Example 3: full_system

### 6.1 目标
覆盖除 bm-ultra 外的全部核心 API：模块错误处理、看门狗、多优先级事件风暴。

### 6.2 业务逻辑
- **模块划分**:
  | 模块 | 优先级 | init | start | stop | deinit | 特殊行为 |
  |------|--------|------|-------|------|--------|----------|
  | `sensor_mod` | 2 | ✅ | ✅ (20% 失败率) | ✅ | ✅ | 偶发 `BM_ERR_BUSY` |
  | `comm_mod` | 1 | ✅ | ✅ | ✅ | ✅ | 发布 `EVENT_COMM_READY` (prio 0) |
  | `display_mod` | 3 | ✅ | ✅ | — | — | 订阅 EVENT_TEMP |
  | `fault_mod` | 0 | ✅ | ✅ | — | — | 订阅 EVENT_SENSOR_FAULT |

- **数据流**:
  1. `sensor_mod` 正常发布 `EVENT_TEMP`（同 core_sensor，使用 mempool）。
  2. **错误注入**（`#define ENABLE_FAULT_TEST 1`）:
     - `sensor_mod.start()` 首次调用返回 `BM_ERR_BUSY`，重试后成功（确定性，便于 CI 验证错误处理路径）。
     - 运行中，第 5 个 sensor 周期触发一次 `EVENT_SENSOR_FAULT`（优先级 `0`）。
     - `fault_mod` 订阅该事件，打印 `FAULT: sensor error, code=X`。
  3. `comm_mod` 订阅 `EVENT_TEMP`，发送前发布 `EVENT_COMM_READY`（优先级 `0`）。
  4. **看门狗**:
     - `main()` 中注册 3 个模块：`bm_wdg_register("sensor")`, `bm_wdg_register("comm")`, `bm_wdg_register("display")`。
     - 主循环中每个模块喂自己：`bm_wdg_feed_module("sensor")` 等。
     - 最后 `bm_wdg_feed()` 喂硬件狗。
  5. **事件风暴**（`#define ENABLE_STORM_TEST 1`）:
     - 主循环一次性发布 5 个事件，优先级分别为 `0, 1, 2, 3, 1`。
     - 验证 `_queue_pop_highest_prio` 出队顺序严格为 `0, 1, 1, 2, 3`。
     - 输出 `STORM: order OK`。

### 6.3 配置 (`bm_config.h`)
```c
#define BM_CONFIG_MAX_EVENT_TYPES           16
#define BM_CONFIG_MAX_EVENT_SUBSCRIBERS     32
#define BM_CONFIG_EVENT_QUEUE_SIZE          32
#define BM_CONFIG_EVENT_PRIORITIES          4
#define BM_CONFIG_MAX_MODULES               8
#define BM_CONFIG_MEMPOOL_MAX_POOLS         4
#define BM_CONFIG_MAX_WDG_MODULES           4
```

### 6.4 验证输出与终止条件

**终止条件**: Storm test 通过 + 至少 1 次 WDG feed + 至少 1 次 fault 处理完毕后输出 `EXAMPLE_FULL: PASS`，随后进入 `while(1)`。

```
Zeplod Example: full_system
[mod] sensor_mod init ok
[mod] comm_mod init ok
[mod] display_mod init ok
[mod] fault_mod init ok
[WARN] sensor_mod start failed, rc=-5
[mod] retry sensor_mod start ok
[mod] all modules started
[WDG] registered: sensor, comm, display
Temp: 22 C, Hum: 44 %
[COMM] ready
[COMM] frame sent
FAULT: sensor error, code=3
STORM: order OK
EXAMPLE_FULL: PASS
```

---

## 7. 辅助文件

### 7.1 `examples/README.md`
内容纲要：
1. 三个示例的递进关系图。
2. 前置要求（`arm-none-eabi-gcc`, `qemu-system-arm`）。
3. 每个示例的构建命令：
   ```bash
   cd examples/ultra_blink
   cmake -B build -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-arm-none-eabi.cmake .
   cmake --build build
   qemu-system-arm -M microbit -kernel build/ultra_blink.elf -nographic
   ```
4. 预期输出片段（与上文验证输出一致）。
5. "如何拷贝示例到自己的项目"说明（修改 `ZEPLOD_ROOT`）。

### 7.2 `examples/run_all.sh`
功能：
1. 遍历 `ultra_blink`, `core_sensor`, `full_system`。
2. 对每个示例执行 cmake 构建。
3. QEMU 运行，通过 `timeout` 等待 `EXAMPLE_.*: PASS` 魔法字符串出现（超时 10s）。
4. 汇总报告：
   ```
   ultra_blink  ... PASS
   core_sensor  ... PASS
   full_system  ... PASS
   All examples passed.
   ```

---

## 8. 第三方库集成支持（Keil / IAR / Makefile）

框架定位为第三方库，示例本身即是"集成模板"。每个示例必须提供三种构建方式的入口。

### 8.1 目录结构更新

```
examples/
├── ultra_blink/
│   ├── CMakeLists.txt          # 方式1: CMake (主推)
│   ├── Makefile                # 方式2: 纯 Makefile
│   ├── PROJECT_SOURCES.md      # 方式3: Keil/IAR 手动导入指南
│   ├── bm_config.h
│   └── main.c
```

`core_sensor/` 与 `full_system/` 结构相同。

### 8.2 Makefile 示例

> **前置条件**：当前根目录 `Makefile` 仅为源文件清单（`echo` 模式），需先扩展为可实际编译（或提供 `examples/common/Makefile.baremetal.mk` 共享片段）。本 Makefile 示例展示的是「示例独立编译」的最终目标结构。

**ultra_blink**（不链接 core `.c`）：

```makefile
# 指向框架根目录（用户拷贝后只需改这一行）
ZEPLOD_ROOT := $(realpath ../..)

CC      := arm-none-eabi-gcc
CFLAGS  := -mcpu=cortex-m0 -mthumb -Os -std=c99 \
           -I$(ZEPLOD_ROOT)/include -I. -include bm_config.h
LDFLAGS := -T$(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/linker.ld -nostartfiles

SRCS := main.c \
        $(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/startup_qemu_cm0.s \
        $(ZEPLOD_ROOT)/hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c

TARGET := ultra_blink.elf

all: $(TARGET)
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $@
```

**core_sensor / full_system** 在此基础上追加 `bm_event.c`、`bm_mempool.c` 等（详见 `PROJECT_SOURCES.md`）。

### 8.3 GCC/QEMU 路径导入指南 (`PROJECT_SOURCES.md`)

每个示例的 `PROJECT_SOURCES.md` 面向 **GCC + QEMU** 场景（Keil/IAR 真机移植细节见 §8.4）：

```markdown
## 手动导入步骤（GCC / QEMU）

### 1. 源文件
| 文件 | 说明 |
|------|------|
| `main.c` | 本示例入口 |
| `../../src/core/bm_critical.c` | 临界区（core/full 需要） |
| `../../src/core/bm_event.c` | 事件系统（core/full 需要） |
| `../../src/core/bm_mempool.c` | 内存池（core/full 需要） |
| `../../src/module/bm_module.c` | 模块生命周期（core/full 需要） |
| `../../src/core/bm_wdg.c` | 看门狗（full 需要） |
| `../../hal_reference/qemu_cortex_m0/startup_qemu_cm0.s` | 启动文件 |
| `../../hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c` | UART HAL |
| `../../hal_reference/qemu_cortex_m0/bm_hal_timer_qemu.c` | Timer HAL（可选） |
| `../../hal_reference/qemu_cortex_m0/bm_hal_wdg_qemu.c` | WDG HAL（可选） |

### 2. 头文件路径
- `../../include`
- `.` (本目录，用于 `bm_config.h`)

### 3. 编译器选项
- C standard: C99
- 其他：`-mcpu=cortex-m0 -mthumb -Os`

### 4. 链接器设置（GCC）
- 指定 `../../hal_reference/qemu_cortex_m0/linker.ld`

### 按层级选择源文件
- **ultra_blink**: 仅需 `bm_ultra.h`（头文件库，无 .c），加 `startup_qemu_cm0.s` + `bm_hal_uart_qemu.c`。
- **core_sensor**: 追加 `bm_critical.c`, `bm_event.c`, `bm_mempool.c`, `bm_module.c`。
- **full_system**: 再追加 `bm_wdg.c`。
```

### 8.4 Keil / IAR 真机移植指南

> **交付物**：当前 `docs/porting/` 仅含 `.gitkeep`，实施阶段需新建以下文档。

**`docs/porting/keil-integration.md`**：
- 如何新建 Keil 工程并添加 zeplod-baremetal
- 如何根据 `BM_ENABLE_MODULE` / `BM_ENABLE_WDG` 等开关增减源文件
- `bm_config.h` 的放置建议（工程根目录，优先于框架默认配置）
- **链接器差异**：Keil 使用 `.sct` (Scatter) 文件，而非 GCC 的 `.ld`。如需精确控制内存布局，需将 `linker.ld` 的 `MEMORY/SECTIONS` 语义手工转为 `.sct`，或直接使用 Keil 默认分散加载并调整 RAM/FLASH 范围。

**`docs/porting/iar-integration.md`**：
- 与 Keil 指南结构对称
- **链接器差异**：IAR 使用 `.icf` 文件，语法与 GCC `.ld` 不同。建议先使用 IAR 默认 linker configuration，再按需调整 RAM/FLASH 范围。
- 编译器选项备注：`--c99` 或 `-e`（取决于 IAR 版本）

### 8.5 辅助脚本 `tools/list_sources.py`

功能：根据当前 CMake 配置开关，自动生成源文件清单。

**配置开关 → 示例 对照表**：

| 示例 | `--enable-module` | `--enable-wdg` | 生成的源文件 |
|------|-------------------|----------------|-------------|
| `ultra_blink` | `OFF` | `OFF` | 仅 HAL + startup |
| `core_sensor` | `ON` | `OFF` | + `bm_critical.c` `bm_event.c` `bm_mempool.c` `bm_module.c` |
| `full_system` | `ON` | `ON` | 再 + `bm_wdg.c` |

```bash
# 生成适合 Keil 导入的源文件列表
python tools/list_sources.py --format=keil --enable-module=ON --enable-wdg=ON

# 输出示例:
# ../../src/core/bm_critical.c
# ../../src/core/bm_event.c
# ../../src/core/bm_mempool.c
# ../../src/module/bm_module.c
# ../../src/core/bm_wdg.c
```

支持格式：`keil`（相对路径列表）、`iar`、`makefile`（变量赋值形式）。

---

## 9. 自检清单

- [x] **Placeholder scan**: 无 TBD/TODO；无 "add appropriate error handling"。
- [x] **Type consistency**: `bm_event_priority_t` 值越小优先级越高，与框架实现一致。
- [x] **API 语义一致**: `bm_event_publish_event` 的生命周期由调用方/消费方保证（core_sensor 中 display_mod 负责 free）。
- [x] **订阅者顺序**: 框架 `bm_event_subscribe()` 为 LIFO 链表插入，无订阅者优先级；spec 中 core_sensor 的模块 priority 分配（display=0, logger=1）确保了 logger 后订阅、先执行。
- [x] **边界条件**: core_sensor 的 9B 数据明确超过 `publish_copy` 8B 内联上限，强制触发 mempool 路径。
- [x] **工具链兼容**: 所有示例提供 CMake + Makefile + Keil/IAR 导入指南三种入口。
- [x] **第三方库定位**: 示例目录结构支持 `cp -r` 后独立使用，不依赖框架根构建系统。
- [x] **确定性验证**: full_system 的 fault/storm 为计数器触发，非概率随机，CI 可稳定复现。
