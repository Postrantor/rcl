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

#include "rcl/client.h"

#include <stdio.h>
#include <string.h>

#include "./common.h"
#include "./service_event_publisher.h"
#include "rcl/error_handling.h"
#include "rcl/node.h"
#include "rcl/publisher.h"
#include "rcl/time.h"
#include "rcutils/logging_macros.h"
#include "rcutils/macros.h"
#include "rcutils/stdatomic_helper.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rosidl_runtime_c/service_type_support_struct.h"
#include "service_msgs/msg/service_event_info.h"
#include "tracetools/tracetools.h"

/**
 * @struct rcl_client_impl_s
 * @brief ROS2客户端实现结构体
 */
struct rcl_client_impl_s {
  rcl_client_options_t options;                            ///< 客户端选项
  rmw_qos_profile_t actual_request_publisher_qos;          ///< 请求发布者的QoS配置
  rmw_qos_profile_t actual_response_subscription_qos;      ///< 响应订阅者的QoS配置
  rmw_client_t *rmw_handle;                                ///< RMW客户端句柄
  atomic_int_least64_t sequence_number;                    ///< 序列号
  rcl_service_event_publisher_t *service_event_publisher;  ///< 服务事件发布者
  char *remapped_service_name;                             ///< 重映射后的服务名称
};

/**
 * @brief 获取一个零初始化的rcl_client_t对象
 *
 * @return 返回一个零初始化的rcl_client_t对象
 */
rcl_client_t rcl_get_zero_initialized_client() {
  static rcl_client_t null_client = {0};
  return null_client;
}

/**
 * @brief 取消配置服务内省
 *
 * @param[in] node 指向rcl_node_t类型的指针
 * @param[in] client_impl 指向rcl_client_impl_s类型的指针
 * @param[in] allocator 指向rcl_allocator_t类型的指针
 * @return 返回rcl_ret_t类型的结果，表示操作成功或失败
 */
static rcl_ret_t unconfigure_service_introspection(
    rcl_node_t *node, struct rcl_client_impl_s *client_impl, rcl_allocator_t *allocator) {
  // 如果client_impl为空，则返回错误
  if (client_impl == NULL) {
    return RCL_RET_ERROR;
  }

  // 如果service_event_publisher为空，则返回成功
  if (client_impl->service_event_publisher == NULL) {
    return RCL_RET_OK;
  }

  // 结束服务事件发布者并获取结果
  rcl_ret_t ret = rcl_service_event_publisher_fini(client_impl->service_event_publisher, node);

  // 释放内存
  allocator->deallocate(client_impl->service_event_publisher, allocator->state);
  client_impl->service_event_publisher = NULL;

  // 返回操作结果
  return ret;
}

