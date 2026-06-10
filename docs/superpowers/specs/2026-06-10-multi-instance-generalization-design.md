# Multi-Instance Generalization Design（多实例泛化架构设计）

> 日期：2026-06-10
> 状态：设计阶段 v2
> 范围：zeplod-baremetal 的静态多实例控制模型
> 目标：在同一 MCU 上复用控制算法，同时显式隔离状态、配置、调度和硬件资源

---

## 1. 设计范围

多轴伺服、多通道 BMS 和多相电源都可以表达为：

```text
实例 = 算法入口
     + 私有运行状态
     + 只读初始配置
     + 硬件资源句柄
     + HRT/SRT 调度声明
     + 资源占用声明
```

本设计解决的是静态内存所有权、资源冲突和调度上下文问题，不承诺：

- 在单核 MCU 上从 CPU HardFault 中隔离并继续运行其他实例。
- 自动证明任意用户算法可重入。
- 自动推导所有芯片的定时器触发路由。
- 运行时热插拔实例。

---

## 2. 核心原则

1. **显式注册表**：实例由应用提供 `const bm_ctrl_inst_t *` 数组，不依赖链接器段、构造函数或 `__COUNTER__`。
2. **稳定 ID**：实例 ID 由产品配置显式赋值，并在初始化时检查唯一性。
3. **统一回调上下文**：底层 HRT 使用 `void (*)(void *)`；控制实例层通过适配器把实例传给类型化控制函数。
4. **通用资源声明**：资源冲突检查不解引用 `void *resources`，而是检查标准化资源 Claim。
5. **静态容量**：启动期可以把显式表注册到固定容量数组，但不使用堆分配。
6. **功能故障隔离**：每个实例拥有独立状态和故障记录；硬件 Break 的影响范围由实际外设布线决定。
7. **C99 基线**：宏和示例遵守仓库当前 C99 构建标准。

---

## 3. 控制实例模型

### 3.1 数据结构

```c
typedef struct bm_ctrl_inst bm_ctrl_inst_t;

typedef void (*bm_ctrl_step_fn_t)(const bm_ctrl_inst_t *instance);

typedef enum {
    BM_CTRL_SLOT_HARDWARE,
    BM_CTRL_SLOT_SCHEDULED
} bm_ctrl_slot_kind_t;

typedef struct {
    bm_ctrl_slot_kind_t kind;
    uint32_t period_us;
    bm_hrt_trigger_t trigger;
    bm_ctrl_step_fn_t step;
    int (*bind_hardware)(const bm_ctrl_inst_t *instance,
                         const bm_hal_hrt_binding_t *binding);
    const char *name;
} bm_ctrl_slot_t;

typedef struct {
    int (*init)(const bm_ctrl_inst_t *instance);
    int (*start)(const bm_ctrl_inst_t *instance);
    void (*safe_stop)(const bm_ctrl_inst_t *instance);
} bm_ctrl_ops_t;

struct bm_ctrl_inst {
    uint32_t id;
    const char *name;
    void *state;
    const void *config;
    const void *resources;
    const bm_ctrl_slot_t *slots;
    uint32_t slot_count;
    const bm_resource_claim_t *claims;
    uint32_t claim_count;
    const bm_ctrl_ops_t *ops;
};
```

`state`、`config` 和 `resources` 保留应用类型；框架不尝试猜测其布局。

### 3.2 显式实例表

```c
static const bm_ctrl_inst_t *const app_instances[] = {
    &servo_x,
    &servo_y,
    &pack_main
};

#define APP_INSTANCE_COUNT \
    ((uint32_t)(sizeof(app_instances) / sizeof(app_instances[0])))
```

生命周期 API 接收表和数量，不隐式寻找“所有已定义实例”：

```c
int bm_ctrl_init_all(const bm_ctrl_inst_t *const *instances, uint32_t count);
int bm_ctrl_start_all(const bm_ctrl_inst_t *const *instances, uint32_t count);
void bm_ctrl_safe_stop_all(const bm_ctrl_inst_t *const *instances,
                           uint32_t count);
const bm_ctrl_inst_t *bm_ctrl_find(
    const bm_ctrl_inst_t *const *instances,
    uint32_t count,
    uint32_t id);
```

