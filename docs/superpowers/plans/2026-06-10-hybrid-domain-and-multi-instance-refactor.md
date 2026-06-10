# Hybrid-Domain + Multi-Instance Refactor Implementation Plan

> 日期：2026-06-10
> 状态：实施计划 v2
> 对应设计文档：
> - `docs/superpowers/specs/2026-06-10-hybrid-domain-architecture-design.md`
> - `docs/superpowers/specs/2026-06-10-multi-instance-generalization-design.md`
> 目标：按阶段将双域架构和多实例模型落地为可编译、可测试、可验证的代码

---

## 1. 重构目标与范围

### 1.1 总体目标

将 `zeplod-baremetal` 从纯事件驱动框架重构为支持以下能力的平台：

1. **三层执行模型**：Hardware HRT（PWM/ADC 硬件事件）、Scheduled HRT（定时器 ISR 分发）、SRT（主循环事件驱动）。
2. **跨域数据交换**：`bm_snapshot` 三缓冲邮箱，供 HRT → SRT 传递最新值。
3. **增强 HAL**：PWM、ADC、COMP 使用资源句柄，支持多实例绑定不同外设。
4. **多实例控制层**：`bm_ctrl_inst` + `bm_resource_claim_t` + 显式实例表，支持多轴伺服、多通道 BMS 等产品的静态实例化。
5. **同步域**：`bm_sync_domain_t` 支持多轴 PWM 硬件同步。

### 1.2 非目标

- 不改写现有 `bm_event`、`bm_module`、`bm_channel`、`bm_shell` 的 API 语义。
- 不引入动态内存分配。
- 不支持运行时热插拔实例。
- 不为所有芯片家族一次性提供完整 HAL 实现；优先 STM32G4，其他芯片通过后续迭代补充。

### 1.3 兼容性策略

- 现有示例和单元测试在不启用 `BM_ENABLE_HRT`、`BM_ENABLE_TICKER`、`BM_ENABLE_CTRL_INST` 时必须保持通过。
- 新增模块默认关闭，通过 CMake 选项和 `bm_config.h` 宏显式启用。
- `bm_critical` 保持现有 `PRIMASK` 行为，分级临界区作为可选扩展在支持的平台上启用。

---

## 2. 目录结构变更

```text
zeplod-baremetal/
├── include/
│   ├── bm_hrt.h                    # 新增：Scheduled HRT API
│   ├── bm_snapshot.h               # 新增：三缓冲邮箱（头文件组件）
│   ├── bm_ticker.h                 # 新增：SRT 周期任务分发
│   ├── bm_ctrl_inst.h              # 新增：多实例控制模型
│   ├── bm_resource.h               # 新增：资源 Claim 定义
│   ├── bm_sync.h                   # 新增：同步域
│   ├── bm_hal_pwm.h                # 新增：PWM HAL
│   ├── bm_hal_adc.h                # 新增：ADC HAL
│   ├── bm_hal_comp.h               # 新增：比较器 HAL
│   ├── bm_hal_encoder.h            # 新增：编码器 HAL（配合多轴示例）
│   ├── bm_hal_memory.h             # 新增：平台内存屏障
│   ├── bm_hal_timer.h              # 现有：扩展时间源接口
│   └── bm_hal_critical.h           # 现有：扩展 BASEPRI 路径
├── src/
│   ├── hrt/
│   │   ├── bm_hrt.c                # 新增：Scheduled HRT 实现
│   │   └── bm_ticker.c             # 新增：SRT ticker 实现
│   ├── ctrl/
│   │   ├── bm_ctrl_inst.c          # 新增：控制实例生命周期、资源冲突检测
│   │   └── bm_sync.c               # 新增：同步域实现（平台通用逻辑）
│   ├── core/
│   │   ├── bm_event.c              # 改造：可选分级临界区
│   │   └── bm_mempool.c            # 改造：可选分级临界区
│   └── hal/
│       └── ...                     # 桩实现移动到 hal_reference/
├── hal_reference/
│   ├── qemu_cortex_m0/             # 新增/扩展：HRT、PWM、ADC、COMP 桩
│   ├── qemu_riscv32/               # 新增/扩展：同上
│   ├── native_sim/                 # 新增/扩展：模拟时间源、虚拟 PWM/ADC
│   └── stm32g4/                    # 新增：STM32G4 真实硬件参考 HAL
├── examples/
│   ├── hrt_servo_stub/             # 新增：单轴伺服 HRT 示例
│   ├── hrt_bms_coulomb/            # 新增：BMS 库仑计示例
│   ├── multi_axis_sync/            # 新增：3 轴同步示例
│   └── multi_channel_bms/          # 新增：16 通道 BMS 示例
├── tests/
│   ├── unit/
│   │   ├── test_hrt.c              # 新增
│   │   ├── test_snapshot.c         # 新增
│   │   ├── test_ticker.c           # 新增
│   │   ├── test_ctrl_inst.c        # 新增
│   │   └── test_resource_claim.c   # 新增
│   └── qemu/
│       ├── hrt_servo_stub/         # 新增
│       ├── hrt_bms_coulomb/        # 新增
│       └── multi_axis_sync/        # 新增
└── docs/
    └── ...                         # 本文档 + 设计文档
```

