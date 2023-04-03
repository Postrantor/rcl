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

#ifndef RCL__PUBLISHER_IMPL_H_
#define RCL__PUBLISHER_IMPL_H_

#include "rcl/publisher.h"
#include "rmw/rmw.h"

/**
 * @struct rcl_publisher_impl_s
 * @brief ROS2中的rcl发布者实现结构体，包含了发布者的选项、QoS配置、上下文和底层RMW句柄。
 */
struct rcl_publisher_impl_s {
  /**
   * @brief 发布者选项，包括分配器、主题名等。
   */
  rcl_publisher_options_t options;

  /**
   * @brief 实际使用的QoS配置，包括可靠性、持久性等。
   */
  rmw_qos_profile_t actual_qos;

  /**
   * @brief 指向rcl_context_t类型的指针，用于存储ROS2节点的上下文信息。
   */
  rcl_context_t* context;

  /**
   * @brief 指向rmw_publisher_t类型的指针，用于存储底层ROS中间件（如DDS）的发布者句柄。
   */
  rmw_publisher_t* rmw_handle;
};

#endif  // RCL__PUBLISHER_IMPL_H_
