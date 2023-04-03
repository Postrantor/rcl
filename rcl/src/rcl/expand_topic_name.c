// Copyright 2017 Open Source Robotics Foundation, Inc.
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

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/expand_topic_name.h"

#include <stdio.h>
#include <string.h>

#include "./common.h"
#include "rcl/error_handling.h"
#include "rcl/types.h"
#include "rcl/validate_topic_name.h"
#include "rcutils/error_handling.h"
#include "rcutils/format_string.h"
#include "rcutils/repl_str.h"
#include "rcutils/strdup.h"
#include "rmw/error_handling.h"
#include "rmw/types.h"
#include "rmw/validate_namespace.h"
#include "rmw/validate_node_name.h"

// 内置替换字符串
#define SUBSTITUION_NODE_NAME "{node}"
#define SUBSTITUION_NAMESPACE "{ns}"
#define SUBSTITUION_NAMESPACE2 "{namespace}"

/**
 * @brief 扩展主题名称，支持替换字符串。
 *
 * @param input_topic_name 输入的主题名称
 * @param node_name 节点名称
 * @param node_namespace 节点命名空间
 * @param substitutions 替换字符串映射表
 * @param allocator 分配器
 * @param output_topic_name 输出扩展后的主题名称
 * @return rcl_ret_t 返回状态码
 */
