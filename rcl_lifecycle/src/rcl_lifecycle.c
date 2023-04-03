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

#include "rcl_lifecycle/rcl_lifecycle.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./com_interface.h"
#include "rcl/error_handling.h"
#include "rcl/rcl.h"
#include "rcl_lifecycle/default_state_machine.h"
#include "rcl_lifecycle/transition_map.h"
#include "rcutils/logging_macros.h"
#include "rcutils/macros.h"
#include "rcutils/strdup.h"
#include "tracetools/tracetools.h"

/**
 * @brief 获取一个初始化为零的生命周期状态对象
 *
 * @return 返回一个初始化为零的 rcl_lifecycle_state_t 对象
 */
rcl_lifecycle_state_t rcl_lifecycle_get_zero_initialized_state()
{
  rcl_lifecycle_state_t state;      // 定义一个生命周期状态对象
  state.id = 0;                     // 将状态 ID 设置为 0
  state.label = NULL;               // 将状态标签设置为空
  state.valid_transitions = NULL;   // 将有效转换数组设置为空
  state.valid_transition_size = 0;  // 将有效转换数组大小设置为 0
  return state;                     // 返回初始化为零的状态对象
}

/**
 * @brief 初始化生命周期状态对象
 *
 * @param[out] state 生命周期状态对象指针
 * @param[in] id 状态 ID
 * @param[in] label 状态标签
 * @param[in] allocator 分配器指针
 * @return 返回操作结果，成功返回 RCL_RET_OK
 */
rcl_ret_t rcl_lifecycle_state_init(
  rcl_lifecycle_state_t * state, uint8_t id, const char * label, const rcl_allocator_t * allocator)
{
  // 检查分配器是否存在
  RCL_CHECK_ALLOCATOR_WITH_MSG(
    allocator, "can't initialize state, no allocator given\n", return RCL_RET_INVALID_ARGUMENT);

  // 检查状态指针是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(state, "state pointer is null\n", return RCL_RET_INVALID_ARGUMENT);

  // 检查状态标签是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(label, "State label is null\n", return RCL_RET_INVALID_ARGUMENT);

  state->id = id;  // 设置状态 ID
  // 使用分配器复制标签字符串
  state->label = rcutils_strndup(label, strlen(label), *allocator);
  // 检查标签复制是否成功
  RCL_CHECK_FOR_NULL_WITH_MSG(
    state->label, "failed to duplicate label for rcl_lifecycle_state_t\n", return RCL_RET_ERROR);

  return RCL_RET_OK;  // 返回操作成功
}

/**
 * @brief 清理生命周期状态对象
 *
 * @param[out] state 生命周期状态对象指针
 * @param[in] allocator 分配器指针
 * @return 返回操作结果，成功返回 RCL_RET_OK
 */
rcl_ret_t rcl_lifecycle_state_fini(rcl_lifecycle_state_t * state, const rcl_allocator_t * allocator)
{
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);

  // 检查分配器是否存在
  RCL_CHECK_ALLOCATOR_WITH_MSG(
    allocator, "can't free state, no allocator given\n", return RCL_RET_INVALID_ARGUMENT);
  // 如果状态指针为空，则直接返回成功
  if (!state) {
    return RCL_RET_OK;
  }

  // 如果状态标签不为空，则使用分配器释放内存
  if (state->label) {
    allocator->deallocate((char *)state->label, allocator->state);
    state->label = NULL;  // 将状态标签设置为空
  }

  return RCL_RET_OK;  // 返回操作成功
}

/**
 * @brief 获取一个初始化为零的生命周期转换对象
 *
 * @return 返回一个初始化为零的 rcl_lifecycle_transition_t 对象
 */
rcl_lifecycle_transition_t rcl_lifecycle_get_zero_initialized_transition()
{
  rcl_lifecycle_transition_t transition;  // 定义一个生命周期转换对象
  transition.id = 0;                      // 将转换 ID 设置为 0
  transition.label = NULL;                // 将转换标签设置为空
  transition.start = NULL;                // 将起始状态设置为空
  transition.goal = NULL;                 // 将目标状态设置为空
  return transition;                      // 返回初始化为零的转换对象
}

