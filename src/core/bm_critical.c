/**
 * @file bm_critical.c
 * @brief 原子变量读写实现（临界区封装）
 *
 * 通过 HAL 临界区保证 32 位原子操作的原子性。
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
#include "bm_hal_critical.h"
#include "bm_atomic.h"

/**
 * @brief 在临界区内原子读取 32 位值
 *
 * @param v 原子变量指针
 * @return 当前值
 */
uint32_t bm_atomic_load(bm_atomic_t *v) {
    bm_irq_state_t s = bm_hal_critical_enter();
    uint32_t val = *v;
    bm_hal_critical_exit(s);
    return val;
}

/**
 * @brief 在临界区内原子写入 32 位值
 *
 * @param v 原子变量指针
 * @param val 待写入的值
 */
void bm_atomic_store(bm_atomic_t *v, uint32_t val) {
    bm_irq_state_t s = bm_hal_critical_enter();
    *v = val;
    bm_hal_critical_exit(s);
}

/**
 * @brief 在临界区内原子自增并返回递增前的值
 *
 * @param v 原子变量指针
 * @return 自增前的值
 */
uint32_t bm_atomic_inc(bm_atomic_t *v) {
    bm_irq_state_t s = bm_hal_critical_enter();
    uint32_t val = (*v)++;
    bm_hal_critical_exit(s);
    return val;
}
