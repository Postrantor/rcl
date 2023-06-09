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

#include "rcl/service.h"

#include <stdio.h>
#include <string.h>

#include "./common.h"
#include "./service_event_publisher.h"
#include "rcl/error_handling.h"
#include "rcl/node.h"
#include "rcl/publisher.h"
#include "rcl/time.h"
#include "rcl/types.h"
#include "rcutils/logging_macros.h"
#include "rcutils/macros.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rosidl_runtime_c/service_type_support_struct.h"
#include "service_msgs/msg/service_event_info.h"
#include "tracetools/tracetools.h"

/**
 * @struct rcl_service_impl_s
 * @brief ROS2服务实现的结构体
 *
 * @var rcl_service_options_t options
 * @brief 服务选项，包括分配器、服务名等
 * @var rmw_qos_profile_t actual_request_subscription_qos
 * @brief 请求订阅的实际QoS配置
 * @var rmw_qos_profile_t actual_response_publisher_qos
 * @brief 响应发布者的实际QoS配置
 * @var rmw_service_t * rmw_handle
 * @brief 指向rmw服务实例的指针
 * @var rcl_service_event_publisher_t * service_event_publisher
 * @brief 指向服务事件发布者实例的指针
 * @var char * remapped_service_name
 * @brief 重映射后的服务名称
 */
struct rcl_service_impl_s {
  rcl_service_options_t options;
  rmw_qos_profile_t actual_request_subscription_qos;
  rmw_qos_profile_t actual_response_publisher_qos;
  rmw_service_t *rmw_handle;
  rcl_service_event_publisher_t *service_event_publisher;
  char *remapped_service_name;
};

/**
 * @brief 获取一个零初始化的rcl_service_t实例
 * @return 返回一个零初始化的rcl_service_t实例
 */
rcl_service_t rcl_get_zero_initialized_service() {
  static rcl_service_t null_service = {0};
  return null_service;
}

/**
 * @brief 取消配置服务内省
 * @param[in] node 指向rcl_node_t实例的指针
 * @param[in] service_impl 指向rcl_service_impl_s实例的指针
 * @param[in] allocator 指向rcl_allocator_t实例的指针
 * @return 返回一个rcl_ret_t类型的结果，表示操作成功或失败
 */
static rcl_ret_t unconfigure_service_introspection(
    rcl_node_t *node, struct rcl_service_impl_s *service_impl, rcl_allocator_t *allocator) {
  // 检查service_impl是否为空
  if (service_impl == NULL) {
    return RCL_RET_ERROR;
  }

  // 如果服务事件发布者为空，则返回RCL_RET_OK
  if (service_impl->service_event_publisher == NULL) {
    return RCL_RET_OK;
  }

  // 结束服务事件发布者并获取返回值
  rcl_ret_t ret = rcl_service_event_publisher_fini(service_impl->service_event_publisher, node);

  // 使用分配器释放服务事件发布者内存
  allocator->deallocate(service_impl->service_event_publisher, allocator->state);
  // 将服务事件发布者指针设置为NULL
  service_impl->service_event_publisher = NULL;

  // 返回操作结果
  return ret;
}

/**
 * @brief 初始化一个服务 (Initialize a service)
 *
 * @param[in] service 指向要初始化的服务结构体的指针 (Pointer to the service structure to be
 * initialized)
 * @param[in] node 与服务关联的节点 (Node associated with the service)
 * @param[in] type_support 服务类型支持 (Service type support)
 * @param[in] service_name 服务名称 (Service name)
 * @param[in] options 服务选项，包括分配器和QoS设置 (Service options, including allocator and QoS
 * settings)
 * @return 返回RCL_RET_OK表示成功，其他值表示失败 (Returns RCL_RET_OK on success, other values
 * indicate failure)
 */
