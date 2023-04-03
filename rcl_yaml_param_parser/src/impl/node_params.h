// Copyright 2018 Apex.AI, Inc.
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

#ifndef IMPL__NODE_PARAMS_H_
#define IMPL__NODE_PARAMS_H_

#include "rcl_yaml_param_parser/types.h"
#include "rcl_yaml_param_parser/visibility_control.h"
#include "rcutils/allocator.h"
#include "rcutils/macros.h"
#include "rcutils/types/rcutils_ret.h"

#ifdef __cplusplus
extern "C" {
#endif

/// 创建 rcl_node_params_t 结构体
///
/// \param[out] node_params 指向要初始化的 rcl_node_params_t 结构体的指针
/// \param[in] allocator 用于分配内存的 rcutils_allocator_t 结构体实例
/// \return rcutils_ret_t 返回操作结果，成功则返回 RCUTILS_RET_OK
RCL_YAML_PARAM_PARSER_PUBLIC
RCUTILS_WARN_UNUSED
rcutils_ret_t node_params_init(
  rcl_node_params_t * node_params, const rcutils_allocator_t allocator);

/// 创建具有容量的 rcl_node_params_t 结构体
///
/// \param[out] node_params 指向要初始化的 rcl_node_params_t 结构体的指针
/// \param[in] capacity 初始化时结构体的容量大小
/// \param[in] allocator 用于分配内存的 rcutils_allocator_t 结构体实例
/// \return rcutils_ret_t 返回操作结果，成功则返回 RCUTILS_RET_OK
RCL_YAML_PARAM_PARSER_PUBLIC
RCUTILS_WARN_UNUSED
rcutils_ret_t node_params_init_with_capacity(
  rcl_node_params_t * node_params, size_t capacity, const rcutils_allocator_t allocator);

/// 重新分配具有新容量的 rcl_node_params_t 结构体
/// \post 如果结果值为 `RCL_RET_BAD_ALLOC`，则 \p node_params 中的 \p parameter_names 的地址可能会更改。
///
/// \param[out] node_params 指向要重新分配的 rcl_node_params_t 结构体的指针
/// \param[in] new_capacity 新的容量大小
/// \param[in] allocator 用于分配内存的 rcutils_allocator_t 结构体实例
/// \return rcutils_ret_t 返回操作结果，成功则返回 RCUTILS_RET_OK
RCL_YAML_PARAM_PARSER_PUBLIC
RCUTILS_WARN_UNUSED
rcutils_ret_t node_params_reallocate(
  rcl_node_params_t * node_params, size_t new_capacity, const rcutils_allocator_t allocator);

/// 终止 rcl_node_params_t 结构体
///
/// \param[out] node_params 指向要终止的 rcl_node_params_t 结构体的指针
/// \param[in] allocator 用于释放内存的 rcutils_allocator_t 结构体实例
RCL_YAML_PARAM_PARSER_PUBLIC
void rcl_yaml_node_params_fini(
  rcl_node_params_t * node_params, const rcutils_allocator_t allocator);

#ifdef __cplusplus
}
#endif

#endif  // IMPL__NODE_PARAMS_H_
