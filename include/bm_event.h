/**
 * @file bm_event.h
 * @brief 发布-订阅事件总线
 *
 * 支持优先级队列、ISR 安全发布及批量消费处理。
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
#ifndef BM_EVENT_H
#define BM_EVENT_H

#include "bm_types.h"

#include <stddef.h>

typedef uint8_t bm_event_type_t;              /**< 事件类型 ID */
typedef uint8_t bm_event_priority_t;          /**< 事件优先级 */
typedef uint32_t bm_event_subscriber_id_t;    /**< 订阅者句柄 */

/** 事件载荷 */
typedef struct {
    bm_event_type_t      type;
    bm_event_priority_t  priority;
    const void          *data;
    size_t               data_len;
    uint8_t              source_id;
} bm_event_t;

/** 订阅回调函数 */
typedef void (*bm_event_callback_t)(const bm_event_t *event, void *user_data);

/**
 * @brief 重置事件子系统为初始状态
 */
void bm_event_reset(void);

/**
 * @brief 注册事件类型
 *
 * @param type 事件类型 ID
 * @param name 类型名称（非 NULL）
 * @return BM_OK 成功；BM_ERR_ALREADY 已注册；BM_ERR_INVALID 参数无效
 */
int bm_event_register_type(bm_event_type_t type, const char *name);

/**
 * @brief 订阅指定类型的事件
 *
 * @param type 事件类型 ID
 * @param cb 回调函数
 * @param user_data 用户上下文指针
 * @param id 输出订阅者句柄（可为 NULL）
 * @return BM_OK 成功；BM_ERR_NO_MEM 订阅表已满；BM_ERR_INVALID 参数无效
 */
int bm_event_subscribe(bm_event_type_t type, bm_event_callback_t cb,
                       void *user_data, bm_event_subscriber_id_t *id);

/**
 * @brief 取消订阅
 *
 * @param type 事件类型 ID
 * @param id 订阅者句柄
 * @return BM_OK 成功；BM_ERR_NOT_FOUND 未找到订阅
 */
int bm_event_unsubscribe(bm_event_type_t type, bm_event_subscriber_id_t id);

/**
 * @brief 发布事件（内部拷贝数据）
 *
 * @param type 事件类型 ID
 * @param prio 事件优先级
 * @param data 载荷数据指针
 * @param len 载荷字节长度
 * @return BM_OK 成功；BM_ERR_OVERFLOW 队列已满；BM_ERR_INVALID 参数无效
 */
int bm_event_publish_copy(bm_event_type_t type, bm_event_priority_t prio,
                          const void *data, size_t len);

/**
 * @brief SRT 域 ISR 上下文发布事件（内部拷贝数据）
 *
 * 单核下通过关中断临界区实现；禁止在 HRT ISR 中调用。
 *
 * @param type 事件类型 ID
 * @param prio 事件优先级
 * @param data 载荷数据指针
 * @param len 载荷字节长度
 * @return BM_OK 成功；BM_ERR_OVERFLOW 队列已满；BM_ERR_INVALID 参数无效
 */
int bm_event_publish_copy_from_isr(bm_event_type_t type, bm_event_priority_t prio,
                                   const void *data, size_t len);

/**
 * @brief 发布完整事件结构（载荷拷贝到内联缓冲，≤ BM_CONFIG_EVENT_INLINE_DATA_SIZE）
 *
 * @param event 事件描述指针
 * @return BM_OK 成功；BM_ERR_OVERFLOW 队列已满；BM_ERR_NO_MEM 载荷过大；BM_ERR_INVALID 参数无效
 */
int bm_event_publish_event(const bm_event_t *event);

/**
 * @brief SRT 域 ISR 上下文发布完整事件结构
 *
 * 单核下通过关中断临界区实现；禁止在 HRT ISR 中调用。
 *
 * @param event 事件描述指针
 * @return BM_OK 成功；BM_ERR_OVERFLOW 队列已满；BM_ERR_INVALID 参数无效
 */
int bm_event_publish_event_from_isr(const bm_event_t *event);

/**
 * @brief 从队列取出并分发事件
 *
 * 回调内禁止 subscribe/unsubscribe/publish/reset（非重入）。
 *
 * @param max_events 本次最多处理的事件条数
 * @return 实际处理的事件条数
 */
int bm_event_process(uint32_t max_events);

/**
 * @brief 查询因事件队列满而丢弃的发布次数
 *
 * @return 丢弃计数（reset 后清零）
 */
uint32_t bm_event_get_dropped_count(void);

/**
 * @brief 查询分发阶段因无效类型或快照截断而跳过的次数
 *
 * @return 跳过计数（reset 后清零）
 */
uint32_t bm_event_get_dispatch_skipped_count(void);

#endif /* BM_EVENT_H */
