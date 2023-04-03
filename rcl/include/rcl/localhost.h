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

/// @file

#ifndef RCL__LOCALHOST_H_
#define RCL__LOCALHOST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/types.h"
#include "rcl/visibility_control.h"
#include "rmw/localhost.h"

// 声明一个外部常量字符串，表示RCL本地主机环境变量。
extern const char* const RCL_LOCALHOST_ENV_VAR;

// 定义一个函数，用于确定用户是否希望仅使用环回（loopback）进行通信。
/**
 * 函数功能：基于环境检查是否应该使用localhost进行网络通信。
 *
 * 参数列表：
 * \param[out] localhost_only 输出参数，表示是否仅使用本地主机进行通信。不能为空（Must not be
 * NULL）。
 *
 * 返回值：
 * \return #RCL_RET_INVALID_ARGUMENT 如果参数无效，则返回此值。
 * \return #RCL_RET_ERROR 如果发生意外错误，则返回此值。
 * \return #RCL_RET_OK 如果一切正常，则返回此值。
 */
RCL_PUBLIC
rcl_ret_t rcl_get_localhost_only(rmw_localhost_only_t* localhost_only);  // 函数声明

#ifdef __cplusplus
}
#endif

#endif  // RCL__LOCALHOST_H_
