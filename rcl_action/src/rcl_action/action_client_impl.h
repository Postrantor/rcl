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

#ifndef RCL_ACTION__ACTION_CLIENT_IMPL_H_
#define RCL_ACTION__ACTION_CLIENT_IMPL_H_

#include "rcl/rcl.h"
#include "rcl_action/types.h"

/**
 * @struct rcl_action_client_impl_s
 * @brief 一个用于实现ROS action客户端的结构体。
 */
typedef struct rcl_action_client_impl_s
{
  rcl_client_t goal_client;                  ///< 发送目标请求的客户端
  rcl_client_t cancel_client;                ///< 发送取消请求的客户端
  rcl_client_t result_client;                ///< 获取结果请求的客户端
  rcl_subscription_t feedback_subscription;  ///< 订阅反馈信息的订阅器
  rcl_subscription_t status_subscription;    ///< 订阅状态信息的订阅器
  rcl_action_client_options_t options;       ///< action客户端选项
  char * action_name;                        ///< action名称

  // Wait set records
  size_t wait_set_goal_client_index;            ///< 等待集合中goal_client的索引
  size_t wait_set_cancel_client_index;          ///< 等待集合中cancel_client的索引
  size_t wait_set_result_client_index;          ///< 等待集合中result_client的索引
  size_t wait_set_feedback_subscription_index;  ///< 等待集合中feedback_subscription的索引
  size_t wait_set_status_subscription_index;    ///< 等待集合中status_subscription的索引
} rcl_action_client_impl_t;

#endif  // RCL_ACTION__ACTION_CLIENT_IMPL_H_
