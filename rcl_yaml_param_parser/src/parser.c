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

#include "rcl_yaml_param_parser/parser.h"

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#include "./impl/node_params.h"
#include "./impl/parse.h"
#include "./impl/types.h"
#include "./impl/yaml_variant.h"
#include "rcl_yaml_param_parser/types.h"
#include "rcutils/allocator.h"
#include "rcutils/error_handling.h"
#include "rcutils/strdup.h"
#include "rcutils/types/rcutils_ret.h"

#define INIT_NUM_NODE_ENTRIES 128U

/// \brief 创建 rcl_params_t 参数结构
///
/// \param[in] allocator 分配器，用于分配内存
/// \return 返回一个指向新创建的 rcl_params_t 结构的指针
rcl_params_t * rcl_yaml_node_struct_init(const rcutils_allocator_t allocator)
{
  // 使用默认容量初始化参数结构
  return rcl_yaml_node_struct_init_with_capacity(INIT_NUM_NODE_ENTRIES, allocator);
}

/// \brief 使用指定容量创建 rcl_params_t 参数结构
///
/// \param[in] capacity 初始节点容量
/// \param[in] allocator 分配器，用于分配内存
/// \return 返回一个指向新创建的 rcl_params_t 结构的指针
rcl_params_t * rcl_yaml_node_struct_init_with_capacity(
  size_t capacity, const rcutils_allocator_t allocator)
{
  // 检查分配器是否有效
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(&allocator, "invalid allocator", return NULL);

  // 容量不能为零
  if (capacity == 0) {
    RCUTILS_SET_ERROR_MSG("capacity can't be zero");
    return NULL;
  }

  // 为参数结构分配内存
  rcl_params_t * params_st = allocator.zero_allocate(1, sizeof(rcl_params_t), allocator.state);
  if (NULL == params_st) {
    RCUTILS_SET_ERROR_MSG("Failed to allocate memory for parameters");
    return NULL;
  }

  // 设置分配器
  params_st->allocator = allocator;

  // 为节点名称分配内存
  params_st->node_names = allocator.zero_allocate(capacity, sizeof(char *), allocator.state);
  if (NULL == params_st->node_names) {
    RCUTILS_SET_ERROR_MSG("Failed to allocate memory for parameter node names");
    goto clean;
  }

  // 为参数值分配内存
  params_st->params = allocator.zero_allocate(capacity, sizeof(rcl_node_params_t), allocator.state);
  if (NULL == params_st->params) {
    // 如果分配失败，释放之前分配的节点名称内存
    allocator.deallocate(params_st->node_names, allocator.state);
    params_st->node_names = NULL;
    RCUTILS_SET_ERROR_MSG("Failed to allocate memory for parameter values");
    goto clean;
  }

  // 初始化节点数量和容量
  params_st->num_nodes = 0U;
  params_st->capacity_nodes = capacity;

  // 返回新创建的参数结构指针
  return params_st;

clean:
  // 清理并释放分配的内存
  allocator.deallocate(params_st, allocator.state);
  return NULL;
}

/**
 * @brief 重新分配 rcl_params_t 结构体中的节点参数内存空间
 *
 * @param[in,out] params_st 指向 rcl_params_t 结构体的指针，用于存储节点参数
 * @param[in] new_capacity 新的容量大小，必须大于或等于当前节点数量
 * @param[in] allocator 分配器，用于重新分配内存
 * @return rcutils_ret_t 返回操作结果，成功返回 RCUTILS_RET_OK
 */
