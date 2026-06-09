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
│   ├── bm_config.h
│   └── main.c
│
├── core_sensor/              # Example 2: bm-core + bm-module + mempool
│   ├── CMakeLists.txt
│   ├── bm_config.h
│   └── main.c
│
└── full_system/              # Example 3: 全栈（含 wdg、错误处理、事件风暴）
    ├── CMakeLists.txt
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

### 4.5 验证输出
```
Zeplod Example: ultra_blink
LED: ON
LED: OFF
LED: ON
BUTTON: pressed
LED: OFF
...
EXAMPLE_ULTRA: PASS
```

---

## 5. Example 2: core_sensor

### 5.1 目标
展示 `bm-core`（事件系统 + 内存池 + 优先级扫描）与 `bm-module`（生命周期管理）。

### 5.2 业务逻辑
- **模块划分**:
  | 模块 | 优先级 | init | start | stop | deinit |
  |------|--------|------|-------|------|--------|
  | `sensor_mod` | 2 | ✅ | ✅ | ✅ | ✅ |
  | `display_mod` | 1 | ✅ | ✅ | — | — |
  | `logger_mod` | 0 | ✅ | ✅ | — | — |

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
         .priority = BM_EVENT_PRIO_NORMAL, /* 1 */
         .data = d,
         .data_len = sizeof(*d)
     };
     bm_event_publish_event(&ev);
     ```
  3. `logger_mod` 订阅 `EVENT_TEMP`，优先级 `0`（HIGH），打印 `[LOG] temp=XX`。
  4. `display_mod` 订阅 `EVENT_TEMP`，优先级 `1`（NORMAL），打印 `Temp: XX C, Hum: YY %`。
  5. `bm_event_process(4)` 按优先级扫描出队，确保 `[LOG]` 始终先于 `Temp:` 出现。
  6. 事件处理完毕后，`display_mod` 负责 `bm_mempool_free(&sensor_pool, ev->data)`。

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

### 5.5 验证输出
```
Zeplod Example: core_sensor
[mod] sensor_mod init ok
[mod] display_mod init ok
[mod] logger_mod init ok
[mod] all modules started
[LOG] temp=23, hum=45
Temp: 23 C, Hum: 45 %
[LOG] temp=24, hum=46
Temp: 24 C, Hum: 46 %
...
EXAMPLE_CORE: PASS
```
**关键验证点**: `[LOG]` 行必须始终紧邻出现在 `Temp:` 行之前。

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
     - `sensor_mod.start()` 有 20% 概率返回 `BM_ERR_BUSY`。
     - 运行中，`sensor_mod` 有 10% 概率发布 `EVENT_SENSOR_FAULT`（优先级 `0`）。
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

### 6.4 验证输出
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
[LOG] temp=22, hum=44
[COMM] ready
[COMM] frame sent
Temp: 22 C, Hum: 44 %
FAULT: sensor error, code=3
STORM: order OK
...
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
3. QEMU 运行 5 秒后 `kill`。
4. 捕获串口输出并 `grep "EXAMPLE_.*: PASS"`。
5. 汇总报告：
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

每个示例的 `Makefile` 复用框架顶层 `Makefile` 逻辑：

```makefile
# 指向框架根目录（用户拷贝后只需改这一行）
ZEPLOD_ROOT := $(realpath ../..)

include $(ZEPLOD_ROOT)/Makefile

# 追加本示例的源文件与配置
BM_SRCS += main.c
CFLAGS  += -I. -include bm_config.h

TARGET := ultra_blink.elf

all: $(TARGET)
$(TARGET): $(BM_SRCS)
	$(CC) $(CFLAGS) $(BM_SRCS) $(LDFLAGS) -o $@
```

### 8.3 Keil / IAR 导入指南 (`PROJECT_SOURCES.md`)

每个示例的 `PROJECT_SOURCES.md` 明确列出：

```markdown
## Keil / IAR 手动导入步骤

### 1. 源文件（Project → Add Existing Files）
| 文件 | 说明 |
|------|------|
| `main.c` | 本示例入口 |
| `../../src/core/bm_critical.c` | 临界区 |
| `../../src/core/bm_event.c` | 事件系统 |
| `../../src/core/bm_mempool.c` | 内存池 |
| `../../hal_reference/qemu_cortex_m0/startup_qemu_cm0.s` | 启动文件 |
| `../../hal_reference/qemu_cortex_m0/bm_hal_uart_qemu.c` | UART HAL |
| ... | 根据层级递增（详见下方"按层级选择"） |

### 2. 头文件路径（C/C++ → Include Paths）
- `../../include`
- `.` (本目录，用于 `bm_config.h`)

### 3. 编译器选项
- C standard: C99
- Define: 无（`bm_config.h` 已通过 `-include` 或 `#include` 引入）

### 4. 链接器设置
- 使用默认分散加载文件，或指定 `../../hal_reference/qemu_cortex_m0/linker.ld`

### 按层级选择源文件
- **ultra_blink**: 仅需 `bm_ultra.h`（头文件库，无 .c），加 HAL 启动文件。
- **core_sensor**: 增加 `bm_event.c`, `bm_mempool.c`, `bm_critical.c`, `bm_module.c`。
- **full_system**: 再增加 `bm_wdg.c`。
```

### 8.4 框架级集成文档

在框架根目录提供通用指南：

- `docs/porting/keil-integration.md`
  - 如何新建 Keil 工程并添加 zeplod-baremetal
  - 如何根据 `BM_ENABLE_MODULE` / `BM_ENABLE_WDG` 等开关增减源文件
  - `bm_config.h` 的放置建议（工程根目录，优先于框架默认配置）

- `docs/porting/iar-integration.md`
  - 与 Keil 指南结构对称，针对 IAR 的选项做说明（如 `--c99`, 链接器配置 `.icf` vs `.ld`）

### 8.5 辅助脚本 `tools/list_sources.py`

功能：根据当前 CMake 配置开关，自动生成源文件清单。

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
- [x] **边界条件**: core_sensor 的 9B 数据明确超过 `publish_copy` 8B 内联上限，强制触发 mempool 路径。
- [x] **工具链兼容**: 所有示例提供 CMake + Makefile + Keil/IAR 导入指南三种入口。
- [x] **第三方库定位**: 示例目录结构支持 `cp -r` 后独立使用，不依赖框架根构建系统。
