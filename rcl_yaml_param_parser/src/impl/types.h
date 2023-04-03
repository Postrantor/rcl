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

/// NOTE: Will allow a max YAML mapping depth of 5
/// map level 1 : Node name mapping
/// map level 2 : Params mapping

#ifndef IMPL__TYPES_H_
#define IMPL__TYPES_H_

#include <inttypes.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PARAMS_KEY "ros__parameters"
#define NODE_NS_SEPERATOR "/"
#define PARAMETER_NS_SEPERATOR "."

/**
 * @brief YAML映射层级枚举类型
 */
typedef enum yaml_map_lvl_e {
  MAP_UNINIT_LVL = 0U,     ///< 未初始化的映射层级
  MAP_NODE_NAME_LVL = 1U,  ///< 节点名称映射层级
  MAP_PARAMS_LVL = 2U,     ///< 参数映射层级
} yaml_map_lvl_t;

/**
 * @brief YAML文件中基本支持的数据类型枚举类型
 */
typedef enum data_types_e {
  DATA_TYPE_UNKNOWN = 0U,  ///< 未知数据类型
  DATA_TYPE_BOOL = 1U,     ///< 布尔数据类型
  DATA_TYPE_INT64 = 2U,    ///< 64位整数数据类型
  DATA_TYPE_DOUBLE = 3U,   ///< 双精度浮点数数据类型
  DATA_TYPE_STRING = 4U    ///< 字符串数据类型
} data_types_t;

/**
 * @brief 命名空间类型枚举类型
 */
typedef enum namespace_type_e {
  NS_TYPE_NODE = 1U,  ///< 节点命名空间类型
  NS_TYPE_PARAM = 2U  ///< 参数命名空间类型
} namespace_type_t;

/**
 * @brief 命名空间跟踪器结构体，用于跟踪节点和参数命名空间
 */
typedef struct namespace_tracker_s
{
  char * node_ns;             ///< 节点命名空间
  uint32_t num_node_ns;       ///< 节点命名空间数量
  char * parameter_ns;        ///< 参数命名空间
  uint32_t num_parameter_ns;  ///< 参数命名空间数量
} namespace_tracker_t;

#ifdef __cplusplus
}
#endif

#endif  // IMPL__TYPES_H_