rcutils_ret_t rcl_yaml_node_struct_reallocate(
  rcl_params_t * params_st, size_t new_capacity, const rcutils_allocator_t allocator)
{
  // 检查参数是否为空
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(params_st, RCUTILS_RET_INVALID_ARGUMENT);
  // 检查分配器是否有效
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(
    &allocator, "invalid allocator", return RCUTILS_RET_INVALID_ARGUMENT);
  // 如果新容量小于节点数量，则为无效参数
  if (new_capacity < params_st->num_nodes) {
    RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "new capacity '%zu' must be greater than or equal to '%zu'", new_capacity,
      params_st->num_nodes);
    return RCUTILS_RET_INVALID_ARGUMENT;
  }

  // 重新分配节点名称内存空间
  void * node_names =
    allocator.reallocate(params_st->node_names, new_capacity * sizeof(char *), allocator.state);
  // 如果分配失败，返回错误信息
  if (NULL == node_names) {
    RCUTILS_SET_ERROR_MSG("Failed to reallocate memory for parameter node names");
    return RCUTILS_RET_BAD_ALLOC;
  }
  params_st->node_names = node_names;
  // 对新增加的内存进行零初始化
  if (new_capacity > params_st->capacity_nodes) {
    memset(
      params_st->node_names + params_st->capacity_nodes, 0,
      (new_capacity - params_st->capacity_nodes) * sizeof(char *));
  }

  // 重新分配参数值内存空间
  void * params = allocator.reallocate(
    params_st->params, new_capacity * sizeof(rcl_node_params_t), allocator.state);
  // 如果分配失败，返回错误信息
  if (NULL == params) {
    RCUTILS_SET_ERROR_MSG("Failed to reallocate memory for parameter values");
    return RCUTILS_RET_BAD_ALLOC;
  }
  params_st->params = params;
  // 对新增加的内存进行零初始化
  if (new_capacity > params_st->capacity_nodes) {
    memset(
      &params_st->params[params_st->capacity_nodes], 0,
      (new_capacity - params_st->capacity_nodes) * sizeof(rcl_node_params_t));
  }

  // 更新节点容量
  params_st->capacity_nodes = new_capacity;
  // 返回操作成功
  return RCUTILS_RET_OK;
}

/**
 * @brief 复制 rcl_params_t 参数结构
 *
 * @param[in] params_st 指向要复制的 rcl_params_t 结构的指针
 * @return 返回一个新分配的 rcl_params_t 结构的指针，如果失败则返回 NULL
 */
rcl_params_t * rcl_yaml_node_struct_copy(const rcl_params_t * params_st)
{
  // 检查输入参数是否为空，如果为空则返回 NULL
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(params_st, NULL);

  // 获取分配器
  rcutils_allocator_t allocator = params_st->allocator;
  // 使用给定的容量和分配器初始化一个新的 rcl_params_t 结构
  rcl_params_t * out_params_st =
    rcl_yaml_node_struct_init_with_capacity(params_st->capacity_nodes, allocator);

  // 检查新分配的结构是否为空，如果为空则输出错误信息并返回 NULL
  if (NULL == out_params_st) {
    RCUTILS_SAFE_FWRITE_TO_STDERR("Error allocating mem\n");
    return NULL;
  }

  rcutils_ret_t ret;
  // 遍历输入参数结构中的节点
  for (size_t node_idx = 0U; node_idx < params_st->num_nodes; ++node_idx) {
    // 复制节点名称
    out_params_st->node_names[node_idx] =
      rcutils_strdup(params_st->node_names[node_idx], allocator);
    // 检查复制后的节点名称是否为空，如果为空则输出错误信息并跳转到 fail 标签
    if (NULL == out_params_st->node_names[node_idx]) {
      RCUTILS_SAFE_FWRITE_TO_STDERR("Error allocating mem\n");
      goto fail;
    }
    // 增加新结构中的节点数量
    out_params_st->num_nodes++;

    // 获取输入参数结构中的节点参数
    rcl_node_params_t * node_params_st = &(params_st->params[node_idx]);
    // 获取新结构中的节点参数
    rcl_node_params_t * out_node_params_st = &(out_params_st->params[node_idx]);
    // 使用给定的容量和分配器初始化新结构中的节点参数
    ret = node_params_init_with_capacity(
      out_node_params_st, node_params_st->capacity_params, allocator);
    // 检查返回值，如果不是 RCUTILS_RET_OK，则输出错误信息并跳转到 fail 标签
    if (RCUTILS_RET_OK != ret) {
      if (RCUTILS_RET_BAD_ALLOC == ret) {
        RCUTILS_SAFE_FWRITE_TO_STDERR("Error allocating mem\n");
      }
      goto fail;
    }
    // 遍历输入参数结构中的节点参数
    for (size_t parameter_idx = 0U; parameter_idx < node_params_st->num_params; ++parameter_idx) {
      // 复制参数名称
      out_node_params_st->parameter_names[parameter_idx] =
        rcutils_strdup(node_params_st->parameter_names[parameter_idx], allocator);
      // 检查复制后的参数名称是否为空，如果为空则输出错误信息并跳转到 fail 标签
      if (NULL == out_node_params_st->parameter_names[parameter_idx]) {
        RCUTILS_SAFE_FWRITE_TO_STDERR("Error allocating mem\n");
        goto fail;
      }
      // 增加新结构中的参数数量
      out_node_params_st->num_params++;

      // 获取输入参数结构中的参数值
      const rcl_variant_t * param_var = &(node_params_st->parameter_values[parameter_idx]);
      // 获取新结构中的参数值
      rcl_variant_t * out_param_var = &(out_node_params_st->parameter_values[parameter_idx]);
      // 复制参数值，如果失败则跳转到 fail 标签
      if (!rcl_yaml_variant_copy(out_param_var, param_var, allocator)) {
        goto fail;
      }
    }
  }
  // 返回新分配的 rcl_params_t 结构的指针
  return out_params_st;

fail:
  // 如果发生错误，释放新分配的结构并返回 NULL
  rcl_yaml_node_struct_fini(out_params_st);
  return NULL;
}

