# Hybrid-Domain Architecture Design（混合域架构设计）

> 日期：2026-06-10
> 状态：设计阶段 v2
> 范围：zeplod-baremetal 的硬实时（HRT）与软实时（SRT）双域扩展
> 目标：保留现有事件驱动核心，同时为伺服、BMS 和数字电源提供可验证的硬实时执行路径

---

## 1. 背景与边界

现有 `bm_event`、`bm_module`、`bm_channel` 和 `bm_shell` 适合主循环驱动的异步业务，但不能单独承担 PWM 同步采样、电流环和硬件故障保护。

本设计将执行路径分为三类，而不是把所有周期任务放进同一个软件 Tick：

| 执行类 | 典型频率 | 触发方式 | 适用任务 |
|-------|---------|---------|---------|
| Hardware HRT | 5-200kHz | PWM/ADC/比较器硬件事件 | 电流环、数字电源、硬件保护 |
| Scheduled HRT | 100Hz-10kHz | 专用定时器 ISR | 速度环、低频采样、确定性派生环 |
| SRT | 无硬截止期限 | 主循环轮询和事件队列 | 状态机、通信、Shell、日志 |

关键边界：

1. `<1us` 保护由比较器和定时器 Break 等硬件链路完成，不依赖 CPU。
2. `<100ns` 相位或抖动目标只能由定时器、PWM、ADC 触发链路保证，软件调度器不作此承诺。
3. HRT ISR 不访问 `bm_event`、`bm_mempool`、`bm_channel` 或其他由 SRT 临界区保护的结构。
4. 可验证指标必须包含目标 MCU、时钟、编译器、优化等级和最坏执行时间（WCET）测量条件。
5. Hardware HRT 的中断优先级高于 Scheduled HRT；Scheduled HRT 的分析必须包含被 Hardware HRT 抢占的时间。

---

## 2. 架构总览

```text
Hardware trigger path                     Main-loop path

PWM/ADC/COMP IRQ                           bm_ticker_poll()
       |                                         |
       v                                         v
Hardware HRT callback                    bm_event_publish_copy()
       |                                         |
       +---- bm_snapshot / command mailbox ------+
       |
Scheduled HRT timer IRQ
       |
       v
bounded slot dispatcher
```

### 2.1 设计原则

1. **硬件优先**：载波同步任务直接绑定 PWM/ADC 事件，不经过软件 Tick 扫描。
2. **数据结构隔离**：HRT 和 SRT 不并发修改同一框架容器。
3. **有界执行**：每个 HRT 回调都有 WCET 预算，禁止阻塞、分配内存和格式化输出。
4. **显式降级**：没有分级中断能力的平台不宣称在 SRT 临界区存在时仍能满足 HRT 延迟。
5. **C99 可移植**：公共设计不依赖 `_Static_assert`、匿名 union、`__COUNTER__` 或链接器自动收集。

---

## 3. HRT 执行模型

### 3.1 统一回调 ABI

所有 HRT 路径使用同一回调形式：

```c
typedef void (*bm_hrt_callback_t)(void *context);

typedef enum {
    BM_HRT_TRIGGER_TIMER,
    BM_HRT_TRIGGER_PWM_UPDATE,
    BM_HRT_TRIGGER_ADC_COMPLETE
} bm_hrt_trigger_t;

typedef struct {
    uint32_t period_us;
    bm_hrt_trigger_t trigger;
    bm_hrt_callback_t callback;
    void *context;
    const char *name;
} bm_hrt_slot_t;
```

`context` 由应用静态分配，可指向单实例状态，也可指向多实例控制上下文。

### 3.2 Hardware HRT

Hardware HRT 由增强 HAL 直接分发：

```c
typedef struct {
    bm_hrt_callback_t callback;
    void *context;
} bm_hal_hrt_binding_t;

int bm_hal_pwm_bind_update(const bm_hal_pwm_t *pwm,
                           const bm_hal_hrt_binding_t *binding);
int bm_hal_adc_bind_complete(const bm_hal_adc_t *adc,
                             const bm_hal_hrt_binding_t *binding);
```

`binding == NULL` 表示解绑。解绑必须先禁用对应中断源，再清除保存的 callback/context；普通绑定只安装回调，不得自动使能 PWM 输出或启动 ADC。

HAL ISR 只完成以下工作：

1. 清除或确认硬件中断标志。
2. 获取已经完成的 ADC 结果。
3. 调用绑定的 HRT 回调。
4. 返回中断。

禁止在该路径中发布事件、扫描全局任务表或调用日志接口。

### 3.3 Scheduled HRT

`bm_hrt` 是可选的有界周期分发器，服务于不要求 PWM 相位锁定的中低频 HRT 任务：

```c
int bm_hrt_init(const bm_hrt_slot_t *slots, uint32_t slot_count);
int bm_hrt_start(void);
void bm_hrt_stop(void);
void bm_hrt_reset(void);
```

