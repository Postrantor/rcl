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

#include "rcl/localhost.h"

#include <stdlib.h>
#include <string.h>

#include "rcl/error_handling.h"
#include "rcl/types.h"
#include "rcutils/env.h"

/**
 * @brief 本地主机环境变量名
 */
const char* const RCL_LOCALHOST_ENV_VAR = "ROS_LOCALHOST_ONLY";

/**
 * @brief 获取本地主机限制设置
 *
 * @param[out] localhost_only 指向一个 rmw_localhost_only_t
 * 类型的指针，用于存储获取到的本地主机限制设置
 * @return rcl_ret_t 返回操作结果，成功返回 RCL_RET_OK，否则返回相应的错误代码
 */
rcl_ret_t rcl_get_localhost_only(rmw_localhost_only_t* localhost_only) {
  // 定义一个指向环境变量值的指针
  const char* ros_local_host_env_val = NULL;
  // 定义一个指向获取环境变量错误信息的指针
  const char* get_env_error_str = NULL;

  // 检查是否可以设置错误消息并返回 RCL_RET_INVALID_ARGUMENT 错误
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  // 检查是否可以设置错误消息并返回 RCL_RET_ERROR 错误
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_ERROR);
  // 检查传入的参数 localhost_only 是否为空，如果为空则返回 RCL_RET_INVALID_ARGUMENT 错误
  RCL_CHECK_ARGUMENT_FOR_NULL(localhost_only, RCL_RET_INVALID_ARGUMENT);

  // 获取环境变量 RCL_LOCALHOST_ENV_VAR 的值，并将其存储在 ros_local_host_env_val 中
  get_env_error_str = rcutils_get_env(RCL_LOCALHOST_ENV_VAR, &ros_local_host_env_val);
  // 如果获取环境变量时发生错误，设置错误消息并返回 RCL_RET_ERROR 错误
  if (NULL != get_env_error_str) {
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Error getting env var '" RCUTILS_STRINGIFY(RCL_LOCALHOST_ENV_VAR) "': %s\n",
        get_env_error_str);
    return RCL_RET_ERROR;
  }
  // 根据获取到的环境变量值设置 localhost_only 的值
  *localhost_only = (ros_local_host_env_val != NULL && strcmp(ros_local_host_env_val, "1") == 0)
                        ? RMW_LOCALHOST_ONLY_ENABLED
                        : RMW_LOCALHOST_ONLY_DISABLED;
  // 返回操作成功
  return RCL_RET_OK;
}
