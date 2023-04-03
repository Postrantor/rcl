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

#ifndef IMPL__ADD_TO_ARRAYS_H_
#define IMPL__ADD_TO_ARRAYS_H_

#include "./types.h"
#include "rcl_yaml_param_parser/types.h"
#include "rcl_yaml_param_parser/visibility_control.h"
#include "rcutils/allocator.h"
#include "rcutils/macros.h"
#include "rcutils/types/rcutils_ret.h"
#include "rcutils/types/string_array.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief 向布尔数组中添加一个值。如果数组不存在，则创建它
///
/// \param[in,out] val_array 布尔数组指针
/// \param[in] value 要添加的布尔值指针
/// \param[in] allocator 分配器
/// \return rcutils_ret_t 返回操作结果
RCL_YAML_PARAM_PARSER_PUBLIC
RCUTILS_WARN_UNUSED
rcutils_ret_t add_val_to_bool_arr(
  rcl_bool_array_t * const val_array, bool * value, const rcutils_allocator_t allocator);

/// \brief 向整数数组中添加一个值。如果数组不存在，则创建它
///
/// \param[in,out] val_array 整数数组指针
/// \param[in] value 要添加的整数值指针
/// \param[in] allocator 分配器
/// \return rcutils_ret_t 返回操作结果
RCL_YAML_PARAM_PARSER_PUBLIC
RCUTILS_WARN_UNUSED
rcutils_ret_t add_val_to_int_arr(
  rcl_int64_array_t * const val_array, int64_t * value, const rcutils_allocator_t allocator);

/// \brief 向双精度浮点数数组中添加一个值。如果数组不存在，则创建它
///
/// \param[in,out] val_array 双精度浮点数数组指针
/// \param[in] value 要添加的双精度浮点数值指针
/// \param[in] allocator 分配器
/// \return rcutils_ret_t 返回操作结果
RCL_YAML_PARAM_PARSER_PUBLIC
RCUTILS_WARN_UNUSED
rcutils_ret_t add_val_to_double_arr(
  rcl_double_array_t * const val_array, double * value, const rcutils_allocator_t allocator);

/// \brief 向字符串数组中添加一个值。如果数组不存在，则创建它
///
/// \param[in,out] val_array 字符串数组指针
/// \param[in] value 要添加的字符串值指针
/// \param[in] allocator 分配器
/// \return rcutils_ret_t 返回操作结果
RCL_YAML_PARAM_PARSER_PUBLIC
RCUTILS_WARN_UNUSED
rcutils_ret_t add_val_to_string_arr(
  rcutils_string_array_t * const val_array, char * value, const rcutils_allocator_t allocator);

/// \todo (anup.pemmaiah): 支持字节数组
///

#ifdef __cplusplus
}
#endif

#endif  // IMPL__ADD_TO_ARRAYS_H_