Scheduled HRT 复用现有 Timer HAL 作为系统单调 Tick，避免在只有一个 SysTick 的平台上配置两套互相冲突的时间源：

```c
int bm_hal_timer_init(uint32_t tick_hz);
void bm_hal_timer_set_callback(void (*callback)(void));
void bm_hal_timer_stop(void);
uint32_t bm_hal_timer_get_ticks(void);
uint32_t bm_hal_timer_get_freq(void);
```

`bm_hrt_start()` 先安装 callback，再按 `BM_CONFIG_HRT_TICK_US` 配置定时器；callback 在 ISR 中驱动一次有界 Slot 分发。`bm_hrt_stop()` 停止定时器并清除 callback，`bm_hrt_reset()` 进一步清空 Slot 和状态，供失败回滚与测试隔离使用。`bm_ticker` 从同一 Tick 读取时间，不重复初始化定时器。未启用 `bm_hrt` 时，由应用负责初始化 Timer HAL 后再使用 `bm_ticker`。

约束：

- `period_us` 必须是 `BM_CONFIG_HRT_TICK_US` 的整数倍。
- Slot 数在初始化时校验，不超过 `BM_CONFIG_HRT_MAX_SLOTS`。
- ISR 每次最多扫描固定数量 Slot。
- 同一 Tick 到期的回调按表顺序执行，因此后续回调延迟包含前序回调 WCET。
- 若延迟达到或超过一个完整周期，分发器记录 deadline miss 并调用应用故障钩子；它不会在一个 ISR 中循环补跑历史周期。
- 必须满足：

```text
dispatcher WCET + sum(due callback WCET) < HRT tick period - safety margin
```

实现使用无符号差值处理 32 位时间回绕，不使用简单的绝对时间大于比较：

```c
if ((int32_t)(now - slot->next_tick) >= 0) {
    if ((uint32_t)(now - slot->next_tick) >= slot->period_ticks) {
        bm_hrt_deadline_missed(slot);
        slot->next_tick = now + slot->period_ticks;
    } else {
        slot->next_tick += slot->period_ticks;
        slot->callback(slot->context);
    }
}
```

### 3.4 SRT 周期任务

`bm_ticker` 仅服务 SRT，由主循环通过现有 `bm_hal_timer_get_ticks()` 和 `bm_hal_timer_get_freq()` 读取单调时间源并发布事件。它不在高优先级 HRT ISR 中操作事件队列：

```c
typedef struct {
    uint32_t period_ms;
    bm_event_type_t event_type;
    bm_event_priority_t priority;
    const char *name;
} bm_ticker_slot_t;

int bm_ticker_init(const bm_ticker_slot_t *slots, uint32_t slot_count);
int bm_ticker_poll(void);
uint32_t bm_ticker_get_dropped(uint32_t slot_index);
void bm_ticker_reset(void);
```

`bm_ticker_poll()` 使用无符号 Tick 差值处理时间回绕，并在主循环中调用 `bm_event_publish_copy(type, priority, NULL, 0)`。周期事件只表达“到期”，不携带可能被 HRT 并发修改的数据；事件处理器需要数据时，从 `bm_snapshot` 读取。队列满时返回错误并累计对应 Slot 的丢失计数；是否重试由应用策略决定。`bm_ticker_reset()` 用于测试和应用重新初始化。

---

## 4. 跨域数据交换

### 4.1 语义

`bm_snapshot` 是单生产者、单消费者的“最新值”邮箱。下面代码描述协议，不是对所有 C99 编译器都可直接复用的完整实现：

- 不保证保存每一帧。
- 写端和读端都执行一次有界拷贝，因此不称为零拷贝。
- 三个缓冲区避免生产者覆盖消费者正在读取的数据。
- 控制字段要求目标平台支持对齐的 8 位读写原子性，并由 HAL 提供编译器/CPU 内存屏障。
- `bm_hal_memory.h` 必须用经过验证的编译器内建、汇编屏障或原子扩展实现屏障；仅使用 `volatile` 不构成通用 C 内存同步保证。
- 通用保证限于单核 MCU 的 ISR/主循环抢占模型；多核或主机线程模型需要平台原子实现。
- 每个邮箱必须只有一个生产者和一个消费者。故障 ISR、控制 ISR 和主循环不能共同写同一邮箱。

需要逐帧历史数据时，使用独立的 SPSC ring buffer；不能用 `bm_snapshot` 替代队列。

### 4.2 三缓冲协议

```c
#define BM_SNAPSHOT_NONE 0xFFu

#define BM_SNAPSHOT_DEFINE(name, type) \
    typedef struct { \
        volatile uint8_t published; \
        volatile uint8_t reading; \
        type buffer[3]; \
    } name##_snapshot_t

#define BM_SNAPSHOT_INITIALIZER \
    { .published = 0u, .reading = BM_SNAPSHOT_NONE, .buffer = { 0 } }
```

