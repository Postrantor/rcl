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

#ifndef RCL_ACTION__ACTION_SERVER_H_
#define RCL_ACTION__ACTION_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/event_callback.h"
#include "rcl/macros.h"
#include "rcl/node.h"
#include "rcl/time.h"
#include "rcl_action/goal_handle.h"
#include "rcl_action/types.h"
#include "rcl_action/visibility_control.h"
#include "rosidl_runtime_c/action_type_support_struct.h"

/// \brief 内部 rcl_action 实现结构体。
typedef struct rcl_action_server_impl_s rcl_action_server_impl_t;

/// \brief 封装 ROS Action 服务器的结构体。
typedef struct rcl_action_server_s
{
  /// \brief 指向 action server 实现的指针
  rcl_action_server_impl_t * impl;
} rcl_action_server_t;

/// \brief rcl_action_server_t 可用的选项。
typedef struct rcl_action_server_options_s
{
  /// \brief action server 的中间件服务质量设置。
  /// \brief 目标服务的服务质量
  rmw_qos_profile_t goal_service_qos;
  /// \brief 取消服务的服务质量
  rmw_qos_profile_t cancel_service_qos;
  /// \brief 结果服务的服务质量
  rmw_qos_profile_t result_service_qos;
  /// \brief 反馈主题的服务质量
  rmw_qos_profile_t feedback_topic_qos;
  /// \brief 状态主题的服务质量
  rmw_qos_profile_t status_topic_qos;
  /// \brief action server 的自定义分配器，用于偶发性分配。
  /** 默认行为（malloc/free），请参阅：rcl_get_default_allocator() */
  rcl_allocator_t allocator;
  /// \brief 结果超过此时间的目标句柄将被释放。
  rcl_duration_t result_timeout;
} rcl_action_server_options_t;

