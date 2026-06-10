/**
 * @file bm_event.c
 * @brief 发布-订阅事件总线实现
 *
 * 环形优先级队列 + 链表订阅者；临界区保护多生产者/消费者。
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 *
 */
#include "bm_event.h"
#include "bm_critical_wrap.h"
#include "bm_log.h"

#include <stdbool.h>
#include <string.h>

#ifndef BM_CONFIG_MAX_EVENT_TYPES
#define BM_CONFIG_MAX_EVENT_TYPES       16
#endif

#ifndef BM_CONFIG_MAX_EVENT_SUBSCRIBERS
#define BM_CONFIG_MAX_EVENT_SUBSCRIBERS 32
#endif

#ifndef BM_CONFIG_EVENT_QUEUE_SIZE
#define BM_CONFIG_EVENT_QUEUE_SIZE      16
#endif

#ifndef BM_CONFIG_EVENT_PRIORITIES
#define BM_CONFIG_EVENT_PRIORITIES      4
#endif

#if BM_CONFIG_EVENT_QUEUE_SIZE < 2 || \
    (BM_CONFIG_EVENT_QUEUE_SIZE & (BM_CONFIG_EVENT_QUEUE_SIZE - 1)) != 0
#error "BM_CONFIG_EVENT_QUEUE_SIZE must be a power of two and at least 2"
#endif

#ifndef BM_CONFIG_EVENT_INLINE_DATA_SIZE
#define BM_CONFIG_EVENT_INLINE_DATA_SIZE 8
#endif

/** 订阅者节点 */
typedef struct bm_subscriber {
    bm_event_callback_t           cb;
    void                         *user_data;
    bm_event_subscriber_id_t      id;
    struct bm_subscriber         *next;
} bm_subscriber_t;

/** 事件类型槽：名称 + 订阅者链表头 */
typedef struct {
    const char          *name;
    bm_subscriber_t     *head;
} bm_event_type_slot_t;

/** 队列项：事件 + 内联数据缓冲 */
typedef struct {
    bm_event_t  event;
    uint8_t     inline_data[BM_CONFIG_EVENT_INLINE_DATA_SIZE];
} bm_queue_item_t;

static bm_event_type_slot_t _event_types[BM_CONFIG_MAX_EVENT_TYPES];
static bm_subscriber_t      _subscribers[BM_CONFIG_MAX_EVENT_SUBSCRIBERS];
static bm_queue_item_t      _event_queue[BM_CONFIG_EVENT_QUEUE_SIZE];
static uint32_t             _queue_write = 0;
static uint32_t             _queue_read = 0;
static uint32_t             _next_subscriber_id = 1;

/**
 * @brief 深拷贝队列项并修正内联 data 指针
 *
 * @param dst 目标队列项
 * @param src 源队列项
 */
static void _queue_item_copy(bm_queue_item_t *dst, const bm_queue_item_t *src) {
    bool owns_inline_data = (src->event.data == src->inline_data);
    *dst = *src;
    if (owns_inline_data) {
        dst->event.data = dst->inline_data;
    }
}

/**
 * @brief 重置事件总线全部内部状态
 */
void bm_event_reset(void) {
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    memset(_event_types, 0, sizeof(_event_types));
    memset(_subscribers, 0, sizeof(_subscribers));
    memset(_event_queue, 0, sizeof(_event_queue));
    _queue_write = 0;
    _queue_read = 0;
    _next_subscriber_id = 1;
    BM_CRITICAL_EXIT(s);
    BM_LOGI("event", "event bus reset");
}

/**
 * @brief 注册事件类型并绑定名称
 *
 * @param type 事件类型枚举值
 * @param name 类型名称字符串
 * @return BM_OK 成功；BM_ERR_INVALID 参数无效
 */
int bm_event_register_type(bm_event_type_t type, const char *name) {
    if (type >= BM_CONFIG_MAX_EVENT_TYPES || !name) {
        BM_LOGE("event", "register_type invalid type=%u", (unsigned)type);
        return BM_ERR_INVALID;
    }
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    _event_types[type].name = name;
    BM_CRITICAL_EXIT(s);
    BM_LOGD("event", "type %u registered as '%s'", (unsigned)type, name);
    return BM_OK;
}

/**
 * @brief 订阅指定类型的事件
 *
 * @param type 事件类型
 * @param cb 回调函数
 * @param user_data 用户上下文指针
 * @param id 输出订阅者 ID
 * @return BM_OK 成功；BM_ERR_INVALID 参数无效；BM_ERR_NO_MEM 订阅者槽已满
 */
