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

#include "rcl/subscription.h"

#include <stdio.h>

#include "./common.h"
#include "./subscription_impl.h"
#include "rcl/error_handling.h"
#include "rcl/node.h"
#include "rcutils/logging_macros.h"
#include "rcutils/strdup.h"
#include "rcutils/types/string_array.h"
#include "rmw/error_handling.h"
#include "rmw/subscription_content_filter_options.h"
#include "rmw/validate_full_topic_name.h"
#include "tracetools/tracetools.h"

/**
 * @brief 获取一个零初始化的订阅器
 *
 * @return rcl_subscription_t 零初始化的订阅器
 */
rcl_subscription_t rcl_get_zero_initialized_subscription() {
  // 定义并初始化一个静态空订阅器
  static rcl_subscription_t null_subscription = {0};
  // 返回空订阅器
  return null_subscription;
}

/**
 * @brief 初始化订阅器
 *
 * @param[in,out] subscription 指向待初始化的订阅器的指针
 * @param[in] node 指向关联节点的指针
 * @param[in] type_support 消息类型支持
 * @param[in] topic_name 主题名称
 * @param[in] options 订阅器选项
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_subscription_init(
    rcl_subscription_t *subscription,
    const rcl_node_t *node,
    const rosidl_message_type_support_t *type_support,
    const char *topic_name,
    const rcl_subscription_options_t *options) {
  // 初始化失败返回值
  rcl_ret_t fail_ret = RCL_RET_ERROR;

  /**
   * @brief 检查选项和分配器，以便在错误中使用分配器
   *
   * @param[in] options 传入的选项指针，不能为空
   * @param[out] RCL_RET_INVALID_ARGUMENT 如果选项为空，则返回无效参数错误
   * @param[in] allocator 分配器指针，不能为空
   * @param[out] RCL_RET_INVALID_ARGUMENT 如果分配器无效，则返回无效参数错误
   * @param[in] subscription 订阅指针，不能为空
   * @param[out] RCL_RET_INVALID_ARGUMENT 如果订阅为空，则返回无效参数错误
   * @param[in] node 节点指针，必须是有效的节点
   * @param[out] RCL_RET_NODE_INVALID 如果节点无效，则返回节点无效错误
   * @param[in] type_support 类型支持指针，不能为空
   * @param[out] RCL_RET_INVALID_ARGUMENT 如果类型支持为空，则返回无效参数错误
   * @param[in] topic_name 主题名称指针，不能为空
   * @param[out] RCL_RET_INVALID_ARGUMENT 如果主题名称为空，则返回无效参数错误
   * @param[out] RCUTILS_LOG_DEBUG_NAMED 初始化订阅时记录调试信息
   * @param[in] subscription->impl 订阅实现指针
   * @param[out] RCL_RET_ALREADY_INIT 如果订阅已经初始化或内存未初始化，则返回已初始化错误
   */
  // 检查选项和分配器，以便在错误中使用分配器
  RCL_CHECK_ARGUMENT_FOR_NULL(options, RCL_RET_INVALID_ARGUMENT);
  rcl_allocator_t *allocator = (rcl_allocator_t *)&options->allocator;
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(subscription, RCL_RET_INVALID_ARGUMENT);
  if (!rcl_node_is_valid(node)) {
    return RCL_RET_NODE_INVALID;  // error already set
  }
  RCL_CHECK_ARGUMENT_FOR_NULL(type_support, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(topic_name, RCL_RET_INVALID_ARGUMENT);
  RCUTILS_LOG_DEBUG_NAMED(
      ROS_PACKAGE_NAME, "Initializing subscription for topic name '%s'", topic_name);
  if (subscription->impl) {
    RCL_SET_ERROR_MSG("subscription already initialized, or memory was uninitialized");
    return RCL_RET_ALREADY_INIT;
  }

  /**
   * @brief 扩展并重新映射给定的主题名称
   *
   * @param[in] node 指向rcl_node_t类型的指针，用于解析主题名称
   * @param[in] topic_name 要扩展和重新映射的原始主题名称
   * @param[in] allocator 分配器，用于分配内存
   * @param[out] remapped_topic_name 存储扩展和重新映射后的主题名称
   * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
   */
  // 扩展并重新映射给定的主题名称
  char *remapped_topic_name = NULL;
  // 调用rcl_node_resolve_name函数来解析主题名称，并将结果存储在remapped_topic_name中
  rcl_ret_t ret =
      rcl_node_resolve_name(node, topic_name, *allocator, false, false, &remapped_topic_name);
  // 判断解析结果是否成功
  if (ret != RCL_RET_OK) {
    // 如果返回值为RCL_RET_TOPIC_NAME_INVALID或RCL_RET_UNKNOWN_SUBSTITUTION，则设置返回值为RCL_RET_TOPIC_NAME_INVALID
    if (ret == RCL_RET_TOPIC_NAME_INVALID || ret == RCL_RET_UNKNOWN_SUBSTITUTION) {
      ret = RCL_RET_TOPIC_NAME_INVALID;
    } else if (
        ret != RCL_RET_BAD_ALLOC) {  // 如果返回值不是RCL_RET_BAD_ALLOC，则设置返回值为RCL_RET_ERROR
      ret = RCL_RET_ERROR;
    }
    // 跳转到cleanup标签进行清理操作
    goto cleanup;
  }
  // 输出调试信息，显示扩展和重新映射后的主题名称
  RCUTILS_LOG_DEBUG_NAMED(
      ROS_PACKAGE_NAME, "Expanded and remapped topic name '%s'", remapped_topic_name);

  /**
   * @brief 为实现结构分配内存
   *
   * @param[in] subscription 指向要初始化的rcl_subscription_t结构体的指针
   * @param[in] allocator 分配器，用于分配内存
   * @return 成功时返回RCL_RET_OK，失败时返回相应的错误代码
   */
  subscription->impl = (rcl_subscription_impl_t *)allocator->zero_allocate(
      1, sizeof(rcl_subscription_impl_t), allocator->state);
  // 检查内存分配是否成功
  RCL_CHECK_FOR_NULL_WITH_MSG(
      subscription->impl, "allocating memory failed", ret = RCL_RET_BAD_ALLOC; goto cleanup);

  /**
   * @brief 填充实现结构
   */
  // rmw_handle
  // TODO(wjwwood): pass allocator once supported in rmw api.
  subscription->impl->rmw_handle = rmw_create_subscription(
      rcl_node_get_rmw_handle(node), type_support, remapped_topic_name, &(options->qos),
      &(options->rmw_subscription_options));
  // 检查rmw_handle是否创建成功
  if (!subscription->impl->rmw_handle) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    goto fail;
  }

  /**
   * @brief 获取实际的QoS，并存储它
   */
  rmw_ret_t rmw_ret = rmw_subscription_get_actual_qos(
      subscription->impl->rmw_handle, &subscription->impl->actual_qos);
  // 检查获取实际QoS是否成功
  if (RMW_RET_OK != rmw_ret) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    goto fail;
  }
  subscription->impl->actual_qos.avoid_ros_namespace_conventions =
      options->qos.avoid_ros_namespace_conventions;

  /**
   * @brief 设置options
   */
  subscription->impl->options = *options;
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Subscription initialized");
  ret = RCL_RET_OK;
  TRACEPOINT(
      rcl_subscription_init, (const void *)subscription, (const void *)node,
      (const void *)subscription->impl->rmw_handle, remapped_topic_name, options->qos.depth);
  goto cleanup;

