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

/// @file

#ifndef RCL__VALIDATE_ENCLAVE_NAME_H_
#define RCL__VALIDATE_ENCLAVE_NAME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/macros.h"
#include "rcl/types.h"
#include "rcl/visibility_control.h"
#include "rmw/validate_namespace.h"
#include "rmw/validate_node_name.h"

/// 验证 enclave 名称是否有效。
#define RCL_ENCLAVE_NAME_VALID RMW_NAMESPACE_VALID

/// enclave 名称无效，因为它是一个空字符串。
#define RCL_ENCLAVE_NAME_INVALID_IS_EMPTY_STRING RMW_NAMESPACE_INVALID_IS_EMPTY_STRING

/// enclave 名称无效，因为它不是绝对的。
#define RCL_ENCLAVE_NAME_INVALID_NOT_ABSOLUTE RMW_NAMESPACE_INVALID_NOT_ABSOLUTE

/// enclave 名称无效，因为它以正斜杠结尾。
#define RCL_ENCLAVE_NAME_INVALID_ENDS_WITH_FORWARD_SLASH \
  RMW_NAMESPACE_INVALID_ENDS_WITH_FORWARD_SLASH

/// enclave 名称无效，因为它包含不允许的字符。
#define RCL_ENCLAVE_NAME_INVALID_CONTAINS_UNALLOWED_CHARACTERS \
  RMW_NAMESPACE_INVALID_CONTAINS_UNALLOWED_CHARACTERS

/// enclave 名称无效，因为它包含重复的正斜杠。
#define RCL_ENCLAVE_NAME_INVALID_CONTAINS_REPEATED_FORWARD_SLASH \
  RMW_NAMESPACE_INVALID_CONTAINS_REPEATED_FORWARD_SLASH

/// enclave 名称无效，因为其中一个标记以数字开头。
#define RCL_ENCLAVE_NAME_INVALID_NAME_TOKEN_STARTS_WITH_NUMBER \
  RMW_NAMESPACE_INVALID_NAME_TOKEN_STARTS_WITH_NUMBER

/// enclave 名称无效，因为名称太长。
#define RCL_ENCLAVE_NAME_INVALID_TOO_LONG RMW_NAMESPACE_INVALID_TOO_LONG

/// enclave 名称的最大长度。
#define RCL_ENCLAVE_NAME_MAX_LENGTH RMW_NODE_NAME_MAX_NAME_LENGTH

/// 判断给定的 enclave 名称是否有效。
/**
 * 使用与 rmw_validate_namespace() 相同的规则。
 * 唯一的区别是允许的最大长度，可以达到 255 个字符。
 *
 * \param[in] enclave 要验证的 enclave
 * \param[out] validation_result 存储检查结果的 int
 * \param[out] invalid_index 输入字符串中发生错误的索引
 * \return #RCL_RET_OK 如果成功运行检查，或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果参数无效，或者
 * \return #RCL_RET_ERROR 当发生未指定的错误时。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_validate_enclave_name(
  const char * enclave, int * validation_result, size_t * invalid_index);

/// 判断给定的 enclave 名称是否有效。
/**
 * 这是 rcl_validate_enclave_name() 的重载，带有一个额外的参数
 * 用于 enclave 的长度。
 *
 * \param[in] enclave 要验证的 enclave
 * \param[in] enclave_length enclave 中的字符数
 * \param[out] validation_result 存储检查结果的 int
 * \param[out] invalid_index 输入字符串中发生错误的索引
 * \return #RCL_RET_OK 如果成功运行检查，或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果参数无效，或者
 * \return #RCL_RET_ERROR 当发生未指定的错误时。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_validate_enclave_name_with_size(
  const char * enclave, size_t enclave_length, int * validation_result, size_t * invalid_index);

/// 返回验证结果描述，如果未知或 RCL_ENCLAVE_NAME_VALID，则返回 NULL。
/**
 * \param[in] validation_result 要获取字符串的验证结果
 * \return 验证结果的字符串描述（如果成功），或者
 * \return 如果验证结果无效，则返回 NULL。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const char * rcl_enclave_name_validation_result_string(int validation_result);

#ifdef __cplusplus
}
#endif

#endif  // RCL__VALIDATE_ENCLAVE_NAME_H_
