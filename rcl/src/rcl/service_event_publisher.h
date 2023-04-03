// Copyright 2022 Open Source Robotics Foundation, Inc.
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

#ifndef RCL__SERVICE_EVENT_PUBLISHER_H_
#define RCL__SERVICE_EVENT_PUBLISHER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/macros.h"
#include "rcl/node.h"
#include "rcl/publisher.h"
#include "rcl/service_introspection.h"
#include "rcl/time.h"
#include "rcl/types.h"
#include "rcl/visibility_control.h"
#include "rosidl_runtime_c/service_type_support_struct.h"

/**
 * @struct rcl_service_event_publisher_s
 * @brief 用于发布服务事件的结构体
 */
typedef struct rcl_service_event_publisher_s {
  /// 指向用于发布服务事件的publisher的句柄
  rcl_publisher_t* publisher;
  /// 服务内省主题的名称：<service_name>/<RCL_SERVICE_INTROSPECTION_TOPIC_POSTFIX>
  char* service_event_topic_name;
  /// 当前内省状态；关闭，元数据或内容
  rcl_service_introspection_state_t introspection_state;
  /// 用于给服务事件加时间戳的时钟句柄
  rcl_clock_t* clock;
  /// 服务事件发布者的发布选项
  rcl_publisher_options_t publisher_options;
  /// 指向服务类型支持的句柄
  const rosidl_service_type_support_t* service_type_support;
} rcl_service_event_publisher_t;

/**
 * @brief 返回一个成员设置为`NULL`的rcl_service_event_publisher_t结构体。
 *
 * 在传递给rcl_service_event_publisher_init()之前，应该调用此函数以获取一个空的rcl_service_event_publisher_t。
 *
 * @return rcl_service_event_publisher_t 结构体，其成员设置为`NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_service_event_publisher_t rcl_get_zero_initialized_service_event_publisher();

/// 初始化服务事件发布器。
/**
 * 在 rcl_service_event_publisher_t 上调用此函数后，可以通过调用 rcl_send_service_event_message()
 * 发送服务内省消息。
 *
 * 给定的 rcl_node_t 必须是有效的，且生成的 rcl_service_event_publisher_t 只有在给定的 rcl_node_t
 * 保持有效时才有效。
 *
 * 同样，给定的 rcl_clock_t 必须是有效的，且生成的 rcl_service_event_publisher_t 只有在给定的
 * rcl_clock_t 保持有效时才有效。
 *
 * 传入的 service_name
 * 应该是完全限定的、重新映射的服务名称。服务事件发布器将添加一个自定义后缀作为主题名称。
 *
 * rosidl_service_type_support_t 是基于每个 `.srv` 类型获得的。当用户定义一个 ROS
 * 服务时，会生成提供所需 rosidl_service_type_support_t 对象的代码。
 *
 * <hr>
 * 属性                | 遵循性
 * ------------------ | -------------
 * 分配内存            | 是
 * 线程安全            | 否
 * 使用原子操作        | 可能 [1]
 * 无锁                | 可能 [1]
 * <i>[1] rmw 实现定义</i>
 *
 * \param[inout] service_event_publisher 预分配的 rcl_service_event_publisher_t
 * \param[in] node 用于创建内省发布器的有效 rcl_node_t
 * \param[in] clock 用于生成内省时间戳的有效 rcl_clock_t
 * \param[in] publisher_options 创建内省发布器时使用的选项
 * \param[in] service_name 完全限定且重新映射的服务名称
 * \param[in] service_type_support 与此服务关联的类型支持库
 * \return #RCL_RET_OK 如果调用成功
 * \return #RCL_RET_INVALID_ARGUMENT 如果事件发布器、客户端或节点无效，
 * \return #RCL_RET_NODE_INVALID 如果给定节点无效，或
 * \return #RCL_RET_BAD_ALLOC 如果内存分配失败，或
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_service_event_publisher_init(
    rcl_service_event_publisher_t* service_event_publisher,
    const rcl_node_t* node,
    rcl_clock_t* clock,
    const rcl_publisher_options_t publisher_options,
    const char* service_name,
    const rosidl_service_type_support_t* service_type_support);

/// 结束一个 rcl_service_event_publisher_t.
/**
 * 调用此函数后，对这里的其他任何函数的调用
 * （除了 rcl_service_event_publisher_init()）都将失败。
 * 但是，给定的节点句柄仍然有效。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[inout] service_event_publisher 要完成的事件发布器句柄
 * \param[in] node 用于创建客户端的有效（未完成）节点句柄
 * \return #RCL_RET_OK 如果客户端成功完成，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_NODE_INVALID 如果节点无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_service_event_publisher_fini(
    rcl_service_event_publisher_t* service_event_publisher, rcl_node_t* node);

/// 检查服务事件发布器是否有效。
/**
 * 如果服务事件发布器无效，则返回的布尔值为 `false`。
 * 否则，返回的布尔值为 `true`。
 * 在返回 `false` 的情况下，会设置错误消息。
 * 此功能不能失败。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] service_event_publisher 指向服务事件发布器的指针
 * \return 如果 `service_event_publisher` 有效，则为 `true`，否则为 `false`
 */
RCL_PUBLIC
bool rcl_service_event_publisher_is_valid(
    const rcl_service_event_publisher_t* service_event_publisher);

/// 发送服务事件消息。
/**
 * 调用者有责任确保 `ros_request` 参数的类型与
 * 通过类型支持关联的事件发布器的类型匹配。
 * 将不同类型传递给发布会产生未定义的行为，并且不能
 * 由此功能检查，因此不会发生故意错误。
 *
 * rcl_send_service_event_message() 是一个可能阻塞的调用。
 *
 * 由 `ros_response_request` void 指针给出的 ROS 请求消息始终
 * 属于调用代码，但在 rcl_send_service_event_message() 期间应保持不变。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] service_event_publisher 指向服务事件发布器的指针
 * \param[in] event_type 来自 service_msgs::msg::ServiceEventInfo 的内省事件类型
 * \param[in] ros_response_request 类型擦除的指向 ROS 响应请求的指针
 * \param[in] sequence_number 事件的序列号
 * \param[in] guid 与此事件关联的 GUID
 * \return #RCL_RET_OK 如果事件成功发布，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_send_service_event_message(
    const rcl_service_event_publisher_t* service_event_publisher,
    uint8_t event_type,
    const void* ros_response_request,
    int64_t sequence_number,
    const uint8_t guid[16]);

/// 更改此服务事件发布器的操作状态。
/**
 * \param[in] service_event_publisher 指向服务事件发布器的指针
 * \param[in] introspection_state 新的内省状态
 * \return #RCL_RET_OK 如果事件成功发布，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
rcl_ret_t rcl_service_event_publisher_change_state(
    rcl_service_event_publisher_t* service_event_publisher,
    rcl_service_introspection_state_t introspection_state);

#ifdef __cplusplus
}
#endif
#endif  // RCL__SERVICE_EVENT_PUBLISHER_H_
