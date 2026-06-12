/**
 * @file bm_hal_adc.c
 * @brief ADC HAL 分发层（契约 → driver API）
 */
#include "bm_hal_adc.h"
#include "bm_types.h"

int bm_hal_adc_read_injected(const bm_hal_adc_t *adc,
                             uint32_t rank, uint16_t *value) {
    if (!adc || !adc->api || !adc->api->read_injected) {
        return BM_ERR_NOT_INIT;
    }
    return adc->api->read_injected(adc, rank, value);
}

int bm_hal_adc_bind_complete(const bm_hal_adc_t *adc,
                             const bm_hal_hrt_binding_t *binding) {
    if (!adc || !adc->api || !adc->api->bind_complete) {
        return BM_ERR_NOT_INIT;
    }
    return adc->api->bind_complete(adc, binding);
}
