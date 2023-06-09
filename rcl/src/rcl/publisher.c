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

#include "rcl/publisher.h"

#include <stdio.h>
#include <string.h>

#include "./common.h"
#include "./publisher_impl.h"
#include "rcl/allocator.h"
#include "rcl/error_handling.h"
#include "rcl/node.h"
#include "rcl/time.h"
#include "rcutils/logging_macros.h"
#include "rcutils/macros.h"
#include "rmw/error_handling.h"
#include "rmw/time.h"
#include "tracetools/tracetools.h"

/**
 * @brief 获取一个零初始化的发布者对象
 *
 * @return 返回一个零初始化的rcl_publisher_t对象
 */
rcl_publisher_t rcl_get_zero_initialized_publisher() {
  // 定义一个静态的空发布者对象并初始化为0
  static rcl_publisher_t null_publisher = {0};
  // 返回空发布者对象
  return null_publisher;
}

/**
 * @brief 初始化发布者
 *
 * @param[in,out] publisher 指向待初始化的发布者对象的指针
 * @param[in] node 指向关联节点的指针
 * @param[in] type_support 消息类型支持
 * @param[in] topic_name 主题名称
 * @param[in] options 发布者选项
 * @return 返回rcl_ret_t类型的结果，成功返回RCL_RET_OK
 */
rcl_ret_t rcl_publisher_init(
    rcl_publisher_t *publisher,
    const rcl_node_t *node,
    const rosidl_message_type_support_t *type_support,
    const char *topic_name,
    const rcl_publisher_options_t *options) {
  // 定义可能的错误返回值
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_ALREADY_INIT);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_NODE_INVALID);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_BAD_ALLOC);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_ERROR);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_TOPIC_NAME_INVALID);

  // 定义失败时的返回值
  rcl_ret_t fail_ret = RCL_RET_ERROR;

  // 首先检查选项和分配器，以便在出错时可以使用分配器
  RCL_CHECK_ARGUMENT_FOR_NULL(options, RCL_RET_INVALID_ARGUMENT);
  rcl_allocator_t *allocator = (rcl_allocator_t *)&options->allocator;
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);

  // 检查发布者、节点、类型支持和主题名称是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(publisher, RCL_RET_INVALID_ARGUMENT);
  if (publisher->impl) {
    RCL_SET_ERROR_MSG("publisher already initialized, or memory was unintialized");
    return RCL_RET_ALREADY_INIT;
  }
  if (!rcl_node_is_valid(node)) {
    return RCL_RET_NODE_INVALID;  // error already set
  }
  RCL_CHECK_ARGUMENT_FOR_NULL(type_support, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(topic_name, RCL_RET_INVALID_ARGUMENT);
  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(
      ROS_PACKAGE_NAME, "Initializing publisher for topic name '%s'", topic_name);

  // 扩展并重新映射给定的主题名称
  char *remapped_topic_name = NULL;
  rcl_ret_t ret =
      rcl_node_resolve_name(node, topic_name, *allocator, false, false, &remapped_topic_name);
  if (ret != RCL_RET_OK) {
    if (ret == RCL_RET_TOPIC_NAME_INVALID || ret == RCL_RET_UNKNOWN_SUBSTITUTION) {
      ret = RCL_RET_TOPIC_NAME_INVALID;
    } else if (ret != RCL_RET_BAD_ALLOC) {
      ret = RCL_RET_ERROR;
    }
    goto cleanup;
  }
  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(
      ROS_PACKAGE_NAME, "Expanded and remapped topic name '%s'", remapped_topic_name);

  // 为实现结构分配空间
  publisher->impl =
      (rcl_publisher_impl_t *)allocator->allocate(sizeof(rcl_publisher_impl_t), allocator->state);
  RCL_CHECK_FOR_NULL_WITH_MSG(publisher->impl, "allocating memory failed", ret = RCL_RET_BAD_ALLOC;
                              goto cleanup);

  // 填充实现结构
  // rmw handle (create rmw publisher)
  // TODO(wjwwood): pass along the allocator to rmw when it supports it
  publisher->impl->rmw_handle = rmw_create_publisher(
      rcl_node_get_rmw_handle(node), type_support, remapped_topic_name, &(options->qos),
      &(options->rmw_publisher_options));
  RCL_CHECK_FOR_NULL_WITH_MSG(publisher->impl->rmw_handle, rmw_get_error_string().str, goto fail);
  // 获取实际的QoS，并存储它
  rmw_ret_t rmw_ret =
      rmw_publisher_get_actual_qos(publisher->impl->rmw_handle, &publisher->impl->actual_qos);
  if (RMW_RET_OK != rmw_ret) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    goto fail;
  }
  publisher->impl->actual_qos.avoid_ros_namespace_conventions =
      options->qos.avoid_ros_namespace_conventions;
  publisher->impl->options = *options;
  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Publisher initialized");
  publisher->impl->context = node->context;
  TRACEPOINT(
      rcl_publisher_init, (const void *)publisher, (const void *)node,
      (const void *)publisher->impl->rmw_handle, remapped_topic_name, options->qos.depth);
  goto cleanup;