生产者只写既不是 `published`、也不是 `reading` 的缓冲区：

```c
static inline uint8_t bm_snapshot_choose_buffer(uint8_t published,
                                                uint8_t reading) {
    uint8_t i;

    for (i = 0u; i < 3u; ++i) {
        if (i != published && i != reading) {
            return i;
        }
    }
    return 0u; /* 有效状态下不可达 */
}

#define BM_SNAPSHOT_PUBLISH(box, value_ptr) do { \
    uint8_t _p = (box).published; \
    uint8_t _r = (box).reading; \
    uint8_t _w = bm_snapshot_choose_buffer(_p, _r); \
    (box).buffer[_w] = *(value_ptr); \
    bm_memory_barrier_release(); \
    (box).published = _w; \
} while (0)
```

消费者先声明读取所有权，再确认发布索引没有变化：

```c
#define BM_SNAPSHOT_READ(box, out_ptr) do { \
    uint8_t _p; \
    do { \
        _p = (box).published; \
        (box).reading = _p; \
        bm_memory_barrier_full(); \
    } while ((box).published != _p); \
    *(out_ptr) = (box).buffer[_p]; \
    bm_memory_barrier_release(); \
    (box).reading = BM_SNAPSHOT_NONE; \
} while (0)
```

实例必须使用 `BM_SNAPSHOT_INITIALIZER`，使 `published` 指向已初始化的缓冲区，并令 `reading` 为 `BM_SNAPSHOT_NONE`。宏参数必须是无副作用左值。

### 4.3 SRT 到 HRT 的命令

运行时参数更新是允许的，但必须通过显式命令邮箱完成：

1. SRT 构造完整、已校验的新配置。
2. SRT 发布配置快照并递增版本号。
3. HRT 在控制周期边界读取快照。
4. HRT 一次性切换本地活动配置，不读取正在被 SRT 修改的对象。

紧急停止不走软件邮箱，必须使用硬件 Break 或专用安全输入。

---

## 5. 临界区与中断优先级

### 5.1 嵌套安全 API

现有 API 保持兼容：

```c
bm_irq_state_t bm_hal_critical_enter(void);
void bm_hal_critical_exit(bm_irq_state_t state);
```

支持 `BASEPRI` 的平台可增加：

```c
bm_irq_state_t bm_hal_critical_enter_below(uint8_t threshold);
void bm_hal_critical_exit_below(bm_irq_state_t previous_state);
```

前一状态由调用者持有，不能保存在单个全局变量中，否则嵌套临界区会恢复错误。

分级临界区使用编译期能力契约：

```c
#define BM_HAL_HAS_PRIORITY_MASK 1
```

平台支持时在自己的 HAL 源文件中实现上述函数；不支持的平台定义为 `0`，且不提供伪造的运行时错误返回。若 `BM_CONFIG_ENABLE_PRIORITY_MASK=1` 而 `BM_HAL_HAS_PRIORITY_MASK=0`，构建必须通过 `#error` 或 CMake 配置错误终止。

### 5.2 使用规则

- `bm_event`、`bm_mempool` 和 `bm_channel` 可以在配置允许时使用分级临界区。
- 高于阈值的 HRT ISR 不得访问上述结构，因此它们可以安全抢占 SRT 临界区。
- 调用 `bm_event_publish_copy_from_isr()` 的 ISR 必须处在会被该临界区屏蔽的优先级范围内。
- Hardware HRT 和 Scheduled HRT 不调用任何 `bm_event_*_from_isr()` API。

### 5.3 平台能力

| 平台 | 能力与承诺 |
|-----|-----------|
| Cortex-M3/M4/M7/M33 | 可用 BASEPRI 隔离 HRT 与 SRT |
| Cortex-M0/M0+ | 仅有全局屏蔽；启用 SRT 临界区时不承诺高频 HRT 最坏延迟 |
| RISC-V | 由具体 PLIC/CLIC 实现声明能力，不能假设标准 `mie` 提供优先级阈值 |
| 8-bit MCU | 默认只支持低频控制；高频 HRT 需要平台专项证明 |

---

## 6. 增强 HAL

HAL 使用资源句柄，避免全局单例 API，使多实例可以绑定不同外设：

```c
typedef struct bm_hal_pwm bm_hal_pwm_t;
typedef struct bm_hal_adc bm_hal_adc_t;
typedef struct bm_hal_comp bm_hal_comp_t;

int bm_hal_pwm_set_duty(const bm_hal_pwm_t *pwm,
                        uint32_t phase, uint16_t duty);
int bm_hal_pwm_enable_outputs(const bm_hal_pwm_t *pwm, int enable);
void bm_hal_pwm_request_safe_state(const bm_hal_pwm_t *pwm);

int bm_hal_adc_read_injected(const bm_hal_adc_t *adc,
                             uint32_t rank, uint16_t *value);

int bm_hal_comp_clear_latch(const bm_hal_comp_t *comp);
```

