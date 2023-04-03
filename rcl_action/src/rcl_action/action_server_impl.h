// Copyright 2020 Open Source Robotics Foundation, Inc.
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

#ifndef RCL_ACTION__ACTION_SERVER_IMPL_H_
#define RCL_ACTION__ACTION_SERVER_IMPL_H_

#include "rcl/rcl.h"
#include "rcl_action/types.h"

/// @brief 内部 rcl_action 实现结构体
typedef struct rcl_action_server_impl_s
{
  rcl_service_t goal_service; ///< 目标服务
  rcl_service_t cancel_service; ///< 取消服务
  rcl_service_t result_service; ///< 结果服务
  rcl_publisher_t feedback_publisher; ///< 反馈发布器
  rcl_publisher_t status_publisher; ///< 状态发布器
  rcl_timer_t expire_timer; ///< 过期定时器
  char * action_name; ///< 动作名称
  rcl_action_server_options_t options; ///< 动作服务器选项
  // Array of goal handles
  rcl_action_goal_handle_t ** goal_handles; ///< 目标句柄数组
  size_t num_goal_handles; ///< 目标句柄数量
  // Clock
  rcl_clock_t * clock; ///< 时钟
  // Wait set records
  size_t wait_set_goal_service_index; ///< 等待集目标服务索引
  size_t wait_set_cancel_service_index; ///< 等待集取消服务索引
  size_t wait_set_result_service_index; ///< 等待集结果服务索引
  size_t wait_set_expire_timer_index; ///< 等待集过期定时器索引
} rcl_action_server_impl_t;


#endif  // RCL_ACTION__ACTION_SERVER_IMPL_H_
