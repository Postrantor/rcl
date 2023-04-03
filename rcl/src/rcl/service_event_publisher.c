// Copyright 2022 Open Source Robotics Foundation, Inc.
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

#include "rcl/service_event_publisher.h"

#include <string.h>

#include "rcl/allocator.h"
#include "rcl/error_handling.h"
#include "rcl/macros.h"
#include "rcl/node.h"
#include "rcl/publisher.h"
#include "rcl/service_introspection.h"
#include "rcl/time.h"
#include "rcl/types.h"
#include "rcutils/logging_macros.h"
#include "rcutils/macros.h"
#include "rmw/error_handling.h"
#include "service_msgs/msg/service_event_info.h"

/**
 * @brief 获取一个初始化为零的服务事件发布器 (Get a zero-initialized service event publisher)
 *
 * @return 返回一个初始化为零的服务事件发布器 (Return a zero-initialized service event publisher)
 */
rcl_service_event_publisher_t rcl_get_zero_initialized_service_event_publisher() {
  // 定义并初始化一个静态的服务事件发布器结构体变量 (Define and initialize a static service event
  // publisher struct variable)
  static rcl_service_event_publisher_t zero_service_event_publisher = {0};

  // 返回初始化为零的服务事件发布器 (Return the zero-initialized service event publisher)
  return zero_service_event_publisher;
}

/**
 * @brief 检查服务事件发布器是否有效 (Check if the service event publisher is valid)
 *
 * @param[in] service_event_publisher 指向服务事件发布器的指针 (Pointer to the service event
 * publisher)
 * @return 如果服务事件发布器有效，则返回 true，否则返回 false (Return true if the service event
 * publisher is valid, otherwise return false)
 */
bool rcl_service_event_publisher_is_valid(
    const rcl_service_event_publisher_t *service_event_publisher) {
  // 检查 service_event_publisher 是否为空，如果为空，返回错误消息并返回 false (Check if
  // service_event_publisher is NULL, if it is, return an error message and return false)
  RCL_CHECK_FOR_NULL_WITH_MSG(
      service_event_publisher, "service_event_publisher is invalid", return false);

  // 检查 service_event_publisher 的 service_type_support 是否为空，如果为空，返回错误消息并返回
  // false (Check if service_event_publisher's service_type_support is NULL, if it is, return an
  // error message and return false)
  RCL_CHECK_FOR_NULL_WITH_MSG(
      service_event_publisher->service_type_support,
      "service_event_publisher's service type support is invalid", return false);

  // 检查 service_event_publisher 的 clock 是否有效，如果无效，设置错误消息并返回 false (Check if
  // service_event_publisher's clock is valid, if it's not, set an error message and return false)
  if (!rcl_clock_valid(service_event_publisher->clock)) {
    RCL_SET_ERROR_MSG("service_event_publisher's clock is invalid");
    return false;
  }

  // 如果所有检查都通过，则返回 true 表示服务事件发布器有效 (If all checks pass, return true
  // indicating the service event publisher is valid)
  return true;
}

/**
 * @brief 创建一个用于发布服务事件的publisher
 *
 * @param[in,out] service_event_publisher 服务事件发布器指针，用于存储创建的publisher信息
 * @param[in] node ROS2节点指针，用于初始化publisher
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
static rcl_ret_t introspection_create_publisher(
    rcl_service_event_publisher_t *service_event_publisher, const rcl_node_t *node) {
  // 获取分配器
  rcl_allocator_t allocator = service_event_publisher->publisher_options.allocator;
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(&allocator, "allocator is invalid", return RCL_RET_ERROR);

  // 分配内存空间给publisher
  service_event_publisher->publisher = allocator.allocate(sizeof(rcl_publisher_t), allocator.state);
  // 检查分配结果是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(
      service_event_publisher->publisher, "allocate service_event_publisher failed in enable",
      return RCL_RET_BAD_ALLOC);
  // 初始化publisher结构体为零值
  *service_event_publisher->publisher = rcl_get_zero_initialized_publisher();
  // 初始化publisher
  rcl_ret_t ret = rcl_publisher_init(
      service_event_publisher->publisher, node,
      service_event_publisher->service_type_support->event_typesupport,
      service_event_publisher->service_event_topic_name,
      &service_event_publisher->publisher_options);
  // 检查初始化结果
  if (RCL_RET_OK != ret) {
    // 如果失败，释放之前分配的内存空间
    allocator.deallocate(service_event_publisher->publisher, allocator.state);
    service_event_publisher->publisher = NULL;
    // 重置错误信息
    rcutils_reset_error();
    // 设置错误消息
    RCL_SET_ERROR_MSG(rcl_get_error_string().str);
    return ret;
  }

  return RCL_RET_OK;
}

/**
 * @brief 初始化服务事件发布器 (Initialize the service event publisher)
 *
 * @param[in] service_event_publisher 服务事件发布器指针 (Pointer to the service event publisher)
 * @param[in] node ROS2节点指针 (Pointer to the ROS2 node)
 * @param[in] clock 时钟指针 (Pointer to the clock)
 * @param[in] publisher_options 发布器选项 (Publisher options)
 * @param[in] service_name 服务名称 (Service name)
 * @param[in] service_type_support 服务类型支持指针 (Pointer to the service type support)
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败 (Return RCL_RET_OK for success, other
 * values indicate failure)
 */
