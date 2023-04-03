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

#ifndef RCL__CLIENT_H_
#define RCL__CLIENT_H_

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

/// @file

/// 内部rcl客户端实现结构体 (Internal rcl client implementation struct).
typedef struct rcl_client_impl_s rcl_client_impl_t;

/// 封装ROS客户端的结构体 (Structure which encapsulates a ROS Client).
typedef struct rcl_client_s {
  /// 指向客户端实现的指针 (Pointer to the client implementation)
  rcl_client_impl_t* impl;
} rcl_client_t;

/// rcl_client_t可用的选项 (Options available for a rcl_client_t).
typedef struct rcl_client_options_s {
  /// 客户端的中间件服务质量设置 (Middleware quality of service settings for the client).
  rmw_qos_profile_t qos;
  /// 用于偶发分配的客户端自定义分配器 (Custom allocator for the client, used for incidental
  /// allocations).
  /** 默认行为（malloc/free）时使用：rcl_get_default_allocator() (For default behavior
   * (malloc/free), use: rcl_get_default_allocator()) */
  rcl_allocator_t allocator;
} rcl_client_options_t;

/// 返回一个成员设置为`NULL`的rcl_client_t结构体 (Return a rcl_client_t struct with members set to
/// `NULL`).
/**
 * 在传递给rcl_client_init()之前，应该调用此函数以获取空的rcl_client_t (Should be called to get a
 * null rcl_client_t before passing to rcl_client_init()).
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_client_t rcl_get_zero_initialized_client(void);

/// 初始化一个 rcl 客户端。
/**
 * 在 rcl_client_t 上调用此函数后，可以通过调用 rcl_send_request() 发送给定类型的请求。
 * 如果请求被（可能是远程的）服务接收，并且服务发送响应，则客户端可以在响应可用于客户端时通过
 * rcl_take_response() 访问响应。
 *
 * 给定的 rcl_node_t 必须有效，生成的 rcl_client_t 只有在给定的 rcl_node_t 保持有效时才有效。
 *
 * rosidl_service_type_support_t 是基于每个 `.srv` 类型获得的。
 * 当用户定义 ROS 服务时，会生成提供所需 rosidl_service_type_support_t 对象的代码。
 * 此对象可以使用适合语言的机制获得。
 * \todo TODO(wjwwood) 编写这些说明并链接到它
 *
 * 对于 C，可以使用宏（例如 `example_interfaces/AddTwoInts`）：
 *
 * ```c
 * #include <rosidl_runtime_c/service_type_support_struct.h>
 * #include <example_interfaces/srv/add_two_ints.h>
 *
 * const rosidl_service_type_support_t * ts =
 *   ROSIDL_GET_SRV_TYPE_SUPPORT(example_interfaces, srv, AddTwoInts);
 * ```
 *
 * 对于 C++，使用模板函数：
 *
 * ```cpp
 * #include <rosidl_typesupport_cpp/service_type_support.hpp>
 * #include <example_interfaces/srv/add_two_ints.hpp>
 *
 * using rosidl_typesupport_cpp::get_service_type_support_handle;
 * const rosidl_service_type_support_t * ts =
 *   get_service_type_support_handle<example_interfaces::srv::AddTwoInts>();
 * ```
 *
 * rosidl_service_type_support_t 对象包含用于发送或接收请求和响应的服务类型特定信息。
 *
 * 主题名称必须是遵循未展开名称的主题和服务名称格式规则的 c 字符串，也称为非完全限定名称：
 *
 * \see rcl_expand_topic_name
 *
 * options
 * 结构允许用户设置服务质量设置以及在初始化/终止客户端时用于分配杂项空间（例如服务名称字符串）的自定义分配器。
 *
 * 预期用法（对于 C 服务）：
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
 * rcl_client_t client = rcl_get_zero_initialized_client();
 * rcl_client_options_t client_ops = rcl_client_get_default_options();
 * ret = rcl_client_init(&client, &node, ts, "add_two_ints", &client_ops);
 * // ... 错误处理，并在关闭时进行最终处理：
 * ret = rcl_client_fini(&client, &node);
 * // ... rcl_client_fini() 的错误处理
 * ret = rcl_node_fini(&node);
 * // ... rcl_node_fini() 的错误处理
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
 * \param[inout] client 预分配的 rcl_client_t 结构
 * \param[in] node 有效的 rcl_node_t
 * \param[in] type_support 服务类型的类型支持对象
 * \param[in] service_name 要请求的服务名称
 * \param[in] options 客户端选项，包括服务质量设置
 * \return #RCL_RET_OK 如果客户端成功初始化，或
 * \return #RCL_RET_NODE_INVALID 如果节点无效，或
 * \return #RCL_RET_ALREADY_INIT 如果客户端已经初始化，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_SERVICE_NAME_INVALID 如果给定的服务名称无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_client_init(
    rcl_client_t* client,
    const rcl_node_t* node,
    const rosidl_service_type_support_t* type_support,
    const char* service_name,
    const rcl_client_options_t* options);

/// 终止一个 rcl_client_t.
/**
 * 调用此函数后，使用此客户端的 rcl_send_request() 和
 * rcl_take_response() 将失败。
 * 但是，给定的节点句柄仍然有效。
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[inout] client 要终止的客户端句柄
 * \param[in] node 用于创建客户端的有效（未终止）节点句柄
 * \return #RCL_RET_OK 如果客户端成功终止, 或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效, 或者
 * \return #RCL_RET_NODE_INVALID 如果节点无效, 或者
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_client_fini(rcl_client_t* client, rcl_node_t* node);

/// 返回 rcl_client_options_t 中的默认客户端选项。
/**
 * 默认值为：
 *
 * - qos = rmw_qos_profile_services_default
 * - allocator = rcl_get_default_allocator()
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_client_options_t rcl_client_get_default_options(void);

/// 使用客户端发送 ROS 请求。
/**
 * 调用者有责任确保 `ros_request` 参数的类型与
 * 客户端关联的类型（通过类型支持）匹配。
 * 将不同类型传递给 `send_request` 会产生未定义的行为，
 * 并且此函数无法检查，因此不会发生故意错误。
 *
 * rcl_send_request() 是一个非阻塞调用。
 *
 * 由 `ros_request` void 指针给出的 ROS 请求消息始终由
 * 调用代码拥有，但在 `send_request` 期间应保持不变。
 *
 * 只要对客户端和 `ros_request` 的访问得到同步，
 * 此函数就是线程安全的。
 * 这意味着允许从多个线程调用 rcl_send_request()，
 * 但与非线程安全客户端函数同时调用 rcl_send_request() 是不允许的，
 * 例如，同时调用 rcl_send_request() 和 rcl_client_fini() 是不允许的。
 * 在 rcl_send_request() 调用期间，消息不能更改。
 * 在调用 rcl_send_request() 之前，消息可以更改，但在调用
 * rcl_send_request() 之后，它取决于 RMW 实现行为。
 * 同一个 `ros_request` 可以同时传递给多个 rcl_send_request() 调用，
 * 即使客户端不同。rcl_send_request() 不会修改 `ros_request`。
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是 [1]
 * 使用原子操作      | 否
 * 无锁              | 是
 * <i>[1] 对于唯一的客户端和请求对，参见上文了解更多信息</i>
 *
 * \param[in] client 将发出响应的客户端句柄
 * \param[in] ros_request 指向 ROS 请求消息的类型擦除指针
 * \param[out] sequence_number 序列号
 * \return #RCL_RET_OK 如果请求发送成功, 或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效, 或者
 * \return #RCL_RET_CLIENT_INVALID 如果客户端无效, 或者
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_send_request(
    const rcl_client_t* client, const void* ros_request, int64_t* sequence_number);

/// 使用客户端获取ROS响应
/**
 * 调用者需要确保 `ros_response` 参数的类型与客户端关联的类型（通过类型支持）匹配。
 * 将不同类型传递给 take_response 会产生未定义的行为，此函数无法检查，因此不会出现故意的错误。
 * request_header 是一个 rmw 结构，用于请求发送的元信息（例如序列号）。
 * 调用者必须提供指向已分配结构的指针。
 * 此函数将填充结构字段。
 * `ros_response` 应该指向一个已经分配的正确类型的 ROS 响应消息结构，服务的响应将被复制到其中。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 可能 [1]
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 * <i>[1] 仅在填充消息时需要，对于固定大小则避免</i>
 *
 * \param[in] client 将获取响应的客户端句柄
 * \param[inout] request_header 指向请求头的指针
 * \param[inout] ros_response 类型擦除的指向 ROS 响应消息的指针
 * \return #RCL_RET_OK 如果成功获取响应，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_CLIENT_INVALID 如果客户端无效，或
 * \return #RCL_RET_CLIENT_TAKE_FAILED 如果获取失败但中间件中没有错误发生，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_take_response_with_info(
    const rcl_client_t* client, rmw_service_info_t* request_header, void* ros_response);

/// 向后兼容函数，仅接受 rmw_request_id_t
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_take_response(
    const rcl_client_t* client, rmw_request_id_t* request_header, void* ros_response);

/// 获取此客户端将从中请求响应的服务的名称。
/**
 * 此函数返回客户端的内部服务名称字符串。
 * 此函数可能会失败，因此返回 `NULL`，如果：
 *   - 客户端为 `NULL`
 *   - 客户端无效（从未调用 init、调用 fini 或无效节点）
 *
 * 返回的字符串只在 rcl_client_t 有效时有效。
 * 如果服务名称更改，字符串的值可能会更改，因此如果这是一个问题，建议复制字符串。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 是
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] client 指向客户端的指针
 * \return 成功时的名称字符串，否则为 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const char* rcl_client_get_service_name(const rcl_client_t* client);

/// 返回 rcl 客户端选项。
/**
 * 此函数返回客户端的内部选项结构体。
 * 以下情况下，此函数可能失败并返回 `NULL`：
 *   - client 为 `NULL`
 *   - client 无效（未调用 init、已调用 fini 或节点无效）
 *
 * 返回的结构体只在 rcl_client_t 有效期间有效。
 * 如果客户端选项发生变化，结构体中的值可能会改变，
 * 因此，如果这是一个问题，请建议复制该结构体。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 是
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] client 指向客户端的指针
 * \return 成功时返回选项结构体，否则返回 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const rcl_client_options_t* rcl_client_get_options(const rcl_client_t* client);

/// 返回 rmw 客户端句柄。
/**
 * 返回的句柄是指向内部持有的 rmw 句柄的指针。
 * 以下情况下，此函数可能失败并返回 `NULL`：
 *   - client 为 `NULL`
 *   - client 无效（未调用 init、已调用 fini 或节点无效）
 *
 * 如果客户端被终止或调用 rcl_shutdown()，返回的句柄将无效。
 * 返回的句柄不能保证在客户端的生命周期内一直有效，因为它可能被终止并重新创建。
 * 因此，建议每次需要时使用此函数从客户端获取句柄，并避免与可能更改句柄的函数同时使用句柄。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 是
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] client 指向 rcl 客户端的指针
 * \return 成功时返回 rmw 客户端句柄，否则返回 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rmw_client_t* rcl_client_get_rmw_handle(const rcl_client_t* client);

/// 检查客户端是否有效。
/**
 * 如果客户端无效，则返回的布尔值为 `false`。
 * 否则，返回的布尔值为 `true`。
 * 在返回 `false` 的情况下，会设置错误消息。
 * 此函数不会失败。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] client 指向 rcl 客户端的指针
 * \return 如果 `client` 有效，则返回 `true`，否则返回 `false`
 */