/**
 * @brief 初始化一个rcl客户端
 *
 * @param[in] client 指向要初始化的rcl_client_t结构体的指针
 * @param[in] node 与客户端关联的rcl_node_t结构体的指针
 * @param[in] type_support 服务类型支持的指针
 * @param[in] service_name 要连接的服务的名称
 * @param[in] options 客户端选项的指针，包括分配器和QoS设置
 * @return 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_client_init(
    rcl_client_t *client,
    const rcl_node_t *node,
    const rosidl_service_type_support_t *type_support,
    const char *service_name,
    const rcl_client_options_t *options) {
  // 首先检查选项和分配器，以便将分配器传递给错误
  RCL_CHECK_ARGUMENT_FOR_NULL(options, RCL_RET_INVALID_ARGUMENT);
  rcl_allocator_t *allocator = (rcl_allocator_t *)&options->allocator;
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(client, RCL_RET_INVALID_ARGUMENT);
  if (!rcl_node_is_valid(node)) {
    return RCL_RET_NODE_INVALID;  // error already set
  }
  RCL_CHECK_ARGUMENT_FOR_NULL(type_support, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(service_name, RCL_RET_INVALID_ARGUMENT);
  RCUTILS_LOG_DEBUG_NAMED(
      ROS_PACKAGE_NAME, "Initializing client for service name '%s'", service_name);
  if (client->impl) {
    RCL_SET_ERROR_MSG("client already initialized, or memory was unintialized");
    return RCL_RET_ALREADY_INIT;
  }

  // 为实现结构体分配空间
  client->impl =
      (rcl_client_impl_t *)allocator->zero_allocate(1, sizeof(rcl_client_impl_t), allocator->state);
  RCL_CHECK_FOR_NULL_WITH_MSG(client->impl, "allocating memory failed", return RCL_RET_BAD_ALLOC;);

  // 扩展给定的服务名称
  rcl_ret_t ret = rcl_node_resolve_name(
      node, service_name, *allocator, true, false, &client->impl->remapped_service_name);
  if (ret != RCL_RET_OK) {
    if (ret == RCL_RET_SERVICE_NAME_INVALID || ret == RCL_RET_UNKNOWN_SUBSTITUTION) {
      ret = RCL_RET_SERVICE_NAME_INVALID;
    } else if (RCL_RET_BAD_ALLOC != ret) {
      ret = RCL_RET_ERROR;
    }
    goto free_client_impl;
  }
  RCUTILS_LOG_DEBUG_NAMED(
      ROS_PACKAGE_NAME, "Expanded and remapped service name '%s'",
      client->impl->remapped_service_name);

  // 填充实现结构体
  // rmw handle (create rmw client)
  // TODO(wjwwood): pass along the allocator to rmw when it supports it
  client->impl->rmw_handle = rmw_create_client(
      rcl_node_get_rmw_handle(node), type_support, client->impl->remapped_service_name,
      &options->qos);
  if (!client->impl->rmw_handle) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    ret = RCL_RET_ERROR;
    goto free_remapped_service_name;
  }

  // 获取实际的qos，并存储它
  rmw_ret_t rmw_ret = rmw_client_request_publisher_get_actual_qos(
      client->impl->rmw_handle, &client->impl->actual_request_publisher_qos);
  if (RMW_RET_OK != rmw_ret) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    ret = rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
    goto destroy_client;
  }

  rmw_ret = rmw_client_response_subscription_get_actual_qos(
      client->impl->rmw_handle, &client->impl->actual_response_subscription_qos);
  if (RMW_RET_OK != rmw_ret) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    ret = rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
    goto destroy_client;
  }

  // ROS特定的命名空间约定避免
  // 不是通过get_actual_qos获取的
  client->impl->actual_request_publisher_qos.avoid_ros_namespace_conventions =
      options->qos.avoid_ros_namespace_conventions;
  client->impl->actual_response_subscription_qos.avoid_ros_namespace_conventions =
      options->qos.avoid_ros_namespace_conventions;

  // 选项
  client->impl->options = *options;
  atomic_init(&client->impl->sequence_number, 0);
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Client initialized");
  TRACEPOINT(
      rcl_client_init, (const void *)client, (const void *)node,
      (const void *)client->impl->rmw_handle, client->impl->remapped_service_name);

  return RCL_RET_OK;

destroy_client:
  // 调用 rmw_destroy_client 销毁底层 RMW 客户端
  rmw_ret = rmw_destroy_client(rcl_node_get_rmw_handle(node), client->impl->rmw_handle);
  // 判断销毁操作是否成功
  if (RMW_RET_OK != rmw_ret) {
    // 如果失败，将错误信息输出到标准错误流
    RCUTILS_SAFE_FWRITE_TO_STDERR(rmw_get_error_string().str);
    RCUTILS_SAFE_FWRITE_TO_STDERR("\n");
  }

free_remapped_service_name:
  // 使用分配器释放 remapped_service_name 的内存
  allocator->deallocate(client->impl->remapped_service_name, allocator->state);
  // 将 remapped_service_name 设置为 NULL
  client->impl->remapped_service_name = NULL;

free_client_impl:
  // 使用分配器释放 client->impl 的内存
  allocator->deallocate(client->impl, allocator->state);
  // 将 client->impl 设置为 NULL
  client->impl = NULL;

  // 返回操作结果
  return ret;
}

/**
 * @brief 销毁一个rcl客户端实例并释放相关资源
 *
 * @param[in,out] client 指向要销毁的rcl_client_t结构体指针
 * @param[in] node 指向与客户端关联的rcl_node_t结构体指针
 * @return 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_client_fini(rcl_client_t *client, rcl_node_t *node) {
  // 可以返回以下错误类型
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_NODE_INVALID);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_ERROR);

  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Finalizing client");
  rcl_ret_t result = RCL_RET_OK;

  // 检查client参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(client, RCL_RET_INVALID_ARGUMENT);

  // 检查节点是否有效
  if (!rcl_node_is_valid_except_context(node)) {
    return RCL_RET_NODE_INVALID;  // error already set
  }

  // 如果客户端实现不为空，则进行清理操作
  if (client->impl) {
    // 获取分配器
    rcl_allocator_t allocator = client->impl->options.allocator;

    // 获取rmw节点句柄
    rmw_node_t *rmw_node = rcl_node_get_rmw_handle(node);
    if (!rmw_node) {
      return RCL_RET_INVALID_ARGUMENT;
    }

    // 取消配置服务内省
    rcl_ret_t rcl_ret = unconfigure_service_introspection(node, client->impl, &allocator);
    if (RCL_RET_OK != rcl_ret) {
      RCL_SET_ERROR_MSG(rcl_get_error_string().str);
      result = rcl_ret;
    }

    // 销毁rmw客户端实例
    rmw_ret_t ret = rmw_destroy_client(rmw_node, client->impl->rmw_handle);
    if (ret != RMW_RET_OK) {
      RCL_SET_ERROR_MSG(rmw_get_error_string().str);
      result = RCL_RET_ERROR;
    }

    // 释放重映射服务名称内存
    allocator.deallocate(client->impl->remapped_service_name, allocator.state);
    client->impl->remapped_service_name = NULL;

    // 释放客户端实现内存
    allocator.deallocate(client->impl, allocator.state);
    client->impl = NULL;
  }

  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Client finalized");
  return result;
}

/**
 * @brief 获取默认的rcl_client选项
 *
 * @return 返回一个默认的rcl_client_options_t结构体实例
 */
