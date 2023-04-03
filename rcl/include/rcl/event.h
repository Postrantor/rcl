// Copyright 2019 Open Source Robotics Foundation, Inc.
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

#ifndef RCL__EVENT_H_
#define RCL__EVENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <rmw/event.h>

#include "rcl/client.h"
#include "rcl/event_callback.h"
#include "rcl/macros.h"
#include "rcl/publisher.h"
#include "rcl/service.h"
#include "rcl/subscription.h"
#include "rcl/visibility_control.h"

/// 发布者事件类型枚举
typedef enum rcl_publisher_event_type_e {
  RCL_PUBLISHER_OFFERED_DEADLINE_MISSED,   ///< 提供的截止日期未满足
  RCL_PUBLISHER_LIVELINESS_LOST,           ///< 活跃度丢失
  RCL_PUBLISHER_OFFERED_INCOMPATIBLE_QOS,  ///< 提供的QoS不兼容
  RCL_PUBLISHER_INCOMPATIBLE_TYPE,         ///< 类型不兼容
  RCL_PUBLISHER_MATCHED,                   ///< 匹配成功
} rcl_publisher_event_type_t;

/// 订阅者事件类型枚举
typedef enum rcl_subscription_event_type_e {
  RCL_SUBSCRIPTION_REQUESTED_DEADLINE_MISSED,   ///< 请求的截止日期未满足
  RCL_SUBSCRIPTION_LIVELINESS_CHANGED,          ///< 活跃度发生变化
  RCL_SUBSCRIPTION_REQUESTED_INCOMPATIBLE_QOS,  ///< 请求的QoS不兼容
  RCL_SUBSCRIPTION_MESSAGE_LOST,                ///< 消息丢失
  RCL_SUBSCRIPTION_INCOMPATIBLE_TYPE,           ///< 类型不兼容
  RCL_SUBSCRIPTION_MATCHED,                     ///< 匹配成功
} rcl_subscription_event_type_t;

/// rcl内部实现结构体
typedef struct rcl_event_impl_s rcl_event_impl_t;

/// 封装ROS QoS事件句柄的结构体
typedef struct rcl_event_s
{
  /// 指向事件实现的指针
  rcl_event_impl_t * impl;
} rcl_event_t;

/// 返回一个成员设置为`NULL`的rcl_event_t结构体。
/**
 * 在传递给rcl_event_init()之前，应该调用此函数以获取空的rcl_event_t。
 *
 * \return 零初始化的rcl_event_t。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_event_t rcl_get_zero_initialized_event(void);

/// 使用发布者初始化rcl_event_t。
/**
 * 用发布者和期望的event_type填充rcl_event_t。
 *
 * \param[in,out] event 要填充的指针
 * \param[in] publisher 从中获取事件的发布者
 * \param[in] event_type 要监听的事件类型
 * \return 如果rcl_event_t被填充，则返回#RCL_RET_OK，或
 * \return 如果任何参数无效，则返回#RCL_RET_INVALID_ARGUMENT，或
 * \return 如果分配内存失败，则返回#RCL_RET_BAD_ALLOC，或
 * \return 如果不支持event_type，则返回#RCL_RET_UNSUPPORTED，或
 * \return 如果发生未指定的错误，则返回#RCL_RET_ERROR。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_publisher_event_init(
  rcl_event_t * event, const rcl_publisher_t * publisher,
  const rcl_publisher_event_type_t event_type);

/// 使用订阅者初始化rcl_event_t。
/**
 * 用订阅者和期望的event_type填充rcl_event_t。
 *
 * \param[in,out] event 要填充的指针
 * \param[in] subscription 从中获取事件的订阅者
 * \param[in] event_type 要监听的事件类型
 * \return 如果rcl_event_t被填充，则返回#RCL_RET_OK，或
 * \return 如果任何参数无效，则返回#RCL_RET_INVALID_ARGUMENT，或
 * \return 如果分配内存失败，则返回#RCL_RET_BAD_ALLOC，或
 * \return 如果不支持event_type，则返回#RCL_RET_UNSUPPORTED，或
 * \return 如果发生未指定的错误，则返回#RCL_RET_ERROR。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_subscription_event_init(
  rcl_event_t * event, const rcl_subscription_t * subscription,
  const rcl_subscription_event_type_t event_type);

// 使用事件句柄获取事件。
/**
 * 从事件句柄中获取一个事件。
 *
 * \param[in] event 要获取的事件对象
 * \param[in, out] event_info 将获取到的数据写入的事件信息对象
 * \return #RCL_RET_OK 如果成功，或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或者
 * \return #RCL_RET_BAD_ALLOC 如果内存分配失败，或者
 * \return #RCL_RET_EVENT_TAKE_FAILED 如果获取事件失败，或者
 * \return #RCL_RET_ERROR 如果发生意外错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_take_event(const rcl_event_t * event, void * event_info);

// 结束一个事件。
/**
 * 结束一个事件。
 *
 * \param[in] event 要结束的事件
 * \return #RCL_RET_OK 如果成功，或者
 * \return #RCL_RET_EVENT_INVALID 如果事件为 null，或者
 * \return #RCL_RET_ERROR 如果发生意外错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_event_fini(rcl_event_t * event);

/// 返回 rmw 事件句柄。
/**
 * 返回的句柄是指向内部持有的 rmw 句柄的指针。
 * 此函数可能失败，并因此返回 `NULL`，如果：
 *   - 事件为 `NULL`
 *   - 事件无效（从未调用 init、调用 fini 或无效节点）
 *
 * 如果事件被结束或调用 rcl_shutdown()，返回的句柄将无效。
 * 返回的句柄不能保证在事件的生命周期内有效，因为它可能被结束并重新创建。
 * 因此，建议使用此函数在每次需要时从事件中获取句柄，并避免与可能更改它的函数同时使用句柄。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] event 指向 rcl 事件的指针
 * \return 如果成功，则返回 rmw 事件句柄，否则返回 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rmw_event_t * rcl_event_get_rmw_handle(const rcl_event_t * event);

/// 检查事件是否有效。
/**
 * 如果 `event` 无效，则返回的布尔值为 `false`。
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
 * \param[in] event 指向 rcl 事件的指针
 * \return 如果 `event` 有效，则返回 `true`，否则返回 `false`
 */
RCL_PUBLIC
bool rcl_event_is_valid(const rcl_event_t * event);

/// 为事件设置回调函数。
/**
 * 此 API 将回调函数设置为在事件通知新事件实例时调用。
 *
 * \sa rmw_event_set_callback 有关此函数的更多详细信息。
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
 * \param[in] event 要设置回调的事件
 * \param[in] callback 当新事件发生时要调用的回调，可以为 NULL
 * \param[in] user_data 后续调用回调时提供，可以为 NULL
 * \return `RCL_RET_OK` 如果回调已设置为监听器，或者
 * \return `RCL_RET_INVALID_ARGUMENT` 如果 `event` 为 NULL，或者
 * \return `RCL_RET_UNSUPPORTED` 如果 dds 实现中未实现 API
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_event_set_callback(
  const rcl_event_t * event, rcl_event_callback_t callback, const void * user_data);

#ifdef __cplusplus
}
#endif

#endif  // RCL__EVENT_H_
