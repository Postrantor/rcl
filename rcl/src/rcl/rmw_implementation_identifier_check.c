// Copyright 2016 Open Source Robotics Foundation, Inc.
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

#include "rcl/rmw_implementation_identifier_check.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rcl/allocator.h"
#include "rcl/error_handling.h"
#include "rcl/types.h"
#include "rcutils/env.h"
#include "rcutils/logging_macros.h"
#include "rcutils/strdup.h"
#include "rmw/rmw.h"

// 从SO中提取了这种可移植的"共享库构造函数"方法：
//   http://stackoverflow.com/a/2390626/671658
// MSVC和GCC/Clang的初始化器/终结器示例。
// 2010-2016 Joe Lowe。发布到公共领域。
#if defined(_MSC_VER)
#pragma section(".CRT$XCU", read)
// 定义一个宏，用于在MSVC编译器下创建初始化函数
#define INITIALIZER2_(f, p)                                \
  static void f(void);                                     \
  __declspec(allocate(".CRT$XCU")) void (*f##_)(void) = f; \
  __pragma(comment(linker, "/include:" p #f "_")) static void f(void)
#ifdef _WIN64
// 对于64位Windows系统，使用INITIALIZER2_宏定义初始化函数
#define INITIALIZER(f) INITIALIZER2_(f, "")
#else
// 对于32位Windows系统，使用INITIALIZER2_宏定义初始化函数，并添加一个下划线前缀
#define INITIALIZER(f) INITIALIZER2_(f, "_")
#endif
#else
// 对于非MSVC编译器（如GCC或Clang），定义一个宏，用于创建带有构造函数属性的初始化函数
#define INITIALIZER(f)                              \
  static void f(void) __attribute__((constructor)); \
  static void f(void)
#endif

/**
 * @brief 检查 ROS2 中的 RMW 实现标识符是否匹配。
 *
 * 如果设置了环境变量 RMW_IMPLEMENTATION 或 RCL_ASSERT_RMW_ID_MATCHES，
 * 则检查 `rmw_get_implementation_identifier` 的结果是否匹配。
 *
 * @return 返回 rcl_ret_t 类型的结果，表示检查是否成功。
 */
rcl_ret_t rcl_rmw_implementation_identifier_check(void) {
  // 初始化返回值为 RCL_RET_OK
  rcl_ret_t ret = RCL_RET_OK;
  // 获取默认分配器
  rcl_allocator_t allocator = rcl_get_default_allocator();
  // 期望的 RMW 实现字符串指针
  char* expected_rmw_impl = NULL;
  // 期望的 RMW 实现环境变量指针
  const char* expected_rmw_impl_env = NULL;
  // 获取环境变量错误信息字符串
  const char* get_env_error_str =
      rcutils_get_env(RMW_IMPLEMENTATION_ENV_VAR_NAME, &expected_rmw_impl_env);
  // 如果获取环境变量出错，设置错误消息并返回 RCL_RET_ERROR
  if (NULL != get_env_error_str) {
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Error getting env var '" RCUTILS_STRINGIFY(RMW_IMPLEMENTATION_ENV_VAR_NAME) "': %s\n",
        get_env_error_str);
    return RCL_RET_ERROR;
  }
  // 如果期望的 RMW 实现环境变量非空，复制环境变量以防止被下一个 getenv 调用覆盖
  if (strlen(expected_rmw_impl_env) > 0) {
    expected_rmw_impl = rcutils_strdup(expected_rmw_impl_env, allocator);
    if (!expected_rmw_impl) {
      RCL_SET_ERROR_MSG("allocation failed");
      return RCL_RET_BAD_ALLOC;
    }
  }

  // 断言的 RMW 实现字符串指针
  char* asserted_rmw_impl = NULL;
  // 断言的 RMW 实现环境变量指针
  const char* asserted_rmw_impl_env = NULL;
  // 获取断言的 RMW 实现环境变量错误信息字符串
  get_env_error_str =
      rcutils_get_env(RCL_ASSERT_RMW_ID_MATCHES_ENV_VAR_NAME, &asserted_rmw_impl_env);
  // 如果获取环境变量出错，设置错误消息并跳转到 cleanup 标签
  if (NULL != get_env_error_str) {
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Error getting env var '" RCUTILS_STRINGIFY(
            RCL_ASSERT_RMW_ID_MATCHES_ENV_VAR_NAME) "': %s\n",
        get_env_error_str);
    ret = RCL_RET_ERROR;
    goto cleanup;
  }
  // 如果断言的 RMW 实现环境变量非空，复制环境变量以防止被下一个 getenv 调用覆盖
  if (strlen(asserted_rmw_impl_env) > 0) {
    asserted_rmw_impl = rcutils_strdup(asserted_rmw_impl_env, allocator);
    if (!asserted_rmw_impl) {
      RCL_SET_ERROR_MSG("allocation failed");
      ret = RCL_RET_BAD_ALLOC;
      goto cleanup;
    }
  }

  // 如果两个环境变量都设置了且不匹配，打印错误并退出
  if (expected_rmw_impl && asserted_rmw_impl && strcmp(expected_rmw_impl, asserted_rmw_impl) != 0) {
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Values of RMW_IMPLEMENTATION ('%s') and RCL_ASSERT_RMW_ID_MATCHES ('%s') environment "
        "variables do not match, exiting with %d.",
        expected_rmw_impl, asserted_rmw_impl, RCL_RET_ERROR);
    ret = RCL_RET_ERROR;
    goto cleanup;
  }

  // 折叠 expected_rmw_impl 和 asserted_rmw_impl 变量，以便从现在开始只需要使用 expected_rmw_impl
  if (expected_rmw_impl && asserted_rmw_impl) {
    // 此时字符串必须相等
    // 不再需要 asserted_rmw_impl，释放内存
    allocator.deallocate(asserted_rmw_impl, allocator.state);
    asserted_rmw_impl = NULL;
  } else {
    // 设置一个或没有设置
    // 如果 asserted_rmw_impl 有内容，将其移动到 expected_rmw_impl
    if (asserted_rmw_impl) {
      expected_rmw_impl = asserted_rmw_impl;
      asserted_rmw_impl = NULL;
    }
  }

  /**
   * @brief 检查预期的RMW实现与实际的RMW实现是否匹配，如果不匹配则设置错误信息并退出。
   *
   * @param[in] expected_rmw_impl 预期的RMW实现标识符。
   *
   * @return 返回rcl_ret_t类型的结果。RCL_RET_OK表示成功，其他值表示失败。
   */
  // 如果设置了任一环境变量且不匹配，打印错误并退出
  if (expected_rmw_impl) {
    // 获取实际的RMW实现标识符
    const char* actual_rmw_impl_id = rmw_get_implementation_identifier();
    // 获取当前的错误信息
    const rcutils_error_string_t rmw_error_msg = rcl_get_error_string();
    // 重置错误信息
    rcl_reset_error();

    // 如果实际的RMW实现标识符为空，则设置错误信息并返回RCL_RET_ERROR
    if (!actual_rmw_impl_id) {
      RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
          "Error getting RMW implementation identifier / RMW implementation not installed "
          "(expected identifier of '%s'), with error message '%s', exiting with %d.",
          expected_rmw_impl, rmw_error_msg.str, RCL_RET_ERROR);
      ret = RCL_RET_ERROR;
      goto cleanup;
    }

    // 如果实际的RMW实现标识符与预期的RMW实现标识符不匹配，则设置错误信息并返回RCL_RET_MISMATCHED_RMW_ID
    if (strcmp(actual_rmw_impl_id, expected_rmw_impl) != 0) {
      RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
          "Expected RMW implementation identifier of '%s' but instead found '%s', exiting with %d.",
          expected_rmw_impl, actual_rmw_impl_id, RCL_RET_MISMATCHED_RMW_ID);
      ret = RCL_RET_MISMATCHED_RMW_ID;
      goto cleanup;
    }
  }

  // 如果预期的RMW实现与实际的RMW实现匹配，则返回RCL_RET_OK
  ret = RCL_RET_OK;
// 跳转到此处进行清理
cleanup:
  allocator.deallocate(expected_rmw_impl, allocator.state);
  allocator.deallocate(asserted_rmw_impl, allocator.state);
  return ret;
}

/**
 * @brief 初始化函数，用于检查 ROS2 中的 rcl 和 rmw 实现是否匹配。
 *
 * @note 如果实现不匹配，此函数将记录错误并退出程序。
 */
INITIALIZER(initialize) {
  // 调用 rcl_rmw_implementation_identifier_check 函数以检查 rcl 和 rmw 实现是否匹配
  rcl_ret_t ret = rcl_rmw_implementation_identifier_check();

  // 判断返回值是否为 RCL_RET_OK，即 rcl 和 rmw 实现是否匹配
  if (ret != RCL_RET_OK) {
    // 如果不匹配，使用 RCUTILS_LOG_ERROR_NAMED 记录错误信息
    RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "%s\n", rcl_get_error_string().str);

    // 退出程序，并返回错误代码
    exit(ret);
  }
}

#ifdef __cplusplus
}
#endif
