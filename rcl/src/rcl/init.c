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

#include "rcl/init.h"

#include "./arguments_impl.h"
#include "./common.h"
#include "./context_impl.h"
#include "./init_options_impl.h"
#include "rcl/arguments.h"
#include "rcl/domain_id.h"
#include "rcl/error_handling.h"
#include "rcl/localhost.h"
#include "rcl/logging.h"
#include "rcl/security.h"
#include "rcl/validate_enclave_name.h"
#include "rcutils/logging_macros.h"
#include "rcutils/stdatomic_helper.h"
#include "rcutils/strdup.h"
#include "rmw/error_handling.h"
#include "tracetools/tracetools.h"

// 原子变量，用于生成唯一ID
static atomic_uint_least64_t __rcl_next_unique_id = ATOMIC_VAR_INIT(1);

/**
 * @brief 初始化ROS客户端库
 *
 * @param[in] argc 参数个数
 * @param[in] argv 参数值数组
 * @param[in] options 初始化选项
 * @param[out] context 初始化后的上下文
 * @return 返回初始化结果状态码
 */
rcl_ret_t rcl_init(
    int argc, char const *const *argv, const rcl_init_options_t *options, rcl_context_t *context) {
  // 初始化失败时返回的错误码
  rcl_ret_t fail_ret = RCL_RET_ERROR;

  // 检查参数个数是否大于0
  if (argc > 0) {
    // 检查参数值数组是否为空
    RCL_CHECK_ARGUMENT_FOR_NULL(argv, RCL_RET_INVALID_ARGUMENT);
    // 遍历参数值数组，检查每个参数值是否为空
    for (int i = 0; i < argc; ++i) {
      RCL_CHECK_ARGUMENT_FOR_NULL(argv[i], RCL_RET_INVALID_ARGUMENT);
    }
  } else {
    // 如果参数个数小于等于0，但参数值数组不为空，则返回无效参数错误
    if (NULL != argv) {
      RCL_SET_ERROR_MSG("argc is <= 0, but argv is not NULL");
      return RCL_RET_INVALID_ARGUMENT;
    }
  }
  // 检查初始化选项是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(options, RCL_RET_INVALID_ARGUMENT);
  // 检查初始化选项的实现是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(options->impl, RCL_RET_INVALID_ARGUMENT);
  // 获取分配器
  rcl_allocator_t allocator = options->impl->allocator;
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR(&allocator, return RCL_RET_INVALID_ARGUMENT);
  // 检查上下文是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(context, RCL_RET_INVALID_ARGUMENT);

  RCUTILS_LOG_DEBUG_NAMED(
      ROS_PACKAGE_NAME, "Initializing ROS client library, for context at address: %p",
      (void *)context);

  // 检查给定的上下文是否已经初始化过
  if (NULL != context->impl) {
    // 注意，这种情况也可能发生在给定的上下文在初始化之前就被使用了
    // 即它在栈上声明但从未定义或初始化为零
    RCL_SET_ERROR_MSG("rcl_init called on an already initialized context");
    return RCL_RET_ALREADY_INIT;
  }

  // 将全局参数初始化为零
  context->global_arguments = rcl_get_zero_initialized_arguments();

  // 为上下文设置实现
  // 使用zero_allocate，以便清理函数稍后不会尝试清理未初始化的部分
  context->impl = allocator.zero_allocate(1, sizeof(rcl_context_impl_t), allocator.state);
  // 检查分配的内存是否为空，并返回错误消息
  RCL_CHECK_FOR_NULL_WITH_MSG(
      context->impl, "failed to allocate memory for context impl", return RCL_RET_BAD_ALLOC);

  // 首先将rmw上下文初始化为零，以便在清理中检查其有效性
  context->impl->rmw_context = rmw_get_zero_initialized_context();

  // 存储分配器
  context->impl->allocator = allocator;

  // Copy the options into the context for future reference.
  rcl_ret_t ret = rcl_init_options_copy(options, &(context->impl->init_options));
  if (RCL_RET_OK != ret) {
    fail_ret = ret;  // error message already set
    goto fail;
  }

  // 将 argc 和 argv 复制到上下文中，如果 argc >= 0
  context->impl->argc = argc;
  context->impl->argv = NULL;

  // 检查 argc 是否不等于 0 且 argv 不为空
  if (0 != argc && argv != NULL) {
    // 为 argv 分配内存
    context->impl->argv = (char **)allocator.zero_allocate(argc, sizeof(char *), allocator.state);

    // 检查分配是否成功，如果失败则返回错误信息并跳转到 fail 标签
    RCL_CHECK_FOR_NULL_WITH_MSG(context->impl->argv, "failed to allocate memory for argv",
                                fail_ret = RCL_RET_BAD_ALLOC;
                                goto fail);

    // 遍历 argv 数组
    int64_t i;
    for (i = 0; i < argc; ++i) {
      // 获取当前参数字符串长度并加 1
      size_t argv_i_length = strlen(argv[i]) + 1;

      // 为当前参数字符串分配内存
      context->impl->argv[i] = (char *)allocator.allocate(argv_i_length, allocator.state);

      // 检查分配是否成功，如果失败则返回错误信息并跳转到 fail 标签
      RCL_CHECK_FOR_NULL_WITH_MSG(context->impl->argv[i],
                                  "failed to allocate memory for string entry in argv",
                                  fail_ret = RCL_RET_BAD_ALLOC;
                                  goto fail);

      // 将原始参数字符串复制到新分配的内存中
      memcpy(context->impl->argv[i], argv[i], argv_i_length);
    }
  }

  // 解析 ROS 特定的参数
  ret = rcl_parse_arguments(argc, argv, allocator, &context->global_arguments);

  // 检查解析是否成功，如果失败则记录错误信息并跳转到 fail 标签
  if (RCL_RET_OK != ret) {
    fail_ret = ret;
    RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Failed to parse global arguments");
    goto fail;
  }

  // 设置实例id。
  uint64_t next_instance_id = rcutils_atomic_fetch_add_uint64_t(&__rcl_next_unique_id, 1);
  if (0 == next_instance_id) {
    // 发生了翻转，这是极不可能发生的情况。
    RCL_SET_ERROR_MSG("unique rcl instance ids exhausted");
    // 回滚以尝试避免下一次调用成功，但这里存在数据竞争。
    rcutils_atomic_store(&__rcl_next_unique_id, -1);
    goto fail;
  }
  // 将下一个实例ID存储到上下文中。
  rcutils_atomic_store((atomic_uint_least64_t *)(&context->instance_id_storage), next_instance_id);
  // 设置rmw_init_options的实例ID。
  context->impl->init_options.impl->rmw_init_options.instance_id = next_instance_id;

  // 获取域ID。
  size_t *domain_id = &context->impl->init_options.impl->rmw_init_options.domain_id;
  if (RCL_DEFAULT_DOMAIN_ID == *domain_id) {
    // 根据环境变量获取实际的域ID。
    ret = rcl_get_default_domain_id(domain_id);
    if (RCL_RET_OK != ret) {
      fail_ret = ret;
      goto fail;
    }
  }

  // 获取仅限本地主机的值。
  rmw_localhost_only_t *localhost_only =
      &context->impl->init_options.impl->rmw_init_options.localhost_only;
  if (RMW_LOCALHOST_ONLY_DEFAULT == *localhost_only) {
    // 如果需要，根据环境变量获取实际的仅限本地主机值。
    ret = rcl_get_localhost_only(localhost_only);
    if (RCL_RET_OK != ret) {
      fail_ret = ret;
      goto fail;
    }
  }

  /**
   * @brief 设置上下文的 enclave 名称，并验证其有效性。
   *
   * @param[in] context 上下文对象，包含全局参数和初始化选项。
   *
   * @return 若成功，则返回 RCL_RET_OK；否则返回相应的错误代码。
   */
  static rcl_ret_t set_and_validate_context_enclave_name(rcl_context_t * context) {
    // 如果全局参数中有 enclave 名称，则将其复制到 rmw_init_options 中
    if (context->global_arguments.impl->enclave) {
      context->impl->init_options.impl->rmw_init_options.enclave =
          rcutils_strdup(context->global_arguments.impl->enclave, context->impl->allocator);
    } else {
      // 否则，使用默认的 enclave 名称（即 "/"）
      context->impl->init_options.impl->rmw_init_options.enclave =
          rcutils_strdup("/", context->impl->allocator);
    }

    // 检查 enclave 名称是否已成功设置
    if (!context->impl->init_options.impl->rmw_init_options.enclave) {
      RCL_SET_ERROR_MSG("failed to set context name");
      fail_ret = RCL_RET_BAD_ALLOC;
      goto fail;
    }

    int validation_result;
    size_t invalid_index;
    // 验证 enclave 名称的有效性
    ret = rcl_validate_enclave_name(
        context->impl->init_options.impl->rmw_init_options.enclave, &validation_result,
        &invalid_index);
    if (RCL_RET_OK != ret) {
      RCL_SET_ERROR_MSG("rcl_validate_enclave_name() failed");
      fail_ret = ret;
      goto fail;
    }
    // 如果 enclave 名称无效，则设置错误消息并返回错误代码
    if (RCL_ENCLAVE_NAME_VALID != validation_result) {
      RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
          "Enclave name is not valid: '%s'. Invalid index: %zu",
          rcl_enclave_name_validation_result_string(validation_result), invalid_index);
      fail_ret = RCL_RET_ERROR;
      goto fail;
    }

    return RCL_RET_OK;

  fail:
    // 处理失败情况，释放资源并返回错误代码
    if (context->impl->init_options.impl->rmw_init_options.enclave) {
      context->impl->allocator.deallocate(
          context->impl->init_options.impl->rmw_init_options.enclave,
          context->impl->allocator.state);
      context->impl->init_options.impl->rmw_init_options.enclave = NULL;
    }
    return fail_ret;
  }

  /**
   * @brief 从环境变量中获取安全选项并初始化上下文
   *
   * @param[in] context 上下文指针，用于存储实现细节和安全选项
   * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
   */
  rmw_security_options_t *security_options =
      // 获取安全选项的引用
      &context->impl->init_options.impl->rmw_init_options.security_options;

  // 从环境变量中获取安全选项
  ret = rcl_get_security_options_from_environment(
      context->impl->init_options.impl->rmw_init_options.enclave, &context->impl->allocator,
      security_options);

  // 检查返回值是否为RCL_RET_OK
  if (RCL_RET_OK != ret) {
    fail_ret = ret;
    goto fail;
  }

  // 初始化rmw_init
  rmw_ret_t rmw_ret = rmw_init(
      &(context->impl->init_options.impl->rmw_init_options), &(context->impl->rmw_context));

  // 检查rmw_init的返回值是否为RMW_RET_OK
  if (RMW_RET_OK != rmw_ret) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    fail_ret = rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
    goto fail;
  }

  // 添加跟踪点
  TRACEPOINT(rcl_init, (const void *)context);

  // 返回成功
  return RCL_RET_OK;

