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

#ifndef RCL__ERROR_HANDLING_H_
#define RCL__ERROR_HANDLING_H_

#include "rcutils/error_handling.h"

/// \file rcl_error_handling.h
/// \brief RCL中的错误处理，实际上是对rcutils错误处理的别名。

// 引入 rcutils 的错误处理头文件
#include "rcutils/error_handling.h"

/// 定义 rcl_error_state_t 类型为 rcutils_error_state_t 类型
typedef rcutils_error_state_t rcl_error_state_t;

/// 定义 rcl_error_string_t 类型为 rcutils_error_string_t 类型
typedef rcutils_error_string_t rcl_error_string_t;

/// 初始化线程局部存储的错误处理
#define rcl_initialize_error_handling_thread_local_storage \
  rcutils_initialize_error_handling_thread_local_storage

/// 设置错误状态
#define rcl_set_error_state rcutils_set_error_state

/// 检查参数是否为空，如果为空则返回指定的错误类型
#define RCL_CHECK_ARGUMENT_FOR_NULL(argument, error_return_type) \
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(argument, error_return_type)

/// 检查值是否为空，如果为空则执行带有指定消息的错误语句
#define RCL_CHECK_FOR_NULL_WITH_MSG(value, msg, error_statement) \
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(value, msg, error_statement)

/// 设置错误消息
#define RCL_SET_ERROR_MSG(msg) RCUTILS_SET_ERROR_MSG(msg)

/// 使用格式化字符串设置错误消息
#define RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(fmt_str, ...) \
  RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(fmt_str, __VA_ARGS__)

/// 检查是否设置了错误
#define rcl_error_is_set rcutils_error_is_set

/// 获取错误状态
#define rcl_get_error_state rcutils_get_error_state

/// 获取错误字符串
#define rcl_get_error_string rcutils_get_error_string

/// 重置错误
#define rcl_reset_error rcutils_reset_error

#endif  // RCL__ERROR_HANDLING_H_
