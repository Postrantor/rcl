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

#ifndef COM_INTERFACE_H_
#define COM_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/macros.h"
#include "rcl_lifecycle/data_types.h"
/**
 * @brief 获取一个初始化为零的生命周期通信接口结构体
 *
 * @return 返回一个初始化为零的rcl_lifecycle_com_interface_t结构体
 */
rcl_lifecycle_com_interface_t rcl_lifecycle_get_zero_initialized_com_interface();

/**
 * @brief 初始化生命周期通信接口
 *
 * @param[in] com_interface 指向要初始化的生命周期通信接口结构体的指针
 * @param[in] node_handle 指向节点句柄的指针
 * @param[in] ts_pub_notify 通知发布器的类型支持
 * @param[in] ts_srv_change_state 改变状态服务的类型支持
 * @param[in] ts_srv_get_state 获取状态服务的类型支持
 * @param[in] ts_srv_get_available_states 获取可用状态服务的类型支持
 * @param[in] ts_srv_get_available_transitions 获取可用转换服务的类型支持
 * @param[in] ts_srv_get_transition_graph 获取转换图服务的类型支持
 * @return 返回rcl_ret_t类型的结果，表示操作是否成功
 */
rcl_ret_t RCL_WARN_UNUSED rcl_lifecycle_com_interface_init(
  rcl_lifecycle_com_interface_t * com_interface, rcl_node_t * node_handle,
  const rosidl_message_type_support_t * ts_pub_notify,
  const rosidl_service_type_support_t * ts_srv_change_state,
  const rosidl_service_type_support_t * ts_srv_get_state,
  const rosidl_service_type_support_t * ts_srv_get_available_states,
  const rosidl_service_type_support_t * ts_srv_get_available_transitions,
  const rosidl_service_type_support_t * ts_srv_get_transition_graph);

/**
 * @brief 初始化生命周期通信接口的发布器
 *
 * @param[in] com_interface 指向要初始化的生命周期通信接口结构体的指针
 * @param[in] node_handle 指向节点句柄的指针
 * @param[in] ts_pub_notify 通知发布器的类型支持
 * @return 返回rcl_ret_t类型的结果，表示操作是否成功
 */
rcl_ret_t RCL_WARN_UNUSED rcl_lifecycle_com_interface_publisher_init(
  rcl_lifecycle_com_interface_t * com_interface, rcl_node_t * node_handle,
  const rosidl_message_type_support_t * ts_pub_notify);

/**
 * @brief 清理生命周期通信接口的发布器
 *
 * @param[in] com_interface 指向要清理的生命周期通信接口结构体的指针
 * @param[in] node_handle 指向节点句柄的指针
 * @return 返回rcl_ret_t类型的结果，表示操作是否成功
 */
rcl_ret_t RCL_WARN_UNUSED rcl_lifecycle_com_interface_publisher_fini(
  rcl_lifecycle_com_interface_t * com_interface, rcl_node_t * node_handle);

/**
 * @brief 初始化生命周期通信接口的服务
 *
 * @param[in] com_interface 指向要初始化的生命周期通信接口结构体的指针
 * @param[in] node_handle 指向节点句柄的指针
 * @param[in] ts_srv_change_state 改变状态服务的类型支持
 * @param[in] ts_srv_get_state 获取状态服务的类型支持
 * @param[in] ts_srv_get_available_states 获取可用状态服务的类型支持
 * @param[in] ts_srv_get_available_transitions 获取可用转换服务的类型支持
 * @param[in] ts_srv_get_transition_graph 获取转换图服务的类型支持
 * @return 返回rcl_ret_t类型的结果，表示操作是否成功
 */
rcl_ret_t RCL_WARN_UNUSED rcl_lifecycle_com_interface_services_init(
  rcl_lifecycle_com_interface_t * com_interface, rcl_node_t * node_handle,
  const rosidl_service_type_support_t * ts_srv_change_state,
  const rosidl_service_type_support_t * ts_srv_get_state,
  const rosidl_service_type_support_t * ts_srv_get_available_states,
  const rosidl_service_type_support_t * ts_srv_get_available_transitions,
  const rosidl_service_type_support_t * ts_srv_get_transition_graph);

/**
 * @brief 清理生命周期通信接口的服务
 *
 * @param[in] com_interface 指向要清理的生命周期通信接口结构体的指针
 * @param[in] node_handle 指向节点句柄的指针
 * @return 返回rcl_ret_t类型的结果，表示操作是否成功
 */
rcl_ret_t RCL_WARN_UNUSED rcl_lifecycle_com_interface_services_fini(
  rcl_lifecycle_com_interface_t * com_interface, rcl_node_t * node_handle);

/**
 * @brief 清理生命周期通信接口
 *
 * @param[in] com_interface 指向要清理的生命周期通信接口结构体的指针
 * @param[in] node_handle 指向节点句柄的指针
 * @return 返回rcl_ret_t类型的结果，表示操作是否成功
 */
rcl_ret_t RCL_WARN_UNUSED rcl_lifecycle_com_interface_fini(
  rcl_lifecycle_com_interface_t * com_interface, rcl_node_t * node_handle);

/**
 * @brief 发布生命周期通知
 *
 * @param[in] com_interface 指向生命周期通信接口结构体的指针
 * @param[in] start 指向开始状态的指针
 * @param[in] goal 指向目标状态的指针
 * @return 返回rcl_ret_t类型的结果，表示操作是否成功
 */
rcl_ret_t RCL_WARN_UNUSED rcl_lifecycle_com_interface_publish_notification(
  rcl_lifecycle_com_interface_t * com_interface, const rcl_lifecycle_state_t * start,
  const rcl_lifecycle_state_t * goal);

#ifdef __cplusplus
}
#endif

#endif  // COM_INTERFACE_H_
