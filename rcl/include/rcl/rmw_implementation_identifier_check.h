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

#ifndef RCL__RMW_IMPLEMENTATION_IDENTIFIER_CHECK_H_
#define RCL__RMW_IMPLEMENTATION_IDENTIFIER_CHECK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/visibility_control.h"

/// \file
/// \brief 用于检查 RMW 实现的头文件

/// 控制使用哪个 RMW 实现的环境变量名称。
#define RMW_IMPLEMENTATION_ENV_VAR_NAME "RMW_IMPLEMENTATION"

/// 控制所选 RMW 实现是否与正在使用的实现匹配的环境变量名称。
#define RCL_ASSERT_RMW_ID_MATCHES_ENV_VAR_NAME "RCL_ASSERT_RMW_ID_MATCHES"

/// 检查当前使用的 RMW 实现是否与用户请求的实现相匹配。
/**
 * \return #RCL_RET_OK 如果当前使用的 RMW 实现与用户请求的实现相匹配，
 * \return #RCL_RET_MISMATCHED_RMW_ID 如果 RMW 实现不匹配，
 * \return #RCL_RET_BAD_ALLOC 如果内存分配失败，
 * \return #RCL_RET_ERROR 如果发生其他错误。
 */
RCL_PUBLIC
rcl_ret_t rcl_rmw_implementation_identifier_check(void);

#ifdef __cplusplus
}
#endif

#endif  // RCL__RMW_IMPLEMENTATION_IDENTIFIER_CHECK_H_
