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

#ifndef RCL__ALLOCATOR_H_
#define RCL__ALLOCATOR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcutils/allocator.h"

/// 封装分配器
/**
 * \sa rcutils_allocator_t
 */
typedef rcutils_allocator_t rcl_allocator_t;

/// 返回一个使用默认值正确初始化的 rcl_allocator_t。
/**
 * \sa rcutils_get_default_allocator()
 */
#define rcl_get_default_allocator rcutils_get_default_allocator

/// 模拟 [reallocf](https://linux.die.net/man/3/reallocf) 的行为。
/**
 * \sa rcutils_reallocf()
 */
#define rcl_reallocf rcutils_reallocf

/// 检查给定的分配器是否已初始化。
/**
 * 如果分配器未初始化，则运行 fail_statement。
 * \param[in] allocator 要检查的分配器
 * \param[in] fail_statement 如果分配器未初始化，要执行的语句
 */
#define RCL_CHECK_ALLOCATOR(allocator, fail_statement) \
  RCUTILS_CHECK_ALLOCATOR(allocator, fail_statement)

/// 检查给定的分配器是否已初始化，如果没有则带有消息失败。
/**
 * 如果分配器未初始化，设置错误为 msg，并运行 fail_statement。
 * \param[in] allocator 要检查的分配器
 * \param[in] msg 设置的错误消息
 * \param[in] fail_statement 如果分配器未初始化，要执行的语句
 */
#define RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, msg, fail_statement) \
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(allocator, msg, fail_statement)

#ifdef __cplusplus
}
#endif

#endif  // RCL__ALLOCATOR_H_
