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

#ifndef RCL__NODE_H_
#define RCL__NODE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include "rcl/allocator.h"
#include "rcl/arguments.h"
#include "rcl/context.h"
#include "rcl/guard_condition.h"
#include "rcl/macros.h"
#include "rcl/node_options.h"
#include "rcl/types.h"
#include "rcl/visibility_control.h"

/// \file rcl_node.h

// ...

extern const char * const RCL_DISABLE_LOANED_MESSAGES_ENV_VAR;

typedef struct rcl_node_impl_s rcl_node_impl_t;

/// 封装ROS节点的结构体
typedef struct rcl_node_s
{
  /// 与此节点关联的上下文
  rcl_context_t * context;

  /// 私有实现指针
  rcl_node_impl_t * impl;
} rcl_node_t;

/// 返回一个rcl_node_t结构体，其成员初始化为`NULL`
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_node_t rcl_get_zero_initialized_node(void);

/// 初始化一个ROS节点
/**
 * 在rcl_node_t上调用此函数会使其成为一个有效的节点句柄，直到调用rcl_shutdown或在其上调用rcl_node_fini。
 *
 * 调用后，可以使用ROS节点对象创建其他中间件原语，如发布者、服务、参数等。
 *
 * 节点的名称不能为空，并且必须符合命名限制，请参阅rmw_validate_node_name()函数了解规则。
 *
 * \todo TODO(wjwwood): 目前尚未强制执行节点名称的唯一性
 *
 * 同名的节点不能与另一个同名节点重合。如果域中已经存在同名节点，它将被关闭。
 *
 * 节点的命名空间不能为空，还应遵循rmw_validate_namespace()函数的规则。
 *
 * 此外，此函数允许缺少前导正斜杠的命名空间。因为没有相对命名空间的概念，所以缺少正斜杠的命名空间与带有正斜杠的相同命名空间之间没有区别。因此，像``"foo/bar"``这样的命名空间会被此函数自动更改为``"/foo/bar"``。类似地，命名空间``""``将隐式变为``"/"``，这是一个有效的命名空间。
 *
 * \todo TODO(wjwwood):
 *   参数基础结构目前在特定于语言的客户端库中初始化，例如C++的rclcpp，但将来将在此处初始化。当发生这种情况时，rcl_node_options_t结构中将有一个选项来避免参数基础结构。
 *
 * 节点包含ROS参数的基础设施，其中包括广告发布者和服务服务器。即使稍后不使用参数，此函数也会创建这些外部参数接口。
 *
 * 给定的rcl_node_t必须分配并初始化为零。在已经调用过此函数的rcl_node_t上调用此函数，比rcl_node_fini更近，将失败。分配了rcl_node_t但未初始化内存的行为是未定义的。
 *
 * 预期用法：
 *
 * ```c
 * rcl_context_t context = rcl_get_zero_initialized_context();
 * // ... 使用rcl_init()初始化上下文
 * rcl_node_t node = rcl_get_zero_initialized_node();
 * rcl_node_options_t node_ops = rcl_node_get_default_options();
 * // ... 节点选项定制
 * rcl_ret_t ret = rcl_node_init(&node, "node_name", "/node_ns", &context, &node_ops);
 * // ... 错误处理，然后使用节点，但最终取消初始化：
 * ret = rcl_node_fini(&node);
 * // ... rcl_node_fini()的错误处理
 * ```
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存           | 是
 * 线程安全           | 否
 * 使用原子操作       | 是
 * 无锁               | 是 [1]
 * <i>[1] 如果`atomic_is_lock_free()`对`atomic_uint_least64_t`返回true</i>
 *
 * \pre 节点句柄必须分配、零初始化且无效
 * \pre 上下文句柄必须分配、初始化且有效
 * \post 节点句柄有效，可以在其他`rcl_*`函数中使用
 *
 * \param[inout] node 预先分配的rcl_node_t
 * \param[in] name 节点名称，必须是有效的c字符串
 * \param[in] namespace_ 节点的命名空间，必须是有效的c字符串
 * \param[in] context 节点应与之关联的上下文实例
 * \param[in] options 节点选项。
 *   选项将深度复制到节点中。
 *   调用者始终负责释放他们传入的选项所使用的内存。
 * \return #RCL_RET_OK 如果节点成功初始化，或
 * \return #RCL_RET_ALREADY_INIT 如果节点已经初始化，或
 * \return #RCL_RET_NOT_INIT 如果给定的上下文无效，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_NODE_INVALID_NAME 如果名称无效，或
 * \return #RCL_RET_NODE_INVALID_NAMESPACE 如果namespace_无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_node_init(
  rcl_node_t * node, const char * name, const char * namespace_, rcl_context_t * context,
  const rcl_node_options_t * options);

/// 结束一个rcl_node_t
/**
 * 销毁任何自动创建的基础设施并释放内存。调用后，可以安全地取消分配rcl_node_t。
 *
 * 用户创建的所有中间件原语（例如发布者、服务等），它们是从此节点创建的，必须在调用此函数之前使用它们各自的`rcl_*_fini()`函数完成。
 * \sa rcl_publisher_fini()
 * \sa rcl_subscription_fini()
 * \sa rcl_client_fini()
 * \sa rcl_service_fini()
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存           | 是
 * 线程安全           | 否
 * 使用原子操作       | 是
 * 无锁               | 是 [1]
 * <i>[1] 如果`atomic_is_lock_free()`对`atomic_uint_least64_t`返回true</i>
 *
 * \param[in] node 要结束的rcl_node_t
 * \return #RCL_RET_OK 如果节点成功完成，或
 * \return #RCL_RET_NODE_INVALID 如果节点指针为空，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_node_fini(rcl_node_t * node);

/// 返回节点是否有效，如果有效返回 `true`，否则返回 `false`。
/**
 * 如果节点指针为 `NULL` 或分配器无效，也返回 `false`。
 *
 * 节点无效的情况：
 *   - 实现为 `NULL`（未调用 rcl_node_init 或调用失败）
 *   - 自节点初始化以来已调用 rcl_shutdown
 *   - 使用 rcl_node_fini 对节点进行了终止处理
 *
 * 存在可能的有效性竞争条件。
 *
 * 考虑：
 *
 * ```c
 * assert(rcl_node_is_valid(node));  // <-- 线程 1
 * rcl_shutdown();                   // <-- 线程 2
 * // 假设节点有效地使用节点           // <-- 线程 1
 * ```
 *
 * 在第三行中，尽管线程 1 的前一行检查到节点有效，但节点现在已经无效。
 * 这就是为什么这个函数被认为是非线程安全的。
 *
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 是
 * 无锁                | 是 [1]
 * <i>[1] 如果 `atomic_is_lock_free()` 返回 true，则针对 `atomic_uint_least64_t`</i>
 *
 * \param[in] node 要验证的 rcl_node_t
 * \return 如果节点和分配器有效，则返回 `true`，否则返回 `false`。
 */