rcl_ret_t rcl_expand_topic_name(
    const char* input_topic_name,
    const char* node_name,
    const char* node_namespace,
    const rcutils_string_map_t* substitutions,
    rcl_allocator_t allocator,
    char** output_topic_name) {
  // 检查可能为空的参数
  RCL_CHECK_ARGUMENT_FOR_NULL(input_topic_name, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(node_name, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(node_namespace, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(substitutions, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(output_topic_name, RCL_RET_INVALID_ARGUMENT);

  // 验证输入的主题名称
  int validation_result;
  rcl_ret_t ret = rcl_validate_topic_name(input_topic_name, &validation_result, NULL);
  if (ret != RCL_RET_OK) {
    // 错误信息已设置
    return ret;
  }
  if (validation_result != RCL_TOPIC_NAME_VALID) {
    RCL_SET_ERROR_MSG("topic name is invalid");
    return RCL_RET_TOPIC_NAME_INVALID;
  }

  // 验证节点名称
  rmw_ret_t rmw_ret;
  rmw_ret = rmw_validate_node_name(node_name, &validation_result, NULL);
  if (rmw_ret != RMW_RET_OK) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
  }
  if (validation_result != RMW_NODE_NAME_VALID) {
    RCL_SET_ERROR_MSG("node name is invalid");
    return RCL_RET_NODE_INVALID_NAME;
  }

  // 验证命名空间
  rmw_ret = rmw_validate_namespace(node_namespace, &validation_result, NULL);
  if (rmw_ret != RMW_RET_OK) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
  }
  if (validation_result != RMW_NODE_NAME_VALID) {
    RCL_SET_ERROR_MSG("node namespace is invalid");
    return RCL_RET_NODE_INVALID_NAMESPACE;
  }

  // 检查主题是否有替换字符串需要处理
  bool has_a_substitution = strchr(input_topic_name, '{') != NULL;
  bool has_a_namespace_tilde = input_topic_name[0] == '~';
  bool is_absolute = input_topic_name[0] == '/';

  // 如果是绝对路径且没有替换字符串
  if (is_absolute && !has_a_substitution) {
    // 无需处理，复制并返回
    *output_topic_name = rcutils_strdup(input_topic_name, allocator);
    if (!*output_topic_name) {
      *output_topic_name = NULL;
      RCL_SET_ERROR_MSG("failed to allocate memory for output topic");
      return RCL_RET_BAD_ALLOC;
    }
    return RCL_RET_OK;
  }

  char* local_output = NULL;

  // 如果有命名空间波浪线，先替换
  if (has_a_namespace_tilde) {
    // 特殊情况，node_namespace只是'/'
    // 那么不需要额外的分隔'/'
    const char* fmt = (strlen(node_namespace) == 1) ? "%s%s%s" : "%s/%s%s";
    local_output =
        rcutils_format_string(allocator, fmt, node_namespace, node_name, input_topic_name + 1);
    if (!local_output) {
      *output_topic_name = NULL;
      RCL_SET_ERROR_MSG("failed to allocate memory for output topic");
      return RCL_RET_BAD_ALLOC;
    }
  }

  // 如果有替换字符串，进行替换
  if (has_a_substitution) {
    const char* current_output = (local_output) ? local_output : input_topic_name;
    char* next_opening_brace = NULL;

    /**
     * @brief 对每个替换字符串进行循环处理
     *
     * @param[in] current_output 当前输出字符串
     * @param[in] next_opening_brace 下一个左大括号的位置
     * @param[in] next_closing_brace 下一个右大括号的位置
     * @param[in] substitution_substr_len 替换子串的长度
     * @param[in] replacement 替换内容
     * @param[in] output_topic_name 输出主题名称
     * @param[in] allocator 分配器
     * @return RCL_RET_UNKNOWN_SUBSTITUTION, RCL_RET_BAD_ALLOC 或其他错误代码
     */
    while ((next_opening_brace = strchr(current_output, '{')) != NULL) {
      // 查找下一个右大括号
      char* next_closing_brace = strchr(current_output, '}');
      // 计算替换子串的长度
      size_t substitution_substr_len = next_closing_brace - next_opening_brace + 1;

      // 确定此替换字符串的替换内容
      const char* replacement = NULL;
      if (strncmp(SUBSTITUION_NODE_NAME, next_opening_brace, substitution_substr_len) == 0) {
        // 如果是节点名称，则使用节点名称替换
        replacement = node_name;
      } else if (  // NOLINT
          strncmp(SUBSTITUION_NAMESPACE, next_opening_brace, substitution_substr_len) == 0 ||
          strncmp(SUBSTITUION_NAMESPACE2, next_opening_brace, substitution_substr_len) == 0) {
        // 如果是命名空间，则使用命名空间替换
        replacement = node_namespace;
      } else {
        // 否则，从替换映射中获取替换内容
        replacement = rcutils_string_map_getn(
            substitutions, next_opening_brace + 1, substitution_substr_len - 2);
        if (!replacement) {
          *output_topic_name = NULL;
          char* unmatched_substitution =
              rcutils_strndup(next_opening_brace, substitution_substr_len, allocator);
          if (unmatched_substitution) {
            RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
                "unknown substitution: %s", unmatched_substitution);
          } else {
            RCUTILS_SAFE_FWRITE_TO_STDERR("failed to allocate memory for unmatched substitution\n");
          }
          // 释放未匹配的替换和本地输出内存
          allocator.deallocate(unmatched_substitution, allocator.state);
          allocator.deallocate(local_output, allocator.state);
          return RCL_RET_UNKNOWN_SUBSTITUTION;
        }
      }

      // 进行替换
      char* next_substitution =
          rcutils_strndup(next_opening_brace, substitution_substr_len, allocator);
      if (!next_substitution) {
        *output_topic_name = NULL;
        RCL_SET_ERROR_MSG("failed to allocate memory for substitution");
        allocator.deallocate(local_output, allocator.state);
        return RCL_RET_BAD_ALLOC;
      }
      char* original_local_output = local_output;
      // 使用替换内容替换原始字符串
      local_output = rcutils_repl_str(current_output, next_substitution, replacement, &allocator);
      // 释放下一个替换和原始本地输出内存
      allocator.deallocate(next_substitution, allocator.state);
      allocator.deallocate(original_local_output, allocator.state);
      if (!local_output) {
        *output_topic_name = NULL;
        RCL_SET_ERROR_MSG("failed to allocate memory for expanded topic");
        return RCL_RET_BAD_ALLOC;
      }
      current_output = local_output;

      // 循环直到所有替换字符串都被替换
    }  // while
  }

  // 最后，如果名称不是绝对路径，则将其转换为绝对路径
  if ((local_output && local_output[0] != '/') || (!local_output && input_topic_name[0] != '/')) {
    // 保存原始的 local_output 指针
    char* original_local_output = local_output;
    // 根据节点命名空间长度确定格式字符串
    const char* fmt = (strlen(node_namespace) == 1) ? "%s%s" : "%s/%s";
    // 使用 rcutils_format_string 函数将节点命名空间和 local_output 或 input_topic_name 进行拼接
    local_output = rcutils_format_string(
        allocator, fmt, node_namespace, (local_output) ? local_output : input_topic_name);
    // 如果原始的 local_output 不为空，释放其内存
    if (original_local_output) {
      allocator.deallocate(original_local_output, allocator.state);
    }
    // 如果新的 local_output 为空，设置错误信息并返回 RCL_RET_BAD_ALLOC
    if (!local_output) {
      *output_topic_name = NULL;
      RCL_SET_ERROR_MSG("failed to allocate memory for output topic");
      return RCL_RET_BAD_ALLOC;
    }
  }

  // 最后将结果存储在输出指针中并返回
  *output_topic_name = local_output;
  return RCL_RET_OK;
}

/**
 * @brief 获取默认的主题名称替换规则
 *
 * 该函数用于获取默认的主题名称替换规则，当前没有默认的替换规则。
 *
 * @param[out] string_map 存储默认主题名称替换规则的字符串映射
 * @return 返回 rcl_ret_t 类型的结果状态
 *         - RCL_RET_OK：成功获取默认主题名称替换规则
 *         - RCL_RET_INVALID_ARGUMENT：传入的参数无效
 */
rcl_ret_t rcl_get_default_topic_name_substitutions(rcutils_string_map_t* string_map) {
  // 检查传入的 string_map 参数是否为空，如果为空，则返回 RCL_RET_INVALID_ARGUMENT 错误
  RCL_CHECK_ARGUMENT_FOR_NULL(string_map, RCL_RET_INVALID_ARGUMENT);

  // 目前没有默认的替换规则

  // 返回 RCL_RET_OK 表示成功获取默认主题名称替换规则
  return RCL_RET_OK;
}

#ifdef __cplusplus
}
#endif