rcl_ret_t rcl_service_init(
    rcl_service_t *service,
    const rcl_node_t *node,
    const rosidl_service_type_support_t *type_support,
    const char *service_name,
    const rcl_service_options_t *options) {
  // 可以返回的错误类型 (Error types that can be returned)
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_ALREADY_INIT);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_NODE_INVALID);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_BAD_ALLOC);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_ERROR);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_SERVICE_NAME_INVALID);

  // 首先检查选项和分配器，以便在错误中使用分配器 (Check options and allocator first, so the
  // allocator can be used in errors)
  RCL_CHECK_ARGUMENT_FOR_NULL(options, RCL_RET_INVALID_ARGUMENT);
  rcl_allocator_t *allocator = (rcl_allocator_t *)&options->allocator;
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);

  // 检查参数是否为空 (Check if arguments are NULL)
  RCL_CHECK_ARGUMENT_FOR_NULL(service, RCL_RET_INVALID_ARGUMENT);
  if (!rcl_node_is_valid(node)) {
    return RCL_RET_NODE_INVALID;  // error already set
  }
  RCL_CHECK_ARGUMENT_FOR_NULL(type_support, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(service_name, RCL_RET_INVALID_ARGUMENT);
  // 初始化服务日志 (Initialize service log)
  RCUTILS_LOG_DEBUG_NAMED(
      ROS_PACKAGE_NAME, "Initializing service for service name '%s'", service_name);
  if (service->impl) {
    RCL_SET_ERROR_MSG("service already initialized, or memory was unintialized");
    return RCL_RET_ALREADY_INIT;
  }

  // 为实现结构体分配空间 (Allocate space for the implementation struct)
  service->impl = (rcl_service_impl_t *)allocator->zero_allocate(
      1, sizeof(rcl_service_impl_t), allocator->state);
  RCL_CHECK_FOR_NULL_WITH_MSG(service->impl, "allocating memory failed", return RCL_RET_BAD_ALLOC;);

  // 扩展并重新映射给定的服务名称 (Expand and remap the given service name)
  rcl_ret_t ret = rcl_node_resolve_name(
      node, service_name, *allocator, true, false, &service->impl->remapped_service_name);
  if (ret != RCL_RET_OK) {
    if (ret == RCL_RET_SERVICE_NAME_INVALID || ret == RCL_RET_UNKNOWN_SUBSTITUTION) {
      ret = RCL_RET_SERVICE_NAME_INVALID;
    } else if (ret != RCL_RET_BAD_ALLOC) {
      ret = RCL_RET_ERROR;
    }
    goto free_service_impl;
  }
  // 日志记录扩展和重新映射的服务名称 (Log the expanded and remapped service name)
  RCUTILS_LOG_DEBUG_NAMED(
      ROS_PACKAGE_NAME, "Expanded and remapped service name '%s'",
      service->impl->remapped_service_name);

  // 检查QoS策略 (Check QoS policy)
  if (RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL == options->qos.durability) {
    RCUTILS_LOG_WARN_NAMED(
        ROS_PACKAGE_NAME,
        "Warning: Setting QoS durability to 'transient local' for service servers "
        "can cause them to receive requests from clients that have since terminated.");
  }
  // 填充实现结构体 (Fill out implementation struct)
  // rmw handle (create rmw service)
  // TODO(wjwwood): pass along the allocator to rmw when it supports it
  service->impl->rmw_handle = rmw_create_service(
      rcl_node_get_rmw_handle(node), type_support, service->impl->remapped_service_name,
      &options->qos);
  if (!service->impl->rmw_handle) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    ret = RCL_RET_ERROR;
    goto free_remapped_service_name;
  }

  // 获取实际的QoS，并存储它 (Get actual qos, and store it)
  rmw_ret_t rmw_ret = rmw_service_request_subscription_get_actual_qos(
      service->impl->rmw_handle, &service->impl->actual_request_subscription_qos);
  if (RMW_RET_OK != rmw_ret) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    ret = rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
    goto destroy_service;
  }

  rmw_ret = rmw_service_response_publisher_get_actual_qos(
      service->impl->rmw_handle, &service->impl->actual_response_publisher_qos);
  if (RMW_RET_OK != rmw_ret) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    ret = rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
    goto destroy_service;
  }

  // ROS特定的命名空间约定不是通过get_actual_qos获取的 (ROS specific namespacing conventions is not
  // retrieved by get_actual_qos)
  service->impl->actual_request_subscription_qos.avoid_ros_namespace_conventions =
      options->qos.avoid_ros_namespace_conventions;
  service->impl->actual_response_publisher_qos.avoid_ros_namespace_conventions =
      options->qos.avoid_ros_namespace_conventions;

  // options
  service->impl->options = *options;
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Service initialized");
  TRACEPOINT(
      rcl_service_init, (const void *)service, (const void *)node,
      (const void *)service->impl->rmw_handle, service->impl->remapped_service_name);

  return RCL_RET_OK;