/**
 * @brief 清理订阅实现并处理错误情况
 *
 * @param[in] subscription->impl 订阅实现指针
 * @param[in] subscription->impl->rmw_handle RMW订阅句柄
 * @param[out] rmw_fail_ret 销毁订阅时的RMW返回值
 * @param[out] RCUTILS_SAFE_FWRITE_TO_STDERR 如果销毁订阅失败，将错误信息写入标准错误输出
 * @param[out] ret 清理订阅选项时的RCL返回值
 * @param[out] RCUTILS_SAFE_FWRITE_TO_STDERR 如果清理订阅选项失败，将错误信息写入标准错误输出
 * @param[out] allocator->deallocate 使用分配器释放订阅实现内存
 * @param[out] subscription->impl 将订阅实现指针设置为NULL
 * @param[out] ret 返回失败的RCL返回值
 */
fail:
  if (subscription->impl) {
    if (subscription->impl->rmw_handle) {
      rmw_ret_t rmw_fail_ret =
          rmw_destroy_subscription(rcl_node_get_rmw_handle(node), subscription->impl->rmw_handle);
      if (RMW_RET_OK != rmw_fail_ret) {
        RCUTILS_SAFE_FWRITE_TO_STDERR(rmw_get_error_string().str);
        RCUTILS_SAFE_FWRITE_TO_STDERR("\n");
      }
    }

    ret = rcl_subscription_options_fini(&subscription->impl->options);
    if (RCL_RET_OK != ret) {
      RCUTILS_SAFE_FWRITE_TO_STDERR(rmw_get_error_string().str);
      RCUTILS_SAFE_FWRITE_TO_STDERR("\n");
    }

    allocator->deallocate(subscription->impl, allocator->state);
    subscription->impl = NULL;
  }
  ret = fail_ret;

cleanup:  // 跳转到清理
  allocator->deallocate(remapped_topic_name, allocator->state);
  return ret;
}

/**
 * @brief 销毁一个rcl订阅者并释放相关资源
 *
 * @param[in,out] subscription 指向要销毁的rcl订阅者的指针
 * @param[in] node 与订阅者关联的rcl节点
 * @return 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_subscription_fini(rcl_subscription_t *subscription, rcl_node_t *node) {
  // 可以返回以下错误类型
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_SUBSCRIPTION_INVALID);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_NODE_INVALID);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_ERROR);

  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Finalizing subscription");

  // 初始化结果为RCL_RET_OK
  rcl_ret_t result = RCL_RET_OK;

  // 检查subscription参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(subscription, RCL_RET_SUBSCRIPTION_INVALID);

  // 检查节点是否有效
  if (!rcl_node_is_valid_except_context(node)) {
    return RCL_RET_NODE_INVALID;  // error already set
  }

  // 如果订阅者实现不为空
  if (subscription->impl) {
    // 获取分配器
    rcl_allocator_t allocator = subscription->impl->options.allocator;

    // 获取rmw节点句柄
    rmw_node_t *rmw_node = rcl_node_get_rmw_handle(node);

    // 如果rmw节点为空，返回错误
    if (!rmw_node) {
      return RCL_RET_INVALID_ARGUMENT;
    }

    // 销毁订阅者并获取结果
    rmw_ret_t ret = rmw_destroy_subscription(rmw_node, subscription->impl->rmw_handle);

    // 如果销毁失败，设置错误信息
    if (ret != RMW_RET_OK) {
      RCL_SET_ERROR_MSG(rmw_get_error_string().str);
      result = RCL_RET_ERROR;
    }

    // 销毁订阅者选项并获取结果
    rcl_ret_t rcl_ret = rcl_subscription_options_fini(&subscription->impl->options);

    // 如果销毁失败，输出错误信息
    if (RCL_RET_OK != rcl_ret) {
      RCUTILS_SAFE_FWRITE_TO_STDERR(rcl_get_error_string().str);
      RCUTILS_SAFE_FWRITE_TO_STDERR("\n");
      result = RCL_RET_ERROR;
    }

    // 释放订阅者实现内存
    allocator.deallocate(subscription->impl, allocator.state);

    // 将订阅者实现指针置空
    subscription->impl = NULL;
  }

  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Subscription finalized");

  // 返回结果
  return result;
}

/**
 * @brief 获取默认的订阅选项
 *
 * 该函数返回一个包含默认值的 rcl_subscription_options_t 结构体实例。
 * 如果需要修改这些默认值，请确保在头文件的文档字符串中进行相应的更新。
 *
 * @return 返回一个 rcl_subscription_options_t 结构体，其中包含默认的订阅选项
 */
