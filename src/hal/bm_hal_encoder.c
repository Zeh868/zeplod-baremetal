/**
 * @file bm_hal_encoder.c
 * @brief 编码器 HAL 默认弱符号桩
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */
#include "bm_hal_encoder.h"
#include "bm_hal_weak.h"
#include "bm_types.h"

BM_HAL_WEAK
int bm_hal_encoder_read(const bm_hal_encoder_t *enc, int32_t *value) {
    (void)enc;
    (void)value;
    return BM_ERR_NOT_INIT;
}