// 销毁服务 (Destroy a service)
destroy_service:
  // 销毁底层RMW服务 (Destroy the underlying RMW service)
  rmw_ret_t rmw_ret = rmw_destroy_service(rcl_node_get_rmw_handle(node), service->impl->rmw_handle);
  // 检查销毁结果 (Check the result of the destruction)
  if (RMW_RET_OK != rmw_ret) {
    // 如果失败，将错误信息输出到标准错误流 (If failed, output the error message to the standard
    // error stream)
    RCUTILS_SAFE_FWRITE_TO_STDERR(rmw_get_error_string().str);
    RCUTILS_SAFE_FWRITE_TO_STDERR("\n");
  }

// 释放重新映射的服务名称内存 (Free the memory of the remapped service name)
free_remapped_service_name:
  allocator->deallocate(service->impl->remapped_service_name, allocator->state);
  service->impl->remapped_service_name = NULL;

// 释放服务实现结构体内存 (Free the memory of the service implementation structure)
free_service_impl:
  allocator->deallocate(service->impl, allocator->state);
  service->impl = NULL;

  // 返回操作结果 (Return the result of the operation)
  return ret;
}
}

/**
 * @brief 销毁一个rcl服务实例 (Finalize an rcl service instance)
 *
 * @param[in,out] service 指向要销毁的rcl_service_t结构体的指针 (Pointer to the rcl_service_t
 * structure to be destroyed)
 * @param[in] node 与服务关联的rcl_node_t结构体的指针 (Pointer to the rcl_node_t structure
 * associated with the service)
 * @return 返回RCL_RET_OK表示成功，其他值表示失败 (Returns RCL_RET_OK on success, other values
 * indicate failure)
 */
rcl_ret_t rcl_service_fini(rcl_service_t *service, rcl_node_t *node) {
  // 可以返回以下错误类型 (Can return the following error types)
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_SERVICE_INVALID);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_NODE_INVALID);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_ERROR);

  // 记录调试信息 (Log debug information)
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Finalizing service");

  // 检查service参数是否为空 (Check if the service parameter is NULL)
  RCL_CHECK_ARGUMENT_FOR_NULL(service, RCL_RET_SERVICE_INVALID);

  // 检查节点是否有效 (Check if the node is valid)
  if (!rcl_node_is_valid_except_context(node)) {
    return RCL_RET_NODE_INVALID;  // error already set
  }

  // 初始化结果变量 (Initialize result variable)
  rcl_ret_t result = RCL_RET_OK;

  // 如果服务实现不为空 (If the service implementation is not NULL)
  if (service->impl) {
    // 获取分配器 (Get the allocator)
    rcl_allocator_t allocator = service->impl->options.allocator;

    // 获取rmw节点句柄 (Get the rmw node handle)
    rmw_node_t *rmw_node = rcl_node_get_rmw_handle(node);
    if (!rmw_node) {
      return RCL_RET_INVALID_ARGUMENT;
    }

    // 反配置服务内省 (Unconfigure service introspection)
    rcl_ret_t rcl_ret = unconfigure_service_introspection(node, service->impl, &allocator);
    if (RCL_RET_OK != rcl_ret) {
      RCL_SET_ERROR_MSG(rcl_get_error_string().str);
      result = rcl_ret;
    }

    // 销毁rmw服务 (Destroy the rmw service)
    rmw_ret_t ret = rmw_destroy_service(rmw_node, service->impl->rmw_handle);
    if (ret != RMW_RET_OK) {
      RCL_SET_ERROR_MSG(rmw_get_error_string().str);
      result = RCL_RET_ERROR;
    }

    // 释放重映射服务名称的内存 (Free memory for remapped service name)
    allocator.deallocate(service->impl->remapped_service_name, allocator.state);
    service->impl->remapped_service_name = NULL;

    // 释放服务实现的内存 (Free memory for service implementation)
    allocator.deallocate(service->impl, allocator.state);
    service->impl = NULL;
  }

  // 记录调试信息 (Log debug information)
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Service finalized");

  // 返回结果 (Return result)
  return result;
}