/**
 * @brief 释放参数结构
 * 
 * 注意：如果出现错误，建议直接安全退出进程，而不是调用此释放函数并继续执行。
 *
 * @param[in,out] params_st 指向要释放的 rcl_params_t 结构体的指针
 */
void rcl_yaml_node_struct_fini(rcl_params_t * params_st)
{
  // 如果参数结构为空，则直接返回
  if (NULL == params_st) {
    return;
  }
  // 获取分配器
  rcutils_allocator_t allocator = params_st->allocator;

  // 如果节点名称数组不为空
  if (NULL != params_st->node_names) {
    // 遍历节点名称数组
    for (size_t node_idx = 0U; node_idx < params_st->num_nodes; node_idx++) {
      char * node_name = params_st->node_names[node_idx];
      // 如果节点名称不为空，则释放节点名称内存
      if (NULL != node_name) {
        allocator.deallocate(node_name, allocator.state);
      }
    }

    // 释放节点名称数组内存
    allocator.deallocate(params_st->node_names, allocator.state);
    params_st->node_names = NULL;
  }

  // 如果参数数组不为空
  if (NULL != params_st->params) {
    // 遍历参数数组
    for (size_t node_idx = 0U; node_idx < params_st->num_nodes; node_idx++) {
      rcl_yaml_node_params_fini(&(params_st->params[node_idx]), allocator);
    }  // for (node_idx)

    // 释放参数数组内存
    allocator.deallocate(params_st->params, allocator.state);
    params_st->params = NULL;
  }  // if (params)

  // 将节点数量和节点容量重置为0
  params_st->num_nodes = 0U;
  params_st->capacity_nodes = 0U;
  // 释放参数结构内存
  allocator.deallocate(params_st, allocator.state);
}

/**
 * @brief 解析YAML文件并填充params_st结构体
 *
 * @param[in] file_path YAML文件路径
 * @param[out] params_st 用于存储解析结果的rcl_params_t结构体指针
 * @return 成功返回true，失败返回false
 */
bool rcl_parse_yaml_file(const char * file_path, rcl_params_t * params_st)
{
  // 检查file_path是否为空，如果为空则返回错误信息并返回false
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(file_path, "YAML file path is NULL", return false);

  // 检查params_st是否为空，如果为空则输出错误信息并返回false
  if (NULL == params_st) {
    RCUTILS_SAFE_FWRITE_TO_STDERR("Pass an initialized parameter structure");
    return false;
  }

  // 初始化YAML解析器
  yaml_parser_t parser;
  int success = yaml_parser_initialize(&parser);
  // 如果解析器初始化失败，则设置错误信息并返回false
  if (0 == success) {
    RCUTILS_SET_ERROR_MSG("Could not initialize the parser");
    return false;
  }

  // 打开YAML文件
  FILE * yaml_file = fopen(file_path, "r");
  // 如果打开文件失败，则删除解析器，设置错误信息并返回false
  if (NULL == yaml_file) {
    yaml_parser_delete(&parser);
    RCUTILS_SET_ERROR_MSG("Error opening YAML file");
    return false;
  }

  // 设置解析器输入为yaml_file
  yaml_parser_set_input_file(&parser, yaml_file);

  // 初始化命名空间跟踪器
  namespace_tracker_t ns_tracker;
  memset(&ns_tracker, 0, sizeof(namespace_tracker_t));
  // 解析文件事件
  rcutils_ret_t ret = parse_file_events(&parser, &ns_tracker, params_st);

  // 关闭YAML文件
  fclose(yaml_file);

  // 删除解析器
  yaml_parser_delete(&parser);

  // 获取分配器
  rcutils_allocator_t allocator = params_st->allocator;
  // 如果命名空间跟踪器的node_ns不为空，则释放内存
  if (NULL != ns_tracker.node_ns) {
    allocator.deallocate(ns_tracker.node_ns, allocator.state);
  }
  // 如果命名空间跟踪器的parameter_ns不为空，则释放内存
  if (NULL != ns_tracker.parameter_ns) {
    allocator.deallocate(ns_tracker.parameter_ns, allocator.state);
  }

  // 返回解析结果
  return RCUTILS_RET_OK == ret;
}

