// Copyright 2018 Open Source Robotics Foundation, Inc.
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

#ifndef RCL__INIT_OPTIONS_H_
#define RCL__INIT_OPTIONS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/allocator.h"
#include "rcl/macros.h"
#include "rcl/types.h"
#include "rcl/visibility_control.h"
#include "rmw/init.h"

/// rcl_init_options_impl_s 结构体的类型定义
typedef struct rcl_init_options_impl_s rcl_init_options_impl_t;

/**
 * @brief 初始化选项和实现定义的初始化选项的封装。
 */
typedef struct rcl_init_options_s
{
  /// 实现特定的指针。
  rcl_init_options_impl_t * impl;
} rcl_init_options_t;

/**
 * @brief 返回一个零初始化的 rcl_init_options_t 结构体。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_init_options_t rcl_get_zero_initialized_init_options(void);

/**
 * @brief 使用默认值和实现特定值初始化给定的 init_options。
 *
 * @param[inout] init_options 要设置的对象
 * @param[in] allocator 在设置和初始化期间使用的分配器
 * @return #RCL_RET_OK 如果设置成功，或
 * @return #RCL_RET_ALREADY_INIT 如果 init_options 已经初始化，或
 * @return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * @return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * @return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_init_options_init(rcl_init_options_t * init_options, rcl_allocator_t allocator);

/**
 * @brief 将给定的源 init_options 复制到目标 init_options。
 *
 * @param[in] src 要从中复制的 rcl_init_options_t 对象
 * @param[out] dst 要复制到的 rcl_init_options_t 对象
 * @return #RCL_RET_OK 如果复制成功，或
 * @return #RCL_RET_ALREADY_INIT 如果 dst 已经初始化，或
 * @return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * @return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * @return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_init_options_copy(const rcl_init_options_t * src, rcl_init_options_t * dst);

/**
 * @brief 结束给定的 init_options。
 *
 * @param[inout] init_options 要设置的对象
 * @return #RCL_RET_OK 如果设置成功，或
 * @return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * @return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_init_options_fini(rcl_init_options_t * init_options);

/**
 * @brief 返回 init_options 中存储的 domain_id。
 *
 * @param[in] init_options 从中检索域 ID 的对象
 * @param[out] domain_id 要在 init_options 对象中设置的域 ID
 * @return #RCL_RET_OK 如果成功，或
 * @return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_init_options_get_domain_id(
  const rcl_init_options_t * init_options, size_t * domain_id);

/**
 * @brief 在提供的 init_options 中设置一个 domain id。
 *
 * @param[in] init_options 要设置指定域 ID 的对象
 * @param[in] domain_id 要在 init_options 对象中设置的域 ID
 * @return #RCL_RET_OK 如果成功，或
 * @return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_init_options_set_domain_id(rcl_init_options_t * init_options, size_t domain_id);

/**
 * @brief 返回存储在内部的 rmw 初始化选项。
 *
 * @param[in] init_options 从中检索 rmw 初始化选项的对象
 * @return 指向 rcl 初始化选项的指针，或
 * @return `NULL` 如果有错误
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rmw_init_options_t * rcl_init_options_get_rmw_init_options(rcl_init_options_t * init_options);

/**
 * @brief 返回 init_options 中存储的分配器。
 *
 * @param[in] init_options 从中检索分配器的对象
 * @return 指向 rcl 分配器的指针，或
 * @return `NULL` 如果有错误
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const rcl_allocator_t * rcl_init_options_get_allocator(const rcl_init_options_t * init_options);