rcl_client_options_t rcl_client_get_default_options() {
  // !!! 确保这些默认值的更改反映在头文件文档字符串中
  static rcl_client_options_t default_options;
  // 必须在此之后设置分配器和qos，因为它们不是编译时常量。
  default_options.qos = rmw_qos_profile_services_default;
  default_options.allocator = rcl_get_default_allocator();
  return default_options;
}

/**
 * @brief 获取客户端的服务名称
 *
 * @param client 指向rcl_client_t结构体的指针
 * @return 如果客户端有效，则返回服务名称；否则返回NULL
 */
const char *rcl_client_get_service_name(const rcl_client_t *client) {
  if (!rcl_client_is_valid(client)) {
    return NULL;  // 错误已设置
  }
  return client->impl->rmw_handle->service_name;
}

// 定义一个宏，用于获取客户端选项
#define _client_get_options(client) &client->impl->options;

/**
 * @brief 获取客户端的选项
 *
 * @param client 指向rcl_client_t结构体的指针
 * @return 如果客户端有效，则返回指向rcl_client_options_t结构体的指针；否则返回NULL
 */
const rcl_client_options_t *rcl_client_get_options(const rcl_client_t *client) {
  if (!rcl_client_is_valid(client)) {
    return NULL;  // 错误已设置
  }
  return _client_get_options(client);
}

/**
 * @brief 获取客户端的rmw句柄
 *
 * @param client 指向rcl_client_t结构体的指针
 * @return 如果客户端有效，则返回指向rmw_client_t结构体的指针；否则返回NULL
 */
