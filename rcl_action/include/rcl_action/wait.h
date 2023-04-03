// Copyright 2018 Open Source Robotics Foundation, Inc.
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

#ifndef RCL_ACTION__WAIT_H_
#define RCL_ACTION__WAIT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/wait.h"
#include "rcl_action/action_client.h"
#include "rcl_action/action_server.h"
#include "rcl_action/visibility_control.h"

/// 向 wait set 中添加一个 rcl_action_client_t。
/**
 * 此函数将底层服务客户端和订阅者添加到 wait set 中。
 *
 * 该函数的行为类似于向 wait set 添加订阅，但会添加五个实体：
 *
 * - 三个服务客户端
 * - 两个订阅
 *
 * \see rcl_wait_set_add_subscription
 *
 * 如果此函数因任何原因失败，`client_index` 和 `subscription_index` 不会被设置。
 * 还可能导致提供的 wait set 处于不一致状态（例如，某些客户端和订阅已添加到 wait set 中，但并非全部）。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[inout] wait_set 结构，用于存储 action client 服务客户端和订阅
 * \param[in] action_client 要添加到 wait set 的 action client
 * \param[out] client_index 在 wait set 的客户端容器中的起始索引，其中添加了 action clients 底层服务客户端。如果忽略，则可选地设置为 `NULL`。
 * \param[out] subscription_index 在 wait set 的订阅容器中的起始索引，其中添加了 action clients 底层订阅。如果忽略，则可选地设置为 `NULL`。
 * \return `RCL_RET_OK` 如果成功添加，或
 * \return `RCL_RET_WAIT_SET_INVALID` 如果 wait set 为零初始化，或
 * \return `RCL_RET_WAIT_SET_FULL` 如果订阅集已满，或
 * \return `RCL_RET_ACTION_CLIENT_INVALID` 如果 action client 无效，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
*/
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_wait_set_add_action_client(
  rcl_wait_set_t * wait_set, const rcl_action_client_t * action_client, size_t * client_index,
  size_t * subscription_index);

/// 向 wait set 中添加一个 rcl_action_server_t。
/**
 * 此函数将底层服务添加到 wait set 中。
 *
 * 该函数的行为类似于向 wait set 添加服务，但会添加三个服务。
 *
 * \see rcl_wait_set_add_service
 *
 * 如果此函数因任何原因失败，`service_index` 不会被设置。
 * 还可能导致提供的 wait set 处于不一致状态（例如，某些客户端和订阅者已添加到 wait set 中，但并非全部）。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[inout] wait_set 结构，用于存储 action server 服务
 * \param[in] action_server 要添加到 wait set 的 action server
 * \param[out] service_index 在 wait set 的服务容器中的起始索引，其中添加了 action servers 底层服务。如果忽略，则可选地设置为 `NULL`。
 * \return `RCL_RET_OK` 如果成功添加，或
 * \return `RCL_RET_WAIT_SET_INVALID` 如果 wait set 为零初始化，或
 * \return `RCL_RET_WAIT_SET_FULL` 如果订阅集已满，或
 * \return `RCL_RET_ACTION_SERVER_INVALID` 如果 action server 无效，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
*/
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_wait_set_add_action_server(
  rcl_wait_set_t * wait_set, const rcl_action_server_t * action_server, size_t * service_index);

/// 获取与 rcl_action_client_t 关联的 wait set 实体数量。
/**
 * 返回在调用 rcl_action_wait_set_add_action_client() 时添加到 wait set 的实体数量。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] action_client 要查询的 action client
 * \param[out] num_subscriptions 当将 action client 添加到 wait set 时添加的订阅数量
 * \param[out] num_guard_conditions 当将 action client 添加到 wait set 时添加的保护条件数量
 * \param[out] num_timers 当将 action client 添加到 wait set 时添加的计时器数量
 * \param[out] num_clients 当将 action client 添加到 wait set 时添加的客户端数量
 * \param[out] num_services 当将 action client 添加到 wait set 时添加的服务数量
 * \return `RCL_RET_OK` 如果调用成功，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_ACTION_CLIENT_INVALID` 如果 action client 无效，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
*/
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_client_wait_set_get_num_entities(
  const rcl_action_client_t * action_client, size_t * num_subscriptions,
  size_t * num_guard_conditions, size_t * num_timers, size_t * num_clients, size_t * num_services);

