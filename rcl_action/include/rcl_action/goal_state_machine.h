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

#ifndef RCL_ACTION__GOAL_STATE_MACHINE_H_
#define RCL_ACTION__GOAL_STATE_MACHINE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl_action/types.h"
#include "rcl_action/visibility_control.h"

/// 转换目标状态。
/**
 * 根据给定的目标状态和目标事件，返回下一个状态。
 *
 * \param[in] state 要转换的状态
 * \param[in] event 触发转换的事件
 * \return 如果转换有效，则返回下一个目标状态，或者
 * \return 如果转换无效或发生错误，则返回 `GOAL_STATE_UNKNOWN`
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_action_goal_state_t rcl_action_transition_goal_state(
  const rcl_action_goal_state_t state, const rcl_action_goal_event_t event);

#ifdef __cplusplus
}
#endif

#endif  // RCL_ACTION__GOAL_STATE_MACHINE_H_
