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

#include "com_interface.h"  // NOLINT

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "lifecycle_msgs/msg/transition_event.h"
#include "rcl/error_handling.h"
#include "rcl_lifecycle/data_types.h"
#include "rcutils/format_string.h"
#include "rcutils/logging_macros.h"
#include "rmw/validate_full_topic_name.h"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_runtime_c/string_functions.h"

// 定义发布器的主题名称
static const char* pub_transition_event_topic = "~/transition_event";
// 定义服务的名称
static const char* srv_change_state_service = "~/change_state";
static const char* srv_get_state_service = "~/get_state";
static const char* srv_get_available_states_service = "~/get_available_states";
static const char* srv_get_available_transitions_service = "~/get_available_transitions";
static const char* srv_get_transition_graph = "~/get_transition_graph";

/**
 * @brief 初始化并返回一个零值生命周期通信接口结构体
 * @return rcl_lifecycle_com_interface_t 返回一个初始化为零值的生命周期通信接口结构体
 */
rcl_lifecycle_com_interface_t rcl_lifecycle_get_zero_initialized_com_interface() {
  // 声明一个生命周期通信接口变量
  rcl_lifecycle_com_interface_t com_interface;
  // 将节点句柄设置为 NULL
  com_interface.node_handle = NULL;
  // 初始化各个通信接口组件
  com_interface.pub_transition_event = rcl_get_zero_initialized_publisher();
  com_interface.srv_change_state = rcl_get_zero_initialized_service();
  com_interface.srv_get_state = rcl_get_zero_initialized_service();
  com_interface.srv_get_available_states = rcl_get_zero_initialized_service();
  com_interface.srv_get_available_transitions = rcl_get_zero_initialized_service();
  com_interface.srv_get_transition_graph = rcl_get_zero_initialized_service();
  // 初始化消息结构体
  lifecycle_msgs__msg__TransitionEvent msg = {0};
  com_interface.msg = msg;
  // 返回初始化后的生命周期通信接口结构体
  return com_interface;
}

/**
 * @brief 初始化生命周期通信接口
 *
 * @param[in,out] com_interface 生命周期通信接口指针
 * @param[in] node_handle 节点句柄指针
 * @param[in] ts_pub_notify 发布器类型支持指针
 * @param[in] ts_srv_change_state 更改状态服务类型支持指针
 * @param[in] ts_srv_get_state 获取状态服务类型支持指针
 * @param[in] ts_srv_get_available_states 获取可用状态服务类型支持指针
 * @param[in] ts_srv_get_available_transitions 获取可用转换服务类型支持指针
 * @param[in] ts_srv_get_transition_graph 获取转换图服务类型支持指针
 * @return rcl_ret_t 返回初始化结果，成功返回 RCL_RET_OK
 */
rcl_ret_t rcl_lifecycle_com_interface_init(
    rcl_lifecycle_com_interface_t* com_interface,
    rcl_node_t* node_handle,
    const rosidl_message_type_support_t* ts_pub_notify,
    const rosidl_service_type_support_t* ts_srv_change_state,
    const rosidl_service_type_support_t* ts_srv_get_state,
    const rosidl_service_type_support_t* ts_srv_get_available_states,
    const rosidl_service_type_support_t* ts_srv_get_available_transitions,
    const rosidl_service_type_support_t* ts_srv_get_transition_graph) {
  // 初始化生命周期通信接口的发布器
  rcl_ret_t ret =
      rcl_lifecycle_com_interface_publisher_init(com_interface, node_handle, ts_pub_notify);
  // 如果发布器初始化失败，返回错误码
  if (ret != RCL_RET_OK) {
    return ret;
  }

  // 初始化生命周期通信接口的服务
  ret = rcl_lifecycle_com_interface_services_init(
      com_interface,                     //
      node_handle,                       //
      ts_srv_change_state,               //
      ts_srv_get_state,                  //
      ts_srv_get_available_states,       //
      ts_srv_get_available_transitions,  //
      ts_srv_get_transition_graph);

  // 如果服务初始化失败
  if (RCL_RET_OK != ret) {
    // 清理已经正确初始化的发布器
    rcl_ret_t ret_fini = rcl_lifecycle_com_interface_publisher_fini(com_interface, node_handle);
    // 警告已经设置，此处无需记录日志
    (void)ret_fini;
  }

  // 返回初始化结果
  return ret;
}

