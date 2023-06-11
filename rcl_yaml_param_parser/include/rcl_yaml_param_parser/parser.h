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

/** rcl_yaml_param_parser: Parse a YAML parameter file and populate the C data structure
 *
 *  - Parser
 *  - rcl/parser.h
 *
 * Some useful abstractions and utilities:
 * - Return code types
 *   - rcl/types.h
 * - Macros for controlling symbol visibility on the library
 *   - rcl/visibility_control.h
 */

#ifndef RCL_YAML_PARAM_PARSER__PARSER_H_
#define RCL_YAML_PARAM_PARSER__PARSER_H_

#include <stdlib.h>

#include "rcl_yaml_param_parser/types.h"
#include "rcl_yaml_param_parser/visibility_control.h"
#include "rcutils/allocator.h"
#include "rcutils/types/rcutils_ret.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief 初始化参数结构
/// \param[in] allocator 要使用的内存分配器
/// \return 成功时返回指向参数结构的指针，失败时返回NULL
RCL_YAML_PARAM_PARSER_PUBLIC
rcl_params_t* rcl_yaml_node_struct_init(const rcutils_allocator_t allocator);

/// \brief 使用容量初始化参数结构
/// \param[in] capacity 参数结构的容量
/// \param[in] allocator 要使用的内存分配器
/// \return 成功时返回指向参数结构的指针，失败时返回NULL
RCL_YAML_PARAM_PARSER_PUBLIC
rcl_params_t* rcl_yaml_node_struct_init_with_capacity(
    size_t capacity, const rcutils_allocator_t allocator);

/// \brief 用新容量重新分配参数结构
/// \post 即使结果值为`RCL_RET_BAD_ALLOC`，\p params_st 中的 \p node_names 的地址也可能发生变化。
/// \param[in] params_st 参数结构
/// \param[in] new_capacity 必须大于 num_params 的参数结构的新容量
/// \param[in] allocator 要使用的内存分配器
/// \return 如果结构成功重新分配，则返回 `RCL_RET_OK`，或
/// \return 如果 params_st 为 NULL，或
///  分配器无效，或
///  new_capacity 小于 num_nodes
/// \return 则返回 `RCL_RET_INVALID_ARGUMENT`
/// \return 如果分配内存失败，则返回 `RCL_RET_BAD_ALLOC`。
RCL_YAML_PARAM_PARSER_PUBLIC
rcutils_ret_t rcl_yaml_node_struct_reallocate(
    rcl_params_t* params_st, size_t new_capacity, const rcutils_allocator_t allocator);

/// \brief 复制参数结构
/// \param[in] params_st 指向要复制的参数结构
/// \return 成功时返回指向复制的参数结构的指针，失败时返回NULL
RCL_YAML_PARAM_PARSER_PUBLIC
rcl_params_t* rcl_yaml_node_struct_copy(const rcl_params_t* params_st);

/// \brief 释放参数结构
/// \param[in] params_st 指向已填充的参数结构
RCL_YAML_PARAM_PARSER_PUBLIC
void rcl_yaml_node_struct_fini(rcl_params_t* params_st);

/// \brief 解析YAML文件并填充 \p params_st
/// \pre 给定的 \p params_st 必须是由 `rcl_yaml_node_struct_init()` 返回的有效参数结构
/// \param[in] file_path 是YAML文件的路径
/// \param[inout] params_st 指向要填充的结构
/// \return 成功时返回true，失败时返回false
RCL_YAML_PARAM_PARSER_PUBLIC
bool rcl_parse_yaml_file(const char* file_path, rcl_params_t* params_st);

/// \brief 解析作为YAML字符串的参数值，并相应地更新params_st
/// \param[in] node_name 是参数所属节点的名称
/// \param[in] param_name 是将解析其值的参数的名称
/// \param[in] yaml_value 是要解析的YAML字符串形式的参数值
/// \param[inout] params_st 指向参数结构
/// \return 成功时返回true，失败时返回false
RCL_YAML_PARAM_PARSER_PUBLIC
bool rcl_parse_yaml_value(
    const char* node_name, const char* param_name, const char* yaml_value, rcl_params_t* params_st);

/// \brief 获取给定参数的变体值，如果尚未存在，则在过程中将其初始化为零
/// \param[in] node_name 是参数所属节点的名称
/// \param[in] param_name 是要检索其值的参数的名称
/// \param[inout] params_st 指向已填充（或待填充）的参数结构
/// \return 成功时返回参数变体值，失败时返回NULL
RCL_YAML_PARAM_PARSER_PUBLIC
rcl_variant_t* rcl_yaml_node_struct_get(
    const char* node_name, const char* param_name, rcl_params_t* params_st);

/// \brief 将参数结构打印到stdout
/// \param[in] params_st 指向已填充的参数结构
RCL_YAML_PARAM_PARSER_PUBLIC
void rcl_yaml_node_struct_print(const rcl_params_t* const params_st);

#ifdef __cplusplus
}
#endif

#endif  // RCL_YAML_PARAM_PARSER__PARSER_H_
