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

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/guard_condition.h"

#include "./context_impl.h"
#include "rcl/error_handling.h"
#include "rcl/rcl.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"

/**
 * @file
 * @brief
 * 本文件包含了rcl_guard_condition_impl_s结构体的定义以及rcl_get_zero_initialized_guard_condition函数的实现。
 */

/**
 * @struct rcl_guard_condition_impl_s
 * @brief rcl_guard_condition_impl_s 结构体，用于存储守护条件的实现细节。
 */
struct rcl_guard_condition_impl_s {
  rmw_guard_condition_t
      *rmw_handle;  ///< rmw_handle
                    ///< 指向一个rmw_guard_condition_t类型的指针，用于存储底层中间件的守护条件句柄。
  bool allocated_rmw_guard_condition;  ///< allocated_rmw_guard_condition
                                       ///< 布尔值，表示是否已分配底层中间件的守护条件句柄。
  rcl_guard_condition_options_t
      options;  ///< options 是一个rcl_guard_condition_options_t类型的变量，用于存储守护条件的选项。
};

/**
 * @brief 获取一个初始化为零的守护条件对象。
 *
 * @return 返回一个初始化为零的rcl_guard_condition_t类型的对象。
 */
rcl_guard_condition_t rcl_get_zero_initialized_guard_condition() {
  static rcl_guard_condition_t null_guard_condition = {
      .context = 0, .impl = 0};  ///< 定义并初始化一个静态的空守护条件对象。
  return null_guard_condition;   ///< 返回空守护条件对象。
}

/**
 * @brief 初始化一个rcl_guard_condition_t实例，从rmw_guard_condition_t实例中获取信息。
 *
 * 如果参数为null，此函数将创建一个rmw_guard_condition。
 *
 * @param[in,out] guard_condition 用于存储初始化后的rcl_guard_condition_t实例的指针。
 * @param[in] rmw_guard_condition
 * 用于初始化rcl_guard_condition_t实例的rmw_guard_condition_t实例的指针。
 * @param[in] context rcl_context_t实例的指针，确保rcl已经初始化。
 * @param[in] options 包含分配器等选项的rcl_guard_condition_options_t实例。
 * @return 返回RCL_RET_OK（成功）或错误代码（失败）。
 */
rcl_ret_t __rcl_guard_condition_init_from_rmw_impl(
    rcl_guard_condition_t *guard_condition,
    const rmw_guard_condition_t *rmw_guard_condition,
    rcl_context_t *context,
    const rcl_guard_condition_options_t options) {
  // 执行参数验证
  const rcl_allocator_t *allocator = &options.allocator;
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(guard_condition, RCL_RET_INVALID_ARGUMENT);

  // 确保guard_condition句柄已初始化为零
  if (guard_condition->impl) {
    RCL_SET_ERROR_MSG("guard_condition already initialized, or memory was unintialized");
    return RCL_RET_ALREADY_INIT;
  }

  // 确保rcl已经初始化
  RCL_CHECK_ARGUMENT_FOR_NULL(context, RCL_RET_INVALID_ARGUMENT);
  if (!rcl_context_is_valid(context)) {
    RCL_SET_ERROR_MSG(
        "the given context is not valid, "
        "either rcl_init() was not called or rcl_shutdown() was called.");
    return RCL_RET_NOT_INIT;
  }

  // 为guard_condition impl分配空间
  guard_condition->impl = (rcl_guard_condition_impl_t *)allocator->allocate(
      sizeof(rcl_guard_condition_impl_t), allocator->state);
  if (!guard_condition->impl) {
    RCL_SET_ERROR_MSG("allocating memory failed");
    return RCL_RET_BAD_ALLOC;
  }

  // 创建rmw guard condition
  if (rmw_guard_condition) {
    // 如果给定，只需分配（取消const）
    guard_condition->impl->rmw_handle = (rmw_guard_condition_t *)rmw_guard_condition;
    guard_condition->impl->allocated_rmw_guard_condition = false;
  } else {
    // 否则创建一个
    guard_condition->impl->rmw_handle = rmw_create_guard_condition(&(context->impl->rmw_context));
    if (!guard_condition->impl->rmw_handle) {
      // 释放impl并退出
      allocator->deallocate(guard_condition->impl, allocator->state);
      guard_condition->impl = NULL;
      RCL_SET_ERROR_MSG(rmw_get_error_string().str);
      return RCL_RET_ERROR;
    }
    guard_condition->impl->allocated_rmw_guard_condition = true;
  }

  // 将选项复制到impl中
  guard_condition->impl->options = options;
  return RCL_RET_OK;
}

/**
 * @brief 初始化一个rcl_guard_condition_t结构体实例
 *
 * @param[in,out] guard_condition 指向待初始化的rcl_guard_condition_t结构体指针
 * @param[in] context 指向rcl_context_t结构体实例的指针，用于提供上下文信息
 * @param[in] options rcl_guard_condition_options_t类型的选项，用于配置guard_condition
 * @return 返回rcl_ret_t类型的结果，表示操作成功或失败
 */
