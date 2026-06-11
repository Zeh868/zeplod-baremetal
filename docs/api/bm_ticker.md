# bm_ticker — 毫秒级周期事件发布器

头文件：`include/bm_ticker.h`  
实现：`src/hybrid/bm_ticker.c`

## 概述

Ticker 在慢速域按固定毫秒周期向事件总线发布事件，供后台任务轮询处理。须在主循环中周期性调用 `bm_ticker_poll()`。

## 类型

| 类型 | 字段 |
|------|------|
| `bm_ticker_slot_t` | `period_ms`、`event_type`、`priority`、`name` |

## API

### `bm_ticker_init(slots, slot_count)`

注册 ticker slot 表。  
返回：`BM_OK`；`BM_ERR_INVALID`。

### `bm_ticker_poll()`

检查各 slot 是否到期，到期则 `bm_event_publish`。事件队列满时递增对应 slot 的 `dropped` 计数，不阻塞。  
返回：`BM_OK`；`BM_ERR_NOT_INIT`。

### `bm_ticker_get_dropped(slot_index)`

返回指定 slot 因队列满丢弃的事件数，用于监控背压。

### `bm_ticker_reset()`

清空内部计时与 dropped 计数。

## 使用示例

```c
static const bm_ticker_slot_t g_ticker[] = {
    { 100u, BM_EVENT_TICK_100MS, BM_EVENT_PRIO_LOW, "slow" },
};

bm_ticker_init(g_ticker, BM_ARRAY_SIZE(g_ticker));

for (;;) {
    bm_ticker_poll();
    bm_event_process();
}
```

## 与 HRT 的边界

| 域 | 周期 | 上下文 | 输出 |
|----|------|--------|------|
| HRT | μs | ISR | 直接回调 |
| Ticker | ms | 主循环 | `bm_event_publish` |

不要在 HRT 回调中调用 ticker API。
