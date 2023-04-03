// Copyright 2015 Open Source Robotics Foundation, Inc.
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

#ifndef RCL__SUBSCRIPTION_H_
#define RCL__SUBSCRIPTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/event_callback.h"
#include "rcl/macros.h"
#include "rcl/node.h"
#include "rcl/visibility_control.h"
#include "rmw/message_sequence.h"
#include "rosidl_runtime_c/message_type_support_struct.h"

/// \brief 内部rcl实现结构体
typedef struct rcl_subscription_impl_s rcl_subscription_impl_t;

/// \brief 封装ROS订阅器的结构体
typedef struct rcl_subscription_s
{
  /// 指向订阅器实现的指针
  rcl_subscription_impl_t * impl;
} rcl_subscription_t;

/// \brief rcl订阅器可用选项
typedef struct rcl_subscription_options_s
{
  /// 订阅器的中间件服务质量设置
  rmw_qos_profile_t qos;
  /// 用于偶发分配的订阅器自定义分配器
  /** 默认行为（malloc/free）请参见：rcl_get_default_allocator() */
  rcl_allocator_t allocator;
  /// 特定于rmw的订阅器选项，例如rmw实现特定的有效载荷
  rmw_subscription_options_t rmw_subscription_options;
  /// 禁用标志以LoanedMessage，通过环境变量初始化
  bool disable_loaned_message;
} rcl_subscription_options_t;

/// \brief 订阅器内容过滤器选项
typedef struct rcl_subscription_content_filter_options_s
{
  rmw_subscription_content_filter_options_t rmw_subscription_content_filter_options;
} rcl_subscription_content_filter_options_t;

