// Copyright 2020 Open Source Robotics Foundation, Inc.
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

#include "./remap_impl.h"
#include "rcl/error_handling.h"
#include "rcl/expand_topic_name.h"
#include "rcl/node.h"
#include "rcl/remap.h"
#include "rcutils/error_handling.h"
#include "rcutils/logging_macros.h"
#include "rcutils/types/string_map.h"
#include "rmw/error_handling.h"
#include "rmw/validate_full_topic_name.h"

/**
 * @brief 解析并处理ROS2主题或服务名称，包括扩展和重映射。
 *
 * @param[in] local_args 本地参数，用于解析节点特定的命名规则。
 * @param[in] global_args 全局参数，用于解析全局命名规则。
 * @param[in] input_topic_name 输入的主题或服务名称。
 * @param[in] node_name 当前节点的名称。
 * @param[in] node_namespace 当前节点的命名空间。
 * @param[in] allocator 分配器，用于分配内存。
 * @param[in] is_service 是否为服务名称，如果为true，则处理服务名称；否则处理主题名称。
 * @param[in] only_expand 是否仅执行名称扩展，如果为true，则不进行重映射。
 * @param[out] output_topic_name 输出处理后的主题或服务名称。
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败。
 */
static rcl_ret_t rcl_resolve_name(
    const rcl_arguments_t* local_args,
    const rcl_arguments_t* global_args,
    const char* input_topic_name,
    const char* node_name,
    const char* node_namespace,
    rcl_allocator_t allocator,
    bool is_service,
    bool only_expand,
    char** output_topic_name) {
  // 检查output_topic_name参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(output_topic_name, RCL_RET_INVALID_ARGUMENT);

  // 创建默认主题名称替换映射
  rcutils_string_map_t substitutions_map = rcutils_get_zero_initialized_string_map();
  rcutils_ret_t rcutils_ret = rcutils_string_map_init(&substitutions_map, 0, allocator);
  if (rcutils_ret != RCUTILS_RET_OK) {
    rcutils_error_string_t error = rcutils_get_error_string();
    rcutils_reset_error();
    RCL_SET_ERROR_MSG(error.str);
    if (RCUTILS_RET_BAD_ALLOC == rcutils_ret) {
      return RCL_RET_BAD_ALLOC;
    }
    return RCL_RET_ERROR;
  }

  char* expanded_topic_name = NULL;
  char* remapped_topic_name = NULL;

  // 获取默认主题名称替换
  rcl_ret_t ret = rcl_get_default_topic_name_substitutions(&substitutions_map);
  if (ret != RCL_RET_OK) {
    if (RCL_RET_BAD_ALLOC != ret) {
      ret = RCL_RET_ERROR;
    }
    goto cleanup;
  }

  // 扩展主题名称
  ret = rcl_expand_topic_name(
      input_topic_name, node_name, node_namespace, &substitutions_map, allocator,
      &expanded_topic_name);
  if (RCL_RET_OK != ret) {
    goto cleanup;
  }

  // 重映射主题名称
  if (!only_expand) {
    ret = rcl_remap_name(
        local_args, global_args, is_service ? RCL_SERVICE_REMAP : RCL_TOPIC_REMAP,
        expanded_topic_name, node_name, node_namespace, &substitutions_map, allocator,
        &remapped_topic_name);
    if (RCL_RET_OK != ret) {
      goto cleanup;
    }
  }
  // 如果没有重映射主题名称，则使用扩展后的主题名称
  if (NULL == remapped_topic_name) {
    remapped_topic_name = expanded_topic_name;
    expanded_topic_name = NULL;
  }

  /**
   * @brief 验证并设置输出主题名称
   *
   * 该函数首先验证重新映射的主题名称，然后根据验证结果设置错误消息和返回值。
   * 如果主题名称有效，则将其分配给输出主题名称。
   *
   * @param[in] remapped_topic_name 重新映射的主题名称
   * @param[out] output_topic_name 输出主题名称的指针
   * @param[out] validation_result 验证结果的指针
   * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
   */
  int validation_result;
  // 调用rmw_validate_full_topic_name函数验证重新映射的主题名称
  rmw_ret_t rmw_ret = rmw_validate_full_topic_name(remapped_topic_name, &validation_result, NULL);
  // 判断验证函数是否执行成功
  if (rmw_ret != RMW_RET_OK) {
    // 获取错误信息
    const char* error = rmw_get_error_string().str;
    // 重置错误状态
    rmw_reset_error();
    // 设置错误消息
    RCL_SET_ERROR_MSG(error);
    // 设置返回值为错误
    ret = RCL_RET_ERROR;
    // 跳转到清理部分
    goto cleanup;
  }

  // 判断主题名称是否有效
  if (validation_result != RMW_TOPIC_VALID) {
    // 设置错误消息为无效主题名称
    RCL_SET_ERROR_MSG(rmw_full_topic_name_validation_result_string(validation_result));
    // 设置返回值为主题名称无效
    ret = RCL_RET_TOPIC_NAME_INVALID;
    // 跳转到清理部分
    goto cleanup;
  }

  // 将重新映射的主题名称分配给输出主题名称
  *output_topic_name = remapped_topic_name;
  // 将重新映射的主题名称设置为NULL，避免在清理部分被释放
  remapped_topic_name = NULL;

/*!
 * \brief 清理操作，释放分配的内存并处理错误。
 *
 * 该函数执行以下操作：
 * - 销毁 substitutions_map
 * - 处理 rcutils 错误
 * - 释放 expanded_topic_name 和 remapped_topic_name 的内存
 * - 如果是服务且返回值为 RCL_RET_TOPIC_NAME_INVALID，则将其更改为 RCL_RET_SERVICE_NAME_INVALID
 *
 * \param[out] ret 返回值，表示操作成功或失败的状态
 * \param[in] substitutions_map 存储替换规则的字符串映射
 * \param[in] expanded_topic_name 扩展后的主题名称
 * \param[in] remapped_topic_name 重映射后的主题名称
 * \param[in] is_service 布尔值，表示是否为服务
 * \param[in] allocator 分配器，用于分配和释放内存
 *
 * \return ret 返回操作结果
 */
cleanup:
  rcutils_ret = rcutils_string_map_fini(&substitutions_map);    // 销毁 substitutions_map
  if (rcutils_ret != RCUTILS_RET_OK) {                          // 检查 rcutils 错误
    rcutils_error_string_t error = rcutils_get_error_string();  // 获取错误字符串
    rcutils_reset_error();                                      // 重置错误
    if (RCL_RET_OK == ret) {
      RCL_SET_ERROR_MSG(error.str);                             // 设置错误消息
      ret = RCL_RET_ERROR;                                      // 更新返回值
    } else {
      RCUTILS_LOG_ERROR_NAMED(
          ROS_PACKAGE_NAME, "failed to fini string_map (%d) during error handling: %s", rcutils_ret,
          error.str);  // 记录错误日志
    }
  }

  allocator.deallocate(expanded_topic_name, allocator.state);  // 释放 expanded_topic_name 的内存
  allocator.deallocate(remapped_topic_name, allocator.state);  // 释放 remapped_topic_name 的内存

  if (is_service &&
      RCL_RET_TOPIC_NAME_INVALID == ret) {  // 检查是否为服务且返回值为 RCL_RET_TOPIC_NAME_INVALID
    ret = RCL_RET_SERVICE_NAME_INVALID;  // 更新返回值
  }

  return ret;  // 返回操作结果
}

