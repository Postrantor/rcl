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

#ifndef RCL__EVENT_IMPL_H_
#define RCL__EVENT_IMPL_H_

#include "rcl/event.h"
#include "rmw/rmw.h"

/** \file rcl_event_impl_s.h
 *  \brief 文件描述 (File description)
 */

/**
 * \struct rcl_event_impl_s
 * \brief ROS2 RCL 事件实现结构体 (ROS2 RCL event implementation structure)
 */
struct rcl_event_impl_s {
  /**
   * \brief RMW 事件句柄 (RMW event handle)
   *
   * 用于与底层中间件通信的事件句柄 (Event handle for communication with the underlying middleware)
   */
  rmw_event_t rmw_handle;

  /**
   * \brief 分配器 (Allocator)
   *
   * 用于内存管理的分配器 (Allocator for memory management)
   */
  rcl_allocator_t allocator;
};

#endif  // RCL__EVENT_IMPL_H_
