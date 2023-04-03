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

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/context.h"

#include <stdbool.h>

#include "./common.h"
#include "./context_impl.h"
#include "rcutils/stdatomic_helper.h"

/**
 * @brief 获取一个零初始化的rcl_context_t实例
 *
 * @return 返回一个零初始化的rcl_context_t实例
 */
rcl_context_t rcl_get_zero_initialized_context(void) {
  // 静态初始化一个rcl_context_t结构体
  static rcl_context_t context = {
      .impl = NULL,
      .instance_id_storage = {0},
  };
  // 这不是constexpr，所以不能在结构体初始化中
  context.global_arguments = rcl_get_zero_initialized_arguments();
  // 确保关于静态存储的假设
  static_assert(
      sizeof(context.instance_id_storage) >= sizeof(atomic_uint_least64_t),
      "expected rcl_context_t's instance id storage to be >= size of atomic_uint_least64_t");
  // 初始化原子变量
  atomic_init((atomic_uint_least64_t *)(&context.instance_id_storage), 0);
  return context;
}

// 参见`rcl_init()`来初始化上下文

/**
 * @brief 结束并清理rcl_context_t实例
 *
 * @param[in] context 要清理的rcl_context_t指针
 * @return 返回rcl_ret_t类型的结果
 */
rcl_ret_t rcl_context_fini(rcl_context_t *context) {
  RCL_CHECK_ARGUMENT_FOR_NULL(context, RCL_RET_INVALID_ARGUMENT);
  if (!context->impl) {
    // Context是零初始化的
    return RCL_RET_OK;
  }
  if (rcl_context_is_valid(context)) {
    RCL_SET_ERROR_MSG("rcl_shutdown() not called on the given context");
    return RCL_RET_INVALID_ARGUMENT;
  }
  RCL_CHECK_ALLOCATOR_WITH_MSG(
      &(context->impl->allocator), "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  return __cleanup_context(context);
}

// 参见`rcl_shutdown()`来使上下文无效

/**
 * @brief 获取rcl_context_t实例的初始化选项
 *
 * @param[in] context 要获取初始化选项的rcl_context_t指针
 * @return 返回一个指向rcl_init_options_t类型的常量指针
 */
const rcl_init_options_t *rcl_context_get_init_options(const rcl_context_t *context) {
  RCL_CHECK_ARGUMENT_FOR_NULL(context, NULL);
  RCL_CHECK_FOR_NULL_WITH_MSG(context->impl, "context is zero-initialized", return NULL);
  return &(context->impl->init_options);
}

/**
 * @file
 * @brief 本文件包含了与 rcl_context_t 相关的函数实现。
 */

/**
 * @brief 获取给定上下文的实例 ID。
 *
 * @param[in] context 指向 rcl_context_t 结构体的指针。
 * @return 返回给定上下文的实例 ID，如果参数为 NULL，则返回 0。
 */
rcl_context_instance_id_t rcl_context_get_instance_id(const rcl_context_t *context) {
  // 检查 context 参数是否为 NULL，如果是则返回 0
  RCL_CHECK_ARGUMENT_FOR_NULL(context, 0);
  // 从 context 中加载并返回 instance_id_storage 的原子值
  return rcutils_atomic_load_uint64_t((atomic_uint_least64_t *)(&context->instance_id_storage));
}

/**
 * @brief 获取给定上下文的域 ID。
 *
 * @param[in] context 指向 rcl_context_t 结构体的指针。
 * @param[out] domain_id 存储域 ID 的指针。
 * @return 返回操作结果，成功时返回 RCL_RET_OK，否则返回相应的错误代码。
 */
rcl_ret_t rcl_context_get_domain_id(rcl_context_t *context, size_t *domain_id) {
  // 检查 context 是否有效，无效时返回 RCL_RET_INVALID_ARGUMENT
  if (!rcl_context_is_valid(context)) {
    return RCL_RET_INVALID_ARGUMENT;
  }
  // 检查 domain_id 参数是否为 NULL，如果是则返回 RCL_RET_INVALID_ARGUMENT
  RCL_CHECK_ARGUMENT_FOR_NULL(domain_id, RCL_RET_INVALID_ARGUMENT);
  // 将 context 的实际域 ID 赋值给 domain_id
  *domain_id = context->impl->rmw_context.actual_domain_id;
  // 返回操作成功
  return RCL_RET_OK;
}

/**
 * @brief 检查给定的 rcl_context_t 是否有效。
 *
 * @param[in] context 指向 rcl_context_t 结构体的指针。
 * @return 如果 context 有效，则返回 true，否则返回 false。
 */
bool rcl_context_is_valid(const rcl_context_t *context) {
  // 检查 context 参数是否为 NULL，如果是则返回 false
  RCL_CHECK_ARGUMENT_FOR_NULL(context, false);
  // 调用 rcl_context_get_instance_id 函数检查 context 是否有效
  return 0 != rcl_context_get_instance_id(context);
}

/**
 * @brief 获取给定上下文的 rmw_context_t 指针。
 *
 * @param[in] context 指向 rcl_context_t 结构体的指针。
 * @return 返回指向 rmw_context_t 的指针，如果参数无效或 context 未初始化，则返回 NULL。
 */
rmw_context_t *rcl_context_get_rmw_context(rcl_context_t *context) {
  // 检查 context 参数是否为 NULL，如果是则返回 NULL
  RCL_CHECK_ARGUMENT_FOR_NULL(context, NULL);
  // 检查 context 的 impl 是否为 NULL，如果是则返回 NULL 并输出错误信息
  RCL_CHECK_FOR_NULL_WITH_MSG(context->impl, "context is zero-initialized", return NULL);
  // 返回 context 的 rmw_context 指针
  return &(context->impl->rmw_context);
}

/**
 * @brief 清理上下文函数
 *
 * @param[in] context 指向要清理的rcl_context_t结构体的指针
 * @return 返回rcl_ret_t类型的结果，表示清理操作是否成功
 */
rcl_ret_t __cleanup_context(rcl_context_t *context) {
  // 初始化返回值为RCL_RET_OK（成功）
  rcl_ret_t ret = RCL_RET_OK;

  // 将实例ID重置为0，表示“无效”（本应已经为0，但这是防御性的）
  rcutils_atomic_store((atomic_uint_least64_t *)(&context->instance_id_storage), 0);

  // 如果global_arguments已初始化，则进行清理
  if (NULL != context->global_arguments.impl) {
    // 清理global_arguments
    ret = rcl_arguments_fini(&(context->global_arguments));
    // 如果清理失败，输出错误信息并重置错误
    if (RCL_RET_OK != ret) {
      RCUTILS_SAFE_FWRITE_TO_STDERR("[rcl|context.c:" RCUTILS_STRINGIFY(
          __LINE__) "] failed to finalize global arguments while cleaning up context, memory may "
                    "be "
                    "leaked: ");
      RCUTILS_SAFE_FWRITE_TO_STDERR(rcl_get_error_string().str);
      RCUTILS_SAFE_FWRITE_TO_STDERR("\n");
      rcl_reset_error();
    }
  }

  // 如果impl为null，无需进行其他清理
  if (NULL != context->impl) {
    // 获取分配器以在释放过程中使用
    rcl_allocator_t allocator = context->impl->allocator;

    // 如果init_options有效，则进行清理
    if (NULL != context->impl->init_options.impl) {
      // 清理init_options
      rcl_ret_t init_options_fini_ret = rcl_init_options_fini(&(context->impl->init_options));
      // 如果清理失败，输出错误信息并重置错误
      if (RCL_RET_OK != init_options_fini_ret) {
        if (RCL_RET_OK == ret) {
          ret = init_options_fini_ret;
        }
        RCUTILS_SAFE_FWRITE_TO_STDERR("[rcl|context.c:" RCUTILS_STRINGIFY(
            __LINE__) "] failed to finalize init options while cleaning up context, memory may be "
                      "leaked: ");
        RCUTILS_SAFE_FWRITE_TO_STDERR(rcl_get_error_string().str);
        RCUTILS_SAFE_FWRITE_TO_STDERR("\n");
        rcl_reset_error();
      }
    }

    // 清理rmw_context
    if (NULL != context->impl->rmw_context.implementation_identifier) {
      // 清理rmw_context
      rmw_ret_t rmw_context_fini_ret = rmw_context_fini(&(context->impl->rmw_context));
      // 如果清理失败，输出错误信息并重置错误
      if (RMW_RET_OK != rmw_context_fini_ret) {
        if (RCL_RET_OK == ret) {
          ret = rcl_convert_rmw_ret_to_rcl_ret(rmw_context_fini_ret);
        }
        RCUTILS_SAFE_FWRITE_TO_STDERR("[rcl|context.c:" RCUTILS_STRINGIFY(
            __LINE__) "] failed to finalize rmw context while cleaning up context, memory may be "
                      "leaked: ");
        RCUTILS_SAFE_FWRITE_TO_STDERR(rcutils_get_error_string().str);
        RCUTILS_SAFE_FWRITE_TO_STDERR("\n");
        rcutils_reset_error();
      }
    }

    // 如果argv有效，则进行清理
    if (NULL != context->impl->argv) {
      int64_t i;
      // 释放argv中的每个元素
      for (i = 0; i < context->impl->argc; ++i) {
        if (NULL != context->impl->argv[i]) {
          allocator.deallocate(context->impl->argv[i], allocator.state);
        }
      }
      // 释放argv数组
      allocator.deallocate(context->impl->argv, allocator.state);
    }
    // 释放impl结构体
    allocator.deallocate(context->impl, allocator.state);
  }  // if (NULL != context->impl)

  // 将上下文初始化为零值
  *context = rcl_get_zero_initialized_context();

  // 返回结果
  return ret;
}

#ifdef __cplusplus
}
#endif