rmw_client_t *rcl_client_get_rmw_handle(const rcl_client_t *client) {
  if (!rcl_client_is_valid(client)) {
    return NULL;  // 错误已设置
  }
  return client->impl->rmw_handle;
}

/**
 * @brief 发送服务请求 (Send a service request)
 *
 * @param[in] client 指向有效的rcl_client_t结构体的指针 (Pointer to a valid rcl_client_t structure)
 * @param[in] ros_request 指向ROS服务请求消息的指针 (Pointer to the ROS service request message)
 * @param[out] sequence_number 请求序列号的指针，用于存储发送请求的序列号 (Pointer to the request
 * sequence number, used to store the sent request's sequence number)
 * @return 返回rcl_ret_t类型的结果，表示函数执行成功或失败 (Returns an rcl_ret_t type result,
 * indicating success or failure of the function execution)
 */
rcl_ret_t rcl_send_request(
    const rcl_client_t *client, const void *ros_request, int64_t *sequence_number) {
  // 记录调试信息 (Log debug information)
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Client sending service request");

  // 检查客户端是否有效 (Check if the client is valid)
  if (!rcl_client_is_valid(client)) {
    return RCL_RET_CLIENT_INVALID;  // error already set
  }

  // 检查ros_request和sequence_number参数是否为空 (Check if ros_request and sequence_number
  // arguments are NULL)
  RCL_CHECK_ARGUMENT_FOR_NULL(ros_request, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(sequence_number, RCL_RET_INVALID_ARGUMENT);

  // 获取当前序列号 (Get the current sequence number)
  *sequence_number = rcutils_atomic_load_int64_t(&client->impl->sequence_number);

  // 使用rmw发送请求 (Send the request using rmw)
  if (rmw_send_request(client->impl->rmw_handle, ros_request, sequence_number) != RMW_RET_OK) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    return RCL_RET_ERROR;
  }

  // 更新序列号 (Update the sequence number)
  rcutils_atomic_exchange_int64_t(&client->impl->sequence_number, *sequence_number);

  // 如果服务事件发布器不为空，则发送服务事件消息 (If the service event publisher is not NULL, send
  // the service event message)
  if (client->impl->service_event_publisher != NULL) {
    rmw_gid_t gid;
    rmw_ret_t rmw_ret = rmw_get_gid_for_client(client->impl->rmw_handle, &gid);
    if (rmw_ret != RMW_RET_OK) {
      RCL_SET_ERROR_MSG(rmw_get_error_string().str);
      return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
    }
    rcl_ret_t ret = rcl_send_service_event_message(
        client->impl->service_event_publisher, service_msgs__msg__ServiceEventInfo__REQUEST_SENT,
        ros_request, *sequence_number, gid.data);
    if (RCL_RET_OK != ret) {
      RCL_SET_ERROR_MSG(rcl_get_error_string().str);
      return ret;
    }
  }

  // 返回成功 (Return success)
  return RCL_RET_OK;
}

/**
 * @brief 从服务端接收响应，并获取相关信息。
 *
 * @param[in] client 指向有效的rcl_client_t结构体的指针，用于与服务端通信。
 * @param[out] request_header 包含请求头信息的rmw_service_info_t结构体指针。
 * @param[out] ros_response 存储接收到的服务响应的指针。
 * @return rcl_ret_t 返回操作结果，成功返回RCL_RET_OK，否则返回相应的错误代码。
 */
