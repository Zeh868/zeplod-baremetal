# include/

Zeplod Baremetal **公共 API 均在本目录根层**，集成时只需将 `include/` 加入 Include Path。

## 推荐用法

```c
#include "zeplod.h"         /* 按 bm_config.h 暴露已启用子系统 */
#include "bm_hal_uart.h"    /* 板级 HAL 按需单独包含 */
```

`zeplod.h` 根据 `BM_CONFIG_ENABLE_*` 聚合 API，与 CMake `BM_ENABLE_*` / `PROFILE` 对齐。

| 层级 | 配置要点 | 备选聚合头 |
|------|----------|------------|
| Ultra | `BM_CONFIG_ENABLE_ULTRA=1` | `bm_ultra.h` |
| Nano / Event | module + wdg | `bm_lite.h` |
| Lite | + channel / shell | `bm_lite.h` |
| Control | + hrt / ticker / ctrl_inst / sync | `bm_hybrid.h` |

## 头文件分类

| 前缀 | 受众 | 说明 |
|------|------|------|
| `zeplod.h` | 应用 | 统一入口 |
| `bm_*.h`（非 hal/drv） | 应用 | 框架子系统，可按需单独包含 |
| `bm_hal_*.h` | 应用 | HAL 契约 |
| `bm_drv_*.h` | Port 作者 | 驱动后端 API |

详见 [docs/20-头文件布局.md](../docs/20-头文件布局.md)。
