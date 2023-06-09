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

#ifndef RCL__PUBLISHER_H_
#define RCL__PUBLISHER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/macros.h"
#include "rcl/node.h"
#include "rcl/time.h"
#include "rcl/visibility_control.h"
#include "rosidl_runtime_c/message_type_support_struct.h"

/// @brief 内部rcl发布者实现结构体
typedef struct rcl_publisher_impl_s rcl_publisher_impl_t;

/// @brief 封装ROS发布者的结构体
typedef struct rcl_publisher_s {
  /// 指向发布者实现的指针
  rcl_publisher_impl_t* impl;
} rcl_publisher_t;

/// @brief rcl发布者可用选项
typedef struct rcl_publisher_options_s {
  /// 发布者的中间件服务质量设置
  rmw_qos_profile_t qos;
  /// 发布者的自定义分配器，用于偶发性分配
  /// 对于默认行为（malloc/free），使用：rcl_get_default_allocator()
  rcl_allocator_t allocator;
  /// rmw特定的发布者选项，例如rmw实现特定的有效载荷
  rmw_publisher_options_t rmw_publisher_options;
  /// 禁用标志给LoanedMessage，通过环境变量初始化
  bool disable_loaned_message;
} rcl_publisher_options_t;

/// @brief 返回一个rcl_publisher_t结构体，其成员设置为`NULL`
/**
 * 在传递给rcl_publisher_init()之前，应该调用此函数以获取空的rcl_publisher_t
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_publisher_t rcl_get_zero_initialized_publisher(void);

/// 初始化一个 rcl 发布者。
/**
 * 在 rcl_publisher_t 上调用此函数后，可以使用 rcl_publish() 将给定类型的消息发布到给定主题。
 *
 * 给定的 rcl_node_t 必须有效，且生成的 rcl_publisher_t 只有在给定的 rcl_node_t 保持有效时才有效。
 *
 * rosidl_message_type_support_t 是基于 .msg 类型获得的。
 * 当用户定义 ROS 消息时，会生成提供所需 rosidl_message_type_support_t 对象的代码。
 * 此对象可以使用适合语言的机制获得。
 * \todo TODO(wjwwood) 编写这些说明并链接到它
 *
 * 对于 C，可以使用宏（例如 `std_msgs/String`）：
 *
 * ```c
 * #include <rosidl_runtime_c/message_type_support_struct.h>
 * #include <std_msgs/msg/string.h>
 * const rosidl_message_type_support_t * string_ts =
 *   ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String);
 * ```
 *
 * 对于 C++，使用模板函数：
 *
 * ```cpp
 * #include <rosidl_typesupport_cpp/message_type_support.hpp>
 * #include <std_msgs/msg/string.hpp>
 * const rosidl_message_type_support_t * string_ts =
 *   rosidl_typesupport_cpp::get_message_type_support_handle<std_msgs::msg::String>();
 * ```
 *
 * rosidl_message_type_support_t 对象包含用于发布消息的消息类型特定信息。
 *
 * 主题名称必须是遵循未展开名称的主题和服务名称格式规则的 c 字符串，也称为非完全限定名称：
 *
 * \see rcl_expand_topic_name
 *
 * options
 * 结构允许用户设置服务质量设置以及在初始化/终止发布者时用于分配杂项空间（例如主题名称字符串）的自定义分配器。
 *
 * 预期用法（对于 C 消息）：
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
 * const rosidl_message_type_support_t * ts = ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, String);
 * rcl_publisher_t publisher = rcl_get_zero_initialized_publisher();
 * rcl_publisher_options_t publisher_ops = rcl_publisher_get_default_options();
 * ret = rcl_publisher_init(&publisher, &node, ts, "chatter", &publisher_ops);
 * // ... 错误处理，并在关闭时进行最后处理：
 * ret = rcl_publisher_fini(&publisher, &node);
 * // ... rcl_publisher_fini() 的错误处理
 * ret = rcl_node_fini(&node);
 * // ... rcl_deinitialize_node() 的错误处理
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
 * \param[inout] publisher 预分配的发布者结构
 * \param[in] node 有效的 rcl 节点句柄
 * \param[in] type_support 主题类型的类型支持对象
 * \param[in] topic_name 要发布的主题名称
 * \param[in] options 发布者选项，包括服务质量设置
 * \return #RCL_RET_OK 如果发布者成功初始化，或
 * \return #RCL_RET_NODE_INVALID 如果节点无效，或
 * \return #RCL_RET_ALREADY_INIT 如果发布者已经初始化，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_TOPIC_NAME_INVALID 如果给定的主题名称无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_publisher_init(
    rcl_publisher_t* publisher,
    const rcl_node_t* node,
    const rosidl_message_type_support_t* type_support,
    const char* topic_name,
    const rcl_publisher_options_t* options);

/// 结束一个 rcl_publisher_t.
/**
 * 调用后，节点将不再宣告它正在发布此主题（假设这是此主题上的唯一发布者）。
 *
 * 调用后，使用此发布者进行 rcl_publish 的调用将失败。
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
 * \param[inout] publisher 要完成的发布者句柄
 * \param[in] node 用于创建发布者的有效（未完成）节点句柄
 * \return #RCL_RET_OK 如果发布者成功完成，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_PUBLISHER_INVALID 如果发布者无效，或
 * \return #RCL_RET_NODE_INVALID 如果节点无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_publisher_fini(rcl_publisher_t* publisher, rcl_node_t* node);

/// 返回 rcl_publisher_options_t 中的默认发布者选项。
/**
 * 默认值为：
 *
 * - qos = rmw_qos_profile_default
 * - allocator = rcl_get_default_allocator()
 * - rmw_publisher_options = rmw_get_default_publisher_options()
 * - disable_loaned_message = false, 只有在 ROS_DISABLE_LOANED_MESSAGES=1 时为 true
 *
 * \return 具有默认发布者选项的结构。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_publisher_options_t rcl_publisher_get_default_options(void);

/// 借用一个已借出的消息。
/**
 * 分配给 ros 消息的内存属于中间件，除非通过调用 \sa
 * rcl_return_loaned_message_from_publisher，否则不得释放。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否 [0]
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 * [0] 底层中间件可能分配新内存或从池中返回现有块。
 *     但是，rcl 中的函数不会分配任何额外的内存。
 *
 * \param[in] publisher 分配的消息关联的发布者。
 * \param[in] type_support 内部 ros 消息分配的类型支持。
 * \param[out] ros_message 中间件将填充为有效 ros 消息的指针。
 * \return #RCL_RET_OK 如果 ros 消息正确初始化，或
 * \return #RCL_RET_PUBLISHER_INVALID 如果传入的发布者无效，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果除 ros 消息之外的参数为空，或
 * \return #RCL_RET_BAD_ALLOC 如果无法正确创建 ros 消息，或
 * \return #RCL_RET_UNSUPPORTED 如果中间件不支持该功能，或
 * \return #RCL_RET_ERROR 如果发生意外错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_borrow_loaned_message(
    const rcl_publisher_t* publisher,
    const rosidl_message_type_support_t* type_support,
    void** ros_message);

/// 归还先前从发布者借用的已借出消息。
/**
 * 传入的 ros 消息的所有权将转移回中间件。
 * 中间件可能会释放并销毁消息，因此在该调用之后，指针不再保证有效。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] publisher 已借出消息关联的发布者。
 * \param[in] loaned_message 要释放和销毁的已借出消息。
 * \return #RCL_RET_OK 如果成功，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果参数为空，或
 * \return #RCL_RET_UNSUPPORTED 如果中间件不支持该功能，或
 * \return #RCL_RET_PUBLISHER_INVALID 如果发布者无效，或
 * \return #RCL_RET_ERROR 如果发生意外错误且无法初始化消息。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_return_loaned_message_from_publisher(
    const rcl_publisher_t* publisher, void* loaned_message);

/// 使用发布器在主题上发布ROS消息。
/**
 * 调用者需要确保 ros_message 参数的类型与发布器关联的类型（通过类型支持）匹配。
 * 将不同类型传递给 publish 会产生未定义的行为，此函数无法检查，因此不会出现故意的错误。
 *
 * \todo TODO(wjwwood):
 *   publish 的阻塞行为仍然存在争议。
 *   一旦行为明确定义，此部分应进行更新。
 *   参见：https://github.com/ros2/ros2/issues/255
 *
 * 调用 rcl_publish() 是一个可能阻塞的调用。
 * 当调用 rcl_publish() 时，它将立即执行任何与发布相关的工作，
 * 包括但不限于将消息转换为其他类型、序列化消息、收集发布统计信息等。
 * 它最后要做的事情是调用底层中间件的发布函数，该函数可能会根据通过 rcl_publisher_init()
 * 中的发布器选项给出的服务质量设置阻塞或不阻塞。
 * 例如，如果可靠性设置为可靠，则发布可能会阻塞，直到发布队列中有空间可用；但如果可靠性设置为尽力而为，则不应阻塞。
 *
 * 由 `ros_message` void 指针给出的 ROS 消息始终由调用代码拥有，但在发布期间应保持不变。
 *
 * 只要对发布器和 `ros_message` 的访问得到同步，此函数就是线程安全的。
 * 这意味着允许从多个线程调用 rcl_publish()，但与非线程安全发布器函数同时调用 rcl_publish()
 * 是不允许的，例如并发调用 rcl_publish() 和 rcl_publisher_fini() 是不允许的。 在 rcl_publish()
 * 调用期间，消息不能更改。 在调用 rcl_publish() 之前，消息可以更改，但在调用 rcl_publish()
 * 之后，它取决于 RMW 实现行为。 同一个 `ros_message` 可以同时传递给多个 rcl_publish()
 * 调用，即使发布器不同。 `ros_message` 不会被 rcl_publish() 修改。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 是 [1]
 * 使用原子操作        | 否
 * 无锁                | 是
 * <i>[1] 对于唯一的发布器和消息对，请参见上文了解更多信息</i>
 *
 * \param[in] publisher 将执行发布操作的发布器句柄
 * \param[in] ros_message 指向ROS消息的类型擦除指针
 * \param[in] allocation 结构指针，用于内存预分配（可以为NULL）
 * \return #RCL_RET_OK 如果消息发布成功，或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或者
 * \return #RCL_RET_PUBLISHER_INVALID 如果发布器无效，或者
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_publish(
    const rcl_publisher_t* publisher,
    const void* ros_message,
    rmw_publisher_allocation_t* allocation);

/// 使用发布者在主题上发布序列化消息。
/**
 * 调用者需要确保序列化消息的类型与发布者关联的类型（通过类型支持）匹配。
 * 尽管此调用要发布的已经序列化的消息，但发布者必须将其类型注册为ROS已知的消息类型。
 * 传递来自不同类型的序列化消息会导致订阅者端的未定义行为。
 * 发布调用可能能够发送任意序列化消息，但不能保证订阅者端成功反序列化此字节流。
 *
 * 除此之外，`publish_serialized` 函数与 rcl_publish() 具有相同的行为，
 * 只是不进行序列化步骤。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是 [1]
 * 使用原子操作      | 否
 * 无锁              | 是
 * <i>[1] 对于唯一的发布者和消息对，请参阅上述更多信息</i>
 *
 * \param[in] publisher 将执行发布操作的发布者句柄
 * \param[in] serialized_message 指向已序列化消息的原始形式的指针
 * \param[in] allocation 结构指针，用于内存预分配（可以为NULL）
 * \return #RCL_RET_OK 如果消息发布成功，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_PUBLISHER_INVALID 如果发布者无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_publish_serialized_message(
    const rcl_publisher_t* publisher,
    const rcl_serialized_message_t* serialized_message,
    rmw_publisher_allocation_t* allocation);

/// 使用发布者在主题上发布借用的消息。
/**
 * 通过此调用 rcl_publish_loaned_message() 可以发送先前借用的借用消息。
 * 调用此函数后，借用消息的所有权将转移回中间件。
 * `ros_message` 指针在此之后可能无效，因为中间件可能会在内部释放此消息的内存。
 * 因此，建议仅与 \sa rcl_borrow_loaned_message() 结合使用此函数。
 *
 * 除此之外，`publish_loaned_message` 函数与 rcl_publish() 具有相同的行为，
 * 只是不进行序列化步骤。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否 [0]
 * 线程安全          | 是 [1]
 * 使用原子操作      | 否
 * 无锁              | 是
 * <i>[0] 中间件可能会释放借用的消息。但 RCL 函数不分配任何内存。</i>
 * <i>[1] 对于唯一的发布者和消息对，请参阅上述更多信息</i>
 *
 * \param[in] publisher 将执行发布操作的发布者句柄
 * \param[in] ros_message 指向先前借用的借用消息的指针
 * \param[in] allocation 结构指针，用于内存预分配（可以为NULL）
 * \return #RCL_RET_OK 如果消息发布成功，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_PUBLISHER_INVALID 如果发布者无效，或
 * \return #RCL_RET_UNSUPPORTED 如果中间件不支持该功能，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_publish_loaned_message(
    const rcl_publisher_t* publisher, void* ros_message, rmw_publisher_allocation_t* allocation);

/// 手动声明此发布者是活跃的（对于 RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC）
/**
 * 如果 rmw Liveliness 策略设置为 RMW_QOS_POLICY_LIVELINESS_MANUAL_BY_TOPIC，则此发布者的创建者
 * 可以在某个时间点手动调用 `assert_liveliness`，以向系统的其余部分发出该节点仍然活跃的信号。
 * 此函数必须至少与 qos_profile 的 liveliness_lease_duration 一样频繁地调用。
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] publisher 需要声明活跃性的发布者句柄
 * \return #RCL_RET_OK 如果活跃性声明成功完成，或
 * \return #RCL_RET_PUBLISHER_INVALID 如果发布者无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_publisher_assert_liveliness(const rcl_publisher_t* publisher);

/// 等待所有已发布的消息数据被确认或直到指定的超时时间过去。
/**
 * 此函数等待所有已发布的消息数据被对等节点确认或超时。
 *
 * 超时单位为纳秒。
 * 如果超时为负，则此函数将无限期阻塞，直到所有已发布的消息数据被确认。
 * 如果超时为 0，则此函数将非阻塞；检查所有已发布的消息数据是否被确认（如果确认，则返回
 * RCL_RET_OK。否则，返回 RCL_RET_TIMEOUT），但 不等待。 如果超时大于
 * 0，则此函数将在该时间段过去后返回（返回 RCL_RET_TIMEOUT）或所有已发布的消息数据被确认（返回
 * RCL_RET_OK）。
 *
 * 只有当发布者的 QOS 配置文件为 RELIABLE 时，此函数才会等待确认。
 * 否则，此函数将立即返回 RCL_RET_OK。
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 否
 * 无锁              | 否
 *
 * \param[in] publisher 需要等待所有确认的发布者句柄。
 * \param[in] timeout 等待所有已发布消息数据被确认的持续时间，以纳秒为单位。
 * \return #RCL_RET_OK 如果成功，或
 * \return #RCL_RET_TIMEOUT 如果超时，或
 * \return #RCL_RET_PUBLISHER_INVALID 如果发布者无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误，或
 * \return #RCL_RET_UNSUPPORTED 如果中间件不支持该功能。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_publisher_wait_for_all_acked(
    const rcl_publisher_t* publisher, rcl_duration_value_t timeout);

/// 获取发布者的主题名称。
/**
 * 此函数返回发布者的内部主题名称字符串。
 * 如果以下情况之一成立，此函数可能会失败，并因此返回 `NULL`：
 *   - 发布者为 `NULL`
 *   - 发布者无效（从未调用 init、调用 fini 或节点无效）
 *
 * 只要 rcl_publisher_t 有效，返回的字符串就有效。
 * 如果主题名称发生更改，字符串的值可能会更改，因此，如果这是一个问题，请建议复制字符串。
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] publisher 指向发布者的指针
 * \return 成功时的名称字符串，否则为 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const char* rcl_publisher_get_topic_name(const rcl_publisher_t* publisher);

/// 返回 rcl 发布者选项。
/**
 * 此函数返回发布者的内部选项结构体。
 * 如果以下情况之一成立，此函数可能会失败，并因此返回 `NULL`：
 *   - 发布者为 `NULL`
 *   - 发布者无效（从未调用 init、调用 fini 或节点无效）
 *
 * 只要 rcl_publisher_t 有效，返回的结构体就有效。
 * 如果发布者选项发生更改，结构体中的值可能会更改，因此，如果这是一个问题，请建议复制结构体。
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] publisher 指向发布者的指针
 * \return 成功时的选项结构体，否则为 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const rcl_publisher_options_t* rcl_publisher_get_options(const rcl_publisher_t* publisher);

/// 返回rmw发布者句柄。
/**
 * 返回的句柄是指向内部持有的rmw句柄的指针。
 * 此函数可能失败，因此返回 `NULL`，如果：
 *   - 发布者为 `NULL`
 *   - 发布者无效（未调用init、调用fini或无效节点）
 *
 * 如果发布者被终止或调用rcl_shutdown()，返回的句柄将变为无效。
 * 由于发布者可能被终止并重新创建，所以不能保证返回的句柄在发布者的生命周期内一直有效。
 * 因此建议每次需要句柄时都使用此函数从发布者获取句柄，并避免与可能更改句柄的函数同时使用句柄。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] publisher 指向rcl发布者的指针
 * \return 成功时返回rmw发布者句柄，否则返回 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rmw_publisher_t* rcl_publisher_get_rmw_handle(const rcl_publisher_t* publisher);

/// 返回与此发布者关联的上下文。
/**
 * 此函数可能失败，因此返回 `NULL`，如果：
 *   - 发布者为 `NULL`
 *   - 发布者无效（未调用init、调用fini等）
 *
 * 如果发布者被终止或调用rcl_shutdown()，返回的上下文将变为无效。
 * 因此建议每次需要句柄时都使用此函数从发布者获取句柄，并避免与可能更改句柄的函数同时使用句柄。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] publisher 指向rcl发布者的指针
 * \return 成功时返回上下文，否则返回 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_context_t* rcl_publisher_get_context(const rcl_publisher_t* publisher);

/// 如果发布者有效，则返回true，否则返回false。
/**
 * 如果 `publisher` 无效，则返回的布尔值为 `false`。
 * 否则，返回的布尔值为 `true`。
 * 在返回 `false` 的情况下，会设置错误消息。
 * 此函数不能失败。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] publisher 指向rcl发布者的指针
 * \return 如果 `publisher` 有效，则返回 `true`，否则返回 `false`
 */
