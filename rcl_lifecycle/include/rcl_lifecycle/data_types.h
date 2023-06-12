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

#ifndef RCL_LIFECYCLE__DATA_TYPES_H_
#define RCL_LIFECYCLE__DATA_TYPES_H_

#include "lifecycle_msgs/msg/transition_event.h"
#include "rcl/rcl.h"
#include "rcl_lifecycle/visibility_control.h"

#ifdef __cplusplus
extern "C" {
#endif

// 定义一个生命周期转换结构体类型别名
typedef struct rcl_lifecycle_transition_s rcl_lifecycle_transition_t;

/// 生命周期状态机的状态结构体定义
typedef struct rcl_lifecycle_state_s {
  /// 状态名称字符串：Unconfigured, Inactive, Active 或 Finalized
  const char* label;
  /// 状态标识符
  uint8_t id;
  /// 指向有效转换结构体的指针
  rcl_lifecycle_transition_t* valid_transitions;
  /// 有效转换的数量
  unsigned int valid_transition_size;
} rcl_lifecycle_state_t;

/// 生命周期状态机的转换结构体定义
typedef struct rcl_lifecycle_transition_s {
  /// 转换名称字符串：configuring, cleaningup, activating, deactivating,
  /// errorprocessing 或 shuttingdown.
  const char* label;
  /// 转换标识符
  unsigned int id;
  /// 转换初始化的值
  rcl_lifecycle_state_t* start;
  /// 转换的目标
  rcl_lifecycle_state_t* goal;
} rcl_lifecycle_transition_t;

/// 包含状态和转换的转换映射结构体定义
typedef struct rcl_lifecycle_transition_map_s {
  /// 用于生成转换映射的状态
  rcl_lifecycle_state_t* states;
  /// 状态数量
  unsigned int states_size;
  /// 用于生成转换映射的转换
  rcl_lifecycle_transition_t* transitions;
  /// 转换数量
  unsigned int transitions_size;
} rcl_lifecycle_transition_map_t;

// ========= ========= ========= //

/// 包含与ROS世界通信接口的结构体定义
typedef struct rcl_lifecycle_com_interface_s {
  /// 用于创建发布者和服务的节点句柄
  rcl_node_t* node_handle;
  /// 用于发布转换事件的事件
  rcl_publisher_t pub_transition_event;
  /// 允许触发状态更改的服务
  rcl_service_t srv_change_state;
  /// 允许获取当前状态的服务
  rcl_service_t srv_get_state;
  /// 允许获取可用状态的服务
  rcl_service_t srv_get_available_states;
  /// 允许获取可用转换的服务
  rcl_service_t srv_get_available_transitions;
  /// 允许从图中获取转换的服务
  rcl_service_t srv_get_transition_graph;
  /// 缓存的转换事件消息
  lifecycle_msgs__msg__TransitionEvent msg;
} rcl_lifecycle_com_interface_t;

/// 包含用于配置rcl_lifecycle_state_machine_t实例的各种选项的结构体定义
typedef struct rcl_lifecycle_state_machine_options_s {
  /// 标志，指示状态机是否使用默认状态进行初始化
  bool initialize_default_states;
  /// 标志，指示是否使用com接口
  bool enable_com_interface;
  /// 用于分配状态和转换的分配器
  rcl_allocator_t allocator;
} rcl_lifecycle_state_machine_options_t;

/// 包含状态机数据的结构体定义
typedef struct rcl_lifecycle_state_machine_s {
  /// 状态机的当前状态
  const rcl_lifecycle_state_t* current_state;
  /// 注册状态和转换的映射/关联数组
  rcl_lifecycle_transition_map_t transition_map;
  /// 通向ROS世界的通信接口
  rcl_lifecycle_com_interface_t com_interface;
  /// 初始化状态机时使用的选项结构体
  rcl_lifecycle_state_machine_options_t options;
} rcl_lifecycle_state_machine_t;

#ifdef __cplusplus
}
#endif

#endif  // RCL_LIFECYCLE__DATA_TYPES_H_