fail:
  if (publisher->impl) {
    if (publisher->impl->rmw_handle) {
      rmw_ret_t rmw_fail_ret =
          rmw_destroy_publisher(rcl_node_get_rmw_handle(node), publisher->impl->rmw_handle);
      if (RMW_RET_OK != rmw_fail_ret) {
        RCUTILS_SAFE_FWRITE_TO_STDERR(rmw_get_error_string().str);
        RCUTILS_SAFE_FWRITE_TO_STDERR("\n");
      }
    }

    allocator->deallocate(publisher->impl, allocator->state);
    publisher->impl = NULL;
  }
  ret = fail_ret;

cleanup:  // 跳转到cleanup
  allocator->deallocate(remapped_topic_name, allocator->state);
  return ret;
}

/**
 * @brief 销毁一个rcl_publisher_t实例
 *
 * @param[in] publisher 要销毁的发布者指针
 * @param[in] node 与发布者关联的节点指针
 * @return 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_publisher_fini(rcl_publisher_t *publisher, rcl_node_t *node) {
  // 检查可能的错误返回值
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_PUBLISHER_INVALID);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_NODE_INVALID);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_ERROR);

  rcl_ret_t result = RCL_RET_OK;
  // 检查publisher是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(publisher, RCL_RET_PUBLISHER_INVALID);
  // 检查节点是否有效
  if (!rcl_node_is_valid_except_context(node)) {
    return RCL_RET_NODE_INVALID;  // error already set
  }

  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Finalizing publisher");
  if (publisher->impl) {
    // 获取分配器和rmw节点
    rcl_allocator_t allocator = publisher->impl->options.allocator;
    rmw_node_t *rmw_node = rcl_node_get_rmw_handle(node);
    if (!rmw_node) {
      return RCL_RET_INVALID_ARGUMENT;
    }
    // 销毁rmw发布者
    rmw_ret_t ret = rmw_destroy_publisher(rmw_node, publisher->impl->rmw_handle);
    if (ret != RMW_RET_OK) {
      RCL_SET_ERROR_MSG(rmw_get_error_string().str);
      result = RCL_RET_ERROR;
    }
    // 释放内存
    allocator.deallocate(publisher->impl, allocator.state);
    publisher->impl = NULL;
  }
  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Publisher finalized");
  return result;
}

/**
 * @brief 获取rcl_publisher_t的默认选项
 *
 * @return 返回一个rcl_publisher_options_t实例，包含默认选项
 */
rcl_publisher_options_t rcl_publisher_get_default_options() {
  // !!! 确保这些默认值的更改反映在头文件文档字符串中
  static rcl_publisher_options_t default_options;
  // 必须在编译时设置分配器和qos常量
  default_options.qos = rmw_qos_profile_default;
  default_options.allocator = rcl_get_default_allocator();
  default_options.rmw_publisher_options = rmw_get_default_publisher_options();

  // 通过环境变量加载LoanedMessage的禁用标志
  bool disable_loaned_message = false;
  rcl_ret_t ret = rcl_get_disable_loaned_message(&disable_loaned_message);
  if (ret == RCL_RET_OK) {
    default_options.disable_loaned_message = disable_loaned_message;
  } else {
    RCUTILS_SAFE_FWRITE_TO_STDERR("Failed to get disable_loaned_message: ");
    RCUTILS_SAFE_FWRITE_TO_STDERR(rcl_get_error_string().str);
    rcl_reset_error();
    default_options.disable_loaned_message = false;
  }

  return default_options;
}