/// \brief 返回一个成员设置为 `NULL` 的 rcl_action_server_t 结构体。
/**
 * 在传递给 rcl_action_server_init() 之前，应该调用此函数以获取空的 rcl_action_server_t。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_action_server_t rcl_action_get_zero_initialized_server(void);

/// \brief 初始化 action server。
/**
 * 在 rcl_action_server_t 上调用此函数后，可以使用 rcl_action_take_goal_request() 获取给定类型的目标，
 * 并使用 rcl_action_take_cancel_request() 获取取消请求。
 * 它还可以使用 rcl_action_send_result() 或 rcl_action_send_cancel_response() 为请求发送结果。
 *
 * 在使用 rcl_action_take_goal_request() 接受目标后，action server 可以使用 rcl_action_publish_feedback() 发送反馈，
 * 并使用 rcl_action_publish_status() 发送状态消息。
 *
 * 给定的 rcl_node_t 必须是有效的，生成的 rcl_action_server_t 只有在给定的 rcl_node_t 保持有效时才有效。
 *
 * 给定的 rcl_clock_t 必须是有效的，生成的 rcl_ction_server_t 只有在给定的 rcl_clock_t 保持有效时才有效。
 *
 * rosidl_action_type_support_t 是根据每个 .action 类型获得的。
 * 当用户定义 ROS action 时，会生成提供所需 rosidl_action_type_support_t 对象的代码。
 * 此对象可以使用适合语言的机制获得。
 * \todo TODO(jacobperron) 编写这些说明一次并链接到它
 *
 * 对于 C，可以使用宏（例如 `example_interfaces/Fibonacci`）：
 *
 * ```c
 * #include <rosidl_runtime_c/action_type_support_struct.h>
 * #include <example_interfaces/action/fibonacci.h>
 * const rosidl_action_type_support_t * ts =
 *   ROSIDL_GET_ACTION_TYPE_SUPPORT(example_interfaces, Fibonacci);
 * ```
 *
 * 对于 C++，使用模板函数：
 *
 * ```cpp
 * #include <rosidl_runtime_cpp/action_type_support.hpp>
 * #include <example_interfaces/action/fibonacci.h>
 * using rosidl_typesupport_cpp::get_action_type_support_handle;
 * const rosidl_action_type_support_t * ts =
 *   get_action_type_support_handle<example_interfaces::action::Fibonacci>();
 * ```
 *
 * rosidl_action_type_support_t 对象包含用于发送或获取目标、结果和反馈的 action 类型特定信息。
 *
 * 主题名称必须是遵循未展开名称（也称为非完全限定名称）的主题和服务名称格式规则的 c 字符串：
 *
 * \see rcl_expand_topic_name
 *
 * options 结构允许用户设置服务质量设置以及在初始化/终止客户端时用于分配空间的自定义分配器，
 * 例如 action server 名称字符串。
 *
 * 预期用法（对于 C action servers）：
 *
 * ```c
 * #include <rcl/rcl.h>
 * #include <rcl_action/rcl_action.h>
 * #include <rosidl_runtime_c/action_type_support_struct.h>
 * #include <example_interfaces/action/fibonacci.h>
 *
 * rcl_node_t node = rcl_get_zero_initialized_node();
 * rcl_node_options_t node_ops = rcl_node_get_default_options();
 * rcl_ret_t ret = rcl_node_init(&node, "node_name", "/my_namespace", &node_ops);
 * // ... 错误处理
 * const rosidl_action_type_support_t * ts =
 *   ROSIDL_GET_ACTION_TYPE_SUPPORT(example_interfaces, Fibonacci);
 * rcl_action_server_t action_server = rcl_action_get_zero_initialized_server();
 * rcl_action_server_options_t action_server_ops = rcl_action_server_get_default_options();
 * ret = rcl_action_server_init(&action_server, &node, ts, "fibonacci", &action_server_ops);
 * // ... 错误处理，以及在关闭时执行终止操作：
 * ret = rcl_action_server_fini(&action_server, &node);
 * // ... rcl_action_server_fini() 的错误处理
 * ret = rcl_node_fini(&node);
 * // ... rcl_node_fini() 的错误处理
 * ```
 *
 * <hr>
 * 属性          | 遵循
 * ------------------ | -------------
 * 分配内存   | 是
 * 线程安全        | 否
 * 使用原子       | 否
 * 无锁          | 是
 *
 * \param[out] action_server 要初始化的预分配、零初始化 action server 结构体的句柄。
 * \param[in] node 有效的节点句柄
 * \param[in] clock 有效的时钟句柄
 * \param[in] type_support action 类型的类型支持对象
 * \param[in] action_name action 的名称
 * \param[in] options 包括服务质量设置的 action_server 选项
 * \return `RCL_RET_OK` 如果 action_server 成功初始化，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_NODE_INVALID` 如果节点无效，或
 * \return `RCL_RET_BAD_ALLOC` 如果分配内存失败，或
 * \return `RCL_RET_ACTION_NAME_INVALID` 如果给定的 action 名称无效，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_server_init(
  rcl_action_server_t * action_server, rcl_node_t * node, rcl_clock_t * clock,
  const rosidl_action_type_support_t * type_support, const char * action_name,
  const rcl_action_server_options_t * options);

/// 结束一个动作服务器。
/**
 * 调用后，节点将不再为此动作服务器监听服务和主题。
 * （假设这是此节点中此类型的唯一动作服务器）。
 *
 * 调用后，使用此动作服务器时，对 rcl_wait()、rcl_action_take_goal_request()、
 * rcl_action_take_cancel_request()、rcl_action_publish_feedback()、
 * rcl_action_publish_status()、rcl_action_send_result() 和
 * rcl_action_send_cancel_response() 的调用将失败。
 * 另外，如果当前正在阻塞，rcl_wait() 将被中断。
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
 * \param[inout] action_server 要取消初始化的动作服务器句柄
 * \param[in] node 用于创建动作服务器的节点句柄
 * \return `RCL_RET_OK` 如果动作服务器成功取消初始化，或者
 * \return `RCL_RET_ACTION_SERVER_INVALID` 如果动作服务器指针为空，或者
 * \return `RCL_RET_NODE_INVALID` 如果节点无效，或者
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_server_fini(rcl_action_server_t * action_server, rcl_node_t * node);

/// 以 rcl_action_server_options_t 的形式返回默认动作服务器选项。
/**
 * 默认值为：
 *
 * - goal_service_qos = rmw_qos_profile_services_default;
 * - cancel_service_qos = rmw_qos_profile_services_default;
 * - result_service_qos = rmw_qos_profile_services_default;
 * - feedback_topic_qos = rmw_qos_profile_default;
 * - status_topic_qos = rcl_action_qos_profile_status_default;
 * - allocator = rcl_get_default_allocator();
 * - result_timeout = RCUTILS_S_TO_NS(10);  // 10 秒
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_action_server_options_t rcl_action_server_get_default_options(void);

/// 使用动作服务器获取一个待处理的 ROS 目标。
/**
 * \todo TODO(jacobperron) 获取的阻塞？
 *
 * \todo TODO(jacobperron) 消息所有权的前置条件、过程中和后置条件？
 *
 * \todo TODO(jacobperron) 这是线程安全的吗？
 *
 * 调用者有责任确保 `ros_goal_request` 的类型与客户端关联的类型（通过类型支持）匹配。
 * 传递不同类型会产生未定义的行为，这不能由此函数检查，因此不会发生故意错误。
 *
 * `ros_goal_request` 应指向一个预分配的、零初始化的 ROS 目标消息。
 * 如果成功获取目标请求，它将被复制到 `ros_goal_request` 中。
 *
 * 如果在获取请求时需要分配内存，例如，如果需要为目标消息中的动态大小数组分配空间，
 * 那么将使用动作服务器选项中给定的分配器。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 可能 [1]
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 * <i>[1] 仅在填充请求时需要，对于固定大小避免使用</i>
 *
 * \param[in] action_server 将获取请求的动作服务器句柄
 * \param[out] request_header 指向目标请求头的指针
 * \param[out] ros_goal_request 预分配的、零初始化的 ROS 目标请求消息
 *   其中复制了请求
 * \return `RCL_RET_OK` 如果请求被获取，或者
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或者
 * \return `RCL_RET_ACTION_SERVER_INVALID` 如果动作服务器无效，或者
 * \return `RCL_RET_BAD_ALLOC` 如果分配内存失败，或者
 * \return `RCL_RET_ACTION_SERVER_TAKE_FAILED` 如果获取失败但中间件中没有错误发生，或者
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_take_goal_request(
  const rcl_action_server_t * action_server, rmw_request_id_t * request_header,
  void * ros_goal_request);

/// 使用动作服务器向动作客户端发送目标请求的响应。
/**
 * 这是一个非阻塞调用。
 *
 * 调用者有责任确保 `ros_goal_response` 的类型与客户端关联的类型（通过类型支持）匹配。
 * 传递不同类型会产生未定义的行为，这不能由此函数检查，因此不会发生故意错误。
 *
 * 如果调用者打算发送“已接受”的响应，在调用此函数之前，
 * 调用者应使用 rcl_action_accept_new_goal() 获取一个 rcl_action_goal_handle_t，
 * 以便将来与目标进行交互（例如发布反馈和取消目标）。
 *
 * 只要对动作服务器和 `ros_goal_response` 的访问得到同步，此函数就是线程安全的。
 * 这意味着允许从多个线程调用 rcl_action_send_goal_response()，
 * 但在与非线程安全的动作服务器函数同时调用 rcl_action_send_goal_response() 是不允许的，
 * 例如，同时调用 rcl_action_send_goal_response() 和 rcl_action_server_fini() 是不允许的。
 * 在调用 rcl_action_send_goal_response() 之前，`ros_goal_request` 可以更改，
 * 在调用 rcl_action_send_goal_response() 之后，`ros_goal_request` 可以更改，
 * 但在 rcl_action_send_goal_response() 调用期间不能更改。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是 [1]
 * 使用原子操作      | 否
 * 无锁              | 是
 * <i>[1] 对于唯一的动作服务器和响应对，有关更多信息，请参见上文</i>
 *
 * \param[in] action_server 将发出目标响应的动作服务器句柄
 * \param[in] response_header 指向目标响应头的指针
 * \param[in] ros_goal_response 要发送的 ROS 目标响应消息
 * \return `RCL_RET_OK` 如果响应成功发送，或者
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或者
 * \return `RCL_RET_ACTION_SERVER_INVALID` 如果动作服务器无效，或者
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_send_goal_response(
  const rcl_action_server_t * action_server, rmw_request_id_t * response_header,
  void * ros_goal_response);

/// 使用动作服务器接受一个新目标。
/**
 * 这是一个非阻塞调用。
 *
 * 创建并返回一个新的目标句柄。
 * 动作服务器开始在内部跟踪它。
 * 如果发生故障，返回`NULL`并设置错误消息。
 * 故障的可能原因：
 *   - 动作服务器无效
 *   - 目标信息无效
 *   - 目标ID已经被动作服务器跟踪
 *   - 内存分配失败
 *
 * 在使用rcl_action_take_goal_request()接收到新的目标请求后，
 * 并在使用rcl_action_send_goal_response()发送响应之前，应调用此函数。
 *
 * 调用此函数后，动作服务器将开始跟踪目标。
 * 在调用`rcl_action_server_fini()`后，指向目标句柄的指针变为无效。
 * 调用者以后需要负责完成目标句柄。
 *
 * 示例用法：
 *
 * ```c
 * #include <rcl/rcl_action.h>
 *
 * // ... 初始化一个动作服务器
 * // 获取一个目标请求（客户端库类型）
 * rcl_ret_t ret = rcl_action_take_goal_request(&action_server, &goal_request);
 * // ... 错误处理
 * // 如果目标被接受，则告知动作服务器
 * // 首先，创建一个目标信息消息
 * rcl_action_goal_info_t goal_info = rcl_action_get_zero_initialized_goal_info();
 * // ... 填充 goal_info.uuid (unique_identifier_msgs/UUID)
 * // ... 填充 goal_info.stamp (builtin_interfaces/Time)
 * rcl_action_goal_handle_t * goal_handle = rcl_action_accept_new_goal(&action_server, &goal_info);
 * // ... 错误处理
 * // ... 填充目标响应（客户端库类型）
 * ret = rcl_action_send_goal_response(&action_server, &goal_response);
 * // ... 错误处理，在关闭之前的某个时候完成目标信息消息
 * ret = rcl_action_goal_info_fini(&goal_info, &action_server);
 * ```
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 是
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * \param[in] action_server 接受目标的动作服务器句柄
 * \param[in] goal_info 包含有关被接受目标的信息的消息
 * \return 一个指向新目标句柄的指针，表示已接受的目标，或
 * \return 如果发生故障，则返回`NULL`。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_action_goal_handle_t * rcl_action_accept_new_goal(
  rcl_action_server_t * action_server, const rcl_action_goal_info_t * goal_info);

/// 使用动作服务器为活动目标发布ROS反馈消息。
/**
 * 调用者负责确保`ros_feedback`的类型与
 * 通过类型支持关联的客户端类型匹配。
 * 传递不同类型会产生未定义的行为，这不能由此函数检查，
 * 因此不会发生故意错误。
 *
 * 此函数类似于ROS发布器，可能是阻塞调用。
 * \see rcl_publish()
 *
 * 只要对动作服务器和`ros_feedback`的访问得到同步，
 * 此函数就是线程安全的。
 * 这意味着允许从多个线程调用rcl_action_publish_feedback()，
 * 但在与非线程安全的动作服务器函数同时调用rcl_action_publish_feedback()时不允许，
 * 例如，在同时调用rcl_action_publish_feedback()和rcl_action_server_fini()时不允许。
 *
 * 在调用rcl_action_publish_feedback()之前，`ros_feedback`消息可以更改，
 * 在调用rcl_action_publish_feedback()之后，`ros_feedback`消息可以更改，
 * 但在发布调用期间不能更改。
 * 相同的`ros_feedback`可以同时传递给多个rcl_action_publish_feedback()调用，
 * 即使动作服务器不同。`ros_feedback`不会被rcl_action_publish_feedback()修改。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 否
 * 线程安全           | 是 [1]
 * 使用原子操作       | 否
 * 无锁               | 是
 * <i>[1] 对于动作服务器和反馈的唯一对，参见上文</i>
 *
 * \param[in] action_server 将发布反馈的动作服务器句柄
 * \param[in] ros_feedback 包含目标反馈的ROS消息
 * \return 如果成功发送响应，则返回`RCL_RET_OK`，或
 * \return 如果任何参数无效，则返回`RCL_RET_INVALID_ARGUMENT`，或
 * \return 如果动作服务器无效，则返回`RCL_RET_ACTION_SERVER_INVALID`，或
 * \return 如果发生未指定错误，则返回`RCL_RET_ERROR`。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_publish_feedback(
  const rcl_action_server_t * action_server, void * ros_feedback);

/// 获取与动作服务器关联的已接受目标的状态数组消息。
/**
 * 在调用此函数之前，应使用
 * rcl_action_get_zero_initialized_goal_status_array()将提供的`status_message`归零初始化。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 否
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * \param[in] action_server 将发布状态消息的动作服务器句柄
 * \param[out] status_message 一个action_msgs/StatusArray ROS消息
 * \return 如果成功发送响应，则返回`RCL_RET_OK`，或
 * \return 如果任何参数无效，则返回`RCL_RET_INVALID_ARGUMENT`，或
 * \return 如果动作服务器无效，则返回`RCL_RET_ACTION_SERVER_INVALID`，或
 * \return 如果发生未指定错误，则返回`RCL_RET_ERROR`。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_get_goal_status_array(
  const rcl_action_server_t * action_server, rcl_action_goal_status_array_t * status_message);

/// 发布与动作服务器关联的已接受目标的状态数组消息。
/**
 * 此函数类似于ROS发布器，可能是阻塞调用。
 * \see rcl_publish()
 *
 * 可以使用rcl_action_get_goal_status_array()创建与动作服务器关联的状态数组消息。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 否
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * \param[in] action_server 将发布状态消息的动作服务器句柄
 * \param[in] status_message 要发布的action_msgs/StatusArray ROS消息
 * \return 如果成功发送响应，则返回`RCL_RET_OK`，或
 * \return 如果任何参数无效，则返回`RCL_RET_INVALID_ARGUMENT`，或
 * \return 如果动作服务器无效，则返回`RCL_RET_ACTION_SERVER_INVALID`，或
 * \return 如果发生未指定错误，则返回`RCL_RET_ERROR`。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_publish_status(
  const rcl_action_server_t * action_server, const void * status_message);

/// 使用动作服务器获取一个待处理的结果请求。
/**
 * \todo TODO(jacobperron) 阻塞 take？
 *
 * \todo TODO(jacobperron) 消息所有权的前置条件、过程中条件和后置条件？
 *
 * \todo TODO(jacobperron) 这个线程安全吗？
 *
 * 调用者负责确保 `ros_result_request` 的类型与
 * 通过类型支持与客户端关联的类型匹配。
 * 传递不同的类型会产生未定义的行为，这个函数无法检查，
 * 因此不会出现故意的错误。
 *
 * <hr>
 * 属性              | 符合情况
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] action_server 将获取结果请求的动作服务器句柄
 * \param[out] request_header 指向结果请求头的指针
 * \param[out] ros_result_request 预先分配的 ROS 结果请求消息，其中将复制请求。
 * \return `RCL_RET_OK` 如果成功发送了响应，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_ACTION_SERVER_INVALID` 如果动作服务器无效，或
 * \return `RCL_RET_ACTION_SERVER_TAKE_FAILED` 如果获取失败但中间件中没有发生错误，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_take_result_request(
  const rcl_action_server_t * action_server, rmw_request_id_t * request_header,
  void * ros_result_request);

/// 使用动作服务器发送结果响应。
/**
 * 这是一个非阻塞调用。
 *
 * 调用者负责确保 `ros_result_response` 的类型与
 * 通过类型支持与客户端关联的类型匹配。
 * 传递不同的类型会产生未定义的行为，这个函数无法检查，
 * 因此不会出现故意的错误。
 *
 * 在调用此函数之前，调用者应使用 rcl_action_update_goal_state()
 * 将目标状态更新为适当的终止状态。
 *
 * <hr>
 * 属性              | 符合情况
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] action_server 将发送结果响应的动作服务器句柄
 * \param[in] response_header 指向结果响应头的指针
 * \param[in] ros_result_response 要发送的 ROS 结果响应消息
 * \return `RCL_RET_OK` 如果成功发送了响应，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_ACTION_SERVER_INVALID` 如果动作服务器无效，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_send_result_response(
  const rcl_action_server_t * action_server, rmw_request_id_t * response_header,
  void * ros_result_response);

/// 使与动作服务器关联的目标过期。
/**
 * 如果某个目标处于终止状态（具有结果）的时间超过了某个持续时间，
 * 则该目标“过期”。超时持续时间是作为动作服务器选项设置的。
 *
 * 如果提供了负超时值，则目标结果永不过期（永久保留）。
 * 如果设置了零超时，则目标结果将立即被丢弃（即每当调用此函数时，
 * 目标结果都会被丢弃）。
 *
 * 过期的目标将从内部目标句柄数组中删除。
 * 对于已过期的目标，rcl_action_server_goal_exists() 将返回 false。
 *
 * \attention 如果一个或多个目标过期，那么之前从 rcl_action_server_get_goal_handles()
 * 返回的目标句柄数组将变得无效。
 *
 * `expired_goals`、`expired_goals_capacity` 和 `num_expired` 是可选参数。
 * 如果设置为（`NULL`，0u，`NULL`），则不使用它们。
 * 若要使用它们，请分配一个大小等于要过期的最大目标数量的数组。
 * 将数组可以容纳的目标数量作为 `expired_goals_capacity` 传入。
 * 此函数将把 `num_expired` 设置为过期的目标数量。
 *
 * <hr>
 * 属性              | 符合情况
 * ------------------ | -------------
 * 分配内存          | 可能[1]
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 * <i>[1] 如果一个或多个目标过期，那么内部目标句柄数组可能会被
 * 调整大小或释放</i>
 *
 * \param[in] action_server 将从中清除过期目标的动作服务器句柄。
 * \param[inout] expired_goals 过期目标的标识符，或设置为 `NULL` 如果未使用
 * \param[inout] expired_goals_capacity 已分配的 `expired_goals` 大小，或为 0 如果未使用
 * \param[out] num_expired 过期目标的数量，或设置为 `NULL` 如果未使用
 * \return `RCL_RET_OK` 如果成功发送了响应，或
 * \return `RCL_RET_ACTION_SERVER_INVALID` 如果动作服务器无效，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_BAD_ALLOC` 如果分配内存失败，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_expire_goals(
  const rcl_action_server_t * action_server, rcl_action_goal_info_t * expired_goals,
  size_t expired_goals_capacity, size_t * num_expired);

/// 通知动作服务器，一个目标句柄已达到终止状态。
/**
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] action_server 动作服务器的句柄
 * \return `RCL_RET_OK` 如果一切正常，或者
 * \return `RCL_RET_ACTION_SERVER_INVALID` 如果动作服务器无效，或者
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或者
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_notify_goal_done(const rcl_action_server_t * action_server);

/// 使用动作服务器获取待处理的取消请求。
/**
 * \todo TODO(jacobperron) 获取的阻塞？
 *
 * \todo TODO(jacobperron) 消息所有权的前置条件、过程中和后置条件？
 *
 * \todo TODO(jacobperron) 这是线程安全的吗？
 *
 * 调用者有责任确保 `ros_cancel_request` 的类型与
 * 客户端关联的类型（通过类型支持）匹配。
 * 传递不同的类型会产生未定义的行为，这个函数无法检查，
 * 因此不会出现故意的错误。
 *
 * 在收到成功的取消请求后，可以使用 rcl_action_process_cancel_request() 将适当的目标
 * 转换为 CANCELING 状态。
 *
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] action_server 将获取取消请求的动作服务器句柄
 * \param[out] request_header 指向取消请求头的指针
 * \param[out] ros_cancel_request 预先分配的 ROS 取消请求，请求消息将被复制到此处
 * \return `RCL_RET_OK` 如果响应发送成功，或者
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或者
 * \return `RCL_RET_ACTION_SERVER_INVALID` 如果动作服务器无效，或者
 * \return `RCL_RET_ACTION_SERVER_TAKE_FAILED` 如果获取失败，但中间件中没有发生错误，或者
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_take_cancel_request(
  const rcl_action_server_t * action_server, rmw_request_id_t * request_header,
  void * ros_cancel_request);

/// 使用动作服务器处理取消请求。
/**
 * 这是一个非阻塞调用。
 *
 * 此函数将计算取消请求试图取消的目标列表。
 * 它不会改变任何目标的状态。
 * 根据取消请求中的目标 ID 和时间戳，应用以下取消策略：
 *
 * - 如果目标 ID 为零且时间戳为零，则取消所有目标。
 * - 如果目标 ID 为零且时间戳不为零，则取消在该时间戳之前或者接受的所有目标。
 * - 如果目标 ID 不为零且时间戳为零，则无论何时接受都取消具有给定 ID 的目标。
 * - 如果目标 ID 不为零且时间戳不为零，则取消具有给定 ID 的目标以及在该时间戳之前或者接受的所有目标。
 *
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 是
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] action_server 将处理取消请求的动作服务器句柄
 * \param[in] cancel_request 要处理的 C 类型 ROS 取消请求
 * \param[out] cancel_response 初始化为零的取消响应结构，其中应取消的目标信息将被复制
 * \return `RCL_RET_OK` 如果响应发送成功，或者
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或者
 * \return `RCL_RET_ACTION_SERVER_INVALID` 如果动作服务器无效，或者
 * \return `RCL_RET_BAD_ALLOC` 如果分配内存失败，或者
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_process_cancel_request(
  const rcl_action_server_t * action_server, const rcl_action_cancel_request_t * cancel_request,
  rcl_action_cancel_response_t * cancel_response);

/// 使用动作服务器发送取消响应。
/**
 * 这是一个非阻塞调用
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param[in] action_server 将发送取消响应的动作服务器句柄
 * \param[in] response_header 指向取消响应头的指针
 * \param[in] ros_cancel_response 要发送的ROS取消响应
 * \return `RCL_RET_OK` 如果请求被接受，或者
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或者
 * \return `RCL_RET_ACTION_SERVER_INVALID` 如果动作服务器无效，或者
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_send_cancel_response(
  const rcl_action_server_t * action_server, rmw_request_id_t * response_header,
  void * ros_cancel_response);

/// 获取动作服务器的动作名称。
/**
 * 此函数返回动作服务器的内部主题名称字符串。
 * 此函数可能失败，因此返回`NULL`，如果：
 *   - 动作服务器为`NULL`
 *   - 动作服务器无效（例如，从未调用init或调用fini）
 *
 * 返回的字符串只在动作服务器有效时有效。
 * 如果主题名称更改，则字符串的值可能会更改，因此，如果这是一个问题，建议复制字符串。
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param[in] action_server 指向动作服务器的指针
 * \return 成功时的名称字符串，或者
 * \return `NULL` 其他情况。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
const char * rcl_action_server_get_action_name(const rcl_action_server_t * action_server);

/// 返回动作服务器的rcl_action_server_options_t。
/**
 * 此函数返回动作服务器的内部选项结构体。
 * 此函数可能失败，因此返回`NULL`，如果：
 *   - 动作服务器为`NULL`
 *   - 动作服务器无效（例如，从未调用init或调用fini）
 *
 * 只有在动作服务器有效时，返回的结构体才有效。
 * 如果动作服务器的选项发生更改，则结构体中的值可能会更改，因此，如果这是一个问题，建议复制结构体。
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param[in] action_server 动作服务器句柄
 * \return 成功时的选项结构体，或者
 * \return `NULL` 其他情况。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
const rcl_action_server_options_t * rcl_action_server_get_options(
  const rcl_action_server_t * action_server);

/// 获取动作服务器正在跟踪的所有目标的目标句柄。
/**
 * 返回指向内部持有的指向目标句柄结构体指针的数组的指针以及数组中的项目数。
 *
 * 如果动作服务器被终止，如果调用rcl_shutdown()，或者如果调用rcl_action_expire_goals()并且一个或多个
 * 目标过期，则返回的句柄将无效。
 * 返回的句柄不能保证在动作服务器的生命周期内有效，因为它可能会被终止并重新创建。
 * 因此，建议每次需要时使用此函数从动作服务器获取句柄，并避免与可能更改其的函数同时使用句柄。
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param[in] action_server 动作服务器句柄
 * \param[out] goal_handles 如果成功，则设置为指向目标句柄的指针数组。
 * \param[out] num_goals 如果成功，则设置为返回数组中的目标数，否则不设置。
 * \return `RCL_RET_OK` 如果成功，或者
 * \return `RCL_RET_ACTION_SERVER_INVALID` 如果动作服务器无效，或者
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或者
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_server_get_goal_handles(
  const rcl_action_server_t * action_server, rcl_action_goal_handle_t *** goal_handles,
  size_t * num_goals);

/// 检查动作服务器是否已经在跟踪目标。
/**
 * 检查内部目标数组中是否正在跟踪目标。
 * 目标状态对返回值没有影响。
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param[in] action_server 动作服务器句柄
 * \param[in] goal_info 包含要检查的目标ID的结构体句柄
 * \return `true` 如果`action_server`当前正在跟踪具有提供的目标ID的目标，或者
 * \return `false` 其他情况。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
bool rcl_action_server_goal_exists(
  const rcl_action_server_t * action_server, const rcl_action_goal_info_t * goal_info);

/// 检查动作服务器是否有效。
/**
 * 如果返回 `false`（即动作服务器无效），
 * 将设置错误消息。
 *
 * 此函数不会失败。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] action_server 动作服务器句柄
 * \return 如果 `action_server` 有效，则返回 `true`，否则
 * \return 返回 `false`。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
bool rcl_action_server_is_valid(const rcl_action_server_t * action_server);

/// 在库关闭时检查动作服务器是否有效，而不报错。
/**
 * 如果返回 `false`（即动作服务器无效），
 * 将设置错误消息。
 *
 * 此函数不会失败。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] action_server 动作服务器句柄
 * \return 如果 `action_server` 有效，则返回 `true`，否则
 * \return 返回 `false`。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
bool rcl_action_server_is_valid_except_context(const rcl_action_server_t * action_server);

/// 设置目标服务回调。
/**
 * \param[in] action_server 动作服务器句柄
 * \param[in] callback 事件回调函数
 * \param[in] user_data 用户数据指针
 * \return 返回操作结果状态码
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_server_set_goal_service_callback(
  const rcl_action_server_t * action_server, rcl_event_callback_t callback, const void * user_data);

/// 设置取消服务回调。
/**
 * \param[in] action_server 动作服务器句柄
 * \param[in] callback 事件回调函数
 * \param[in] user_data 用户数据指针
 * \return 返回操作结果状态码
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_server_set_cancel_service_callback(
  const rcl_action_server_t * action_server, rcl_event_callback_t callback, const void * user_data);

/// 设置结果服务回调。
/**
 * \param[in] action_server 动作服务器句柄
 * \param[in] callback 事件回调函数
 * \param[in] user_data 用户数据指针
 * \return 返回操作结果状态码
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_server_set_result_service_callback(
  const rcl_action_server_t * action_server, rcl_event_callback_t callback, const void * user_data);

#ifdef __cplusplus
}
#endif

#endif  // RCL_ACTION__ACTION_SERVER_H_
