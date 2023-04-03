// Copyright 2015 Open Source Robotics Foundation, Inc.
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

#ifndef RCL__SUBSCRIPTION_IMPL_H_
#define RCL__SUBSCRIPTION_IMPL_H_

#include "rcl/subscription.h"
#include "rmw/rmw.h"

/**
 * @struct rcl_subscription_impl_s
 * @brief ROS2中的rcl订阅器实现结构体，包含订阅器选项、QoS配置和底层RMW订阅器句柄。
 */
struct rcl_subscription_impl_s {
  /**
   * @var rcl_subscription_options_t options
   * @brief 订阅器选项，包括分配器、节点名等。
   */
  rcl_subscription_options_t options;

  /**
   * @var rmw_qos_profile_t actual_qos
   * @brief 实际使用的QoS配置，包括可靠性、历史深度等。
   */
  rmw_qos_profile_t actual_qos;

  /**
   * @var rmw_subscription_t * rmw_handle
   * @brief 底层RMW订阅器句柄，用于与ROS2中间件通信。
   */
  rmw_subscription_t* rmw_handle;
};

#endif  // RCL__SUBSCRIPTION_IMPL_H_