---

## 3. 阶段规划

### Phase 0：前置准备（1-2 天）

**目标**：建立重构基线，确保现有代码可测试、可度量。

| # | 任务 | 负责人 | 输出 |
|---|------|--------|------|
| 0.1 | 为现有 `bm_event`、`bm_mempool`、`bm_channel` 添加 native_sim 单元测试覆盖率基准 | TBD | 覆盖率报告 |
| 0.2 | 在 QEMU Cortex-M0 上建立指令/仿真周期测量脚本；真实时间测量留给目标板 | TBD | `tools/measure_wcet.py` |
| 0.3 | 确认 CI 流程支持新 CMake 选项和新的 `tests/unit` 用例 | TBD | 绿 CI |
| 0.4 | 创建功能分支 `feature/hybrid-domain` | TBD | Git 分支 |

**验证标准**：
- 现有所有测试在 `main` 分支通过。
- WCET 脚本能在 QEMU 上输出可重复的仿真周期数，并明确标记为回归指标而非真实硬件 WCET。

---

### Phase 1：混合域基础设施与核心隔离（4-5 天）

**目标**：落地临界区分级、现有核心隔离、`bm_hrt`、`bm_ticker`、`bm_snapshot`。

#### 1.1 临界区扩展

**新增/修改文件**：
- `include/bm_hal_critical.h`：声明 `bm_hal_critical_enter_below()` / `bm_hal_critical_exit_below()`
- `hal_reference/stm32g4/bm_hal_critical_stm32g4.c`：Cortex-M4 BASEPRI 实现
- `tests/unit/fakes/bm_hal_priority_mask_fake.c`：仅用于嵌套状态单元测试
- `bm_config.h.template`：增加 `BM_CONFIG_ENABLE_PRIORITY_MASK`
- 各平台配置：声明 `BM_HAL_HAS_PRIORITY_MASK` 能力

**实现要点**：
```c
bm_irq_state_t bm_hal_critical_enter_below(uint8_t threshold);
void bm_hal_critical_exit_below(bm_irq_state_t previous_state);
```

- `previous_state` 由调用者持有，禁止用全局变量保存。
- API 不返回“是否支持”；支持能力在编译期确定。
- 当 `BM_CONFIG_ENABLE_PRIORITY_MASK=1` 且 `BM_HAL_HAS_PRIORITY_MASK=0` 时，构建直接失败。
- Cortex-M0/M0+、当前 QEMU micro:bit 和 native_sim 默认声明能力为 `0`，不提供假的 BASEPRI 实现。

**验证标准**：
- 单元测试验证嵌套分级临界区的恢复正确性。
- 配置测试验证“不支持平台 + 启用分级临界区”必然构建失败。
- STM32G4 目标板验证：在 SRT 临界区内，高优先级 HRT 中断仍能正常触发。

#### 1.2 现有核心的可选分级临界区

**修改文件**：
- `src/core/bm_event.c`
- `src/core/bm_mempool.c`
- `src/channel/bm_channel.c`

**实现要点**：
- 增加内部 `BM_CRITICAL_ENTER()` / `BM_CRITICAL_EXIT(state)` 包装。
- `BM_CONFIG_ENABLE_PRIORITY_MASK=0` 时继续使用现有 PRIMASK/平台临界区。
- 启用分级临界区时使用 `BM_CONFIG_HRT_PRIORITY_THRESHOLD`。
- 高于阈值的 HRT ISR 禁止访问 `bm_event`、`bm_mempool` 和 `bm_channel`。

**验证标准**：
- 默认配置下全部现有测试不变。
- 分级路径的嵌套状态恢复测试通过。
- 后续 HRT 测试确认高优先级路径不调用任何 SRT 容器。

#### 1.3 bm_hrt（Scheduled HRT）

**新增文件**：
- `include/bm_hrt.h`
- `src/hrt/bm_hrt.c`
- `include/bm_hal_timer.h`：为现有 Timer HAL 增加停止接口
- `hal_reference/native_sim/bm_hal_timer_native.c`、QEMU/STM32F0/RISC-V 对应实现：补充 `bm_hal_timer_stop()`

**实现要点**：
```c
int bm_hrt_init(const bm_hrt_slot_t *slots, uint32_t slot_count);
int bm_hrt_start(void);
void bm_hrt_stop(void);
void bm_hrt_reset(void);

int bm_hal_timer_init(uint32_t tick_hz);
void bm_hal_timer_set_callback(void (*callback)(void));
void bm_hal_timer_stop(void);
uint32_t bm_hal_timer_get_ticks(void);
uint32_t bm_hal_timer_get_freq(void);
```

