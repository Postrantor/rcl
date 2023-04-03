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

#ifndef RCL__NETWORK_FLOW_ENDPOINTS_H_
#define RCL__NETWORK_FLOW_ENDPOINTS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <rmw/network_flow_endpoint.h>
#include <rmw/network_flow_endpoint_array.h>

#include "rcl/allocator.h"
#include "rcl/arguments.h"
#include "rcl/context.h"
#include "rcl/macros.h"
#include "rcl/publisher.h"
#include "rcl/subscription.h"
#include "rcl/types.h"
#include "rcl/visibility_control.h"

// 定义 rcl_network_flow_endpoint_t 类型为 rmw_network_flow_endpoint_t 类型
typedef rmw_network_flow_endpoint_t rcl_network_flow_endpoint_t;
// 定义 rcl_network_flow_endpoint_array_t 类型为 rmw_network_flow_endpoint_array_t 类型
typedef rmw_network_flow_endpoint_array_t rcl_network_flow_endpoint_array_t;
// 定义 rcl_transport_protocol_t 类型为 rmw_transport_protocol_t 类型
typedef rmw_transport_protocol_t rcl_transport_protocol_t;
// 定义 rcl_internet_protocol_t 类型为 rmw_internet_protocol_t 类型
typedef rmw_internet_protocol_t rcl_internet_protocol_t;

// 定义 rcl_get_zero_initialized_network_flow_endpoint_array 为 rmw_get_zero_initialized_network_flow_endpoint_array
#define rcl_get_zero_initialized_network_flow_endpoint_array \
  rmw_get_zero_initialized_network_flow_endpoint_array
// 定义 rcl_network_flow_endpoint_array_fini 为 rmw_network_flow_endpoint_array_fini
#define rcl_network_flow_endpoint_array_fini rmw_network_flow_endpoint_array_fini

// 定义 rcl_network_flow_endpoint_get_transport_protocol_string 为 rmw_network_flow_endpoint_get_transport_protocol_string
#define rcl_network_flow_endpoint_get_transport_protocol_string \
  rmw_network_flow_endpoint_get_transport_protocol_string
// 定义 rcl_network_flow_endpoint_get_internet_protocol_string 为 rmw_network_flow_endpoint_get_internet_protocol_string
#define rcl_network_flow_endpoint_get_internet_protocol_string \
  rmw_network_flow_endpoint_get_internet_protocol_string

/// 获取发布者的网络流端点
/**
 * 查询给定发布者的底层中间件的网络流端点
 *
 * `publisher` 参数必须指向一个有效的发布者。
 *
 * `allocator` 参数必须是一个有效的分配器。
 *
 * `network_flow_endpoint_array` 参数必须已分配且初始化为零。
 * 该函数返回 `network_flow_endpoint_array` 参数中的网络流端点，
 * 在需要时使用分配器为 `network_flow_endpoint_array`
 * 参数的内部数据结构分配内存。调用者负责通过将 `network_flow_endpoint_array`
 * 参数传递给 `rcl_network_flow_endpoint_array_fini` 函数来进行内存释放。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 是
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 可能 [1]
 * <i>[1] 实现可能需要使用锁保护数据结构</i>
 *
 * \param[in] publisher 要检查的发布者实例
 * \param[in] allocator 用于为 network_flow_endpoint_array_t 分配空间的分配器
 * \param[out] network_flow_endpoint_array 网络流端点
 * \return `RCL_RET_OK` 如果成功，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数为空，或
 * \return `RCL_RET_BAD_ALLOC` 如果内存分配失败，或
 * \return `RCL_RET_UNSUPPORTED` 如果不支持，或
 * \return `RCL_RET_ERROR` 如果发生意外错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_publisher_get_network_flow_endpoints(
  const rcl_publisher_t * publisher, rcutils_allocator_t * allocator,
  rcl_network_flow_endpoint_array_t * network_flow_endpoint_array);

/// 获取订阅的网络流端点
/**
 * 查询给定订阅的底层中间件的网络流端点
 *
 * `subscription` 参数必须指向一个有效的订阅。
 *
 * `allocator` 参数必须是一个有效的分配器。
 *
 * `network_flow_endpoint_array` 参数必须已分配且初始化为零。
 * 该函数返回 `network_flow_endpoint_array` 参数中的网络流端点，
 * 在需要时使用分配器为 `network_flow_endpoint_array`
 * 参数的内部数据结构分配内存。调用者负责通过将 `network_flow_endpoint_array`
 * 参数传递给 `rcl_network_flow_endpoint_array_fini` 函数来进行内存释放。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 是
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 可能 [1]
 * <i>[1] 实现可能需要使用锁保护数据结构</i>
 *
 * \param[in] subscription 要检查的订阅实例
 * \param[in] allocator 用于为 network_flow_endpoint_array_t 分配空间的分配器
 * \param[out] network_flow_endpoint_array 网络流端点
 * \return `RCL_RET_OK` 如果成功，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数为空，或
 * \return `RCL_RET_BAD_ALLOC` 如果内存分配失败，或
 * \return `RCL_RET_UNSUPPORTED` 如果不支持，或
 * \return `RCL_RET_ERROR` 如果发生意外错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_subscription_get_network_flow_endpoints(
  const rcl_subscription_t * subscription, rcutils_allocator_t * allocator,
  rcl_network_flow_endpoint_array_t * network_flow_endpoint_array);

#ifdef __cplusplus
}
#endif

#endif  // RCL__NETWORK_FLOW_ENDPOINTS_H_
