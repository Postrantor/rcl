// Copyright 2015 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/// @file

#ifndef RCL__GUARD_CONDITION_H_
#define RCL__GUARD_CONDITION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/allocator.h"
#include "rcl/context.h"
#include "rcl/macros.h"
#include "rcl/types.h"
#include "rcl/visibility_control.h"

/// 内部 rcl 守护条件实现结构体。
typedef struct rcl_guard_condition_impl_s rcl_guard_condition_impl_t;

/// rcl 守护条件的句柄。
typedef struct rcl_guard_condition_s {
  /// 与此守护条件关联的上下文。
  rcl_context_t* context;
  /// 指向守护条件实现的指针
  rcl_guard_condition_impl_t* impl;
} rcl_guard_condition_t;

/// rcl 守护条件可用的选项。
typedef struct rcl_guard_condition_options_s {
  /// 守护条件的自定义分配器，用于内部分配。
  rcl_allocator_t allocator;
} rcl_guard_condition_options_t;

/// 返回一个 rcl_guard_condition_t 结构，其成员设置为 `NULL`。
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_guard_condition_t rcl_get_zero_initialized_guard_condition(void);

/// 初始化 rcl 守护条件。
/**
 * 在 rcl_guard_condition_t 上调用此函数后，可以将其传递给
 * rcl_wait()，然后并发地触发它以唤醒 rcl_wait()。
 *
 * 预期用法：
 *
 * ```c
 * #include <rcl/rcl.h>
 *
 * // ... 错误处理
 * rcl_guard_condition_t guard_condition = rcl_get_zero_initialized_guard_condition();
 * // ... 自定义守护条件选项
 * rcl_ret_t ret = rcl_guard_condition_init(
 *   &guard_condition, context, rcl_guard_condition_get_default_options());
 * // ... 错误处理，并在关闭时进行反初始化：
 * ret = rcl_guard_condition_fini(&guard_condition);
 * // ... rcl_guard_condition_fini() 的错误处理
 * ```
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 是
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * \param[inout] guard_condition 预分配的守护条件结构
 * \param[in] context 应与守护条件关联的上下文实例
 * \param[in] options 守护条件的选项
 * \return #RCL_RET_OK 如果守护条件成功初始化，或
 * \return #RCL_RET_ALREADY_INIT 如果守护条件已经初始化，或
 * \return #RCL_RET_NOT_INIT 如果给定的上下文无效，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_guard_condition_init(
    rcl_guard_condition_t* guard_condition,
    rcl_context_t* context,
    const rcl_guard_condition_options_t options);

/// 与 rcl_guard_condition_init() 相同，但重用现有的 rmw 句柄。
/**
 * 除了 rcl_guard_condition_init() 的文档外，
 * `rmw_guard_condition` 参数不能为 `NULL`，并且必须指向有效的
 * rmw 守护条件。
 *
 * 同样，rcl 守护条件的生命周期与 rmw 守护条件的生命周期相绑定。
 * 因此，如果在 rcl 守护条件之前销毁 rmw 守护条件，
 * 则 rcl 守护条件变为无效。
 *
 * 类似地，如果在 rmw 守护条件之前完成了生成的 rcl 守护条件，
 * 那么 rmw 守护条件将不再有效。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 是
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * \param[inout] guard_condition 预分配的守护条件结构
 * \param[in] rmw_guard_condition 要重用的现有 rmw 守护条件
 * \param[in] context 初始化 rmw 守护条件的上下文实例，即 rcl 上下文中的 rmw 上下文需要
 *   与 rmw 守护条件中的 rmw 上下文匹配
 * \param[in] options 守护条件的选项
 * \return #RCL_RET_OK 如果守护条件成功初始化，或
 * \return #RCL_RET_ALREADY_INIT 如果守护条件已经初始化，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
rcl_ret_t rcl_guard_condition_init_from_rmw(
    rcl_guard_condition_t* guard_condition,
    const rmw_guard_condition_t* rmw_guard_condition,
    rcl_context_t* context,
    const rcl_guard_condition_options_t options);

/// 结束 rcl_guard_condition_t。
/**
 * 调用后，使用此守护条件的 rcl_trigger_guard_condition() 调用将失败。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 是
 * 线程安全           | 否 [1]
 * 使用原子操作       | 否
 * 无锁               | 是
 * <i>[1] 与 rcl_trigger_guard_condition() 特别不是线程安全的</i>
 *
 * \param[inout] guard_condition 要完成的守护条件句柄
 * \return #RCL_RET_OK 如果守护条件成功完成，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_guard_condition_fini(rcl_guard_condition_t* guard_condition);

/// 返回 rcl_guard_condition_options_t 结构中的默认选项。
/**
 * 默认值为：
 *
 * - allocator = rcl_get_default_allocator()
 *
 * \return rcl_guard_condition_options_t 结构中的默认选项。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_guard_condition_options_t rcl_guard_condition_get_default_options(void);

/// 触发 rcl 守护条件。
/**
 * 此函数可能失败，并返回 RCL_RET_INVALID_ARGUMENT，如果：
 *   - 守护条件为 `NULL`
 *   - 守护条件无效（从未调用 init 或调用 fini）
 *
 * 守护条件可以从任何线程触发。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 是
 * 线程安全           | 否 [1]
 * 使用原子操作       | 否
 * 无锁               | 是
 * <i>[1] 即使在相同的守护条件上，也可以与自身并发调用</i>
 *
 * \param[in] guard_condition 要触发的守护条件句柄
 * \return #RCL_RET_OK 如果守护条件被触发，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_trigger_guard_condition(rcl_guard_condition_t* guard_condition);

/// 返回守护条件选项。
/**
 * 返回的是指向内部持有的 rcl_guard_condition_options_t 的指针。
 * 如果以下情况之一成立，此函数可能失败，因此返回 `NULL`：
 *   - guard_condition 为 `NULL`
 *   - guard_condition 无效（从未调用 init、调用 fini 或无效节点）
 *
 * 如果守护条件被完成，则返回的指针变为无效。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 否
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * \param[in] guard_condition 指向 rcl 守护条件的指针
 * \return 如果成功，则返回 rcl 守护条件选项，否则返回 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const rcl_guard_condition_options_t* rcl_guard_condition_get_options(
    const rcl_guard_condition_t* guard_condition);

/// 返回 rmw 守护条件句柄。
/**
 * 返回的句柄是指向内部持有的 rmw 句柄的指针。
 * 如果以下情况之一成立，此函数可能失败，因此返回 `NULL`：
 *   - guard_condition 为 `NULL`
 *   - guard_condition 无效（从未调用 init、调用 fini 或无效节点）
 *
 * 如果守护条件被完成或调用 rcl_shutdown()，则返回的句柄将变为无效。
 * 返回的句柄不能保证在守护条件的生命周期内始终有效，因为它可能自身被完成并重新创建。
 * 因此，建议使用此函数在每次需要时从守护条件获取句柄，并避免与可能更改它的函数同时使用句柄。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 否
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * \param[in] guard_condition 指向 rcl 守护条件的指针
 * \return 如果成功，则返回 rmw 守护条件句柄，否则返回 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rmw_guard_condition_t* rcl_guard_condition_get_rmw_handle(
    const rcl_guard_condition_t* guard_condition);

#ifdef __cplusplus
}
#endif

#endif  // RCL__GUARD_CONDITION_H_