/**
 * @brief 初始化生命周期转换
 *
 * @param[out] transition 生命周期转换指针
 * @param[in] id 转换的唯一标识符
 * @param[in] label 转换的标签
 * @param[in] start 起始状态指针
 * @param[in] goal 目标状态指针
 * @param[in] allocator 分配器指针
 * @return 返回rcl_ret_t类型的结果，成功返回RCL_RET_OK，否则返回相应的错误代码
 */
rcl_ret_t rcl_lifecycle_transition_init(
  rcl_lifecycle_transition_t * transition, unsigned int id, const char * label,
  rcl_lifecycle_state_t * start, rcl_lifecycle_state_t * goal, const rcl_allocator_t * allocator)
{
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(
    allocator, "can't initialize transition, no allocator given\n",
    return RCL_RET_INVALID_ARGUMENT);

  // 检查transition指针是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(
    transition, "transition pointer is null\n", return RCL_RET_INVALID_ARGUMENT);

  // 检查label指针是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(label, "label pointer is null\n", return RCL_RET_INVALID_ARGUMENT);

  // 设置起始状态和目标状态
  transition->start = start;
  transition->goal = goal;

  // 设置转换ID和标签
  transition->id = id;
  transition->label = rcutils_strndup(label, strlen(label), *allocator);
  // 检查标签是否复制成功
  RCL_CHECK_FOR_NULL_WITH_MSG(
    transition->label, "failed to duplicate label for rcl_lifecycle_transition_t\n",
    return RCL_RET_ERROR);

  // 返回成功
  return RCL_RET_OK;
}

/**
 * @brief 结束生命周期转换
 *
 * @param[out] transition 生命周期转换指针
 * @param[in] allocator 分配器指针
 * @return 返回rcl_ret_t类型的结果，成功返回RCL_RET_OK，否则返回相应的错误代码
 */
rcl_ret_t rcl_lifecycle_transition_fini(
  rcl_lifecycle_transition_t * transition, const rcl_allocator_t * allocator)
{
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(
    allocator, "can't finalize transition, no allocator given\n", return RCL_RET_INVALID_ARGUMENT);
  // 如果transition为空，则已经完成
  if (!transition) {
    return RCL_RET_OK;
  }

  rcl_ret_t ret = RCL_RET_OK;

  // 结束起始状态并释放内存
  if (rcl_lifecycle_state_fini(transition->start, allocator) != RCL_RET_OK) {
    ret = RCL_RET_ERROR;
  }
  allocator->deallocate(transition->start, allocator->state);
  transition->start = NULL;

  // 结束目标状态并释放内存
  if (rcl_lifecycle_state_fini(transition->goal, allocator) != RCL_RET_OK) {
    ret = RCL_RET_ERROR;
  }
  allocator->deallocate(transition->goal, allocator->state);
  transition->goal = NULL;

  // 释放标签内存
  allocator->deallocate((char *)transition->label, allocator->state);
  transition->label = NULL;

  // 返回结果
  return ret;
}

/**
 * @brief 获取默认的生命周期状态机选项
 *
 * @return 返回rcl_lifecycle_state_machine_options_t类型的默认选项
 */
rcl_lifecycle_state_machine_options_t rcl_lifecycle_get_default_state_machine_options()
{
  rcl_lifecycle_state_machine_options_t options;
  // 启用通信接口
  options.enable_com_interface = true;
  // 初始化默认状态
  options.initialize_default_states = true;
  // 使用默认分配器
  options.allocator = rcl_get_default_allocator();

  // 返回默认选项
  return options;
}

/**
 * @brief 获取一个零初始化的状态机
 *
 * 该函数用于创建一个零初始化的生命周期状态机。
 *
 * @return 返回一个零初始化的生命周期状态机
 */
rcl_lifecycle_state_machine_t rcl_lifecycle_get_zero_initialized_state_machine()
{
  // 创建一个生命周期状态机变量
  rcl_lifecycle_state_machine_t state_machine;

  // 将当前状态设置为 NULL
  state_machine.current_state = NULL;

  // 获取并设置一个零初始化的转换映射
  state_machine.transition_map = rcl_lifecycle_get_zero_initialized_transition_map();

  // 获取并设置一个零初始化的通信接口
  state_machine.com_interface = rcl_lifecycle_get_zero_initialized_com_interface();

  // 获取并设置默认的状态机选项
  state_machine.options = rcl_lifecycle_get_default_state_machine_options();

  // 返回零初始化的状态机
  return state_machine;
}