/// \brief 返回一个成员设置为`NULL`的rcl_subscription_t结构体
/**
 * 在传递给rcl_subscription_init()之前，应该调用此函数以获取空的rcl_subscription_t。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_subscription_t rcl_get_zero_initialized_subscription(void);

/// 初始化一个ROS订阅器。
/**
 * 在rcl_subscription_t上调用此函数后，可以使用rcl_take()将给定类型的消息发送到给定主题。
 *
 * 给定的rcl_node_t必须有效，且只有在给定的rcl_node_t保持有效时，结果rcl_subscription_t才有效。
 *
 * rosidl_message_type_support_t是基于每个.msg类型获得的。
 * 当用户定义一个ROS消息时，会生成提供所需rosidl_message_type_support_t对象的代码。
 * 可以使用适合语言的机制获取此对象。
 * \todo TODO(wjwwood) 编写这些说明并链接到它
 * 对于C，可以使用宏（例如`std_msgs/String`）：
 *
 * ```c
 * #include <rosidl_runtime_c/message_type_support_struct.h>
 * #include <std_msgs/msg/string.h>
 * const rosidl_message_type_support_t * string_ts =
 *   ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String);
 * ```
 *
 * 对于C++，使用模板函数：
 *
 * ```cpp
 * #include <rosidl_typesupport_cpp/message_type_support.hpp>
 * #include <std_msgs/msgs/string.hpp>
 * using rosidl_typesupport_cpp::get_message_type_support_handle;
 * const rosidl_message_type_support_t * string_ts =
 *   get_message_type_support_handle<std_msgs::msg::String>();
 * ```
 *
 * rosidl_message_type_support_t对象包含用于发布消息的消息类型特定信息。
 *
 * 主题名称必须是遵循未展开名称的主题和服务名称格式规则的c字符串，也称为非完全限定名称：
 *
 * \see rcl_expand_topic_name
 *
 * options结构允许用户设置服务质量设置以及在（取消）初始化订阅时用于分配空间的自定义分配器，例如主题名称字符串。
 *
 * 预期用法（对于C消息）：
 *
 * ```c
 * #include <rcl/rcl.h>
 * #include <rosidl_runtime_c/message_type_support_struct.h>
 * #include <std_msgs/msg/string.h>
 *
 * rcl_node_t node = rcl_get_zero_initialized_node();
 * rcl_node_options_t node_ops = rcl_node_get_default_options();
 * rcl_ret_t ret = rcl_node_init(&node, "node_name", "/my_namespace", &node_ops);
 * // ... 错误处理
 * const rosidl_message_type_support_t * ts =
 *   ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String);
 * rcl_subscription_t subscription = rcl_get_zero_initialized_subscription();
 * rcl_subscription_options_t subscription_ops = rcl_subscription_get_default_options();
 * ret = rcl_subscription_init(&subscription, &node, ts, "chatter", &subscription_ops);
 * // ... 错误处理，完成后取消初始化
 * ret = rcl_subscription_fini(&subscription, &node);
 * // ... rcl_subscription_fini()的错误处理
 * ret = rcl_node_fini(&node);
 * // ... rcl_node_fini()的错误处理
 * ```
 *
 * <hr>
 * 属性              | 符合性
 * ------------------ | -------------
 * 分配内存           | 是
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * \param[out] subscription 预分配的订阅结构
 * \param[in] node 有效的rcl节点句柄
 * \param[in] type_support 主题类型的类型支持对象
 * \param[in] topic_name 主题名称
 * \param[in] options 订阅选项，包括服务质量设置
 * \return #RCL_RET_OK 如果订阅成功初始化，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ALREADY_INIT 如果订阅已经初始化，或
 * \return #RCL_RET_NODE_INVALID 如果节点无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_TOPIC_NAME_INVALID 如果给定的主题名称无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_subscription_init(
  rcl_subscription_t * subscription, const rcl_node_t * node,
  const rosidl_message_type_support_t * type_support, const char * topic_name,
  const rcl_subscription_options_t * options);

/// 结束一个 rcl_subscription_t.
/**
 * 调用后，节点将不再订阅此主题
 * （假设这是此节点中此主题的唯一订阅）。
 *
 * 调用后，使用此订阅时，rcl_wait 和 rcl_take 的调用将失败。
 * 此外，如果当前正在阻塞，rcl_wait 将被中断。
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
 * \param[inout] subscription 要取消初始化的订阅句柄
 * \param[in] node 用于创建订阅的有效（未完成）节点句柄
 * \return #RCL_RET_OK 如果订阅成功取消初始化，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_SUBSCRIPTION_INVALID 如果订阅无效，或
 * \return #RCL_RET_NODE_INVALID 如果节点无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_subscription_fini(rcl_subscription_t * subscription, rcl_node_t * node);

/// 返回 rcl_subscription_options_t 中的默认订阅选项。
/**
 * 默认值为：
 *
 * - qos = rmw_qos_profile_default
 * - allocator = rcl_get_default_allocator()
 * - rmw_subscription_options = rmw_get_default_subscription_options();
 * - disable_loaned_message = false, 如果 ROS_DISABLE_LOANED_MESSAGES=1 则为 true
 *
 * \return 包含订阅的默认选项的结构。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_subscription_options_t rcl_subscription_get_default_options(void);

/// 回收 rcl_subscription_options_t 结构内部持有的资源。
/**
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 否
 *
 * \param[in] option 要释放其资源的结构。
 * \return `RCL_RET_OK` 如果内存成功释放，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果 option 为 NULL，或
 * \return `RCL_RET_BAD_ALLOC` 如果释放内存失败。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_subscription_options_fini(rcl_subscription_options_t * option);

/// 为给定的订阅选项设置内容过滤器选项。
/**
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 否
 *
 * \param[in] filter_expression 过滤表达式类似于 SQL 子句的 WHERE 部分。
 * \param[in] expression_parameters_argc 表达式参数 argc 的最大值为 100。
 * \param[in] expression_parameter_argv 表达式参数 argv 是过滤表达式中的占位符
 * ‘parameters’（即，"%n" 令牌从 0 开始）。
 *
 * 如果 filter_expression 中没有 "%n" 令牌占位符，则可以为 NULL。
 * \param[out] options 要设置的订阅选项。
 * \return `RCL_RET_OK` 如果成功设置选项，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果参数无效，或
 * \return `RCL_RET_BAD_ALLOC` 如果分配内存失败。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_subscription_options_set_content_filter_options(
  const char * filter_expression, size_t expression_parameters_argc,
  const char * expression_parameter_argv[], rcl_subscription_options_t * options);

/// 返回零初始化的订阅内容过滤器选项。
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_subscription_content_filter_options_t
rcl_get_zero_initialized_subscription_content_filter_options(void);

/// 初始化给定订阅选项的内容过滤器选项。
/**
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 否
 *
 * \param[in] subscription 订阅句柄。
 * \param[in] filter_expression 过滤表达式类似于SQL子句的WHERE部分，
 * 使用空字符串("")可以重置（或清除）订阅的内容过滤器设置。
 * \param[in] expression_parameters_argc 表达式参数argc的最大值为100。
 * \param[in] expression_parameter_argv 表达式参数argv是过滤表达式中的占位符
 * ‘parameters’（即从0开始的"%n"标记）。
 *
 * 如果filter_expression中没有"%n"标记占位符，则可以为NULL。
 * \param[out] options 要设置的订阅选项。
 * \return `RCL_RET_OK` 如果成功设置选项，或
 * \return `RCL_RET_SUBSCRIPTION_INVALID` 如果订阅无效，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果参数无效，或
 * \return `RCL_RET_BAD_ALLOC` 如果分配内存失败。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_subscription_content_filter_options_init(
  const rcl_subscription_t * subscription, const char * filter_expression,
  size_t expression_parameters_argc, const char * expression_parameter_argv[],
  rcl_subscription_content_filter_options_t * options);

/// 设置给定订阅选项的内容过滤器选项。
/**
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 否
 *
 * \param[in] subscription 订阅句柄。
 * \param[in] filter_expression 过滤表达式类似于SQL子句的WHERE部分，
 * 使用空字符串("")可以重置（或清除）订阅的内容过滤器设置。
 * \param[in] expression_parameters_argc 表达式参数argc的最大值为100。
 * \param[in] expression_parameter_argv 表达式参数argv是过滤表达式中的占位符
 * ‘parameters’（即从0开始的"%n"标记）。
 *
 * 如果filter_expression中没有"%n"标记占位符，则可以为NULL。
 * \param[out] options 要设置的订阅选项。
 * \return `RCL_RET_OK` 如果成功设置选项，或
 * \return `RCL_RET_SUBSCRIPTION_INVALID` 如果订阅无效，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果参数无效，或
 * \return `RCL_RET_BAD_ALLOC` 如果分配内存失败。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_subscription_content_filter_options_set(
  const rcl_subscription_t * subscription, const char * filter_expression,
  size_t expression_parameters_argc, const char * expression_parameter_argv[],
  rcl_subscription_content_filter_options_t * options);

/// 回收rcl_subscription_content_filter_options_t结构。
/**
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 否
 *
 * \param[in] subscription 订阅句柄。
 * \param[in] options 要释放资源的结构。
 * \return `RCL_RET_OK` 如果成功释放内存，或
 * \return `RCL_RET_SUBSCRIPTION_INVALID` 如果订阅无效，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果选项为NULL，或
 * 如果分配器无效且结构包含已初始化的内存。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_subscription_content_filter_options_fini(
  const rcl_subscription_t * subscription, rcl_subscription_content_filter_options_t * options);

/// 检查订阅中是否启用了内容过滤主题功能。
/**
 * 根据中间件以及订阅中是否启用了cft来判断。
 *
 * \return 如果订阅的内容过滤主题已启用，则返回`true`，否则返回`false`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
bool rcl_subscription_is_cft_enabled(const rcl_subscription_t * subscription);

/// 设置订阅的过滤表达式和表达式参数。
/**
 * 此函数将为给定订阅设置过滤表达式和表达式参数数组。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 可能 [1]
 * 无锁              | 可能 [1]
 *
 * \param[in] subscription 要设置内容过滤选项的订阅。
 * \param[in] options rcl内容过滤选项。
 * \return `RCL_RET_OK` 如果查询成功，或者
 * \return `RCL_RET_INVALID_ARGUMENT` 如果 `subscription` 为 NULL，或者
 * \return `RCL_RET_INVALID_ARGUMENT` 如果 `options` 为 NULL，或者
 * \return `RCL_RET_UNSUPPORTED` 如果实现不支持内容过滤主题，或者
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_subscription_set_content_filter(
  const rcl_subscription_t * subscription,
  const rcl_subscription_content_filter_options_t * options);

/// 获取订阅的过滤表达式。
/**
 * 此函数将返回给定订阅的过滤表达式。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 可能 [1]
 * 无锁              | 可能 [1]
 *
 * \param[in] subscription 要检查的订阅对象。
 * \param[out] options rcl内容过滤选项。
 *   调用者需要使用
 *   rcl_subscription_content_filter_options_fini()来最终确定这些选项。
 * \return `RCL_RET_OK` 如果查询成功，或者
 * \return `RCL_RET_INVALID_ARGUMENT` 如果 `subscription` 为 NULL，或者
 * \return `RCL_RET_INVALID_ARGUMENT` 如果 `options` 为 NULL，或者
 * \return `RCL_RET_BAD_ALLOC` 如果内存分配失败，或者
 * \return `RCL_RET_UNSUPPORTED` 如果实现不支持内容过滤主题，或者
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_subscription_get_content_filter(
  const rcl_subscription_t * subscription, rcl_subscription_content_filter_options_t * options);

/// 使用rcl订阅从主题中获取ROS消息。
/**
 * 调用者需要确保ros_message参数的类型与订阅关联的类型（通过类型支持）匹配。
 * 将不同类型传递给rcl_take会产生未定义行为，此函数无法检查，因此不会出现故意的错误。
 *
 * TODO(wjwwood) 获取的阻塞？
 * TODO(wjwwood) 消息所有权的前置条件、过程中条件和后置条件？
 * TODO(wjwwood) rcl_take是否线程安全？
 * TODO(wjwwood) 是否应该有rcl_message_info_t？
 *
 * ros_message指针应指向一个已分配的正确类型的ROS消息结构，如果有可用的消息，则将获取到的ROS消息复制到其中。
 * 如果调用后取出的布尔值为false，则ROS消息将保持不变。
 *
 * 即使在某些情况下，等待集报告订阅已准备好从中获取，取出的布尔值也可能为false，例如，当订阅的状态发生变化时，它可能导致等待集唤醒，但随后的获取可能无法获取任何内容。
 *
 * 如果在获取消息时需要分配内存，例如，如果需要为目标消息中的动态大小数组分配空间，则使用订阅选项中给定的分配器。
 *
 * rmw_message_info结构包含有关此特定消息实例的元信息，例如发布者的GUID或消息是否来自同一进程。
 * message_info参数应该是一个已分配的rmw_message_info_t结构。
 * 对于message_info传递`NULL`将导致参数被忽略。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 可能 [1]
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 * <i>[1] 仅在填充消息时需要时，避免固定大小</i>
 *
 * \param[in] subscription 要获取的订阅句柄
 * \param[inout] ros_message 指向已分配ROS消息的类型擦除指针
 * \param[out] message_info 包含消息元数据的rmw结构
 * \param[in] allocation 用于内存预分配的结构指针（可以为NULL）
 * \return #RCL_RET_OK 如果消息被获取，或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或者
 * \return #RCL_RET_SUBSCRIPTION_INVALID 如果订阅无效，或者
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或者
 * \return #RCL_RET_SUBSCRIPTION_TAKE_FAILED 如果获取失败但中间件没有发生错误，或者
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_take(
  const rcl_subscription_t * subscription, void * ros_message, rmw_message_info_t * message_info,
  rmw_subscription_allocation_t * allocation);

/// 从一个主题中使用 rcl 订阅获取一系列消息。
/**
 * 与 rcl_take() 相比，此函数可以同时获取多个消息。
 * 确保 message_sequence 参数的类型与订阅关联的类型（通过类型支持）匹配是调用者的责任。
 *
 * message_sequence 指针应指向已分配的正确类型的 ROS 消息序列，
 * 如果有可用的消息，则将获取到的 ROS 消息复制到其中。
 * message_sequence `size` 成员将设置为正确获取的消息数。
 *
 * rmw_message_info_sequence 结构包含相应消息实例索引的元信息。
 * message_info_sequence 参数应该是已分配的 rmw_message_info_sequence_t 结构。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 可能 [1]
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 * <i>[1] 仅当 serialized_message 中的存储空间不足时</i>
 *
 * \param[in] subscription 要获取的订阅句柄。
 * \param[in] count 尝试获取的消息数量。
 * \param[inout] message_sequence 指向（预先分配的）消息序列的指针。
 * \param[inout] message_info_sequence 指向（预先分配的）消息信息序列的指针。
 * \param[in] allocation 用于内存预分配的结构指针（可以为 NULL）
 * \return #RCL_RET_OK 如果获取到一个或多个消息，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_SUBSCRIPTION_INVALID 如果订阅无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_SUBSCRIPTION_TAKE_FAILED 如果获取失败但中间件中没有错误发生，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_take_sequence(
  const rcl_subscription_t * subscription, size_t count, rmw_message_sequence_t * message_sequence,
  rmw_message_info_sequence_t * message_info_sequence, rmw_subscription_allocation_t * allocation);

/// 使用 rcl 订阅从主题获取序列化的原始消息。
/**
 * 与 rcl_take() 相比，此函数将获取到的消息存储在其原始二进制表示中。
 * 调用者需要确保与订阅关联的类型匹配，并且可以通过正确的类型支持选择性地反序列化为其 ROS 消息。
 * 如果 `serialized_message` 参数包含足够的预分配内存，则可以在不进行任何其他内存分配的情况下获取传入的消息。
 * 如果没有，则该函数将动态分配足够的内存以容纳消息。
 * 将不同类型传递给 rcl_take 会产生未定义的行为，而且这个函数无法检查，因此不会发生故意的错误。
 *
 * 除了上述差异之外，此函数的行为类似于 rcl_take()。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 可能 [1]
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 * <i>[1] 仅当 serialized_message 中的存储空间不足时</i>
 *
 * \param[in] subscription 要获取的订阅句柄
 * \param[inout] serialized_message 指向（预先分配的）序列化消息的指针。
 * \param[out] message_info 包含消息元数据的 rmw 结构
 * \param[in] allocation 用于内存预分配的结构指针（可以为 NULL）
 * \return #RCL_RET_OK 如果消息已发布，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_SUBSCRIPTION_INVALID 如果订阅无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_SUBSCRIPTION_TAKE_FAILED 如果获取失败但中间件中没有错误发生，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_take_serialized_message(
  const rcl_subscription_t * subscription, rcl_serialized_message_t * serialized_message,
  rmw_message_info_t * message_info, rmw_subscription_allocation_t * allocation);

/// 使用 rcl 订阅从主题获取借用的消息。
/**
 * 根据中间件，传入的消息可以在不进行进一步复制的情况下借给用户的回调。
 * 这里的隐含约定是中间件拥有为此消息分配的内存。
 * 用户不能销毁消息，而是必须通过调用 \sa rcl_return_loaned_message 将其返回给中间件。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] subscription 要获取的订阅句柄
 * \param[inout] loaned_message 指向借用的消息的指针。
 * \param[out] message_info 包含消息元数据的 rmw 结构。
 * \param[in] allocation 用于内存预分配的结构指针（可以为 NULL）
 * \return #RCL_RET_OK 如果获取到了借用的消息序列，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_SUBSCRIPTION_INVALID 如果订阅无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_SUBSCRIPTION_TAKE_FAILED 如果获取失败但中间件中没有错误发生，或
 * \return #RCL_RET_UNSUPPORTED 如果中间件不支持该功能，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_take_loaned_message(
  const rcl_subscription_t * subscription, void ** loaned_message,
  rmw_message_info_t * message_info, rmw_subscription_allocation_t * allocation);

/// 使用 rcl 订阅从主题返回借用的消息。
/**
 * 如果先前使用 \sa rcl_take_loaned_message 从中间件获取了借用的消息，
 * 则必须将此消息返回以指示中间件用户不再需要该内存。
 * 用户不能删除消息。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] subscription 要获取的订阅句柄
 * \param[in] loaned_message 指向借用的消息的指针。
 * \return #RCL_RET_OK 如果消息已发布，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_SUBSCRIPTION_INVALID 如果订阅无效，或
 * \return #RCL_RET_UNSUPPORTED 如果中间件不支持该功能，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_return_loaned_message_from_subscription(
  const rcl_subscription_t * subscription, void * loaned_message);

/// 获取订阅的主题名称。
/**
 * 此函数返回订阅的内部主题名称字符串。
 * 如果以下情况之一成立，此函数可能失败并返回 `NULL`：
 *   - 订阅为 `NULL`
 *   - 订阅无效（从未调用 init、调用 fini 或无效）
 *
 * 返回的字符串只在订阅有效期间有效。
 * 如果主题名称发生变化，字符串的值可能会改变，因此如果担心这个问题，建议复制该字符串。
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存           | 否
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * \param[in] subscription 指向订阅的指针
 * \return 成功时返回名称字符串，否则返回 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const char * rcl_subscription_get_topic_name(const rcl_subscription_t * subscription);

/// 返回 rcl 订阅选项。
/**
 * 此函数返回订阅的内部选项结构体。
 * 如果以下情况之一成立，此函数可能失败并返回 `NULL`：
 *   - 订阅为 `NULL`
 *   - 订阅无效（从未调用 init、调用 fini 或无效）
 *
 * 只有在订阅有效时，返回的结构体才有效。
 * 如果订阅的选项发生变化，结构体中的值可能会改变，因此如果担心这个问题，建议复制该结构体。
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存           | 否
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * \param[in] subscription 指向订阅的指针
 * \return 成功时返回选项结构体，否则返回 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const rcl_subscription_options_t * rcl_subscription_get_options(
  const rcl_subscription_t * subscription);

/// 返回 rmw 订阅句柄。
/**
 * 返回的句柄是指向内部持有的 rmw 句柄的指针。
 * 如果以下情况之一成立，此函数可能失败并返回 `NULL`：
 *   - 订阅为 `NULL`
 *   - 订阅无效（从未调用 init、调用 fini 或无效）
 *
 * 如果订阅被终止或调用 rcl_shutdown()，返回的句柄将失效。
 * 返回的句柄不能保证在订阅的生命周期内一直有效，因为它可能被终止并重新创建。
 * 因此，建议使用此函数每次需要时从订阅获取句柄，并避免与可能更改句柄的函数同时使用句柄。
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存           | 否
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * \param[in] subscription 指向 rcl 订阅的指针
 * \return 成功时返回 rmw 订阅句柄，否则返回 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rmw_subscription_t * rcl_subscription_get_rmw_handle(const rcl_subscription_t * subscription);

/// 检查订阅是否有效。
/**
 * 如果 `subscription` 无效，则返回的 bool 值为 `false`。
 * 否则，返回的 bool 值为 `true`。
 * 在返回 `false` 的情况下，会设置一条错误消息。
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
 * \param[in] subscription 指向 rcl 订阅的指针
 * \return 如果 `subscription` 有效，则返回 `true`，否则返回 `false`
 */
