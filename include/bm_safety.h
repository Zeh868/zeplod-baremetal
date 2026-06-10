/**
 * @file bm_safety.h
 * @brief 框架内部安全算术与范围校验辅助（header-only）
 *
 * 用于 SIL-2 对齐的运行时防御：乘法/加法溢出检测、索引上界检查。
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 */
#ifndef BM_SAFETY_H
#define BM_SAFETY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief 无符号 32 位乘法溢出检测
 *
 * @param a 乘数
 * @param b 乘数
 * @param out 成功时写入乘积（可为 NULL）
 * @return true 发生溢出；false 无溢出
 */
static inline bool bm_mul_u32_overflow(uint32_t a, uint32_t b, uint32_t *out) {
    if (a == 0u || b == 0u) {
        if (out) {
            *out = 0u;
        }
        return false;
    }
    if (a > (UINT32_MAX / b)) {
        return true;
    }
    if (out) {
        *out = a * b;
    }
    return false;
}

/**
 * @brief 无符号 32 位加法溢出检测
 *
 * @param a 加数
 * @param b 加数
 * @param out 成功时写入和（可为 NULL）
 * @return true 发生溢出；false 无溢出
 */
static inline bool bm_add_u32_overflow(uint32_t a, uint32_t b, uint32_t *out) {
    if (a > (UINT32_MAX - b)) {
        return true;
    }
    if (out) {
        *out = a + b;
    }
    return false;
}

/**
 * @brief 判断索引是否小于上界（fail-stop 范围检查）
 */
static inline bool bm_index_in_range_u32(uint32_t index, uint32_t limit) {
    return index < limit;
}

#endif /* BM_SAFETY_H */