/// \brief 解析YAML字符串并填充params_st
///
/// \param[in] node_name 节点名称
/// \param[in] param_name 参数名称
/// \param[in] yaml_value YAML值的字符串表示形式
/// \param[out] params_st 用于存储解析结果的rcl_params_t结构指针
/// \return 成功解析并填充参数时返回true，否则返回false
bool rcl_parse_yaml_value(
  const char * node_name, const char * param_name, const char * yaml_value,
  rcl_params_t * params_st)
{
  // 检查输入参数是否为空
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(node_name, false);
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(param_name, false);
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(yaml_value, false);

  // 检查输入字符串长度是否为0
  if (0U == strlen(node_name) || 0U == strlen(param_name) || 0U == strlen(yaml_value)) {
    return false;
  }

  // 检查params_st是否为空
  if (NULL == params_st) {
    RCUTILS_SAFE_FWRITE_TO_STDERR("Pass an initialized parameter structure");
    return false;
  }

  // 查找节点在params_st中的索引
  size_t node_idx = 0U;
  rcutils_ret_t ret = find_node(node_name, params_st, &node_idx);
  if (RCUTILS_RET_OK != ret) {
    return false;
  }

  // 查找参数在节点中的索引
  size_t parameter_idx = 0U;
  ret = find_parameter(node_idx, param_name, params_st, &parameter_idx);
  if (RCUTILS_RET_OK != ret) {
    return false;
  }

  // 初始化YAML解析器
  yaml_parser_t parser;
  int success = yaml_parser_initialize(&parser);
  if (0 == success) {
    RCUTILS_SET_ERROR_MSG("Could not initialize the parser");
    return false;
  }

  // 设置YAML解析器的输入字符串
  yaml_parser_set_input_string(&parser, (const unsigned char *)yaml_value, strlen(yaml_value));

  // 解析YAML值事件
  ret = parse_value_events(&parser, node_idx, parameter_idx, params_st);

  // 删除YAML解析器
  yaml_parser_delete(&parser);

  // 返回解析结果
  return RCUTILS_RET_OK == ret;
}

/// \brief 获取指定节点和参数名称的rcl_variant_t结构
///
/// \param[in] node_name 节点名称
/// \param[in] param_name 参数名称
/// \param[in] params_st 存储参数信息的rcl_params_t结构指针
/// \return 成功时返回指向rcl_variant_t结构的指针，否则返回NULL
rcl_variant_t * rcl_yaml_node_struct_get(
  const char * node_name, const char * param_name, rcl_params_t * params_st)
{
  // 检查输入参数是否为空
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(node_name, NULL);
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(param_name, NULL);
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(params_st, NULL);

  // 初始化参数值指针
  rcl_variant_t * param_value = NULL;

  // 查找节点在params_st中的索引
  size_t node_idx = 0U;
  rcutils_ret_t ret = find_node(node_name, params_st, &node_idx);
  if (RCUTILS_RET_OK == ret) {
    // 查找参数在节点中的索引
    size_t parameter_idx = 0U;
    ret = find_parameter(node_idx, param_name, params_st, &parameter_idx);
    if (RCUTILS_RET_OK == ret) {
      // 获取参数值指针
      param_value = &(params_st->params[node_idx].parameter_values[parameter_idx]);
    }
  }
  // 返回参数值指针
  return param_value;
}

