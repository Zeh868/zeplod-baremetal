# Ultra → Core 迁移指南

## 事件系统

| Ultra | Core |
|---|---|
| `BM_ULTRA_EVENT_BIND(EVENT, cb)` | `bm_event_register_type(EVENT, "name"); bm_event_subscribe(EVENT, cb, ud, &id);` |
| `bm_ultra_publish(type, data, len)` | `bm_event_publish_copy(type, prio, data, len)` |
| 无优先级 | 显式 `bm_event_priority_t`（0=最高） |
| 无 user_data | 订阅时传入 `user_data` |
| FIFO | 优先级扫描出队 |
| 单回调 | 多订阅者链表 |

## 代码示例

```c
// Ultra
static const bm_ultra_callback_t _bm_ultra_callbacks[8] = {
    BM_ULTRA_CB(EVENT_TEMP, on_temp)
};

void on_temp(const void *data, uint8_t len) {
    uint16_t val = *(const uint16_t *)data;
}

bm_ultra_publish(EVENT_TEMP, &val, sizeof(val));
```

```c
// Core
bm_event_register_type(EVENT_TEMP, "TEMP");
bm_event_subscriber_id_t id;
bm_event_subscribe(EVENT_TEMP, on_temp, NULL, &id);

void on_temp(const bm_event_t *event, void *user_data) {
    uint16_t val = *(const uint16_t *)event->data;
}

bm_event_publish_copy(EVENT_TEMP, BM_EVENT_PRIO_NORMAL, &val, sizeof(val));
```

## 数据所有权

- **Ultra**: 值拷贝到队列内联数组，无需管理内存
- **Core**: `publish_copy` 由框架拷贝（≤8B 内联），调用方无需管理；`publish_event` 由调用方保证生命周期