- 固定容量数组存储 Slot（容量 `BM_CONFIG_HRT_MAX_SLOTS`）。
- `bm_hrt_init()` 校验并复制静态 Slot；`bm_hrt_start()` 安装 callback 并配置现有 Timer HAL。
- HAL callback 在定时器 ISR 中调用一次有界分发；默认配置为 100μs/Tick。
- `bm_hrt_stop()` 停止 Timer HAL 并清除 callback。
- `bm_hrt_reset()` 清空 Slot 和运行状态，用于测试隔离及控制层初始化失败回滚。
- `bm_ticker` 与 Scheduled HRT 共享同一单调 Tick，不重复初始化硬件定时器；未启用 HRT 时由应用初始化 Timer HAL。
- 各平台按真实外设时钟计算 reload，并校验 `tick_hz` 可表达；`bm_hal_timer_get_freq()` 返回实际配置频率。
- `bm_hal_timer_stop()` 必须先禁用中断源并等待必要的硬件同步，再清除 callback，返回后不得再调用旧 callback。
- 处理 32 位时间回绕：使用无符号差值和 `int32_t` 比较。
- Deadline miss 时调用 `bm_hrt_deadline_missed_hook(slot)`，默认空实现，应用可覆盖。
- 不在 ISR 中调用 `bm_event_*`。

**验证标准**：
- 单元测试：3 个 Slot 不同周期，验证调度顺序和 deadline miss 检测。
- QEMU 测试：100μs Slot 运行 1000 个周期，调度次数和仿真周期偏差满足预期；该结果只用于回归，不作为真实 jitter 证明。

#### 1.4 bm_ticker（SRT 周期任务）

**新增文件**：
- `include/bm_ticker.h`
- `src/hrt/bm_ticker.c`

**实现要点**：
```c
int bm_ticker_init(const bm_ticker_slot_t *slots, uint32_t slot_count);
int bm_ticker_poll(void);
uint32_t bm_ticker_get_dropped(uint32_t slot_index);
void bm_ticker_reset(void);
```

- 在主循环中使用现有 `bm_hal_timer_get_ticks()` / `bm_hal_timer_get_freq()` 换算经过时间。
- 使用无符号 Tick 差值处理计数器回绕。
- 到期时调用 `bm_event_publish_copy(evt, prio, NULL, 0)`，**不携带数据**。
- 按 Slot 记录丢失计数，并通过只读查询 API 暴露。

**验证标准**：
- 单元测试：10ms/50ms/100ms Slot 验证事件发布时序。
- 与 `bm_event` 集成测试：事件队列满时正确累加 `dropped`。
- 单元测试：计数器接近 `UINT32_MAX` 时周期触发仍正确。

#### 1.5 bm_snapshot（三缓冲邮箱）

**新增文件**：
- `include/bm_snapshot.h`（纯头文件组件）
- `include/bm_hal_memory.h`
- native_sim、QEMU、STM32G4 对应的内存屏障实现

**实现要点**：
- `BM_SNAPSHOT_DEFINE(name, type)` + `BM_SNAPSHOT_INITIALIZER`
- `BM_SNAPSHOT_PUBLISH(box, value_ptr)`
- `BM_SNAPSHOT_READ(box, out_ptr)`
- `bm_memory_barrier_release()` / `bm_memory_barrier_full()` 在独立的 `bm_hal_memory.h` 中声明并由平台实现。

**验证标准**：
- 每帧使用 `{version, payload, inverse_version, checksum}`，验证消费者从不观察到混合版本。
- native_sim 使用确定性抢占钩子，在发布和读取协议的每个关键步骤插入“ISR”。
- 主机线程压力测试仅在平台屏障使用真实原子实现时启用，并明确它不替代目标 MCU 抢占测试。

---

### Phase 2：增强 HAL（4-5 天）

**目标**：定义并实现对伺服/BMS 至关重要的 HAL 接口，先在 native_sim/QEMU 上跑通，再补充 STM32G4 真实实现。

#### 2.1 HAL 接口定义

**新增文件**：
- `include/bm_hal_pwm.h`
- `include/bm_hal_adc.h`
- `include/bm_hal_comp.h`
- `include/bm_hal_encoder.h`

**接口要点**：
```c
// PWM
int bm_hal_pwm_set_duty(const bm_hal_pwm_t *pwm, uint32_t phase, uint16_t duty);
int bm_hal_pwm_enable_outputs(const bm_hal_pwm_t *pwm, int enable);
void bm_hal_pwm_request_safe_state(const bm_hal_pwm_t *pwm);
int bm_hal_pwm_bind_update(const bm_hal_pwm_t *pwm,
                           const bm_hal_hrt_binding_t *binding);

// ADC
int bm_hal_adc_read_injected(const bm_hal_adc_t *adc,
                             uint32_t rank, uint16_t *value);
int bm_hal_adc_bind_complete(const bm_hal_adc_t *adc,
                             const bm_hal_hrt_binding_t *binding);

// COMP
int bm_hal_comp_clear_latch(const bm_hal_comp_t *comp);

// Encoder
int bm_hal_encoder_read(const bm_hal_encoder_t *enc, int32_t *value);
```