/**
 * @brief 获取默认的服务选项
 *
 * @return 返回一个 rcl_service_options_t 结构体，包含了默认的服务选项
 */
rcl_service_options_t rcl_service_get_default_options() {
  // 确保这些默认值的更改会反映在头文件文档字符串中
  static rcl_service_options_t default_options;
  // 必须在此之后设置分配器和 qos，因为它们不是编译时常量。
  default_options.qos = rmw_qos_profile_services_default;
  default_options.allocator = rcl_get_default_allocator();
  return default_options;
}

/**
 * @brief 获取服务的名称
 *
 * @param[in] service 指向 rcl_service_t 结构体的指针
 * @return 如果成功，则返回服务名称；否则，返回 NULL
 */
const char *rcl_service_get_service_name(const rcl_service_t *service) {
  const rcl_service_options_t *options = rcl_service_get_options(service);
  if (!options) {
    return NULL;
  }
  RCL_CHECK_FOR_NULL_WITH_MSG(service->impl->rmw_handle, "service is invalid", return NULL);
  return service->impl->rmw_handle->service_name;
}

#define _service_get_options(service) &service->impl->options

/**
 * @brief 获取服务的选项
 *
 * @param[in] service 指向 rcl_service_t 结构体的指针
 * @return 如果成功，则返回指向 rcl_service_options_t 结构体的指针；否则，返回 NULL
 */
const rcl_service_options_t *rcl_service_get_options(const rcl_service_t *service) {
  if (!rcl_service_is_valid(service)) {
    return NULL;  // error already set
  }
  return _service_get_options(service);
}

/**
 * @brief 获取服务的 rmw_handle
 *
 * @param[in] service 指向 rcl_service_t 结构体的指针
 * @return 如果成功，则返回指向 rmw_service_t 结构体的指针；否则，返回 NULL
 */
rmw_service_t *rcl_service_get_rmw_handle(const rcl_service_t *service) {
  if (!rcl_service_is_valid(service)) {
    return NULL;  // error already set
  }
  return service->impl->rmw_handle;
}

/**
 * @brief 从服务中获取请求并附带请求头信息
 *
 * @param[in] service 指向有效的rcl_service_t结构体的指针
 * @param[out] request_header 请求头信息，包含请求ID和时间戳等元数据
 * @param[out] ros_request 存储接收到的ROS请求消息的内存区域
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_take_request_with_info(
    const rcl_service_t *service, rmw_service_info_t *request_header, void *ros_request) {
  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Service server taking service request");

  // 检查服务是否有效
  if (!rcl_service_is_valid(service)) {
    return RCL_RET_SERVICE_INVALID;  // error already set
  }

  // 检查参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(request_header, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(ros_request, RCL_RET_INVALID_ARGUMENT);

  // 获取服务选项
  const rcl_service_options_t *options = rcl_service_get_options(service);
  RCL_CHECK_FOR_NULL_WITH_MSG(options, "Failed to get service options", return RCL_RET_ERROR);

  // 初始化taken标志为false
  bool taken = false;

  // 调用底层rmw实现获取请求
  rmw_ret_t ret = rmw_take_request(service->impl->rmw_handle, request_header, ros_request, &taken);

  // 检查rmw操作结果
  if (RMW_RET_OK != ret) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    if (RMW_RET_BAD_ALLOC == ret) {
      return RCL_RET_BAD_ALLOC;
    }
    return RCL_RET_ERROR;
  }

  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(
      ROS_PACKAGE_NAME, "Service take request succeeded: %s", taken ? "true" : "false");

  // 检查是否成功获取请求
  if (!taken) {
    return RCL_RET_SERVICE_TAKE_FAILED;
  }

  // 如果服务事件发布器不为空，则发送服务事件消息
  if (service->impl->service_event_publisher != NULL) {
    rcl_ret_t rclret = rcl_send_service_event_message(
        service->impl->service_event_publisher,
        service_msgs__msg__ServiceEventInfo__REQUEST_RECEIVED, ros_request,
        request_header->request_id.sequence_number, request_header->request_id.writer_guid);

    // 检查rcl操作结果
    if (RCL_RET_OK != rclret) {
      RCL_SET_ERROR_MSG(rcl_get_error_string().str);
      return rclret;
    }
  }

  // 返回成功状态
  return RCL_RET_OK;
}

/**
 * @brief 从服务中获取请求
 *
 * @param[in] service 指向rcl_service_t类型的指针，表示要处理的服务
 * @param[out] request_header 指向rmw_request_id_t类型的指针，用于存储请求头信息
 * @param[out] ros_request 指向void类型的指针，用于存储ROS请求数据
 * @return 返回rcl_ret_t类型的结果，表示函数执行状态
 */
