/**
 * @file bm_hal_comp_stm32g4.c
 * @brief STM32G4 比较器 HAL 参考实现
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */
#include "bm_hal_comp.h"
#include "bm_hal_instances_stm32g4.h"
#include "bm_hal_regs_stm32g4.h"
#include "bm_types.h"

struct bm_hal_comp {
    uint32_t comp_base;
};

const bm_hal_comp_t BM_HAL_COMP1 = {
    BM_STM32G4_COMP1_BASE,
};

static const bm_hal_comp_t *comp_valid(const bm_hal_comp_t *comp) {
    if (!comp || comp->comp_base == 0u) {
        return NULL;
    }
    return comp;
}

int bm_hal_comp_clear_latch(const bm_hal_comp_t *comp) {
    const bm_hal_comp_t *inst = comp_valid(comp);
    uint32_t csr;

    if (!inst) {
        return BM_ERR_INVALID;
    }

    csr = BM_COMP_CSR(inst->comp_base);
    if ((csr & BM_COMP_CSR_LOCK) != 0u) {
        return BM_ERR_BUSY;
    }
    csr &= ~BM_COMP_CSR_VALUE;
    BM_COMP_CSR(inst->comp_base) = csr;
    return BM_OK;
}