/// \brief 打印参数结构体
///
/// 该函数用于打印 rcl_params_t 结构体中的内容，包括节点名称和参数。
///
/// \param[in] params_st 指向 rcl_params_t 结构体的指针
void rcl_yaml_node_struct_print(const rcl_params_t * const params_st)
{
  // 如果参数结构体为空，则直接返回
  if (NULL == params_st) {
    return;
  }

  // 打印表头
  printf("\n Node Name\t\t\t\tParameters\n");

  // 遍历所有节点
  for (size_t node_idx = 0U; node_idx < params_st->num_nodes; node_idx++) {
    int32_t param_col = 50;
    const char * const node_name = params_st->node_names[node_idx];

    // 如果节点名称不为空，则打印节点名称
    if (NULL != node_name) {
      printf("%s\n", node_name);
    }

    // 如果参数数组不为空，则遍历并打印参数
    if (NULL != params_st->params) {
      rcl_node_params_t * node_params_st = &(params_st->params[node_idx]);
      for (size_t parameter_idx = 0U; parameter_idx < node_params_st->num_params; parameter_idx++) {
        if (
          (NULL != node_params_st->parameter_names) && (NULL != node_params_st->parameter_values)) {
          char * param_name = node_params_st->parameter_names[parameter_idx];
          rcl_variant_t * param_var = &(node_params_st->parameter_values[parameter_idx]);

          // 如果参数名称不为空，则打印参数名称
          if (NULL != param_name) {
            printf("%*s", param_col, param_name);
          }

          // 如果参数值不为空，则根据参数类型打印参数值
          /**
 * @brief 打印参数变量的值
 *
 * 根据参数变量的类型，打印其对应的值。支持布尔值、整数值、浮点数值、字符串值以及它们的数组形式。
 *
 * @param[in] param_var 参数变量指针，包含了参数的类型和值信息
 */
          void print_param_value(const rcl_params_t * param_var)
          {
            if (NULL != param_var) {                // 检查参数变量指针是否为空
              if (NULL != param_var->bool_value) {  // 如果是布尔值类型
                printf(": %s\n", *(param_var->bool_value) ? "true" : "false");  // 打印布尔值
              } else if (NULL != param_var->integer_value) {             // 如果是整数值类型
                printf(": %" PRId64 "\n", *(param_var->integer_value));  // 打印整数值
              } else if (NULL != param_var->double_value) {      // 如果是浮点数值类型
                printf(": %lf\n", *(param_var->double_value));   // 打印浮点数值
              } else if (NULL != param_var->string_value) {      // 如果是字符串值类型
                printf(": %s\n", param_var->string_value);       // 打印字符串值
              } else if (NULL != param_var->bool_array_value) {  // 如果是布尔值数组类型
                printf(": ");
                for (size_t i = 0; i < param_var->bool_array_value->size; i++) {  // 遍历布尔值数组
                  if (param_var->bool_array_value->values) {
                    printf(
                      "%s, ", (param_var->bool_array_value->values[i])
                                ? "true"
                                : "false");  // 打印布尔值数组元素
                  }
                }
                printf("\n");
              } else if (NULL != param_var->integer_array_value) {  // 如果是整数值数组类型
                printf(": ");
                for (size_t i = 0; i < param_var->integer_array_value->size;
                     i++) {  // 遍历整数值数组
                  if (param_var->integer_array_value->values) {
                    printf(
                      "%" PRId64 ", ",
                      param_var->integer_array_value->values[i]);  // 打印整数值数组元素
                  }
                }
                printf("\n");
              } else if (NULL != param_var->double_array_value) {  // 如果是浮点数值数组类型
                printf(": ");
                for (size_t i = 0; i < param_var->double_array_value->size;
                     i++) {  // 遍历浮点数值数组
                  if (param_var->double_array_value->values) {
                    printf(
                      "%lf, ", param_var->double_array_value->values[i]);  // 打印浮点数值数组元素
                  }
                }
                printf("\n");
              } else if (NULL != param_var->string_array_value) {  // 如果是字符串值数组类型
                printf(": ");
                for (size_t i = 0; i < param_var->string_array_value->size;
                     i++) {  // 遍历字符串值数组
                  if (param_var->string_array_value->data[i]) {
                    printf("%s, ", param_var->string_array_value->data[i]);  // 打印字符串值数组元素
                  }
                }
                printf("\n");
              }
            }
          }
        }
      }
    }
  }
}