/**
 * @brief 初始化生命周期状态机
 *
 * @param[out] state_machine 指向要初始化的生命周期状态机的指针
 * @param[in] node_handle 指向节点句柄的指针
 * @param[in] ts_pub_notify 指向发布通知消息类型支持的指针
 * @param[in] ts_srv_change_state 指向更改状态服务类型支持的指针
 * @param[in] ts_srv_get_state 指向获取状态服务类型支持的指针
 * @param[in] ts_srv_get_available_states 指向获取可用状态服务类型支持的指针
 * @param[in] ts_srv_get_available_transitions 指向获取可用转换服务类型支持的指针
 * @param[in] ts_srv_get_transition_graph 指向获取转换图服务类型支持的指针
 * @param[in] state_machine_options 指向状态机选项的指针
 *
 * @return 返回初始化结果，成功返回 RCL_RET_OK，否则返回相应的错误代码
 */
rcl_ret_t rcl_lifecycle_state_machine_init(
  rcl_lifecycle_state_machine_t * state_machine, rcl_node_t * node_handle,
  const rosidl_message_type_support_t * ts_pub_notify,
  const rosidl_service_type_support_t * ts_srv_change_state,
  const rosidl_service_type_support_t * ts_srv_get_state,
  const rosidl_service_type_support_t * ts_srv_get_available_states,
  const rosidl_service_type_support_t * ts_srv_get_available_transitions,
  const rosidl_service_type_support_t * ts_srv_get_transition_graph,
  const rcl_lifecycle_state_machine_options_t * state_machine_options)
{
  // 检查状态机是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(
    state_machine, "State machine is null\n", return RCL_RET_INVALID_ARGUMENT);

  // 检查节点句柄是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(
    node_handle, "Node handle is null\n", return RCL_RET_INVALID_ARGUMENT);

  // 检查分配器是否为空
  RCL_CHECK_ALLOCATOR_WITH_MSG(
    &state_machine_options->allocator, "can't initialize state machine, no allocator given\n",
    return RCL_RET_INVALID_ARGUMENT);

  // 设置状态机选项
  state_machine->options = *state_machine_options;

  // 如果启用了完整的通信接口（包括发布器和服务）
  if (state_machine->options.enable_com_interface) {
    // 初始化通信接口
    rcl_ret_t ret = rcl_lifecycle_com_interface_init(
      &state_machine->com_interface, node_handle, ts_pub_notify, ts_srv_change_state,
      ts_srv_get_state, ts_srv_get_available_states, ts_srv_get_available_transitions,
      ts_srv_get_transition_graph);
    // 如果初始化失败，返回错误
    if (ret != RCL_RET_OK) {
      return RCL_RET_ERROR;
    }
  } else {
    // 否则，仅初始化发布器
    rcl_ret_t ret = rcl_lifecycle_com_interface_publisher_init(
      &state_machine->com_interface, node_handle, ts_pub_notify);
    // 如果初始化失败，返回错误
    if (ret != RCL_RET_OK) {
      return RCL_RET_ERROR;
    }
  }

  // 如果启用了默认状态的初始化
  if (state_machine->options.initialize_default_states) {
    // 初始化默认状态机
    rcl_ret_t ret =
      rcl_lifecycle_init_default_state_machine(state_machine, &state_machine->options.allocator);
    // 如果初始化失败
    if (ret != RCL_RET_OK) {
      // 可能已分配内存，因此需要调用 fini
      ret = rcl_lifecycle_state_machine_fini(state_machine, node_handle);
      // 如果释放失败，输出警告并泄漏内存
      if (ret != RCL_RET_OK) {
        RCUTILS_SAFE_FWRITE_TO_STDERR(
          "Freeing state machine failed while handling a previous error. Leaking memory!\n");
      }
      // 返回错误
      return RCL_RET_ERROR;
    }
  }

  // 添加跟踪点
  TRACEPOINT(
    rcl_lifecycle_state_machine_init, (const void *)node_handle, (const void *)state_machine);

  // 返回成功
  return RCL_RET_OK;
}

/**
 * @brief 销毁生命周期状态机
 *
 * @param[in] state_machine 生命周期状态机指针
 * @param[in] node_handle 节点句柄指针
 * @return 返回 rcl_ret_t 类型的结果，成功返回 RCL_RET_OK，失败返回相应的错误代码
 */