rcl_subscription_options_t rcl_subscription_get_default_options() {
  // 静态变量 default_options 用于存储默认的订阅选项
  static rcl_subscription_options_t default_options;

  // 必须在声明之后设置这些值，因为它们不是编译时常量
  default_options.qos = rmw_qos_profile_default;            // 设置默认的 QoS 配置
  default_options.allocator = rcl_get_default_allocator();  // 设置默认的内存分配器
  default_options.rmw_subscription_options =
      rmw_get_default_subscription_options();               // 设置默认的 RMW 订阅选项

  // 通过环境变量加载 LoanedMessage 的禁用标志
  bool disable_loaned_message = false;
  rcl_ret_t ret = rcl_get_disable_loaned_message(&disable_loaned_message);
  if (ret == RCL_RET_OK) {
    // 如果成功获取到禁用标志，则将其设置为默认选项的对应值
    default_options.disable_loaned_message = disable_loaned_message;
  } else {
    // 如果获取失败，则输出错误信息，并将默认选项的对应值设置为 false
    RCUTILS_SAFE_FWRITE_TO_STDERR("Failed to get disable_loaned_message: ");
    RCUTILS_SAFE_FWRITE_TO_STDERR(rcl_get_error_string().str);
    rcl_reset_error();
    default_options.disable_loaned_message = false;
  }

  // 返回包含默认值的订阅选项结构体
  return default_options;
}

/**
 * @brief 终止并清理 rcl_subscription_options_t 结构体中的内容
 *
 * @param[in,out] option 指向待终止的 rcl_subscription_options_t 结构体指针
 * @return 返回 rcl_ret_t 类型的结果，成功返回 RCL_RET_OK，否则返回相应的错误代码
 */
rcl_ret_t rcl_subscription_options_fini(rcl_subscription_options_t *option) {
  // 检查传入的 option 参数是否为空，如果为空则返回 RCL_RET_INVALID_ARGUMENT 错误
  RCL_CHECK_ARGUMENT_FOR_NULL(option, RCL_RET_INVALID_ARGUMENT);

  // 获取分配器
  const rcl_allocator_t *allocator = &option->allocator;

  // 检查分配器是否有效，如果无效则返回 RCL_RET_INVALID_ARGUMENT 错误
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);

  // 如果 content_filter_options 不为空，则进行清理操作
  if (option->rmw_subscription_options.content_filter_options) {
    // 调用 rmw_subscription_content_filter_options_fini 函数清理 content_filter_options
    rmw_ret_t ret = rmw_subscription_content_filter_options_fini(
        option->rmw_subscription_options.content_filter_options, allocator);

    // 如果清理失败，输出错误信息并将 rmw_ret_t 类型的错误转换为 rcl_ret_t 类型后返回
    if (RCUTILS_RET_OK != ret) {
      RCUTILS_SAFE_FWRITE_TO_STDERR("Failed to fini content filter options.\n");
      return rcl_convert_rmw_ret_to_rcl_ret(ret);
    }

    // 使用分配器释放 content_filter_options 的内存
    allocator->deallocate(
        option->rmw_subscription_options.content_filter_options, allocator->state);

    // 将 content_filter_options 设置为 NULL
    option->rmw_subscription_options.content_filter_options = NULL;
  }

  // 返回 RCL_RET_OK 表示成功完成操作
  return RCL_RET_OK;
}

/**
 * @brief 设置订阅选项的内容过滤器选项
 *
 * @param[in] filter_expression 过滤表达式，不能为空
 * @param[in] expression_parameters_argc 表达式参数数量，不能超过100
 * @param[in] expression_parameter_argv 表达式参数值数组，可以为空
 * @param[out] options 订阅选项指针，用于存储设置的内容过滤器选项，不能为空
 * @return rcl_ret_t 返回操作结果状态码
 */