/**
 * @brief 借用已发布消息的内存空间
 *
 * @param[in] publisher 指向有效的rcl_publisher_t结构体的指针
 * @param[in] type_support 消息类型支持的指针
 * @param[out] ros_message 存储借用消息内存空间的指针的指针
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_borrow_loaned_message(
    const rcl_publisher_t *publisher,
    const rosidl_message_type_support_t *type_support,
    void **ros_message) {
  // 检查发布者是否有效
  if (!rcl_publisher_is_valid(publisher)) {
    return RCL_RET_PUBLISHER_INVALID;  // 错误已设置
  }
  // 转换RMW返回值为RCL返回值
  return rcl_convert_rmw_ret_to_rcl_ret(
      rmw_borrow_loaned_message(publisher->impl->rmw_handle, type_support, ros_message));
}

/**
 * @brief 归还从发布者借用的消息内存空间
 *
 * @param[in] publisher 指向有效的rcl_publisher_t结构体的指针
 * @param[in] loaned_message 需要归还的消息内存空间的指针
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_return_loaned_message_from_publisher(
    const rcl_publisher_t *publisher, void *loaned_message) {
  // 检查发布者是否有效
  if (!rcl_publisher_is_valid(publisher)) {
    return RCL_RET_PUBLISHER_INVALID;  // 错误已设置
  }
  // 检查借用的消息是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(loaned_message, RCL_RET_INVALID_ARGUMENT);
  // 转换RMW返回值为RCL返回值
  return rcl_convert_rmw_ret_to_rcl_ret(
      rmw_return_loaned_message_from_publisher(publisher->impl->rmw_handle, loaned_message));
}

/**
 * @brief 发布一条ROS消息
 *
 * @param[in] publisher 指向有效的rcl_publisher_t结构体的指针
 * @param[in] ros_message 需要发布的ROS消息的指针
 * @param[in] allocation 指向分配器的指针，用于自定义内存管理
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_publish(
    const rcl_publisher_t *publisher,
    const void *ros_message,
    rmw_publisher_allocation_t *allocation) {
  // 设置可能的错误返回值
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_PUBLISHER_INVALID);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_ERROR);
  // 检查发布者是否有效
  if (!rcl_publisher_is_valid(publisher)) {
    return RCL_RET_PUBLISHER_INVALID;  // 错误已设置
  }
  // 检查ROS消息是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(ros_message, RCL_RET_INVALID_ARGUMENT);
  // 记录发布操作的跟踪点
  TRACEPOINT(rcl_publish, (const void *)publisher, (const void *)ros_message);

  // 如果发布失败，设置错误信息并返回错误
  if (rmw_publish(publisher->impl->rmw_handle, ros_message, allocation) != RMW_RET_OK) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    return RCL_RET_ERROR;
  }
  return RCL_RET_OK;
}

/**
 * @brief 发布序列化消息
 *
 * @param[in] publisher 指向有效的rcl_publisher_t结构体的指针
 * @param[in] serialized_message 指向有效的rcl_serialized_message_t结构体的指针
 * @param[in,out] allocation 指向rmw_publisher_allocation_t结构体的指针，用于分配内存
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_publish_serialized_message(
    const rcl_publisher_t *publisher,
    const rcl_serialized_message_t *serialized_message,
    rmw_publisher_allocation_t *allocation) {
  // 检查发布者是否有效
  if (!rcl_publisher_is_valid(publisher)) {
    return RCL_RET_PUBLISHER_INVALID;  // 错误已设置
  }
  // 检查序列化消息参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(serialized_message, RCL_RET_INVALID_ARGUMENT);
  // 调用底层函数发布序列化消息
  rmw_ret_t ret =
      rmw_publish_serialized_message(publisher->impl->rmw_handle, serialized_message, allocation);
  // 判断返回值是否为RMW_RET_OK
  if (ret != RMW_RET_OK) {
    // 设置错误信息
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    // 判断返回值是否为RMW_RET_BAD_ALLOC
    if (ret == RMW_RET_BAD_ALLOC) {
      return RCL_RET_BAD_ALLOC;
    }
    return RCL_RET_ERROR;
  }
  return RCL_RET_OK;
}

/**
 * @brief 发布借用消息
 *
 * @param[in] publisher 指向有效的rcl_publisher_t结构体的指针
 * @param[in] ros_message 指向ROS消息的指针
 * @param[in,out] allocation 指向rmw_publisher_allocation_t结构体的指针，用于分配内存
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_publish_loaned_message(
    const rcl_publisher_t *publisher, void *ros_message, rmw_publisher_allocation_t *allocation) {
  // 检查发布者是否有效
  if (!rcl_publisher_is_valid(publisher)) {
    return RCL_RET_PUBLISHER_INVALID;  // 错误已设置
  }
  // 检查ros_message参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(ros_message, RCL_RET_INVALID_ARGUMENT);
  // 调用底层函数发布借用消息
  rmw_ret_t ret = rmw_publish_loaned_message(publisher->impl->rmw_handle, ros_message, allocation);
  // 判断返回值是否为RMW_RET_OK
  if (ret != RMW_RET_OK) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    return RCL_RET_ERROR;
  }
  return RCL_RET_OK;
}

/**
 * @brief 确认发布者的活跃状态
 *
 * @param[in] publisher 指向rcl_publisher_t结构体的指针
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_publisher_assert_liveliness(const rcl_publisher_t *publisher) {
  // 检查发布者是否有效
  if (!rcl_publisher_is_valid(publisher)) {
    return RCL_RET_PUBLISHER_INVALID;  // 错误已设置
  }
  // 调用底层函数确认发布者的活跃状态
  if (rmw_publisher_assert_liveliness(publisher->impl->rmw_handle) != RMW_RET_OK) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    return RCL_RET_ERROR;
  }
  return RCL_RET_OK;
}

/**
 * @brief 等待所有发布者的确认消息
 *
 * @param[in] publisher 指向rcl_publisher_t结构体的指针
 * @param[in] timeout 超时时间，单位为纳秒
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_publisher_wait_for_all_acked(
    const rcl_publisher_t *publisher, rcl_duration_value_t timeout) {
  // 检查发布者是否有效
  if (!rcl_publisher_is_valid(publisher)) {
    return RCL_RET_PUBLISHER_INVALID;  // 错误已设置
  }

  rmw_time_t rmw_timeout;
  // 根据超时时间设置rmw_timeout
  if (timeout > 0) {
    rmw_timeout.sec = RCL_NS_TO_S(timeout);
    rmw_timeout.nsec = timeout % 1000000000;
  } else if (timeout < 0) {
    rmw_time_t infinite = RMW_DURATION_INFINITE;
    rmw_timeout = infinite;
  } else {
    rmw_time_t zero = RMW_DURATION_UNSPECIFIED;
    rmw_timeout = zero;
  }

  // 调用底层函数等待所有发布者的确认消息
  rmw_ret_t ret = rmw_publisher_wait_for_all_acked(publisher->impl->rmw_handle, rmw_timeout);
  // 处理返回结果
  if (ret != RMW_RET_OK) {
    if (ret == RMW_RET_TIMEOUT) {
      return RCL_RET_TIMEOUT;
    }
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    if (ret == RMW_RET_UNSUPPORTED) {
      return RCL_RET_UNSUPPORTED;
    } else {
      return RCL_RET_ERROR;
    }
  }

  return RCL_RET_OK;
}

/**
 * @brief 获取发布者的主题名称
 *
 * @param[in] publisher 指向rcl_publisher_t结构体的指针
 * @return 主题名称字符串，如果出错则返回NULL
 */