rcl_ret_t rcl_lifecycle_state_machine_fini(
  rcl_lifecycle_state_machine_t * state_machine, rcl_node_t * node_handle)
{
  // 初始化函数返回值为 RCL_RET_OK
  rcl_ret_t fcn_ret = RCL_RET_OK;

  // 如果通信接口销毁失败，则设置错误信息并更新函数返回值
  if (rcl_lifecycle_com_interface_fini(&state_machine->com_interface, node_handle) != RCL_RET_OK) {
    rcl_error_string_t error_string = rcl_get_error_string();
    rcutils_reset_error();
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "could not free lifecycle com interface. Leaking memory!\n%s", error_string.str);
    fcn_ret = RCL_RET_ERROR;
  }

  // 如果转换映射销毁失败，则设置错误信息并更新函数返回值
  if (
    rcl_lifecycle_transition_map_fini(
      &state_machine->transition_map, &state_machine->options.allocator) != RCL_RET_OK) {
    rcl_error_string_t error_string = rcl_get_error_string();
    rcutils_reset_error();
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "could not free lifecycle transition map. Leaking memory!\n%s", error_string.str);
    fcn_ret = RCL_RET_ERROR;
  }

  // 返回函数执行结果
  return fcn_ret;
}

/**
 * @brief 检查生命周期状态机是否已初始化
 *
 * @param[in] state_machine 生命周期状态机指针
 * @return 返回 rcl_ret_t 类型的结果，成功返回 RCL_RET_OK，失败返回相应的错误代码
 */
rcl_ret_t rcl_lifecycle_state_machine_is_initialized(
  const rcl_lifecycle_state_machine_t * state_machine)
{
  // 如果启用了通信接口，则检查 get_state 和 change_state 服务是否为空
  if (state_machine->options.enable_com_interface) {
    RCL_CHECK_FOR_NULL_WITH_MSG(
      state_machine->com_interface.srv_get_state.impl, "get_state service is null\n",
      return RCL_RET_INVALID_ARGUMENT);

    RCL_CHECK_FOR_NULL_WITH_MSG(
      state_machine->com_interface.srv_change_state.impl, "change_state service is null\n",
      return RCL_RET_INVALID_ARGUMENT);
  }

  // 检查转换映射是否已初始化
  if (rcl_lifecycle_transition_map_is_initialized(&state_machine->transition_map) != RCL_RET_OK) {
    RCL_SET_ERROR_MSG("transition map is null");
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 返回成功
  return RCL_RET_OK;
}

/**
 * @brief 根据 ID 获取生命周期状态的转换
 *
 * @param[in] state 生命周期状态指针
 * @param[in] id 转换的 ID
 * @return 返回 rcl_lifecycle_transition_t 类型的指针，找到则返回对应指针，未找到则返回 NULL
 */
const rcl_lifecycle_transition_t * rcl_lifecycle_get_transition_by_id(
  const rcl_lifecycle_state_t * state, uint8_t id)
{
  // 检查状态指针是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(state, "state pointer is null", return NULL);

  // 遍历有效转换，寻找匹配的 ID
  for (unsigned int i = 0; i < state->valid_transition_size; ++i) {
    if (state->valid_transitions[i].id == id) {
      return &state->valid_transitions[i];
    }
  }

  // 如果未找到匹配的转换，记录警告信息
  RCUTILS_LOG_WARN_NAMED(
    ROS_PACKAGE_NAME, "No transition matching %d found for current state %s", id, state->label);

  // 返回 NULL
  return NULL;
}

/**
 * @brief 根据标签获取生命周期状态的转换
 *
 * @param[in] state 生命周期状态指针
 * @param[in] label 转换的标签
 * @return 返回 rcl_lifecycle_transition_t 类型的指针，找到则返回对应指针，未找到则返回 NULL
 */
const rcl_lifecycle_transition_t * rcl_lifecycle_get_transition_by_label(
  const rcl_lifecycle_state_t * state, const char * label)
{
  // 检查状态指针是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(state, "state pointer is null", return NULL);

  // 遍历有效转换，寻找匹配的标签
  for (unsigned int i = 0; i < state->valid_transition_size; ++i) {
    if (strcmp(state->valid_transitions[i].label, label) == 0) {
      return &state->valid_transitions[i];
    }
  }

  // 如果未找到匹配的转换，记录警告信息
  RCUTILS_LOG_WARN_NAMED(
    ROS_PACKAGE_NAME, "No transition matching %s found for current state %s", label, state->label);

  // 返回 NULL
  return NULL;
}

/**
 * @brief 触发状态机的转换
 *
 * @param state_machine 指向状态机的指针
 * @param transition 指向要触发的转换的指针
 * @param publish_notification 是否发布通知
 * @return rcl_ret_t 返回执行结果
 */
rcl_ret_t _trigger_transition(
  rcl_lifecycle_state_machine_t * state_machine, const rcl_lifecycle_transition_t * transition,
  bool publish_notification)
{
  // 如果我们有一个错误的转换指针
  RCL_CHECK_FOR_NULL_WITH_MSG(
    transition, "Transition is not registered.", return RCL_RET_INVALID_ARGUMENT);

  // 检查目标状态是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(
    transition->goal, "No valid goal is set.", return RCL_RET_INVALID_ARGUMENT);
  // 更新当前状态为目标状态
  state_machine->current_state = transition->goal;

  // 如果需要发布通知
  if (publish_notification) {
    // 发布通知
    rcl_ret_t fcn_ret = rcl_lifecycle_com_interface_publish_notification(
      &state_machine->com_interface, transition->start, state_machine->current_state);
    // 如果发布失败
    if (fcn_ret != RCL_RET_OK) {
      rcl_error_string_t error_string = rcl_get_error_string();
      rcutils_reset_error();
      RCL_SET_ERROR_MSG_WITH_FORMAT_STRING("Could not publish transition: %s", error_string.str);
      return RCL_RET_ERROR;
    }
  }

  // 记录跟踪点
  TRACEPOINT(
    rcl_lifecycle_transition, (const void *)state_machine, transition->start->label,
    state_machine->current_state->label);
  return RCL_RET_OK;
}

