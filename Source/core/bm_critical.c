/**
 * @file bm_critical.c
 * @brief 原子变量读写实现（临界区封装）
 *
 * 通过框架临界区宏保证 32 位原子操作的原子性。
 * @author zeh (china_qzh@163.com)
 * @version 1.1
 * @date 2026-06-10
 *
 * @par 修改日志:
 *
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 * 2026-06-10       1.1            zeh            统一 BM_CRITICAL 宏；修正 inc 语义
 *
 */
#include "bm_atomic.h"
#include "bm_critical_wrap.h"

#include <stdint.h>

/**
 * @brief 在临界区内原子读取 32 位值
 *
 * @param v 原子变量指针
 * @return 当前值；v 为 NULL 时返回 0
 */
uint32_t bm_atomic_load(bm_atomic_t *v) {
    if (!v) {
        return 0u;
    }
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    uint32_t val = *v;
    BM_CRITICAL_EXIT(s);
    return val;
}

/**
 * @brief 在临界区内原子写入 32 位值
 *
 * @param v 原子变量指针
 * @param val 待写入的值
 */
void bm_atomic_store(bm_atomic_t *v, uint32_t val) {
    if (!v) {
        return;
    }
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    *v = val;
    BM_CRITICAL_EXIT(s);
}

/**
 * @brief 在临界区内原子自增并返回递增后的值
 *
 * @param v 原子变量指针
 * @return 自增后的值；v 为 NULL 时返回 0
 */
uint32_t bm_atomic_inc(bm_atomic_t *v) {
    if (!v) {
        return 0u;
    }
    bm_irq_state_t s = BM_CRITICAL_ENTER();
    uint32_t val = *v;

    if (val < UINT32_MAX) {
        val++;
        *v = val;
    }
    BM_CRITICAL_EXIT(s);
    return val;
}