const char *rcl_publisher_get_topic_name(const rcl_publisher_t *publisher) {
  // 检查publisher是否有效，但不检查上下文
  if (!rcl_publisher_is_valid_except_context(publisher)) {
    return NULL;  // 错误已设置
  }
  // 返回主题名称
  return publisher->impl->rmw_handle->topic_name;
}

/**
 * @brief 定义一个宏，用于获取发布者的选项
 *
 * @param[in] pub 指向rcl_publisher_t结构体的指针
 * @return 发布者选项的指针
 */
#define _publisher_get_options(pub) &pub->impl->options

/**
 * @brief 获取发布者的选项
 *
 * @param[in] publisher 指向rcl_publisher_t结构体的指针
 * @return rcl_publisher_options_t结构体的指针，如果出错则返回NULL
 */
const rcl_publisher_options_t *rcl_publisher_get_options(const rcl_publisher_t *publisher) {
  // 检查publisher是否有效，但不检查上下文
  if (!rcl_publisher_is_valid_except_context(publisher)) {
    return NULL;  // 错误已设置
  }
  // 返回发布者选项
  return _publisher_get_options(publisher);
}

/**
 * @brief 获取发布者的rmw句柄
 *
 * @param[in] publisher 指向rcl_publisher_t结构体的指针
 * @return rmw_publisher_t结构体的指针，如果出错则返回NULL
 */
rmw_publisher_t *rcl_publisher_get_rmw_handle(const rcl_publisher_t *publisher) {
  // 检查publisher是否有效，但不检查上下文
  if (!rcl_publisher_is_valid_except_context(publisher)) {
    return NULL;  // 错误已设置
  }
  // 返回rmw句柄
  return publisher->impl->rmw_handle;
}

/**
 * @brief 获取发布者的上下文
 *
 * @param[in] publisher 指向rcl_publisher_t结构体的指针
 * @return rcl_context_t结构体的指针，如果出错则返回NULL
 */
