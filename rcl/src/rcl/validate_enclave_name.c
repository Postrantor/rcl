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

#include "rcl/validate_enclave_name.h"

#include <ctype.h>
#include <rcutils/macros.h>
#include <rcutils/snprintf.h>
#include <stdio.h>
#include <string.h>

#include "./common.h"
#include "rcl/error_handling.h"
#include "rmw/validate_namespace.h"

/**
 * @brief 验证 enclave 名称是否有效
 *
 * @param[in] enclave 要验证的 enclave 名称
 * @param[out] validation_result 验证结果，指向一个整数，表示验证状态
 * @param[out] invalid_index 无效字符的索引（如果有）
 * @return rcl_ret_t 返回 RCL_RET_OK 表示成功，其他值表示失败
 */
rcl_ret_t rcl_validate_enclave_name(
    const char* enclave, int* validation_result, size_t* invalid_index) {
  // 检查 enclave 参数是否为空，如果为空则返回 RCL_RET_INVALID_ARGUMENT 错误
  RCL_CHECK_ARGUMENT_FOR_NULL(enclave, RCL_RET_INVALID_ARGUMENT);
  // 使用 rcl_validate_enclave_name_with_size 函数进行验证
  return rcl_validate_enclave_name_with_size(
      enclave, strlen(enclave), validation_result, invalid_index);
}

/**
 * @brief 使用给定的长度验证 enclave 名称是否有效
 *
 * @param[in] enclave 要验证的 enclave 名称
 * @param[in] enclave_length enclave 名称的长度
 * @param[out] validation_result 验证结果，指向一个整数，表示验证状态
 * @param[out] invalid_index 无效字符的索引（如果有）
 * @return rcl_ret_t 返回 RCL_RET_OK 表示成功，其他值表示失败
 */
rcl_ret_t rcl_validate_enclave_name_with_size(
    const char* enclave, size_t enclave_length, int* validation_result, size_t* invalid_index) {
  // 设置错误返回值
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_ERROR);

  // 检查 enclave 和 validation_result 参数是否为空，如果为空则返回 RCL_RET_INVALID_ARGUMENT 错误
  RCL_CHECK_ARGUMENT_FOR_NULL(enclave, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(validation_result, RCL_RET_INVALID_ARGUMENT);

  int tmp_validation_result;
  size_t tmp_invalid_index;
  // 使用 rmw_validate_namespace_with_size 函数进行验证
  rmw_ret_t ret = rmw_validate_namespace_with_size(
      enclave, enclave_length, &tmp_validation_result, &tmp_invalid_index);
  if (ret != RMW_RET_OK) {
    return rcl_convert_rmw_ret_to_rcl_ret(ret);
  }

  // 根据验证结果进行处理
  if (tmp_validation_result != RMW_NAMESPACE_VALID &&
      tmp_validation_result != RMW_NAMESPACE_INVALID_TOO_LONG) {
    // 使用 switch 语句处理不同的验证结果
    switch (tmp_validation_result) {
      case RMW_NAMESPACE_INVALID_IS_EMPTY_STRING:
        // 如果 enclave 名称为空字符串，设置相应的验证结果
        *validation_result = RCL_ENCLAVE_NAME_INVALID_IS_EMPTY_STRING;
        break;
      case RMW_NAMESPACE_INVALID_NOT_ABSOLUTE:
        // 如果 enclave 名称不是绝对路径，设置相应的验证结果
        *validation_result = RCL_ENCLAVE_NAME_INVALID_NOT_ABSOLUTE;
        break;
      case RMW_NAMESPACE_INVALID_ENDS_WITH_FORWARD_SLASH:
        // 如果 enclave 名称以正斜杠结尾，设置相应的验证结果
        *validation_result = RCL_ENCLAVE_NAME_INVALID_ENDS_WITH_FORWARD_SLASH;
        break;
      case RMW_NAMESPACE_INVALID_CONTAINS_UNALLOWED_CHARACTERS:
        // 如果 enclave 名称包含不允许的字符，设置相应的验证结果
        *validation_result = RCL_ENCLAVE_NAME_INVALID_CONTAINS_UNALLOWED_CHARACTERS;
        break;
      case RMW_NAMESPACE_INVALID_CONTAINS_REPEATED_FORWARD_SLASH:
        // 如果 enclave 名称包含重复的正斜杠，设置相应的验证结果
        *validation_result = RCL_ENCLAVE_NAME_INVALID_CONTAINS_REPEATED_FORWARD_SLASH;
        break;
      case RMW_NAMESPACE_INVALID_NAME_TOKEN_STARTS_WITH_NUMBER:
        // 如果 enclave 名称的某个部分以数字开头，设置相应的验证结果
        *validation_result = RCL_ENCLAVE_NAME_INVALID_NAME_TOKEN_STARTS_WITH_NUMBER;
        break;
      default: {
        char default_err_msg[256];
        // 显示未知验证结果的错误信息
        int ret = rcutils_snprintf(
            default_err_msg, sizeof(default_err_msg),
            "rcl_validate_enclave_name_with_size(): "
            "unknown rmw_validate_namespace_with_size() result '%d'",
            tmp_validation_result);
        if (ret < 0) {
          // 如果 rcutils_snprintf 失败，设置相应的错误消息
          RCL_SET_ERROR_MSG(
              "rcl_validate_enclave_name_with_size(): "
              "rcutils_snprintf() failed while reporting an unknown validation result");
        } else {
          // 设置默认的错误消息
          RCL_SET_ERROR_MSG(default_err_msg);
        }
      }
        // 返回 RCL_RET_ERROR 表示发生错误
        return RCL_RET_ERROR;
    }
    // 如果提供了 invalid_index 参数，将其设置为无效字符的索引
    if (invalid_index) {
      *invalid_index = tmp_invalid_index;
    }
    // 返回 RCL_RET_OK 表示验证成功
    return RCL_RET_OK;
  }

  /**
   * @brief 检查命名空间是否有效，并根据结果设置 enclave 名称的验证状态。
   *
   * @param[in] tmp_validation_result 临时命名空间验证结果，用于判断是否需要进一步检查 enclave
   * 名称。
   * @param[in] enclave_length enclave 名称的长度。
   * @param[out] validation_result 用于存储最终的 enclave 名称验证结果。
   * @param[out] invalid_index 如果 enclave 名称无效，此参数将存储无效字符的索引。
   * @return 返回 RCL_RET_OK 表示函数执行成功。
   */
  if (RMW_NAMESPACE_INVALID_TOO_LONG == tmp_validation_result) {
    // 如果命名空间长度超过限制，检查 enclave 名称长度是否在允许的范围内
    if (RCL_ENCLAVE_NAME_MAX_LENGTH >= enclave_length) {
      // 如果 enclave 名称长度合法，则设置验证结果为有效
      *validation_result = RCL_ENCLAVE_NAME_VALID;
    } else {
      // 如果 enclave 名称长度过长，则设置验证结果为无效并指定无效索引
      *validation_result = RCL_ENCLAVE_NAME_INVALID_TOO_LONG;
      if (invalid_index) {
        *invalid_index = RCL_ENCLAVE_NAME_MAX_LENGTH - 1;
      }
    }
    // 返回函数执行成功的状态码
    return RCL_RET_OK;
  }
}

