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

/** \mainpage rcl: 为其他ROS客户端库提供通用功能
 *
 * `rcl` 包含了按照ROS概念组织的函数和结构体（纯C）：
 *
 * - 节点
 *   - rcl/node.h
 * - 发布者
 *   - rcl/publisher.h
 * - 订阅者
 *   - rcl/subscription.h
 * - 服务客户端
 *   - rcl/client.h
 * - 服务服务器
 *   - rcl/service.h
 * - 定时器
 *   - rcl/timer.h
 *
 * 还有一些用于处理 "主题" 和 "服务" 的函数：
 *
 * - 验证主题或服务名称的函数（不一定是完全限定名）：
 *   - rcl_validate_topic_name()
 *   - rcl/validate_topic_name.h
 * - 将主题或服务名称扩展为完全限定名称的函数：
 *   - rcl_expand_topic_name()
 *   - rcl/expand_topic_name.h
 *
 * 它还具有一些等待和操作这些概念所必需的机制：
 *
 * - 初始化和关闭管理
 *   - rcl/init.h
 * - 等待集，用于等待消息/服务请求和响应/定时器准备就绪
 *   - rcl/wait.h
 * - 用于异步唤醒等待集的保护条件
 *   - rcl/guard_condition.h
 * - 用于内省和获取ROS图更改通知的函数
 *   - rcl/graph.h
 *
 * 此外，还有一些有用的抽象和实用程序：
 *
 * - 分配器概念，可用于控制 `rcl_*` 函数中的分配
 *   - rcl/allocator.h
 * - ROS时间概念以及对稳定和系统挂钟时间的访问
 *   - rcl/time.h
 * - 错误处理功能（C风格）
 *   - rcl/error_handling.h
 * - 宏
 *   - rcl/macros.h
 * - 返回代码类型
 *   - rcl/types.h
 * - 控制库上符号可见性的宏
 *   - rcl/visibility_control.h
 */

#ifndef RCL__RCL_H_
#define RCL__RCL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/init.h"
#include "rcl/node.h"
#include "rcl/publisher.h"
#include "rcl/subscription.h"
#include "rcl/wait.h"

#ifdef __cplusplus
}
#endif

#endif  // RCL__RCL_H_
