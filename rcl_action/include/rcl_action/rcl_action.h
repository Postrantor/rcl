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

/** \mainpage rcl: ROS 动作的通用功能
 *
 * `rcl_action` 提供了一个纯 C 语言实现的 ROS 动作（\b action）概念。
 * 它基于 `rcl` 中主题（topics）和服务（services）的实现。
 *
 * `rcl_action` 包含以下 ROS 动作实体的函数和结构：
 *
 * - 动作客户端（Action client）
 *   - rcl_action/action_client.h
 * - 动作服务器（Action server）
 *   - rcl_action/action_server.h
 * - 目标句柄（Goal handle）
 *   - rcl_action/goal_handle.h
 * - 目标状态机（Goal state machine）
 *   - rcl_action/goal_state_machine.h
 *
 * 它还具有一些等待和操作这些实体所必需的机制：
 *
 * - 等待集合，用于等待动作客户端和动作服务器准备就绪
 *   - rcl_action/wait.h
 *
 * 一些有用的抽象和实用程序：
 *
 * - 返回代码和其他类型
 *   - rcl_action/types.h
 */

#ifndef RCL_ACTION__RCL_ACTION_H_
#define RCL_ACTION__RCL_ACTION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl_action/action_client.h"
#include "rcl_action/action_server.h"
#include "rcl_action/default_qos.h"
#include "rcl_action/goal_handle.h"
#include "rcl_action/goal_state_machine.h"
#include "rcl_action/graph.h"
#include "rcl_action/types.h"
#include "rcl_action/wait.h"

#ifdef __cplusplus
}
#endif

#endif  // RCL_ACTION__RCL_ACTION_H_
