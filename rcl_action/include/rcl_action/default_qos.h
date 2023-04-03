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

#ifndef RCL_ACTION__DEFAULT_QOS_H_
#define RCL_ACTION__DEFAULT_QOS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rmw/types.h"

/**
 * @brief 默认的rcl_action_qos_profile_status配置
 *
 * 用于配置ROS2 Action通信的QoS（Quality of Service，服务质量）策略。
 * 这个结构体包含了一系列的QoS策略设置，用于控制Action通信的行为。
 */
static const rmw_qos_profile_t rcl_action_qos_profile_status_default = {
  RMW_QOS_POLICY_HISTORY_KEEP_LAST,     ///< 历史记录策略：保留最后一个消息
  1,                                    ///< 深度：保留的历史消息数量，这里设置为1
  RMW_QOS_POLICY_RELIABILITY_RELIABLE,  ///< 可靠性策略：可靠传输
  RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL,  ///< 持久性策略：本地瞬态数据
  RMW_QOS_DEADLINE_DEFAULT,                   ///< Deadline策略：默认值
  RMW_QOS_LIFESPAN_DEFAULT,                   ///< 生命周期策略：默认值
  RMW_QOS_POLICY_LIVELINESS_SYSTEM_DEFAULT,   ///< 活跃度策略：系统默认值
  RMW_QOS_LIVELINESS_LEASE_DURATION_DEFAULT,  ///< 租约时长策略：默认值
  false                                       ///< 不使用不属于RTPS标准的扩展字段
};

#ifdef __cplusplus
}
#endif

#endif  // RCL_ACTION__DEFAULT_QOS_H_