- PWM/ADC bind API 接受 `binding == NULL` 作为解绑请求。
- 解绑必须先禁用对应中断源，再清除 callback/context。
- 安装 binding 不得隐式使能输出、启动 ADC 或打开中断；启动动作由实例 `ops->start` 控制。

#### 2.2 native_sim HAL 桩

**新增文件**：
- `hal_reference/native_sim/bm_hal_pwm_sim.c`
- `hal_reference/native_sim/bm_hal_adc_sim.c`
- `hal_reference/native_sim/bm_hal_comp_sim.c`
- `hal_reference/native_sim/bm_hal_encoder_sim.c`

**实现要点**：
- PWM：内存中的 duty 数组，支持软件触发 `update` 回调。
- ADC：预置波形数据，按 rank 返回。
- COMP：可配置阈值，超过时触发 Break。
- Encoder：模拟正交编码器计数。

#### 2.3 QEMU Cortex-M0 桩

**新增文件**：
- `hal_reference/qemu_cortex_m0/bm_hal_pwm_qemu.c`
- `hal_reference/qemu_cortex_m0/bm_hal_adc_qemu.c`
- `hal_reference/qemu_cortex_m0/bm_hal_comp_qemu.c`
- `hal_reference/qemu_cortex_m0/bm_hal_encoder_qemu.c`

**实现要点**：
- 使用 QEMU 内存映射外设或简单寄存器模拟。
- 验证中断绑定路径（PWM update → HRT callback）。

#### 2.4 STM32G4 真实 HAL 参考实现

**新增目录**：
- `hal_reference/stm32g4/`

**实现要点**：
- 使用 STM32 HAL 库或寄存器直接操作（推荐后者以减少代码体积）。
- 实现：
  - TIM1/TIM8 高级定时器（三相 PWM、死区、Break）
  - ADC1/ADC2 注入组（由 PWM TRGO 触发）
  - COMP1-COMP7（输出到 TIM1/8/20 Break）
  - TIM2/TIM3 编码器接口
  - TIM6/TIM7 作为 HRT Tick 源

**验证标准**：
- native_sim：HRT 回调被正确触发，PWM duty 值可被读取。
- QEMU：中断路径端到端通过，GPIO 翻转可被脚本检测。
- STM32G4（真实硬件）：
  - PWM 输出波形正确（示波器）。
  - ADC 采样时刻与 PWM 谷底对齐（逻辑分析仪）。
  - COMP Break 响应 <1μs（示波器）。

---

### Phase 3：多实例控制层（4-5 天）

**目标**：落地 `bm_ctrl_inst`、`bm_resource_claim_t`、`bm_sync_domain_t`。

#### 3.1 资源 Claim 模块

**新增文件**：
- `include/bm_resource.h`
- `src/ctrl/bm_resource.c`

**实现要点**：
```c
typedef enum {
    BM_RESOURCE_PWM,
    BM_RESOURCE_ADC_GROUP,
    BM_RESOURCE_TIMER,
    BM_RESOURCE_DMA_CHANNEL,
    BM_RESOURCE_GPIO,
    BM_RESOURCE_COMP,
    BM_RESOURCE_IRQ
} bm_resource_kind_t;

typedef enum {
    BM_RESOURCE_EXCLUSIVE,
    BM_RESOURCE_OWNER,
    BM_RESOURCE_SHARED_READ,
    BM_RESOURCE_SHARED_COORDINATED
} bm_resource_access_t;

typedef struct {
    bm_resource_kind_t kind;
    uintptr_t key;
    bm_resource_access_t access;
    uintptr_t share_group;
    const char *name;
} bm_resource_claim_t;

int bm_resource_check_conflicts(const bm_resource_claim_t *const *claims,
                                const uint32_t *claim_counts,
                                uint32_t instance_count);
```

- O(n²) 启动期检查，`instance_count` 和 `claim_counts[i]` 均有静态上限。
- `EXCLUSIVE` 与同一物理资源的任何其他 Claim 冲突。
- `OWNER + SHARED_READ` 仅在 `share_group` 相同且非零时允许；存在 Reader 时必须恰好有一个 Owner。
- `SHARED_COORDINATED` 仅允许同一非零 `share_group` 内互相共享。

**验证标准**：
- 单元测试：
  - 两实例独占同一 PWM → 冲突。
  - 一个 Owner + 两个同组 Reader 共享 ADC 规则组 → 允许。
  - Reader 无 Owner、两个 Owner、Owner/Reader 组不同 → 冲突。
  - 协调共享同一 Sync Domain → 允许。
  - 共享读 + 协调共享冲突 → 冲突。

#### 3.2 控制实例生命周期

**新增文件**：
- `include/bm_ctrl_inst.h`
- `src/ctrl/bm_ctrl_inst.c`

