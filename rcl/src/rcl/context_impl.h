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

#ifndef RCL__CONTEXT_IMPL_H_
#define RCL__CONTEXT_IMPL_H_

#include "./init_options_impl.h"
#include "rcl/context.h"
#include "rcl/error_handling.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \internal
 * \struct rcl_context_impl_s
 * \brief ROS2 RCL 上下文实现结构体 (ROS2 RCL context implementation structure)
 */
struct rcl_context_impl_s {
  // 初始化和关闭时使用的分配器 (Allocator used during init and shutdown)
  rcl_allocator_t allocator;
  // 初始化时给定的初始化选项副本 (Copy of init options given during init)
  rcl_init_options_t init_options;
  // argv 的长度（可能为 `0`）(Length of argv (may be `0`))
  int64_t argc;
  // 初始化时使用的 argv 副本（可能为 `NULL`）(Copy of argv used during init (may be `NULL`))
  char** argv;
  // RMW 上下文 (rmw context)
  rmw_context_t rmw_context;
};

/**
 * \brief 清理上下文的函数 (Function to clean up the context)
 * \param[in] context 指向要清理的上下文的指针 (Pointer to the context to be cleaned up)
 * \return 返回一个表示操作结果的 `rcl_ret_t` 类型值 (Returns a `rcl_ret_t` type value representing
 * the result of the operation)
 */
RCL_LOCAL
rcl_ret_t __cleanup_context(rcl_context_t* context);

#ifdef __cplusplus
}
#endif

#endif  // RCL__CONTEXT_IMPL_H_