rcl_context_t *rcl_publisher_get_context(const rcl_publisher_t *publisher) {
  // 检查publisher是否有效，但不检查上下文
  if (!rcl_publisher_is_valid_except_context(publisher)) {
    return NULL;  // 错误已设置
  }
  // 返回上下文
  return publisher->impl->context;
}

/**
 * @brief 检查发布者是否有效
 *
 * @param[in] publisher 要检查的发布者指针
 * @return 如果发布者有效，则返回true，否则返回false
 */
bool rcl_publisher_is_valid(const rcl_publisher_t *publisher) {
  // 检查发布者是否有效（不包括上下文）
  if (!rcl_publisher_is_valid_except_context(publisher)) {
    return false;  // 错误已设置
  }
  // 检查发布者的上下文是否有效
  if (!rcl_context_is_valid(publisher->impl->context)) {
    RCL_SET_ERROR_MSG("publisher's context is invalid");
    return false;
  }
  // 检查发布者的rmw句柄是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(
      publisher->impl->rmw_handle, "publisher's rmw handle is invalid", return false);
  return true;
}

/**
 * @brief 检查发布者是否有效（不包括上下文）
 *
 * @param[in] publisher 要检查的发布者指针
 * @return 如果发布者有效（不包括上下文），则返回true，否则返回false
 */
bool rcl_publisher_is_valid_except_context(const rcl_publisher_t *publisher) {
  // 检查是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(publisher, "publisher pointer is invalid", return false);
  RCL_CHECK_FOR_NULL_WITH_MSG(publisher->impl, "publisher implementation is invalid", return false);
  RCL_CHECK_FOR_NULL_WITH_MSG(
      publisher->impl->rmw_handle, "publisher's rmw handle is invalid", return false);
  return true;
}

/**
 * @brief 获取发布者的订阅数量
 *
 * @param[in] publisher 要查询的发布者指针
 * @param[out] subscription_count 存储订阅数量的指针
 * @return 成功返回RCL_RET_OK，否则返回相应的错误代码
 */
rcl_ret_t rcl_publisher_get_subscription_count(
    const rcl_publisher_t *publisher, size_t *subscription_count) {
  // 检查发布者是否有效
  if (!rcl_publisher_is_valid(publisher)) {
    return RCL_RET_PUBLISHER_INVALID;
  }
  // 检查subscription_count参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(subscription_count, RCL_RET_INVALID_ARGUMENT);

  // 调用rmw函数获取匹配的订阅数量
  rmw_ret_t ret =
      rmw_publisher_count_matched_subscriptions(publisher->impl->rmw_handle, subscription_count);

  // 如果rmw函数返回错误，设置错误消息并转换为rcl错误代码
  if (ret != RMW_RET_OK) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    return rcl_convert_rmw_ret_to_rcl_ret(ret);
  }
  return RCL_RET_OK;
}

/**
 * @brief 获取给定发布者的实际QoS配置。
 *
 * 此函数检查发布者是否有效，然后返回其实际QoS配置。
 *
 * @param[in] publisher 指向rcl_publisher_t结构体的指针，表示要查询的发布者。
 * @return 如果发布者有效，则返回指向实际QoS配置的指针；否则返回NULL。
 */
const rmw_qos_profile_t *rcl_publisher_get_actual_qos(const rcl_publisher_t *publisher) {
  // 检查发布者是否有效（除上下文外）
  if (!rcl_publisher_is_valid_except_context(publisher)) {
    return NULL;
  }
  // 返回实际QoS配置的引用
  return &publisher->impl->actual_qos;
}

/**
 * @brief 判断给定的发布者是否可以借出消息。
 *
 * 此函数首先检查发布者是否有效，然后根据选项和底层RMW实现判断是否可以借出消息。
 *
 * @param[in] publisher 指向rcl_publisher_t结构体的指针，表示要查询的发布者。
 * @return 如果发布者可以借出消息，则返回true；否则返回false。
 */
bool rcl_publisher_can_loan_messages(const rcl_publisher_t *publisher) {
  // 检查发布者是否有效
  if (!rcl_publisher_is_valid(publisher)) {
    return false;  // 错误消息已设置
  }

  // 检查是否禁用了借出消息选项
  if (publisher->impl->options.disable_loaned_message) {
    return false;
  }

  // 返回底层RMW实现的can_loan_messages值
  return publisher->impl->rmw_handle->can_loan_messages;
}

#ifdef __cplusplus
}
#endif
