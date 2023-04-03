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

#ifndef RCL_ACTION__ACTION_CLIENT_H_
#define RCL_ACTION__ACTION_CLIENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/event_callback.h"
#include "rcl/macros.h"
#include "rcl/node.h"
#include "rcl_action/types.h"
#include "rcl_action/visibility_control.h"

/// 内部 action 客户端实现结构体。
typedef struct rcl_action_client_impl_s rcl_action_client_impl_t;

/// 封装了一个 ROS action 客户端的结构体。
typedef struct rcl_action_client_s
{
  /// 指向 action 客户端实现的指针
  rcl_action_client_impl_t * impl;
} rcl_action_client_t;

/// rcl_action_client_t 可用的选项。
typedef struct rcl_action_client_options_s
{
  /// action 客户端的中间件服务质量设置。
  /// 目标服务的服务质量
  rmw_qos_profile_t goal_service_qos;
  /// 结果服务的服务质量
  rmw_qos_profile_t result_service_qos;
  /// 取消服务的服务质量
  rmw_qos_profile_t cancel_service_qos;
  /// 反馈主题的服务质量
  rmw_qos_profile_t feedback_topic_qos;
  /// 状态主题的服务质量
  rmw_qos_profile_t status_topic_qos;
  /// action 客户端的自定义分配器，用于偶发性分配。
  /** 默认行为（malloc/free），请参阅：rcl_get_default_allocator() */
  rcl_allocator_t allocator;
} rcl_action_client_options_t;