int bm_event_subscribe(bm_event_type_t type, bm_event_callback_t cb,
                       void *user_data, bm_event_subscriber_id_t *id) {
    if (!cb || type >= BM_CONFIG_MAX_EVENT_TYPES || !id) {
        BM_LOGE("event", "subscribe invalid args type=%u", (unsigned)type);
        return BM_ERR_INVALID;
    }
    bm_irq_state_t s = BM_CRITICAL_ENTER();

    bm_subscriber_t *sub = NULL;
    for (int i = 0; i < BM_CONFIG_MAX_EVENT_SUBSCRIBERS; i++) {
        if (_subscribers[i].id == 0) {
            sub = &_subscribers[i];
            break;
        }
    }
    if (!sub) {
        BM_CRITICAL_EXIT(s);
        BM_LOGW("event", "subscribe no free slot for type=%u", (unsigned)type);
        return BM_ERR_NO_MEM;
    }

    sub->cb = cb;
    sub->user_data = user_data;
    sub->id = _next_subscriber_id++;
    sub->next = _event_types[type].head;
    _event_types[type].head = sub;
    *id = sub->id;

    BM_CRITICAL_EXIT(s);
    BM_LOGD("event", "subscribed id=%u type=%u", (unsigned)*id, (unsigned)type);
    return BM_OK;
}

/**
 * @brief 取消指定订阅者的事件订阅
 *
 * @param type 事件类型
 * @param id 订阅者 ID
 * @return BM_OK 成功；BM_ERR_INVALID 参数无效；BM_ERR_NOT_FOUND 未找到订阅者
 */
int bm_event_unsubscribe(bm_event_type_t type, bm_event_subscriber_id_t id) {
    if (type >= BM_CONFIG_MAX_EVENT_TYPES || id == 0) {
        return BM_ERR_INVALID;
    }
    bm_irq_state_t s = BM_CRITICAL_ENTER();

    bm_subscriber_t **pp = &_event_types[type].head;
    while (*pp) {
        if ((*pp)->id == id) {
            bm_subscriber_t *to_remove = *pp;
            *pp = to_remove->next;
            to_remove->id = 0;
            to_remove->cb = NULL;
            to_remove->next = NULL;
            BM_CRITICAL_EXIT(s);
            BM_LOGD("event", "unsubscribed id=%u type=%u", (unsigned)id, (unsigned)type);
            return BM_OK;
        }
        pp = &(*pp)->next;
    }

    BM_CRITICAL_EXIT(s);
    BM_LOGW("event", "unsubscribe id=%u not found", (unsigned)id);
    return BM_ERR_NOT_FOUND;
}

/**
 * @brief 将事件及可选载荷压入环形队列
 *
 * @param event 事件描述符指针
 * @param data 载荷数据指针（可为 NULL）
 * @param len 载荷字节长度
 * @return BM_OK 成功；BM_ERR_OVERFLOW 队列满；BM_ERR_NO_MEM 载荷过大
 */
static int _queue_push_copy(const bm_event_t *event, const void *data, size_t len) {
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    uint32_t mask = BM_CONFIG_EVENT_QUEUE_SIZE - 1;
    uint32_t next = (_queue_write + 1) & mask;
    if (next == _queue_read) {
        BM_CRITICAL_EXIT(s);
        BM_LOGW("event", "queue overflow type=%u", (unsigned)event->type);
        return BM_ERR_OVERFLOW;
    }

    bm_queue_item_t *item = &_event_queue[_queue_write & mask];
    item->event = *event;

    if (data && len > 0) {
        if (len <= sizeof(item->inline_data)) {
            memcpy(item->inline_data, data, len);
            item->event.data = item->inline_data;
        } else {
            BM_CRITICAL_EXIT(s);
            BM_LOGE("event", "payload too large len=%u", (unsigned)len);
            return BM_ERR_NO_MEM;
        }
    } else {
        item->event.data = NULL;
        item->event.data_len = 0;
    }

    _queue_write = next;
    BM_CRITICAL_EXIT(s);
    return BM_OK;
}

/**
 * @brief 发布事件并拷贝载荷到内联缓冲
 *
 * @param type 事件类型
 * @param prio 事件优先级
 * @param data 载荷数据指针（可为 NULL）
 * @param len 载荷字节长度
 * @return BM_OK 成功；BM_ERR_INVALID 参数无效；其他为队列错误码
 */
int bm_event_publish_copy(bm_event_type_t type, bm_event_priority_t prio,
                          const void *data, size_t len) {
    if (type >= BM_CONFIG_MAX_EVENT_TYPES ||
        prio >= BM_CONFIG_EVENT_PRIORITIES ||
        (len > 0 && !data)) {
        return BM_ERR_INVALID;
    }
    bm_event_t ev = {
        .type = type,
        .priority = prio,
        .data = NULL,
        .data_len = len,
        .source_id = 0
    };
    return _queue_push_copy(&ev, data, len);
}

