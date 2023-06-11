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

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl_lifecycle/transition_map.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rcl/error_handling.h"
#include "rcl/macros.h"
#include "rcutils/format_string.h"

/**
 * @brief 获取一个初始化为零的生命周期转换映射结构体
 * 该函数创建并返回一个初始化为零的生命周期转换映射结构体。
 * @return 初始化为零的生命周期转换映射结构体
 */
rcl_lifecycle_transition_map_t rcl_lifecycle_get_zero_initialized_transition_map() {
  // 创建一个静态的生命周期转换映射结构体变量
  static rcl_lifecycle_transition_map_t transition_map;
  // 将状态指针设置为 NULL
  transition_map.states = NULL;
  // 将状态大小设置为 0
  transition_map.states_size = 0;
  // 将转换指针设置为 NULL
  transition_map.transitions = NULL;
  // 将转换大小设置为 0
  transition_map.transitions_size = 0;
  // 返回初始化为零的生命周期转换映射结构体
  return transition_map;
}

/**
 * @brief 检查生命周期转换映射是否已初始化
 * 该函数检查给定的生命周期转换映射结构体是否已经初始化。
 * @param[in] transition_map 要检查的生命周期转换映射结构体指针
 * @return RCL_RET_OK 如果已初始化，否则返回 RCL_RET_ERROR
 */
rcl_ret_t rcl_lifecycle_transition_map_is_initialized(
    const rcl_lifecycle_transition_map_t* transition_map) {
  // 初始化返回值为 RCL_RET_OK
  rcl_ret_t is_initialized = RCL_RET_OK;
  // 检查传入的 transition_map 指针是否为 NULL
  RCL_CHECK_FOR_NULL_WITH_MSG(
      transition_map, "transition_map pointer is null\n", return RCL_RET_INVALID_ARGUMENT);
  // 如果状态和转换指针都为 NULL，则将返回值设置为 RCL_RET_ERROR
  if (!transition_map->states && !transition_map->transitions) {
    is_initialized = RCL_RET_ERROR;
  }
  // 返回检查结果
  return is_initialized;
}

/**
 * @brief 清理生命周期转换映射结构体
 * 该函数释放给定的生命周期转换映射结构体中的所有资源，并将其重置为零。
 * @param[in,out] transition_map 要清理的生命周期转换映射结构体指针
 * @param[in] allocator 用于释放资源的分配器指针
 * @return RCL_RET_OK 如果成功，否则返回相应的错误代码
 */
rcl_ret_t rcl_lifecycle_transition_map_fini(
    rcl_lifecycle_transition_map_t* transition_map, const rcutils_allocator_t* allocator) {
  // 设置可能的错误返回值
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_ERROR);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  // 检查传入的 transition_map 指针是否为 NULL
  RCL_CHECK_FOR_NULL_WITH_MSG(
      transition_map, "transition_map pointer is null\n", return RCL_RET_INVALID_ARGUMENT);
  // 检查传入的 allocator 指针是否为 NULL
  RCL_CHECK_ALLOCATOR_WITH_MSG(
      allocator, "can't free transition map, no allocator given\n",
      return RCL_RET_INVALID_ARGUMENT);

  // 初始化函数返回值为 RCL_RET_OK
  rcl_ret_t fcn_ret = RCL_RET_OK;

  // 释放所有状态的有效转换资源
  for (unsigned int i = 0; i < transition_map->states_size; ++i) {
    if (transition_map->states[i].valid_transitions != NULL) {
      allocator->deallocate(transition_map->states[i].valid_transitions, allocator->state);
      transition_map->states[i].valid_transitions = NULL;
    }
  }
  // 释放主状态资源
  allocator->deallocate(transition_map->states, allocator->state);
  transition_map->states = NULL;
  transition_map->states_size = 0;
  // 释放转换资源
  allocator->deallocate(transition_map->transitions, allocator->state);
  transition_map->transitions = NULL;
  transition_map->transitions_size = 0;

  // 返回函数执行结果
  return fcn_ret;
}

/**
 * @brief 注册生命周期状态
 *
 * @param[in] transition_map 生命周期转换映射指针
 * @param[in] state 要注册的生命周期状态
 * @param[in] allocator 内存分配器指针
 * @return 返回rcl_ret_t类型的结果，成功返回RCL_RET_OK
 */
rcl_ret_t rcl_lifecycle_register_state(
    rcl_lifecycle_transition_map_t* transition_map,
    rcl_lifecycle_state_t state,
    const rcutils_allocator_t* allocator) {
  // 可以返回以下错误类型
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_LIFECYCLE_STATE_REGISTERED);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_BAD_ALLOC);

  // 检查transition_map是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(
      transition_map, "transition_map pointer is null\n", return RCL_RET_INVALID_ARGUMENT);
  // 检查状态是否已经注册
  if (rcl_lifecycle_get_state(transition_map, state.id) != NULL) {
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING("state %u is already registered\n", state.id);
    return RCL_RET_LIFECYCLE_STATE_REGISTERED;
  }

  // 检查内存分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT)
  // 为新的主状态分配内存
  unsigned int new_states_size = transition_map->states_size + 1;
  rcl_lifecycle_state_t* new_states = allocator->reallocate(
      transition_map->states, new_states_size * sizeof(rcl_lifecycle_state_t), allocator->state);
  // 检查新状态内存分配是否成功
  RCL_CHECK_FOR_NULL_WITH_MSG(
      new_states, "failed to reallocate memory for new states\n", return RCL_RET_BAD_ALLOC);
  // 更新状态映射
  transition_map->states_size = new_states_size;
  transition_map->states = new_states;
  transition_map->states[transition_map->states_size - 1] = state;

  return RCL_RET_OK;
}

