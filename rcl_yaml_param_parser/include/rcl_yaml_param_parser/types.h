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
#ifndef RCL_YAML_PARAM_PARSER__TYPES_H_
#define RCL_YAML_PARAM_PARSER__TYPES_H_

#include "rcutils/allocator.h"
#include "rcutils/types/string_array.h"

/// 布尔值数组
/**
 * \typedef rcl_bool_array_t
 */
typedef struct rcl_bool_array_s
{
  /// 布尔值数组
  bool * values;
  /// 数组中的值的数量
  size_t size;
} rcl_bool_array_t;

/// int64_t值数组
/**
 * \typedef rcl_int64_array_t
 */
typedef struct rcl_int64_array_s
{
  /// int64值数组
  int64_t * values;
  /// 数组中的值的数量
  size_t size;
} rcl_int64_array_t;

/// 双精度浮点数值数组
/**
 * \typedef rcl_double_array_t
 */
typedef struct rcl_double_array_s
{
  /// 双精度浮点数值数组
  double * values;
  /// 数组中的值的数量
  size_t size;
} rcl_double_array_t;

/// 字节值数组
/**
 * \typedef rcl_byte_array_t
 */
typedef struct rcl_byte_array_s
{
  /// uint8_t值数组
  uint8_t * values;
  /// 数组中的值的数量
  size_t size;
} rcl_byte_array_t;

/// variant_t 存储参数的值
/**
 * 此结构体中只有一个指针存储值
 * \typedef rcl_variant_t
 */
typedef struct rcl_variant_s
{
  bool * bool_value;                        ///< 如果是布尔值，则存储在此处
  int64_t * integer_value;                  ///< 如果是整数，则存储在此处
  double * double_value;                    ///< 如果是双精度浮点数，则存储在此处
  char * string_value;                      ///< 如果是字符串，则存储在此处
  rcl_byte_array_t * byte_array_value;      ///< 如果是字节数组
  rcl_bool_array_t * bool_array_value;      ///< 如果是布尔值数组
  rcl_int64_array_t * integer_array_value;  ///< 如果是整数数组
  rcl_double_array_t * double_array_value;  ///< 如果是双精度浮点数数组
  rcutils_string_array_t * string_array_value;  ///< 如果是字符串数组
} rcl_variant_t;

/// node_params_t 存储单个节点的所有参数（键:值）
/**
* \typedef rcl_node_params_t
*/
typedef struct rcl_node_params_s
{
  char ** parameter_names;           ///< 参数名（键）数组
  rcl_variant_t * parameter_values;  ///< 相应参数值数组
  size_t num_params;                 ///< 节点中的参数数量
  size_t capacity_params;            ///< 节点中的参数容量
} rcl_node_params_t;

/// 存储进程中所有节点的所有参数
/**
* \typedef rcl_params_t
*/
typedef struct rcl_params_s
{
  char ** node_names;             ///< 节点名称列表
  rcl_node_params_t * params;     ///< 参数数组
  size_t num_nodes;               ///< 节点数量
  size_t capacity_nodes;          ///< 节点容量
  rcutils_allocator_t allocator;  ///< 使用的分配器
} rcl_params_t;

#endif  // RCL_YAML_PARAM_PARSER__TYPES_H_