RCL_PUBLIC
bool rcl_subscription_is_valid(const rcl_subscription_t * subscription);

/// 获取与订阅匹配的发布者数量。
/**
 * 用于获取与订阅匹配的内部发布者计数。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 可能 [1]
 * 无锁              | 可能 [1]
 * <i>[1] 只有在底层 rmw 不使用此功能时才适用 </i>
 *
 * \param[in] subscription 指向 rcl 订阅的指针
 * \param[out] publisher_count 匹配的发布者数量
 * \return #RCL_RET_OK 如果计数已检索，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_SUBSCRIPTION_INVALID 如果订阅无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rmw_ret_t rcl_subscription_get_publisher_count(
  const rcl_subscription_t * subscription, size_t * publisher_count);

/// 获取订阅的实际 qos 设置。
/**
 * 用于获取订阅的实际 qos 设置。
 * 当使用 RMW_*_SYSTEM_DEFAULT 时，实际应用的配置只能在创建订阅后解析，
 * 并且取决于底层 rmw 实现。
 * 如果正在使用的底层设置无法用 ROS 术语表示，
 * 则将其设置为 RMW_*_UNKNOWN。
 * 返回的结构仅在 rcl_subscription_t 有效时有效。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] subscription 指向 rcl 订阅的指针
 * \return 成功时返回 qos 结构，否则返回 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const rmw_qos_profile_t * rcl_subscription_get_actual_qos(const rcl_subscription_t * subscription);

/// 检查订阅实例是否可以借用消息。
/**
 * 根据中间件和消息类型，如果中间件可以分配 ROS 消息实例，则返回 true。
 *
 * \param[in] subscription 要检查是否能够借用消息的订阅实例
 * \return 如果订阅实例可以借用消息，则返回 `true`，否则返回 `false`。
 */
RCL_PUBLIC
bool rcl_subscription_can_loan_messages(const rcl_subscription_t * subscription);

/// 为订阅设置新消息回调函数。
/**
 * 此 API 设置回调函数，以便在订阅收到新消息通知时调用。
 *
 * \sa rmw_subscription_set_on_new_message_callback 了解有关此功能的详细信息。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 可能 [1]
 * 无锁              | 可能 [1]
 * <i>[1] rmw 实现定义</i>
 *
 * \param[in] subscription 要设置回调的订阅
 * \param[in] callback 当新消息到达时要调用的回调，可以为 NULL
 * \param[in] user_data 后续调用回调时提供，可以为 NULL
 * \return `RCL_RET_OK` 如果成功，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果 `subscription` 为 NULL，或
 * \return `RCL_RET_UNSUPPORTED` 如果 dds 实现中未实现该 API
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_subscription_set_on_new_message_callback(
  const rcl_subscription_t * subscription, rcl_event_callback_t callback, const void * user_data);

#ifdef __cplusplus
}
#endif

#endif  // RCL__SUBSCRIPTION_H_
