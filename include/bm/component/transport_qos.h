/**
 * @file transport_qos.h
 * @brief 传输 QoS：延迟与抖动监控骨架
 *
 * @maturity E1
 * @author zeh (china_qzh@163.com)
 * @version 0.1
 * @date 2026-06-13
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-13       0.1            zeh            初始骨架
 */
#ifndef BM_TRANSPORT_QOS_H
#define BM_TRANSPORT_QOS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BM_TRANSPORT_QOS_TEL_VALID   (1u << 0u)
#define BM_TRANSPORT_QOS_TEL_ALARM (1u << 1u)

typedef struct {
    uint32_t sequence;
    uint32_t status;
    float    latency_ms;
    float    jitter_ms;
    float    latency_ema_ms;
} bm_transport_qos_telemetry_t;

typedef uint32_t (*bm_transport_qos_now_ms_fn)(void *user);

typedef void (*bm_transport_qos_publish_fn)(
    void *user,
    const bm_transport_qos_telemetry_t *telemetry);

typedef struct {
    bm_transport_qos_now_ms_fn    now_ms;
    void                         *now_ms_user;
    bm_transport_qos_publish_fn  publish_telemetry;
    void                         *publish_telemetry_user;
} bm_transport_qos_resources_t;

typedef struct {
    float    latency_alarm_ms;
    float    jitter_alarm_ms;
    float    ema_alpha;
} bm_transport_qos_config_t;

typedef struct {
    uint32_t prev_tx_ms;
    uint32_t prev_rx_ms;
    float    latency_ms;
    float    jitter_ms;
    float    latency_ema_ms;
    int      have_prev;
    uint32_t step_count;
    bm_transport_qos_telemetry_t telemetry;
} bm_transport_qos_state_t;

typedef struct {
    bm_transport_qos_config_t    config;
    bm_transport_qos_resources_t resources;
    bm_transport_qos_state_t     state;
} bm_transport_qos_axis_t;

int  bm_transport_qos_validate_config(const bm_transport_qos_config_t *config);
int  bm_transport_qos_init(bm_transport_qos_axis_t *axis);
void bm_transport_qos_reset(bm_transport_qos_axis_t *axis);
void bm_transport_qos_on_tx(bm_transport_qos_axis_t *axis);
void bm_transport_qos_on_rx(bm_transport_qos_axis_t *axis);
void bm_transport_qos_step(bm_transport_qos_axis_t *axis);

#ifdef __cplusplus
}
#endif

#endif /* BM_TRANSPORT_QOS_H */