**实现要点**：
```c
int bm_ctrl_init_all(const bm_ctrl_inst_t *const *instances, uint32_t count);
int bm_ctrl_start_all(const bm_ctrl_inst_t *const *instances, uint32_t count);
void bm_ctrl_safe_stop_all(const bm_ctrl_inst_t *const *instances,
                           uint32_t count);
const bm_ctrl_inst_t *bm_ctrl_find(const bm_ctrl_inst_t *const *instances,
                                   uint32_t count, uint32_t id);
```

- 全表校验顺序：
  1. ID 唯一性
  2. Slot 数量 + Claim 数量不超过容量
  3. Slot 周期合法（Scheduled 周期可被 HRT Tick 整除）
  4. 资源冲突检测
  5. 组装 binding 池和完整 Scheduled HRT 表
  6. 一次性调用 `bm_hrt_init()`
  7. 按表顺序调用 `ops->init`
  8. 为已初始化实例绑定 Hardware Slot
- 失败或显式停止时先执行 `bm_hrt_stop()`，再逆序调用 `safe_stop` 关闭输出和硬件触发源，随后调用 `bind_hardware(instance, NULL)` 解绑已绑定 Slot，最后执行 `bm_hrt_reset()` 并清空 binding 池。
- 固定容量池分配 `bm_ctrl_binding_t`（容量 `BM_CONFIG_MAX_CTRL_SLOTS`）。
- 全部 Scheduled Slot 被转换到一张固定容量 `bm_hrt_slot_t` 数组，随后由 `bm_ctrl_init_all()` 一次性调用 `bm_hrt_init()`。
- 使用 `bm_ctrl_inst` 时，应用不再直接调用 `bm_hrt_init()`；未启用控制层的应用仍可独立使用 `bm_hrt_init()`。

**验证标准**：
- 单元测试：验证初始化成功/失败路径、逆序 safe_stop。
- 单元测试：在实例 init、Hardware bind 和 `bm_hrt_init` 各阶段注入失败，验证无残留 callback、Slot 或已使能输出；失败后触发模拟 IRQ 不得调用旧 binding。
- 单元测试：验证 `bm_ctrl_find` 在有效/无效 ID 下的行为。

#### 3.3 调度绑定

**实现要点**：
- `BM_CTRL_SLOT_HARDWARE` 必须提供 `bind_hardware(instance, binding)`；产品适配函数负责转换 `instance->resources` 并调用 `bm_hal_pwm_bind_update()` 或 `bm_hal_adc_bind_complete()`
- `BM_CTRL_SLOT_SCHEDULED` → 通过 `bm_ctrl_run_binding` 适配器转换为 `bm_hrt_slot_t`，全部组装完成后一次性初始化 `bm_hrt`
- 适配器固定形式：
  ```c
  static void bm_ctrl_run_binding(void *context) {
      const bm_ctrl_binding_t *binding = context;
      binding->slot->step(binding->instance);
  }
  ```
- 框架不检查或解引用产品专用的 `void *resources` 布局。
- Scheduled Slot 的 `bind_hardware` 必须为 `NULL`；Hardware Slot 必须非空。
- `bind_hardware(instance, NULL)` 必须可重复调用，用于失败回滚和正常停机。

**验证标准**：
- 单元测试：Hardware Slot 绑定到模拟 PWM，验证 update 事件触发控制函数。
- 单元测试：多个实例的 Scheduled Slot 被组装成完整 HRT 表，`bm_hrt_init()` 只调用一次并按周期执行。

#### 3.4 同步域

**新增文件**：
- `include/bm_sync.h`
- `src/ctrl/bm_sync.c`

**实现要点**：
```c
int bm_sync_configure(const bm_sync_domain_t *domain);
int bm_sync_arm(const bm_sync_domain_t *domain);
int bm_sync_trigger(const bm_sync_domain_t *domain);
void bm_sync_safe_stop(const bm_sync_domain_t *domain);
```

- 平台通用逻辑检查：成员数、相位数组长度、周期兼容性。
- HAL 特定实现（如 STM32G4 主从定时器配置）放在 `hal_reference/stm32g4/bm_sync_hal.c`。

**验证标准**：
- native_sim：验证配置/触发/安全停止的调用顺序。
- QEMU：验证软件配置和触发调用顺序；当前 Cortex-M0 模型不用于证明真实多定时器相位。
- STM32G4：示波器测量多路 PWM 同步精度 <100ns。

---

### Phase 4：构建集成与配置矩阵（1-2 天）

**目标**：把新增组件接入现有目标结构，并让非法选项组合在配置阶段明确失败。

#### 4.1 CMake 集成

**修改文件**：
- `CMakeLists.txt`

