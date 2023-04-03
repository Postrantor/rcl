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

#include "rcl/remap.h"

#include "./arguments_impl.h"
#include "./remap_impl.h"
#include "rcl/error_handling.h"
#include "rcl/expand_topic_name.h"
#include "rcutils/allocator.h"
#include "rcutils/macros.h"
#include "rcutils/strdup.h"
#include "rcutils/types/string_map.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 获取一个零初始化的 rcl_remap_t 结构体
 *
 * @return 返回一个零初始化的 rcl_remap_t 结构体
 */
rcl_remap_t rcl_get_zero_initialized_remap(void) {
  // 定义一个静态的默认规则，其实现指针为 NULL
  static rcl_remap_t default_rule = {.impl = NULL};
  // 返回默认规则
  return default_rule;
}

/**
 * @brief 复制一个 rcl_remap_t 结构体
 *
 * @param[in] rule 指向要复制的 rcl_remap_t 结构体的指针
 * @param[out] rule_out 指向存储复制结果的 rcl_remap_t 结构体的指针
 * @return 返回 RCL_RET_OK 表示成功，其他值表示失败
 */
rcl_ret_t rcl_remap_copy(const rcl_remap_t* rule, rcl_remap_t* rule_out) {
  // 检查输入参数是否有效
  RCL_CHECK_ARGUMENT_FOR_NULL(rule, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(rule_out, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(rule->impl, RCL_RET_INVALID_ARGUMENT);

  // 如果 rule_out 的实现指针不为空，则返回错误
  if (NULL != rule_out->impl) {
    RCL_SET_ERROR_MSG("rule_out must be zero initialized");
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 获取分配器
  rcl_allocator_t allocator = rule->impl->allocator;

  // 为 rule_out 分配内存
  rule_out->impl = allocator.allocate(sizeof(rcl_remap_impl_t), allocator.state);
  if (NULL == rule_out->impl) {
    return RCL_RET_BAD_ALLOC;
  }

  // 设置分配器
  rule_out->impl->allocator = allocator;

  // 初始化 rule_out 的实现结构体，以便在复制过程中出错时可以安全地调用 rcl_remap_fini()
  rule_out->impl->type = RCL_UNKNOWN_REMAP;
  rule_out->impl->node_name = NULL;
  rule_out->impl->match = NULL;
  rule_out->impl->replacement = NULL;

  // 复制类型
  rule_out->impl->type = rule->impl->type;
  // 复制节点名称
  if (NULL != rule->impl->node_name) {
    rule_out->impl->node_name = rcutils_strdup(rule->impl->node_name, allocator);
    if (NULL == rule_out->impl->node_name) {
      goto fail;
    }
  }
  // 复制匹配字符串
  if (NULL != rule->impl->match) {
    rule_out->impl->match = rcutils_strdup(rule->impl->match, allocator);
    if (NULL == rule_out->impl->match) {
      goto fail;
    }
  }
  // 复制替换字符串
  if (NULL != rule->impl->replacement) {
    rule_out->impl->replacement = rcutils_strdup(rule->impl->replacement, allocator);
    if (NULL == rule_out->impl->replacement) {
      goto fail;
    }
  }
  // 返回成功
  return RCL_RET_OK;

fail:
  // 如果在复制过程中出错，则调用 rcl_remap_fini() 清理资源
  if (RCL_RET_OK != rcl_remap_fini(rule_out)) {
    RCL_SET_ERROR_MSG("Error while finalizing remap rule due to another error");
  }
  // 返回分配错误
  return RCL_RET_BAD_ALLOC;
}

/// \brief 获取链中第一个匹配的规则。
/// \details 在搜索规则时，如果没有发生错误，则返回 RCL_RET_OK。
///
/// \param[in] remap_rules 重映射规则数组。
/// \param[in] num_rules 规则数组中的规则数量。
/// \param[in] type_bitmask 要查找的重映射类型的位掩码。
/// \param[in] name 要查找的名称。
/// \param[in] node_name 节点名称。
/// \param[in] node_namespace 节点命名空间。
/// \param[in] substitutions 字符串替换映射。
/// \param[in] allocator 分配器。
/// \param[out] output_rule 输出找到的第一个匹配规则的指针。
/// \return 如果在搜索规则时没有发生错误，则返回 RCL_RET_OK。
static rcl_ret_t rcl_remap_first_match(
    rcl_remap_t* remap_rules,
    int num_rules,
    rcl_remap_type_t type_bitmask,
    const char* name,
    const char* node_name,
    const char* node_namespace,
    const rcutils_string_map_t* substitutions,
    rcutils_allocator_t allocator,
    rcl_remap_t** output_rule) {
  *output_rule = NULL;                      // 初始化输出规则为空
  for (int i = 0; i < num_rules; ++i) {     // 遍历规则数组
    rcl_remap_t* rule = &(remap_rules[i]);  // 获取当前规则
    if (!(rule->impl->type & type_bitmask)) {
      // 如果当前规则的类型与要查找的类型不匹配，则跳过此规则
      continue;
    }
    if (rule->impl->node_name != NULL && 0 != strcmp(rule->impl->node_name, node_name)) {
      // 如果规则具有节点名称前缀且提供的节点名称与之不匹配，则跳过此规则
      continue;
    }
    bool matched = false;  // 初始化匹配标志为 false
    if (rule->impl->type & (RCL_TOPIC_REMAP | RCL_SERVICE_REMAP)) {
      // 如果规则类型是主题或服务重映射，需要将匹配侧扩展为完全限定名（FQN）
      char* expanded_match = NULL;
      rcl_ret_t ret = rcl_expand_topic_name(
          rule->impl->match, node_name, node_namespace, substitutions, allocator, &expanded_match);
      if (RCL_RET_OK != ret) {
        rcl_reset_error();
        if (RCL_RET_NODE_INVALID_NAMESPACE == ret || RCL_RET_NODE_INVALID_NAME == ret ||
            RCL_RET_BAD_ALLOC == ret) {
          // 这些错误可能会再次发生。停止处理规则
          return ret;
        }
        continue;
      }
      if (NULL != name) {
        // 此检查是为了满足 clang-tidy - 当 type_bitmask 是 RCL_TOPIC_REMAP 或 RCL_SERVICE_REMAP
        // 时， name 总是非空。这是因为 rcl_remap_first_match 和 rcl_remap_name 不是公共的。
        matched = (0 == strcmp(expanded_match, name));
      }
      allocator.deallocate(expanded_match, allocator.state);  // 释放 expanded_match
    } else {
      // 如果类型和节点名称前缀检查通过，则应用节点名和命名空间替换
      matched = true;
    }
    if (matched) {          // 如果匹配成功
      *output_rule = rule;  // 将找到的规则设置为输出规则
      break;                // 跳出循环
    }
  }
  return RCL_RET_OK;  // 返回 RCL_RET_OK，表示在搜索规则时没有发生错误
}

/// 使用匹配给定类型掩码的规则将一个名称映射到另一个名称。
/// \param[in] local_arguments 本地参数，用于查找本地重映射规则
/// \param[in] global_arguments 全局参数，用于查找全局重映射规则
/// \param[in] type_bitmask 类型掩码，用于筛选符合条件的重映射规则
/// \param[in] name 要重映射的原始名称
/// \param[in] node_name 当前节点的名称
/// \param[in] node_namespace 当前节点的命名空间
/// \param[in] substitutions 替换字符串映射
/// \param[in] allocator 分配器，用于分配内存
/// \param[out] output_name 输出的重映射后的名称
/// \return 返回 RCL_RET_OK 表示成功，其他值表示失败
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
    char** output_name) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(node_name, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(output_name, RCL_RET_INVALID_ARGUMENT);

  // 如果 local_arguments 或 global_arguments 的 impl 为 NULL，则将其设置为 NULL
  if (NULL != local_arguments && NULL == local_arguments->impl) {
    local_arguments = NULL;
  }
  if (NULL != global_arguments && NULL == global_arguments->impl) {
    global_arguments = NULL;
  }

  // 如果 local_arguments 和 global_arguments 都为 NULL，则返回错误
  if (NULL == local_arguments && NULL == global_arguments) {
    RCL_SET_ERROR_MSG("local_arguments invalid and not using global arguments");
    return RCL_RET_INVALID_ARGUMENT;
  }

  *output_name = NULL;
  rcl_remap_t* rule = NULL;

  // 首先查看本地规则
  if (NULL != local_arguments) {
    rcl_ret_t ret = rcl_remap_first_match(
        local_arguments->impl->remap_rules, local_arguments->impl->num_remap_rules, type_bitmask,
        name, node_name, node_namespace, substitutions, allocator, &rule);
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  // 如果没有本地规则匹配，检查全局规则
  if (NULL == rule && NULL != global_arguments) {
    rcl_ret_t ret = rcl_remap_first_match(
        global_arguments->impl->remap_rules, global_arguments->impl->num_remap_rules, type_bitmask,
        name, node_name, node_namespace, substitutions, allocator, &rule);
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  // 执行重映射
  if (NULL != rule) {
    if (rule->impl->type & (RCL_TOPIC_REMAP | RCL_SERVICE_REMAP)) {
      // 主题和服务规则需要将替换项扩展为完全限定名（FQN）
      rcl_ret_t ret = rcl_expand_topic_name(
          rule->impl->replacement, node_name, node_namespace, substitutions, allocator,
          output_name);
      if (RCL_RET_OK != ret) {
        return ret;
      }
    } else {
      // 节点名称和命名空间规则不需要扩展替换项
      *output_name = rcutils_strdup(rule->impl->replacement, allocator);
    }

    // 检查输出名称是否为空
    if (NULL == *output_name) {
      RCL_SET_ERROR_MSG("Failed to set output");
      return RCL_RET_ERROR;
    }
  }

  return RCL_RET_OK;
}

/**
 * @brief 重新映射话题名称，根据给定的本地和全局参数，节点名称和命名空间。
 *
 * @param[in] local_arguments 本地参数，用于指定特定节点的参数。
 * @param[in] global_arguments 全局参数，用于指定所有节点的参数。
 * @param[in] topic_name 要重新映射的原始话题名称。
 * @param[in] node_name 当前节点的名称。
 * @param[in] node_namespace 当前节点的命名空间。
 * @param[in] allocator 分配器，用于分配内存。
 * @param[out] output_name 输出参数，存储重新映射后的话题名称。
 * @return rcl_ret_t 返回 RCL_RET_OK 表示成功，其他值表示失败。
 */
rcl_ret_t rcl_remap_topic_name(
    const rcl_arguments_t* local_arguments,
    const rcl_arguments_t* global_arguments,
    const char* topic_name,
    const char* node_name,
    const char* node_namespace,
    rcl_allocator_t allocator,
    char** output_name) {
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(&allocator, "allocator is invalid", return RCL_RET_INVALID_ARGUMENT);
  // 检查话题名称是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(topic_name, RCL_RET_INVALID_ARGUMENT);

  // 初始化字符串映射结构体
  rcutils_string_map_t substitutions = rcutils_get_zero_initialized_string_map();
  // 使用分配器初始化字符串映射
  rcutils_ret_t rcutils_ret = rcutils_string_map_init(&substitutions, 0, allocator);
  rcl_ret_t ret = RCL_RET_ERROR;
  // 如果字符串映射初始化成功
  if (RCUTILS_RET_OK == rcutils_ret) {
    // 获取默认的话题名称替换规则
    ret = rcl_get_default_topic_name_substitutions(&substitutions);
    // 如果获取默认替换规则成功
    if (RCL_RET_OK == ret) {
      // 执行重新映射操作
      ret = rcl_remap_name(
          local_arguments, global_arguments, RCL_TOPIC_REMAP, topic_name, node_name, node_namespace,
          &substitutions, allocator, output_name);
    }
  }
  // 清理字符串映射结构体
  if (RCUTILS_RET_OK != rcutils_string_map_fini(&substitutions)) {
    return RCL_RET_ERROR;
  }
  // 返回结果
  return ret;
}

/**
 * @brief 重新映射服务名称，支持命名空间和节点名称的替换。
 *
 * @param[in] local_arguments 本地参数，用于获取特定节点的命名规则。
 * @param[in] global_arguments 全局参数，用于获取全局命名规则。
 * @param[in] service_name 原始服务名称。
 * @param[in] node_name 节点名称。
 * @param[in] node_namespace 节点命名空间。
 * @param[in] allocator 分配器，用于分配内存。
 * @param[out] output_name 输出的重新映射后的服务名称。
 * @return rcl_ret_t 返回 RCL_RET_OK 表示成功，其他值表示失败。
 */
rcl_ret_t rcl_remap_service_name(
    const rcl_arguments_t* local_arguments,
    const rcl_arguments_t* global_arguments,
    const char* service_name,
    const char* node_name,
    const char* node_namespace,
    rcl_allocator_t allocator,
    char** output_name) {
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(&allocator, "allocator is invalid", return RCL_RET_INVALID_ARGUMENT);
  // 检查服务名称是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(service_name, RCL_RET_INVALID_ARGUMENT);

  // 初始化字符串映射表，用于存储替换规则
  rcutils_string_map_t substitutions = rcutils_get_zero_initialized_string_map();
  rcutils_ret_t rcutils_ret = rcutils_string_map_init(&substitutions, 0, allocator);
  rcl_ret_t ret = RCL_RET_ERROR;
  // 如果字符串映射表初始化成功
  if (rcutils_ret == RCUTILS_RET_OK) {
    // 获取默认的主题名称替换规则
    ret = rcl_get_default_topic_name_substitutions(&substitutions);
    // 如果获取默认替换规则成功
    if (ret == RCL_RET_OK) {
      // 执行服务名称重新映射
      ret = rcl_remap_name(
          local_arguments, global_arguments, RCL_SERVICE_REMAP, service_name, node_name,
          node_namespace, &substitutions, allocator, output_name);
    }
  }
  // 清理字符串映射表
  if (RCUTILS_RET_OK != rcutils_string_map_fini(&substitutions)) {
    return RCL_RET_ERROR;
  }
  // 返回执行结果
  return ret;
}

/**
 * @brief 重映射节点名称
 *
 * @param[in] local_arguments 本地参数，用于指定当前节点的命名空间和名称
 * @param[in] global_arguments 全局参数，用于指定所有节点的命名空间和名称
 * @param[in] node_name 节点原始名称
 * @param[in] allocator 分配器，用于分配内存
 * @param[out] output_name 输出重映射后的节点名称
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_remap_node_name(
    const rcl_arguments_t* local_arguments,
    const rcl_arguments_t* global_arguments,
    const char* node_name,
    rcl_allocator_t allocator,
    char** output_name) {
  // 设置错误返回值
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_NODE_INVALID_NAME);
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_BAD_ALLOC);
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_ERROR);

  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(node_name, RCL_RET_INVALID_ARGUMENT);
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(&allocator, "allocator is invalid", return RCL_RET_INVALID_ARGUMENT);
  // 调用重映射函数
  return rcl_remap_name(
      local_arguments, global_arguments, RCL_NODENAME_REMAP, NULL, node_name, NULL, NULL, allocator,
      output_name);
}

/**
 * @brief 重映射节点命名空间
 *
 * @param[in] local_arguments 本地参数，用于指定当前节点的命名空间和名称
 * @param[in] global_arguments 全局参数，用于指定所有节点的命名空间和名称
 * @param[in] node_name 节点原始名称
 * @param[in] allocator 分配器，用于分配内存
 * @param[out] output_namespace 输出重映射后的节点命名空间
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_remap_node_namespace(
    const rcl_arguments_t* local_arguments,
    const rcl_arguments_t* global_arguments,
    const char* node_name,
    rcl_allocator_t allocator,
    char** output_namespace) {
  // 设置错误返回值
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_NODE_INVALID_NAMESPACE);
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_BAD_ALLOC);
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_ERROR);

  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(node_name, RCL_RET_INVALID_ARGUMENT);
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(&allocator, "allocator is invalid", return RCL_RET_INVALID_ARGUMENT);
  // 调用重映射函数
  return rcl_remap_name(
      local_arguments, global_arguments, RCL_NAMESPACE_REMAP, NULL, node_name, NULL, NULL,
      allocator, output_namespace);
}

/**
 * @brief 释放rcl_remap_t结构体中的内存并将其设置为NULL。
 *
 * @param[in,out] rule 指向要释放的rcl_remap_t结构体的指针。
 * @return 返回RCL_RET_OK，如果成功释放内存；否则返回相应的错误代码。
 * @retval RCL_RET_INVALID_ARGUMENT 如果输入参数无效。
 * @retval RCL_RET_ERROR 如果rcl_remap_t已经被释放两次。
 */
rcl_ret_t rcl_remap_fini(rcl_remap_t* rule) {
  // 检查输入参数是否为空，如果为空则返回RCL_RET_INVALID_ARGUMENT错误代码
  RCL_CHECK_ARGUMENT_FOR_NULL(rule, RCL_RET_INVALID_ARGUMENT);

  // 如果rule->impl不为空，则进行内存释放操作
  if (rule->impl) {
    // 初始化返回值为RCL_RET_OK
    rcl_ret_t ret = RCL_RET_OK;

    // 如果rule->impl->node_name不为空，则释放内存并将其设置为NULL
    if (NULL != rule->impl->node_name) {
      rule->impl->allocator.deallocate(rule->impl->node_name, rule->impl->allocator.state);
      rule->impl->node_name = NULL;
    }

    // 如果rule->impl->match不为空，则释放内存并将其设置为NULL
    if (NULL != rule->impl->match) {
      rule->impl->allocator.deallocate(rule->impl->match, rule->impl->allocator.state);
      rule->impl->match = NULL;
    }

    // 如果rule->impl->replacement不为空，则释放内存并将其设置为NULL
    if (NULL != rule->impl->replacement) {
      rule->impl->allocator.deallocate(rule->impl->replacement, rule->impl->allocator.state);
      rule->impl->replacement = NULL;
    }

    // 释放rule->impl内存并将其设置为NULL
    rule->impl->allocator.deallocate(rule->impl, rule->impl->allocator.state);
    rule->impl = NULL;

    // 返回操作结果
    return ret;
  }

  // 如果rule->impl为空，则表示rcl_remap_t已经被释放两次，返回RCL_RET_ERROR错误代码
  RCL_SET_ERROR_MSG("rcl_remap_t finalized twice");
  return RCL_RET_ERROR;
}

#ifdef __cplusplus
}
#endif