/**
 * @brief 注册生命周期转换
 *
 * @param[in] transition_map 生命周期转换映射指针
 * @param[in] transition 要注册的生命周期转换
 * @param[in] allocator 内存分配器指针
 * @return 返回rcl_ret_t类型的结果，成功返回RCL_RET_OK
 */
rcl_ret_t rcl_lifecycle_register_transition(
    rcl_lifecycle_transition_map_t* transition_map,
    rcl_lifecycle_transition_t transition,
    const rcutils_allocator_t* allocator) {
  // 可以返回以下错误类型
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_ERROR);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_BAD_ALLOC);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_LIFECYCLE_STATE_NOT_REGISTERED);

  // 检查transition_map是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(
      transition_map, "transition_map pointer is null\n", return RCL_RET_INVALID_ARGUMENT);
  // 检查内存分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);

  // 检查起始状态是否已注册
  rcl_lifecycle_state_t* state = rcl_lifecycle_get_state(transition_map, transition.start->id);
  if (!state) {
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING("state %u is not registered\n", transition.start->id);
    return RCL_RET_LIFECYCLE_STATE_NOT_REGISTERED;
  }

  // 检查目标状态是否已注册
  rcl_lifecycle_state_t* goal = rcl_lifecycle_get_state(transition_map, transition.goal->id);
  if (!goal) {
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING("state %u is not registered\n", transition.goal->id);
    return RCL_RET_LIFECYCLE_STATE_NOT_REGISTERED;
  }
  // 尝试添加新的转换，如果失败则不更新映射
  unsigned int new_transitions_size = transition_map->transitions_size + 1;
  rcl_lifecycle_transition_t* new_transitions = allocator->reallocate(
      transition_map->transitions, new_transitions_size * sizeof(rcl_lifecycle_transition_t),
      allocator->state);
  if (!new_transitions) {
    RCL_SET_ERROR_MSG("failed to reallocate memory for new transitions");
    return RCL_RET_BAD_ALLOC;
  }
  // 更新转换映射
  transition_map->transitions_size = new_transitions_size;
  transition_map->transitions = new_transitions;
  // 将新转换添加到数组末尾
  transition_map->transitions[transition_map->transitions_size - 1] = transition;

  // 我们需要再次复制一次transitons到实际状态
  // 因为我们不能只分配指针。每当我们添加新的转换并重新分配内存时，此指针会失效。
  unsigned int new_valid_transitions_size = state->valid_transition_size + 1;
  rcl_lifecycle_transition_t* new_valid_transitions = allocator->reallocate(
      state->valid_transitions, new_valid_transitions_size * sizeof(rcl_lifecycle_transition_t),
      allocator->state);
  if (!new_valid_transitions) {
    RCL_SET_ERROR_MSG("failed to reallocate memory for new transitions on state");
    return RCL_RET_BAD_ALLOC;
  }
  // 更新有效转换映射
  state->valid_transition_size = new_valid_transitions_size;
  state->valid_transitions = new_valid_transitions;

  // 将新转换添加到数组末尾
  state->valid_transitions[state->valid_transition_size - 1] = transition;

  return RCL_RET_OK;
}

/**
 * @brief 获取指定状态ID的生命周期状态
 *
 * @param transition_map 生命周期转换映射表指针
 * @param state_id 要查找的状态ID
 * @return rcl_lifecycle_state_t* 指向找到的生命周期状态的指针，如果未找到则返回NULL
 */
rcl_lifecycle_state_t* rcl_lifecycle_get_state(
    rcl_lifecycle_transition_map_t* transition_map, unsigned int state_id) {
  // 检查传入的transition_map是否为空，如果为空则输出错误信息并返回NULL
  RCL_CHECK_FOR_NULL_WITH_MSG(transition_map, "transition_map pointer is null\n", return NULL);

  // 遍历transition_map中的所有状态
  for (unsigned int i = 0; i < transition_map->states_size; ++i) {
    // 如果找到与给定state_id匹配的状态，则返回该状态的指针
    if (transition_map->states[i].id == state_id) {
      return &transition_map->states[i];
    }
  }

  // 如果没有找到匹配的状态，则返回NULL
  return NULL;
}

/**
 * @brief 获取指定转换ID的生命周期转换
 *
 * @param transition_map 生命周期转换映射表指针
 * @param transition_id 要查找的转换ID
 * @return rcl_lifecycle_transition_t* 指向找到的生命周期转换的指针，如果未找到则返回NULL
 */
rcl_lifecycle_transition_t* rcl_lifecycle_get_transitions(
    rcl_lifecycle_transition_map_t* transition_map, unsigned int transition_id) {
  // 检查传入的transition_map是否为空，如果为空则输出错误信息并返回NULL
  RCL_CHECK_FOR_NULL_WITH_MSG(transition_map, "transition_map pointer is null\n", return NULL);

  // 遍历transition_map中的所有转换
  for (unsigned int i = 0; i < transition_map->transitions_size; ++i) {
    // 如果找到与给定transition_id匹配的转换，则返回该转换的指针
    if (transition_map->transitions[i].id == transition_id) {
      return &transition_map->transitions[i];
    }
  }

  // 如果没有找到匹配的转换，则返回NULL
  return NULL;
}

#ifdef __cplusplus
}
#endif
