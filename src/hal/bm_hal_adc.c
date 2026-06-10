/**
 * @file bm_hal_adc.c
 * @brief ADC HAL 默认弱符号桩
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */
#include "bm_hal_adc.h"
#include "bm_hal_weak.h"
#include "bm_types.h"

BM_HAL_WEAK
int bm_hal_adc_read_injected(const bm_hal_adc_t *adc,
                             uint32_t rank, uint16_t *value) {
    (void)adc;
    (void)rank;
    (void)value;
    return BM_ERR_NOT_INIT;
}

BM_HAL_WEAK
int bm_hal_adc_bind_complete(const bm_hal_adc_t *adc,
                             const bm_hal_hrt_binding_t *binding) {
    (void)adc;
    (void)binding;
    return BM_ERR_NOT_INIT;
}
