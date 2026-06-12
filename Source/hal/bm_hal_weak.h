/**
 * @file bm_hal_weak.h
 * @brief HAL 弱符号宏（默认桩可被平台参考实现覆盖）
 * @author zeh (china_qzh@163.com)
 * @version 1.0
 * @date 2026-06-10
 * @par 修改日志:
 *    Date         Version        Author          Description
 * 2026-06-10       1.0            zeh            正式发布
 */
#ifndef BM_HAL_WEAK_H
#define BM_HAL_WEAK_H

#if defined(__GNUC__) || defined(__clang__)
#define BM_HAL_WEAK __attribute__((weak))
#else
#define BM_HAL_WEAK
#endif

#endif /* BM_HAL_WEAK_H */
