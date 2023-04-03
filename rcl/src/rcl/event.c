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

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/event.h"

#include <stdio.h>

#include "./common.h"
#include "./event_impl.h"
#include "./publisher_impl.h"
#include "./subscription_impl.h"
#include "rcl/error_handling.h"
#include "rcl/expand_topic_name.h"
#include "rcl/remap.h"
#include "rcutils/allocator.h"
#include "rcutils/logging_macros.h"
#include "rmw/error_handling.h"
#include "rmw/event.h"
#include "rmw/validate_full_topic_name.h"

/**
 * @brief 获取一个初始化为零的 rcl_event_t 结构体实例
 *
 * @return 初始化为零的 rcl_event_t 结构体实例
 */
rcl_event_t rcl_get_zero_initialized_event() {
  // 定义并初始化一个静态的空事件结构体
  static rcl_event_t null_event = {0};
  // 返回这个空事件结构体
  return null_event;
}

/**
 * @brief 初始化发布者事件
 *
 * @param[out] event 指向要初始化的 rcl_event_t 结构体指针
 * @param[in] publisher 指向 rcl_publisher_t 结构体的指针，用于获取分配器和 rmw_handle
 * @param[in] event_type 发布者事件类型
 * @return rcl_ret_t 返回 RCL_RET_OK 或相应的错误代码
 */
rcl_ret_t rcl_publisher_event_init(
    rcl_event_t *event,
    const rcl_publisher_t *publisher,
    const rcl_publisher_event_type_t event_type) {
  // 检查 event 参数是否为空，如果为空则返回 RCL_RET_EVENT_INVALID 错误
  RCL_CHECK_ARGUMENT_FOR_NULL(event, RCL_RET_EVENT_INVALID);
  // 首先检查发布者和分配器，以便在出现错误时可以使用分配器
  RCL_CHECK_ARGUMENT_FOR_NULL(publisher, RCL_RET_INVALID_ARGUMENT);
  // 获取分配器指针
  rcl_allocator_t *allocator = &publisher->impl->options.allocator;
  // 检查分配器是否有效，如果无效则返回 RCL_RET_INVALID_ARGUMENT 错误
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  // 定义并初始化 rmw_event_type_t 变量
  rmw_event_type_t rmw_event_type = RMW_EVENT_INVALID;
  // 根据 event_type 参数设置相应的 rmw_event_type 值
  switch (event_type) {
    case RCL_PUBLISHER_OFFERED_DEADLINE_MISSED:
      rmw_event_type = RMW_EVENT_OFFERED_DEADLINE_MISSED;
      break;
    case RCL_PUBLISHER_LIVELINESS_LOST:
      rmw_event_type = RMW_EVENT_LIVELINESS_LOST;
      break;
    case RCL_PUBLISHER_OFFERED_INCOMPATIBLE_QOS:
      rmw_event_type = RMW_EVENT_OFFERED_QOS_INCOMPATIBLE;
      break;
    case RCL_PUBLISHER_INCOMPATIBLE_TYPE:
      rmw_event_type = RMW_EVENT_PUBLISHER_INCOMPATIBLE_TYPE;
      break;
    case RCL_PUBLISHER_MATCHED:
      rmw_event_type = RMW_EVENT_PUBLICATION_MATCHED;
      break;
    default:
      // 如果不支持的事件类型，则设置错误消息并返回 RCL_RET_INVALID_ARGUMENT 错误
      RCL_SET_ERROR_MSG("Event type for publisher not supported");
      return RCL_RET_INVALID_ARGUMENT;
  }

  // 为实现结构体分配空间
  event->impl = (rcl_event_impl_t *)allocator->allocate(sizeof(rcl_event_impl_t), allocator->state);
  // 检查分配的内存是否为空，如果为空则返回 RCL_RET_BAD_ALLOC 错误
  RCL_CHECK_FOR_NULL_WITH_MSG(event->impl, "allocating memory failed", return RCL_RET_BAD_ALLOC);

  // 初始化 rmw_handle 和分配器
  event->impl->rmw_handle = rmw_get_zero_initialized_event();
  event->impl->allocator = *allocator;

  // 调用 rmw_publisher_event_init 函数初始化事件
  rmw_ret_t ret = rmw_publisher_event_init(
      &event->impl->rmw_handle, publisher->impl->rmw_handle, rmw_event_type);
  // 如果返回值不是 RMW_RET_OK，则跳转到 fail 标签
  if (ret != RMW_RET_OK) {
    goto fail;
  }

  // 返回 RCL_RET_OK 表示成功
  return RCL_RET_OK;
fail:
  // 处理失败情况，释放已分配的内存并将 event->impl 设置为 NULL
  allocator->deallocate(event->impl, allocator->state);
  event->impl = NULL;
  // 将 rmw_ret_t 类型的错误代码转换为 rcl_ret_t 类型并返回
  return rcl_convert_rmw_ret_to_rcl_ret(ret);
}

