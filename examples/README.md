# Zeplod Baremetal Examples

这里包含四个逐步扩展的示例：

| 示例 | 重点 |
|---|---|
| `ultra_blink` | 头文件版 `bm-ultra` 事件队列 |
| `core_sensor` | 事件、内存池和模块生命周期 |
| `full_system` | 多模块、事件优先级和看门狗 |
| `interrupt_demo` | SysTick、外设中断和 ISR 发布事件 |

## 目录约定

```text
examples/
  common/                 # 示例共享的轻量输出工具
  cmake/                  # QEMU 示例的共享 CMake 逻辑
  examples.txt            # 运行脚本使用的示例清单
  qemu_example.mk         # 共享 Makefile 规则
  <example>/
    main.c                # 只保留应用流程和框架用法
    bm_config.h           # 该示例的资源配置
    CMakeLists.txt        # 声明组件和 HAL，避免复制构建细节
```

平台寄存器代码不应放进 `main.c`。例如 `interrupt_demo` 将 nRF51 TIMER1
实现隔离在 `interrupt_timer.c`，便于替换为真实目标平台。

## 构建与运行

需要：

- `arm-none-eabi-gcc`
- `qemu-system-arm`
- CMake 3.20+
- Windows: `mingw32-make`; Linux/macOS: `make`

运行全部示例：

```powershell
.\run_all.ps1
```

```bash
./run_all.sh
```

运行单个示例：

```powershell
.\run_single.ps1 core_sensor
```

```bash
./run_single.sh core_sensor
```

构建产物统一放在 `examples/build/windows/` 或 `examples/build/unix/`。
每个示例成功后会输出
`EXAMPLE_...: PASS`。

## 设计要点

- 异步事件优先使用 `bm_event_publish_copy()`，避免把短生命周期栈指针放入队列。
- 内存池对象在事件复制完成后由发布方立即释放，订阅者只读数据。
- 示例共享工具只处理展示性细节，不封装框架 API，以免掩盖核心用法。
- 硬件移植所需文件见 [PORTING.md](PORTING.md)。