RCL_PUBLIC
bool rcl_publisher_is_valid(const rcl_publisher_t* publisher);

/// 如果发布者有效（除上下文外），则返回true，否则返回false。
/**
 * 这用于清理函数，这些函数需要访问发布者，但不需要使用任何与上下文相关的函数。
 *
 * 它与rcl_publisher_is_valid相同，只是它忽略了与发布者关联的上下文的状态。
 * \sa rcl_publisher_is_valid()
 */
RCL_PUBLIC
bool rcl_publisher_is_valid_except_context(const rcl_publisher_t* publisher);

/// 获取与发布者匹配的订阅数量。
/**
 * 用于获取与发布者匹配的内部订阅计数。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 是
 * 使用原子操作        | 可能 [1]
 * 无锁                | 可能 [1]
 * <i>[1] 只有在底层rmw没有使用此功能的情况下才适用 </i>
 *
 * \param[in] publisher 指向rcl发布者的指针
 * \param[out] subscription_count 匹配订阅的数量
 * \return #RCL_RET_OK 如果计数已检索，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_PUBLISHER_INVALID 如果发布者无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_publisher_get_subscription_count(
    const rcl_publisher_t* publisher, size_t* subscription_count);

/// 获取发布者的实际qos设置。
/**
 * 用于获取发布者的实际qos设置。
 * 当使用RMW_*_SYSTEM_DEFAULT时，只有在创建发布者之后才能解析实际配置，
 * 并且它取决于底层rmw实现。
 * 如果正在使用的底层设置无法用ROS术语表示，则将其设置为RMW_*_UNKNOWN。
 * 返回的结构仅在rcl_publisher_t有效时有效。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 是
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] publisher 指向rcl发布者的指针
 * \return 成功时返回qos结构，否则返回 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const rmw_qos_profile_t* rcl_publisher_get_actual_qos(const rcl_publisher_t* publisher);

/// 检查发布者实例是否可以借用消息。
/**
 * 根据中间件和消息类型，如果中间件可以分配ROS消息实例，这将返回true。
 */
RCL_PUBLIC
bool rcl_publisher_can_loan_messages(const rcl_publisher_t* publisher);

#ifdef __cplusplus
}
#endif

#endif  // RCL__PUBLISHER_H_