/**
 * @brief 从 ISR 上下文发布事件并拷贝载荷
 *
 * @param type 事件类型
 * @param prio 事件优先级
 * @param data 载荷数据指针（可为 NULL）
 * @param len 载荷字节长度
 * @return BM_OK 成功；BM_ERR_INVALID 参数无效；其他为队列错误码
 */
int bm_event_publish_copy_from_isr(bm_event_type_t type, bm_event_priority_t prio,
                                   const void *data, size_t len) {
    return bm_event_publish_copy(type, prio, data, len);
}

/**
 * @brief 发布完整事件结构（不拷贝载荷，调用方保证生命周期）
 *
 * @param event 事件描述符指针
 * @return BM_OK 成功；BM_ERR_INVALID 参数无效；BM_ERR_OVERFLOW 队列满
 */
int bm_event_publish_event(const bm_event_t *event) {
    if (!event ||
        event->type >= BM_CONFIG_MAX_EVENT_TYPES ||
        event->priority >= BM_CONFIG_EVENT_PRIORITIES ||
        (event->data_len > 0 && !event->data)) {
        return BM_ERR_INVALID;
    }
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    uint32_t mask = BM_CONFIG_EVENT_QUEUE_SIZE - 1;
    uint32_t next = (_queue_write + 1) & mask;
    if (next == _queue_read) {
        BM_CRITICAL_EXIT(s);
        BM_LOGW("event", "queue overflow type=%u", (unsigned)event->type);
        return BM_ERR_OVERFLOW;
    }
    _event_queue[_queue_write & mask].event = *event;
    _queue_write = next;
    BM_CRITICAL_EXIT(s);
    return BM_OK;
}

/**
 * @brief 从 ISR 上下文发布完整事件结构
 *
 * @param event 事件描述符指针
 * @return BM_OK 成功；BM_ERR_INVALID 参数无效；BM_ERR_OVERFLOW 队列满
 */
int bm_event_publish_event_from_isr(const bm_event_t *event) {
    /* 平台临界区序列化 ISR 与主循环生产者 */
    return bm_event_publish_event(event);
}

/**
 * @brief 弹出队列中当前最高优先级事件
 *
 * @param out 输出队列项缓冲区
 * @return BM_OK 成功；BM_ERR_WOULD_BLOCK 队列为空
 */
static int _queue_pop_highest_prio(bm_queue_item_t *out) {
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    if (_queue_read == _queue_write) {
        BM_CRITICAL_EXIT(s);
        return BM_ERR_WOULD_BLOCK;
    }

    uint32_t mask = BM_CONFIG_EVENT_QUEUE_SIZE - 1;
    uint32_t best_idx = 0;
    uint8_t best_prio = 0xFF;
    bool found = false;

    for (uint32_t i = _queue_read; i != _queue_write; i = (i + 1) & mask) {
        bm_event_t *ev = &_event_queue[i & mask].event;
        if (ev->priority < best_prio) {
            best_prio = ev->priority;
            best_idx = i;
            found = true;
        }
    }

    if (!found) {
        BM_CRITICAL_EXIT(s);
        return BM_ERR_WOULD_BLOCK;
    }

    uint32_t read_slot = _queue_read & mask;
    uint32_t best_slot = best_idx & mask;
    _queue_item_copy(out, &_event_queue[best_slot]);
    if (best_slot != read_slot) {
        _queue_item_copy(&_event_queue[best_slot], &_event_queue[read_slot]);
    }
    _queue_read = (_queue_read + 1) & mask;

    BM_CRITICAL_EXIT(s);
    return BM_OK;
}

/**
 * @brief 处理队列中至多 max_events 条事件并分发给订阅者
 *
 * @param max_events 单次最多处理的事件数
 * @return 实际处理的事件数量
 */
int bm_event_process(uint32_t max_events) {
    uint32_t processed = 0;
    for (uint32_t i = 0; i < max_events; i++) {
        bm_queue_item_t item;
        int rc = _queue_pop_highest_prio(&item);
        if (rc != BM_OK) break;

        bm_subscriber_t *sub = _event_types[item.event.type].head;
        while (sub) {
            bm_subscriber_t *next = sub->next;
            if (sub->cb) {
                sub->cb(&item.event, sub->user_data);
            }
            sub = next;
        }
        processed++;
    }
    BM_LOGT("event", "processed %u events", (unsigned)processed);
    return (int)processed;
}
