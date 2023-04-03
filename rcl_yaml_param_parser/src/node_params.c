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

#include "./impl/node_params.h"

#include <string.h>

#include "./impl/types.h"
#include "./impl/yaml_variant.h"
#include "rcutils/allocator.h"
#include "rcutils/error_handling.h"
#include "rcutils/types/rcutils_ret.h"

#define INIT_NUM_PARAMS_PER_NODE 128U

/**
 * @brief 初始化节点参数
 *
 * @param[in] node_params 要初始化的节点参数结构体指针
 * @param[in] allocator 分配器，用于分配内存
 * @return rcutils_ret_t 返回操作结果
 */
rcutils_ret_t node_params_init(rcl_node_params_t * node_params, const rcutils_allocator_t allocator)
{
  // 使用默认容量初始化节点参数
  return node_params_init_with_capacity(node_params, INIT_NUM_PARAMS_PER_NODE, allocator);
}

/**
 * @brief 使用指定容量初始化节点参数
 *
 * @param[in] node_params 要初始化的节点参数结构体指针
 * @param[in] capacity 指定的容量大小
 * @param[in] allocator 分配器，用于分配内存
 * @return rcutils_ret_t 返回操作结果
 */
rcutils_ret_t node_params_init_with_capacity(
  rcl_node_params_t * node_params, size_t capacity, const rcutils_allocator_t allocator)
{
  // 检查输入参数是否为空
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(node_params, RCUTILS_RET_INVALID_ARGUMENT);

  // 检查分配器是否有效
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(
    &allocator, "invalid allocator", return RCUTILS_RET_INVALID_ARGUMENT);

  // 容量不能为零
  if (capacity == 0) {
    RCUTILS_SET_ERROR_MSG("capacity can't be zero");
    return RCUTILS_RET_INVALID_ARGUMENT;
  }

  // 为节点参数名称分配内存
  node_params->parameter_names = allocator.zero_allocate(capacity, sizeof(char *), allocator.state);
  if (NULL == node_params->parameter_names) {
    RCUTILS_SET_ERROR_MSG("Failed to allocate memory for node parameter names");
    return RCUTILS_RET_BAD_ALLOC;
  }

  // 为节点参数值分配内存
  node_params->parameter_values =
    allocator.zero_allocate(capacity, sizeof(rcl_variant_t), allocator.state);
  if (NULL == node_params->parameter_values) {
    // 如果分配失败，释放已分配的参数名称内存
    allocator.deallocate(node_params->parameter_names, allocator.state);
    node_params->parameter_names = NULL;
    RCUTILS_SET_ERROR_MSG("Failed to allocate memory for node parameter values");
    return RCUTILS_RET_BAD_ALLOC;
  }

  // 初始化节点参数数量和容量
  node_params->num_params = 0U;
  node_params->capacity_params = capacity;

  // 返回操作成功
  return RCUTILS_RET_OK;
}

/**
 * @brief 重新分配节点参数的内存空间
 *
 * @param[in,out] node_params 指向要重新分配内存的节点参数结构体的指针
 * @param[in] new_capacity 新的容量大小
 * @param[in] allocator 分配器，用于重新分配内存
 * @return rcutils_ret_t 返回操作结果
 */
rcutils_ret_t node_params_reallocate(
  rcl_node_params_t * node_params, size_t new_capacity, const rcutils_allocator_t allocator)
{
  // 检查node_params是否为空，如果为空则返回无效参数错误
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(node_params, RCUTILS_RET_INVALID_ARGUMENT);
  
  // 检查分配器是否有效，如果无效则返回无效参数错误
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(
    &allocator, "invalid allocator", return RCUTILS_RET_INVALID_ARGUMENT);

  // 如果新容量小于当前参数数量，则返回无效参数错误
  if (new_capacity < node_params->num_params) {
    RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "new capacity '%zu' must be greater than or equal to '%zu'", new_capacity,
      node_params->num_params);
    return RCUTILS_RET_INVALID_ARGUMENT;
  }

  // 重新分配参数名的内存空间
  void * parameter_names = allocator.reallocate(
    node_params->parameter_names, new_capacity * sizeof(char *), allocator.state);
  if (NULL == parameter_names) {
    RCUTILS_SET_ERROR_MSG("Failed to reallocate node parameter names");
    return RCUTILS_RET_BAD_ALLOC;
  }
  node_params->parameter_names = parameter_names;

  // 对新增加的内存进行零初始化
  if (new_capacity > node_params->capacity_params) {
    memset(
      node_params->parameter_names + node_params->capacity_params, 0,
      (new_capacity - node_params->capacity_params) * sizeof(char *));
  }

  // 重新分配参数值的内存空间
  void * parameter_values = allocator.reallocate(
    node_params->parameter_values, new_capacity * sizeof(rcl_variant_t), allocator.state);
  if (NULL == parameter_values) {
    RCUTILS_SET_ERROR_MSG("Failed to reallocate node parameter values");
    return RCUTILS_RET_BAD_ALLOC;
  }
  node_params->parameter_values = parameter_values;

  // 对新增加的内存进行零初始化
  if (new_capacity > node_params->capacity_params) {
    memset(
      &node_params->parameter_values[node_params->capacity_params], 0,
      (new_capacity - node_params->capacity_params) * sizeof(rcl_variant_t));
  }

  // 更新节点参数的容量
  node_params->capacity_params = new_capacity;
  
  // 返回操作成功
  return RCUTILS_RET_OK;
}

/**
 * @brief 释放节点参数资源
 *
 * @param[in,out] node_params_st 指向要释放的节点参数结构体的指针
 * @param[in] allocator 分配器，用于释放内存
 */
void rcl_yaml_node_params_fini(
  rcl_node_params_t * node_params_st, const rcutils_allocator_t allocator)
{
  // 如果节点参数结构体指针为空，则直接返回
  if (NULL == node_params_st) {
    return;
  }

  // 如果参数名数组不为空
  if (NULL != node_params_st->parameter_names) {
    // 遍历参数名数组
    for (size_t parameter_idx = 0U; parameter_idx < node_params_st->num_params; parameter_idx++) {
      // 获取当前参数名
      char * param_name = node_params_st->parameter_names[parameter_idx];
      // 如果参数名不为空，则使用分配器释放其内存
      if (NULL != param_name) {
        allocator.deallocate(param_name, allocator.state);
      }
    }
    // 释放参数名数组内存
    allocator.deallocate(node_params_st->parameter_names, allocator.state);
    // 将参数名数组指针置空
    node_params_st->parameter_names = NULL;
  }

  // 如果参数值数组不为空
  if (NULL != node_params_st->parameter_values) {
    // 遍历参数值数组
    for (size_t parameter_idx = 0U; parameter_idx < node_params_st->num_params; parameter_idx++) {
      // 释放当前参数值资源
      rcl_yaml_variant_fini(&(node_params_st->parameter_values[parameter_idx]), allocator);
    }

    // 释放参数值数组内存
    allocator.deallocate(node_params_st->parameter_values, allocator.state);
    // 将参数值数组指针置空
    node_params_st->parameter_values = NULL;
  }

  // 将节点参数数量置为0
  node_params_st->num_params = 0;
  // 将节点参数容量置为0
  node_params_st->capacity_params = 0;
}
