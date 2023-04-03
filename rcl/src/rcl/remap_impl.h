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

#ifndef RCL__REMAP_IMPL_H_
#define RCL__REMAP_IMPL_H_

#include "rcl/allocator.h"
#include "rcl/macros.h"
#include "rcl/remap.h"
#include "rcl/types.h"
#include "rcl/visibility_control.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief rcl_remap_type_t 枚举类型，用作主题和服务规则的位掩码。
typedef enum rcl_remap_type_t {
  RCL_UNKNOWN_REMAP = 0,         ///< 未知的重映射类型
  RCL_TOPIC_REMAP = 1u << 0,     ///< 主题重映射类型
  RCL_SERVICE_REMAP = 1u << 1,   ///< 服务重映射类型
  RCL_NODENAME_REMAP = 1u << 2,  ///< 节点名称重映射类型
  RCL_NAMESPACE_REMAP = 1u << 3  ///< 命名空间重映射类型
} rcl_remap_type_t;

/// \brief rcl_remap_impl_s 结构体，包含重映射规则的实现细节。
struct rcl_remap_impl_s {
  /// 表示规则类型的位掩码。
  rcl_remap_type_t type;
  /// 此规则限制的节点名称，如果适用于任何节点，则为 NULL。
  char* node_name;
  /// 规则的匹配部分，如果是节点名称或命名空间替换，则为 NULL。
  char* match;
  /// 规则的替换部分。
  char* replacement;

  /// 用于在此结构中分配对象的分配器。
  rcl_allocator_t allocator;
};

/**
 * \brief 重映射名称函数。
 *
 * \param[in] local_arguments 本地参数
 * \param[in] global_arguments 全局参数
 * \param[in] type_bitmask 类型位掩码
 * \param[in] name 要重映射的名称
 * \param[in] node_name 节点名称
 * \param[in] node_namespace 节点命名空间
 * \param[in] substitutions 替换字符串映射
 * \param[in] allocator 分配器
 * \param[out] output_name 输出重映射后的名称
 * \return 返回 rcl_ret_t 类型的结果状态。
 */
RCL_LOCAL
rcl_ret_t rcl_remap_name(
    const rcl_arguments_t* local_arguments,
    const rcl_arguments_t* global_arguments,
    rcl_remap_type_t type_bitmask,
    const char* name,
    const char* node_name,
    const char* node_namespace,
    const rcutils_string_map_t* substitutions,
    rcl_allocator_t allocator,
    char** output_name);

#ifdef __cplusplus
}
#endif

#endif  // RCL__REMAP_IMPL_H_
