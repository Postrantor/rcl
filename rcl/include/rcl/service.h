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

/// @file

#ifndef RCL__SERVICE_H_
#define RCL__SERVICE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/allocator.h"
#include "rcl/event_callback.h"
#include "rcl/macros.h"
#include "rcl/node.h"
#include "rcl/publisher.h"
#include "rcl/service_introspection.h"
#include "rcl/time.h"
#include "rcl/visibility_control.h"
#include "rmw/types.h"
#include "rosidl_runtime_c/service_type_support_struct.h"

/// 内部 rcl 实现结构体.
typedef struct rcl_service_impl_s rcl_service_impl_t;

/// 封装 ROS 服务的结构体.
typedef struct rcl_service_s
{
  /// 指向服务实现的指针
  rcl_service_impl_t * impl;
} rcl_service_t;

/// rcl 服务可用选项.
typedef struct rcl_service_options_s
{
  /// 服务的中间件质量服务设置.
  rmw_qos_profile_t qos;
  /// 用于偶发分配的服务自定义分配器.
  /** 默认行为（malloc/free）参见：rcl_get_default_allocator() */
  rcl_allocator_t allocator;
} rcl_service_options_t;

/// 返回一个成员设置为 `NULL` 的 rcl_service_t 结构体.
/**
 * 在传递给 rcl_service_init() 之前，应该调用此函数以获取空的 rcl_service_t。
 *
 * \return 一个零初始化服务的结构体.
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_service_t rcl_get_zero_initialized_service(void);

/// 初始化一个rcl服务。
/**
 * 在rcl_service_t上调用此函数后，可以使用rcl_take_request()处理给定类型的请求。
 * 它还可以使用rcl_send_response()向请求发送响应。
 *
 * 给定的rcl_node_t必须有效，且只有在给定的rcl_node_t保持有效时，结果rcl_service_t才有效。
 *
 * rosidl_service_type_support_t是基于每个.srv类型获得的。
 * 当用户定义一个ROS服务时，会生成提供所需rosidl_service_type_support_t对象的代码。
 * 可以使用适合语言的机制获取此对象。
 * \todo TODO(wjwwood) 编写一次这些说明并链接到它
 *
 * 对于C，可以使用宏（例如`example_interfaces/AddTwoInts`）：
 *
 * ```c
 * #include <rosidl_runtime_c/service_type_support_struct.h>
 * #include <example_interfaces/srv/add_two_ints.h>
 * const rosidl_service_type_support_t * ts =
 *   ROSIDL_GET_SRV_TYPE_SUPPORT(example_interfaces, srv, AddTwoInts);
 * ```
 *
 * 对于C++，使用模板函数：
 *
 * ```cpp
 * #include <rosidl_runtime_cpp/service_type_support.hpp>
 * #include <example_interfaces/srv/add_two_ints.h>
 * using rosidl_typesupport_cpp::get_service_type_support_handle;
 * const rosidl_service_type_support_t * ts =
 *   get_service_type_support_handle<example_interfaces::srv::AddTwoInts>();
 * ```
 *
 * rosidl_service_type_support_t对象包含用于发送或接收请求和响应的服务类型特定信息。
 *
 * 主题名称必须是遵循未展开名称的主题和服务名称格式规则的c字符串，也称为非完全限定名称：
 *
 * \see rcl_expand_topic_name
 *
 * options结构允许用户设置服务质量设置以及在初始化/终止服务时用于分配杂项空间（例如服务名称字符串）的自定义分配器。
 *
 * 预期用法（对于C服务）：
 *
 * ```c
 * #include <rcl/rcl.h>
 * #include <rosidl_runtime_c/service_type_support_struct.h>
 * #include <example_interfaces/srv/add_two_ints.h>
 *
 * rcl_node_t node = rcl_get_zero_initialized_node();
 * rcl_node_options_t node_ops = rcl_node_get_default_options();
 * rcl_ret_t ret = rcl_node_init(&node, "node_name", "/my_namespace", &node_ops);
 * // ... 错误处理
 * const rosidl_service_type_support_t * ts =
 *   ROSIDL_GET_SRV_TYPE_SUPPORT(example_interfaces, srv, AddTwoInts);
 * rcl_service_t service = rcl_get_zero_initialized_service();
 * rcl_service_options_t service_ops = rcl_service_get_default_options();
 * ret = rcl_service_init(&service, &node, ts, "add_two_ints", &service_ops);
 * // ... 错误处理，并在关闭时进行最后处理：
 * ret = rcl_service_fini(&service, &node);
 * // ... rcl_service_fini()的错误处理
 * ret = rcl_node_fini(&node);
 * // ... rcl_node_fini()的错误处理
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
 * \param[out] service 预分配的服务结构
 * \param[in] node 有效的rcl节点句柄
 * \param[in] type_support 服务类型的类型支持对象
 * \param[in] service_name 服务名称
 * \param[in] options 包括服务质量设置的服务选项
 * \return #RCL_RET_OK 如果服务成功初始化，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ALREADY_INIT 如果服务已经初始化，或
 * \return #RCL_RET_NODE_INVALID 如果节点无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_SERVICE_NAME_INVALID 如果给定的服务名称无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_service_init(
  rcl_service_t * service, const rcl_node_t * node,
  const rosidl_service_type_support_t * type_support, const char * service_name,
  const rcl_service_options_t * options);

/// 结束一个 rcl_service_t.
/**
 * 调用后，节点将不再监听此服务的请求。
 * （假设这是此节点中此类型的唯一服务）。
 *
 * 调用后，使用此服务时，rcl_wait()、rcl_take_request() 和
 * rcl_send_response() 将失败。
 * 此外，如果当前阻塞，rcl_wait() 也会被中断。
 * 然而，给定的节点句柄仍然有效。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[inout] service 要取消初始化的服务句柄
 * \param[in] node 用于创建服务的有效（未完成）节点句柄
 * \return #RCL_RET_OK 如果服务成功取消初始化, 或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效, 或者
 * \return #RCL_RET_SERVICE_INVALID 如果服务无效, 或者
 * \return #RCL_RET_NODE_INVALID 如果节点无效, 或者
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_service_fini(rcl_service_t * service, rcl_node_t * node);

/// 返回 rcl_service_options_t 中的默认服务选项。
/**
 * 默认值为：
 *
 * - qos = rmw_qos_profile_services_default
 * - allocator = rcl_get_default_allocator()
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_service_options_t rcl_service_get_default_options(void);

/// 使用 rcl 服务获取一个挂起的 ROS 请求。
/**
 * 调用者有责任确保 ros_request 参数的类型与
 * 通过类型支持关联的服务类型匹配。
 * 将不同类型传递给 rcl_take 会产生未定义的行为，这不能
 * 由此函数检查，因此不会发生故意的错误。
 *
 * TODO(jacquelinekay) 获取阻塞？
 * TODO(jacquelinekay) 消息所有权的前置条件、过程中和后置条件？
 * TODO(jacquelinekay) rcl_take_request 是否线程安全？
 * TODO(jacquelinekay) 应该有一个 rcl_request_id_t 吗？
 *
 * ros_request 指针应指向一个已分配的正确类型的 ROS 请求消息
 * 结构，如果有可用的请求，则将获取到的 ROS 请求复制到其中。
 * 如果调用后 taken 为 false，则 ROS 请求将保持不变。
 *
 * 如果在获取请求时需要分配内存，例如，如果需要为目标消息中的动态大小数组分配空间，
 * 则使用服务选项中给定的分配器。
 *
 * request_header 是指向预先分配的包含请求元信息（如序列号）的 rmw 结构的指针。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 可能 [1]
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 * <i>[1] 仅在填充请求时需要时分配内存，对于固定大小避免使用</i>
 *
 * \param[in] service 要获取的服务句柄
 * \param[inout] request_header 指向保存请求元数据的结构的指针
 * \param[inout] ros_request 指向已分配的 ROS 请求消息的类型擦除指针
 * \return #RCL_RET_OK 如果请求被获取, 或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效, 或者
 * \return #RCL_RET_SERVICE_INVALID 如果服务无效, 或者
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败, 或者
 * \return #RCL_RET_SERVICE_TAKE_FAILED 如果获取失败但中间件中没有错误发生, 或者
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_take_request_with_info(
  const rcl_service_t * service, rmw_service_info_t * request_header, void * ros_request);

/// 用于向后兼容的函数，通过 rcl 服务获取待处理的 ROS 请求。
/**
 * 此版本仅接受请求 ID。有关此操作的完整说明，请参阅 rcl_take_request_with_info()。
 *
 * \param[in] service 要获取请求的服务句柄
 * \param[inout] request_header 指向保存请求 id 的结构体的指针
 * \param[inout] ros_request 指向已分配的 ROS 请求消息的类型擦除指针
 * \return #RCL_RET_OK 如果请求被成功获取，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_SERVICE_INVALID 如果服务无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_SERVICE_TAKE_FAILED 如果获取失败但中间件中没有错误发生，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_take_request(
  const rcl_service_t * service, rmw_request_id_t * request_header, void * ros_request);

/// 使用服务将 ROS 响应发送给客户端。
/**
 * 调用者需要确保 `ros_response` 参数的类型与服务关联的类型（通过类型支持）匹配。
 * 将不同类型传递给 send_response 会产生未定义的行为，并且此函数无法检查，因此不会出现故意的错误。
 *
 * send_response() 是一个非阻塞调用。
 *
 * 由 `ros_response` void 指针给出的 ROS 响应消息始终由调用代码拥有，但在
 * rcl_send_response()期间应保持不变。
 *
 * 只要对服务和 `ros_response` 的访问得到同步，此函数就是线程安全的。
 * 这意味着允许从多个线程调用 rcl_send_response()，但与非线程安全的服务函数同时调用 rcl_send_response() 是不允许的，例如并发调用 rcl_send_response() 和 rcl_service_fini() 是不允许的。
 * 在 rcl_send_response() 调用期间，消息不能更改。
 * 在调用 rcl_send_response() 之前，消息可以更改，但在调用 rcl_send_response() 之后，它取决于 RMW 实现行为。
 * 同一个 `ros_response` 可以同时传递给多个 rcl_send_response() 调用，即使服务不同。
 * rcl_send_response() 不会修改 `ros_response`。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 是 [1]
 * 使用原子操作        | 否
 * 无锁                | 是
 * <i>[1] 对于唯一的服务和响应对，请参阅上文了解更多</i>
 *
 * \param[in] service 将进行响应的服务句柄
 * \param[inout] response_header 指向保存关于请求 ID 元数据的结构体的指针
 * \param[in] ros_response 指向 ROS 响应消息的类型擦除指针
 * \return #RCL_RET_OK 如果响应成功发送，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_SERVICE_INVALID 如果服务无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_send_response(
  const rcl_service_t * service, rmw_request_id_t * response_header, void * ros_response);

/// 获取服务的主题名称。
/**
 * 此函数返回服务的内部主题名称字符串。
 * 如果以下情况之一成立，此函数可能失败，因此返回 `NULL`：
 *   - 服务为 `NULL`
 *   - 服务无效（从未调用 init、调用 fini 或无效）
 *
 * 只要服务有效，返回的字符串就有效。
 * 如果主题名称更改，字符串的值可能会更改，因此如果这是一个问题，请建议复制字符串。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] service 服务指针
 * \return 成功时的名称字符串，否则为 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const char * rcl_service_get_service_name(const rcl_service_t * service);

/// 返回 rcl 服务选项。
/**
 * 此函数返回服务的内部选项结构体。
 * 如果以下情况之一成立，此函数可能失败，因此返回 `NULL`：
 *   - 服务为 `NULL`
 *   - 服务无效（从未调用 init、调用 fini 或无效）
 *
 * 只要服务有效，返回的结构体就有效。
 * 如果服务选项更改，结构体中的值可能会更改，因此如果这是一个问题，请建议复制结构体。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] service 服务指针
 * \return 成功时的选项结构体，否则为 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const rcl_service_options_t * rcl_service_get_options(const rcl_service_t * service);

/// 返回 rmw 服务句柄。
/**
 * 返回的句柄是指向内部持有的 rmw 句柄的指针。
 * 此函数可能失败，因此返回 `NULL`，如果：
 *   - 服务为 `NULL`
 *   - 服务无效（从未调用 init、调用 fini 或无效）
 *
 * 如果服务被终止或调用 rcl_shutdown()，则返回的句柄将变为无效。
 * 返回的句柄不能保证在服务的生命周期内一直有效，因为它可能被终止并重新创建。
 * 因此建议每次需要时使用此函数从服务获取句柄，并避免与可能更改句柄的函数同时使用句柄。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] service 指向 rcl 服务的指针
 * \return 成功时的 rmw 服务句柄，否则为 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rmw_service_t * rcl_service_get_rmw_handle(const rcl_service_t * service);

/// 检查服务是否有效。
/**
 * 如果 `service` 无效，则返回的布尔值为 `false`。
 * 否则，返回的布尔值为 `true`。
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
 * \param[in] service 指向 rcl 服务的指针
 * \return 如果 `service` 有效，则为 `true`，否则为 `false`
 */
