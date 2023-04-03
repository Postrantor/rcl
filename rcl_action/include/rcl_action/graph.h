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

#ifndef RCL_ACTION__GRAPH_H_
#define RCL_ACTION__GRAPH_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/graph.h"
#include "rcl/node.h"
#include "rcl_action/visibility_control.h"

/// 获取与节点关联的动作客户端的动作名称和类型列表。
/**
 * `node` 参数必须指向一个有效的节点。
 *
 * `action_names_and_types` 参数必须分配并初始化为零。
 * 此函数为返回的名称和类型列表分配内存，因此调用者有责任在不再需要时将 `action_names_and_types` 传递给 rcl_names_and_types_fini()。
 * 否则将导致内存泄漏。
 *
 * 此函数不会自动重新映射返回的名称。
 * 尝试使用此函数返回的名称创建动作客户端或动作服务器可能不会得到所需的动作名称，具体取决于正在使用的重映射规则。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 是
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 可能 [1]
 * <i>[1] 实现可能需要用锁保护数据结构</i>
 *
 * \param[in] node 正在用于查询 ROS 图的节点句柄
 * \param[in] allocator 为字符串分配空间的分配器
 * \param[in] node_name 要返回的动作的节点名称
 * \param[in] node_namespace 要返回的动作的节点命名空间
 * \param[out] action_names_and_types 动作名称及其类型列表
 * \return `RCL_RET_OK` 如果查询成功，或
 * \return `RCL_RET_NODE_INVALID` 如果节点无效，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_get_client_names_and_types_by_node(
  const rcl_node_t * node, rcl_allocator_t * allocator, const char * node_name,
  const char * node_namespace, rcl_names_and_types_t * action_names_and_types);

/// 获取与节点关联的动作服务器的动作名称和类型列表。
/**
 * 此函数返回与提供的节点名称关联的动作服务器的动作名称和类型列表。
 *
 * `node` 参数必须指向一个有效的节点。
 *
 * `action_names_and_types` 参数必须分配并初始化为零。
 * 此函数为返回的名称和类型列表分配内存，因此调用者有责任在不再需要时将 `action_names_and_types` 传递给 rcl_names_and_types_fini()。
 * 否则将导致内存泄漏。
 *
 * 此函数不会自动重新映射返回的名称。
 * 尝试使用此函数返回的名称创建动作客户端或动作服务器可能不会得到所需的动作名称，具体取决于正在使用的重映射规则。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 是
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 可能 [1]
 * <i>[1] 实现可能需要用锁保护数据结构</i>
 *
 * \param[in] node 正在用于查询 ROS 图的节点句柄
 * \param[in] allocator 为字符串分配空间的分配器
 * \param[in] node_name 要返回的动作的节点名称
 * \param[in] node_namespace 要返回的动作的节点命名空间
 * \param[out] action_names_and_types 动作名称及其类型列表
 * \return `RCL_RET_OK` 如果查询成功，或
 * \return `RCL_RET_NODE_INVALID` 如果节点无效，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_get_server_names_and_types_by_node(
  const rcl_node_t * node, rcl_allocator_t * allocator, const char * node_name,
  const char * node_namespace, rcl_names_and_types_t * action_names_and_types);

/// 返回动作名称及其类型列表。
/**
 * 此函数返回 ROS 图中的动作名称和类型列表。
 *
 * `node` 参数必须指向一个有效的节点。
 *
 * `action_names_and_types` 参数必须分配并初始化为零。
 * 此函数为返回的名称和类型列表分配内存，因此调用者有责任在不再需要时将 `action_names_and_types` 传递给 rcl_names_and_types_fini()。
 * 否则将导致内存泄漏。
 *
 * 此函数不会自动重新映射返回的名称。
 * 尝试使用此函数返回的名称创建动作客户端或动作服务器可能不会得到所需的动作名称，具体取决于正在使用的重映射规则。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 是
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 可能 [1]
 * <i>[1] 实现可能需要用锁保护数据结构</i>
 *
 * \param[in] node 正在用于查询 ROS 图的节点句柄
 * \param[in] allocator 为字符串分配空间的分配器
 * \param[out] action_names_and_types 动作名称及其类型列表
 * \return `RCL_RET_OK` 如果查询成功，或
 * \return `RCL_RET_NODE_INVALID` 如果节点无效，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_get_names_and_types(
  const rcl_node_t * node, rcl_allocator_t * allocator,
  rcl_names_and_types_t * action_names_and_types);

#ifdef __cplusplus
}
#endif

#endif  // RCL_ACTION__GRAPH_H_
