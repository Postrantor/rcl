// Copyright 2014 Open Source Robotics Foundation, Inc.
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

#ifndef RCL__INIT_H_
#define RCL__INIT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/allocator.h"
#include "rcl/context.h"
#include "rcl/init_options.h"
#include "rcl/macros.h"
#include "rcl/types.h"
#include "rcl/visibility_control.h"

/// 初始化 rcl.
/**
 * 此函数可以运行任意次数，只要给定的上下文已经被正确准备。
 *
 * 给定的 rcl_context_t 必须使用函数 rcl_get_zero_initialized_context() 进行零初始化，
 * 并且不能已经通过此函数初始化。
 * 如果上下文已经初始化，此函数将失败并返回 #RCL_RET_ALREADY_INIT 错误代码。
 * 上下文在使用 rcl_shutdown() 函数完成后可以再次初始化，并使用
 * rcl_get_zero_initialized_context() 再次进行零初始化。
 *
 * `argc` 和 `argv` 参数可能包含程序的命令行参数。
 * 将解析但不删除 rcl 特定的参数。
 * 如果 `argc` 为 `0`，`argv` 为 `NULL`，则不会解析任何参数。
 *
 * `options` 参数必须为非 `NULL`，并且必须使用 rcl_init_options_init() 初始化。
 * 此函数不会修改它，所有权也不会转移到上下文中，而是将其复制到上下文中以供以后参考。
 * 因此，在此函数返回后，需要使用 rcl_init_options_fini() 清理给定的选项。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 是
 * 无锁              | 是 [1]
 * <i>[1] 如果 `atomic_is_lock_free()` 返回 true，则针对 `atomic_uint_least64_t`</i>
 *
 * \param[in] argc argv 中的字符串数量
 * \param[in] argv 命令行参数；rcl 特定参数将被移除
 * \param[in] options 初始化期间使用的选项
 * \param[out] context 表示此初始化的结果上下文对象
 * \return #RCL_RET_OK 如果初始化成功，或
 * \return #RCL_RET_ALREADY_INIT 如果 rcl_init 已经被调用，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_INVALID_ROS_ARGS 如果找到无效的 ROS 参数，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_init(
  int argc, char const * const * argv, const rcl_init_options_t * options, rcl_context_t * context);

/// 关闭给定的 rcl 上下文。
/**
 * 给定的上下文必须使用 rcl_init() 初始化。
 * 如果没有，此函数将失败并返回 #RCL_RET_ALREADY_SHUTDOWN。
 *
 * 调用此函数时：
 *  - 使用此上下文创建的任何 rcl 对象都将失效。
 *  - 在无效对象上调用的函数可能会失败，也可能不会失败。
 *  - 调用 rcl_context_is_initialized() 将返回 `false`。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 是
 * 使用原子操作      | 是
 * 无锁              | 是 [1]
 * <i>[1] 如果 `atomic_is_lock_free()` 返回 true，则针对 `atomic_uint_least64_t`</i>
 *
 * \param[inout] context 要关闭的对象
 * \return #RCL_RET_OK 如果关闭成功完成，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ALREADY_SHUTDOWN 如果上下文当前无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_shutdown(rcl_context_t * context);

#ifdef __cplusplus
}
#endif

#endif  // RCL__INIT_H_