rcl_ret_t rcl_subscription_options_set_content_filter_options(
    const char *filter_expression,
    size_t expression_parameters_argc,
    const char *expression_parameter_argv[],
    rcl_subscription_options_t *options) {
  // 检查过滤表达式是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(filter_expression, RCL_RET_INVALID_ARGUMENT);
  // 检查表达式参数数量是否超过100
  if (expression_parameters_argc > 100) {
    RCL_SET_ERROR_MSG("The maximum of expression parameters argument number is 100");
    return RCL_RET_INVALID_ARGUMENT;
  }
  // 检查订阅选项是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(options, RCL_RET_INVALID_ARGUMENT);
  // 获取分配器
  const rcl_allocator_t *allocator = &options->allocator;
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);

  rcl_ret_t ret;
  rmw_ret_t rmw_ret;
  // 获取原始内容过滤器选项
  rmw_subscription_content_filter_options_t *original_content_filter_options =
      options->rmw_subscription_options.content_filter_options;
  // 初始化内容过滤器选项备份
  rmw_subscription_content_filter_options_t content_filter_options_backup =
      rmw_get_zero_initialized_content_filter_options();

  if (original_content_filter_options) {
    // 如果原始内容过滤器选项存在，进行备份，以便在失败时恢复数据
    rmw_ret = rmw_subscription_content_filter_options_copy(
        original_content_filter_options, allocator, &content_filter_options_backup);
    if (rmw_ret != RMW_RET_OK) {
      return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
    }
  } else {
    // 分配内存并初始化内容过滤器选项
    options->rmw_subscription_options.content_filter_options =
        allocator->allocate(sizeof(rmw_subscription_content_filter_options_t), allocator->state);
    if (!options->rmw_subscription_options.content_filter_options) {
      RCL_SET_ERROR_MSG("failed to allocate memory");
      return RCL_RET_BAD_ALLOC;
    }
    *options->rmw_subscription_options.content_filter_options =
        rmw_get_zero_initialized_content_filter_options();
  }

  // 设置内容过滤器选项
  rmw_ret = rmw_subscription_content_filter_options_set(
      filter_expression, expression_parameters_argc, expression_parameter_argv, allocator,
      options->rmw_subscription_options.content_filter_options);

  if (rmw_ret != RMW_RET_OK) {
    ret = rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
    goto failed;
  }

  // 清理内容过滤器选项备份
  rmw_ret = rmw_subscription_content_filter_options_fini(&content_filter_options_backup, allocator);
  if (rmw_ret != RMW_RET_OK) {
    return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
  }

  return RMW_RET_OK;

failed:

  if (original_content_filter_options == NULL) {
    if (options->rmw_subscription_options.content_filter_options) {
      // 清理失败时分配的内容过滤器选项
      rmw_ret = rmw_subscription_content_filter_options_fini(
          options->rmw_subscription_options.content_filter_options, allocator);

      if (rmw_ret != RMW_RET_OK) {
        return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
      }

      // 释放内存
      allocator->deallocate(
          options->rmw_subscription_options.content_filter_options, allocator->state);
      options->rmw_subscription_options.content_filter_options = NULL;
    }
  } else {
    // 恢复原始内容过滤器选项
    rmw_ret = rmw_subscription_content_filter_options_copy(
        &content_filter_options_backup, allocator,
        options->rmw_subscription_options.content_filter_options);
    if (rmw_ret != RMW_RET_OK) {
      return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
    }

    // 清理内容过滤器选项备份
    rmw_ret =
        rmw_subscription_content_filter_options_fini(&content_filter_options_backup, allocator);
    if (rmw_ret != RMW_RET_OK) {
      return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
    }
  }

  return ret;
}

/**
 * @brief 获取一个初始化为零的订阅内容过滤器选项结构体
 *
 * @return 返回一个初始化为零的 rcl_subscription_content_filter_options_t 结构体实例
 */
rcl_subscription_content_filter_options_t
rcl_get_zero_initialized_subscription_content_filter_options() {
  // 使用 rmw 的函数获取一个初始化为零的内容过滤器选项，并将其赋值给 rcl 结构体
  return (const rcl_subscription_content_filter_options_t){
      .rmw_subscription_content_filter_options =
          rmw_get_zero_initialized_content_filter_options()};  // NOLINT(readability/braces): false
                                                               // positive
}

/**
 * @brief 初始化订阅内容过滤器选项
 *
 * @param[in] subscription 指向有效的 rcl_subscription_t 结构体的指针
 * @param[in] filter_expression 过滤表达式字符串
 * @param[in] expression_parameters_argc 表达式参数数量
 * @param[in] expression_parameter_argv 表达式参数字符串数组
 * @param[out] options 指向 rcl_subscription_content_filter_options_t
 * 结构体的指针，用于存储初始化后的选项
 * @return 返回 RCL_RET_OK（成功）或相应的错误代码
 */
