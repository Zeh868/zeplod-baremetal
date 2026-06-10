# 从 Core 迁移到混合域架构

## 1. 启用 CMake 选项

```cmake
set(BM_ENABLE_HRT ON)
set(BM_ENABLE_TICKER ON)
set(BM_ENABLE_CTRL_INST ON)  # 可选
```

## 2. 配置 `bm_config.h`

增加 `BM_CONFIG_HRT_TICK_US`、`BM_CONFIG_HRT_MAX_SLOTS` 等宏（见 `bm_config.h.template`）。

## 3. 初始化顺序

```c
bm_event_reset();
bm_hal_timer_init(tick_hz);           // 未使用 ctrl_inst 时
bm_ctrl_init_all(instances, count);   // 可选
bm_hrt_start();
bm_ctrl_start_all(instances, count);  // 可选
bm_ticker_init(ticker_slots, count);

for (;;) {
    bm_ticker_poll();
    bm_event_process(max);
}
```

## 4. 边界

- HRT ISR 不调用 `bm_event_*`
- 跨域数据用 `bm_snapshot`
- 默认 `BM_CONFIG_ENABLE_PRIORITY_MASK=0` 保持原有临界区行为