rcl_ret_t rcl_take_response_with_info(
    const rcl_client_t *client, rmw_service_info_t *request_header, void *ros_response) {
  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Client taking service response");

  // 检查客户端是否有效
  if (!rcl_client_is_valid(client)) {
    return RCL_RET_CLIENT_INVALID;  // error already set
  }

  // 检查参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(request_header, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(ros_response, RCL_RET_INVALID_ARGUMENT);

  // 初始化变量
  bool taken = false;
  request_header->source_timestamp = 0;
  request_header->received_timestamp = 0;

  // 从服务端接收响应
  if (rmw_take_response(client->impl->rmw_handle, request_header, ros_response, &taken) !=
      RMW_RET_OK) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    return RCL_RET_ERROR;
  }

  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(
      ROS_PACKAGE_NAME, "Client take response succeeded: %s", taken ? "true" : "false");

  // 检查是否成功接收响应
  if (!taken) {
    return RCL_RET_CLIENT_TAKE_FAILED;
  }

  // 发送服务事件消息
  if (client->impl->service_event_publisher != NULL) {
    // 定义一个rmw_gid_t类型的变量gid
    rmw_gid_t gid;
    // 获取客户端的全局唯一标识符（GID）
    rmw_ret_t rmw_ret = rmw_get_gid_for_client(client->impl->rmw_handle, &gid);
    // 检查获取GID操作是否成功
    if (rmw_ret != RMW_RET_OK) {
      // 设置错误信息
      RCL_SET_ERROR_MSG(rmw_get_error_string().str);
      // 将RMW返回值转换为RCL返回值并返回
      return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
    }
    // 调用rcl_send_service_event_message发送服务事件消息
    rcl_ret_t ret = rcl_send_service_event_message(
        client->impl->service_event_publisher,
        service_msgs__msg__ServiceEventInfo__RESPONSE_RECEIVED, ros_response,
        request_header->request_id.sequence_number, gid.data);
    // 检查发送操作是否成功
    if (RCL_RET_OK != ret) {
      // 设置错误信息
      RCL_SET_ERROR_MSG(rcl_get_error_string().str);
      // 返回错误代码
      return ret;
    }
  }

  // 返回操作结果
  return RCL_RET_OK;
}

/**
 * @brief 从服务端接收响应。
 *
 * @param[in] client 指向rcl_client_t结构体的指针，用于与服务端通信。
 * @param[out] request_header 指向rmw_request_id_t结构体的指针，存储请求头信息。
 * @param[out] ros_response 存储接收到的响应消息的指针。
 * @return 返回rcl_ret_t类型的结果，表示操作成功或失败。
 */
rcl_ret_t rcl_take_response(
    const rcl_client_t *client, rmw_request_id_t *request_header, void *ros_response) {
  // 定义一个rmw_service_info_t类型的变量header，并将request_header的值赋给它
  rmw_service_info_t header;
  header.request_id = *request_header;

  // 调用rcl_take_response_with_info函数，并将结果存储在ret中
  rcl_ret_t ret = rcl_take_response_with_info(client, &header, ros_response);

  // 将header的request_id值更新为新的request_header值
  *request_header = header.request_id;

  // 返回操作结果
  return ret;
}

/**
 * @brief 检查客户端是否有效。
 *
 * @param[in] client 指向rcl_client_t结构体的指针。
 * @return 如果客户端有效，则返回true，否则返回false。
 */
bool rcl_client_is_valid(const rcl_client_t *client) {
  // 检查client指针是否为空，如果为空则返回错误信息并返回false
  RCL_CHECK_FOR_NULL_WITH_MSG(client, "client pointer is invalid", return false);

  // 检查client的实现是否为空，如果为空则返回错误信息并返回false
  RCL_CHECK_FOR_NULL_WITH_MSG(client->impl, "client's rmw implementation is invalid", return false);

  // 检查client的rmw句柄是否为空，如果为空则返回错误信息并返回false
  RCL_CHECK_FOR_NULL_WITH_MSG(
      client->impl->rmw_handle, "client's rmw handle is invalid", return false);

  // 如果以上检查都通过，则返回true
  return true;
}

/**
 * @brief 获取客户端请求发布器的实际QoS配置。
 *
 * @param[in] client 指向rcl_client_t结构体的指针。
 * @return 返回指向rmw_qos_profile_t结构体的指针，表示请求发布器的实际QoS配置。
 */
const rmw_qos_profile_t *rcl_client_request_publisher_get_actual_qos(const rcl_client_t *client) {
  // 检查客户端是否有效，如果无效则返回NULL
  if (!rcl_client_is_valid(client)) {
    return NULL;
  }

  // 返回客户端请求发布器的实际QoS配置
  return &client->impl->actual_request_publisher_qos;
}