**实现要点**：
```cmake
option(BM_ENABLE_HRT "Enable bm-hrt" OFF)
option(BM_ENABLE_TICKER "Enable bm-ticker" OFF)
option(BM_ENABLE_CTRL_INST "Enable multi-instance control layer" OFF)
option(BM_ENABLE_SYNC "Enable sync domain" OFF)

if(BM_ENABLE_CTRL_INST AND NOT BM_ENABLE_HRT)
    message(FATAL_ERROR "BM_ENABLE_CTRL_INST requires BM_ENABLE_HRT")
endif()

if(BM_ENABLE_SYNC AND NOT BM_ENABLE_CTRL_INST)
    message(FATAL_ERROR "BM_ENABLE_SYNC requires BM_ENABLE_CTRL_INST")
endif()

if(BM_ENABLE_HRT)
    add_library(bm_hrt STATIC src/hrt/bm_hrt.c)
    target_link_libraries(bm_hrt PUBLIC bm_core)
    target_link_libraries(bm_framework INTERFACE bm_hrt)
endif()

if(BM_ENABLE_TICKER)
    add_library(bm_ticker STATIC src/hrt/bm_ticker.c)
    target_link_libraries(bm_ticker PUBLIC bm_core)
    target_link_libraries(bm_framework INTERFACE bm_ticker)
endif()

if(BM_ENABLE_CTRL_INST)
    add_library(bm_ctrl_inst STATIC src/ctrl/bm_ctrl_inst.c src/ctrl/bm_resource.c)
    target_link_libraries(bm_ctrl_inst PUBLIC bm_hrt bm_core)
    if(BM_ENABLE_SYNC)
        target_sources(bm_ctrl_inst PRIVATE src/ctrl/bm_sync.c)
    endif()
    target_link_libraries(bm_framework INTERFACE bm_ctrl_inst)
endif()
```

**配置矩阵**：

| HRT | Ticker | Ctrl | Sync | 期望 |
|-----|--------|------|------|------|
| OFF | OFF | OFF | OFF | 默认构建通过 |
| ON | OFF | OFF | OFF | 独立 Scheduled HRT |
| OFF | ON | OFF | OFF | 独立 SRT ticker |
| ON | 任意 | ON | OFF | 多实例控制层 |
| ON | 任意 | ON | ON | 多实例 + Sync |
| OFF | 任意 | ON | 任意 | 配置失败 |
| 任意 | 任意 | OFF | ON | 配置失败 |

**验证标准**：
- CI 对所有合法组合完成 configure、build 和适用测试。
- 非法组合在 CMake configure 阶段失败，并输出明确原因。

---

### Phase 5：示例与验证（5-7 天）

#### 5.1 hrt_servo_stub（单轴伺服 HRT 示例）

**新增目录**：
- `examples/hrt_servo_stub/`

**实现要点**：
- 使用 native_sim 和 QEMU Cortex-M0 运行。
- 模拟三相电流采样 → FOC 伪算法 → PWM duty 更新。
- Hardware HRT Slot 绑定到 ADC complete 事件。
- Scheduled HRT Slot 1kHz 速度环。
- `bm_ticker` 100ms 发布位置事件到 SRT。

**验证标准**：
- native_sim：运行 10 秒，无内存错误，输出日志符合预期。
- QEMU：测量电流环周期 100μs ±1μs（仿真周期数）。
- 输出 `EXAMPLE_HRT_SERVO_STUB: PASS`。

#### 5.2 hrt_bms_coulomb（BMS 库仑计示例）

**新增目录**：
- `examples/hrt_bms_coulomb/`

**实现要点**：
- `pack_sampler` 实例：1kHz 总电流采样，通过 `bm_snapshot` 发布电流值。
- 单个 `cell_0` 实例：100ms SRT 周期读取电流快照并累加库仑计。
- 模拟过流：COMP 触发 → PWM Break → 安全状态。

**验证标准**：
- native_sim：给定恒定电流 1A，1 秒后库仑计 ≈ `1/3600Ah`（约 `0.000278Ah`，在离散积分误差范围内）。
- 过流触发后，PWM 在 1 个仿真 Tick 内进入安全状态。

#### 5.3 multi_axis_sync（3 轴同步示例）

**新增目录**：
- `examples/multi_axis_sync/`

**实现要点**：
- 3 个伺服实例，各绑定不同的模拟 PWM。
- 1 个 Sync Domain，所有轴挂载到同一主定时器。
- 触发后验证三轴 PWM 计数器同步启动。

**验证标准**：
- native_sim：三轴启动时间戳相同。
- QEMU：三个软件观测点在同一触发序列中执行；真实相位精度只在目标板测量。

#### 5.4 multi_channel_bms（16 通道 BMS 示例）

**新增目录**：
- `examples/multi_channel_bms/`

**实现要点**：
- `pack_sampler` 实例 + 16 个 `cell_n` 实例。
- 验证资源 Claim 无冲突。
- SRT 周期读取电压快照并检测过压。

**验证标准**：
- `bm_ctrl_init_all()` 成功返回 `BM_OK`。
- 16 个通道均正确报告状态。

---

### Phase 6：测试覆盖与文档（3-4 天）

#### 6.1 单元测试

**新增文件**：
- `tests/unit/test_hrt.c`
- `tests/unit/test_snapshot.c`
- `tests/unit/test_ticker.c`
- `tests/unit/test_ctrl_inst.c`
- `tests/unit/test_resource_claim.c`
- `tests/unit/test_sync.c`

