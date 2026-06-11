# bm_ctrl_inst — 控制实例与批量生命周期

头文件：`include/bm_ctrl_inst.h`  
实现：`src/hybrid/bm_ctrl_inst.c`

## 概述

混合域中每个控制回路描述为一个 `bm_ctrl_inst_t`：含状态、配置、资源声明、HRT slot 表及 `ops` 生命周期钩子。框架提供批量 `init` / `start` / `safe_stop`，失败时按实例顺序回滚。

## 类型

| 类型 | 说明 |
|------|------|
| `bm_ctrl_step_fn_t` | 单步算法 `void (*)(const bm_ctrl_inst_t *)` |
| `bm_ctrl_slot_kind_t` | `HARDWARE`（PWM/ADC 触发）或 `SCHEDULED`（软件定时） |
| `bm_ctrl_slot_t` | `kind`、`period_us`、`trigger`、`step`、`bind_hardware`、`name` |
| `bm_ctrl_ops_t` | `init`、`start`、`safe_stop` |
| `bm_ctrl_inst_t` | `id`、`name`、`state`、`config`、`resources`、`slots`、`claims`、`ops` |

## API

### `bm_ctrl_init_all(instances, count)`

依次调用各实例 `ops->init`；任一失败则对已初始化实例执行逆序清理。  
返回首个失败错误码或 `BM_OK`。

### `bm_ctrl_start_all(instances, count)`

启动前执行 `bm_resource_check_conflicts`；通过后注册 HRT slot 并调用 `ops->start`。失败时逆序 `safe_stop` + 解绑 HRT。

### `bm_ctrl_safe_stop_all(instances, count)`

逆序调用 `ops->safe_stop`，解除 HRT 绑定。

### `bm_ctrl_find(instances, count, id)`

按 `id` 查找实例，未找到返回 `NULL`。

## 典型模式

```c
static int my_init(const bm_ctrl_inst_t *inst) { return BM_OK; }
static int my_start(const bm_ctrl_inst_t *inst) { return BM_OK; }
static void my_safe_stop(const bm_ctrl_inst_t *inst) { }

static const bm_ctrl_ops_t g_ops = { my_init, my_start, my_safe_stop };

static const bm_ctrl_inst_t g_inst = {
    .id = 1,
    .name = "axis0",
    .ops = &g_ops,
    /* slots, claims, state, config ... */
};

static const bm_ctrl_inst_t *const g_all[] = { &g_inst };

bm_ctrl_init_all(g_all, 1);
bm_ctrl_start_all(g_all, 1);
```

## 与资源 / 同步的关系

- 启动前必须通过 `bm_resource_check_conflicts`（在 `start_all` 内自动调用）。
- 多轴相位对齐使用 `bm_sync` 配置同步域，成员指向 `bm_ctrl_inst_t`。