rcl_ret_t rcl_take_request(
    const rcl_service_t *service, rmw_request_id_t *request_header, void *ros_request) {
  // 将request_header的内容复制到header中
  rmw_service_info_t header;
  header.request_id = *request_header;

  // 调用rcl_take_request_with_info函数处理请求，并将结果存储在ret中
  rcl_ret_t ret = rcl_take_request_with_info(service, &header, ros_request);

  // 更新request_header的值
  *request_header = header.request_id;

  // 返回执行结果
  return ret;
}

/**
 * @brief 发送服务响应
 *
 * @param[in] service 指向rcl_service_t类型的指针，表示要处理的服务
 * @param[in] request_header 指向rmw_request_id_t类型的指针，表示请求头信息
 * @param[in] ros_response 指向void类型的指针，表示要发送的ROS响应数据
 * @return 返回rcl_ret_t类型的结果，表示函数执行状态
 */
rcl_ret_t rcl_send_response(
    const rcl_service_t *service, rmw_request_id_t *request_header, void *ros_response) {
  // 打印调试信息
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Sending service response");

  // 检查服务是否有效，如果无效则返回错误
  if (!rcl_service_is_valid(service)) {
    return RCL_RET_SERVICE_INVALID;  // error already set
  }

  // 检查输入参数是否为空，如果为空则返回错误
  RCL_CHECK_ARGUMENT_FOR_NULL(request_header, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(ros_response, RCL_RET_INVALID_ARGUMENT);

  // 获取服务选项
  const rcl_service_options_t *options = rcl_service_get_options(service);
  RCL_CHECK_FOR_NULL_WITH_MSG(options, "Failed to get service options", return RCL_RET_ERROR);

  // 发送响应，如果发送失败则设置错误信息并返回错误
  if (rmw_send_response(service->impl->rmw_handle, request_header, ros_response) != RMW_RET_OK) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    return RCL_RET_ERROR;
  }

  // 如果服务事件发布器不为空，则发布内省内容
  if (service->impl->service_event_publisher != NULL) {
    rcl_ret_t ret = rcl_send_service_event_message(
        service->impl->service_event_publisher, service_msgs__msg__ServiceEventInfo__RESPONSE_SENT,
        ros_response, request_header->sequence_number, request_header->writer_guid);
    if (RCL_RET_OK != ret) {
      RCL_SET_ERROR_MSG(rcl_get_error_string().str);
      return ret;
    }
  }

  // 返回执行结果
  return RCL_RET_OK;
}

/**
 * @brief 检查服务是否有效
 *
 * @param[in] service 一个指向 rcl_service_t 结构体的指针
 * @return 如果服务有效，则返回 true，否则返回 false
 */
bool rcl_service_is_valid(const rcl_service_t *service) {
  // 检查 service 指针是否为空，如果为空则返回错误信息并返回 false
  RCL_CHECK_FOR_NULL_WITH_MSG(service, "service pointer is invalid", return false);
  // 检查 service 的实现是否为空，如果为空则返回错误信息并返回 false
  RCL_CHECK_FOR_NULL_WITH_MSG(service->impl, "service's implementation is invalid", return false);
  // 检查 service 的 rmw 句柄是否为空，如果为空则返回错误信息并返回 false
  RCL_CHECK_FOR_NULL_WITH_MSG(
      service->impl->rmw_handle, "service's rmw handle is invalid", return false);
  return true;
}

/**
 * @brief 获取服务请求订阅的实际 QoS 配置
 *
 * @param[in] service 一个指向 rcl_service_t 结构体的指针
 * @return 如果服务有效，则返回指向 rmw_qos_profile_t 结构体的指针，否则返回 NULL
 */
const rmw_qos_profile_t *rcl_service_request_subscription_get_actual_qos(
    const rcl_service_t *service) {
  // 检查服务是否有效，如果无效则返回 NULL
  if (!rcl_service_is_valid(service)) {
    return NULL;
  }
  // 返回服务请求订阅的实际 QoS 配置
  return &service->impl->actual_request_subscription_qos;
}

