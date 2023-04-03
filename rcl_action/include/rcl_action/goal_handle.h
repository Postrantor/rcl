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

#ifndef RCL_ACTION__GOAL_HANDLE_H_
#define RCL_ACTION__GOAL_HANDLE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/allocator.h"
#include "rcl_action/goal_state_machine.h"
#include "rcl_action/types.h"
#include "rcl_action/visibility_control.h"

/// 内部rcl action目标实现结构体
typedef struct rcl_action_goal_handle_impl_s rcl_action_goal_handle_impl_t;

/// Action的目标句柄
typedef struct rcl_action_goal_handle_s
{
  /// 指向action目标句柄实现的指针
  rcl_action_goal_handle_impl_t * impl;
} rcl_action_goal_handle_t;

/**
 * @brief 返回一个成员设置为`NULL`的rcl_action_goal_handle_t结构体
 *
 * 在传递给rcl_action_goal_handle_init()之前，应该调用此函数获取一个空的rcl_action_goal_handle_t。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_action_goal_handle_t rcl_action_get_zero_initialized_goal_handle(void);

/**
 * @brief 初始化一个rcl_action_goal_handle_t
 *
 * 调用此函数后，可以使用rcl_action_goal_handle_t更新目标状态，
 * 使用rcl_action_update_goal_state()。
 * 还可以使用rcl_action_goal_handle_get_message()和rcl_action_goal_handle_is_active()查询目标状态。
 * 可以使用rcl_action_goal_handle_get_message()和rcl_action_goal_handle_get_info()访问目标信息。
 *
 * 目标句柄通常由动作服务器初始化和完成。
 * 即分配器应由动作服务器提供。
 * 使用rcl_action_accept_new_goal()创建目标句柄，并使用rcl_action_clear_expired_goals()或rcl_action_server_fini()销毁。
 *
 * \param[out] goal_handle 预先分配的、零初始化的目标句柄结构，用于初始化
 * \param[in] goal_info 关于要复制到目标句柄的目标的信息
 * \param[in] allocator 用于初始化目标句柄的有效分配器
 * \return `RCL_RET_OK` 如果goal_handle成功初始化，或
 * \return `RCL_RET_INVALID_ARGUMENT` 如果分配器无效，或
 * \return `RCL_RET_ACTION_GOAL_HANDLE_INVALID` 如果目标句柄无效，或
 * \return `RCL_RET_ALREADY_INIT` 如果目标句柄已经初始化，或
 * \return `RCL_RET_BAD_ALLOC` 如果分配内存失败
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_goal_handle_init(
  rcl_action_goal_handle_t * goal_handle, const rcl_action_goal_info_t * goal_info,
  rcl_allocator_t allocator);

/**
 * @brief 结束一个rcl_action_goal_handle_t
 *
 * 调用后，rcl_action_goal_handle_t将不再有效，
 * rcl_action_server_t将不再跟踪与目标句柄关联的目标。
 *
 * 调用后，使用此目标句柄调用rcl_action_publish_feedback()、rcl_action_publish_status()、
 * rcl_action_update_goal_state()、rcl_action_goal_handle_get_status()、
 * rcl_action_goal_handle_is_active()、rcl_action_goal_handle_get_message()和
 * rcl_action_goal_handle_get_info()将失败。
 *
 * 然而，给定的动作服务器仍然有效。
 *
 * \param[inout] goal_handle 要取消初始化的结构
 * \return `RCL_RET_OK` 如果目标句柄成功取消初始化，或
 * \return `RCL_RET_ACTION_GOAL_HANDLE_INVALID` 如果目标句柄无效，或
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_goal_handle_fini(rcl_action_goal_handle_t * goal_handle);

/**
 * @brief 使用rcl_action_goal_handle_t和事件更新目标状态
 *
 * 这是一个非阻塞调用。
 *
 * \param[inout] goal_handle 包含要转换的目标状态的结构
 * \param[in] goal_event 用于转换目标状态的事件
 * \return `RCL_RET_OK` 如果目标状态更新成功，或
 * \return `RCL_RET_ACTION_GOAL_EVENT_INVALID` 如果目标事件无效，或
 * \return `RCL_RET_ACTION_GOAL_HANDLE_INVALID` 如果目标句柄无效，或
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_update_goal_state(
  rcl_action_goal_handle_t * goal_handle, const rcl_action_goal_event_t goal_event);

/// 使用 rcl_action_goal_handle_t 获取目标的 ID。
/**
 * 这是一个非阻塞调用。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] goal_handle 包含目标和元数据的结构体
 * \param[out] goal_info 预分配的结构体，用于复制目标信息
 * \return `RCL_RET_OK` 如果成功访问目标 ID，或者
 * \return `RCL_RET_ACTION_GOAL_HANDLE_INVALID` 如果目标句柄无效，或者
 * \return `RCL_RET_INVALID_ARGUMENT` 如果 goal_info 参数无效
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_goal_handle_get_info(
  const rcl_action_goal_handle_t * goal_handle, rcl_action_goal_info_t * goal_info);

/// 获取目标状态。
/**
 * 这是一个非阻塞调用。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] goal_handle 包含目标和元数据的结构体
 * \param[out] status 预分配的结构体，用于复制目标状态
 * \return `RCL_RET_OK` 如果成功访问目标 ID，或者
 * \return `RCL_RET_ACTION_GOAL_HANDLE_INVALID` 如果目标句柄无效，或者
 * \return `RCL_RET_INVALID_ARGUMENT` 如果 status 参数无效
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_goal_handle_get_status(
  const rcl_action_goal_handle_t * goal_handle, rcl_action_goal_state_t * status);

/// 使用 rcl_action_goal_handle_t 检查目标是否处于活动状态。
/**
 * 这是一个非阻塞调用。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] goal_handle 包含目标和元数据的结构体
 * \return `true` 如果目标处于以下状态之一：ACCEPTED、EXECUTING 或 CANCELING，或者
 * \return `false` 如果目标句柄指针无效，或者
 * \return `false` 其他情况
*/
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
bool rcl_action_goal_handle_is_active(const rcl_action_goal_handle_t * goal_handle);

/// 检查目标在当前状态下是否可以转换为 CANCELING。
/**
 * 这是一个非阻塞调用。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] goal_handle 包含目标和元数据的结构体
 * \return `true` 如果目标可以从当前状态转换为 CANCELING，或者
 * \return `false` 如果目标句柄指针无效，或者
 * \return `false` 其他情况
*/
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
bool rcl_action_goal_handle_is_cancelable(const rcl_action_goal_handle_t * goal_handle);

/// 检查 rcl_action_goal_handle_t 是否有效。
/**
 * 这是一个非阻塞调用。
 *
 * 目标句柄无效的情况：
 *   - 实现为 `NULL`（未调用 rcl_action_goal_handle_init() 或调用失败）
 *   - 自目标句柄初始化以来已调用 rcl_shutdown()
 *   - 使用 rcl_action_goal_handle_fini() 对目标句柄进行了终止处理
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] goal_handle 要评估为有效或无效的结构体
 * \return `true` 如果目标句柄有效，或者
 * \return `false` 如果目标句柄指针为空，或者
 * \return `false` 其他情况
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
bool rcl_action_goal_handle_is_valid(const rcl_action_goal_handle_t * goal_handle);

#ifdef __cplusplus
}
#endif

#endif  // RCL_ACTION__GOAL_HANDLE_H_
