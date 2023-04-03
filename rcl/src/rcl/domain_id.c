// Copyright 2019 Open Source Robotics Foundation, Inc.
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

#include "rcl/domain_id.h"

#include <errno.h>
#include <limits.h>

#include "rcl/error_handling.h"
#include "rcl/types.h"
#include "rcutils/env.h"

/**
 * @file
 * @brief 获取默认域ID的实现
 */

// 定义环境变量名称
const char* const RCL_DOMAIN_ID_ENV_VAR = "ROS_DOMAIN_ID";

/**
 * @brief 获取默认域ID
 *
 * @param[out] domain_id 用于存储获取到的域ID
 * @return rcl_ret_t 返回操作结果，成功返回RCL_RET_OK，否则返回相应的错误代码
 */
rcl_ret_t rcl_get_default_domain_id(size_t* domain_id) {
  // 检查是否可以设置错误消息并返回指定的错误代码
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_ERROR);

  // 声明ROS域ID和环境变量获取错误字符串
  const char* ros_domain_id = NULL;
  const char* get_env_error_str = NULL;

  // 检查domain_id参数是否为空，如果为空则返回无效参数错误
  RCL_CHECK_ARGUMENT_FOR_NULL(domain_id, RCL_RET_INVALID_ARGUMENT);

  // 获取环境变量值
  get_env_error_str = rcutils_get_env(RCL_DOMAIN_ID_ENV_VAR, &ros_domain_id);
  if (NULL != get_env_error_str) {
    // 设置错误消息并返回错误
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Error getting env var '" RCUTILS_STRINGIFY(RCL_DOMAIN_ID_ENV_VAR) "': %s\n",
        get_env_error_str);
    return RCL_RET_ERROR;
  }
  // 如果获取到的环境变量值不为空且不等于空字符串
  if (ros_domain_id && strcmp(ros_domain_id, "") != 0) {
    char* end = NULL;
    // 将字符串转换为无符号长整型数
    unsigned long number = strtoul(ros_domain_id, &end, 0);  // NOLINT(runtime/int)
    // 如果转换结果为0且未到达字符串末尾，则返回错误
    if (number == 0UL && *end != '\0') {
      RCL_SET_ERROR_MSG("ROS_DOMAIN_ID is not an integral number");
      return RCL_RET_ERROR;
    }
    // 如果转换结果超出范围或大于SIZE_MAX，则返回错误
    if ((number == ULONG_MAX && errno == ERANGE) || number > SIZE_MAX) {
      RCL_SET_ERROR_MSG("ROS_DOMAIN_ID is out of range");
      return RCL_RET_ERROR;
    }
    // 将转换结果存储到domain_id中
    *domain_id = (size_t)number;
  }
  // 返回成功
  return RCL_RET_OK;
}
