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

#ifndef RCL_ACTION__TYPES_H_
#define RCL_ACTION__TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "action_msgs/msg/goal_info.h"
#include "action_msgs/msg/goal_status.h"
#include "action_msgs/msg/goal_status_array.h"
#include "action_msgs/srv/cancel_goal.h"
#include "rcl/allocator.h"
#include "rcl/macros.h"
#include "rcl/types.h"
#include "rcl_action/visibility_control.h"
#include "rosidl_runtime_c/action_type_support_struct.h"

// rcl action 特定的返回代码在 2XXX 范围内
/// 动作名称验证未通过的返回代码。
#define RCL_RET_ACTION_NAME_INVALID 2000
/// 动作目标已接受的返回代码。
#define RCL_RET_ACTION_GOAL_ACCEPTED 2100
/// 动作目标被拒绝的返回代码。
#define RCL_RET_ACTION_GOAL_REJECTED 2101
/// 动作客户端无效的返回代码。
#define RCL_RET_ACTION_CLIENT_INVALID 2102
/// 动作客户端获取响应失败的返回代码。
#define RCL_RET_ACTION_CLIENT_TAKE_FAILED 2103
/// 动作服务器无效的返回代码。
#define RCL_RET_ACTION_SERVER_INVALID 2200
/// 动作服务器获取请求失败的返回代码。
#define RCL_RET_ACTION_SERVER_TAKE_FAILED 2201
/// 动作目标句柄无效的返回代码。
#define RCL_RET_ACTION_GOAL_HANDLE_INVALID 2300
/// 动作无效事件的返回代码。
#define RCL_RET_ACTION_GOAL_EVENT_INVALID 2301

// TODO(jacobperron): 将这些移动到 UUID 的公共位置
#define UUID_SIZE 16
#define uuidcmp(uuid0, uuid1) (0 == memcmp(uuid0, uuid1, UUID_SIZE))
#define zerouuid                                   \
  (uint8_t[UUID_SIZE])                             \
  {                                                \
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 \
  }
#define uuidcmpzero(uuid) uuidcmp(uuid, (zerouuid))

// 为方便起见，为生成的消息定义类型
typedef action_msgs__msg__GoalInfo rcl_action_goal_info_t;
typedef action_msgs__msg__GoalStatus rcl_action_goal_status_t;
/// 带有动作目标状态数组的结构体
typedef struct rcl_action_goal_status_array_s
{
  /// 目标状态数组消息
  action_msgs__msg__GoalStatusArray msg;
  /// 用于初始化此结构的分配器。
  rcl_allocator_t allocator;
} rcl_action_goal_status_array_t;
typedef action_msgs__srv__CancelGoal_Request rcl_action_cancel_request_t;
/// 带有动作取消响应的结构体
typedef struct rcl_action_cancel_response_s
{
  /// 取消目标响应消息
  action_msgs__srv__CancelGoal_Response msg;
  /// 用于初始化此结构的分配器。
  rcl_allocator_t allocator;
} rcl_action_cancel_response_t;

/// 目标状态
// TODO(jacobperron): 让状态由 action_msgs/msg/goal_status.h 定义
// 理想情况下，当功能可用时，我们可以直接从消息中使用枚举类型。问题：https://github.com/ros2/rosidl/issues/260
typedef int8_t rcl_action_goal_state_t;
#define GOAL_STATE_UNKNOWN action_msgs__msg__GoalStatus__STATUS_UNKNOWN
#define GOAL_STATE_ACCEPTED action_msgs__msg__GoalStatus__STATUS_ACCEPTED
#define GOAL_STATE_EXECUTING action_msgs__msg__GoalStatus__STATUS_EXECUTING
#define GOAL_STATE_CANCELING action_msgs__msg__GoalStatus__STATUS_CANCELING
#define GOAL_STATE_SUCCEEDED action_msgs__msg__GoalStatus__STATUS_SUCCEEDED
#define GOAL_STATE_CANCELED action_msgs__msg__GoalStatus__STATUS_CANCELED
#define GOAL_STATE_ABORTED action_msgs__msg__GoalStatus__STATUS_ABORTED
#define GOAL_STATE_NUM_STATES 7

/// 用户友好的无效转换错误消息
// 如果枚举值发生变化，类型.c 中的描述变量应该更改
extern const char * goal_state_descriptions[];
extern const char * goal_event_descriptions[];

/// 目标状态转换事件
typedef enum rcl_action_goal_event_e {
  GOAL_EVENT_EXECUTE = 0,
  GOAL_EVENT_CANCEL_GOAL,
  GOAL_EVENT_SUCCEED,
  GOAL_EVENT_ABORT,
  GOAL_EVENT_CANCELED,
  GOAL_EVENT_NUM_EVENTS
} rcl_action_goal_event_t;

/// 返回一个成员设置为零值的 rcl_action_goal_info_t。
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_action_goal_info_t rcl_action_get_zero_initialized_goal_info(void);

