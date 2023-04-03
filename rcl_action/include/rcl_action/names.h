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

#ifndef RCL_ACTION__NAMES_H_
#define RCL_ACTION__NAMES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/allocator.h"
#include "rcl/macros.h"
#include "rcl/types.h"
#include "rcl_action/types.h"
#include "rcl_action/visibility_control.h"

/// 获取动作的目标服务名称。
/**
 * 此函数返回给定动作名称的目标服务名称，
 * 动作客户端和动作服务器必须使用该名称才能成功相互通信。
 *
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 是
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] action_name 要获取其目标服务名称的动作名称。
 * \param[in] allocator 要使用的有效分配器。
 * \param[out] goal_service_name 分配的字符串，包含动作目标服务名称，
 *   或者如果函数无法为其分配内存，则为 `NULL`。调用时必须引用 `NULL` 指针。
 * \return 如果返回了动作目标服务名称，则为 `RCL_RET_OK`，
 * \return 如果动作名称无效（即为空），则为 `RCL_RET_ACTION_NAME_INVALID`，
 * \return 如果动作名称为 `NULL`，则为 `RCL_RET_INVALID_ARGUMENT`，
 * \return 如果分配器无效，则为 `RCL_RET_INVALID_ARGUMENT`，
 * \return 如果目标服务名称指针为 `NULL` 或指向非 `NULL` 指针，
 *   则为 `RCL_RET_INVALID_ARGUMENT`，
 * \return 如果分配内存失败，则为 `RCL_RET_BAD_ALLOC`。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_get_goal_service_name(
  const char * action_name, rcl_allocator_t allocator, char ** goal_service_name);

/// 获取动作的取消服务名称。
/**
 * 此函数返回给定动作名称的取消服务名称，
 * 动作客户端和动作服务器必须使用该名称才能成功相互通信。
 *
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 是
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] action_name 要获取其取消服务名称的动作名称。
 * \param[in] allocator 要使用的有效分配器。
 * \param[out] cancel_service_name 分配的字符串，包含动作取消服务名称，
 *   或者如果函数无法为其分配内存，则为 `NULL`。调用时必须引用 `NULL` 指针。
 * \return 如果返回了动作取消服务名称，则为 `RCL_RET_OK`，
 * \return 如果动作名称无效（即为空），则为 `RCL_RET_ACTION_NAME_INVALID`，
 * \return 如果动作名称为 `NULL`，则为 `RCL_RET_INVALID_ARGUMENT`，
 * \return 如果分配器无效，则为 `RCL_RET_INVALID_ARGUMENT`，
 * \return 如果取消服务名称指针为 `NULL` 或指向非 `NULL` 指针，
 *   则为 `RCL_RET_INVALID_ARGUMENT`，
 * \return 如果分配内存失败，则为 `RCL_RET_BAD_ALLOC`。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_get_cancel_service_name(
  const char * action_name, rcl_allocator_t allocator, char ** cancel_service_name);

/// 获取动作的结果服务名称。
/**
 * 此函数返回给定动作名称的结果服务名称，
 * 动作客户端和动作服务器必须使用该名称才能成功相互通信。
 *
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 是
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] action_name 要获取其结果服务名称的动作名称。
 * \param[in] allocator 要使用的有效分配器。
 * \param[out] result_service_name 分配的字符串，包含动作结果服务名称，
 *   或者如果函数无法为其分配内存，则为 `NULL`。调用时必须引用 `NULL` 指针。
 * \return 如果返回了动作结果服务名称，则为 `RCL_RET_OK`，
 * \return 如果动作名称无效（即为空），则为 `RCL_RET_ACTION_NAME_INVALID`，
 * \return 如果动作名称为 `NULL`，则为 `RCL_RET_INVALID_ARGUMENT`，
 * \return 如果分配器无效，则为 `RCL_RET_INVALID_ARGUMENT`，
 * \return 如果结果服务名称指针为 `NULL` 或指向非 `NULL` 指针，
 *   则为 `RCL_RET_INVALID_ARGUMENT`，
 * \return 如果分配内存失败，则为 `RCL_RET_BAD_ALLOC`。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_get_result_service_name(
  const char * action_name, rcl_allocator_t allocator, char ** result_service_name);

/// 获取动作的反馈主题名称。
/**
 * 此函数返回给定动作名称的反馈主题名称，
 * 动作客户端和动作服务器必须使用该名称才能成功相互通信。
 *
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 是
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] action_name 要获取其反馈主题名称的动作名称。
 * \param[in] allocator 要使用的有效分配器。
 * \param[out] feedback_topic_name 分配的字符串，包含动作反馈主题名称，
 *   或者如果函数无法为其分配内存，则为 `NULL`。调用时必须引用 `NULL` 指针。
 * \return 如果返回了动作反馈主题名称，则为 `RCL_RET_OK`，
 * \return 如果动作名称无效（即为空），则为 `RCL_RET_ACTION_NAME_INVALID`，
 * \return 如果动作名称为 `NULL`，则为 `RCL_RET_INVALID_ARGUMENT`，
 * \return 如果分配器无效，则为 `RCL_RET_INVALID_ARGUMENT`，
 * \return 如果反馈主题名称指针为 `NULL` 或指向非 `NULL` 指针，
 *   则为 `RCL_RET_INVALID_ARGUMENT`，
 * \return 如果分配内存失败，则为 `RCL_RET_BAD_ALLOC`。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_get_feedback_topic_name(
  const char * action_name, rcl_allocator_t allocator, char ** feedback_topic_name);

/// 获取动作的状态主题名称。
/**
 * 此函数返回给定动作名称的状态主题名称，
 * 动作客户端和动作服务器必须使用该名称才能成功相互通信。
 *
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 是
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] action_name 要获取其状态主题名称的动作名称。
 * \param[in] allocator 要使用的有效分配器。
 * \param[out] status_topic_name 分配的字符串，包含动作状态主题名称，
 *   或者如果函数无法为其分配内存，则为 `NULL`。调用时必须引用 `NULL` 指针。
 * \return 如果返回了动作状态主题名称，则为 `RCL_RET_OK`，
 * \return 如果动作名称无效（即为空），则为 `RCL_RET_ACTION_NAME_INVALID`，
 * \return 如果动作名称为 `NULL`，则为 `RCL_RET_INVALID_ARGUMENT`，
 * \return 如果分配器无效，则为 `RCL_RET_INVALID_ARGUMENT`，
 * \return 如果状态主题名称指针为 `NULL` 或指向非 `NULL` 指针，
 *   则为 `RCL_RET_INVALID_ARGUMENT`，
 * \return 如果分配内存失败，则为 `RCL_RET_BAD_ALLOC`。
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_get_status_topic_name(
  const char * action_name, rcl_allocator_t allocator, char ** status_topic_name);

#ifdef __cplusplus
}
#endif

#endif  // RCL_ACTION__NAMES_H_