`bm_ctrl_init_all()` 先完成 ID、容量、Slot 和 Claim 的全表校验，再组装 HRT 表，之后才执行实例 HAL 初始化和 Hardware Slot 绑定。初始化、启动失败或显式停止时，框架先停止 Scheduled HRT，再按逆序调用 `safe_stop` 使输出和硬件触发源进入安全状态，随后解绑 Hardware Slot、重置 `bm_hrt`，最后清空内部 binding 池。应用不得忽略返回值；每个 `safe_stop` 必须在未初始化、部分初始化和已启动状态下都安全且幂等。

---

## 4. 调度绑定

### 4.1 控制层适配器

控制 Slot 使用 `bm_ctrl_step_fn_t`，底层 HRT 使用 `bm_hrt_callback_t`。框架通过一个固定适配器统一 ABI：

```c
typedef struct {
    const bm_ctrl_inst_t *instance;
    const bm_ctrl_slot_t *slot;
} bm_ctrl_binding_t;

static void bm_ctrl_run_binding(void *context) {
    const bm_ctrl_binding_t *binding =
        (const bm_ctrl_binding_t *)context;
    binding->slot->step(binding->instance);
}
```

`bm_ctrl_init_all()` 从固定容量池中分配 `bm_ctrl_binding_t`，容量为 `BM_CONFIG_MAX_CTRL_SLOTS`。它先把所有 Scheduled Slot 转换为一张完整的 `bm_hrt_slot_t` 数组，再一次性调用 `bm_hrt_init()`。使用控制实例层时，应用不得再次直接调用 `bm_hrt_init()`；不使用控制实例层的应用仍可独立初始化 `bm_hrt`。这是有界启动期组装，不是堆分配。

### 4.2 Hardware HRT Slot

`BM_CTRL_SLOT_HARDWARE` 通过 Slot 的 `bind_hardware` 函数绑定。该函数由产品适配层实现，可安全地把 `instance->resources` 转换为实际类型，再调用 PWM 或 ADC HAL。传入 `binding == NULL` 时必须执行解绑。框架不会猜测 `void *resources` 的布局，也不把 Hardware Slot 加入软件扫描表。

```c
static int bind_current_adc(const bm_ctrl_inst_t *instance,
                            const bm_hal_hrt_binding_t *binding) {
    const servo_resources_t *resources =
        (const servo_resources_t *)instance->resources;
    return bm_hal_adc_bind_complete(resources->adc, binding);
}
```

Scheduled Slot 的 `bind_hardware` 必须为 `NULL`；Hardware Slot 必须提供非空绑定函数。

### 4.3 Scheduled HRT Slot

`BM_CTRL_SLOT_SCHEDULED` 转换为 `bm_hrt_slot_t`，加入固定容量调度表。初始化时校验：

- 周期可由 HRT Tick 表达。
- 总 Slot 数没有超过容量。
- 单实例没有重复绑定同一触发源。
- 平台 HAL 支持请求的触发类型。

---

## 5. 资源声明与冲突检测

### 5.1 标准化 Claim

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
```

`key` 由 HAL 为物理资源生成稳定标识。它不能简单使用配置结构体地址，因为两个描述符可能指向同一外设。
`share_group` 仅用于 `BM_RESOURCE_SHARED_COORDINATED`；允许共享的 Claim 必须使用同一个非零组标识。

示例：

```c
static const bm_resource_claim_t servo_x_claims[] = {
    { BM_RESOURCE_PWM, PWM_KEY_TIM1, BM_RESOURCE_EXCLUSIVE, 0u,
      "TIM1 PWM" },
    { BM_RESOURCE_ADC_GROUP, ADC_KEY_1_INJ, BM_RESOURCE_EXCLUSIVE, 0u,
      "ADC1 injected" },
    { BM_RESOURCE_TIMER, TIMER_KEY_TIM2, BM_RESOURCE_EXCLUSIVE, 0u,
      "TIM2 encoder" },
    { BM_RESOURCE_COMP, COMP_KEY_1, BM_RESOURCE_EXCLUSIVE, 0u, "COMP1" }
};
```

### 5.2 冲突规则

相同 `kind + key` 的 Claim：

| A | B | 结果 |
|---|---|-----|
| Exclusive | 任意 | 冲突 |
| Owner | Owner | 冲突 |
| Owner | Shared read | `share_group` 相同且非零时允许 |
| Shared read | Shared read | 允许 |
| Shared coordinated | Shared coordinated | `share_group` 相同且非零时允许 |
| 其他组合 | | 冲突 |

初始化执行 O(n²) 检查。实例数量和 Claim 数均有静态上限，因此时间和栈使用有界。

当同一 `kind + key` 存在 `BM_RESOURCE_SHARED_READ` 时，必须恰好存在一个相同非零 `share_group` 的 `BM_RESOURCE_OWNER`。Owner 负责初始化和写配置，Reader 只能读取结果。Owner 自身不表示独占使用；无需共享的资源应继续声明为 `BM_RESOURCE_EXCLUSIVE`。

### 5.3 编译期检查的边界

C99 常量表达式不能可靠比较任意对象成员或指针，因此通用资源唯一性检查以启动期检查为准。代码生成器可以额外生成枚举值和编译期重复检测，但不是框架正确性的唯一防线。

---

## 6. 实例定义示例

```c
typedef struct bm_hal_encoder bm_hal_encoder_t;
typedef struct bm_hal_timer bm_hal_timer_t;