/**
 * @brief 初始化订阅事件
 *
 * @param[in] event 指向要初始化的rcl_event_t结构体的指针
 * @param[in] subscription 指向相关联的rcl_subscription_t结构体的指针
 * @param[in] event_type 要初始化的订阅事件类型
 * @return 返回rcl_ret_t类型的结果，成功返回RCL_RET_OK，否则返回相应的错误代码
 */
rcl_ret_t rcl_subscription_event_init(
    rcl_event_t *event,
    const rcl_subscription_t *subscription,
    const rcl_subscription_event_type_t event_type) {
  // 检查event参数是否为空，如果为空返回RCL_RET_EVENT_INVALID
  RCL_CHECK_ARGUMENT_FOR_NULL(event, RCL_RET_EVENT_INVALID);

  // 首先检查subscription和allocator，以便在出现错误时可以使用allocator
  RCL_CHECK_ARGUMENT_FOR_NULL(subscription, RCL_RET_INVALID_ARGUMENT);

  // 获取分配器
  rcl_allocator_t *allocator = &subscription->impl->options.allocator;

  // 检查分配器是否有效，如果无效返回RCL_RET_INVALID_ARGUMENT
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);

  // 定义rmw_event_type_t变量并初始化为RMW_EVENT_INVALID
  rmw_event_type_t rmw_event_type = RMW_EVENT_INVALID;

  // 根据传入的event_type设置对应的rmw_event_type
  switch (event_type) {
    case RCL_SUBSCRIPTION_REQUESTED_DEADLINE_MISSED:
      rmw_event_type = RMW_EVENT_REQUESTED_DEADLINE_MISSED;
      break;
    case RCL_SUBSCRIPTION_LIVELINESS_CHANGED:
      rmw_event_type = RMW_EVENT_LIVELINESS_CHANGED;
      break;
    case RCL_SUBSCRIPTION_REQUESTED_INCOMPATIBLE_QOS:
      rmw_event_type = RMW_EVENT_REQUESTED_QOS_INCOMPATIBLE;
      break;
    case RCL_SUBSCRIPTION_MESSAGE_LOST:
      rmw_event_type = RMW_EVENT_MESSAGE_LOST;
      break;
    case RCL_SUBSCRIPTION_INCOMPATIBLE_TYPE:
      rmw_event_type = RMW_EVENT_SUBSCRIPTION_INCOMPATIBLE_TYPE;
      break;
    case RCL_SUBSCRIPTION_MATCHED:
      rmw_event_type = RMW_EVENT_SUBSCRIPTION_MATCHED;
      break;
    default:
      // 如果传入的event_type不支持，设置错误信息并返回RCL_RET_INVALID_ARGUMENT
      RCL_SET_ERROR_MSG("Event type for subscription not supported");
      return RCL_RET_INVALID_ARGUMENT;
  }

  // 为实现结构体分配空间
  event->impl = (rcl_event_impl_t *)allocator->allocate(sizeof(rcl_event_impl_t), allocator->state);

  // 检查分配的内存是否为空，如果为空返回RCL_RET_BAD_ALLOC
  RCL_CHECK_FOR_NULL_WITH_MSG(event->impl, "allocating memory failed", return RCL_RET_BAD_ALLOC);

  // 初始化rmw_handle
  event->impl->rmw_handle = rmw_get_zero_initialized_event();

  // 设置分配器
  event->impl->allocator = *allocator;

  // 调用rmw_subscription_event_init函数初始化事件
  rmw_ret_t ret = rmw_subscription_event_init(
      &event->impl->rmw_handle, subscription->impl->rmw_handle, rmw_event_type);

  // 如果初始化失败，跳转到fail标签
  if (ret != RMW_RET_OK) {
    goto fail;
  }

  // 返回成功
  return RCL_RET_OK;

// 处理失败情况
fail:
  // 释放分配的内存
  allocator->deallocate(event->impl, allocator->state);

  // 将event的实现指针设置为NULL
  event->impl = NULL;

  // 转换rmw_ret_t类型的错误代码为rcl_ret_t类型并返回
  return rcl_convert_rmw_ret_to_rcl_ret(ret);
}

