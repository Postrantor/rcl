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

/// @file

#ifndef RCL__VALIDATE_TOPIC_NAME_H_
#define RCL__VALIDATE_TOPIC_NAME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/macros.h"
#include "rcl/types.h"
#include "rcl/visibility_control.h"

/// 主题名称有效。
#define RCL_TOPIC_NAME_VALID 0

/// 主题名称无效，因为空字符串。
#define RCL_TOPIC_NAME_INVALID_IS_EMPTY_STRING 1

/// 主题名称无效，因为它以正斜杠结尾。
#define RCL_TOPIC_NAME_INVALID_ENDS_WITH_FORWARD_SLASH 2

/// 主题名称无效，因为它包含不允许的字符。
#define RCL_TOPIC_NAME_INVALID_CONTAINS_UNALLOWED_CHARACTERS 3

/// 主题名称无效，因为其中一个标记以数字开头。
#define RCL_TOPIC_NAME_INVALID_NAME_TOKEN_STARTS_WITH_NUMBER 4

/// 主题名称无效，因为它有不匹配的大括号。
#define RCL_TOPIC_NAME_INVALID_UNMATCHED_CURLY_BRACE 5

/// 主题名称无效，因为它有错位的波浪号。
#define RCL_TOPIC_NAME_INVALID_MISPLACED_TILDE 6

/// 主题名称无效，因为波浪号后面没有直接跟正斜杠。
#define RCL_TOPIC_NAME_INVALID_TILDE_NOT_FOLLOWED_BY_FORWARD_SLASH 7

/// 主题名称无效，因为其中一个替换项包含不允许的字符。
#define RCL_TOPIC_NAME_INVALID_SUBSTITUTION_CONTAINS_UNALLOWED_CHARACTERS 8

/// 主题名称无效，因为其中一个替换项以数字开头。
#define RCL_TOPIC_NAME_INVALID_SUBSTITUTION_STARTS_WITH_NUMBER 9

/// 验证给定的主题名称。
/**
 * 主题名称不需要是完全限定的名称，但应遵循以下文档中的一些规则：
 *
 *   http://design.ros2.org/articles/topic_and_service_names.html
 *
 * 请注意，此函数期望上述文档中描述的任何URL后缀已经被删除。
 *
 * 如果输入主题有效，则将 RCL_TOPIC_NAME_VALID 存储到 validation_result 中。
 * 如果输入主题违反了任何规则，那么这些值中的一个将存储到 validation_result 中：
 *
 * - RCL_TOPIC_NAME_INVALID_IS_EMPTY_STRING
 * - RCL_TOPIC_NAME_INVALID_ENDS_WITH_FORWARD_SLASH
 * - RCL_TOPIC_NAME_INVALID_CONTAINS_UNALLOWED_CHARACTERS
 * - RCL_TOPIC_NAME_INVALID_NAME_TOKEN_STARTS_WITH_NUMBER
 * - RCL_TOPIC_NAME_INVALID_UNMATCHED_CURLY_BRACE
 * - RCL_TOPIC_NAME_INVALID_MISPLACED_TILDE
 * - RCL_TOPIC_NAME_INVALID_TILDE_NOT_FOLLOWED_BY_FORWARD_SLASH
 * - RCL_TOPIC_NAME_INVALID_SUBSTITUTION_CONTAINS_UNALLOWED_CHARACTERS
 * - RCL_TOPIC_NAME_INVALID_SUBSTITUTION_STARTS_WITH_NUMBER
 *
 * 一些检查，如检查非法重复的正斜杠，在此功能中不进行检查，因为在扩展后还需要再次检查。
 * 这个子集检查的目的是尝试捕获与 rcl_expand_topic_name() 扩展的内容相关的问题，如 `~` 或
 * `{}` 内的替换项，或者明显阻止扩展的其他问题，如 RCL_TOPIC_NAME_INVALID_IS_EMPTY_STRING。
 *
 * 此函数不检查替换项是否为已知替换项，只检查 `{}` 内容是否遵循上面链接的文档中概述的规则。
 *
 * 使用 rcl_expand_topic_name() 后，可以使用 rmw_validate_full_topic_name() 进行更严格的验证。
 *
 * 此外，如果 invalid_index 参数不为 NULL，则将在主题名称字符串中分配违规发生的索引。
 * 如果验证通过，则不设置 invalid_index。
 *
 * \param[in] topic_name 要验证的主题名称，必须以空终止符结尾
 * \param[out] validation_result 验证失败的原因（如果有）
 * \param[out] invalid_index 输入主题无效时的违规索引
 * \return #RCL_RET_OK 如果主题名称扩展成功，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_validate_topic_name(
  const char * topic_name, int * validation_result, size_t * invalid_index);

/// 验证给定的主题名称。
/**
 * 这是一个带有额外参数的重载，用于表示 topic_name 的长度。
 * \param[in] topic_name 要验证的主题名称，必须以空终止符结尾
 * \param[in] topic_name_length 主题名称中的字符数。
 * \param[out] validation_result 验证失败的原因（如果有）
 * \param[out] invalid_index 输入主题无效时的违规索引
 * \return #RCL_RET_OK 如果主题名称扩展成功，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 *
 * \sa rcl_validate_topic_name()
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_validate_topic_name_with_size(
  const char * topic_name, size_t topic_name_length, int * validation_result,
  size_t * invalid_index);

/// 返回验证结果描述，如果未知或 RCL_TOPIC_NAME_VALID，则返回 NULL。
/**
 * \param[in] validation_result 要获取字符串的验证结果
 * \return 验证结果的字符串描述（如果成功），或
 * \return 如果验证结果无效，则返回 NULL。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const char * rcl_topic_name_validation_result_string(int validation_result);

#ifdef __cplusplus
}
#endif

#endif  // RCL__VALIDATE_TOPIC_NAME_H_
