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

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl_action/goal_state_machine.h"

// 定义一个函数指针类型，用于处理目标状态转换事件
typedef rcl_action_goal_state_t (*rcl_action_goal_event_handler)(
  rcl_action_goal_state_t, rcl_action_goal_event_t);

// 执行事件处理器
/**
 * @brief 处理执行事件
 * @param state 当前的目标状态
 * @param event 要处理的事件
 * @return 返回新的目标状态
 */
rcl_action_goal_state_t _execute_event_handler(
  rcl_action_goal_state_t state, rcl_action_goal_event_t event)
{
  // 检查输入参数是否正确
  assert(GOAL_STATE_ACCEPTED == state);
  assert(GOAL_EVENT_EXECUTE == event);
  // 避免未使用警告，但保留断言以便调试
  (void)state;
  (void)event;
  // 返回新的目标状态
  return GOAL_STATE_EXECUTING;
}

// 取消目标事件处理器
/**
 * @brief 处理取消目标事件
 * @param state 当前的目标状态
 * @param event 要处理的事件
 * @return 返回新的目标状态
 */
rcl_action_goal_state_t _cancel_goal_event_handler(
  rcl_action_goal_state_t state, rcl_action_goal_event_t event)
{
  // 检查输入参数是否正确
  assert(GOAL_STATE_ACCEPTED == state || GOAL_STATE_EXECUTING == state);
  assert(GOAL_EVENT_CANCEL_GOAL == event);
  // 避免未使用警告，但保留断言以便调试
  (void)state;
  (void)event;
  // 返回新的目标状态
  return GOAL_STATE_CANCELING;
}

// 成功事件处理器
/**
 * @brief 处理成功事件
 * @param state 当前的目标状态
 * @param event 要处理的事件
 * @return 返回新的目标状态
 */
rcl_action_goal_state_t _succeed_event_handler(
  rcl_action_goal_state_t state, rcl_action_goal_event_t event)
{
  // 检查输入参数是否正确
  assert(GOAL_STATE_EXECUTING == state || GOAL_STATE_CANCELING == state);
  assert(GOAL_EVENT_SUCCEED == event);
  // 避免未使用警告，但保留断言以便调试
  (void)state;
  (void)event;
  // 返回新的目标状态
  return GOAL_STATE_SUCCEEDED;
}

// 中止事件处理器
/**
 * @brief 处理中止事件
 * @param state 当前的目标状态
 * @param event 要处理的事件
 * @return 返回新的目标状态
 */
rcl_action_goal_state_t _abort_event_handler(
  rcl_action_goal_state_t state, rcl_action_goal_event_t event)
{
  // 检查输入参数是否正确
  assert(GOAL_STATE_EXECUTING == state || GOAL_STATE_CANCELING == state);
  assert(GOAL_EVENT_ABORT == event);
  // 避免未使用警告，但保留断言以便调试
  (void)state;
  (void)event;
  // 返回新的目标状态
  return GOAL_STATE_ABORTED;
}

// 取消事件处理器
/**
 * @brief 处理取消事件
 * @param state 当前的目标状态
 * @param event 要处理的事件
 * @return 返回新的目标状态
 */
rcl_action_goal_state_t _canceled_event_handler(
  rcl_action_goal_state_t state, rcl_action_goal_event_t event)
{
  // 检查输入参数是否正确
  assert(GOAL_STATE_CANCELING == state);
  assert(GOAL_EVENT_CANCELED == event);
  // 避免未使用警告，但保留断言以便调试
  (void)state;
  (void)event;
  // 返回新的目标状态
  return GOAL_STATE_CANCELED;
}

// 目标状态转换映射
static rcl_action_goal_event_handler
  _goal_state_transition_map[GOAL_STATE_NUM_STATES][GOAL_EVENT_NUM_EVENTS] = {
    [GOAL_STATE_ACCEPTED] =
      {
        [GOAL_EVENT_EXECUTE] = _execute_event_handler,
        [GOAL_EVENT_CANCEL_GOAL] = _cancel_goal_event_handler,
      },
    [GOAL_STATE_EXECUTING] =
      {
        [GOAL_EVENT_CANCEL_GOAL] = _cancel_goal_event_handler,
        [GOAL_EVENT_SUCCEED] = _succeed_event_handler,
        [GOAL_EVENT_ABORT] = _abort_event_handler,
      },
    [GOAL_STATE_CANCELING] =
      {
        [GOAL_EVENT_SUCCEED] = _succeed_event_handler,
        [GOAL_EVENT_ABORT] = _abort_event_handler,
        [GOAL_EVENT_CANCELED] = _canceled_event_handler,
      },
};

/**
 * @brief 根据给定的状态和事件进行目标状态转换
 * @param state 当前的目标状态
 * @param event 要处理的事件
 * @return 返回新的目标状态，如果无法转换则返回 GOAL_STATE_UNKNOWN
 */
rcl_action_goal_state_t rcl_action_transition_goal_state(
  const rcl_action_goal_state_t state, const rcl_action_goal_event_t event)
{
  // 检查输入参数是否有效
  if (state < 0 || state >= GOAL_STATE_NUM_STATES || event >= GOAL_EVENT_NUM_EVENTS) {
    return GOAL_STATE_UNKNOWN;
  }
  // 获取对应的事件处理器
  rcl_action_goal_event_handler handler = _goal_state_transition_map[state][event];
  // 如果事件处理器为空，则返回未知状态
  if (NULL == handler) {
    return GOAL_STATE_UNKNOWN;
  }
  // 调用事件处理器并返回新的目标状态
  return handler(state, event);
}

#ifdef __cplusplus
}
#endif
