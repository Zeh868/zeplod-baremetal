# Zeplod 库集成指南

Zeplod 目录布局类比 **FreeRTOS**：

```text
Source/          库内核（类比 FreeRTOS/Source/）
include/         公共头文件
portable/        平台相关（类比 FreeRTOS/portable/）
Demo/            示例（类比 FreeRTOS/Demo/）
```

集成分两步，顺序固定：

```text
① 移植（Port）     复制 portable/template/bm_port.c，对接 Cube / SDK
② 库集成           Source/ 源码进工程，或链接 libbm_*.a
```

```text
┌─────────────────────────────────────────┐
│  你的应用（main、业务、厂商 startup/HAL）   │
├─────────────────────────────────────────┤
│  Port（bm_port.c，自 template 复制）      │
├─────────────────────────────────────────┤
│  Zeplod 库（Source/ + include/）         │
├─────────────────────────────────────────┤
│  bm_config.h（类比 FreeRTOSConfig.h）    │
└─────────────────────────────────────────┘
```

| FreeRTOS | Zeplod |
|----------|--------|
| `Source/*.c` | `Source/core/`、`Source/hal/`、`Source/hybrid/` |
| `FreeRTOSConfig.h` | `bm_config.h` |
| `portable/.../port.c` | [`portable/template/bm_port.c`](../../portable/template/bm_port.c) |
| 源码 / 预编译库 | 方式 A 源码 / 方式 B 静态库 |

---

## 第一步：移植（Port）

见 [port.md](port.md)。模板：[`portable/template/bm_port.c`](../../portable/template/bm_port.c)  
参考实现：`portable/register_<mcu>/`（开发测试可 `BM_BACKEND=native_sim`）

---

## 第二步：库集成

### 方式 A — 源码集成

```bash
python tools/list_sources.py --profile event --format keil
python tools/list_sources.py --profile event --list-includes --format keil
```

**CMake：**

```cmake
include(.../cmake/zeplod.cmake)
zeplod_configure(ROOT ... PROFILE event BACKEND external CONFIG bm_config.h)
target_sources(app PRIVATE Core/Src/bm_port.c)
zeplod_link(app)
```

片段：[`cmake/integration-snippet/CMakeLists.append.txt`](../../cmake/integration-snippet/CMakeLists.append.txt)

### 方式 B — 静态库

见 [static-lib.md](static-lib.md)（`cmake/static-lib/`）。

---

## PROFILE

| PROFILE | 内容 |
|---------|------|
| `minimal` | 事件、内存池、日志 |
| `event` | + 模块、看门狗 |
| `servo` | + HRT、ticker、ctrl_inst |
| `full` | 全开 |

---

## 厂商工程

| 场景 | 文档 |
|------|------|
| STM32 CubeMX | [stm32-cubemx.md](stm32-cubemx.md) |
| NXP MCUXpresso | [nxp-mcuxpresso.md](nxp-mcuxpresso.md) |