/**
 * @brief 解析节点名称，将输入的主题名称解析为完整的名称。
 *
 * @param[in] node 指向rcl_node_t类型的指针，表示当前节点。
 * @param[in] input_topic_name 输入的主题名称，需要进行解析的名称。
 * @param[in] allocator 分配器，用于分配内存空间。
 * @param[in] is_service 布尔值，表示是否为服务名称。
 * @param[in] only_expand 布尔值，表示是否仅展开名称。
 * @param[out] output_topic_name 输出的主题名称，解析后的完整名称。
 * @return rcl_ret_t 返回RCL（ROS客户端库）状态码。
 */
rcl_ret_t rcl_node_resolve_name(
    const rcl_node_t* node,
    const char* input_topic_name,
    rcl_allocator_t allocator,
    bool is_service,
    bool only_expand,
    char** output_topic_name) {
  // 检查节点参数是否为空，如果为空则返回无效参数错误
  RCL_CHECK_ARGUMENT_FOR_NULL(node, RCL_RET_INVALID_ARGUMENT);

  // 获取节点选项
  const rcl_node_options_t* node_options = rcl_node_get_options(node);

  // 如果节点选项为空，则返回错误
  if (NULL == node_options) {
    return RCL_RET_ERROR;
  }

  // 初始化全局参数指针
  rcl_arguments_t* global_args = NULL;

  // 如果使用全局参数，则将全局参数指针指向节点的全局参数
  if (node_options->use_global_arguments) {
    global_args = &(node->context->global_arguments);
  }

  // 调用rcl_resolve_name函数解析名称，并返回结果
  return rcl_resolve_name(
      &(node_options->arguments), global_args, input_topic_name, rcl_node_get_name(node),
      rcl_node_get_namespace(node), allocator, is_service, only_expand, output_topic_name);
}
