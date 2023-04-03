// Copyright 2016-2017 Open Source Robotics Foundation, Inc.
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

#include "rcl/graph.h"

#include "./common.h"
#include "rcl/error_handling.h"
#include "rcl/guard_condition.h"
#include "rcl/wait.h"
#include "rcutils/allocator.h"
#include "rcutils/error_handling.h"
#include "rcutils/macros.h"
#include "rcutils/time.h"
#include "rcutils/types.h"
#include "rmw/error_handling.h"
#include "rmw/get_node_info_and_types.h"
#include "rmw/get_service_names_and_types.h"
#include "rmw/get_topic_endpoint_info.h"
#include "rmw/get_topic_names_and_types.h"
#include "rmw/names_and_types.h"
#include "rmw/rmw.h"
#include "rmw/topic_endpoint_info_array.h"
#include "rmw/validate_namespace.h"
#include "rmw/validate_node_name.h"

/**
 * @brief 验证节点名称和命名空间是否有效
 *
 * @param[in] node_name 要验证的节点名称
 * @param[in] node_namespace 要验证的节点命名空间
 * @return rcl_ret_t 返回RCL_RET_OK表示验证成功，其他值表示验证失败
 */
rcl_ret_t __validate_node_name_and_namespace(const char* node_name, const char* node_namespace) {
  // 初始化验证结果变量
  int validation_result = 0;
  // 验证命名空间，并将结果存储在validation_result中
  rmw_ret_t rmw_ret = rmw_validate_namespace(node_namespace, &validation_result, NULL);

  // 如果rmw_ret不等于RMW_RET_OK，表示验证过程出现错误
  if (RMW_RET_OK != rmw_ret) {
    // 设置错误信息
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    // 将rmw_ret转换为rcl_ret并返回
    return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
  }
  // 如果验证结果不等于RMW_NAMESPACE_VALID，表示命名空间无效
  if (validation_result != RMW_NAMESPACE_VALID) {
    // 获取验证结果对应的错误信息
    const char* msg = rmw_namespace_validation_result_string(validation_result);
    // 设置带格式的错误信息
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING("%s, result: %d", msg, validation_result);
    // 返回无效命名空间错误码
    return RCL_RET_NODE_INVALID_NAMESPACE;
  }

  // 重置验证结果变量
  validation_result = 0;
  // 验证节点名称，并将结果存储在validation_result中
  rmw_ret = rmw_validate_node_name(node_name, &validation_result, NULL);
  // 如果rmw_ret不等于RMW_RET_OK，表示验证过程出现错误
  if (RMW_RET_OK != rmw_ret) {
    // 设置错误信息
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    // 将rmw_ret转换为rcl_ret并返回
    return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
  }
  // 如果验证结果不等于RMW_NODE_NAME_VALID，表示节点名称无效
  if (RMW_NODE_NAME_VALID != validation_result) {
    // 获取验证结果对应的错误信息
    const char* msg = rmw_node_name_validation_result_string(validation_result);
    // 设置带格式的错误信息
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING("%s, result: %d", msg, validation_result);
    // 返回无效节点名称错误码
    return RCL_RET_NODE_INVALID_NAME;
  }

  // 验证成功，返回RCL_RET_OK
  return RCL_RET_OK;
}

/**
 * @brief 获取指定节点的发布者名称和类型
 *
 * 该函数用于获取给定节点上的所有发布者的名称和类型。
 *
 * @param[in] node 指向有效的rcl_node_t结构体的指针
 * @param[in,out] allocator 用于分配内存的rcl_allocator_t指针
 * @param[in] no_demangle 如果为true，则不对话题名称进行解析
 * @param[in] node_name 要查询的节点名称
 * @param[in] node_namespace 要查询的节点命名空间
 * @param[out] topic_names_and_types 存储查询到的发布者名称和类型的rcl_names_and_types_t指针
 * @return 返回RCL_RET_OK，如果成功，否则返回相应的错误代码
 */