### 6.1 PWM 与 ADC

- PWM 句柄描述计数器、通道、死区、极性、Break 和触发输出。
- ADC 句柄描述 ADC 实例、规则组/注入组、触发源和结果寄存器。
- 相位同步由 HAL 配置硬件主从触发链路，不由通用软件循环模拟。

### 6.2 故障保护

安全顺序为：

1. 比较器或外部故障输入触发定时器 Break。
2. 硬件立即关闭输出并锁存故障。
3. 软件 ISR 只记录故障快照。
4. SRT 读取快照并上报。
5. 只有应用安全状态机通过校验后才能清除锁存并重新使能。

软件依次写多个定时器的 MOE 位不称为“原子同时使能”。需要严格同步时，应使用共同硬件触发或外部门控。

---

## 7. 与现有工程集成

### 7.1 目标结构

```text
include/
  bm_hrt.h
  bm_snapshot.h
  bm_ticker.h
  bm_hal_memory.h
  bm_hal_pwm.h
  bm_hal_adc.h
  bm_hal_comp.h
src/
  hrt/bm_hrt.c
  hrt/bm_ticker.c
```

### 7.2 CMake

沿用仓库“一个组件一个目标”的结构：

```cmake
option(BM_ENABLE_HRT "Enable bm-hrt" OFF)
option(BM_ENABLE_TICKER "Enable bm-ticker" OFF)

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
```

`bm_snapshot` 为头文件组件，不需要伪造空的 `.c` 目标。

### 7.3 配置

```c
#define BM_CONFIG_HRT_TICK_US             100u
#define BM_CONFIG_HRT_MAX_SLOTS           8u
#define BM_CONFIG_HRT_PRIORITY_THRESHOLD  2u
#define BM_CONFIG_TICKER_MAX_SLOTS        16u
```

配置头只保存平台无关数值。具体 `TIMx`、ADC 和 PWM 资源通过 HAL 句柄绑定，不放入公共配置模板。

---

## 8. 主循环示例

```c
BM_SHELL_DEFINE(app_shell);

int main(void) {
    int rc;

    hal_init();
    bm_event_reset();
    bm_shell_init(&app_shell);

    rc = bm_ticker_init(app_ticker, APP_TICKER_COUNT);
    if (rc != BM_OK) {
        app_safe_stop();
        app_fault_halt(rc);
    }
    rc = bm_hrt_init(app_hrt_slots, APP_HRT_SLOT_COUNT);
    if (rc != BM_OK) {
        app_safe_stop();
        app_fault_halt(rc);
    }
    rc = bm_hrt_start();
    if (rc != BM_OK) {
        app_safe_stop();
        app_fault_halt(rc);
    }

    while (1) {
        (void)bm_ticker_poll();
        (void)bm_event_process(8u);
        bm_shell_poll(&app_shell);
        bm_wdg_feed();
    }
}
```

`app_fault_halt()` 是应用提供的不返回故障入口。

---

## 9. 验证要求

| 项目 | 验证方式 |
|-----|---------|
| Hardware HRT 抖动 | GPIO/ETM/逻辑分析仪，在目标板和发布编译参数下测量 |
| Scheduled HRT WCET | 同一 Tick 最坏 Slot 组合测量并保留安全裕量 |
| 快照一致性 | 主机压力测试、目标板抢占测试、版本号/校验和检测 |
| 事件隔离 | 在 SRT 临界区压力下验证 HRT 不访问事件队列 |
| 时间回绕 | 人工设置计数器接近 `UINT32_MAX` 的单元测试 |
| 队列溢出 | 验证 ticker 丢失计数和应用降级策略 |
| 故障链路 | 实际触发 Break，测量输出关闭时间和重新使能条件 |

任何资源预算和时序指标在完成测量前都标记为“目标值”，不能作为已保证能力。

---

## 10. 已知限制

- `bm_snapshot` 是最新值邮箱，不适合遥测历史或无损命令流。
- C99 没有标准原子和内存模型 API；每个平台必须实现并验证屏障与控制字段原子性。
- Cortex-M0/M0+ 无法用 BASEPRI 隔离高优先级 HRT。
- 单核 MCU 上不能隔离 CPU HardFault；本设计只提供数据所有权和功能故障隔离。
- HRT 回调的可重入性主要依靠接口约束、静态分析和代码审查，不能由 C 类型系统完全证明。

---

*本设计将硬件同步控制、软件确定性调度和事件驱动业务分开处理。它不以“零开销”或“零抖动”概括所有路径，而是为每条路径定义可实现、可测量的保证。*
