// Copyright 2019 Open Source Robotics Foundation, Inc.
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

#ifndef RCL__NODE_OPTIONS_H_
#define RCL__NODE_OPTIONS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/allocator.h"
#include "rcl/arguments.h"
#include "rcl/domain_id.h"

/// 常量，表示应使用默认的域ID。
#define RCL_NODE_OPTIONS_DEFAULT_DOMAIN_ID RCL_DEFAULT_DOMAIN_ID

/// 封装创建 rcl_node_t 选项的结构体。
typedef struct rcl_node_options_s {
  // bool anonymous_name;
  // rmw_qos_profile_t parameter_qos;
  /// 如果为 true，则不设置参数基础设施。
  // bool no_parameters;
  /// 用于内部分配的自定义分配器。
  rcl_allocator_t allocator;
  /// 如果为 false，则仅使用此结构中的参数，否则还使用全局参数。
  bool use_global_arguments;
  /// 仅适用于此节点的命令行参数。
  rcl_arguments_t arguments;
  /// 启用此节点的 rosout 标志
  bool enable_rosout;
  /// /rosout 的中间件服务质量设置。
  rmw_qos_profile_t rosout_qos;
} rcl_node_options_t;

/// 返回 rcl_node_options_t 中的默认节点选项。
/**
 * 默认值为：
 *
 * - allocator = rcl_get_default_allocator()
 * - use_global_arguments = true
 * - enable_rosout = true
 * - arguments = rcl_get_zero_initialized_arguments()
 * - rosout_qos = rcl_qos_profile_rosout_default
 *
 * \return 具有默认节点选项的结构体。
 */
RCL_PUBLIC
rcl_node_options_t rcl_node_get_default_options(void);

/// 将一个选项结构复制到另一个选项结构。
/**
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] options 要复制的结构体。
 *   其分配器用于将内存复制到新结构体中。
 * \param[out] options_out 包含默认值的选项结构体。
 * \return #RCL_RET_OK 如果结构体复制成功，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何函数参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_node_options_copy(const rcl_node_options_t* options, rcl_node_options_t* options_out);

/// 结束给定的 node_options。
/**
 * 给定的 node_options 必须为非 `NULL` 且有效，即在其上调用了
 * rcl_node_get_default_options()，但尚未调用此函数。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 是
 * 无锁              | 是
 *
 * \param[inout] options 要结束的对象
 * \return #RCL_RET_OK 如果设置成功，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_node_options_fini(rcl_node_options_t* options);

#ifdef __cplusplus
}
#endif

#endif  // RCL__NODE_OPTIONS_H_