rcl_ret_t rcl_get_publisher_names_and_types_by_node(
    const rcl_node_t* node,
    rcl_allocator_t* allocator,
    bool no_demangle,
    const char* node_name,
    const char* node_namespace,
    rcl_names_and_types_t* topic_names_and_types) {
  // 检查节点是否有效
  if (!rcl_node_is_valid(node)) {
    return RCL_RET_NODE_INVALID;
  }
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(node_name, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(node_namespace, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(topic_names_and_types, RCL_RET_INVALID_ARGUMENT);

  // 设置有效的命名空间
  const char* valid_namespace = "/";
  if (strlen(node_namespace) > 0) {
    valid_namespace = node_namespace;
  }
  // 检查topic_names_and_types是否为空
  rmw_ret_t rmw_ret = rmw_names_and_types_check_zero(topic_names_and_types);
  if (RMW_RET_OK != rmw_ret) {
    return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
  }
  // 验证节点名称和命名空间
  rcl_ret_t rcl_ret = __validate_node_name_and_namespace(node_name, valid_namespace);
  if (RCL_RET_OK != rcl_ret) {
    return rcl_ret;
  }
  // 转换分配器类型
  rcutils_allocator_t rcutils_allocator = *allocator;
  // 获取发布者名称和类型
  rmw_ret = rmw_get_publisher_names_and_types_by_node(
      rcl_node_get_rmw_handle(node), &rcutils_allocator, node_name, valid_namespace, no_demangle,
      topic_names_and_types);
  // 返回转换后的结果
  return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
}

/**
 * @brief 获取指定节点的订阅者名称和类型列表
 *
 * @param[in] node 指向要查询的节点的指针
 * @param[in] allocator 用于分配内存的分配器
 * @param[in] no_demangle 是否对话题名称进行解析
 * @param[in] node_name 要查询的节点名称
 * @param[in] node_namespace 要查询的节点命名空间
 * @param[out] topic_names_and_types 存储获取到的订阅者名称和类型的结构体指针
 * @return rcl_ret_t 返回操作结果，成功返回RCL_RET_OK，否则返回相应的错误代码
 */
rcl_ret_t rcl_get_subscriber_names_and_types_by_node(
    const rcl_node_t* node,
    rcl_allocator_t* allocator,
    bool no_demangle,
    const char* node_name,
    const char* node_namespace,
    rcl_names_and_types_t* topic_names_and_types) {
  // 检查节点是否有效
  if (!rcl_node_is_valid(node)) {
    return RCL_RET_NODE_INVALID;
  }
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(node_name, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(node_namespace, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(topic_names_and_types, RCL_RET_INVALID_ARGUMENT);

  // 设置默认命名空间
  const char* valid_namespace = "/";
  if (strlen(node_namespace) > 0) {
    valid_namespace = node_namespace;
  }
  // 检查话题名称和类型是否为空
  rmw_ret_t rmw_ret;
  rmw_ret = rmw_names_and_types_check_zero(topic_names_and_types);
  if (rmw_ret != RMW_RET_OK) {
    return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
  }
  // 转换分配器类型
  rcutils_allocator_t rcutils_allocator = *allocator;
  // 验证节点名称和命名空间
  rcl_ret_t rcl_ret = __validate_node_name_and_namespace(node_name, valid_namespace);
  if (RCL_RET_OK != rcl_ret) {
    return rcl_ret;
  }
  // 获取指定节点的订阅者名称和类型列表
  rmw_ret = rmw_get_subscriber_names_and_types_by_node(
      rcl_node_get_rmw_handle(node), &rcutils_allocator, node_name, valid_namespace, no_demangle,
      topic_names_and_types);
  // 返回转换后的结果
  return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
}

/**
 * @brief 获取指定节点上的服务名称和类型 (Get service names and types for a specific node in ROS2)
 *
 * @param[in] node 指向rcl_node_t结构体的指针，表示要查询的节点 (Pointer to the rcl_node_t structure
 * representing the node to query)
 * @param[in] allocator 用于分配内存的rcl_allocator_t结构体指针 (Pointer to the rcl_allocator_t
 * structure used for memory allocation)
 * @param[in] node_name 要查询的节点的名称 (Name of the node to query)
 * @param[in] node_namespace 要查询的节点的命名空间 (Namespace of the node to query)
 * @param[out] service_names_and_types 存储查询到的服务名称和类型的rcl_names_and_types_t结构体指针
 * (Pointer to the rcl_names_and_types_t structure that will store the queried service names and
 * types)
 * @return 返回rcl_ret_t类型的结果，表示函数执行成功或失败的状态 (Returns an rcl_ret_t type result,
 * indicating the success or failure status of the function execution)
 */
rcl_ret_t rcl_get_service_names_and_types_by_node(
    const rcl_node_t* node,
    rcl_allocator_t* allocator,
    const char* node_name,
    const char* node_namespace,
    rcl_names_and_types_t* service_names_and_types) {
  // 检查节点是否有效 (Check if the node is valid)
  if (!rcl_node_is_valid(node)) {
    return RCL_RET_NODE_INVALID;
  }
  // 检查分配器是否有效，并返回相应的错误消息 (Check if the allocator is valid and return the
  // corresponding error message)
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  // 检查输入参数是否为空，并返回相应的错误代码 (Check if the input arguments are NULL and return
  // the corresponding error code)
  RCL_CHECK_ARGUMENT_FOR_NULL(node_name, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(node_namespace, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(service_names_and_types, RCL_RET_INVALID_ARGUMENT);

  // 设置有效的命名空间 (Set a valid namespace)
  const char* valid_namespace = "/";
  if (strlen(node_namespace) > 0) {
    valid_namespace = node_namespace;
  }
  // 定义rmw_ret_t类型变量，用于存储RMW层返回的结果 (Define an rmw_ret_t type variable to store the
  // result returned by the RMW layer)
  rmw_ret_t rmw_ret;
  // 检查服务名称和类型是否为零 (Check if the service names and types are zero)
  rmw_ret = rmw_names_and_types_check_zero(service_names_and_types);
  if (rmw_ret != RMW_RET_OK) {
    return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
  }
  // 验证节点名称和命名空间是否有效 (Validate if the node name and namespace are valid)
  rcl_ret_t rcl_ret = __validate_node_name_and_namespace(node_name, valid_namespace);
  if (RCL_RET_OK != rcl_ret) {
    return rcl_ret;
  }
  // 将rcl_allocator_t转换为rcutils_allocator_t (Convert rcl_allocator_t to rcutils_allocator_t)
  rcutils_allocator_t rcutils_allocator = *allocator;
  // 调用RMW层的函数，获取指定节点上的服务名称和类型 (Call the function in the RMW layer to get the
  // service names and types on the specified node)
  rmw_ret = rmw_get_service_names_and_types_by_node(
      rcl_node_get_rmw_handle(node), &rcutils_allocator, node_name, valid_namespace,
      service_names_and_types);
  // 将RMW层返回的结果转换为RCL层的结果 (Convert the result returned by the RMW layer to the RCL
  // layer result)
  return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
}

/**
 * @brief 获取指定节点的客户端名称和类型 (Get client names and types for a specific node)
 *
 * @param[in] node 指向有效的rcl_node_t结构体的指针 (Pointer to a valid rcl_node_t structure)
 * @param[in] allocator 用于分配内存的rcl_allocator_t结构体指针 (Pointer to an rcl_allocator_t
 * structure for memory allocation)
 * @param[in] node_name 要查询的节点名称 (Name of the node to query)
 * @param[in] node_namespace 要查询的节点命名空间 (Namespace of the node to query)
 * @param[out] service_names_and_types 存储查询结果的rcl_names_and_types_t结构体指针 (Pointer to an
 * rcl_names_and_types_t structure to store the query result)
 * @return 返回RCL_RET_OK表示成功，其他值表示失败 (Returns RCL_RET_OK on success, other values
 * indicate failure)
 */
rcl_ret_t rcl_get_client_names_and_types_by_node(
    const rcl_node_t* node,
    rcl_allocator_t* allocator,
    const char* node_name,
    const char* node_namespace,
    rcl_names_and_types_t* service_names_and_types) {
  // 检查节点是否有效 (Check if the node is valid)
  if (!rcl_node_is_valid(node)) {
    return RCL_RET_NODE_INVALID;
  }
  // 检查分配器是否有效 (Check if the allocator is valid)
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  // 检查node_name参数是否为空 (Check if the node_name argument is NULL)
  RCL_CHECK_ARGUMENT_FOR_NULL(node_name, RCL_RET_INVALID_ARGUMENT);
  // 检查node_namespace参数是否为空 (Check if the node_namespace argument is NULL)
  RCL_CHECK_ARGUMENT_FOR_NULL(node_namespace, RCL_RET_INVALID_ARGUMENT);
  // 检查service_names_and_types参数是否为空 (Check if the service_names_and_types argument is NULL)
  RCL_CHECK_ARGUMENT_FOR_NULL(service_names_and_types, RCL_RET_INVALID_ARGUMENT);

  // 设置有效的命名空间 (Set a valid namespace)
  const char* valid_namespace = "/";
  if (strlen(node_namespace) > 0) {
    valid_namespace = node_namespace;
  }
  // 声明rmw_ret变量 (Declare rmw_ret variable)
  rmw_ret_t rmw_ret;
  // 检查服务名称和类型是否为零 (Check if service names and types are zero)
  rmw_ret = rmw_names_and_types_check_zero(service_names_and_types);
  if (rmw_ret != RMW_RET_OK) {
    return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
  }
  // 验证节点名称和命名空间 (Validate node name and namespace)
  rcl_ret_t rcl_ret = __validate_node_name_and_namespace(node_name, valid_namespace);
  if (RCL_RET_OK != rcl_ret) {
    return rcl_ret;
  }
  // 转换分配器类型 (Convert allocator type)
  rcutils_allocator_t rcutils_allocator = *allocator;
  // 获取指定节点的客户端名称和类型 (Get client names and types for the specified node)
  rmw_ret = rmw_get_client_names_and_types_by_node(
      rcl_node_get_rmw_handle(node), &rcutils_allocator, node_name, valid_namespace,
      service_names_and_types);
  // 转换并返回rmw_ret到rcl_ret (Convert and return rmw_ret to rcl_ret)
  return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
}

/**
 * @brief 获取ROS2节点中的主题名称和类型
 *
 * @param[in] node 指向rcl_node_t类型的指针，表示一个有效的ROS2节点
 * @param[in] allocator 指向rcl_allocator_t类型的指针，用于分配内存
 * @param[in] no_demangle 布尔值，表示是否对主题名称进行解析
 * @param[out] topic_names_and_types
 * 指向rcl_names_and_types_t类型的指针，用于存储获取到的主题名称和类型
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_get_topic_names_and_types(
    const rcl_node_t* node,
    rcl_allocator_t* allocator,
    bool no_demangle,
    rcl_names_and_types_t* topic_names_and_types) {
  // 检查节点是否有效
  if (!rcl_node_is_valid(node)) {
    return RCL_RET_NODE_INVALID;  // 错误已设置
  }
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  // 检查topic_names_and_types参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(topic_names_and_types, RCL_RET_INVALID_ARGUMENT);

  rmw_ret_t rmw_ret;
  // 检查topic_names_and_types是否为零
  rmw_ret = rmw_names_and_types_check_zero(topic_names_and_types);
  if (rmw_ret != RMW_RET_OK) {
    return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
  }

  rcutils_allocator_t rcutils_allocator = *allocator;
  // 调用rmw_get_topic_names_and_types函数获取主题名称和类型
  rmw_ret = rmw_get_topic_names_and_types(
      rcl_node_get_rmw_handle(node), &rcutils_allocator, no_demangle, topic_names_and_types);
  return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
}

/**
 * @brief 获取ROS2节点中的服务名称和类型
 *
 * @param[in] node 指向rcl_node_t类型的指针，表示一个有效的ROS2节点
 * @param[in] allocator 指向rcl_allocator_t类型的指针，用于分配内存
 * @param[out] service_names_and_types
 * 指向rcl_names_and_types_t类型的指针，用于存储获取到的服务名称和类型
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_get_service_names_and_types(
    const rcl_node_t* node,
    rcl_allocator_t* allocator,
    rcl_names_and_types_t* service_names_and_types) {
  // 检查节点是否有效
  if (!rcl_node_is_valid(node)) {
    return RCL_RET_NODE_INVALID;  // 错误已设置
  }
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  // 检查service_names_and_types参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(service_names_and_types, RCL_RET_INVALID_ARGUMENT);

  rmw_ret_t rmw_ret;
  // 检查service_names_and_types是否为零
  rmw_ret = rmw_names_and_types_check_zero(service_names_and_types);
  if (rmw_ret != RMW_RET_OK) {
    return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
  }

  rcutils_allocator_t rcutils_allocator = *allocator;
  // 调用rmw_get_service_names_and_types函数获取服务名称和类型
  rmw_ret = rmw_get_service_names_and_types(
      rcl_node_get_rmw_handle(node), &rcutils_allocator, service_names_and_types);
  return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
}

/**
 * @brief 初始化 rcl_names_and_types_t 结构体 (Initialize the rcl_names_and_types_t structure)
 *
 * @param[inout] names_and_types 指向待初始化的结构体指针 (Pointer to the structure to be
 * initialized)
 * @param[in] size 初始化结构体中 names 和 types 数组的大小 (Size of the names and types arrays in
 * the structure)
 * @param[in] allocator 分配内存所使用的分配器 (Allocator used for memory allocation)
 * @return 返回 rcl_ret_t 类型的结果，表示成功或失败 (Returns a result of type rcl_ret_t, indicating
 * success or failure)
 */
rcl_ret_t rcl_names_and_types_init(
    rcl_names_and_types_t* names_and_types, size_t size, rcl_allocator_t* allocator) {
  // 检查是否可以返回 RCL_RET_INVALID_ARGUMENT 错误 (Check if it can return an
  // RCL_RET_INVALID_ARGUMENT error)
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);

  // 检查 names_and_types 参数是否为空，如果为空则返回 RCL_RET_INVALID_ARGUMENT 错误 (Check if the
  // names_and_types argument is NULL, and return RCL_RET_INVALID_ARGUMENT error if it is)
  RCL_CHECK_ARGUMENT_FOR_NULL(names_and_types, RCL_RET_INVALID_ARGUMENT);
  // 检查分配器参数是否有效，如果无效则返回 RCL_RET_INVALID_ARGUMENT 错误 (Check if the allocator
  // argument is valid, and return RCL_RET_INVALID_ARGUMENT error if it's not)
  RCL_CHECK_ALLOCATOR(allocator, return RCL_RET_INVALID_ARGUMENT);
  // 调用 rmw_names_and_types_init 函数进行初始化，并获取返回值 (Call the rmw_names_and_types_init
  // function for initialization and get the return value)
  rmw_ret_t rmw_ret = rmw_names_and_types_init(names_and_types, size, allocator);
  // 将 rmw_ret 转换为 rcl_ret_t 类型并返回 (Convert rmw_ret to rcl_ret_t type and return)
  return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
}

/**
 * @brief 清理 rcl_names_and_types_t 结构体 (Clean up the rcl_names_and_types_t structure)
 *
 * @param[inout] topic_names_and_types 指向待清理的结构体指针 (Pointer to the structure to be
 * cleaned up)
 * @return 返回 rcl_ret_t 类型的结果，表示成功或失败 (Returns a result of type rcl_ret_t, indicating
 * success or failure)
 */
rcl_ret_t rcl_names_and_types_fini(rcl_names_and_types_t* topic_names_and_types) {
  // 检查是否可以返回 RCL_RET_INVALID_ARGUMENT 错误 (Check if it can return an
  // RCL_RET_INVALID_ARGUMENT error)
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);

  // 检查 topic_names_and_types 参数是否为空，如果为空则返回 RCL_RET_INVALID_ARGUMENT 错误 (Check if
  // the topic_names_and_types argument is NULL, and return RCL_RET_INVALID_ARGUMENT error if it is)
  RCL_CHECK_ARGUMENT_FOR_NULL(topic_names_and_types, RCL_RET_INVALID_ARGUMENT);
  // 调用 rmw_names_and_types_fini 函数进行清理，并获取返回值 (Call the rmw_names_and_types_fini
  // function for cleanup and get the return value)
  rmw_ret_t rmw_ret = rmw_names_and_types_fini(topic_names_and_types);
  // 将 rmw_ret 转换为 rcl_ret_t 类型并返回 (Convert rmw_ret to rcl_ret_t type and return)
  return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
}

/**
 * @brief 获取节点名称和命名空间 (Get node names and namespaces)
 *
 * @param[in] node 指向 rcl_node_t 结构体的指针 (Pointer to the rcl_node_t structure)
 * @param[in] allocator 分配器，用于分配内存 (Allocator for allocating memory)
 * @param[out] node_names 用于存储节点名称的字符串数组 (String array for storing node names)
 * @param[out] node_namespaces 用于存储节点命名空间的字符串数组 (String array for storing node
 * namespaces)
 * @return rcl_ret_t 返回操作结果 (Return operation result)
 */
rcl_ret_t rcl_get_node_names(
    const rcl_node_t* node,
    rcl_allocator_t allocator,
    rcutils_string_array_t* node_names,
    rcutils_string_array_t* node_namespaces) {
  // 检查节点是否有效 (Check if the node is valid)
  if (!rcl_node_is_valid(node)) {
    return RCL_RET_NODE_INVALID;  // 错误已设置 (Error already set)
  }
  // 检查 node_names 参数是否为空 (Check if the node_names argument is NULL)
  RCL_CHECK_ARGUMENT_FOR_NULL(node_names, RCL_RET_INVALID_ARGUMENT);
  // 检查 node_names 的大小是否为零 (Check if the size of node_names is not zero)
  if (node_names->size != 0) {
    RCL_SET_ERROR_MSG("node_names size is not zero");
    return RCL_RET_INVALID_ARGUMENT;
  }
  // 检查 node_names 的数据是否为空 (Check if the data of node_names is not NULL)
  if (node_names->data) {
    RCL_SET_ERROR_MSG("node_names is not null");
    return RCL_RET_INVALID_ARGUMENT;
  }
  // 检查 node_namespaces 参数是否为空 (Check if the node_namespaces argument is NULL)
  RCL_CHECK_ARGUMENT_FOR_NULL(node_namespaces, RCL_RET_INVALID_ARGUMENT);
  // 检查 node_namespaces 的大小是否为零 (Check if the size of node_namespaces is not zero)
  if (node_namespaces->size != 0) {
    RCL_SET_ERROR_MSG("node_namespaces size is not zero");
    return RCL_RET_INVALID_ARGUMENT;
  }
  // 检查 node_namespaces 的数据是否为空 (Check if the data of node_namespaces is not NULL)
  if (node_namespaces->data) {
    RCL_SET_ERROR_MSG("node_namespaces is not null");
    return RCL_RET_INVALID_ARGUMENT;
  }
  // 分配器暂时不使用，将来会在 rmw_get_node_names 中使用 (Allocator is not used for now, will be
  // used in rmw_get_node_names in the future)
  (void)allocator;
  // 调用 rmw_get_node_names 函数获取节点名称和命名空间 (Call rmw_get_node_names function to get
  // node names and namespaces)
  rmw_ret_t rmw_ret =
      rmw_get_node_names(rcl_node_get_rmw_handle(node), node_names, node_namespaces);

  // 检查 rmw_ret 是否为 RMW_RET_OK (Check if rmw_ret is RMW_RET_OK)
  if (RMW_RET_OK != rmw_ret) {
    return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
  }

  // 检查节点名称是否为 NULL 或空字符串 (Check if any of the node names are NULL or empty strings)
  for (size_t i = 0u; i < node_names->size; ++i) {
    if (!node_names->data[i]) {
      RCL_SET_ERROR_MSG("NULL node name returned by the RMW layer");
      return RCL_RET_NODE_INVALID_NAME;
    }
    if (!strcmp(node_names->data[i], "")) {
      RCL_SET_ERROR_MSG("empty node name returned by the RMW layer");
      return RCL_RET_NODE_INVALID_NAME;
    }
  }
  // 检查节点命名空间是否为 NULL (Check if any of the node namespaces are NULL)
  for (size_t i = 0u; i < node_namespaces->size; ++i) {
    if (!node_namespaces->data[i]) {
      RCL_SET_ERROR_MSG("NULL node namespace returned by the RMW layer");
      return RCL_RET_NODE_INVALID_NAMESPACE;
    }
  }
  // 返回操作成功 (Return operation success)
  return RCL_RET_OK;
}

/**
 * @brief 获取节点名称、命名空间和安全领域信息 (Get node names, namespaces and enclaves information)
 *
 * @param[in] node 指向 rcl_node_t 类型的指针，用于获取节点信息 (Pointer to an rcl_node_t type, used
 * to get node information)
 * @param[in] allocator 分配器，用于分配内存 (Allocator for memory allocation)
 * @param[out] node_names 用于存储节点名称的字符串数组 (String array to store node names)
 * @param[out] node_namespaces 用于存储节点命名空间的字符串数组 (String array to store node
 * namespaces)
 * @param[out] enclaves 用于存储安全领域信息的字符串数组 (String array to store enclaves
 * information)
 * @return rcl_ret_t 返回操作结果 (Return the operation result)
 */
rcl_ret_t rcl_get_node_names_with_enclaves(
    const rcl_node_t* node,
    rcl_allocator_t allocator,
    rcutils_string_array_t* node_names,
    rcutils_string_array_t* node_namespaces,
    rcutils_string_array_t* enclaves) {
  // 检查节点是否有效 (Check if the node is valid)
  if (!rcl_node_is_valid(node)) {
    return RCL_RET_NODE_INVALID;  // error already set
  }

  // 检查 node_names 参数是否为空 (Check if the node_names argument is null)
  RCL_CHECK_ARGUMENT_FOR_NULL(node_names, RCL_RET_INVALID_ARGUMENT);

  // 检查 node_names 的 size 是否为 0 (Check if the size of node_names is 0)
  if (node_names->size != 0) {
    RCL_SET_ERROR_MSG("node_names size is not zero");
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 检查 node_names 的 data 是否为空 (Check if the data of node_names is null)
  if (node_names->data) {
    RCL_SET_ERROR_MSG("node_names is not null");
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 检查 node_namespaces 参数是否为空 (Check if the node_namespaces argument is null)
  RCL_CHECK_ARGUMENT_FOR_NULL(node_namespaces, RCL_RET_INVALID_ARGUMENT);

  // 检查 node_namespaces 的 size 是否为 0 (Check if the size of node_namespaces is 0)
  if (node_namespaces->size != 0) {
    RCL_SET_ERROR_MSG("node_namespaces size is not zero");
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 检查 node_namespaces 的 data 是否为空 (Check if the data of node_namespaces is null)
  if (node_namespaces->data) {
    RCL_SET_ERROR_MSG("node_namespaces is not null");
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 检查 enclaves 参数是否为空 (Check if the enclaves argument is null)
  RCL_CHECK_ARGUMENT_FOR_NULL(enclaves, RCL_RET_INVALID_ARGUMENT);

  // 检查 enclaves 的 size 是否为 0 (Check if the size of enclaves is 0)
  if (enclaves->size != 0) {
    RCL_SET_ERROR_MSG("enclaves size is not zero");
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 检查 enclaves 的 data 是否为空 (Check if the data of enclaves is null)
  if (enclaves->data) {
    RCL_SET_ERROR_MSG("enclaves is not null");
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 忽略分配器参数，将来在 rmw_get_node_names 中使用 (Ignore the allocator argument, to be used in
  // rmw_get_node_names in the future)
  (void)allocator;

  // 调用 rmw_get_node_names_with_enclaves 函数获取节点名称、命名空间和安全领域信息 (Call
  // rmw_get_node_names_with_enclaves function to get node names, namespaces and enclaves
  // information)
  rmw_ret_t rmw_ret = rmw_get_node_names_with_enclaves(
      rcl_node_get_rmw_handle(node), node_names, node_namespaces, enclaves);

  // 将 RMW 返回值转换为 RCL 返回值 (Convert RMW return value to RCL return value)
  return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
}

/**
 * @brief 计算给定主题上的发布者数量 (Count the number of publishers on a given topic)
 *
 * @param[in] node 指向 rcl_node_t 结构体的指针 (Pointer to an rcl_node_t structure)
 * @param[in] topic_name 要查询的主题名称 (The name of the topic to query)
 * @param[out] count 存储发布者数量的变量的指针 (Pointer to a variable to store the number of
 * publishers)
 * @return 返回一个 rcl_ret_t 类型的结果，表示函数执行成功或失败 (Returns an rcl_ret_t type result
 * indicating success or failure of the function execution)
 */
rcl_ret_t rcl_count_publishers(const rcl_node_t* node, const char* topic_name, size_t* count) {
  // 检查节点是否有效 (Check if the node is valid)
  if (!rcl_node_is_valid(node)) {
    return RCL_RET_NODE_INVALID;  // error already set
  }
  // 获取节点选项 (Get the node options)
  const rcl_node_options_t* node_options = rcl_node_get_options(node);
  // 检查节点选项是否为空 (Check if the node options are null)
  if (!node_options) {
    return RCL_RET_NODE_INVALID;  // shouldn't happen, but error is already set if so
  }
  // 检查 topic_name 参数是否为空 (Check if the topic_name argument is null)
  RCL_CHECK_ARGUMENT_FOR_NULL(topic_name, RCL_RET_INVALID_ARGUMENT);
  // 检查 count 参数是否为空 (Check if the count argument is null)
  RCL_CHECK_ARGUMENT_FOR_NULL(count, RCL_RET_INVALID_ARGUMENT);
  // 调用 rmw_count_publishers 函数获取发布者数量 (Call the rmw_count_publishers function to get the
  // number of publishers)
  rmw_ret_t rmw_ret = rmw_count_publishers(rcl_node_get_rmw_handle(node), topic_name, count);
  // 将 rmw_ret 转换为 rcl_ret_t 类型并返回 (Convert rmw_ret to rcl_ret_t type and return)
  return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
}

/**
 * @brief 计算给定主题上的订阅者数量 (Count the number of subscribers on a given topic)
 *
 * @param[in] node 指向 rcl_node_t 结构体的指针 (Pointer to an rcl_node_t structure)
 * @param[in] topic_name 要查询的主题名称 (The name of the topic to query)
 * @param[out] count 存储订阅者数量的变量的指针 (Pointer to a variable to store the number of
 * subscribers)
 * @return 返回一个 rcl_ret_t 类型的结果，表示函数执行成功或失败 (Returns an rcl_ret_t type result
 * indicating success or failure of the function execution)
 */
rcl_ret_t rcl_count_subscribers(const rcl_node_t* node, const char* topic_name, size_t* count) {
  // 检查节点是否有效 (Check if the node is valid)
  if (!rcl_node_is_valid(node)) {
    return RCL_RET_NODE_INVALID;  // error already set
  }
  // 获取节点选项 (Get the node options)
  const rcl_node_options_t* node_options = rcl_node_get_options(node);
  // 检查节点选项是否为空 (Check if the node options are null)
  if (!node_options) {
    return RCL_RET_NODE_INVALID;  // shouldn't happen, but error is already set if so
  }
  // 检查 topic_name 参数是否为空 (Check if the topic_name argument is null)
  RCL_CHECK_ARGUMENT_FOR_NULL(topic_name, RCL_RET_INVALID_ARGUMENT);
  // 检查 count 参数是否为空 (Check if the count argument is null)
  RCL_CHECK_ARGUMENT_FOR_NULL(count, RCL_RET_INVALID_ARGUMENT);
  // 调用 rmw_count_subscribers 函数获取订阅者数量 (Call the rmw_count_subscribers function to get
  // the number of subscribers)
  rmw_ret_t rmw_ret = rmw_count_subscribers(rcl_node_get_rmw_handle(node), topic_name, count);
  // 将 rmw_ret 转换为 rcl_ret_t 类型并返回 (Convert rmw_ret to rcl_ret_t type and return)
  return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
}

/**
 * @brief 计算实体数量的函数类型 (Function type for counting entities)
 */
typedef rcl_ret_t (*count_entities_func_t)(
    const rcl_node_t* node, const char* topic_name, size_t* count);

/**
 * @brief 等待特定数量的实体出现或超时 (Wait for a specific number of entities to appear or timeout)
 *
 * @param[in] node 节点指针 (Pointer to the node)
 * @param[in] allocator 分配器 (Allocator)
 * @param[in] topic_name 主题名称 (Topic name)
 * @param[in] expected_count 期望的实体数量 (Expected number of entities)
 * @param[in] timeout 超时时间 (Timeout duration)
 * @param[out] success 是否成功等到期望数量的实体 (Whether the expected number of entities was
 * successfully waited for)
 * @param[in] count_entities_func 计算实体数量的函数 (Function for counting entities)
 * @return rcl_ret_t 返回执行结果 (Return execution result)
 */
rcl_ret_t _rcl_wait_for_entities(
    const rcl_node_t* node,
    rcl_allocator_t* allocator,
    const char* topic_name,
    const size_t expected_count,
    rcutils_duration_value_t timeout,
    bool* success,
    count_entities_func_t count_entities_func) {
  // 验证节点是否有效 (Check if the node is valid)
  if (!rcl_node_is_valid(node)) {
    return RCL_RET_NODE_INVALID;
  }
  // 检查分配器是否有效 (Check if the allocator is valid)
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  // 检查主题名称是否为空 (Check if the topic name is null)
  RCL_CHECK_ARGUMENT_FOR_NULL(topic_name, RCL_RET_INVALID_ARGUMENT);
  // 检查成功标志是否为空 (Check if the success flag is null)
  RCL_CHECK_ARGUMENT_FOR_NULL(success, RCL_RET_INVALID_ARGUMENT);

  rcl_ret_t ret = RCL_RET_OK;
  *success = false;

  // 如果已经有期望数量的实体，可以避免等待 (We can avoid waiting if there are already the expected
  // number of entities)
  size_t count = 0u;
  ret = count_entities_func(node, topic_name, &count);
  if (ret != RCL_RET_OK) {
    // Error message already set
    return ret;
  }
  if (expected_count <= count) {
    *success = true;
    return RCL_RET_OK;
  }

  // 创建一个 wait set 并将节点图形 guard condition 添加到其中 (Create a wait set and add the node
  // graph guard condition to it)
  rcl_wait_set_t wait_set = rcl_get_zero_initialized_wait_set();
  ret = rcl_wait_set_init(&wait_set, 0, 1, 0, 0, 0, 0, node->context, *allocator);
  if (ret != RCL_RET_OK) {
    // Error message already set
    return ret;
  }

  const rcl_guard_condition_t* guard_condition = rcl_node_get_graph_guard_condition(node);
  if (!guard_condition) {
    // Error message already set
    ret = RCL_RET_ERROR;
    goto cleanup;
  }

  // 将它添加到 wait set 中 (Add it to the wait set)
  ret = rcl_wait_set_add_guard_condition(&wait_set, guard_condition, NULL);
  if (ret != RCL_RET_OK) {
    // Error message already set
    goto cleanup;
  }

  // 获取当前时间 (Get current time)
  // 我们使用系统时间与 rcl_wait() 使用的时钟保持一致 (We use system time to be consistent with the
  // clock used by rcl_wait())
  rcutils_time_point_value_t start;
  rcutils_ret_t time_ret = rcutils_system_time_now(&start);
  if (time_ret != RCUTILS_RET_OK) {
    rcutils_error_string_t error = rcutils_get_error_string();
    rcutils_reset_error();
    RCL_SET_ERROR_MSG(error.str);
    ret = RCL_RET_ERROR;
    goto cleanup;
  }

  // 等待期望数量或超时 (Wait for expected count or timeout)
  rcl_ret_t wait_ret;
  while (true) {
    // 使用单独的 'wait_ret' 代码以避免返回错误的 TIMEOUT 值 (Use separate 'wait_ret' code to avoid
    // returning spurious TIMEOUT value)
    wait_ret = rcl_wait(&wait_set, timeout);
    if (wait_ret != RCL_RET_OK && wait_ret != RCL_RET_TIMEOUT) {
      // Error message already set
      ret = wait_ret;
      break;
    }

    // 检查计数 (Check count)
    ret = count_entities_func(node, topic_name, &count);
    if (ret != RCL_RET_OK) {
      // Error already set
      break;
    }
    if (expected_count <= count) {
      *success = true;
      break;
    }

    // 如果我们没有无限期等待，计算剩余时间 (If we're not waiting indefinitely, compute time
    // remaining)
    if (timeout >= 0) {
      rcutils_time_point_value_t now;
      time_ret = rcutils_system_time_now(&now);
      if (time_ret != RCUTILS_RET_OK) {
        rcutils_error_string_t error = rcutils_get_error_string();
        rcutils_reset_error();
        RCL_SET_ERROR_MSG(error.str);
        ret = RCL_RET_ERROR;
        break;
      }
      timeout = timeout - (now - start);
      if (timeout <= 0) {
        ret = RCL_RET_TIMEOUT;
        break;
      }
    }

    // 清除 wait set 以进行下一次迭代 (Clear wait set for next iteration)
    ret = rcl_wait_set_clear(&wait_set);
    if (ret != RCL_RET_OK) {
      // Error message already set
      break;
    }
  }

  rcl_ret_t cleanup_ret;
cleanup:
  // 清理 (Cleanup)
  cleanup_ret = rcl_wait_set_fini(&wait_set);
  if (cleanup_ret != RCL_RET_OK) {
    // 如果我们得到了两个意外错误，则返回较早的错误 (If we got two unexpected errors, return the
    // earlier error)
    if (ret != RCL_RET_OK && ret != RCL_RET_TIMEOUT) {
      // Error message already set
      ret = cleanup_ret;
    }
  }

  return ret;
}

/**
 * @brief 等待发布者 (Wait for publishers)
 *
 * @param[in] node 指向 rcl_node_t 结构体的指针 (Pointer to the rcl_node_t structure)
 * @param[in] allocator 分配器 (Allocator)
 * @param[in] topic_name 主题名称 (Topic name)
 * @param[in] expected_count 期望的发布者数量 (Expected number of publishers)
 * @param[in] timeout 超时时间 (Timeout duration)
 * @param[out] success 成功标志，表示是否达到期望的发布者数量 (Success flag, indicating if the
 * expected number of publishers is reached)
 * @return rcl_ret_t 返回操作结果 (Return operation result)
 */
rcl_ret_t rcl_wait_for_publishers(
    const rcl_node_t* node,
    rcl_allocator_t* allocator,
    const char* topic_name,
    const size_t expected_count,
    rcutils_duration_value_t timeout,
    bool* success) {
  // 调用 _rcl_wait_for_entities 函数，并传入 rcl_count_publishers 函数作为参数
  // Call the _rcl_wait_for_entities function and pass the rcl_count_publishers function as an
  // argument
  return _rcl_wait_for_entities(
      node, allocator, topic_name, expected_count, timeout, success, rcl_count_publishers);
}

/**
 * @brief 等待订阅者 (Wait for subscribers)
 *
 * @param[in] node 指向 rcl_node_t 结构体的指针 (Pointer to the rcl_node_t structure)
 * @param[in] allocator 分配器 (Allocator)
 * @param[in] topic_name 主题名称 (Topic name)
 * @param[in] expected_count 期望的订阅者数量 (Expected number of subscribers)
 * @param[in] timeout 超时时间 (Timeout duration)
 * @param[out] success 成功标志，表示是否达到期望的订阅者数量 (Success flag, indicating if the
 * expected number of subscribers is reached)
 * @return rcl_ret_t 返回操作结果 (Return operation result)
 */
rcl_ret_t rcl_wait_for_subscribers(
    const rcl_node_t* node,
    rcl_allocator_t* allocator,
    const char* topic_name,
    const size_t expected_count,
    rcutils_duration_value_t timeout,
    bool* success) {
  // 调用 _rcl_wait_for_entities 函数，并传入 rcl_count_subscribers 函数作为参数
  // Call the _rcl_wait_for_entities function and pass the rcl_count_subscribers function as an
  // argument
  return _rcl_wait_for_entities(
      node, allocator, topic_name, expected_count, timeout, success, rcl_count_subscribers);
}

/**
 * @brief 获取主题端点信息函数类型定义 (Get topic endpoint information function type definition)
 *
 * @param[in] node 指向 rmw_node_t 结构体的指针 (Pointer to the rmw_node_t structure)
 * @param[in] allocator 分配器 (Allocator)
 * @param[in] topic_name 主题名称 (Topic name)
 * @param[in] no_mangle 是否对主题名称进行修饰处理 (Whether to mangle the topic name)
 * @param[out] info_array 存储主题端点信息的数组 (Array to store topic endpoint information)
 * @return rmw_ret_t 返回操作结果 (Return operation result)
 */
typedef rmw_ret_t (*get_topic_endpoint_info_func_t)(
    const rmw_node_t* node,
    rcutils_allocator_t* allocator,
    const char* topic_name,
    bool no_mangle,
    rmw_topic_endpoint_info_array_t* info_array);

/**
 * @brief 获取指定主题的端点信息 (Get the endpoint information for a specific topic)
 *
 * @param[in] node 有效的 rcl_node_t 结构体指针 (A valid pointer to an rcl_node_t structure)
 * @param[in] allocator 用于分配内存的 rcutils_allocator_t 结构体指针 (A pointer to an
 * rcutils_allocator_t structure for memory allocation)
 * @param[in] topic_name 要查询的主题名称 (The name of the topic to query)
 * @param[in] no_mangle 是否禁用主题名称修饰 (Whether to disable topic name mangling)
 * @param[out] info_array 存储查询到的端点信息的 rmw_topic_endpoint_info_array_t 结构体指针 (A
 * pointer to an rmw_topic_endpoint_info_array_t structure to store the queried endpoint
 * information)
 * @param[in] get_topic_endpoint_info 函数指针，用于获取主题端点信息 (Function pointer for getting
 * topic endpoint information)
 * @return rcl_ret_t 返回操作结果 (Return the operation result)
 */
rcl_ret_t __rcl_get_info_by_topic(
    const rcl_node_t* node,
    rcutils_allocator_t* allocator,
    const char* topic_name,
    bool no_mangle,
    rmw_topic_endpoint_info_array_t* info_array,
    get_topic_endpoint_info_func_t get_topic_endpoint_info) {
  // 检查节点是否有效 (Check if the node is valid)
  if (!rcl_node_is_valid(node)) {
    return RCL_RET_NODE_INVALID;  // error already set.
  }
  // 获取节点选项 (Get node options)
  const rcl_node_options_t* node_options = rcl_node_get_options(node);
  // 检查节点选项是否有效 (Check if node options are valid)
  if (!node_options) {
    return RCL_RET_NODE_INVALID;  // shouldn't happen, but error is already set if so
  }
  // 检查内存分配器是否有效 (Check if the allocator is valid)
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  // 检查主题名称是否为空 (Check if the topic name is null)
  RCL_CHECK_ARGUMENT_FOR_NULL(topic_name, RCL_RET_INVALID_ARGUMENT);

  rmw_error_string_t error_string;
  // 检查 info_array 是否已经初始化为零 (Check if info_array has been initialized to zero)
  rmw_ret_t rmw_ret = rmw_topic_endpoint_info_array_check_zero(info_array);
  if (rmw_ret != RMW_RET_OK) {
    error_string = rmw_get_error_string();
    rmw_reset_error();
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "rmw_topic_endpoint_info_array_t must be zero initialized: %s,\n"
        "Use rmw_get_zero_initialized_topic_endpoint_info_array",
        error_string.str);
    return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
  }
  // 调用 get_topic_endpoint_info 函数获取端点信息 (Call the get_topic_endpoint_info function to get
  // endpoint information)
  rmw_ret = get_topic_endpoint_info(
      rcl_node_get_rmw_handle(node), allocator, topic_name, no_mangle, info_array);
  if (rmw_ret != RMW_RET_OK) {
    error_string = rmw_get_error_string();
    rmw_reset_error();
    RCL_SET_ERROR_MSG(error_string.str);
  }
  // 返回操作结果 (Return the operation result)
  return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
}

/**
 * @brief 获取指定主题的发布者信息 (Get publishers information for a specific topic)
 *
 * @param[in] node 指向 rcl_node_t 结构体的指针 (Pointer to the rcl_node_t structure)
 * @param[in] allocator 分配器，用于分配内存 (Allocator for memory allocation)
 * @param[in] topic_name 主题名称 (Topic name)
 * @param[in] no_mangle 是否对主题名称进行修饰 (Whether to mangle the topic name or not)
 * @param[out] publishers_info 存储发布者信息的结构体数组 (Array of structures to store publishers
 * information)
 * @return rcl_ret_t 返回操作结果 (Return operation result)
 */
rcl_ret_t rcl_get_publishers_info_by_topic(
    const rcl_node_t* node,
    rcutils_allocator_t* allocator,
    const char* topic_name,
    bool no_mangle,
    rmw_topic_endpoint_info_array_t* publishers_info) {
  // 调用 __rcl_get_info_by_topic 函数获取发布者信息 (Call the __rcl_get_info_by_topic function to
  // get publishers information)
  return __rcl_get_info_by_topic(
      node, allocator, topic_name, no_mangle, publishers_info, rmw_get_publishers_info_by_topic);
}

/**
 * @brief 获取指定主题的订阅者信息 (Get subscriptions information for a specific topic)
 *
 * @param[in] node 指向 rcl_node_t 结构体的指针 (Pointer to the rcl_node_t structure)
 * @param[in] allocator 分配器，用于分配内存 (Allocator for memory allocation)
 * @param[in] topic_name 主题名称 (Topic name)
 * @param[in] no_mangle 是否对主题名称进行修饰 (Whether to mangle the topic name or not)
 * @param[out] subscriptions_info 存储订阅者信息的结构体数组 (Array of structures to store
 * subscriptions information)
 * @return rcl_ret_t 返回操作结果 (Return operation result)
 */
rcl_ret_t rcl_get_subscriptions_info_by_topic(
    const rcl_node_t* node,
    rcutils_allocator_t* allocator,
    const char* topic_name,
    bool no_mangle,
    rmw_topic_endpoint_info_array_t* subscriptions_info) {
  // 调用 __rcl_get_info_by_topic 函数获取订阅者信息 (Call the __rcl_get_info_by_topic function to
  // get subscriptions information)
  return __rcl_get_info_by_topic(
      node, allocator, topic_name, no_mangle, subscriptions_info,
      rmw_get_subscriptions_info_by_topic);
}

/**
 * @brief 检查服务服务器是否可用 (Check if the service server is available)
 *
 * @param[in] node 指向 rcl_node_t 结构体的指针 (Pointer to the rcl_node_t structure)
 * @param[in] client 指向 rcl_client_t 结构体的指针 (Pointer to the rcl_client_t structure)
 * @param[out] is_available 用于存储服务服务器是否可用的布尔值 (Boolean value to store whether the
 * service server is available or not)
 * @return rcl_ret_t 返回操作结果 (Return operation result)
 */
rcl_ret_t rcl_service_server_is_available(
    const rcl_node_t* node, const rcl_client_t* client, bool* is_available) {
  // 检查错误参数 (Check for invalid arguments)
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_NODE_INVALID);

  // 检查节点是否有效 (Check if the node is valid)
  if (!rcl_node_is_valid(node)) {
    return RCL_RET_NODE_INVALID;  // error already set
  }
  // 获取节点选项 (Get node options)
  const rcl_node_options_t* node_options = rcl_node_get_options(node);
  if (!node_options) {
    return RCL_RET_NODE_INVALID;  // shouldn't happen, but error is already set if so
  }
  // 检查 client 和 is_available 参数是否为空 (Check if client and is_available arguments are null)
  RCL_CHECK_ARGUMENT_FOR_NULL(client, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(is_available, RCL_RET_INVALID_ARGUMENT);
  // 调用 rmw_service_server_is_available 函数检查服务服务器是否可用 (Call
  // rmw_service_server_is_available function to check if the service server is available)
  rmw_ret_t rmw_ret = rmw_service_server_is_available(
      rcl_node_get_rmw_handle(node), rcl_client_get_rmw_handle(client), is_available);
  // 将 rmw_ret 转换为 rcl_ret_t 类型并返回 (Convert rmw_ret to rcl_ret_t type and return)
  return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
}

#ifdef __cplusplus
}
#endif