/**
 * @brief 获取服务响应发布者的实际 QoS 配置
 *
 * @param[in] service 一个指向 rcl_service_t 结构体的指针
 * @return 如果服务有效，则返回指向 rmw_qos_profile_t 结构体的指针，否则返回 NULL
 */
const rmw_qos_profile_t *rcl_service_response_publisher_get_actual_qos(
    const rcl_service_t *service) {
  // 检查服务是否有效，如果无效则返回 NULL
  if (!rcl_service_is_valid(service)) {
    return NULL;
  }
  // 返回服务响应发布者的实际 QoS 配置
  return &service->impl->actual_response_publisher_qos;
}

/**
 * @brief 设置新请求回调函数
 *
 * @param[in] service 一个指向 rcl_service_t 结构体的指针
 * @param[in] callback 一个 rcl_event_callback_t 类型的回调函数
 * @param[in] user_data 用户数据，将传递给回调函数
 * @return 如果设置成功，则返回 RCL_RET_OK，否则返回相应的错误代码
 */
rcl_ret_t rcl_service_set_on_new_request_callback(
    const rcl_service_t *service, rcl_event_callback_t callback, const void *user_data) {
  // 检查服务是否有效，如果无效则返回 RCL_RET_INVALID_ARGUMENT
  if (!rcl_service_is_valid(service)) {
    // error state already set
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 设置新请求回调函数
  return rmw_service_set_on_new_request_callback(service->impl->rmw_handle, callback, user_data);
}

/**
 * @brief 配置服务自省功能
 *
 * @param[in] service 指向要配置的rcl_service_t结构体的指针
 * @param[in] node 指向与服务关联的rcl_node_t结构体的指针
 * @param[in] clock 指向用于时间管理的rcl_clock_t结构体的指针
 * @param[in] type_support 服务类型支持的指针
 * @param[in] publisher_options 用于配置发布者的rcl_publisher_options_t选项
 * @param[in] introspection_state 自省状态，用于启用或禁用自省功能
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_service_configure_service_introspection(
    rcl_service_t *service,
    rcl_node_t *node,
    rcl_clock_t *clock,
    const rosidl_service_type_support_t *type_support,
    const rcl_publisher_options_t publisher_options,
    rcl_service_introspection_state_t introspection_state) {
  // 检查服务是否有效
  if (!rcl_service_is_valid(service)) {
    return RCL_RET_SERVICE_INVALID;  // 错误已设置
  }
  // 检查参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(node, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(clock, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(type_support, RCL_RET_INVALID_ARGUMENT);

  // 获取分配器
  rcl_allocator_t allocator = service->impl->options.allocator;

  // 如果自省状态为关闭，则取消配置自省功能
  if (introspection_state == RCL_SERVICE_INTROSPECTION_OFF) {
    return unconfigure_service_introspection(node, service->impl, &allocator);
  }

  // 如果服务事件发布者为空
  if (service->impl->service_event_publisher == NULL) {
    // 我们尚未进行自省，因此需要分配服务事件发布者的内存

    service->impl->service_event_publisher =
        allocator.allocate(sizeof(rcl_service_event_publisher_t), allocator.state);
    // 检查分配的内存是否为空
    RCL_CHECK_FOR_NULL_WITH_MSG(service->impl->service_event_publisher, "allocating memory failed",
                                return RCL_RET_BAD_ALLOC;);

    // 初始化服务事件发布者
    *service->impl->service_event_publisher = rcl_get_zero_initialized_service_event_publisher();
    rcl_ret_t ret = rcl_service_event_publisher_init(
        service->impl->service_event_publisher, node, clock, publisher_options,
        service->impl->remapped_service_name, type_support);
    // 如果初始化失败，则释放内存并返回错误代码
    if (RCL_RET_OK != ret) {
      allocator.deallocate(service->impl->service_event_publisher, allocator.state);
      service->impl->service_event_publisher = NULL;
      return ret;
    }
  }

  // 改变服务事件发布者的自省状态
  return rcl_service_event_publisher_change_state(
      service->impl->service_event_publisher, introspection_state);
}

#ifdef __cplusplus
}
#endif