/// 返回一个成员设置为 `NULL` 的 rcl_action_goal_status_array_t。
/**
 * 在将 rcl_action_goal_status_array_t 传递给 rcl_action_server_get_goal_status_array() 之前，
 * 应调用此函数以获取空的 rcl_action_goal_status_array_t。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_action_goal_status_array_t rcl_action_get_zero_initialized_goal_status_array(void);

/// 返回一个成员设置为 `NULL` 的 rcl_action_cancel_request_t。
/**
 * 在将 rcl_action_cancel_request_t 传递给 rcl_action_cancel_request_init() 之前，
 * 应调用此函数以获取空的 rcl_action_cancel_request_t。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_action_cancel_request_t rcl_action_get_zero_initialized_cancel_request(void);

/// 返回一个成员设置为 `NULL` 的 rcl_action_cancel_response_t。
/**
 * 在将 rcl_action_cancel_response_t 传递给 rcl_action_cancel_response_init() 之前，
 * 应调用此函数以获取空的 rcl_action_cancel_response_t。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_action_cancel_response_t rcl_action_get_zero_initialized_cancel_response(void);

/// 初始化 rcl_action_goal_status_array_t。
/**
 * 调用此函数后，可以使用 rcl_action_goal_status_array_t 来填充和获取状态数组消息，
 * 并分别使用 rcl_action_get_goal_status_array() 和 rcl_action_publish_status() 与动作服务器进行通信。
 *
 * 示例用法：
 *
 * ```c
 * #include <rcl/rcl.h>
 * #include <rcl_action/rcl_action.h>
 *
 * rcl_action_goal_status_array_t goal_status_array =
 *   rcl_action_get_zero_initialized_goal_status_array();
 * size_t num_status = 42;
 * ret = rcl_action_goal_status_array_init(
 *   &goal_status_array,
 *   num_status,
 *   rcl_get_default_allocator());
 * // ... 错误处理，完成消息时，结束
 * ret = rcl_action_goal_status_array_fini(&goal_status_array, rcl_get_default_allocator());
 * // ... 错误处理
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
 * \param[out] status_array 要初始化的预分配、零初始化的目标状态数组消息。
 * \param[in] num_status 要为其分配空间的状态消息数量。必须大于零
 * \param[in] allocator 有效的分配器
 * \return `RCL_RET_OK` 如果取消响应成功初始化，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_ALREADY_INIT` 如果状态数组已经初始化，或
 * \return `RCL_RET_BAD_ALLOC` 如果分配内存失败，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_goal_status_array_init(
  rcl_action_goal_status_array_t * status_array, const size_t num_status,
  const rcl_allocator_t allocator);

/// 结束 rcl_action_goal_status_array_t。
/**
 * 调用后，目标状态数组消息将不再有效。
 *
 * <hr>
 * 属性          | 遵循
 * ------------------ | -------------
 * 分配内存   | 是
 * 线程安全        | 否
 * 使用原子       | 否
 * 无锁          | 是
 *
 * \param[inout] status_array 要取消初始化的目标状态数组消息
 * \return `RCL_RET_OK` 如果目标状态数组成功取消初始化，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_goal_status_array_fini(rcl_action_goal_status_array_t * status_array);

/// 初始化 rcl_action_cancel_response_t。
/**
 * 调用此函数后，可以使用 rcl_action_cancel_response_t 来填充和处理取消请求，
 * 并使用 rcl_action_process_cancel_request() 与动作服务器进行通信。
 *
 * 示例用法：
 *
 * ```c
 * #include <rcl/rcl.h>
 * #include <rcl_action/rcl_action.h>
 *
 * rcl_action_cancel_response_t cancel_response =
 *   rcl_action_get_zero_initialized_cancel_response();
 * size_t num_goals_canceling = 10;
 * ret = rcl_action_cancel_response_init(
 *   &cancel_response,
 *   num_goals_canceling,
 *   rcl_get_default_allocator());
 * // ... 错误处理，完成处理响应时，结束
 * ret = rcl_action_cancel_response_fini(&cancel_response, rcl_get_default_allocator());
 * // ... 错误处理
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
 * \param[out] cancel_response 要初始化的预分配、零初始化的取消响应消息。
 * \param[in] num_goals_canceling 要添加到响应中的取消目标数量。必须大于零
 * \param[in] allocator 有效的分配器
 * \return `RCL_RET_OK` 如果取消响应成功初始化，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效，或
 * \return `RCL_RET_ALREADY_INIT` 如果取消响应已经初始化，或
 * \return `RCL_RET_BAD_ALLOC` 如果分配内存失败，或
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_cancel_response_init(
  rcl_action_cancel_response_t * cancel_response, const size_t num_goals_canceling,
  const rcl_allocator_t allocator);

/// 结束 rcl_action_cancel_response_t。
/**
 * 调用后，取消响应消息将不再有效。
 *
 * <hr>
 * 属性          | 遵循
 * ------------------ | -------------
 * Allocates Memory   | Yes
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param[inout] cancel_response the cancel response message to be deinitialized
 * \return `RCL_RET_OK` if the cancel response was deinitialized successfully, or
 * \return `RCL_RET_INVALID_ARGUMENT` if any arguments are invalid, or
 * \return `RCL_RET_ERROR` if an unspecified error occurs.
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_cancel_response_fini(rcl_action_cancel_response_t * cancel_response);

#ifdef __cplusplus
}
#endif

#endif  // RCL_ACTION__TYPES_H_