rcl_ret_t rcl_service_event_publisher_init(
    rcl_service_event_publisher_t *service_event_publisher,
    const rcl_node_t *node,
    rcl_clock_t *clock,
    const rcl_publisher_options_t publisher_options,
    const char *service_name,
    const rosidl_service_type_support_t *service_type_support) {
  // 检查可能的错误返回值 (Check possible error return values)
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_ALREADY_INIT);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_NODE_INVALID);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_BAD_ALLOC);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_ERROR);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_TOPIC_NAME_INVALID);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_PUBLISHER_INVALID);

  // 检查输入参数是否为空 (Check if input arguments are NULL)
  RCL_CHECK_ARGUMENT_FOR_NULL(service_event_publisher, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(node, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(service_name, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(service_type_support, RCL_RET_INVALID_ARGUMENT);

  // 检查分配器是否有效 (Check if allocator is valid)
  RCL_CHECK_ALLOCATOR_WITH_MSG(
      &publisher_options.allocator, "allocator is invalid", return RCL_RET_ERROR);

  rcl_allocator_t allocator = publisher_options.allocator;

  rcl_ret_t ret = RCL_RET_OK;

  // 检查节点是否有效 (Check if node is valid)
  if (!rcl_node_is_valid(node)) {
    return RCL_RET_NODE_INVALID;
  }

  // 检查时钟是否有效 (Check if clock is valid)
  if (!rcl_clock_valid(clock)) {
    rcutils_reset_error();
    RCL_SET_ERROR_MSG("clock is invalid");
    return RCL_RET_ERROR;
  }

  // 初始化服务内省 (Initialize service introspection)
  RCUTILS_LOG_DEBUG_NAMED(
      ROS_PACKAGE_NAME, "Initializing service introspection for service name '%s'", service_name);

  // 类型支持具有静态生命周期 (Typesupports have static lifetimes)
  service_event_publisher->service_type_support = service_type_support;
  service_event_publisher->clock = clock;
  service_event_publisher->publisher_options = publisher_options;

  // 计算主题长度 (Calculate topic length)
  size_t topic_length = strlen(service_name) + strlen(RCL_SERVICE_INTROSPECTION_TOPIC_POSTFIX) + 1;
  service_event_publisher->service_event_topic_name =
      (char *)allocator.allocate(topic_length, allocator.state);
  RCL_CHECK_FOR_NULL_WITH_MSG(service_event_publisher->service_event_topic_name,
                              "allocating memory for service introspection topic name failed",
                              return RCL_RET_BAD_ALLOC;);

  // 构建服务事件主题名称 (Build service event topic name)
  snprintf(
      service_event_publisher->service_event_topic_name, topic_length, "%s%s", service_name,
      RCL_SERVICE_INTROSPECTION_TOPIC_POSTFIX);

  // 创建内省发布器 (Create introspection publisher)
  ret = introspection_create_publisher(service_event_publisher, node);
  if (ret != RCL_RET_OK) {
    goto free_topic_name;
  }

  // 初始化服务内省完成 (Service introspection initialized)
  RCUTILS_LOG_DEBUG_NAMED(
      ROS_PACKAGE_NAME, "Service introspection for service '%s' initialized", service_name);

  return RCL_RET_OK;

free_topic_name:
  // 释放主题名称内存 (Free topic name memory)
  allocator.deallocate(service_event_publisher->service_event_topic_name, allocator.state);

  return ret;
}

/**
 * @brief 终止并清理服务事件发布器 (Finalize and clean up the service event publisher)
 *
 * @param[in,out] service_event_publisher 指向要终止的服务事件发布器指针 (Pointer to the service
 * event publisher to be finalized)
 * @param[in] node 与服务事件发布器关联的节点 (Node associated with the service event publisher)
 * @return rcl_ret_t 返回操作结果 (Return the result of the operation)
 */
rcl_ret_t rcl_service_event_publisher_fini(
    rcl_service_event_publisher_t *service_event_publisher, rcl_node_t *node) {
  // 可以返回以下错误类型 (Can return the following error types)
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_ALREADY_SHUTDOWN);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_NODE_INVALID);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_PUBLISHER_INVALID);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_ERROR);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_BAD_ALLOC);

  // 检查服务事件发布器是否有效 (Check if the service event publisher is valid)
  if (!rcl_service_event_publisher_is_valid(service_event_publisher)) {
    return RCL_RET_ERROR;
  }

  // 检查节点是否有效，除了上下文 (Check if the node is valid, except for the context)
  if (!rcl_node_is_valid_except_context(node)) {
    return RCL_RET_NODE_INVALID;
  }

  // 获取分配器 (Get the allocator)
  rcl_allocator_t allocator = service_event_publisher->publisher_options.allocator;
  // 检查分配器是否有效 (Check if the allocator is valid)
  RCL_CHECK_ALLOCATOR_WITH_MSG(&allocator, "allocator is invalid", return RCL_RET_ERROR);

  // 如果发布器存在 (If the publisher exists)
  if (service_event_publisher->publisher) {
    // 终止并清理发布器 (Finalize and clean up the publisher)
    rcl_ret_t ret = rcl_publisher_fini(service_event_publisher->publisher, node);
    // 释放发布器内存 (Deallocate the publisher memory)
    allocator.deallocate(service_event_publisher->publisher, allocator.state);
    // 将发布器指针设置为 NULL (Set the publisher pointer to NULL)
    service_event_publisher->publisher = NULL;
    // 如果操作失败，返回错误 (If the operation fails, return the error)
    if (RCL_RET_OK != ret) {
      return ret;
    }
  }

  // 释放服务事件主题名称内存 (Deallocate the service event topic name memory)
  allocator.deallocate(service_event_publisher->service_event_topic_name, allocator.state);
  // 将服务事件主题名称指针设置为 NULL (Set the service event topic name pointer to NULL)
  service_event_publisher->service_event_topic_name = NULL;

  // 返回操作成功 (Return operation success)
  return RCL_RET_OK;
}