RCL_PUBLIC
bool rcl_service_is_valid(const rcl_service_t * service);

/// 获取服务请求订阅的实际 qos 设置。
/**
 * 用于获取服务请求订阅的实际 qos 设置。
 * 当使用 RMW_*_SYSTEM_DEFAULT 时，只有在创建服务后才能解析实际配置，
 * 并且它取决于底层的 rmw 实现。
 * 如果正在使用的底层设置无法用 ROS 术语表示，
 * 它将被设置为 RMW_*_UNKNOWN。
 * 返回的结构只有在 rcl_service_t 有效时才有效。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] service 指向 rcl 服务的指针
 * \return 成功时的 qos 结构，否则为 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const rmw_qos_profile_t * rcl_service_request_subscription_get_actual_qos(
  const rcl_service_t * service);

/// 获取服务响应发布者的实际 qos 设置。
/**
 * 用于获取服务响应发布者的实际 qos 设置。
 * 当使用 RMW_*_SYSTEM_DEFAULT 时，只有在创建服务后才能解析实际配置，
 * 并且它取决于底层的 rmw 实现。
 * 如果正在使用的底层设置无法用 ROS 术语表示，
 * 它将被设置为 RMW_*_UNKNOWN。
 * 返回的结构只有在 rcl_service_t 有效时才有效。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] service 指向 rcl 服务的指针
 * \return 成功时的 qos 结构，否则为 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const rmw_qos_profile_t * rcl_service_response_publisher_get_actual_qos(
  const rcl_service_t * service);

/// 为服务设置新请求回调函数。
/**
 * 此 API 将回调函数设置为在服务收到新请求时调用。
 *
 * \sa rmw_service_set_on_new_request_callback 了解此函数的详细信息。
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
 * \param[in] service 要设置回调的服务
 * \param[in] callback 当新请求到达时要调用的回调，可以为 NULL
 * \param[in] user_data 后续调用时提供给回调的数据，可以为 NULL
 * \return 如果回调已设置为监听器，则为 `RCL_RET_OK`，或
 * \return 如果 `service` 为 NULL，则为 `RCL_RET_INVALID_ARGUMENT`，或
 * \return 如果 API 在 dds 实现中未实现，则为 `RCL_RET_UNSUPPORTED`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_service_set_on_new_request_callback(
  const rcl_service_t * service, rcl_event_callback_t callback, const void * user_data);

/// 为服务配置服务自省功能。
/**
 * 为此服务启用或禁用服务自省功能。
 * 如果自省状态为 RCL_SERVICE_INTROSPECTION_OFF，则自省将被禁用。
 * 如果状态为 RCL_SERVICE_INTROSPECTION_METADATA，则将发布客户端元数据。
 * 如果状态为 RCL_SERVICE_INTROSPECTION_CONTENTS，则将发布客户端元数据以及服务请求和响应内容。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 可能 [1]
 * 无锁              | 可能 [1]
 * <i>[1] rmw 实现定义</i>
 *
 * \param[in] service 要配置服务自省的服务
 * \param[in] node 用于创建自省发布者的有效 rcl_node_t
 * \param[in] clock 用于生成自省时间戳的有效 rcl_clock_t
 * \param[in] type_support 与此服务关联的类型支持库
 * \param[in] publisher_options 创建自省发布者时使用的选项
 * \param[in] introspection_state 描述自省应为 OFF、METADATA 还是 CONTENTS 的 rcl_service_introspection_state_t
 * \return 如果调用成功，则为 #RCL_RET_OK，或
 * \return 如果事件发布者无效，则为 #RCL_RET_ERROR，或
 * \return 如果给定节点无效，则为 #RCL_RET_NODE_INVALID，或
 * \return 如果客户端或节点结构无效，则为 #RCL_RET_INVALID_ARGUMENT，
 * \return 如果内存分配失败，则为 #RCL_RET_BAD_ALLOC
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_service_configure_service_introspection(
  rcl_service_t * service, rcl_node_t * node, rcl_clock_t * clock,
  const rosidl_service_type_support_t * type_support,
  const rcl_publisher_options_t publisher_options,
  rcl_service_introspection_state_t introspection_state);

#ifdef __cplusplus
}
#endif

#endif  // RCL__SERVICE_H_
