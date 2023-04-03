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

/// @file

#ifndef RCL__GRAPH_H_
#define RCL__GRAPH_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <rmw/get_topic_names_and_types.h>
#include <rmw/names_and_types.h>
#include <rmw/topic_endpoint_info_array.h>

#include "rcl/client.h"
#include "rcl/macros.h"
#include "rcl/node.h"
#include "rcl/visibility_control.h"
#include "rcutils/time.h"
#include "rcutils/types.h"
#include "rosidl_runtime_c/service_type_support_struct.h"

/// 包含主题名称和类型的结构体。
typedef rmw_names_and_types_t rcl_names_and_types_t;

/// 封装节点名称、节点命名空间、主题类型、gid 和 qos_profile 的结构体，用于主题的发布者和订阅者。
typedef rmw_topic_endpoint_info_t rcl_topic_endpoint_info_t;

/// 主题端点信息的数组。
typedef rmw_topic_endpoint_info_array_t rcl_topic_endpoint_info_array_t;

/// 返回一个零初始化的 rcl_names_and_types_t 结构体。
#define rcl_get_zero_initialized_names_and_types rmw_get_zero_initialized_names_and_types

/// 返回一个零初始化的 rcl_topic_endpoint_info_t 结构体。
#define rcl_get_zero_initialized_topic_endpoint_info_array \
  rmw_get_zero_initialized_topic_endpoint_info_array

/// 结束一个 topic_endpoint_info_array_t 结构体。
#define rcl_topic_endpoint_info_array_fini rmw_topic_endpoint_info_array_fini

