// Copyright 2014 Open Source Robotics Foundation, Inc.
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

/// @file

#ifndef RCL__TYPES_H_
#define RCL__TYPES_H_

#include <rmw/types.h>

/// rcl_ret_t 类型，用于保存 rcl 返回代码。
typedef rmw_ret_t rcl_ret_t;

/// 成功返回代码。
#define RCL_RET_OK RMW_RET_OK
/// 未指定错误返回代码。
#define RCL_RET_ERROR RMW_RET_ERROR
/// 超时发生返回代码。
#define RCL_RET_TIMEOUT RMW_RET_TIMEOUT
/// 内存分配失败返回代码。
#define RCL_RET_BAD_ALLOC RMW_RET_BAD_ALLOC
/// 无效参数返回代码。
#define RCL_RET_INVALID_ARGUMENT RMW_RET_INVALID_ARGUMENT
/// 不支持的返回代码。
#define RCL_RET_UNSUPPORTED RMW_RET_UNSUPPORTED

// rcl 特定的返回代码从 100 开始
/// rcl_init() 已调用返回代码。
#define RCL_RET_ALREADY_INIT 100
/// rcl_init() 尚未调用返回代码。
#define RCL_RET_NOT_INIT 101
/// 不匹配的 rmw 标识符返回代码。
#define RCL_RET_MISMATCHED_RMW_ID 102
/// 主题名称未通过验证。
#define RCL_RET_TOPIC_NAME_INVALID 103
/// 服务名称（与主题名称相同）未通过验证。
#define RCL_RET_SERVICE_NAME_INVALID 104
/// 主题名称替换未知。
#define RCL_RET_UNKNOWN_SUBSTITUTION 105
/// rcl_shutdown() 已调用返回代码。
#define RCL_RET_ALREADY_SHUTDOWN 106

// rcl 节点特定的返回代码在 2XX
/// 给定的 rcl_node_t 无效返回代码。
#define RCL_RET_NODE_INVALID 200
/// 无效节点名称返回代码。
#define RCL_RET_NODE_INVALID_NAME 201
/// 无效节点命名空间返回代码。
#define RCL_RET_NODE_INVALID_NAMESPACE 202
/// 未找到节点名称
#define RCL_RET_NODE_NAME_NON_EXISTENT 203

// rcl 发布者特定的返回代码在 3XX
/// 给定的 rcl_publisher_t 无效返回代码。
#define RCL_RET_PUBLISHER_INVALID 300

// rcl 订阅特定的返回代码在 4XX
/// 给定的 rcl_subscription_t 无效返回代码。
#define RCL_RET_SUBSCRIPTION_INVALID 400
/// 从订阅中获取消息失败返回代码。
#define RCL_RET_SUBSCRIPTION_TAKE_FAILED 401

// rcl 服务客户端特定的返回代码在 5XX
/// 给定的 rcl_client_t 无效返回代码。
#define RCL_RET_CLIENT_INVALID 500
/// 从客户端获取响应失败返回代码。
#define RCL_RET_CLIENT_TAKE_FAILED 501

// rcl 服务服务器特定的返回代码在 6XX
/// 给定的 rcl_service_t 无效返回代码。
#define RCL_RET_SERVICE_INVALID 600
/// 从服务中获取请求失败返回代码。
#define RCL_RET_SERVICE_TAKE_FAILED 601

// rcl 守护条件特定的返回代码在 7XX

// rcl 计时器特定的返回代码在 8XX
/// 给定的 rcl_timer_t 无效返回代码。
#define RCL_RET_TIMER_INVALID 800
/// 给定的计时器已取消返回代码。
#define RCL_RET_TIMER_CANCELED 801

// rcl 等待和等待集特定的返回代码在 9XX
/// 给定的 rcl_wait_set_t 无效返回代码。
#define RCL_RET_WAIT_SET_INVALID 900
/// 给定的 rcl_wait_set_t 为空返回代码。
#define RCL_RET_WAIT_SET_EMPTY 901
/// 给定的 rcl_wait_set_t 已满返回代码。
#define RCL_RET_WAIT_SET_FULL 902

// rcl 参数解析特定的返回代码在 1XXX
/// 参数不是有效的重映射规则
#define RCL_RET_INVALID_REMAP_RULE 1001
/// 预期一种类型的词素，但得到另一种
#define RCL_RET_WRONG_LEXEME 1002
/// 在解析过程中找到无效的 ros 参数
#define RCL_RET_INVALID_ROS_ARGS 1003
/// 参数不是有效的参数规则
#define RCL_RET_INVALID_PARAM_RULE 1010
/// 参数不是有效的日志级别规则
#define RCL_RET_INVALID_LOG_LEVEL_RULE 1020

// rcl 事件特定的返回代码在 20XX
/// 给定的 rcl_event_t 无效返回代码。
#define RCL_RET_EVENT_INVALID 2000
/// 从事件句柄获取事件失败
#define RCL_RET_EVENT_TAKE_FAILED 2001

/// rcl_lifecycle 状态注册返回代码在 30XX
/// rcl_lifecycle 状态已注册
#define RCL_RET_LIFECYCLE_STATE_REGISTERED 3000
/// rcl_lifecycle 状态未注册
#define RCL_RET_LIFECYCLE_STATE_NOT_REGISTERED 3001

/// rmw_serialized_message_t 的 typedef;
typedef rmw_serialized_message_t rcl_serialized_message_t;

#endif  // RCL__TYPES_H_