/**
 * @brief 发送服务事件消息
 *
 * @param[in] service_event_publisher 服务事件发布器指针
 * @param[in] event_type 事件类型
 * @param[in] ros_response_request ROS响应或请求的指针
 * @param[in] sequence_number 序列号
 * @param[in] guid GUID数组，长度为16
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_send_service_event_message(
    const rcl_service_event_publisher_t *service_event_publisher,
    const uint8_t event_type,
    const void *ros_response_request,
    const int64_t sequence_number,
    const uint8_t guid[16]) {
  // 可以返回以下错误类型
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_PUBLISHER_INVALID);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_ERROR);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_BAD_ALLOC);

  // 检查ros_response_request是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(ros_response_request, RCL_RET_INVALID_ARGUMENT);
  // 检查guid是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(guid, "guid is NULL", return RCL_RET_INVALID_ARGUMENT);

  // 检查服务事件发布器是否有效
  if (!rcl_service_event_publisher_is_valid(service_event_publisher)) {
    return RCL_RET_ERROR;
  }

  // 检查服务内省状态是否关闭
  if (service_event_publisher->introspection_state == RCL_SERVICE_INTROSPECTION_OFF) {
    return RCL_RET_ERROR;
  }

  // 获取分配器
  rcl_allocator_t allocator = service_event_publisher->publisher_options.allocator;
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(&allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);

  // 检查发布器是否有效
  if (!rcl_publisher_is_valid(service_event_publisher->publisher)) {
    return RCL_RET_PUBLISHER_INVALID;
  }

  rcl_ret_t ret;

  // 获取当前时间
  rcl_time_point_value_t now;
  ret = rcl_clock_get_now(service_event_publisher->clock, &now);
  if (RMW_RET_OK != ret) {
    rcutils_reset_error();
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    return RCL_RET_ERROR;
  }

  // 设置服务内省信息
  rosidl_service_introspection_info_t info = {
      .event_type = event_type,
      .stamp_sec = (int32_t)RCL_NS_TO_S(now),
      .stamp_nanosec = now % (1000LL * 1000LL * 1000LL),
      .sequence_number = sequence_number,
  };

  // 复制GUID
  memcpy(info.client_gid, guid, 16);

  void *service_introspection_message;
  if (service_event_publisher->introspection_state == RCL_SERVICE_INTROSPECTION_METADATA) {
    ros_response_request = NULL;
  }
  /**
   * @brief 根据事件类型创建服务内省消息
   *
   * @param[in] event_type 服务事件类型，用于确定创建的内省消息类型
   * @param[out] service_introspection_message 创建的服务内省消息
   * @param[in] service_event_publisher 服务事件发布器，包含服务类型支持信息
   * @param[in] info 服务事件信息
   * @param[in] allocator 内存分配器
   * @param[in] ros_response_request ROS响应或请求消息
   *
   * @return rcl_ret_t 返回执行结果，成功或错误代码
   */
  switch (event_type) {
    // 请求接收或发送时
    case service_msgs__msg__ServiceEventInfo__REQUEST_RECEIVED:
    case service_msgs__msg__ServiceEventInfo__REQUEST_SENT:
      // 使用服务类型支持的事件消息创建函数创建内省消息
      service_introspection_message =
          service_event_publisher->service_type_support->event_message_create_handle_function(
              &info, &allocator, ros_response_request, NULL);
      break;
    // 响应接收或发送时
    case service_msgs__msg__ServiceEventInfo__RESPONSE_RECEIVED:
    case service_msgs__msg__ServiceEventInfo__RESPONSE_SENT:
      // 使用服务类型支持的事件消息创建函数创建内省消息
      service_introspection_message =
          service_event_publisher->service_type_support->event_message_create_handle_function(
              &info, &allocator, NULL, ros_response_request);
      break;
    // 不支持的事件类型
    default:
      // 重置错误信息
      rcutils_reset_error();
      // 设置错误消息
      RCL_SET_ERROR_MSG("unsupported event type");
      // 返回错误代码
      return RCL_RET_ERROR;
  }
  // 检查服务内省消息是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(
      service_introspection_message, "service_introspection_message is NULL", return RCL_RET_ERROR);

  // 发布服务内省消息
  ret = rcl_publish(service_event_publisher->publisher, service_introspection_message, NULL);
  // 销毁服务内省消息并检查错误
  service_event_publisher->service_type_support->event_message_destroy_handle_function(
      service_introspection_message, &allocator);
  if (RCL_RET_OK != ret) {
    rcutils_reset_error();
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
  }

  return ret;
}

/**
 * @brief 更改服务事件发布器的状态 (Change the state of a service event publisher)
 *
 * @param[in,out] service_event_publisher 服务事件发布器指针 (Pointer to the service event
 * publisher)
 * @param[in] introspection_state 新的内省状态 (New introspection state)
 *
 * @return 返回操作结果 (Return operation result)
 * - RCL_RET_OK: 操作成功 (Operation succeeded)
 * - RCL_RET_ERROR: 操作失败，服务事件发布器无效 (Operation failed, service event publisher is
 * invalid)
 */
rcl_ret_t rcl_service_event_publisher_change_state(
    rcl_service_event_publisher_t *service_event_publisher,
    rcl_service_introspection_state_t introspection_state) {
  // 检查服务事件发布器是否有效 (Check if the service event publisher is valid)
  if (!rcl_service_event_publisher_is_valid(service_event_publisher)) {
    return RCL_RET_ERROR;  // 无效时返回错误 (Return error if invalid)
  }

  // 更新服务事件发布器的内省状态 (Update the introspection state of the service event publisher)
  service_event_publisher->introspection_state = introspection_state;

  // 返回操作成功 (Return operation success)
  return RCL_RET_OK;
}