RCL_PUBLIC
bool rcl_node_is_valid(const rcl_node_t * node);

/// 如果节点有效，则返回 true，除非上下文有效。
/**
 * 这用于清理函数，这些函数需要访问节点，但不需要使用与上下文相关的任何函数。
 *
 * 它与 rcl_node_is_valid 相同，只是它忽略了与节点关联的上下文的状态。
 * \sa rcl_node_is_valid()
 */
RCL_PUBLIC
bool rcl_node_is_valid_except_context(const rcl_node_t * node);

/// 返回节点的名称。
/**
 * 此函数返回节点的内部名称字符串。
 * 如果以下情况之一发生，此函数可能失败，因此返回 `NULL`：
 *   - 节点为 `NULL`
 *   - 节点尚未初始化（实现无效）
 *
 * 只要给定的 rcl_node_t 有效，返回的字符串就有效。
 * 如果 rcl_node_t 中的值发生变化，字符串的值可能会发生变化，
 * 因此，如果这是一个问题，建议复制字符串。
 *
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] node 指向节点的指针
 * \return 如果成功，则返回名称字符串，否则返回 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const char * rcl_node_get_name(const rcl_node_t * node);

/// 返回节点的命名空间。
/**
 * 此函数返回节点的内部命名空间字符串。
 * 如果以下情况之一发生，此函数可能失败，因此返回 `NULL`：
 *   - 节点为 `NULL`
 *   - 节点尚未初始化（实现无效）
 *
 * 只要给定的 rcl_node_t 有效，返回的字符串就有效。
 * 如果 rcl_node_t 中的值发生变化，字符串的值可能会发生变化，
 * 因此，如果这是一个问题，建议复制字符串。
 *
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] node 指向节点的指针
 * \return 如果成功，则返回名称字符串，否则返回 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const char * rcl_node_get_namespace(const rcl_node_t * node);

/// 返回节点的完全限定名称。
/**
 * 此函数返回节点的内部命名空间和名称组合字符串。
 * 如果以下情况之一发生，此函数可能失败，因此返回 `NULL`：
 *   - 节点为 `NULL`
 *   - 节点尚未初始化（实现无效）
 *
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] node 指向节点的指针
 * \return 如果成功，则返回完全限定名称字符串，否则返回 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const char * rcl_node_get_fully_qualified_name(const rcl_node_t * node);

/// 返回 rcl 节点选项。
/**
 * 此函数返回节点的内部选项结构体。
 * 如果以下情况之一发生，此函数可能失败，因此返回 `NULL`：
 *   - 节点为 `NULL`
 *   - 节点尚未初始化（实现无效）
 *
 * 只要给定的 rcl_node_t 有效，返回的结构体就有效。
 * 如果 rcl_node_t 的选项发生变化，结构体中的值可能会发生变化，
 * 因此，如果这是一个问题，建议复制结构体。
 *
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] node 指向节点的指针
 * \return 如果成功，则返回选项结构体，否则返回 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const rcl_node_options_t * rcl_node_get_options(const rcl_node_t * node);

/// 返回节点所使用的ROS域ID。
/**
 * 此函数返回节点所在的ROS域ID。
 *
 * 应使用此函数来确定使用了哪个`domain_id`，
 * 而不是检查节点选项中的domain_id字段，因为如果在创建节点时使用
 * #RCL_NODE_OPTIONS_DEFAULT_DOMAIN_ID，则在创建后不会更改，
 * 但此函数将返回实际使用的`domain_id`。
 *
 * `domain_id`字段必须指向一个已分配的`size_t`对象，用于写入ROS域ID。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] node 正在查询的节点句柄
 * \param[out] domain_id 存储域id的位置
 * \return #RCL_RET_OK 如果成功获取节点的域ID，或者
 * \return #RCL_RET_NODE_INVALID 如果节点无效，或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或者
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_node_get_domain_id(const rcl_node_t * node, size_t * domain_id);

/// 返回rmw节点句柄。
/**
 * 返回的句柄是指向内部持有的rmw句柄的指针。
 * 此函数可能失败，因此返回`NULL`，如果：
 *   - 节点为`NULL`
 *   - 节点尚未初始化（实现无效）
 *
 * 如果节点被终止或调用rcl_shutdown()，返回的句柄将变为无效。
 * 返回的句柄在节点的生命周期内不能保证有效，因为它可能被终止并重新创建。
 * 因此，建议每次需要时使用此函数从节点获取句柄，并避免与可能更改它的函数同时使用句柄。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] node 指向rcl节点的指针
 * \return 成功时返回rmw节点句柄，否则返回`NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rmw_node_t * rcl_node_get_rmw_handle(const rcl_node_t * node);

/// 返回关联的rcl实例id。
/**
 * 当调用rcl_node_init时，此id会被存储，可以通过与
 * rcl_get_instance_id()返回的值进行比较，以检查此节点是否在当前的
 * rcl上下文中创建（自上次调用rcl_init()以来）。
 *
 * 此函数可能失败，因此返回`0`，如果：
 *   - 节点为`NULL`
 *   - 节点尚未初始化（实现无效）
 *
 * 即使在创建节点后调用了rcl_shutdown()，此函数也会成功。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] node 指向rcl节点的指针
 * \return 在节点初始化期间捕获的rcl实例id，错误时返回`0`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
uint64_t rcl_node_get_rcl_instance_id(const rcl_node_t * node);

/// 返回一个触发器条件，当ROS图发生变化时触发。
/**
 * 返回的句柄是指向内部持有的rcl触发器条件的指针。
 * 此函数可能失败，因此返回`NULL`，如果：
 *   - 节点为`NULL`
 *   - 节点无效
 *
 * 如果节点被终止或调用rcl_shutdown()，返回的句柄将变为无效。
 *
 * 触发器条件将在ROS图发生任何更改时触发。
 * ROS图的更改包括（但不限于）新发布者宣传、创建新订阅、提供新服务、
 * 取消订阅等。
 *
 * \todo TODO(wjwwood): 链接到图事件的详尽列表
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] node 指向rcl节点的指针
 * \return 成功时返回rcl触发器条件句柄，否则返回`NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const rcl_guard_condition_t * rcl_node_get_graph_guard_condition(const rcl_node_t * node);

/// 返回节点的日志记录器名称。
/**
 * 此函数返回节点内部日志记录器名称字符串。
 * 此函数可能失败，因此返回`NULL`，如果：
 *   - 节点为`NULL`
 *   - 节点尚未初始化（实现无效）
 *
 * 只要给定的rcl_node_t有效，返回的字符串就有效。
 * 如果rcl_node_t中的值发生变化，字符串的值可能会发生变化，
 * 因此，如果这是一个问题，请建议复制字符串。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] node 指向节点的指针
 * \return 成功时返回logger_name字符串，否则返回`NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const char * rcl_node_get_logger_name(const rcl_node_t * node);

/// 将给定名称扩展为完全限定的主题名称，并应用重映射规则。
/**
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] node 节点对象。使用其名称、命名空间、本地/全局命令行参数。
 * \param[in] input_name 要扩展和重映射的主题名称。
 * \param[in] allocator 创建输出主题时使用的分配器。
 * \param[in] is_service 对于服务使用`true`，对于主题使用`false`。
 * \param[in] only_expand 当`true`时，忽略重映射规则。
 * \param[out] output_name 输出char *指针。
 * \return #RCL_RET_OK 如果主题名称扩展成功，或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果input_name、node_name、node_namespace
 *  或output_name为NULL，或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果local_args和global_args都为NULL，或者
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或者
 * \return #RCL_RET_TOPIC_NAME_INVALID 如果给定的主题名称无效
 *  (参见rcl_validate_topic_name())，或者
 * \return #RCL_RET_NODE_INVALID_NAME 如果给定的节点名称无效
 *  (参见rmw_validate_node_name())，或者
 * \return #RCL_RET_NODE_INVALID_NAMESPACE 如果给定的节点命名空间无效
 *  (参见rmw_validate_namespace())，或者
 * \return #RCL_RET_UNKNOWN_SUBSTITUTION 对于名称中未知的替换，或者
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_node_resolve_name(
  const rcl_node_t * node, const char * input_name, rcl_allocator_t allocator, bool is_service,
  bool only_expand, char ** output_name);

/// 根据环境变量检查是否禁用了借用消息。
/**
 * 如果 `ROS_DISABLE_LOANED_MESSAGES` 环境变量设置为 "1"，
 * `disable_loaned_message` 将被设置为 true。
 *
 * \param[out] disable_loaned_message 不能为空（Must not be NULL）。
 * \return #RCL_RET_INVALID_ARGUMENT 如果参数无效，或
 * \return #RCL_RET_ERROR 如果发生意外错误，或
 * \return #RCL_RET_OK。
 */
RCL_PUBLIC
rcl_ret_t rcl_get_disable_loaned_message(bool * disable_loaned_message);

#ifdef __cplusplus
}
#endif

#endif  // RCL__NODE_H_