/**
 * \brief 返回与节点关联的发布者的主题名称和类型列表。
 *
 * `node` 参数必须指向一个有效的节点。
 *
 * `topic_names_and_types` 参数必须已分配并零初始化。
 * 此函数为返回的名称和类型列表分配内存，因此调用者有责任在不再需要 `topic_names_and_types` 时将其传递给 rcl_names_and_types_fini()。
 * 如果没有这样做，将导致内存泄漏。
 *
 * 当从中间件实现列出名称时，可能会发生一些解析操作。
 * 如果 `no_demangle` 参数设置为 `true`，则将避免这种情况，名称将按照中间件显示的方式返回。
 *
 * \see rmw_get_topic_names_and_types 以获取有关 no_demangle 的更多详细信息
 *
 * 此函数不会自动重新映射返回的名称。
 * 使用此函数返回的名称创建发布者或订阅者可能不会导致使用所需的主题名称，具体取决于正在使用的重映射规则。
 *
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 是
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 可能 [1]
 * <i>[1] 实现可能需要用锁保护数据结构</i>
 *
 * \param[in] node 正在用于查询 ROS 图的节点句柄
 * \param[in] allocator 用于为字符串分配空间时使用的分配器
 * \param[in] no_demangle 如果为 true，则列出所有主题而无需解析
 * \param[in] node_name 要返回的主题的节点名称
 * \param[in] node_namespace 要返回的主题的节点命名空间
 * \param[out] topic_names_and_types 主题名称及其类型列表
 * \return #RCL_RET_OK 如果查询成功，或
 * \return #RCL_RET_NODE_INVALID 如果节点无效，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_NODE_INVALID_NAME 如果节点名称无效，或
 * \return #RCL_RET_NODE_INVALID_NAMESPACE 如果节点命名空间无效，或
 * \return #RCL_RET_NODE_NAME_NON_EXISTENT 如果未找到节点名称，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_get_publisher_names_and_types_by_node(
  const rcl_node_t * node, rcl_allocator_t * allocator, bool no_demangle, const char * node_name,
  const char * node_namespace, rcl_names_and_types_t * topic_names_and_types);

/// 返回与节点关联的订阅主题名称和类型列表。
/**
 * `node` 参数必须指向一个有效的节点。
 *
 * `topic_names_and_types` 参数必须已分配并初始化为零。
 * 此函数为返回的名称和类型列表分配内存，因此调用者有责任在不再需要时将 `topic_names_and_types` 传递给 rcl_names_and_types_fini()。
 * 如果没有这样做，将导致内存泄漏。
 *
 * \see rcl_get_publisher_names_and_types_by_node 关于 `no_demangle` 参数的详细信息。
 *
 * 此函数不会自动重映射返回的名称。
 * 尝试使用此函数返回的名称创建发布者或订阅者可能不会根据正在使用的重映射规则得到期望的主题名称。
 *
 * <hr>
 * 属性                | 遵循性
 * ------------------ | -------------
 * 分配内存            | 是
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 可能 [1]
 * <i>[1] 实现可能需要用锁保护数据结构</i>
 *
 * \param[in] node 正在用于查询 ROS 图的节点句柄
 * \param[in] allocator 用于为字符串分配空间的分配器
 * \param[in] no_demangle 如果为 true，则列出所有未经处理的主题
 * \param[in] node_name 要返回的主题的节点名称
 * \param[in] node_namespace 要返回的主题的节点命名空间
 * \param[out] topic_names_and_types 主题名称及其类型列表
 * \return #RCL_RET_OK 如果查询成功，或
 * \return #RCL_RET_NODE_INVALID 如果节点无效，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_NODE_INVALID_NAME 如果节点名称无效，或
 * \return #RCL_RET_NODE_INVALID_NAMESPACE 如果节点命名空间无效，或
 * \return #RCL_RET_NODE_NAME_NON_EXISTENT 如果未找到节点名称，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_get_subscriber_names_and_types_by_node(
  const rcl_node_t * node, rcl_allocator_t * allocator, bool no_demangle, const char * node_name,
  const char * node_namespace, rcl_names_and_types_t * topic_names_and_types);

/// 返回与节点关联的服务名称和类型列表。
/**
 * `node` 参数必须指向一个有效的节点。
 *
 * `service_names_and_types` 参数必须已分配并初始化为零。
 * 此函数为返回的名称和类型列表分配内存，因此调用者有责任在不再需要时将 `service_names_and_types` 传递给 rcl_names_and_types_fini()。
 * 如果没有这样做，将导致内存泄漏。
 *
 * \see rcl_get_publisher_names_and_types_by_node 关于 `no_demangle` 参数的详细信息。
 *
 * 此函数不会自动重映射返回的名称。
 * 尝试使用此函数返回的名称创建服务客户端可能不会根据正在使用的重映射规则得到期望的服务名称。
 *
 * <hr>
 * 属性                | 遵循性
 * ------------------ | -------------
 * 分配内存            | 是
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 可能 [1]
 * <i>[1] 实现可能需要用锁保护数据结构</i>
 *
 * \param[in] node 正在用于查询 ROS 图的节点句柄
 * \param[in] allocator 用于为字符串分配空间的分配器
 * \param[in] node_name 要返回的服务的节点名称
 * \param[in] node_namespace 要返回的服务的节点命名空间
 * \param[out] service_names_and_types 服务名称及其类型列表
 * \return #RCL_RET_OK 如果查询成功，或
 * \return #RCL_RET_NODE_INVALID 如果节点无效，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_NODE_INVALID_NAME 如果节点名称无效，或
 * \return #RCL_RET_NODE_INVALID_NAMESPACE 如果节点命名空间无效，或
 * \return #RCL_RET_NODE_NAME_NON_EXISTENT 如果未找到节点名称，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_get_service_names_and_types_by_node(
  const rcl_node_t * node, rcl_allocator_t * allocator, const char * node_name,
  const char * node_namespace, rcl_names_and_types_t * service_names_and_types);

/// 返回与节点关联的服务客户端名称和类型列表。
/**
 * `node` 参数必须指向一个有效的节点。
 *
 * `service_names_and_types` 参数必须分配并初始化为零。
 * 此函数为返回的名称和类型列表分配内存，因此调用者有责任在不再需要时将 `service_names_and_types` 传递给 rcl_names_and_types_fini()。
 * 如果没有这样做，将导致内存泄漏。
 *
 * \see rcl_get_publisher_names_and_types_by_node 关于 `no_demangle` 参数的详细信息。
 *
 * 此函数不会自动重映射返回的名称。
 * 尝试使用此函数返回的名称创建服务服务器可能不会导致使用所需的服务名称，具体取决于正在使用的重映射规则。
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存           | 是
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 可能 [1]
 * <i>[1] 实现可能需要用锁保护数据结构</i>
 *
 * \param[in] node 正在用于查询 ROS 图的节点句柄
 * \param[in] allocator 用于为字符串分配空间时使用的分配器
 * \param[in] node_name 要返回的服务的节点名称
 * \param[in] node_namespace 要返回的服务的节点命名空间
 * \param[out] service_names_and_types 服务客户端名称及其类型列表
 * \return #RCL_RET_OK 如果查询成功，或
 * \return #RCL_RET_NODE_INVALID 如果节点无效，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_NODE_INVALID_NAME 如果节点名称无效，或
 * \return #RCL_RET_NODE_INVALID_NAMESPACE 如果节点命名空间无效，或
 * \return #RCL_RET_NODE_NAME_NON_EXISTENT 如果未找到节点名称，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_get_client_names_and_types_by_node(
  const rcl_node_t * node, rcl_allocator_t * allocator, const char * node_name,
  const char * node_namespace, rcl_names_and_types_t * service_names_and_types);

/// 返回主题名称及其类型列表。
/**
 * `node` 参数必须指向一个有效的节点。
 *
 * `topic_names_and_types` 参数必须分配并初始化为零。
 * 此函数为返回的名称和类型列表分配内存，因此调用者有责任在不再需要时将 `topic_names_and_types` 传递给 rcl_names_and_types_fini()。
 * 如果没有这样做，将导致内存泄漏。
 *
 * \see rcl_get_publisher_names_and_types_by_node 关于 `no_demangle` 参数的详细信息。
 *
 * 此函数不会自动重映射返回的名称。
 * 尝试使用此函数返回的名称创建发布者或订阅者可能不会导致使用所需的主题名称，具体取决于正在使用的重映射规则。
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存           | 是
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 可能 [1]
 * <i>[1] 实现可能需要用锁保护数据结构</i>
 *
 * \param[in] node 正在用于查询 ROS 图的节点句柄
 * \param[in] allocator 用于为字符串分配空间时使用的分配器
 * \param[in] no_demangle 如果为 true，则列出所有主题而不进行任何解扰
 * \param[out] topic_names_and_types 主题名称及其类型列表
 * \return #RCL_RET_OK 如果查询成功，或
 * \return #RCL_RET_NODE_INVALID 如果节点无效，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_get_topic_names_and_types(
  const rcl_node_t * node, rcl_allocator_t * allocator, bool no_demangle,
  rcl_names_and_types_t * topic_names_and_types);

/// 返回服务名称及其类型列表。
/**
 * `node` 参数必须指向一个有效的节点。
 *
 * `service_names_and_types` 参数必须分配并初始化为零。
 * 此函数为返回的名称和类型列表分配内存，因此调用者有责任在不再需要时将 `service_names_and_types` 传递给 rcl_names_and_types_fini()。
 * 如果没有这样做，将导致内存泄漏。
 *
 * 此函数不会自动重映射返回的名称。
 * 尝试使用此函数返回的名称创建客户端或服务可能不会导致使用所需的服务名称，具体取决于正在使用的重映射规则。
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存           | 是
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 可能 [1]
 * <i>[1] 实现可能需要用锁保护数据结构</i>
 *
 * \param[in] node 正在用于查询 ROS 图的节点句柄
 * \param[in] allocator 用于为字符串分配空间时使用的分配器
 * \param[out] service_names_and_types 服务名称及其类型列表
 * \return #RCL_RET_OK 如果查询成功，或
 * \return #RCL_RET_NODE_INVALID 如果节点无效，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_get_service_names_and_types(
  const rcl_node_t * node, rcl_allocator_t * allocator,
  rcl_names_and_types_t * service_names_and_types);

/// 初始化一个 rcl_names_and_types_t 对象。
/**
 * 此函数初始化 names 的字符串数组，并根据给定的大小为类型的所有字符串数组分配空间，
 * 但不会初始化每组类型的字符串数组。
 * 然而，每组类型的字符串数组都是零初始化的。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 是
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * \param[inout] names_and_types 要初始化的对象
 * \param[in] size 要存储的名称和类型集的数量
 * \param[in] allocator 用于分配和释放内存的分配器
 * \return #RCL_RET_OK 成功时返回，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果内存分配失败，或
 * \return #RCL_RET_ERROR 当发生未指定的错误时返回。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_names_and_types_init(
  rcl_names_and_types_t * names_and_types, size_t size, rcl_allocator_t * allocator);

/// 结束一个 rcl_names_and_types_t 对象。
/**
 * 当对象传递给 rcl_get_*_names_and_types() 函数之一时，对象将被填充。
 * 此函数回收在填充过程中分配的任何资源。
 *
 * `names_and_types` 参数不能为 `NULL`，并且必须指向一个已分配的 rcl_names_and_types_t 结构，
 * 该结构之前已传递给成功的 rcl_get_*_names_and_types() 函数调用。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 是
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * \param[inout] names_and_types 要结束的结构
 * \return #RCL_RET_OK 如果成功，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_names_and_types_fini(rcl_names_and_types_t * names_and_types);

/// 返回 ROS 图中可用节点的列表。
/**
 * `node` 参数必须指向一个有效的节点。
 *
 * `node_names` 参数必须是分配和零初始化的。
 * `node_names` 是此函数的输出，并包含分配的内存。
 * 使用 rcutils_get_zero_initialized_string_array() 初始化空的 rcutils_string_array_t 结构。
 * 因此，当不再需要此 `node_names` 结构时，应将其传递给 rcutils_string_array_fini()。
 * 如果不这样做，将导致内存泄漏。
 *
 * 示例：
 *
 * ```c
 * rcutils_string_array_t node_names =
 *   rcutils_get_zero_initialized_string_array();
 * rcl_ret_t ret = rcl_get_node_names(node, &node_names);
 * if (ret != RCL_RET_OK) {
 *   // ... 错误处理
 * }
 * // ... 使用 node_names 结构，完成后：
 * rcutils_ret_t rcutils_ret = rcutils_string_array_fini(&node_names);
 * if (rcutils_ret != RCUTILS_RET_OK) {
 *   // ... 错误处理
 * }
 * ```
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 是
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 可能 [1]
 * <i>[1] 实现可能需要使用锁保护数据结构</i>
 *
 * \param[in] node 正在用于查询 ROS 图的节点句柄
 * \param[in] allocator 用于控制名称分配和释放的分配器
 * \param[out] node_names 存储发现的节点名称的结构
 * \param[out] node_namespaces 存储发现的节点命名空间的结构
 * \return #RCL_RET_OK 如果查询成功，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存时发生错误，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_NODE_INVALID_NAME 如果检测到具有无效名称的节点，或
 * \return #RCL_RET_NODE_INVALID_NAMESPACE 如果检测到具有无效命名空间的节点，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_get_node_names(
  const rcl_node_t * node, rcl_allocator_t allocator, rcutils_string_array_t * node_names,
  rcutils_string_array_t * node_namespaces);

/// 返回 ROS 图中可用节点的列表，包括它们的围场名称。
/**
 * 一个与 rcl_get_node_names() 等效的函数，但在输出中包含节点使用的围场名称。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 是
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 可能 [1]
 * <i>[1] 使用的 RMW 实现可能需要使用锁保护数据结构</i>
 *
 * \param[in] node 正在用于查询 ROS 图的节点句柄
 * \param[in] allocator 用于控制名称分配和释放的分配器
 * \param[out] node_names 存储发现的节点名称的结构
 * \param[out] node_namespaces 存储发现的节点命名空间的结构
 * \param[out] enclaves 存储发现的节点围场的结构
 * \return #RCL_RET_OK 如果查询成功，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存时发生错误，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_get_node_names_with_enclaves(
  const rcl_node_t * node, rcl_allocator_t allocator, rcutils_string_array_t * node_names,
  rcutils_string_array_t * node_namespaces, rcutils_string_array_t * enclaves);

/// 返回给定主题上的发布者数量。
/**
 * `node` 参数必须指向一个有效的节点。
 *
 * `topic_name` 参数不能为空（`NULL`），也不能是空字符串。
 * 它还应遵循主题名称规则。
 * \todo TODO(wjwwood): 链接到主题名称规则。
 *
 * `count` 参数必须指向一个有效的布尔值。
 * `count` 参数是此函数的输出，将被设置。
 *
 * 如果错误处理需要分配内存，此函数将尝试使用节点的分配器。
 *
 * 此函数不会自动重新映射主题名称。
 * 如果有一个创建了主题名为 `foo` 的发布者和重映射规则 `foo:=bar`，那么调用
 * 这个函数并将 `topic_name` 设置为 `bar` 将返回计数 1，将 `topic_name` 设置为 `foo`
 * 将返回计数 0。
 * /sa rcl_remap_topic_name()
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 可能 [1]
 * <i>[1] 实现可能需要用锁保护数据结构</i>
 *
 * \param[in] node 正在用于查询 ROS 图的节点句柄
 * \param[in] topic_name 要查询的主题名称
 * \param[out] count 给定主题上的发布者数量
 * \return #RCL_RET_OK 如果查询成功，或
 * \return #RCL_RET_NODE_INVALID 如果节点无效，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_count_publishers(const rcl_node_t * node, const char * topic_name, size_t * count);

/// 返回给定主题上的订阅者数量。
/**
 * `node` 参数必须指向一个有效的节点。
 *
 * `topic_name` 参数不能为空（`NULL`），也不能是空字符串。
 * 它还应遵循主题名称规则。
 * \todo TODO(wjwwood): 链接到主题名称规则。
 *
 * `count` 参数必须指向一个有效的布尔值。
 * `count` 参数是此函数的输出，将被设置。
 *
 * 如果错误处理需要分配内存，此函数将尝试使用节点的分配器。
 *
 * 此函数不会自动重新映射主题名称。
 * 如果有一个创建了主题名为 `foo` 的订阅者和重映射规则 `foo:=bar`，那么调用
 * 这个函数并将 `topic_name` 设置为 `bar` 将返回计数 1，将 `topic_name` 设置为 `foo`
 * 将返回计数 0。
 * /sa rcl_remap_topic_name()
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 可能 [1]
 * <i>[1] 实现可能需要用锁保护数据结构</i>
 *
 * \param[in] node 正在用于查询 ROS 图的节点句柄
 * \param[in] topic_name 要查询的主题名称
 * \param[out] count 给定主题上的订阅者数量
 * \return #RCL_RET_OK 如果查询成功，或
 * \return #RCL_RET_NODE_INVALID 如果节点无效，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_count_subscribers(const rcl_node_t * node, const char * topic_name, size_t * count);

/// 等待给定主题上的发布者数量达到指定数量。
/**
 * `node` 参数必须指向一个有效的节点。
 * 该节点的图形保护条件将被此函数使用，因此调用者应注意不要在其他等待集中并发使用保护条件。
 *
 * `allocator` 参数必须指向一个有效的分配器。
 *
 * `topic_name` 参数不能为空（`NULL`），也不能是空字符串。
 * 它还应遵循主题名称规则。
 *
 * 此函数会阻塞，并在 `topic_name` 的发布者数量大于或等于 `count` 参数，或达到指定的 `timeout` 时返回。
 *
 * `timeout` 参数以纳秒为单位。
 * 超时基于系统时间消耗。
 * 负值将禁用超时（即此函数将阻塞，直到发布者数量大于或等于 `count`）。
 *
 * `success` 参数必须指向一个有效的布尔值。
 * `success` 参数是此函数的输出，将被设置。
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 可能 [1]
 * <i>[1] 实现可能需要用锁保护数据结构</i>
 *
 * \param[in] node 正在用于查询 ROS 图的节点句柄
 * \param[in] allocator 为 rcl_wait_set_t 分配空间，用于等待图形事件
 * \param[in] topic_name 要查询的主题名称
 * \param[in] count 要等待的发布者数量
 * \param[in] timeout 等待发布者的最长时间
 * \param[out] success 如果发布者数量等于或大于计数，则为 `true`，
 *   或者如果在等待发布者时发生超时，则为 `false`。
 * \return #RCL_RET_OK 如果没有错误，或
 * \return #RCL_RET_NODE_INVALID 如果节点无效，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_TIMEOUT 如果在检测到发布者数量之前发生超时，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_wait_for_publishers(
  const rcl_node_t * node, rcl_allocator_t * allocator, const char * topic_name, const size_t count,
  rcutils_duration_value_t timeout, bool * success);

/// 等待指定主题上有指定数量的订阅者。
/**
 * \see rcl_wait_for_publishers
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 可能 [1]
 * <i>[1] 实现可能需要用锁保护数据结构</i>
 *
 * \param[in] node 正在使用的节点句柄，用于查询ROS图
 * \param[in] allocator 为rcl_wait_set_t分配空间，用于等待图形事件
 * \param[in] topic_name 相关主题的名称
 * \param[in] count 要等待的订阅者数量
 * \param[in] timeout 等待订阅者的最长时间
 * \param[out] success 如果订阅者数量等于或大于count，则为`true`，
 *   否则，如果在等待订阅者时发生超时，则为`false`。
 * \return #RCL_RET_OK 如果没有错误，或
 * \return #RCL_RET_NODE_INVALID 如果节点无效，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_TIMEOUT 如果在检测到订阅者数量之前发生超时，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_wait_for_subscribers(
  const rcl_node_t * node, rcl_allocator_t * allocator, const char * topic_name, const size_t count,
  rcutils_duration_value_t timeout, bool * success);

/// 返回一个主题的所有发布者列表。
/**
 * `node` 参数必须指向一个有效的节点。
 *
 * `topic_name` 参数不能为空（`NULL`）。
 *
 * 当 `no_mangle` 参数为 `true` 时，提供的 `topic_name` 应该是中间件的有效主题名称
 * （在将 ROS 与原生中间件（如 DDS）应用程序结合使用时有用）。
 * 当 `no_mangle` 参数为 `false` 时，提供的 `topic_name` 应遵循
 * ROS 主题命名约定。
 * 在任何情况下，主题名称都应该是完全限定的。
 *
 * `publishers_info` 数组中的每个元素都将包含节点名称、节点命名空间、
 * 主题类型、gid 和发布者的 qos 配置文件。
 * 调用者有责任确保 `publishers_info` 参数指向
 * 类型为 rcl_topic_endpoint_info_array_t 的有效结构。
 * 结构内的 `count` 字段必须设置为 0，结构内的 `info_array` 字段必须设置为 null。
 * \see rmw_get_zero_initialized_topic_endpoint_info_array
 *
 * `allocator` 将用于为 `publishers_info` 内的 `info_array` 成员分配内存。
 * 此外，rmw_topic_endpoint_info_t 内的每个 const char * 成员都将在分配的内存上分配一个复制值。
 * \see rmw_topic_endpoint_info_set_node_name 等。
 * 但是，调用者有责任
 * 回收分配给 `publishers_info` 的任何资源，以避免内存泄漏。
 * \see rmw_topic_endpoint_info_array_fini
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 可能 [1]
 * <i>[1] 实现可能需要用锁保护数据结构</i>
 *
 * \param[in] node 正在使用的节点句柄，用于查询ROS图
 * \param[in] allocator 在 publishers_info 内为数组分配空间时使用的分配器
 * \param[in] topic_name 相关主题的名称
 * \param[in] no_mangle 如果为 `true`，`topic_name` 需要是有效的中间件主题名称，
 *            否则应该是有效的 ROS 主题名称
 * \param[out] publishers_info 表示发布者信息列表的结构
 * \return #RCL_RET_OK 如果查询成功，或
 * \return #RCL_RET_NODE_INVALID 如果节点无效，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果内存分配失败，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_get_publishers_info_by_topic(
  const rcl_node_t * node, rcutils_allocator_t * allocator, const char * topic_name, bool no_mangle,
  rcl_topic_endpoint_info_array_t * publishers_info);

/// 返回一个主题的所有订阅者列表。
/**
 * `node` 参数必须指向一个有效的节点。
 *
 * `topic_name` 参数不能为空（`NULL`）。
 *
 * 当 `no_mangle` 参数为 `true` 时，提供的 `topic_name` 应该是中间件的有效主题名称
 * （在将 ROS 与原生中间件（如 DDS）应用程序结合使用时有用）。
 * 当 `no_mangle` 参数为 `false` 时，提供的 `topic_name` 应遵循
 * ROS 主题命名约定。
 * 在任何情况下，主题名称都应该是完全限定的。
 *
 * `subscriptions_info` 数组中的每个元素都将包含节点名称、节点命名空间、
 * 主题类型、gid 和订阅的 qos 配置文件。
 * 调用者有责任确保 `subscriptions_info` 参数指向
 * 类型为 rcl_topic_endpoint_info_array_t 的有效结构。
 * 结构内的 `count` 字段必须设置为 0，结构内的 `info_array` 字段必须设置为 null。
 * \see rmw_get_zero_initialized_topic_endpoint_info_array
 *
 * `allocator` 将用于为 `subscriptions_info` 内的 `info_array` 成员分配内存。
 * 此外，rmw_topic_endpoint_info_t 内的每个 const char * 成员都将在分配的内存上分配一个复制值。
 * \see rmw_topic_endpoint_info_set_node_name 等。
 * 但是，调用者有责任
 * 回收分配给 `subscriptions_info` 的任何资源，以避免内存泄漏。
 * \see rmw_topic_endpoint_info_array_fini
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 可能 [1]
 * <i>[1] 实现可能需要用锁保护数据结构</i>
 *
 * \param[in] node 正在使用的节点句柄，用于查询ROS图
 * \param[in] allocator 在 publishers_info 内为数组分配空间时使用的分配器
 * \param[in] topic_name 相关主题的名称
 * \param[in] no_mangle 如果为 `true`，`topic_name` 需要是有效的中间件主题名称，
 *            否则应该是有效的 ROS 主题名称
 * \param[out] subscriptions_info 表示订阅信息列表的结构
 * \return #RCL_RET_OK 如果查询成功，或
 * \return #RCL_RET_NODE_INVALID 如果节点无效，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果内存分配失败，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_get_subscriptions_info_by_topic(
  const rcl_node_t * node, rcutils_allocator_t * allocator, const char * topic_name, bool no_mangle,
  rcl_topic_endpoint_info_array_t * subscriptions_info);

/// 检查给定服务客户端是否有可用的服务服务器。
/**
 * 如果给定客户端有可用的服务服务器，此函数将为 `is_available` 返回 true。
 *
 * `node` 参数必须指向一个有效的节点。
 *
 * `client` 参数必须指向一个有效的客户端。
 *
 * 给定的客户端和节点必须匹配，即客户端必须使用给定的节点创建。
 *
 * `is_available` 参数不能为空（`NULL`），并且必须指向一个布尔变量。
 * 检查结果将存储在 `is_available` 参数中。
 *
 * 如果错误处理需要分配内存，此函数
 * 将尝试使用节点的分配器。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 可能 [1]
 * <i>[1] 实现可能需要用锁保护数据结构</i>
 *
 * \param[in] node 正在用于查询ROS图的节点句柄
 * \param[in] client 正在查询的服务客户端句柄
 * \param[out] is_available 如果有可用的服务服务器，则设置为true，否则为false
 * \return #RCL_RET_OK 如果检查成功完成（无论服务是否准备好），或
 * \return #RCL_RET_NODE_INVALID 如果节点无效，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_service_server_is_available(
  const rcl_node_t * node, const rcl_client_t * client, bool * is_available);

#ifdef __cplusplus
}
#endif

#endif  // RCL__GRAPH_H_