/// 获取与 rcl_action_server_t 关联的等待集实体数量。
/**
 * 如果调用 rcl_action_wait_set_add_action_server()，则返回添加到等待集的实体数量。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] action_server 要查询的动作服务器
 * \param[out] num_subscriptions 当将动作服务器添加到等待集时添加的订阅数
 * \param[out] num_guard_conditions 当将动作服务器添加到等待集时添加的保护条件数
 * \param[out] num_timers 当将动作服务器添加到等待集时添加的计时器数
 * \param[out] num_clients 当将动作服务器添加到等待集时添加的客户端数
 * \param[out] num_services 当将动作服务器添加到等待集时添加的服务数
 * \return `RCL_RET_OK` 如果调用成功，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_ACTION_SERVER_INVALID` 如果动作服务器无效，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
*/
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_server_wait_set_get_num_entities(
  const rcl_action_server_t * action_server, size_t * num_subscriptions,
  size_t * num_guard_conditions, size_t * num_timers, size_t * num_clients, size_t * num_services);

/// 获取 rcl_action_client_t 的准备好的等待集实体。
/**
 * 调用者可以使用此函数确定要调用的相关动作客户端函数：
 * rcl_action_take_feedback()、rcl_action_take_status()、
 * rcl_action_take_goal_response()、rcl_action_take_cancel_response() 或
 * rcl_action_take_result_response()。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] wait_set 存储动作服务器服务的结构
 * \param[in] action_client 要查询的动作客户端
 * \param[out] is_feedback_ready 如果有准备好的反馈消息可获取，则为 `true`，否则为 `false`
 * \param[out] is_status_ready 如果有准备好的状态消息可获取，则为 `true`，否则为 `false`
 * \param[out] is_goal_response_ready 如果有准备好的目标响应消息可获取，则为 `true`，否则为 `false`
 * \param[out] is_cancel_response_ready 如果有准备好的取消响应消息可获取，则为 `true`，否则为 `false`
 * \param[out] is_result_response_ready 如果有准备好的结果响应消息可获取，则为 `true`，否则为 `false`
 * \return `RCL_RET_OK` 如果调用成功，或
 * \return `RCL_RET_WAIT_SET_INVALID` 如果等待集无效，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_ACTION_CLIENT_INVALID` 如果动作客户端无效，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
*/
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_client_wait_set_get_entities_ready(
  const rcl_wait_set_t * wait_set, const rcl_action_client_t * action_client,
  bool * is_feedback_ready, bool * is_status_ready, bool * is_goal_response_ready,
  bool * is_cancel_response_ready, bool * is_result_response_ready);

/// 获取 rcl_action_server_t 的准备好的等待集实体。
/**
 * 调用者可以使用此函数确定要调用的相关动作服务器函数：
 * rcl_action_take_goal_request()、rcl_action_take_cancel_request() 或
 * rcl_action_take_result_request()。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] wait_set 存储动作服务器服务的结构
 * \param[in] action_server 要查询的动作服务器
 * \param[out] is_goal_request_ready 如果有准备好的目标请求消息可获取，则为 `true`，否则为 `false`
 * \param[out] is_cancel_request_ready 如果有准备好的取消请求消息可获取，则为 `true`，否则为 `false`
 * \param[out] is_result_request_ready 如果有准备好的结果请求消息可获取，则为 `true`，否则为 `false`
 * \param[out] is_goal_expired 如果有过期的目标，则为 `true`，否则为 `false`
 * \return `RCL_RET_OK` 如果调用成功，或
 * \return `RCL_RET_WAIT_SET_INVALID` 如果等待集无效，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_ACTION_CLIENT_INVALID` 如果动作服务器无效，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
*/
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_server_wait_set_get_entities_ready(
  const rcl_wait_set_t * wait_set, const rcl_action_server_t * action_server,
  bool * is_goal_request_ready, bool * is_cancel_request_ready, bool * is_result_request_ready,
  bool * is_goal_expired);

#ifdef __cplusplus
}
#endif

#endif  // RCL_ACTION__WAIT_H_