/**
 * @brief 通过ID触发状态机的转换
 *
 * @param state_machine 指向状态机的指针
 * @param id 转换的ID
 * @param publish_notification 是否发布通知
 * @return rcl_ret_t 返回执行结果
 */
rcl_ret_t rcl_lifecycle_trigger_transition_by_id(
  rcl_lifecycle_state_machine_t * state_machine, uint8_t id, bool publish_notification)
{
  // 检查状态机指针是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(
    state_machine, "state machine pointer is null.", return RCL_RET_INVALID_ARGUMENT);

  // 根据ID获取转换
  const rcl_lifecycle_transition_t * transition =
    rcl_lifecycle_get_transition_by_id(state_machine->current_state, id);

  // 触发转换
  return _trigger_transition(state_machine, transition, publish_notification);
}

/**
 * @brief 通过标签触发状态机的转换
 *
 * @param state_machine 指向状态机的指针
 * @param label 转换的标签
 * @param publish_notification 是否发布通知
 * @return rcl_ret_t 返回执行结果
 */
rcl_ret_t rcl_lifecycle_trigger_transition_by_label(
  rcl_lifecycle_state_machine_t * state_machine, const char * label, bool publish_notification)
{
  // 检查状态机指针是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(
    state_machine, "state machine pointer is null.", return RCL_RET_INVALID_ARGUMENT);

  // 根据标签获取转换
  const rcl_lifecycle_transition_t * transition =
    rcl_lifecycle_get_transition_by_label(state_machine->current_state, label);

  // 触发转换
  return _trigger_transition(state_machine, transition, publish_notification);
}

/**
 * @brief 打印状态机信息
 *
 * @param state_machine 指向状态机的指针
 */
void rcl_print_state_machine(const rcl_lifecycle_state_machine_t * state_machine)
{
  const rcl_lifecycle_transition_map_t * map = &state_machine->transition_map;
  // 遍历所有状态
  for (size_t i = 0; i < map->states_size; ++i) {
    // 打印主要状态和有效转换数量
    RCUTILS_LOG_INFO_NAMED(
      ROS_PACKAGE_NAME, "Primary State: %s(%u)\n# of valid transitions: %u", map->states[i].label,
      map->states[i].id, map->states[i].valid_transition_size);
    // 遍历并打印每个状态的有效转换
    for (size_t j = 0; j < map->states[i].valid_transition_size; ++j) {
      RCUTILS_LOG_INFO_NAMED(
        ROS_PACKAGE_NAME, "\tNode %s: Transition: %s", map->states[i].label,
        map->states[i].valid_transitions[j].label);
    }
  }
}

#ifdef __cplusplus
}
#endif  // extern "C"