/**
 * @brief 初始化生命周期通信接口的发布者
 *
 * @param com_interface 生命周期通信接口指针
 * @param node_handle 节点句柄指针
 * @param ts_pub_notify 发布通知消息类型支持的指针
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_lifecycle_com_interface_publisher_init(
    rcl_lifecycle_com_interface_t* com_interface,
    rcl_node_t* node_handle,
    const rosidl_message_type_support_t* ts_pub_notify) {
  // 检查参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(com_interface, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(node_handle, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(ts_pub_notify, RCL_RET_INVALID_ARGUMENT);

  // 初始化发布者
  rcl_publisher_options_t publisher_options = rcl_publisher_get_default_options();
  rcl_ret_t ret = rcl_publisher_init(
      &com_interface->pub_transition_event,  //
      node_handle,                           //
      ts_pub_notify,                         //
      pub_transition_event_topic,            //
      &publisher_options);

  // 如果初始化失败，跳转到错误处理
  if (ret != RCL_RET_OK) {
    goto fail;
  }

  // 初始化静态通知消息
  lifecycle_msgs__msg__TransitionEvent__init(&com_interface->msg);

  return RCL_RET_OK;

fail:
  // 错误信息已经记录在失败时
  ret = rcl_lifecycle_com_interface_publisher_fini(com_interface, node_handle);
  (void)ret;
  return RCL_RET_ERROR;
}

/**
 * @brief 清理生命周期通信接口的发布者
 *
 * @param com_interface 生命周期通信接口指针
 * @param node_handle 节点句柄指针
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_lifecycle_com_interface_publisher_fini(
    rcl_lifecycle_com_interface_t* com_interface, rcl_node_t* node_handle) {
  // 清理静态通知消息
  lifecycle_msgs__msg__TransitionEvent__fini(&com_interface->msg);

  // 销毁发布者
  rcl_ret_t ret = rcl_publisher_fini(&com_interface->pub_transition_event, node_handle);
  if (ret != RCL_RET_OK) {
    RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Failed to destroy transition_event publisher");
  }

  return ret;
}

/**
 * @brief 初始化生命周期通信接口服务
 *
 * @param com_interface 生命周期通信接口指针
 * @param node_handle 节点句柄指针
 * @param ts_srv_change_state 改变状态服务类型支持指针
 * @param ts_srv_get_state 获取状态服务类型支持指针
 * @param ts_srv_get_available_states 获取可用状态服务类型支持指针
 * @param ts_srv_get_available_transitions 获取可用转换服务类型支持指针
 * @param ts_srv_get_transition_graph 获取转换图服务类型支持指针
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_lifecycle_com_interface_services_init(
    rcl_lifecycle_com_interface_t* com_interface,
    rcl_node_t* node_handle,
    const rosidl_service_type_support_t* ts_srv_change_state,
    const rosidl_service_type_support_t* ts_srv_get_state,
    const rosidl_service_type_support_t* ts_srv_get_available_states,
    const rosidl_service_type_support_t* ts_srv_get_available_transitions,
    const rosidl_service_type_support_t* ts_srv_get_transition_graph) {
  // 检查参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(com_interface, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(node_handle, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(ts_srv_change_state, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(ts_srv_get_state, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(ts_srv_get_available_states, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(ts_srv_get_available_transitions, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(ts_srv_get_transition_graph, RCL_RET_INVALID_ARGUMENT);

  rcl_ret_t ret = RCL_RET_OK;

  {  // 初始化改变状态服务
    rcl_service_options_t service_options = rcl_service_get_default_options();
    ret = rcl_service_init(
        &com_interface->srv_change_state,  //
        node_handle,                       //
        ts_srv_change_state,               //
        srv_change_state_service,          //
        &service_options);
    if (ret != RCL_RET_OK) {
      goto fail;
    }
  }

  {  // 初始化获取状态服务
    rcl_service_options_t service_options = rcl_service_get_default_options();
    ret = rcl_service_init(
        &com_interface->srv_get_state,  //
        node_handle,                    //
        ts_srv_get_state,               //
        srv_get_state_service,          //
        &service_options);
    if (ret != RCL_RET_OK) {
      goto fail;
    }
  }

  {  // 初始化获取可用状态服务
    rcl_service_options_t service_options = rcl_service_get_default_options();
    ret = rcl_service_init(
        &com_interface->srv_get_available_states,  //
        node_handle,                               //
        ts_srv_get_available_states,               //
        srv_get_available_states_service,          //
        &service_options);
    if (ret != RCL_RET_OK) {
      goto fail;
    }
  }

  {  // 初始化获取可用转换服务
    rcl_service_options_t service_options = rcl_service_get_default_options();
    ret = rcl_service_init(
        &com_interface->srv_get_available_transitions,  //
        node_handle,                                    //
        ts_srv_get_available_transitions,               //
        srv_get_available_transitions_service,          //
        &service_options);
    if (ret != RCL_RET_OK) {
      goto fail;
    }
  }

  {  // 初始化获取转换图服务
    rcl_service_options_t service_options = rcl_service_get_default_options();
    ret = rcl_service_init(
        &com_interface->srv_get_transition_graph,  //
        node_handle,                               //
        ts_srv_get_transition_graph,               //
        srv_get_transition_graph,                  //
        &service_options);
    if (ret != RCL_RET_OK) {
      goto fail;
    }
  }
  return RCL_RET_OK;

fail:
  // 错误消息已在失败时记录
  ret = rcl_lifecycle_com_interface_services_fini(com_interface, node_handle);
  (void)ret;
  return RCL_RET_ERROR;
}

/**
 * @brief 终止生命周期通信接口服务
 * 该函数用于终止生命周期通信接口的所有服务。
 *
 * @param com_interface 生命周期通信接口指针
 * @param node_handle 节点句柄指针
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_lifecycle_com_interface_services_fini(
    rcl_lifecycle_com_interface_t* com_interface,  //
    rcl_node_t* node_handle) {
  // 初始化返回值为成功
  rcl_ret_t fcn_ret = RCL_RET_OK;

  {  // 销毁 get_transition_graph 服务
    rcl_ret_t ret = rcl_service_fini(&com_interface->srv_get_transition_graph, node_handle);
    if (ret != RCL_RET_OK) {
      // 错误日志：销毁 get_transition_graph 服务失败
      RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Failed to destroy get_transition_graph service");
      fcn_ret = RCL_RET_ERROR;
    }
  }

  {  // 销毁 get_available_transitions 服务
    rcl_ret_t ret = rcl_service_fini(&com_interface->srv_get_available_transitions, node_handle);
    if (ret != RCL_RET_OK) {
      RCUTILS_LOG_ERROR_NAMED(
          ROS_PACKAGE_NAME, "Failed to destroy get_available_transitions service");
      fcn_ret = RCL_RET_ERROR;
    }
  }

  {  // 销毁 get_available_states 服务
    rcl_ret_t ret = rcl_service_fini(&com_interface->srv_get_available_states, node_handle);
    if (ret != RCL_RET_OK) {
      RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Failed to destroy get_available_states service");
      fcn_ret = RCL_RET_ERROR;
    }
  }

  {  // 销毁 get_state 服务
    rcl_ret_t ret = rcl_service_fini(&com_interface->srv_get_state, node_handle);
    if (ret != RCL_RET_OK) {
      RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Failed to destroy get_state service");
      fcn_ret = RCL_RET_ERROR;
    }
  }

  {  // 销毁 change_state 服务
    rcl_ret_t ret = rcl_service_fini(&com_interface->srv_change_state, node_handle);
    if (ret != RCL_RET_OK) {
      RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Failed to destroy change_state service");
      fcn_ret = RCL_RET_ERROR;
    }
  }

  // 返回操作结果
  return fcn_ret;
}

/**
 * @brief 终止生命周期通信接口
 *
 * @param[in] com_interface 生命周期通信接口指针
 * @param[in] node_handle 节点句柄指针
 * @return 返回操作结果，成功返回RCL_RET_OK，失败返回RCL_RET_ERROR
 */