typedef struct {
    foc_state_t foc;
    foc_config_t active_config;
    uint32_t command_version;
} servo_state_t;

typedef struct {
    foc_config_t initial;
} servo_config_t;

typedef struct {
    const bm_hal_pwm_t *pwm;
    const bm_hal_adc_t *adc;
    const bm_hal_encoder_t *encoder;
    const bm_hal_comp_t *comp;
} servo_resources_t;

static servo_state_t servo_x_state;

static const servo_config_t servo_x_config = {
    { 4u, 0.5f, 100.0f, 10.0f }
};

static const servo_resources_t servo_x_resources = {
    &BM_HAL_PWM1,
    &BM_HAL_ADC1_INJ,
    &BM_HAL_ENC2,
    &BM_HAL_COMP1
};

static void foc_current_step(const bm_ctrl_inst_t *instance);
static void foc_speed_step(const bm_ctrl_inst_t *instance);
static int bind_current_adc(const bm_ctrl_inst_t *instance,
                            const bm_hal_hrt_binding_t *binding);
static int servo_init(const bm_ctrl_inst_t *instance);
static int servo_start(const bm_ctrl_inst_t *instance);
static void servo_safe_stop(const bm_ctrl_inst_t *instance);

static const bm_ctrl_ops_t servo_ops = {
    servo_init,
    servo_start,
    servo_safe_stop
};

static const bm_ctrl_slot_t servo_x_slots[] = {
    {
        BM_CTRL_SLOT_HARDWARE,
        0u,
        BM_HRT_TRIGGER_ADC_COMPLETE,
        foc_current_step,
        bind_current_adc,
        "current"
    },
    {
        BM_CTRL_SLOT_SCHEDULED,
        1000u,
        BM_HRT_TRIGGER_TIMER,
        foc_speed_step,
        NULL,
        "speed"
    }
};

static const bm_ctrl_inst_t servo_x = {
    1001u,
    "servo_x",
    &servo_x_state,
    &servo_x_config,
    &servo_x_resources,
    servo_x_slots,
    (uint32_t)(sizeof(servo_x_slots) / sizeof(servo_x_slots[0])),
    servo_x_claims,
    (uint32_t)(sizeof(servo_x_claims) / sizeof(servo_x_claims[0])),
    &servo_ops
};
```

实例 ID `1001u` 由产品配置维护，可用于日志、协议映射和持久化数据。数组索引与实例 ID 是两个不同概念。

---

## 7. 可重入算法契约

```c
static void foc_current_step(const bm_ctrl_inst_t *instance) {
    servo_state_t *state = (servo_state_t *)instance->state;
    const servo_config_t *config =
        (const servo_config_t *)instance->config;
    const servo_resources_t *resources =
        (const servo_resources_t *)instance->resources;
    uint16_t iu;
    uint16_t iv;

    if (bm_hal_adc_read_injected(resources->adc, 0u, &iu) != BM_OK ||
        bm_hal_adc_read_injected(resources->adc, 1u, &iv) != BM_OK) {
        bm_hal_pwm_request_safe_state(resources->pwm);
        return;
    }

    foc_step(&state->foc, &config->initial, iu, iv);
    (void)bm_hal_pwm_set_duty(resources->pwm, 0u, state->foc.duty[0]);
    (void)bm_hal_pwm_set_duty(resources->pwm, 1u, state->foc.duty[1]);
    (void)bm_hal_pwm_set_duty(resources->pwm, 2u, state->foc.duty[2]);
}
```

算法约束：

- 可变状态只能通过 `instance->state` 或明确的跨域邮箱访问。
- `config` 是只读初始配置；运行时配置通过版本化命令快照切换。
- HRT 路径不得使用函数内静态可变变量。
- 共享查表数据必须为 `const`。
- HAL 句柄必须属于实例 Claim 声明的资源。
- WCET 必须包含 HAL 调用和错误路径。

`BM_STATIC_ASSERT_NO_GLOBALS` 不能可靠证明以上性质；建议使用编译器告警、静态分析规则、符号检查和代码评审共同执行。

---

## 8. Sync Domain

### 8.1 数据结构

```c
typedef struct {
    const char *name;
    const bm_hal_timer_t *master_timer;
    const bm_ctrl_inst_t *const *members;
    const uint32_t *phase_ticks;
    uint32_t member_count;
} bm_sync_domain_t;
```

成员表和相位使用定时器 Tick，而不是角度。角度到 Tick 的转换由产品配置工具完成，并检查分辨率和范围。

```c
static const bm_ctrl_inst_t *const robot_members[] = {
    &servo_x,
    &servo_y,
    &servo_z
};

