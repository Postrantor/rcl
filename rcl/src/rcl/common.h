// Copyright 2015 Open Source Robotics Foundation, Inc.
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

#ifndef RCL__COMMON_H_
#define RCL__COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/types.h"

/**
 * @brief 转换常见的 rmw_ret_t 返回代码为 rcl_ret_t 的便捷函数
 *
 * 该函数用于将 RMW 层的返回代码转换为 RCL 层的返回代码，以便在 ROS2 中进行错误处理。
 *
 * @param rmw_ret RMW 层的返回代码
 * @return 对应的 RCL 层返回代码
 */
rcl_ret_t rcl_convert_rmw_ret_to_rcl_ret(rmw_ret_t rmw_ret);

#ifdef __cplusplus
}
#endif

#endif  // RCL__COMMON_H_
