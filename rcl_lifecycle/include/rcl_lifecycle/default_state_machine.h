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

#ifndef RCL_LIFECYCLE__DEFAULT_STATE_MACHINE_H_
#define RCL_LIFECYCLE__DEFAULT_STATE_MACHINE_H_

#include "rcl/macros.h"
#include "rcl/types.h"
#include "rcl_lifecycle/data_types.h"
#include "rcl_lifecycle/visibility_control.h"

#ifdef __cplusplus
extern "C" {
#endif

// 生命周期状态标签
RCL_LIFECYCLE_PUBLIC extern const char * rcl_lifecycle_configure_label;   ///< 配置状态标签
RCL_LIFECYCLE_PUBLIC extern const char * rcl_lifecycle_cleanup_label;     ///< 清理状态标签
RCL_LIFECYCLE_PUBLIC extern const char * rcl_lifecycle_activate_label;    ///< 激活状态标签
RCL_LIFECYCLE_PUBLIC extern const char * rcl_lifecycle_deactivate_label;  ///< 去激活状态标签
RCL_LIFECYCLE_PUBLIC extern const char * rcl_lifecycle_shutdown_label;    ///< 关闭状态标签

// 生命周期转换标签
RCL_LIFECYCLE_PUBLIC extern const char * rcl_lifecycle_transition_success_label;  ///< 转换成功标签
RCL_LIFECYCLE_PUBLIC extern const char * rcl_lifecycle_transition_failure_label;  ///< 转换失败标签
RCL_LIFECYCLE_PUBLIC extern const char * rcl_lifecycle_transition_error_label;  ///< 转换错误标签

/// 初始化默认状态机
/**
 * 此函数初始化一个默认状态机。它注册所有：主要状态、转换状态、转换和初始状态。主要状态是未配置的。
 *
 * 状态：unknown, unconfigured, inactive, active 和 finalized.
 * 转换状态：configuring, cleaningup, activating, deactivating, errorprocessing
 *          和 shuttingdown.
 * 转换：
 *    - 从 unconfigured 到 configuring
 *    - 从 unconfigured 到 shuttingdown
 *    - 从 configuring 到 inactive
 *    - 从 configuring 到 unconfigured
 *    - 从 configuring 到 errorprocessing
 *    - 从 inactive 到 activating
 *    - 从 inactive 到 cleaningup
 *    - 从 inactive 到 shuttingdown
 *    - 从 cleaningup 到 unconfigured
 *    - 从 cleaningup 到 inactive
 *    - 从 cleaningup 到 errorprocessing
 *    - 从 activating 到 active
 *    - 从 activating 到 inactive
 *    - 从 activating 到 errorprocessing
 *    - 从 active 到 deactivating
 *    - 从 active 到 shuttingdown
 *    - 从 deactivating 到 inactive
 *    - 从 deactivating 到 active
 *    - 从 deactivating 到 errorprocessing
 *    - 从 shutting down 到 finalized
 *    - 从 shutting down 到 finalized
 *    - 从 shutting down 到 errorprocessing
 *    - 从 errorprocessing 到 uncofigured
 *    - 从 errorprocessing 到 finalized
 *    - 从 errorprocessing 到 finalized
 *
 * <hr>
 * 属性          | 符合性
 * ------------- | -------------
 * 分配内存       | 是
 * 线程安全       | 否
 * 使用原子操作   | 否
 * 无锁           | 是
 *
 * \param[inout] state_machine 要初始化的结构体
 * \param[in] allocator 用于初始化状态机的有效分配器
 * \return `RCL_RET_OK` 如果状态机成功初始化，或者
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_LIFECYCLE_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_lifecycle_init_default_state_machine(
  rcl_lifecycle_state_machine_t * state_machine, const rcl_allocator_t * allocator);

#ifdef __cplusplus
}
#endif

#endif  // RCL_LIFECYCLE__DEFAULT_STATE_MACHINE_H_