static const uint32_t robot_phase_ticks[] = { 0u, 0u, 0u };

static const bm_sync_domain_t robot_domain = {
    "robot",
    &BM_HAL_TIM2,
    robot_members,
    robot_phase_ticks,
    3u
};
```

### 8.2 HAL 契约

```c
int bm_sync_configure(const bm_sync_domain_t *domain);
int bm_sync_arm(const bm_sync_domain_t *domain);
int bm_sync_trigger(const bm_sync_domain_t *domain);
void bm_sync_safe_stop(const bm_sync_domain_t *domain);
```

语义：

1. `configure` 校验芯片触发路由、计数模式和周期兼容性。
2. `arm` 预装计数器、比较值和输出安全状态。
3. `trigger` 使用主定时器 TRGO 或外部门控启动计数。
4. 严格同时输出只能由硬件链路保证；软件连续写 MOE 不属于原子启动。

不同 STM32 型号的 ITR 路由不同，通用设计文档不硬编码具体 `SMCR.TS` 位值。

---

## 9. 跨实例通信

### 9.1 最新值耦合

龙门双轴位置耦合可以使用混合域设计中的 `bm_snapshot`：

```c
BM_SNAPSHOT_DEFINE(position_bus, float);
static position_bus_snapshot_t x_position_bus = BM_SNAPSHOT_INITIALIZER;
```

轴 X 是唯一生产者，轴 Y 是唯一消费者。若多个消费者需要同一数据，应为每个消费者建立独立邮箱，或引入经过验证的广播原语；不能把 SPSC 类型直接当作多消费者总线。

### 9.2 无损数据

故障历史、遥测和命令序列需要 ring buffer。HRT 不直接发布 `bm_event`；SRT 从 ring buffer 或快照取数据后再发布事件。

---

## 10. BMS 多通道模型

BMS 电芯实例不应各自独占同一个 ADC 规则组。推荐把共享采样器建模为独立实例：

```text
pack_sampler
  owns ADC + DMA
  writes cell sample array snapshot

cell_0 ... cell_15
  own state + balance GPIO
  read their indexed sample in SRT