rcl_ret_t rcl_subscription_content_filter_options_init(
    const rcl_subscription_t *subscription,
    const char *filter_expression,
    size_t expression_parameters_argc,
    const char *expression_parameter_argv[],
    rcl_subscription_content_filter_options_t *options) {
  // 检查订阅是否有效
  if (!rcl_subscription_is_valid(subscription)) {
    return RCL_RET_SUBSCRIPTION_INVALID;
  }
  // 检查 options 参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(options, RCL_RET_INVALID_ARGUMENT);
  // 获取订阅的分配器
  const rcl_allocator_t *allocator = &subscription->impl->options.allocator;
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  // 检查表达式参数数量是否超过最大限制（100）
  if (expression_parameters_argc > 100) {
    RCL_SET_ERROR_MSG("The maximum of expression parameters argument number is 100");
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 调用 rmw 函数初始化订阅内容过滤器选项
  rmw_ret_t rmw_ret = rmw_subscription_content_filter_options_init(
      filter_expression, expression_parameters_argc, expression_parameter_argv, allocator,
      &options->rmw_subscription_content_filter_options);

  // 将 rmw 返回的错误代码转换为 rcl 错误代码并返回
  return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
}

/**
 * @brief 设置订阅的内容过滤选项
 *
 * @param[in] subscription 订阅对象指针
 * @param[in] filter_expression 过滤表达式字符串
 * @param[in] expression_parameters_argc 表达式参数数量
 * @param[in] expression_parameter_argv 表达式参数值数组
 * @param[out] options 内容过滤选项结构体指针
 * @return rcl_ret_t 返回操作结果状态码
 */
rcl_ret_t rcl_subscription_content_filter_options_set(
    const rcl_subscription_t *subscription,
    const char *filter_expression,
    size_t expression_parameters_argc,
    const char *expression_parameter_argv[],
    rcl_subscription_content_filter_options_t *options) {
  // 检查订阅是否有效
  if (!rcl_subscription_is_valid(subscription)) {
    return RCL_RET_SUBSCRIPTION_INVALID;
  }
  // 检查表达式参数数量是否超过最大限制
  if (expression_parameters_argc > 100) {
    RCL_SET_ERROR_MSG("The maximum of expression parameters argument number is 100");
    return RCL_RET_INVALID_ARGUMENT;
  }
  // 检查options参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(options, RCL_RET_INVALID_ARGUMENT);
  // 获取分配器
  const rcl_allocator_t *allocator = &subscription->impl->options.allocator;
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);

  // 调用底层rmw函数设置内容过滤选项
  rmw_ret_t ret = rmw_subscription_content_filter_options_set(
      filter_expression, expression_parameters_argc, expression_parameter_argv, allocator,
      &options->rmw_subscription_content_filter_options);
  // 转换并返回rcl状态码
  return rcl_convert_rmw_ret_to_rcl_ret(ret);
}

/**
 * @brief 清理订阅的内容过滤选项
 *
 * @param[in] subscription 订阅对象指针
 * @param[out] options 内容过滤选项结构体指针
 * @return rcl_ret_t 返回操作结果状态码
 */
rcl_ret_t rcl_subscription_content_filter_options_fini(
    const rcl_subscription_t *subscription, rcl_subscription_content_filter_options_t *options) {
  // 检查订阅是否有效
  if (!rcl_subscription_is_valid(subscription)) {
    return RCL_RET_SUBSCRIPTION_INVALID;
  }
  // 检查options参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(options, RCL_RET_INVALID_ARGUMENT);
  // 获取分配器
  const rcl_allocator_t *allocator = &subscription->impl->options.allocator;
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);

  // 调用底层rmw函数清理内容过滤选项
  rmw_ret_t ret = rmw_subscription_content_filter_options_fini(
      &options->rmw_subscription_content_filter_options, allocator);

  // 转换并返回rcl状态码
  return rcl_convert_rmw_ret_to_rcl_ret(ret);
}

/**
 * @brief 检查订阅是否启用了内容过滤 (Check if content filtering is enabled for the subscription)
 *
 * @param[in] subscription 指向要检查的 rcl_subscription_t 结构体的指针 (Pointer to the
 * rcl_subscription_t structure to check)
 * @return 如果启用了内容过滤，则返回 true，否则返回 false (Returns true if content filtering is
 * enabled, false otherwise)
 */
bool rcl_subscription_is_cft_enabled(const rcl_subscription_t *subscription) {
  // 验证订阅是否有效 (Validate if the subscription is valid)
  if (!rcl_subscription_is_valid(subscription)) {
    return false;
  }
  // 返回订阅是否启用了内容过滤 (Return whether content filtering is enabled for the subscription)
  return subscription->impl->rmw_handle->is_cft_enabled;
}

/**
 * @brief 设置订阅的内容过滤选项 (Set content filter options for the subscription)
 *
 * @param[in] subscription 指向要设置内容过滤选项的 rcl_subscription_t 结构体的指针 (Pointer to the
 * rcl_subscription_t structure to set content filter options)
 * @param[in] options 指向 rcl_subscription_content_filter_options_t
 * 结构体的指针，包含要设置的内容过滤选项 (Pointer to the rcl_subscription_content_filter_options_t
 * structure containing the content filter options to set)
 * @return 成功时返回 RCL_RET_OK，失败时返回相应的错误代码 (Returns RCL_RET_OK on success,
 * appropriate error code on failure)
 */
rcl_ret_t rcl_subscription_set_content_filter(
    const rcl_subscription_t *subscription,
    const rcl_subscription_content_filter_options_t *options) {
  // 检查订阅是否有效，如果无效则返回错误 (Check if the subscription is valid, return error if not)
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_SUBSCRIPTION_INVALID);
  // 检查参数是否有效，如果无效则返回错误 (Check if the argument is valid, return error if not)
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);

  // 验证订阅是否有效 (Validate if the subscription is valid)
  if (!rcl_subscription_is_valid(subscription)) {
    return RCL_RET_SUBSCRIPTION_INVALID;
  }

  // 检查选项参数是否为空，如果为空则返回错误 (Check if the options argument is null, return error
  // if it is)
  RCL_CHECK_ARGUMENT_FOR_NULL(options, RCL_RET_INVALID_ARGUMENT);
  // 调用 rmw 函数设置内容过滤选项 (Call the rmw function to set content filter options)
  rmw_ret_t ret = rmw_subscription_set_content_filter(
      subscription->impl->rmw_handle, &options->rmw_subscription_content_filter_options);

  // 如果 rmw 函数返回错误，则设置错误消息并转换为 rcl 错误代码 (If the rmw function returns an
  // error, set the error message and convert to rcl error code)
  if (ret != RMW_RET_OK) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    return rcl_convert_rmw_ret_to_rcl_ret(ret);
  }

  // 将选项复制到订阅选项中 (Copy the options into the subscription options)
  const rmw_subscription_content_filter_options_t *content_filter_options =
      &options->rmw_subscription_content_filter_options;
  return rcl_subscription_options_set_content_filter_options(
      content_filter_options->filter_expression, content_filter_options->expression_parameters.size,
      (const char **)content_filter_options->expression_parameters.data,
      &subscription->impl->options);
}

