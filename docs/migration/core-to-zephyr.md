# Core → Zephyr 迁移指南

## 模块生命周期

模块生命周期 API 基本兼容：

| Baremetal | Zephyr |
|---|---|
| `BM_MODULE_DEFINE` / `_bm_module_table[]` | `module_register()` |
| `bm_module_init_all()` | `module_init_all()` |
| `bm_module_start_all()` | `module_start_all()` |
| `bm_module_stop_all()` | `module_stop_all()` |

## 事件系统

回调签名一致：`void cb(const event_t *event, void *user_data)`

| Baremetal | Zephyr |
|---|---|
| `bm_event_publish_copy()` | `event_publish_copy()` |
| `bm_event_subscribe()` | `event_subscribe()` |
| `bm_event_register_type()` | `event_register_type()` |

## 数据通道

⚠️ **不兼容**：`bm-channel` 是 SPSC 数据通道，**不是** Zeplod Data Bus。
升级到 Zephyr 后需改用 `data_bus` API（命名 channel、多消费者、引用计数、retain/release）。
