# Src SIL-2 认证准备说明

> 状态：认证准备工件（非正式 SIL-2 证书）  
> 范围：`src/` 核心与可选组件  
> 故障策略：**fail-stop**（返回确定性错误码，不自动复位控制块）

## 安全目标

1. 运行时检测无效控制块、索引越界、算术溢出，返回 `BM_ERR_INVALID` / `BM_ERR_OVERFLOW`。
2. SRT 组件（`bm_event`、`bm_mempool`、`bm_channel`）在临界区内保持索引一致性。
3. 启动/停机失败进入可诊断状态；`bm_ctrl_start_all` 失败时停止 HRT 并解绑硬件。
4. 软件看门狗在 `BM_CONFIG_WDG_MODULE_TIMEOUT_MS` 窗口内要求各模块 `feed_module`。

## 需求—测试追溯（摘录）

| ID | 需求 | 实现 | 测试 |
|----|------|------|------|
| SR-CH-01 | 通道索引越界 fail-stop | `src/channel/bm_channel.c` | `test_channel_invalid_args` |
| SR-CH-02 | 查询 API 临界区一致 | `bm_channel_is_empty/is_full` | `test_channel_query_consistency` |
| SR-MP-01 | 双释放拒绝 | `src/core/bm_mempool.c` | `test_mempool_double_free_ignored` |
| SR-EV-01 | 队列溢出计数 | `bm_event_get_dropped_count` | `test_event_queue_overflow_counts_dropped` |
| SR-EV-02 | 分发阶段订阅者快照 | `bm_event_process` | 现有优先级测试 |
| SR-WDG-01 | 模块超时窗口 | `src/core/bm_wdg.c` | `test_wdg_blocks_hw_feed_until_module_fed` |
| SR-HRT-01 | 每 ISR 每槽最多一次回调 | `src/hrt/bm_hrt.c` | `test_hrt_deadline_miss` |
| SR-CTRL-01 | start 失败安全停机 | `src/ctrl/bm_ctrl_inst.c` | 待补 `test_ctrl_start_failure_safe_stop` |
| SR-MOD-01 | init 失败回滚 | `src/module/bm_module.c` | 待补 |
| SR-SH-01 | 参数截断拒绝 | `src/shell/bm_shell.c` | `test_shell_too_many_args_rejected` |

## 残余风险

- HAL 弱符号桩在未链接平台库时仍可能静默失效；需应用 CMake 强制链接 `bm_hal_<platform>`。
- `bm_event_publish_event` 零拷贝路径仍依赖调用方生命周期保证。
- `bm_shell` 应仅在调试构建启用，不得驱动安全动作。

## 验证命令

```bash
cmake -B build-sil2 -DBM_ENABLE_CHANNEL=ON -DBM_ENABLE_SHELL=ON \
  -DBM_ENABLE_HRT=ON -DBM_ENABLE_TICKER=ON -DBM_ENABLE_CTRL_INST=ON \
  -DBM_ENABLE_SYNC=ON
cmake --build build-sil2 --config Debug
ctest --test-dir build-sil2 -C Debug --output-on-failure
```