rcl_ret_t rcl_lifecycle_com_interface_fini(
    rcl_lifecycle_com_interface_t* com_interface, rcl_node_t* node_handle) {
  rcl_ret_t fcn_ret = RCL_RET_OK;

  {  // 销毁服务
    rcl_ret_t ret = rcl_lifecycle_com_interface_services_fini(com_interface, node_handle);
    if (RCL_RET_OK != ret) {
      fcn_ret = RCL_RET_ERROR;
    }
  }

  {  // 销毁事件发布器
    rcl_ret_t ret = rcl_lifecycle_com_interface_publisher_fini(com_interface, node_handle);
    if (RCL_RET_OK != ret) {
      fcn_ret = RCL_RET_ERROR;
    }
  }

  return fcn_ret;
}

/**
 * @brief 发布生命周期通知
 *
 * @param[in] com_interface 生命周期通信接口指针
 * @param[in] start 起始状态指针
 * @param[in] goal 目标状态指针
 * @return 返回操作结果，成功返回RCL_RET_OK，失败返回其他错误代码
 */
rcl_ret_t rcl_lifecycle_com_interface_publish_notification(
    rcl_lifecycle_com_interface_t* com_interface,
    const rcl_lifecycle_state_t* start,
    const rcl_lifecycle_state_t* goal) {
  // 设置消息的起始状态
  com_interface->msg.start_state.id = start->id;
  rosidl_runtime_c__String__assign(&com_interface->msg.start_state.label, start->label);

  // 设置消息的目标状态
  com_interface->msg.goal_state.id = goal->id;
  rosidl_runtime_c__String__assign(&com_interface->msg.goal_state.label, goal->label);

  // 发布转换事件消息
  return rcl_publish(&com_interface->pub_transition_event, &com_interface->msg, NULL);
}

#ifdef __cplusplus
}
#endif
