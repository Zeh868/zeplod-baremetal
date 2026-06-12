/**
 * @file bm_types.h
 * @brief 框架公共类型与错误码定义
 *
 * 提供统一返回值宏、IRQ 状态类型及原子变量类型别名。
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
#ifndef BM_TYPES_H
#define BM_TYPES_H

#include <bm_config.h>

#include <stdint.h>

/** 操作成功 */
#define BM_OK                0
/** 内存不足 */
#define BM_ERR_NO_MEM       -1
/** 未找到目标 */
#define BM_ERR_NOT_FOUND    -2
/** 非阻塞操作暂无数据 */
#define BM_ERR_WOULD_BLOCK  -3
/** 参数或状态无效 */
#define BM_ERR_INVALID      -4
/** 资源忙 */
#define BM_ERR_BUSY         -5
/** 缓冲区溢出 */
#define BM_ERR_OVERFLOW     -6
/** 子系统未初始化 */
#define BM_ERR_NOT_INIT     -7
/** 重复操作（已存在/已初始化） */
#define BM_ERR_ALREADY      -8

/** 中断屏蔽状态快照 */
typedef uint32_t bm_irq_state_t;
/** 原子计数器变量类型 */
typedef volatile uint32_t bm_atomic_t;

#endif /* BM_TYPES_H */
