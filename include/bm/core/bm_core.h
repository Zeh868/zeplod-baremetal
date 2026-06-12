/**
 * @file bm_core.h
 * @brief 核心子系统兼容聚合头文件
 *
 * 一次性引入 atomic、event、mempool、types 等基础模块。
 * 新代码建议按需直接包含各子模块头文件以明确依赖。
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
#ifndef BM_CORE_H
#define BM_CORE_H

#include "bm_atomic.h"
#include "bm_event.h"
#include "bm_mempool.h"
#include "bm_types.h"

#endif /* BM_CORE_H */