// 失败处理
fail:
  __cleanup_context(context);
  return fail_ret;
}

/**
 * @brief 关闭 ROS 客户端库
 *
 * @param[in] context 指向 rcl_context_t 结构体的指针，用于存储 ROS 客户端库的上下文信息
 * @return rcl_ret_t 返回操作结果状态
 *
 * @details 该函数用于关闭 ROS 客户端库，并释放相关资源。
 */
rcl_ret_t rcl_shutdown(rcl_context_t *context) {
  // 打印调试信息，显示正在关闭的 ROS 客户端库上下文地址
  RCUTILS_LOG_DEBUG_NAMED(
      ROS_PACKAGE_NAME, "Shutting down ROS client library, for context at address: %p",
      (void *)context);

  // 检查 context 参数是否为空，如果为空则返回 RCL_RET_INVALID_ARGUMENT 错误
  RCL_CHECK_ARGUMENT_FOR_NULL(context, RCL_RET_INVALID_ARGUMENT);

  // 检查 context->impl 是否为空，如果为空则表示上下文未初始化，返回 RCL_RET_INVALID_ARGUMENT 错误
  RCL_CHECK_FOR_NULL_WITH_MSG(
      context->impl, "context is zero-initialized", return RCL_RET_INVALID_ARGUMENT);

  // 检查上下文是否有效，如果无效则表示已经调用过 rcl_shutdown，返回 RCL_RET_ALREADY_SHUTDOWN 错误
  if (!rcl_context_is_valid(context)) {
    RCL_SET_ERROR_MSG("rcl_shutdown already called on the given context");
    return RCL_RET_ALREADY_SHUTDOWN;
  }

  // 调用 rmw_shutdown 函数关闭底层 ROS 中间件资源
  rmw_ret_t rmw_ret = rmw_shutdown(&(context->impl->rmw_context));

  // 检查 rmw_shutdown 的返回值，如果不是 RMW_RET_OK，则设置错误信息并转换为 rcl_ret_t 类型返回
  if (RMW_RET_OK != rmw_ret) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
  }

  // 将上下文的 instance_id_storage 设置为 0，表示该上下文已失效
  rcutils_atomic_store((atomic_uint_least64_t *)(&context->instance_id_storage), 0);

  // 返回 RCL_RET_OK 表示操作成功
  return RCL_RET_OK;
}

#ifdef __cplusplus
}
#endif