rcl_ret_t rcl_guard_condition_init(
    rcl_guard_condition_t *guard_condition,
    rcl_context_t *context,
    const rcl_guard_condition_options_t options) {
  // NULL表示"创建一个新的rmw guard condition"
  return __rcl_guard_condition_init_from_rmw_impl(guard_condition, NULL, context, options);
}

/**
 * @brief 使用给定的rmw_guard_condition_t实例初始化一个rcl_guard_condition_t结构体实例
 *
 * @param[in,out] guard_condition 指向待初始化的rcl_guard_condition_t结构体指针
 * @param[in] rmw_guard_condition 指向已存在的rmw_guard_condition_t结构体实例的指针
 * @param[in] context 指向rcl_context_t结构体实例的指针，用于提供上下文信息
 * @param[in] options rcl_guard_condition_options_t类型的选项，用于配置guard_condition
 * @return 返回rcl_ret_t类型的结果，表示操作成功或失败
 */
rcl_ret_t rcl_guard_condition_init_from_rmw(
    rcl_guard_condition_t *guard_condition,
    const rmw_guard_condition_t *rmw_guard_condition,
    rcl_context_t *context,
    const rcl_guard_condition_options_t options) {
  return __rcl_guard_condition_init_from_rmw_impl(
      guard_condition, rmw_guard_condition, context, options);
}

/**
 * @brief 销毁一个rcl_guard_condition_t结构体实例
 *
 * @param[in,out] guard_condition 指向待销毁的rcl_guard_condition_t结构体指针
 * @return 返回rcl_ret_t类型的结果，表示操作成功或失败
 */
rcl_ret_t rcl_guard_condition_fini(rcl_guard_condition_t *guard_condition) {
  // 执行参数验证
  RCL_CHECK_ARGUMENT_FOR_NULL(guard_condition, RCL_RET_INVALID_ARGUMENT);
  rcl_ret_t result = RCL_RET_OK;
  if (guard_condition->impl) {
    // 假设分配器是有效的，因为它在rcl_guard_condition_init()中被检查过
    rcl_allocator_t allocator = guard_condition->impl->options.allocator;
    if (guard_condition->impl->rmw_handle && guard_condition->impl->allocated_rmw_guard_condition) {
      if (rmw_destroy_guard_condition(guard_condition->impl->rmw_handle) != RMW_RET_OK) {
        RCL_SET_ERROR_MSG(rmw_get_error_string().str);
        result = RCL_RET_ERROR;
      }
    }
    allocator.deallocate(guard_condition->impl, allocator.state);
    guard_condition->impl = NULL;
  }
  return result;
}

/**
 * @brief 获取默认的守护条件选项
 *
 * @return 返回默认的守护条件选项
 */
rcl_guard_condition_options_t rcl_guard_condition_get_default_options() {
  // !!! 确保这些默认值的更改反映在头文件文档字符串中
  static rcl_guard_condition_options_t default_options;     // 定义默认的守护条件选项
  default_options.allocator = rcl_get_default_allocator();  // 设置默认分配器
  return default_options;                                   // 返回默认选项
}

/**
 * @brief 触发守护条件
 *
 * @param guard_condition 指向守护条件的指针
 * @return 返回操作结果，成功返回RCL_RET_OK，否则返回相应错误代码
 */
rcl_ret_t rcl_trigger_guard_condition(rcl_guard_condition_t *guard_condition) {
  const rcl_guard_condition_options_t *options =
      rcl_guard_condition_get_options(guard_condition);  // 获取守护条件选项
  if (!options) {
    return RCL_RET_INVALID_ARGUMENT;                     // 错误已设置
  }
  // 触发守护条件
  if (rmw_trigger_guard_condition(guard_condition->impl->rmw_handle) != RMW_RET_OK) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    return RCL_RET_ERROR;
  }
  return RCL_RET_OK;
}

/**
 * @brief 获取守护条件的选项
 *
 * @param guard_condition 指向守护条件的指针
 * @return 返回守护条件选项的指针，如果出错则返回NULL
 */
const rcl_guard_condition_options_t *rcl_guard_condition_get_options(
    const rcl_guard_condition_t *guard_condition) {
  // 执行参数验证
  RCL_CHECK_ARGUMENT_FOR_NULL(guard_condition, NULL);
  RCL_CHECK_FOR_NULL_WITH_MSG(
      guard_condition->impl, "guard condition implementation is invalid", return NULL);
  return &guard_condition->impl->options;
}

/**
 * @brief 获取守护条件的rmw句柄
 *
 * @param guard_condition 指向守护条件的指针
 * @return 返回守护条件的rmw句柄，如果出错则返回NULL
 */
rmw_guard_condition_t *rcl_guard_condition_get_rmw_handle(
    const rcl_guard_condition_t *guard_condition) {
  const rcl_guard_condition_options_t *options =
      rcl_guard_condition_get_options(guard_condition);  // 获取守护条件选项
  if (!options) {
    return NULL;                                         // 错误已设置
  }
  return guard_condition->impl->rmw_handle;
}

#ifdef __cplusplus
}
#endif
