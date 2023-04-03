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

#ifndef IMPL__PARSE_H_
#define IMPL__PARSE_H_

#include <yaml.h>

#include "./types.h"
#include "rcl_yaml_param_parser/types.h"
#include "rcl_yaml_param_parser/visibility_control.h"
#include "rcutils/allocator.h"
#include "rcutils/macros.h"
#include "rcutils/types/rcutils_ret.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 获取值并将其转换为适当的数据类型
 * 
 * @param[in] value 输入值字符串
 * @param[in] style YAML标量样式
 * @param[in] tag YAML标签
 * @param[out] val_type 输出值的数据类型
 * @param[in] allocator 分配器用于分配内存
 * @return void* 转换后的值指针
 */
RCL_YAML_PARAM_PARSER_PUBLIC
RCUTILS_WARN_UNUSED
void * get_value(
  const char * const value, yaml_scalar_style_t style, const yaml_char_t * const tag,
  data_types_t * val_type, const rcutils_allocator_t allocator);

/**
 * @brief 解析YAML事件中的值
 * 
 * @param[in] event YAML事件
 * @param[in] is_seq 值是否属于序列
 * @param[in] node_idx 节点索引
 * @param[in] parameter_idx 参数索引
 * @param[out] seq_data_type 序列的数据类型
 * @param[out] params_st 参数结构体
 * @return rcutils_ret_t 返回操作结果
 */
RCL_YAML_PARAM_PARSER_PUBLIC
RCUTILS_WARN_UNUSED
rcutils_ret_t parse_value(
  const yaml_event_t event, const bool is_seq, const size_t node_idx, const size_t parameter_idx,
  data_types_t * seq_data_type, rcl_params_t * params_st);

/**
 * @brief 解析YAML事件中的键
 * 
 * @param[in] event YAML事件
 * @param[in,out] map_level 当前映射级别
 * @param[out] is_new_map 是否是新的映射
 * @param[out] node_idx 节点索引
 * @param[out] parameter_idx 参数索引
 * @param[in,out] ns_tracker 命名空间跟踪器
 * @param[out] params_st 参数结构体
 * @return rcutils_ret_t 返回操作结果
 */
RCL_YAML_PARAM_PARSER_PUBLIC
RCUTILS_WARN_UNUSED
rcutils_ret_t parse_key(
  const yaml_event_t event, uint32_t * map_level, bool * is_new_map, size_t * node_idx,
  size_t * parameter_idx, namespace_tracker_t * ns_tracker, rcl_params_t * params_st);

/**
 * @brief 解析YAML文件中的事件
 * 
 * @param[in] parser YAML解析器
 * @param[in,out] ns_tracker 命名空间跟踪器
 * @param[out] params_st 参数结构体
 * @return rcutils_ret_t 返回操作结果
 */
RCL_YAML_PARAM_PARSER_PUBLIC
RCUTILS_WARN_UNUSED
rcutils_ret_t parse_file_events(
  yaml_parser_t * parser, namespace_tracker_t * ns_tracker, rcl_params_t * params_st);

/**
 * @brief 解析YAML值事件
 * 
 * @param[in] parser YAML解析器
 * @param[in] node_idx 节点索引
 * @param[in] parameter_idx 参数索引
 * @param[out] params_st 参数结构体
 * @return rcutils_ret_t 返回操作结果
 */
RCL_YAML_PARAM_PARSER_PUBLIC
RCUTILS_WARN_UNUSED
rcutils_ret_t parse_value_events(
  yaml_parser_t * parser, const size_t node_idx, const size_t parameter_idx,
  rcl_params_t * params_st);

/**
 * @brief 查找节点
 * 
 * @param[in] node_name 要查找的节点名称
 * @param[in] param_st 参数结构体
 * @param[out] node_idx 找到的节点索引
 * @return rcutils_ret_t 返回操作结果
 */
RCL_YAML_PARAM_PARSER_PUBLIC
RCUTILS_WARN_UNUSED
rcutils_ret_t find_node(const char * node_name, rcl_params_t * param_st, size_t * node_idx);

/**
 * @brief 查找参数
 * 
 * @param[in] node_idx 节点索引
 * @param[in] parameter_name 要查找的参数名称
 * @param[in] param_st 参数结构体
 * @param[out] parameter_idx 找到的参数索引
 * @return rcutils_ret_t 返回操作结果
 */
RCL_YAML_PARAM_PARSER_PUBLIC
RCUTILS_WARN_UNUSED
rcutils_ret_t find_parameter(
  const size_t node_idx, const char * parameter_name, rcl_params_t * param_st,
  size_t * parameter_idx);

#ifdef __cplusplus
}
#endif

#endif  // IMPL__PARSE_H_