/// 返回一个成员设置为 `NULL` 的 rcl_action_client_t 结构体。
/**
 * 在传递给 rcl_action_client_init() 之前，应调用此函数以获取空的 rcl_action_client_t。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_action_client_t rcl_action_get_zero_initialized_client(void);

/// 初始化一个 rcl_action_client_t。
/**
 * 在对 rcl_action_client_t 调用此函数之后，可以使用 rcl_action_send_goal_request() 向给定主题发送给定类型的目标。
 * 如果将目标请求发送到（可能是远程）服务器并且服务器发送响应，则一旦响应可用，客户端就可以使用 rcl_take_goal_response() 访问响应。
 *
 * 在目标请求被接受之后，与目标关联的 rcl_action_client_t 可以执行以下操作：
 *
 * - 使用 rcl_action_send_result_request() 发送结果请求。
 *   如果在目标终止时服务器发送响应，则一旦响应可用，就可以使用 rcl_action_take_result_response() 访问结果。
 * - 使用 rcl_action_send_cancel_request() 为目标发送取消请求。
 *   如果服务器对取消请求发送响应，则一旦响应可用，客户端就可以使用 rcl_action_take_cancel_response() 访问响应。
 * - 使用 rcl_action_take_feedback() 获取关于目标的反馈。
 *
 * rcl_action_client_t 可用于访问由 action 服务器传达的所有已接受目标的当前状态，方法是使用 rcl_action_take_status()。
 *
 * 给定的 rcl_node_t 必须有效，生成的 rcl_action_client_t 仅在给定的 rcl_node_t 保持有效时有效。
 *
 * rosidl_action_type_support_t 是基于每个 .action 类型获得的。
 * 当用户定义一个 ROS action 时，会生成提供所需 rosidl_action_type_support_t 对象的代码。
 * 可以使用适合语言的机制获取此对象。
 *
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
 * rosidl_action_type_support_t 对象包含用于发送或接收目标、结果和反馈的 action 类型特定信息。
 *
 * 主题名称必须是遵循未展开名称的主题和服务名称格式规则的 c 字符串，也称为非完全限定名称：
 *
 * \see rcl_expand_topic_name
 *
 * options 结构允许用户设置服务质量设置以及在初始化/终止客户端时用于分配空间的自定义分配器，例如 action 客户端名称字符串。
 *
 * 预期用法（对于 C action 客户端）：
 *
 * ```c
 * #include <rcl/rcl.h>
 * #include <rcl_action/action_client.h>
 * #include <rosidl_runtime_c/action_type_support_struct.h>
 * #include <example_interfaces/action/fibonacci.h>
 *
 * rcl_node_t node = rcl_get_zero_initialized_node();
 * rcl_node_options_t node_ops = rcl_node_get_default_options();
 * rcl_ret_t ret = rcl_node_init(&node, "node_name", "/my_namespace", &node_ops);
 * // ... 错误处理
 * const rosidl_action_type_support_t * ts =
 *   ROSIDL_GET_ACTION_TYPE_SUPPORT(example_interfaces, Fibonacci);
 * rcl_action_client_t action_client = rcl_action_get_zero_initialized_client();
 * rcl_action_client_options_t action_client_ops = rcl_action_client_get_default_options();
 * ret = rcl_action_client_init(&action_client, &node, ts, "fibonacci", &action_client_ops);
 * // ... 错误处理，以及在关闭时执行终止操作：
 * ret = rcl_action_client_fini(&action_client, &node);
 * // ... rcl_action_client_fini() 的错误处理
 * ret = rcl_node_fini(&node);
 * // ... rcl_node_fini() 的错误处理
 * ```
 *
 * <hr>
 * 属性          | 符合性
 * ------------------ | -------------
 * 分配内存   | 是
 * 线程安全        | 否
 * 使用原子       | 否
 * 无锁          | 是
 *
 * \param[out] action_client 要初始化的预分配、零初始化的 action 客户端结构
 * \param[in] node 有效的 rcl 节点句柄
 * \param[in] type_support action 类型的类型支持对象
 * \param[in] action_name action 的名称
 * \param[in] options 包括服务质量设置在内的 action_client 选项
 * \return `RCL_RET_OK` 如果 action_client 初始化成功，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_NODE_INVALID` 如果节点无效，或
 * \return `RCL_RET_ALREADY_INIT` 如果 action 客户端已经初始化，或
 * \return `RCL_RET_BAD_ALLOC` 如果分配内存失败，或
 * \return `RCL_RET_ACTION_NAME_INVALID` 如果给定的 action 名称无效，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_client_init(
  rcl_action_client_t * action_client, rcl_node_t * node,
  const rosidl_action_type_support_t * type_support, const char * action_name,
  const rcl_action_client_options_t * options);

/// 结束一个 rcl_action_client_t.
/**
 * 调用后，节点将不再为此操作客户端监听目标
 * （假设这是此节点中此类型的唯一操作客户端）。
 *
 * 调用后，使用此操作客户端时，
 * 对 rcl_wait()、rcl_action_send_goal_request()、
 * rcl_action_take_goal_response()、rcl_action_send_cancel_request()、
 * rcl_action_take_cancel_response()、rcl_action_send_result_request()、
 * rcl_action_take_result_response()、rcl_action_take_feedback() 和
 * rcl_action_take_status() 的调用将失败。
 *
 * 此外，如果当前阻塞，rcl_wait() 将被中断。
 *
 * 给定的节点句柄仍然有效。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[inout] action_client 要取消初始化的 action_client 句柄
 * \param[in] node 用于创建操作客户端的节点句柄
 * \return `RCL_RET_OK` 如果操作客户端成功取消初始化，或
 * \return `RCL_RET_ACTION_CLIENT_INVALID` 如果操作客户端无效，或
 * \return `RCL_RET_NODE_INVALID` 如果节点无效，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_client_fini(rcl_action_client_t * action_client, rcl_node_t * node);

/// 返回 rcl_action_client_options_t 中的默认操作客户端选项。
/**
 * 默认值为：
 *
 * - goal_service_qos = rmw_qos_profile_services_default;
 * - result_service_qos = rmw_qos_profile_services_default;
 * - cancel_service_qos = rmw_qos_profile_services_default;
 * - feedback_topic_qos = rmw_qos_profile_default;
 * - status_topic_qos = rcl_action_qos_profile_status_default;
 * - allocator = rcl_get_default_allocator()
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_action_client_options_t rcl_action_client_get_default_options(void);

/// 检查给定操作客户端是否有可用的操作服务器。
/**
 * 如果给定操作客户端有可用的操作服务器，
 * 此函数将为 is_available 返回 true。
 *
 * 节点参数不能为 `NULL`，并且必须指向有效节点。
 *
 * 客户端参数不能为 `NULL`，并且必须指向有效客户端。
 *
 * 给定的客户端和节点必须匹配，即客户端必须使用给定节点创建。
 *
 * is_available 参数不能为 `NULL`，并且必须指向一个 bool 变量。
 * 检查结果将存储在 is_available 参数中。
 *
 * 如果错误处理需要分配内存，此函数将尝试使用节点的分配器。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 可能 [1]
 * <i>[1] 实现可能需要使用锁保护数据结构</i>
 *
 * \param[in] node 正在用于查询 ROS 图的节点句柄
 * \param[in] client 被查询的操作客户端句柄
 * \param[out] is_available 如果有可用的操作服务器，则设置为 true，否则为 false
 * \return `RCL_RET_OK` 如果成功（无论操作服务器是否可用），或
 * \return `RCL_RET_NODE_INVALID` 如果节点无效，或
 * \return `RCL_RET_ACTION_CLIENT_INVALID` 如果操作客户端无效，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_server_is_available(
  const rcl_node_t * node, const rcl_action_client_t * client, bool * is_available);

/// 使用 rcl_action_client_t 发送一个 ROS 目标。
/**
 * 这是一个非阻塞调用。
 *
 * 调用者有责任确保 `ros_goal_request` 的类型
 * 与客户端关联的类型（通过类型支持）匹配。
 * 传递不同的类型会产生未定义的行为，这个函数无法检查，
 * 因此不会发生故意的错误。
 *
 * 由 `ros_goal_request` void 指针给出的 ROS 目标消息始终
 * 由调用代码拥有，但在此功能执行期间应保持不变。即在 rcl_action_send_goal_request() 调用期间，
 * 消息不能更改。在调用 rcl_action_send_goal_request() 之前，消息可以更改，但在调用
 * rcl_action_send_goal_request() 之后，它取决于 RMW 实现行为。
 * 同一个 `ros_goal_request` 可以同时传递给此函数的多个调用，
 * 即使操作客户端不同。
 *
 * 只要对 rcl_action_client_t 和 `ros_goal_request` 的访问得到同步，
 * 此功能就是线程安全的。
 * 这意味着允许从多个线程调用 rcl_action_send_goal_request()，
 * 但与非线程安全操作一起调用 rcl_action_send_goal_request() 是不允许的，
 * 例如，同时调用 rcl_action_send_goal_request() 和 rcl_action_client_fini() 是不允许的。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是 [1]
 * 使用原子操作      | 否
 * 无锁              | 是
 * <i>[1] 对于唯一的客户端和目标对，参见上文了解更多信息</i>
 *
 * \param[in] action_client 将发出目标请求的客户端句柄
 * \param[in] ros_goal_request 指向 ROS 目标消息的指针
 * \param[out] sequence_number 指向目标请求序列号的指针
 * \return `RCL_RET_OK` 如果请求发送成功，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_ACTION_CLIENT_INVALID` 如果客户端无效，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_send_goal_request(
  const rcl_action_client_t * action_client, const void * ros_goal_request,
  int64_t * sequence_number);

/// 使用 rcl_action_client_t 从操作服务器获取目标请求的响应。
/**
 * \todo TODO(jacobperron) 获取阻塞？
 *
 * \todo TODO(jacobperron) 消息所有权的前置条件、过程中和后置条件？
 *
 * \todo TODO(jacobperron) 这是线程安全的吗？
 *
 * 调用者有责任确保 `ros_goal_response` 的类型
 * 与客户端关联的类型（通过类型支持）匹配。
 * 传递不同的类型会产生未定义的行为，这个函数无法检查，
 * 因此不会发生故意的错误。
 *
 * 调用者必须为 `ros_goal_response` 提供一个指向已分配消息的指针。
 * 如果获取成功，此函数将填充 `ros_goal_response` 的字段。
 * 由 `ros_goal_response` void 指针给出的 ROS 消息始终
 * 由调用代码拥有，但在此功能执行期间应保持不变。即在 rcl_action_send_goal_response() 调用期间，
 * 消息不能更改。在调用 rcl_action_send_goal_response() 之前，消息可以更改，但在调用
 * rcl_action_send_goal_response() 之后，它取决于 RMW 实现行为。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] action_client 将获取目标响应的客户端句柄
 * \param[out] response_header 指向目标响应头的指针
 * \param[out] ros_goal_response 指向目标请求响应的指针
 * \return `RCL_RET_OK` 如果响应成功获取，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_ACTION_CLIENT_INVALID` 如果操作客户端无效，或
 * \return `RCL_RET_ACTION_CLIENT_TAKE_FAILED` 如果获取失败但中间件中没有错误发生，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_take_goal_response(
  const rcl_action_client_t * action_client, rmw_request_id_t * response_header,
  void * ros_goal_response);

/// 使用 rcl_action_client_t 获取与活动目标关联的 ROS 反馈消息。
/**
 * 调用者有责任确保 `ros_feedback` 的类型
 * 与客户端关联的类型（通过类型支持）匹配。
 * 传递不同的类型会产生未定义的行为，这个函数无法检查，
 * 因此不会发生故意的错误。
 *
 * \todo TODO(jacobperron) 获取阻塞？
 *
 * \todo TODO(jacobperron) 消息所有权的前置条件、过程中和后置条件？
 *
 * \todo TODO(jacobperron) 这是线程安全的吗？
 *
 * `ros_feedback` 应该指向一个预先分配的正确类型的 ROS 消息结构。
 * 如果成功获取反馈，则将反馈消息复制到 `ros_feedback` 结构中。
 *
 * 如果在获取反馈时需要分配内存，例如，如果需要为目标消息中的动态大小数组分配空间，
 * 则使用操作客户端选项中给定的分配器。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 可能 [1]
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 * <i>[1] 仅在填充反馈消息时需要，对于固定大小避免</i>
 *
 * \param[in] action_client 将获取操作反馈的客户端句柄
 * \param[out] ros_feedback 指向 ROS 反馈消息的指针。
 * \return `RCL_RET_OK` 如果响应成功获取，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_ACTION_CLIENT_INVALID` 如果操作客户端无效，或
 * \return `RCL_RET_BAD_ALLOC` 如果分配内存失败，或
 * \return `RCL_RET_ACTION_CLIENT_TAKE_FAILED` 如果获取失败但中间件中没有错误发生，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_take_feedback(const rcl_action_client_t * action_client, void * ros_feedback);

/// 使用 rcl_action_client_t 获取一个 ROS 状态消息。
/**
 * 调用者有责任确保 `ros_status_array` 的类型与
 * 通过类型支持与客户端关联的类型匹配。
 * 传递不同的类型会产生未定义的行为，这个函数无法检查，
 * 因此不会发生故意的错误。
 *
 * \todo TODO(jacobperron) 获取时是否阻塞？
 *
 * \todo TODO(jacobperron) 消息所有权的前置条件、过程中和后置条件？
 *
 * \todo TODO(jacobperron) 这是线程安全的吗？
 *
 * 调用者负责使用零初始化分配 `ros_status_array` 结构（内部数组不应分配）。
 * 如果成功获取，则使用操作客户端选项中给定的分配器填充 `ros_status_array`。
 * 调用者有责任使用操作客户端选项中给定的分配器释放 `ros_status_array` 结构。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 是
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * \param[in] action_client 将获取状态消息的客户端句柄
 * \param[out] ros_status_array 将填充有关已接受取消请求的目标信息的
 *   ROS aciton_msgs/StatusArray 消息指针。
 * \return `RCL_RET_OK` 如果成功获取响应，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_ACTION_CLIENT_INVALID` 如果操作客户端无效，或
 * \return `RCL_RET_BAD_ALLOC` 如果分配内存失败，或
 * \return `RCL_RET_ACTION_CLIENT_TAKE_FAILED` 如果获取失败但中间件中没有错误发生，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_take_status(
  const rcl_action_client_t * action_client, void * ros_status_array);

/// 发送一个与 rcl_action_client_t 关联的已完成目标的结果请求。
/**
 * 这是一个非阻塞调用。
 *
 * 调用者有责任确保 `ros_result_request` 的类型与
 * 通过类型支持与客户端关联的类型匹配。
 * 传递不同的类型会产生未定义的行为，这个函数无法检查，
 * 因此不会发生故意的错误。
 *
 * 给定的 `ros_result_request` 空指针的 ROS 消息始终由调用代码拥有，
 * 但在此功能执行期间应保持不变。即，在 rcl_action_send_result_request() 调用期间，
 * 消息不能更改。在调用 rcl_action_send_result_request() 之前，消息可以更改，
 * 但在调用 rcl_action_send_result_request() 之后，它取决于 RMW 实现行为。
 * 同一个 `ros_result_request` 可以同时传递给此函数的多个调用，即使操作客户端不同。
 *
 * 只要对 rcl_action_client_t 和 `ros_result_request` 的访问得到同步，
 * 此功能就是线程安全的。
 * 这意味着允许从多个线程调用 rcl_action_send_result_request()，
 * 但在与非线程安全操作客户端功能同时调用 rcl_action_send_result_request() 是不允许的，
 * 例如，在 rcl_action_send_result_request() 和 rcl_action_client_fini() 并发调用是不允许的。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 否
 * 线程安全           | 是 [1]
 * 使用原子操作       | 否
 * 无锁               | 是
 * <i>[1] 对于唯一的客户端和结果请求对，如上所述</i>
 *
 * \param[in] action_client 将发送结果请求的客户端句柄
 * \param[in] ros_result_request 指向 ROS 结果请求消息的指针
 * \param[out] sequence_number 指向结果请求序列号的指针
 * \return `RCL_RET_OK` 如果请求发送成功，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_ACTION_CLIENT_INVALID` 如果操作客户端无效，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_send_result_request(
  const rcl_action_client_t * action_client, const void * ros_result_request,
  int64_t * sequence_number);

/// 获取与 rcl_action_client_t 关联的已完成目标的 ROS 结果消息。
/**
 * \todo TODO(jacobperron) 获取时是否阻塞？
 *
 * \todo TODO(jacobperron) 消息所有权的前置条件、过程中和后置条件？
 *
 * \todo TODO(jacobperron) 这是线程安全的吗？
 *
 * 调用者有责任确保 `ros_result_response` 的类型与
 * 通过类型支持与客户端关联的类型匹配。
 * 传递不同的类型会产生未定义的行为，这个函数无法检查，
 * 因此不会发生故意的错误。
 *
 * 调用者必须为 `ros_result_response` 提供一个分配的消息指针。
 * 如果获取成功，此函数将填充 `ros_result_response` 的字段。
 * 给定的 `ros_result_response` 空指针的 ROS 消息始终由调用代码拥有，
 * 但在此功能执行期间应保持不变。即，在 rcl_action_take_result_response() 调用期间，
 * 消息不能更改。在调用 rcl_action_take_result_response() 之前，消息可以更改，
 * 但在调用 rcl_action_take_result_response() 之后，它取决于 RMW 实现行为。
 *
 * 如果在获取结果时需要分配内存，例如，如果需要为目标消息中的动态大小数组分配空间，
 * 则使用操作客户端选项中给定的分配器。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 可能 [1]
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 是
 * <i>[1] 仅在填充结果响应消息时需要，对于固定大小避免</i>
 *
 * \param[in] action_client 将获取结果响应的客户端句柄
 * \param[out] response_header 指向结果响应头的指针
 * \param[out] ros_result 预先分配的、零初始化的结构，其中将复制 ROS 结果消息。
 * \return `RCL_RET_OK` 如果成功获取响应，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_ACTION_CLIENT_INVALID` 如果操作客户端无效，或
 * \return `RCL_RET_BAD_ALLOC` 如果分配内存失败，或
 * \return `RCL_RET_ACTION_CLIENT_TAKE_FAILED` 如果获取失败但中间件中没有错误发生，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_take_result_response(
  const rcl_action_client_t * action_client, rmw_request_id_t * response_header, void * ros_result);

/// 使用 rcl_action_client_t 发送取消目标请求。
/**
 * 这是一个非阻塞调用。
 *
 * 调用者有责任确保 `ros_cancel_request` 的类型与
 * 通过类型支持与客户端关联的类型匹配。
 * 传递不同的类型会产生未定义的行为，这个函数无法检查，
 * 因此不会发生故意的错误。
 *
 * 根据 `ros_cancel_request` 消息提供的目标 ID 和时间戳，
 * 以下取消策略适用：
 *
 * - 如果目标 ID 为零且时间戳为零，则取消所有目标。
 * - 如果目标 ID 为零且时间戳不为零，则取消在时间戳之前或接受的所有目标。
 * - 如果目标 ID 不为零且时间戳为零，则取消具有给定 ID 的目标，而不考虑接受时间。
 * - 如果目标 ID 不为零且时间戳不为零，则取消具有给定 ID 的目标以及在时间戳之前或接受的所有目标。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] action_client 将发出取消请求的客户端句柄
 * \param[in] ros_cancel_request 指向 ROS 取消请求消息的指针
 * \param[out] sequence_number 指向取消请求序列号的指针
 * \return `RCL_RET_OK` 如果成功发送响应，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_ACTION_CLIENT_INVALID` 如果操作客户端无效，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_send_cancel_request(
  const rcl_action_client_t * action_client, const void * ros_cancel_request,
  int64_t * sequence_number);

/// 使用 rcl_action_client_t 获取取消响应。
/**
 * \todo TODO(jacobperron) 阻塞 take？
 *
 * \todo TODO(jacobperron) 消息所有权的前置条件、过程中和后置条件？
 *
 * \todo TODO(jacobperron) 这是线程安全的吗？
 *
 * 调用者有责任确保 `ros_cancel_response` 的类型与
 * 通过类型支持与客户端关联的类型匹配。
 * 传递不同的类型会产生未定义的行为，这个函数无法检查，
 * 因此不会发生故意的错误。
 *
 * 调用者有责任使用零初始化分配 `ros_cancel_response` 消息
 * （内部数组不应分配）。
 * 如果成功获取响应，则使用操作客户端选项中给定的分配器填充 `ros_cancel_response`。
 * 调用者有责任使用操作客户端选项中给定的分配器释放 `ros_cancel_response`。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] action_client 将获取取消响应的客户端句柄
 * \param[out] response_header 指向取消响应头的指针
 * \param[out] ros_cancel_response 取消响应将被复制到的零初始化 ROS 取消响应消息
 * \return `RCL_RET_OK` 如果成功获取响应，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_ACTION_CLIENT_INVALID` 如果操作客户端无效，或
 * \return `RCL_RET_BAD_ALLOC` 如果分配内存失败，或
 * \return `RCL_RET_ACTION_CLIENT_TAKE_FAILED` 如果获取失败但中间件中没有错误发生，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_take_cancel_response(
  const rcl_action_client_t * action_client, rmw_request_id_t * response_header,
  void * ros_cancel_response);

/// 获取 rcl_action_client_t 的操作名称。
/**
 * 此函数返回操作客户端的名称字符串。
 * 如果以下情况之一成立，此函数可能失败，因此返回 `NULL`：
 *   - 操作客户端为 `NULL`
 *   - 操作客户端无效（从未调用 init、调用 fini 或无效）
 *
 * 只要操作客户端有效，返回的字符串就有效。
 * 如果主题名称更改，则字符串值可能会更改，因此
 * 如果这是一个问题，建议复制字符串。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] action_client 指向操作客户端的指针
 * \return 成功时的名称字符串，否则为 `NULL`
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
const char * rcl_action_client_get_action_name(const rcl_action_client_t * action_client);

/// 返回 rcl_action_client_t 的选项。
/**
 * 此函数返回操作客户端的内部选项结构。
 * 如果以下情况之一成立，此函数可能失败，因此返回 `NULL`：
 *   - 操作客户端为 `NULL`
 *   - 操作客户端无效（从未调用 init、调用 fini 或无效）
 *
 * 只要操作客户端有效，返回的结构就有效。
 * 如果操作客户端的选项更改，则结构中的值可能会更改，因此
 * 如果这是一个问题，建议复制结构。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] action_client 指向操作客户端的指针
 * \return 成功时的选项结构，否则为 `NULL`
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
const rcl_action_client_options_t * rcl_action_client_get_options(
  const rcl_action_client_t * action_client);

/// 检查 rcl_action_client_t 是否有效。
/**
 * 如果 `action_client` 无效，则返回的布尔值为 `false`。
 * 否则返回的布尔值为 `true`。
 * 在返回 `false` 的情况下，会设置错误消息。
 * 此函数不能失败。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] action_client 指向 rcl 操作客户端的指针
 * \return 如果 `action_client` 有效，则为 `true`，否则为 `false`
 */
RCL_ACTION_PUBLIC
bool rcl_action_client_is_valid(const rcl_action_client_t * action_client);

RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_client_set_goal_client_callback(
  const rcl_action_client_t * action_client, rcl_event_callback_t callback, const void * user_data);

RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_client_set_cancel_client_callback(
  const rcl_action_client_t * action_client, rcl_event_callback_t callback, const void * user_data);

RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_client_set_result_client_callback(
  const rcl_action_client_t * action_client, rcl_event_callback_t callback, const void * user_data);

RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_client_set_feedback_subscription_callback(
  const rcl_action_client_t * action_client, rcl_event_callback_t callback, const void * user_data);

RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_client_set_status_subscription_callback(
  const rcl_action_client_t * action_client, rcl_event_callback_t callback, const void * user_data);

#ifdef __cplusplus
}
#endif

#endif  // RCL_ACTION__ACTION_CLIENT_H_