**覆盖目标**：
- `bm_hrt`：正常调度、deadline miss、时间回绕、Slot 容量超限。
- `bm_snapshot`：版本/反版本/校验和一致性、确定性抢占点、目标 MCU ISR 抢占。
- `bm_ticker`：多 Slot 周期触发、事件队列满、丢失计数。
- `bm_ctrl_inst`：初始化成功/失败、safe_stop 逆序、ID 查找。
- `bm_resource`：独占冲突、Owner/Reader 配对、协调共享、缺失/重复 Owner、混合冲突。
- `bm_sync`：配置校验、触发顺序、安全停止。

#### 6.2 QEMU 冒烟测试

**新增目录**：
- `tests/qemu/hrt_servo_stub/`
- `tests/qemu/hrt_bms_coulomb/`
- `tests/qemu/multi_axis_sync/`

**验证目标**：
- 每个示例编译为 QEMU 镜像并输出 TAP 结果。
- CI 解析 TAP，确保所有 `ok`。

#### 6.3 真实硬件验证（STM32G4）

**验证目标**：
- `hrt_servo_stub` 在 NUCLEO-G474RE 上运行：
  - PWM 波形 20kHz，死区 500ns（示波器）。
  - ADC 触发与 PWM 谷底对齐（逻辑分析仪）。
  - 电流环 10kHz，jitter <100ns。
- `hrt_bms_coulomb` 运行：
  - 1kHz 电流采样稳定。
  - 过流保护 <1μs。

#### 6.4 文档补充

**新增/更新文件**：
- `docs/api/bm_hrt.md`
- `docs/api/bm_snapshot.md`
- `docs/api/bm_ticker.md`
- `docs/api/bm_ctrl_inst.md`
- `docs/api/bm_resource.md`
- `docs/api/bm_sync.md`
- `docs/api/bm_hal_memory.md`
- `docs/migration/core-to-hybrid.md`（现有 Core 用户迁移到双域架构的指南）
- `docs/porting/hal-pwm-adc-comp.md`（HAL 移植指南）

---

## 4. 时间估算

| Phase | 内容 | 预估工期 | 关键路径 |
|-------|------|---------|---------|
| 0 | 前置准备 | 1-2 天 | ✅ |
| 1 | 混合域基础设施与核心隔离 | 4-5 天 | ✅ |
| 2 | 增强 HAL | 4-5 天 | ✅ |
| 3 | 多实例控制层 | 4-5 天 | ✅ |
| 4 | 构建集成与配置矩阵 | 1-2 天 | ✅ |
| 5 | 示例与验证 | 5-7 天 | ✅ |
| 6 | 测试覆盖与文档 | 3-4 天 | ✅ |
| **合计** | | **22-30 天** | |

> 注：真实硬件验证（STM32G4）依赖于硬件到手时间。没有完成硬件验证时，只能声明 Framework Software Complete，不能声明 STM32G4 Hardware Qualified 或承诺真实时序指标。

---

## 5. 风险与缓解

| 风险 | 影响 | 缓解 |
|-----|------|------|
| **BASEPRI 仅在 Cortex-M3+ 可用** | M0/M0+ 无法实现分级临界区 | 默认关闭 `BM_CONFIG_ENABLE_PRIORITY_MASK`；平台通过 `BM_HAL_HAS_PRIORITY_MASK` 声明能力，不支持的组合在构建阶段失败 |
| **HAL 参考实现碎片化** | 每个芯片家族都需要独立实现 PWM/ADC/COMP | 优先完成 STM32G4 作为标杆；其他芯片通过社区贡献或后续里程碑补充；HAL 接口保持稳定 |
| **用户误在 HRT 中调用 `bm_event_publish`** | 编译期无法禁止 | 代码审查清单 + 静态分析规则（如 grep 检查 HRT 回调中的 `bm_event`）+ 文档明确禁止 |
| **资源冲突检查遗漏共享资源的所有者** | 多个实例共享 ADC 规则组，都尝试初始化 | 引入 `BM_RESOURCE_OWNER`；Reader 存在时要求同组恰好一个 Owner，`EXCLUSIVE` 不参与共享 |
| **Sync Domain 硬件触发路由复杂** | 不同 STM32 型号的 ITR 映射不同 | 通用框架只校验能力，具体路由由 `hal_reference/stm32g4/` 实现；其他型号复制并修改参考实现 |
| **Phase 5 示例开发耗时超预期** | FOC/BMS 算法本身复杂 | 示例中 FOC 使用伪算法（开环或简化 SVPWM），专注于框架时序验证，不追求真实电机控制性能 |
| **现有测试因核心改造而失败** | `bm_event` 临界区改造引入回归 | Phase 1 改造前建立完整测试基线；每次改造后立即跑全部现有测试；使用 `BM_CONFIG_ENABLE_PRIORITY_MASK=0` 作为默认安全模式 |

---

## 6. 回滚策略