/**
 * @brief 获取订阅者的内容过滤器选项
 *
 * @param[in] subscription 指向有效的rcl_subscription_t结构体的指针
 * @param[out] options 用于存储内容过滤器选项的指针
 * @return 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_subscription_get_content_filter(
    const rcl_subscription_t *subscription, rcl_subscription_content_filter_options_t *options) {
  // 检查是否可以返回RCL_RET_SUBSCRIPTION_INVALID错误
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_SUBSCRIPTION_INVALID);
  // 检查是否可以返回RCL_RET_INVALID_ARGUMENT错误
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);

  // 检查订阅者是否有效
  if (!rcl_subscription_is_valid(subscription)) {
    return RCL_RET_SUBSCRIPTION_INVALID;
  }
  // 检查options参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(options, RCL_RET_INVALID_ARGUMENT);
  // 获取分配器
  rcl_allocator_t *allocator = &subscription->impl->options.allocator;
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);

  // 调用底层RMW实现获取内容过滤器选项
  rmw_ret_t rmw_ret = rmw_subscription_get_content_filter(
      subscription->impl->rmw_handle, allocator, &options->rmw_subscription_content_filter_options);

  // 将RMW返回值转换为RCL返回值并返回
  return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
}

/**
 * @brief 从订阅中获取消息
 *
 * @param[in] subscription 指向有效的rcl_subscription_t结构体的指针
 * @param[out] ros_message 存储接收到的消息的指针
 * @param[out] message_info 存储接收到的消息的元信息的指针，可以为NULL
 * @param[in] allocation 预分配的内存空间，可以为NULL
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_take(
    const rcl_subscription_t *subscription,
    void *ros_message,
    rmw_message_info_t *message_info,
    rmw_subscription_allocation_t *allocation) {
  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Subscription taking message");

  // 检查订阅是否有效
  if (!rcl_subscription_is_valid(subscription)) {
    return RCL_RET_SUBSCRIPTION_INVALID;  // 错误信息已设置
  }

  // 检查ros_message参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(ros_message, RCL_RET_INVALID_ARGUMENT);

  // 如果message_info为NULL，则使用一个占位符，可以丢弃
  rmw_message_info_t dummy_message_info;
  rmw_message_info_t *message_info_local = message_info ? message_info : &dummy_message_info;

  // 初始化message_info_local
  *message_info_local = rmw_get_zero_initialized_message_info();

  // 调用rmw_take_with_info函数
  bool taken = false;
  rmw_ret_t ret = rmw_take_with_info(
      subscription->impl->rmw_handle, ros_message, &taken, message_info_local, allocation);

  // 检查rmw_take_with_info的返回值
  if (ret != RMW_RET_OK) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    return rcl_convert_rmw_ret_to_rcl_ret(ret);
  }

  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(
      ROS_PACKAGE_NAME, "Subscription take succeeded: %s", taken ? "true" : "false");

  // 跟踪消息接收
  TRACEPOINT(rcl_take, (const void *)ros_message);

  // 如果没有获取到消息，则返回失败
  if (!taken) {
    return RCL_RET_SUBSCRIPTION_TAKE_FAILED;
  }

  // 返回成功
  return RCL_RET_OK;
}

/**
 * @brief 从订阅者中获取一定数量的消息序列
 *
 * @param[in] subscription 指向有效的rcl_subscription_t结构体的指针
 * @param[in] count 要获取的消息数量
 * @param[out] message_sequence 存储获取到的消息序列的指针
 * @param[out] message_info_sequence 存储获取到的消息信息序列的指针
 * @param[in] allocation 指向rmw_subscription_allocation_t结构体的指针，用于分配内存
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_take_sequence(
    const rcl_subscription_t *subscription,
    size_t count,
    rmw_message_sequence_t *message_sequence,
    rmw_message_info_sequence_t *message_info_sequence,
    rmw_subscription_allocation_t *allocation) {
  // 打印调试信息，显示要获取的消息数量
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Subscription taking %zu messages", count);

  // 检查订阅者是否有效
  if (!rcl_subscription_is_valid(subscription)) {
    return RCL_RET_SUBSCRIPTION_INVALID;  // 错误信息已设置
  }

  // 检查message_sequence和message_info_sequence参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(message_sequence, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(message_info_sequence, RCL_RET_INVALID_ARGUMENT);

  // 检查消息序列容量是否足够
  if (message_sequence->capacity < count) {
    RCL_SET_ERROR_MSG("Insufficient message sequence capacity for requested count");
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 检查消息信息序列容量是否足够
  if (message_info_sequence->capacity < count) {
    RCL_SET_ERROR_MSG("Insufficient message info sequence capacity for requested count");
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 将大小设置为零，表示没有有效消息
  message_sequence->size = 0u;
  message_info_sequence->size = 0u;

  // 定义已获取的消息数量变量
  size_t taken = 0u;

  // 调用rmw_take_sequence函数获取消息序列和消息信息序列
  rmw_ret_t ret = rmw_take_sequence(
      subscription->impl->rmw_handle, count, message_sequence, message_info_sequence, &taken,
      allocation);

  // 检查rmw_take_sequence函数返回值
  if (ret != RMW_RET_OK) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    return rcl_convert_rmw_ret_to_rcl_ret(ret);
  }

  // 打印调试信息，显示已获取的消息数量
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Subscription took %zu messages", taken);

  // 如果没有获取到任何消息，则返回失败
  if (0u == taken) {
    return RCL_RET_SUBSCRIPTION_TAKE_FAILED;
  }

  // 返回成功
  return RCL_RET_OK;
}

/**
 * @brief 从订阅中获取序列化消息
 *
 * @param[in] subscription 指向有效的rcl_subscription_t结构体的指针
 * @param[out] serialized_message 存储接收到的序列化消息的指针
 * @param[out] message_info 存储接收到的消息信息的指针，可以为NULL
 * @param[in] allocation 指向rmw_subscription_allocation_t结构体的指针，用于分配内存
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_take_serialized_message(
    const rcl_subscription_t *subscription,
    rcl_serialized_message_t *serialized_message,
    rmw_message_info_t *message_info,
    rmw_subscription_allocation_t *allocation) {
  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Subscription taking serialized message");

  // 检查订阅是否有效
  if (!rcl_subscription_is_valid(subscription)) {
    return RCL_RET_SUBSCRIPTION_INVALID;  // 错误已设置
  }

  // 检查serialized_message参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(serialized_message, RCL_RET_INVALID_ARGUMENT);

  // 如果message_info为NULL，使用一个占位符，可以丢弃
  rmw_message_info_t dummy_message_info;
  rmw_message_info_t *message_info_local = message_info ? message_info : &dummy_message_info;

  // 初始化message_info_local
  *message_info_local = rmw_get_zero_initialized_message_info();

  // 调用rmw_take_with_info函数
  bool taken = false;
  rmw_ret_t ret = rmw_take_serialized_message_with_info(
      subscription->impl->rmw_handle, serialized_message, &taken, message_info_local, allocation);

  // 检查rmw_take_serialized_message_with_info的返回值
  if (ret != RMW_RET_OK) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    return rcl_convert_rmw_ret_to_rcl_ret(ret);
  }

  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(
      ROS_PACKAGE_NAME, "Subscription serialized take succeeded: %s", taken ? "true" : "false");

  // 如果没有获取到消息，返回失败
  if (!taken) {
    return RCL_RET_SUBSCRIPTION_TAKE_FAILED;
  }

  // 成功返回
  return RCL_RET_OK;
}

/**
 * @brief 从订阅者中获取已借出的消息。
 *
 * @param[in] subscription 指向有效的rcl_subscription_t结构体的指针。
 * @param[out] loaned_message 存储已借出消息的指针的地址。如果成功，将分配内存并设置指针。
 * @param[out] message_info 存储与接收到的消息相关的元数据的指针。可以为NULL。
 * @param[in] allocation 预先分配的rmw_subscription_allocation_t结构体的指针。可以为NULL。
 * @return 返回RCL_RET_OK，如果成功地从订阅者中获取了已借出的消息；否则返回一个RCL错误代码。
 */
