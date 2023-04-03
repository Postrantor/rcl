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

#ifndef RCL_LIFECYCLE__TRANSITION_MAP_H_
#define RCL_LIFECYCLE__TRANSITION_MAP_H_

#include "rcl/macros.h"
#include "rcl_lifecycle/data_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/// 初始化 rcl_lifecycle_state_init。
/**
 * 在传递给 rcl_lifecycle_register_state() 或 rcl_lifecycle_register_transition() 之前，
 * 应该调用此函数以获取空的 rcl_lifecycle_transition_map_t。
 *
 * \return rcl_lifecycle_transition_map_t 初始化后的结构体
 */
RCL_LIFECYCLE_PUBLIC
RCL_WARN_UNUSED
rcl_lifecycle_transition_map_t rcl_lifecycle_get_zero_initialized_transition_map();

/// 使用 rcl_lifecycle_state_machine_t 检查转换映射是否激活。
/**
 * 该函数检查转换映射是否已初始化。如果转换映射成功初始化，则返回 `RCL_RET_OK`；
 * 如果转换映射未初始化，则返回 `RCL_RET_ERROR`。
 *
 * <hr>
 * 属性              | 符合性
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] transition_map 要检查的转换映射结构体指针
 * \return `RCL_RET_OK`，如果转换映射成功初始化，或
 * \return `RCL_RET_INVALID_ARGUMENT`，如果任何参数无效，或
 * \return `RCL_RET_ERROR`，如果转换映射未初始化。
 */
RCL_LIFECYCLE_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_lifecycle_transition_map_is_initialized(
  const rcl_lifecycle_transition_map_t * transition_map);

/// 结束 rcl_lifecycle_transition_map_t。
/**
 * 调用此函数将 rcl_lifecycle_transition_map_t 结构体设置为未初始化状态，
 * 该状态在功能上与调用 rcl_lifecycle_register_state 或
 * rcl_lifecycle_register_transition 之前相同。此函数使 rcl_lifecycle_transition_map_t 无效。
 *
 * <hr>
 * 属性              | 符合性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[inout] transition_map 要取消初始化的结构体
 * \param[in] allocator 用于取消初始化状态机的有效分配器
 * \return `RCL_RET_OK`，如果状态成功取消初始化，或
 * \return `RCL_RET_INVALID_ARGUMENT`，如果任何参数无效，或
 * \return `RCL_RET_ERROR`，如果发生未指定的错误。
 */
RCL_LIFECYCLE_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_lifecycle_transition_map_fini(
  rcl_lifecycle_transition_map_t * transition_map, const rcl_allocator_t * allocator);

/// 注册状态
/**
 * 此函数在转换映射中注册新状态。
 *
 * <hr>
 * 属性              | 符合性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] transition_map 要修改的转换映射
 * \param[in] state 要注册的状态
 * \param[in] allocator 用于注册状态机的有效分配器
 * \return `RCL_RET_OK`，如果状态成功注册，或
 * \return `RCL_RET_INVALID_ARGUMENT`，如果任何参数无效，或
 * \return `RCL_RET_LIFECYCLE_STATE_REGISTERED`，如果状态已经注册。
 */
RCL_LIFECYCLE_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_lifecycle_register_state(
  rcl_lifecycle_transition_map_t * transition_map, rcl_lifecycle_state_t state,
  const rcl_allocator_t * allocator);

/// 注册转换
/**
 * 此函数在转换映射中注册新转换。
 *
 * <hr>
 * 属性              | 符合性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] transition_map 要修改的转换映射
 * \param[in] transition 要注册的转换
 * \param[in] allocator 用于注册状态机的有效分配器
 * \return `RCL_RET_OK`，如果状态成功取消初始化，或
 * \return `RCL_RET_BAD_ALLOC`，如果分配内存失败，或
 * \return `RCL_RET_INVALID_ARGUMENT`，如果任何参数无效，或
 * \return `RCL_RET_LIFECYCLE_STATE_NOT_REGISTERED`，如果状态未注册，或
 * \return `RCL_RET_ERROR`，如果发生未指定的错误。
 */
RCL_LIFECYCLE_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_lifecycle_register_transition(
  rcl_lifecycle_transition_map_t * transition_map, rcl_lifecycle_transition_t transition,
  const rcl_allocator_t * allocator);

/// 根据状态 ID 从转换映射中获取状态
/**
 * 根据 `id` 返回内部生命周期状态结构体的指针。
 *
 * <hr>
 * 属性              | 符合性
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] transition_map 转换映射
 * \param[in] state_id 状态 ID
 * \return 指向 rcl_lifecycle_state_t 的指针，如果状态 ID 不存在，则为 NULL
 */
RCL_LIFECYCLE_PUBLIC
RCL_WARN_UNUSED
rcl_lifecycle_state_t * rcl_lifecycle_get_state(
  rcl_lifecycle_transition_map_t * transition_map, unsigned int state_id);

/// 根据状态 ID 从转换映射中获取状态
/**
 * 根据 `label` 返回内部生命周期转换结构体的指针。
 *
 * <hr>
 * 属性              | 符合性
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] transition_map 要修改的转换映射
 * \param[in] state_id 用于获取要搜索的标签
 * \return 指向 rcl_lifecycle_state_t 的指针，如果状态 ID 不存在，则为 NULL
 */
RCL_LIFECYCLE_PUBLIC
RCL_WARN_UNUSED
rcl_lifecycle_transition_t * rcl_lifecycle_get_transitions(
  rcl_lifecycle_transition_map_t * transition_map, unsigned int transition_id);

#ifdef __cplusplus
}
#endif

#endif  // RCL_LIFECYCLE__TRANSITION_MAP_H_
