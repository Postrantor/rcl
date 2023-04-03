// Copyright 2020 Ericsson AB
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

#include "rcl/network_flow_endpoints.h"

#include "./common.h"
#include "rcl/error_handling.h"
#include "rcl/graph.h"
#include "rcl/publisher.h"
#include "rcl/subscription.h"
#include "rcutils/allocator.h"
#include "rcutils/macros.h"
#include "rcutils/types.h"
#include "rmw/error_handling.h"
#include "rmw/get_network_flow_endpoints.h"
#include "rmw/network_flow_endpoint_array.h"
#include "rmw/types.h"

/**
 * @brief 验证网络流端点数组是否有效
 *
 * @param[in] network_flow_endpoint_array 指向要验证的 rcl_network_flow_endpoint_array_t
 * 结构体的指针
 * @return 返回 RCL_RET_OK 表示验证成功，其他值表示失败
 */
rcl_ret_t __validate_network_flow_endpoint_array(
    rcl_network_flow_endpoint_array_t* network_flow_endpoint_array) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(network_flow_endpoint_array, RCL_RET_INVALID_ARGUMENT);

  rmw_error_string_t error_string;
  // 检查网络流端点数组是否为零初始化
  rmw_ret_t rmw_ret = rmw_network_flow_endpoint_array_check_zero(network_flow_endpoint_array);
  if (rmw_ret != RMW_RET_OK) {
    // 获取错误信息并重置错误状态
    error_string = rmw_get_error_string();
    rmw_reset_error();
    // 设置错误消息
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "rcl_network_flow_endpoint_array_t must be zero initialized: %s,\n"
        "Use rcl_get_zero_initialized_network_flow_endpoint_array",
        error_string.str);
  }

  // 转换 rmw_ret 为 rcl_ret 并返回
  return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
}

/**
 * @brief 获取发布者的网络流端点
 *
 * @param[in] publisher 指向 rcl_publisher_t 结构体的指针
 * @param[in] allocator 指向 rcutils_allocator_t 结构体的指针，用于分配内存
 * @param[out] network_flow_endpoint_array 存储获取到的网络流端点数组
 * @return 返回 RCL_RET_OK 表示成功，其他值表示失败
 */
rcl_ret_t rcl_publisher_get_network_flow_endpoints(
    const rcl_publisher_t* publisher,
    rcutils_allocator_t* allocator,
    rcl_network_flow_endpoint_array_t* network_flow_endpoint_array) {
  // 检查发布者是否有效
  if (!rcl_publisher_is_valid(publisher)) {
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);

  // 验证网络流端点数组是否有效
  rcl_ret_t rcl_ret = __validate_network_flow_endpoint_array(network_flow_endpoint_array);
  if (rcl_ret != RCL_RET_OK) {
    return rcl_ret;
  }

  rmw_error_string_t error_string;
  // 获取发布者的网络流端点
  rmw_ret_t rmw_ret = rmw_publisher_get_network_flow_endpoints(
      rcl_publisher_get_rmw_handle(publisher), allocator, network_flow_endpoint_array);
  if (rmw_ret != RMW_RET_OK) {
    // 获取错误信息并重置错误状态
    error_string = rmw_get_error_string();
    rmw_reset_error();
    // 设置错误消息
    RCL_SET_ERROR_MSG(error_string.str);
  }
  // 转换 rmw_ret 为 rcl_ret 并返回
  return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
}

/**
 * @brief 获取订阅者的网络流端点
 *
 * @param[in] subscription 指向 rcl_subscription_t 结构体的指针
 * @param[in] allocator 指向 rcutils_allocator_t 结构体的指针，用于分配内存
 * @param[out] network_flow_endpoint_array 存储获取到的网络流端点数组
 * @return 返回 RCL_RET_OK 表示成功，其他值表示失败
 */
rcl_ret_t rcl_subscription_get_network_flow_endpoints(
    const rcl_subscription_t* subscription,
    rcutils_allocator_t* allocator,
    rcl_network_flow_endpoint_array_t* network_flow_endpoint_array) {
  // 检查订阅者是否有效
  if (!rcl_subscription_is_valid(subscription)) {
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);

  // 验证网络流端点数组是否有效
  rcl_ret_t rcl_ret = __validate_network_flow_endpoint_array(network_flow_endpoint_array);
  if (rcl_ret != RCL_RET_OK) {
    return rcl_ret;
  }

  rmw_error_string_t error_string;
  // 获取订阅者的网络流端点
  rmw_ret_t rmw_ret = rmw_subscription_get_network_flow_endpoints(
      rcl_subscription_get_rmw_handle(subscription), allocator, network_flow_endpoint_array);
  if (rmw_ret != RMW_RET_OK) {
    // 获取错误信息并重置错误状态
    error_string = rmw_get_error_string();
    rmw_reset_error();
    // 设置错误消息
    RCL_SET_ERROR_MSG(error_string.str);
  }
  // 转换 rmw_ret 为 rcl_ret 并返回
  return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
}

#ifdef __cplusplus
}
#endif