rcl_ret_t rcl_take_loaned_message(
    const rcl_subscription_t *subscription,
    void **loaned_message,
    rmw_message_info_t *message_info,
    rmw_subscription_allocation_t *allocation) {
  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Subscription taking loaned message");

  // 检查订阅是否有效
  if (!rcl_subscription_is_valid(subscription)) {
    return RCL_RET_SUBSCRIPTION_INVALID;  // error already set
  }

  // 检查loaned_message参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(loaned_message, RCL_RET_INVALID_ARGUMENT);

  // 检查loaned_message是否已经初始化
  if (*loaned_message) {
    RCL_SET_ERROR_MSG("loaned message is already initialized");
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 如果message_info为NULL，使用一个可以丢弃的占位符
  rmw_message_info_t dummy_message_info;
  rmw_message_info_t *message_info_local = message_info ? message_info : &dummy_message_info;
  *message_info_local = rmw_get_zero_initialized_message_info();

  // 调用rmw_take_with_info
  bool taken = false;
  rmw_ret_t ret = rmw_take_loaned_message_with_info(
      subscription->impl->rmw_handle, loaned_message, &taken, message_info_local, allocation);

  // 检查rmw_take_loaned_message_with_info的返回值
  if (ret != RMW_RET_OK) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    return rcl_convert_rmw_ret_to_rcl_ret(ret);
  }

  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(
      ROS_PACKAGE_NAME, "Subscription loaned take succeeded: %s", taken ? "true" : "false");

  // 如果没有获取到消息，则返回错误代码
  if (!taken) {
    return RCL_RET_SUBSCRIPTION_TAKE_FAILED;
  }

  // 成功返回
  return RCL_RET_OK;
}

/**
 * @brief 从订阅者中返回借用的消息
 *
 * @param[in] subscription 指向有效的rcl_subscription_t结构的指针
 * @param[out] loaned_message 指向借用的消息的指针
 * @return rcl_ret_t 返回RCL_RET_OK或相应的错误代码
 */
rcl_ret_t rcl_return_loaned_message_from_subscription(
    const rcl_subscription_t *subscription, void *loaned_message) {
  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Subscription releasing loaned message");

  // 检查订阅是否有效
  if (!rcl_subscription_is_valid(subscription)) {
    return RCL_RET_SUBSCRIPTION_INVALID;  // error already set
  }

  // 检查借用的消息参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(loaned_message, RCL_RET_INVALID_ARGUMENT);

  // 转换并返回rmw层的结果
  return rcl_convert_rmw_ret_to_rcl_ret(
      rmw_return_loaned_message_from_subscription(subscription->impl->rmw_handle, loaned_message));
}