/**
 * @brief 获取客户端响应订阅器的实际QoS配置。
 *
 * @param[in] client 指向rcl_client_t结构体的指针。
 * @return 返回指向rmw_qos_profile_t结构体的指针，表示响应订阅器的实际QoS配置。
 */
const rmw_qos_profile_t *rcl_client_response_subscription_get_actual_qos(
    const rcl_client_t *client) {
  // 检查客户端是否有效，如果无效则返回NULL
  if (!rcl_client_is_valid(client)) {
    return NULL;
  }

  // 返回客户端响应订阅器的实际QoS配置
  return &client->impl->actual_response_subscription_qos;
}

/**
 * @brief 设置客户端的新响应回调函数
 *
 * @param[in] client 指向有效的rcl_client_t结构体的指针
 * @param[in] callback 新响应回调函数
 * @param[in] user_data 用户数据，将传递给回调函数
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_client_set_on_new_response_callback(
    const rcl_client_t *client, rcl_event_callback_t callback, const void *user_data) {
  // 检查客户端是否有效
  if (!rcl_client_is_valid(client)) {
    // 错误状态已设置
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 设置新响应回调函数
  return rmw_client_set_on_new_response_callback(client->impl->rmw_handle, callback, user_data);
}

/**
 * @brief 配置客户端服务自省功能
 *
 * @param[in,out] client 指向有效的rcl_client_t结构体的指针
 * @param[in] node 指向有效的rcl_node_t结构体的指针
 * @param[in] clock 指向有效的rcl_clock_t结构体的指针
 * @param[in] type_support 指向rosidl_service_type_support_t结构体的指针
 * @param[in] publisher_options 发布者选项
 * @param[in] introspection_state 自省状态
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_client_configure_service_introspection(
    rcl_client_t *client,
    rcl_node_t *node,
    rcl_clock_t *clock,
    const rosidl_service_type_support_t *type_support,
    const rcl_publisher_options_t publisher_options,
    rcl_service_introspection_state_t introspection_state) {
  // 检查客户端是否有效
  if (!rcl_client_is_valid(client)) {
    return RCL_RET_CLIENT_INVALID;  // 错误已设置
  }
  // 检查参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(node, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(clock, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(type_support, RCL_RET_INVALID_ARGUMENT);

  // 获取分配器
  rcl_allocator_t allocator = client->impl->options.allocator;

  // 如果自省状态为关闭，则取消配置服务自省功能
  if (introspection_state == RCL_SERVICE_INTROSPECTION_OFF) {
    return unconfigure_service_introspection(node, client->impl, &allocator);
  }

  // 如果服务事件发布者为空，说明我们还没有进行自省，需要分配服务事件发布者
  if (client->impl->service_event_publisher == NULL) {
    client->impl->service_event_publisher =
        allocator.allocate(sizeof(rcl_service_event_publisher_t), allocator.state);
    // 检查分配结果
    RCL_CHECK_FOR_NULL_WITH_MSG(client->impl->service_event_publisher, "allocating memory failed",
                                return RCL_RET_BAD_ALLOC;);

    // 初始化服务事件发布者
    *client->impl->service_event_publisher = rcl_get_zero_initialized_service_event_publisher();
    rcl_ret_t ret = rcl_service_event_publisher_init(
        client->impl->service_event_publisher, node, clock, publisher_options,
        client->impl->remapped_service_name, type_support);
    // 如果初始化失败，释放内存并返回错误
    if (RCL_RET_OK != ret) {
      allocator.deallocate(client->impl->service_event_publisher, allocator.state);
      client->impl->service_event_publisher = NULL;
      return ret;
    }
  }

  // 改变服务事件发布者的状态
  return rcl_service_event_publisher_change_state(
      client->impl->service_event_publisher, introspection_state);
}

#ifdef __cplusplus
}
#endif
