# 头文件布局

应用代码仍使用扁平 `#include "bm_event.h"`；物理目录按职责分层，由 CMake 注入多个 include 路径。

```text
include/
├── bm/
│   ├── common/     # 全框架共用：types、log、config、atomic…
│   ├── core/       # 核心域：event、mempool、module、shell、wdg…
│   ├── hybrid/     # 混合域：hrt、ticker、ctrl_inst、sync…
│   ├── hal/        # 应用 HAL 契约（bm_hal_*）
│   └── ultra/      # 极裁剪头文件库
└── drv/            # 驱动 API（bm_drv_*），后端实现者使用
```

## 谁该包含哪一层

| 角色 | 典型头文件 | 目录 |
|------|-----------|------|
| 应用 | `bm_event.h`、`bm_hal_pwm.h`、`bm_hrt.h` | `bm/common` `bm/core` `bm/hybrid` `bm/hal` |
| Ultra 固件 | `bm_ultra.h` | `bm/ultra`（+ `bm/common` 若需要 types） |
| 平台后端 | `bm_drv_pwm.h`、`bm_drv_timer.h` | 上述全部 + `drv/` |

## CMake

`cmake/bm_include_dirs.cmake` 定义 `BM_INCLUDE_APP_DIRS` 与 `BM_INCLUDE_BACKEND_DIRS`。  
`bm_config` 目标导出后端路径（因 `bm_hal` 分发层依赖 `bm_drv_*` 类型）。

Keil / IAR 请在 Include Paths 中加入 `include/bm/common`、`core`、`hybrid`、`hal`、`ultra`；编写后端时再加 `include/drv`。