// everything was ok, set result to valid namespace, avoid setting invalid_index, and return
*validation_result = RCL_ENCLAVE_NAME_VALID;
return RCL_RET_OK;
}

/**
 * @brief 根据验证结果返回相应的错误信息字符串
 *
 * @param[in] validation_result 验证结果，是一个整数值
 * @return 返回对应的错误信息字符串，如果验证通过则返回NULL
 */
const char* rcl_enclave_name_validation_result_string(int validation_result) {
  switch (validation_result) {
    case RCL_ENCLAVE_NAME_VALID:                    // 验证通过
      return NULL;
    case RCL_ENCLAVE_NAME_INVALID_IS_EMPTY_STRING:  // 名称为空字符串
      return "context name must not be empty";
    case RCL_ENCLAVE_NAME_INVALID_NOT_ABSOLUTE:  // 名称不是绝对路径（必须以'/'开头）
      return "context name must be absolute, it must lead with a '/'";
    case RCL_ENCLAVE_NAME_INVALID_ENDS_WITH_FORWARD_SLASH:  // 名称以'/'结尾，但不是仅包含一个'/'
      return "context name must not end with a '/', unless only a '/'";
    case RCL_ENCLAVE_NAME_INVALID_CONTAINS_UNALLOWED_CHARACTERS:    // 名称包含非法字符
      return "context name must not contain characters other than alphanumerics, '_', or '/'";
    case RCL_ENCLAVE_NAME_INVALID_CONTAINS_REPEATED_FORWARD_SLASH:  // 名称中包含重复的'/'
      return "context name must not contain repeated '/'";
    case RCL_ENCLAVE_NAME_INVALID_NAME_TOKEN_STARTS_WITH_NUMBER:  // 名称的某个部分以数字开头
      return "context name must not have a token that starts with a number";
    case RCL_ENCLAVE_NAME_INVALID_TOO_LONG:                       // 名称过长
      return "context name should not exceed '" RCUTILS_STRINGIFY(
          RCL_ENCLAVE_NAME_MAX_NAME_LENGTH) "'";
    default:  // 未知的验证结果
      return "unknown result code for rcl context name validation";
  }
}