1. **按 Phase 提交**：每个 Phase 完成时独立提交，避免一次性大 PR。
2. **功能分支保护**：`feature/hybrid-domain` 分支上的合并不需要立即合并到 `main`，直到 Phase 6 完成并通过全部测试。
3. **CMake 选项隔离**：新增模块默认关闭，即使代码存在问题也不会影响默认构建。
4. **关键路径回滚**：若某 Phase 严重延期或无法达到验证标准，可裁剪对应示例（如跳过 `multi_channel_bms`），不影响核心架构交付。

---

## 7. 完成定义（Definition of Done）

完成状态分为两个层级。Framework Software Complete 是软件合并门槛；STM32G4 Hardware Qualified 是发布真实硬实时指标的额外门槛。

### 7.1 Framework Software Complete：代码层面
- [x] `bm_hrt`、`bm_ticker`、`bm_snapshot`、`bm_ctrl_inst`、`bm_resource`、`bm_sync` 全部实现并通过单元测试。
- [x] `bm_hal_pwm.h`、`bm_hal_adc.h`、`bm_hal_comp.h`、`bm_hal_encoder.h` 接口稳定，native_sim 桩完整；QEMU 示例通过链接 native_sim 仿真源 + TIMER1 HAL。
- [x] `bm_hal_memory.h` 在 native_sim、QEMU 和 STM32G4 上有与各自执行模型匹配的实现。
- [x] 所有现有 Timer HAL 实现支持 `bm_hal_timer_stop()`，停止后 callback 不再执行。
- [x] `bm_event`、`bm_mempool`、`bm_channel` 在 `BM_CONFIG_ENABLE_PRIORITY_MASK=0` 时保持原有行为并通过全部测试。
- [x] CMake 新增 `BM_ENABLE_HRT`、`BM_ENABLE_TICKER`、`BM_ENABLE_CTRL_INST`、`BM_ENABLE_SYNC` 选项。
- [x] 所有合法 CMake 组合通过；非法依赖组合在 configure 阶段明确失败。
- [x] 零堆分配原则未被破坏；新增模块无 `malloc`/`free`。

### 7.2 示例层面
- [x] `hrt_servo_stub` 在 native_sim 和 QEMU 上输出 `EXAMPLE_HRT_SERVO_STUB: PASS`。
- [x] `hrt_bms_coulomb` 在 native_sim 和 QEMU 上输出 `EXAMPLE_HRT_BMS_COULOMB: PASS`。
- [x] `multi_axis_sync` 在 native_sim 和 QEMU 上输出 `EXAMPLE_MULTI_AXIS_SYNC: PASS`。
- [x] `multi_channel_bms` 在 native_sim 上输出 `EXAMPLE_MULTI_CHANNEL_BMS: PASS`。

### 7.3 STM32G4 Hardware Qualified
- [ ] STM32G4 上 `hrt_servo_stub` 电流环 jitter <100ns。
- [ ] STM32G4 上 `hrt_bms_coulomb` 过流保护响应 <1μs。
- [ ] PWM/ADC 触发相位、编译器版本、优化参数、系统时钟和测量方法记录在验证报告中。
- [ ] 未满足本节时，发布说明不得声称上述硬实时指标已经得到保证。

### 7.4 文档层面
- [x] API 参考文档覆盖所有新增公共头文件（`docs/api/`）。
- [x] `docs/migration/core-to-hybrid.md` 完成，指导现有用户迁移。
- [x] `docs/porting/hal-pwm-adc-comp.md` 完成，指导 HAL 移植。
- [x] 设计与实施计划文档保持同步，无矛盾。

### 7.5 CI 层面
- [x] 所有 native_sim 单元测试通过。
- [x] 所有 QEMU 冒烟测试通过（`tests/qemu/run_all.sh` + 四个示例 `run_qemu.sh`）。
- [ ] 代码风格检查通过（若仓库有配置）。
- [ ] 静态分析无高危告警（Cppcheck/Clang Static Analyzer）。

---

## 8. 推荐开发顺序

建议按以下顺序执行，每个 Phase 都以前一 Phase 为基础：

```text
Phase 0 → Phase 1 → Phase 2（native_sim 优先）→ Phase 3 → Phase 4 → Phase 5（native_sim 优先）→ Phase 6
           ↓                                      ↓                              ↓
      保持默认 CI 绿色                      验证配置矩阵                    逐步加入 QEMU
```

**关键建议**：
- Phase 1 包含核心临界区改造，完成后立即跑默认配置全量回归。
- Phase 2 的 native_sim 桩应优先于 QEMU/真实硬件实现，保证开发和测试反馈速度。
- Phase 3 完成后立即执行 Phase 4 配置矩阵，避免错误依赖延迟暴露。
- Phase 5 的示例应从小做起（`hrt_servo_stub` 单轴），逐步增加复杂度（`multi_axis_sync`、`multi_channel_bms`）。

---

*本计划作为重构实施的路线图，应根据实际开发进展每周 Review 一次，必要时调整 Phase 范围和时间估算。*