```

DMA ISR 只确认完成并发布一份完整采样快照，不在 ISR 中执行 16 次浮点换算。主循环完成标定、通道分发和故障事件发布。

实例 ID 不能用作数组下标。电芯通道索引是 `cell_config_t.channel_index` 中单独验证的字段。

---

## 11. 生命周期与安全状态

建议启动顺序：

```c
int main(void) {
    int rc;

    hal_init();
    bm_event_reset();

    rc = bm_ctrl_init_all(app_instances, APP_INSTANCE_COUNT);
    if (rc != BM_OK) {
        bm_ctrl_safe_stop_all(app_instances, APP_INSTANCE_COUNT);
        app_fault_halt(rc);
    }

    rc = bm_sync_configure(&robot_domain);
    if (rc != BM_OK) {
        bm_sync_safe_stop(&robot_domain);
        bm_ctrl_safe_stop_all(app_instances, APP_INSTANCE_COUNT);
        app_fault_halt(rc);
    }

    rc = bm_ctrl_start_all(app_instances, APP_INSTANCE_COUNT);
    if (rc != BM_OK) {
        bm_sync_safe_stop(&robot_domain);
        bm_ctrl_safe_stop_all(app_instances, APP_INSTANCE_COUNT);
        app_fault_halt(rc);
    }

    rc = bm_sync_arm(&robot_domain);
    if (rc != BM_OK) {
        bm_sync_safe_stop(&robot_domain);
        bm_ctrl_safe_stop_all(app_instances, APP_INSTANCE_COUNT);
        app_fault_halt(rc);
    }

    rc = bm_hrt_start();
    if (rc != BM_OK) {
        bm_hrt_stop();
        bm_sync_safe_stop(&robot_domain);
        bm_ctrl_safe_stop_all(app_instances, APP_INSTANCE_COUNT);
        app_fault_halt(rc);
    }

    rc = bm_sync_trigger(&robot_domain);
    if (rc != BM_OK) {
        bm_hrt_stop();
        bm_sync_safe_stop(&robot_domain);
        bm_ctrl_safe_stop_all(app_instances, APP_INSTANCE_COUNT);
        app_fault_halt(rc);
    }

    while (1) {
        (void)bm_ticker_poll();
        (void)bm_event_process(8u);
        bm_wdg_feed();
    }
}
```

所有 PWM 在初始化、配置失败和看门狗复位路径中默认保持安全电平。`app_fault_halt()` 是不返回故障入口。

---

## 12. 故障隔离语义

本设计提供：

- 实例状态内存隔离。
- 配置与资源所有权检查。
- 每实例功能故障码。
- 可配置的单实例或整域安全停机策略。

本设计不提供：

- 单核 CPU HardFault 后继续执行其他实例。
- 对越界写、栈破坏或任意指针错误的完整隔离。

需要更强隔离时，应使用 MPU、进程/任务隔离、双核分区或多 MCU 架构。

---

## 13. 验证要求

| 项目 | 必需测试 |
|-----|---------|
| ID 唯一性 | 重复 ID 初始化失败 |
| Claim 冲突 | 独占冲突失败；共享规则按表执行 |
| 容量 | 实例、Slot、Claim 超限时无部分启动 |
| 回滚 | 第 N 个实例启动失败后，前 N-1 个进入安全状态 |
| 多实例状态 | 同算法不同状态并发运行，无全局变量串扰 |
| Hardware HRT | 每个实例绑定到正确 PWM/ADC 句柄 |
| Sync Domain | 错误路由、周期不兼容和相位越界均失败 |
| 命令更新 | HRT 只观察到完整版本配置 |
| BMS 映射 | 实例 ID 与通道索引分离，边界检查有效 |
| 故障策略 | 单实例停机和整域停机均符合产品安全需求 |

---

## 14. 资源预算

资源大小由目标 ABI 决定，设计阶段不写死 `bm_ctrl_inst_t` 为 24B。预算应通过仓库的尺寸测量工具在目标工具链上生成。

建议统计：

- 每实例状态 RAM。
- 每 Slot 绑定记录 RAM。
- Claim 和实例描述符 ROM。
- 快照或 ring buffer RAM。
- HAL 驱动和控制算法 Flash。
- 最坏 Tick 中到期 Slot 的总 WCET。

物理外设数量仍是多实例上限。抽象层不能把三个高级定时器变成六组三相 PWM。

---

## 15. 与混合域设计的关系

| 多实例需求 | 使用的混合域能力 |
|-----------|----------------|
| 高频电流环 | Hardware HRT |
| 速度环 | Scheduled HRT |
| 状态机和通信 | SRT ticker + `bm_event` |
| 跨域参数更新 | 版本化命令快照 |
| 跨轴最新值 | 每消费者独立 `bm_snapshot` |
| 故障上报 | HRT 故障快照，SRT 发布事件 |
| 相位同步 | HAL Sync Domain |

两份设计共同遵守同一条边界：高优先级 HRT 不访问 SRT 的事件队列和临界区保护结构。

---

*本设计采用显式实例表、稳定 ID、标准化资源 Claim 和统一调度 ABI，使多实例机制能够在当前 C99 工程中直接实现并测试。*