/**
 * @brief 获取订阅的主题名称
 *
 * @param[in] subscription 指向有效的rcl_subscription_t结构的指针
 * @return const char* 返回主题名称字符串或NULL（如果订阅无效）
 */
const char *rcl_subscription_get_topic_name(const rcl_subscription_t *subscription) {
  // 检查订阅是否有效
  if (!rcl_subscription_is_valid(subscription)) {
    return NULL;  // error already set
  }

  // 返回主题名称
  return subscription->impl->rmw_handle->topic_name;
}

// 定义一个宏，用于获取订阅选项
#define _subscription_get_options(subscription) &subscription->impl->options

/**
 * @brief 获取订阅的选项
 *
 * @param[in] subscription 指向有效的rcl_subscription_t结构的指针
 * @return const rcl_subscription_options_t* 返回指向订阅选项的指针或NULL（如果订阅无效）
 */
const rcl_subscription_options_t *rcl_subscription_get_options(
    const rcl_subscription_t *subscription) {
  // 检查订阅是否有效
  if (!rcl_subscription_is_valid(subscription)) {
    return NULL;  // error already set
  }

  // 返回订阅选项
  return _subscription_get_options(subscription);
}

/**
 * @brief 获取订阅的rmw句柄
 *
 * @param[in] subscription 指向有效的rcl_subscription_t结构的指针
 * @return rmw_subscription_t* 返回指向rmw订阅句柄的指针或NULL（如果订阅无效）
 */
rmw_subscription_t *rcl_subscription_get_rmw_handle(const rcl_subscription_t *subscription) {
  // 检查订阅是否有效
  if (!rcl_subscription_is_valid(subscription)) {
    return NULL;  // error already  set
  }

  // 返回rmw订阅句柄
  return subscription->impl->rmw_handle;
}

/**
 * @brief 检查订阅是否有效
 *
 * @param[in] subscription 要检查的订阅指针
 * @return 如果订阅有效，则返回true，否则返回false
 */
bool rcl_subscription_is_valid(const rcl_subscription_t *subscription) {
  // 检查订阅指针是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(subscription, "subscription pointer is invalid", return false);
  // 检查订阅的实现是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(
      subscription->impl, "subscription's implementation is invalid", return false);
  // 检查订阅的rmw句柄是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(
      subscription->impl->rmw_handle, "subscription's rmw handle is invalid", return false);
  return true;
}

/**
 * @brief 获取与订阅匹配的发布者数量
 *
 * @param[in] subscription 订阅指针
 * @param[out] publisher_count 匹配的发布者数量
 * @return 成功时返回RCL_RET_OK，失败时返回相应的错误代码
 */
rmw_ret_t rcl_subscription_get_publisher_count(
    const rcl_subscription_t *subscription, size_t *publisher_count) {
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_SUBSCRIPTION_INVALID);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);

  // 检查订阅是否有效
  if (!rcl_subscription_is_valid(subscription)) {
    return RCL_RET_SUBSCRIPTION_INVALID;
  }
  // 检查publisher_count参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(publisher_count, RCL_RET_INVALID_ARGUMENT);
  // 获取匹配的发布者数量
  rmw_ret_t ret =
      rmw_subscription_count_matched_publishers(subscription->impl->rmw_handle, publisher_count);

  // 检查返回值是否为RMW_RET_OK，如果不是，则设置错误消息并转换为rcl_ret_t类型
  if (ret != RMW_RET_OK) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    return rcl_convert_rmw_ret_to_rcl_ret(ret);
  }
  return RCL_RET_OK;
}

/**
 * @brief 获取订阅的实际QoS配置
 *
 * @param[in] subscription 订阅指针
 * @return 如果订阅有效，则返回实际QoS配置的指针，否则返回NULL
 */
const rmw_qos_profile_t *rcl_subscription_get_actual_qos(const rcl_subscription_t *subscription) {
  // 检查订阅是否有效
  if (!rcl_subscription_is_valid(subscription)) {
    return NULL;
  }
  // 返回实际QoS配置的指针
  return &subscription->impl->actual_qos;
}

/**
 * @brief 检查订阅者是否可以借用消息
 *
 * @param[in] subscription 指向rcl_subscription_t结构体的指针
 * @return 如果可以借用消息，则返回true，否则返回false
 */
bool rcl_subscription_can_loan_messages(const rcl_subscription_t *subscription) {
  // 验证订阅者是否有效
  if (!rcl_subscription_is_valid(subscription)) {
    return false;  // 错误信息已设置
  }

  // 检查是否禁用了借用消息功能
  if (subscription->impl->options.disable_loaned_message) {
    return false;
  }

  // 返回订阅者是否可以借用消息的状态
  return subscription->impl->rmw_handle->can_loan_messages;
}

/**
 * @brief 设置新消息回调函数
 *
 * @param[in] subscription 指向rcl_subscription_t结构体的指针
 * @param[in] callback 新消息回调函数
 * @param[in] user_data 用户数据，将传递给回调函数
 * @return 成功时返回RCL_RET_OK，失败时返回相应的错误代码
 */
rcl_ret_t rcl_subscription_set_on_new_message_callback(
    const rcl_subscription_t *subscription, rcl_event_callback_t callback, const void *user_data) {
  // 验证订阅者是否有效
  if (!rcl_subscription_is_valid(subscription)) {
    // 错误状态已设置
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 设置新消息回调函数
  return rmw_subscription_set_on_new_message_callback(
      subscription->impl->rmw_handle, callback, user_data);
}

#ifdef __cplusplus
}
#endif