/**
 * @brief 从事件中获取信息
 *
 * @param[in] event 指向rcl_event_t类型的指针，表示要获取信息的事件
 * @param[out] event_info 存储事件信息的指针
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_take_event(const rcl_event_t *event, void *event_info) {
  // 定义一个布尔变量，表示是否成功获取到事件信息
  bool taken = false;

  // 检查事件是否有效
  if (!rcl_event_is_valid(event)) {
    return RCL_RET_EVENT_INVALID;
  }

  // 检查event_info参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(event_info, RCL_RET_INVALID_ARGUMENT);

  // 调用rmw_take_event函数尝试获取事件信息
  rmw_ret_t ret = rmw_take_event(&event->impl->rmw_handle, event_info, &taken);

  // 判断rmw_take_event函数执行结果
  if (RMW_RET_OK != ret) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    return rcl_convert_rmw_ret_to_rcl_ret(ret);
  }

  // 如果没有成功获取到事件信息，则返回失败
  if (!taken) {
    RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "take_event request complete, unable to take event");
    return RCL_RET_EVENT_TAKE_FAILED;
  }

  // 成功获取到事件信息，记录日志并返回成功
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "take_event request success");
  return rcl_convert_rmw_ret_to_rcl_ret(ret);
}

/**
 * @brief 销毁事件
 *
 * @param[in,out] event 指向rcl_event_t类型的指针，表示要销毁的事件
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_event_fini(rcl_event_t *event) {
  // 定义一个变量存储操作结果，默认为成功
  rcl_ret_t result = RCL_RET_OK;

  // 检查event参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(event, RCL_RET_EVENT_INVALID);

  // 记录日志，开始销毁事件
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Finalizing event");

  // 判断事件实现是否有效
  if (NULL != event->impl) {
    // 获取事件的内存分配器
    rcl_allocator_t allocator = event->impl->allocator;

    // 调用rmw_event_fini函数销毁事件
    rmw_ret_t ret = rmw_event_fini(&event->impl->rmw_handle);

    // 判断rmw_event_fini函数执行结果
    if (ret != RMW_RET_OK) {
      RCL_SET_ERROR_MSG(rmw_get_error_string().str);
      result = rcl_convert_rmw_ret_to_rcl_ret(ret);
    }

    // 释放事件实现的内存
    allocator.deallocate(event->impl, allocator.state);
    event->impl = NULL;
  }

  // 记录日志，事件已销毁
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Event finalized");

  return result;
}

/**
 * @brief 获取事件的RMW句柄
 *
 * @param[in] event 指向rcl_event_t类型的指针，表示要获取RMW句柄的事件
 * @return rmw_event_t* 返回事件的RMW句柄，如果事件无效则返回NULL
 */
rmw_event_t *rcl_event_get_rmw_handle(const rcl_event_t *event) {
  // 检查事件是否有效
  if (!rcl_event_is_valid(event)) {
    return NULL;  // error already set
  } else {
    return &event->impl->rmw_handle;
  }
}

/**
 * @brief 判断事件是否有效
 *
 * @param[in] event 指向rcl_event_t类型的指针，表示要判断的事件
 * @return bool 如果事件有效则返回true，否则返回false
 */
bool rcl_event_is_valid(const rcl_event_t *event) {
  RCL_CHECK_FOR_NULL_WITH_MSG(event, "event pointer is invalid", return false);
  RCL_CHECK_FOR_NULL_WITH_MSG(event->impl, "event's implementation is invalid", return false);
  if (event->impl->rmw_handle.event_type == RMW_EVENT_INVALID) {
    RCUTILS_SET_ERROR_MSG("event's implementation not init");
    return false;
  }
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(&event->impl->allocator, "not valid allocator", return false);
  return true;
}

/**
 * @brief 设置事件回调函数
 *
 * @param[in] event 指向rcl_event_t类型的指针，表示要设置回调函数的事件
 * @param[in] callback 回调函数指针
 * @param[in] user_data 用户数据指针
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_event_set_callback(
    const rcl_event_t *event, rcl_event_callback_t callback, const void *user_data) {
  // 检查事件是否有效
  if (!rcl_event_is_valid(event)) {
    // error state already set
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 调用rmw_event_set_callback函数设置回调函数
  return rmw_event_set_callback(&event->impl->rmw_handle, callback, user_data);
}

#ifdef __cplusplus
}
#endif
