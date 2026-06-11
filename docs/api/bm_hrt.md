# bm_hrt — 高分辨率定时调度器

头文件：`include/bm_hrt.h`  
实现：`src/hybrid/bm_hrt.c`

## 概述

HRT（High-Resolution Timer）管理微秒级周期 slot，支持定时器、PWM 更新、ADC 完成等多种硬件触发源。回调在 ISR 或等效硬实时上下文中执行，**不得**调用 `bm_event_publish` 等慢速核心 API。

## 类型

| 类型 | 说明 |
|------|------|
| `bm_hrt_callback_t` | slot 到期回调 `void (*)(void *context)` |
| `bm_hrt_trigger_t` | `BM_HRT_TRIGGER_TIMER` / `PWM_UPDATE` / `ADC_COMPLETE` |
| `bm_hrt_slot_t` | `period_us`、`trigger`、`callback`、`context`、`name` |

## API

### `bm_hrt_init(slots, slot_count)`

注册并复制 slot 表。周期换算后须不超过 `INT32_MAX` tick。
返回：`BM_OK`；`BM_ERR_INVALID`（空表或超限）。

### `bm_hrt_start()`

启动调度，绑定各 slot 的硬件触发源。  
返回：`BM_OK`；`BM_ERR_NOT_INIT`；平台 HAL 错误码。

### `bm_hrt_stop()` / `bm_hrt_reset()`

停止调度并解除绑定；`reset` 清空内部状态，可再次 `init`。

### `bm_hrt_deadline_missed_hook(slot)`

弱符号钩子，slot 错过 deadline 时在 ISR 中调用。默认空实现；覆盖实现必须有界且不得记录日志。

### `bm_hrt_get_deadline_missed(slot_index)` / `bm_hrt_get_deadline_missed_total()`

查询单槽或全部槽累计 deadline 错过次数（临界区内读取，达到
`UINT32_MAX` 后饱和）。

## 使用示例

```c
static void current_loop(void *ctx) { /* ISR 内快速算法 */ }

static const bm_hrt_slot_t g_slots[] = {
    { 100u, BM_HRT_TRIGGER_PWM_UPDATE, current_loop, NULL, "current" },
};

bm_hrt_init(g_slots, BM_ARRAY_SIZE(g_slots));
bm_hrt_start();
```

## 与 HAL 的关系

PWM/ADC 通过 `bm_hal_*_bind_*` 将 HRT 回调挂到外设更新/完成事件；定时器 slot 使用 `bm_hal_timer` 参考实现。

## 错误码

| 码 | 含义 |
|----|------|
| `BM_ERR_INVALID` | 参数或 slot 配置无效 |
| `BM_ERR_NOT_INIT` | 未调用 `init` |
| `BM_ERR_BUSY` | 资源已被占用 |