RCL_PUBLIC
bool rcl_client_is_valid(const rcl_client_t* client);

/// 获取客户端请求发布器的实际QoS设置。
/**
 * 用于获取客户端请求发布器的实际QoS设置。
 * 当使用RMW_*_SYSTEM_DEFAULT时，实际配置只能在创建客户端后解析，
 * 并且它取决于底层rmw实现。
 * 如果正在使用的底层设置无法用ROS术语表示，
 * 它将被设置为RMW_*_UNKNOWN。
 * 返回的结构只有在rcl_client_t有效时才有效。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 否
 * 线程安全           | 是
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * \param[in] client 指向rcl客户端的指针
 * \return 成功时返回qos结构，否则返回`NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const rmw_qos_profile_t* rcl_client_request_publisher_get_actual_qos(const rcl_client_t* client);

/// 获取客户端响应订阅的实际QoS设置。
/**
 * 用于获取客户端响应订阅的实际QoS设置。
 * 当使用RMW_*_SYSTEM_DEFAULT时，实际配置只能在创建客户端后解析，
 * 并且它取决于底层rmw实现。
 * 如果正在使用的底层设置无法用ROS术语表示，
 * 它将被设置为RMW_*_UNKNOWN。
 * 返回的结构只有在rcl_client_t有效时才有效。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 否
 * 线程安全           | 是
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * \param[in] client 指向rcl客户端的指针
 * \return 成功时返回qos结构，否则返回`NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const rmw_qos_profile_t* rcl_client_response_subscription_get_actual_qos(
    const rcl_client_t* client);

/// 为客户端设置新响应回调函数。
/**
 * 此API将回调函数设置为在客户端收到新响应通知时调用。
 *
 * \sa rmw_client_set_on_new_response_callback 了解此函数的详细信息。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 否
 * 线程安全           | 是
 * 使用原子操作       | 可能 [1]
 * 无锁               | 可能 [1]
 * <i>[1] rmw实现定义</i>
 *
 * \param[in] client 设置回调的客户端
 * \param[in] callback 当新响应到达时调用的回调，可以为NULL
 * \param[in] user_data 后续调用回调时提供，可以为NULL
 * \return 如果回调已设置为监听器，则返回`RCL_RET_OK`，或
 * \return 如果`client`为NULL，则返回`RCL_RET_INVALID_ARGUMENT`，或
 * \return 如果API未在dds实现中实现，则返回`RCL_RET_UNSUPPORTED`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_client_set_on_new_response_callback(
    const rcl_client_t* client, rcl_event_callback_t callback, const void* user_data);

/// 为客户端配置服务自省功能。
/**
 * 为此客户端启用或禁用服务自省功能。
 * 如果自省状态为RCL_SERVICE_INTROSPECTION_OFF，则禁用自省。
 * 如果状态为RCL_SERVICE_INTROSPECTION_METADATA，则发布客户端元数据。
 * 如果状态为RCL_SERVICE_INTROSPECTION_CONTENTS，则发布客户端元数据以及服务请求和响应内容。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 是
 * 线程安全           | 否
 * 使用原子操作       | 可能 [1]
 * 无锁               | 可能 [1]
 * <i>[1] rmw实现定义</i>
 *
 * \param[in] client 配置服务自省的客户端
 * \param[in] node 有效的rcl_node_t，用于创建自省发布器
 * \param[in] clock 有效的rcl_clock_t，用于生成自省时间戳
 * \param[in] type_support 与此客户端关联的类型支持库
 * \param[in] publisher_options 创建自省发布器时使用的选项
 * \param[in] introspection_state
 * 描述自省应为OFF、METADATA还是CONTENTS的rcl_service_introspection_state_t \return
 * 如果调用成功，则返回#RCL_RET_OK，或 \return 如果事件发布器无效，则返回#RCL_RET_ERROR，或 \return
 * 如果给定节点无效，则返回#RCL_RET_NODE_INVALID，或 \return
 * 如果客户端或节点结构无效，则返回#RCL_RET_INVALID_ARGUMENT， \return
 * 如果内存分配失败，则返回#RCL_RET_BAD_ALLOC
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_client_configure_service_introspection(
    rcl_client_t* client,
    rcl_node_t* node,
    rcl_clock_t* clock,
    const rosidl_service_type_support_t* type_support,
    const rcl_publisher_options_t publisher_options,
    rcl_service_introspection_state_t introspection_state);

#ifdef __cplusplus
}
#endif

#endif  // RCL__CLIENT_H_
