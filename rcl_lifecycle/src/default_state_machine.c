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

#include "rcl_lifecycle/default_state_machine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lifecycle_msgs/msg/state.h"
#include "lifecycle_msgs/msg/transition.h"
#include "rcl/error_handling.h"
#include "rcl/rcl.h"
#include "rcl_lifecycle/transition_map.h"
#include "rcutils/strdup.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 生命周期标签定义
 */
// 定义生命周期配置标签
const char* rcl_lifecycle_configure_label = "configure";
// 定义生命周期清理标签
const char* rcl_lifecycle_cleanup_label = "cleanup";
// 定义生命周期激活标签
const char* rcl_lifecycle_activate_label = "activate";
// 定义生命周期停用标签
const char* rcl_lifecycle_deactivate_label = "deactivate";
// 定义生命周期关闭标签
const char* rcl_lifecycle_shutdown_label = "shutdown";

// 定义生命周期过渡成功标签
const char* rcl_lifecycle_transition_success_label = "transition_success";
// 定义生命周期过渡失败标签
const char* rcl_lifecycle_transition_failure_label = "transition_failure";
// 定义生命周期过渡错误标签
const char* rcl_lifecycle_transition_error_label = "transition_error";

/**
 * @brief 为生命周期节点注册一组预定义的主要状态。
 *   - 初始化 unknown、 unconfigured、inactive、active 和 finalized 等主要状态对象
 *   - 调用`rcl_lifecycle_register_state()`将这些状态注册到`transition_map`中
 *   - 如果注册失败,返回错误
 *   通过注册这些基本状态,为后续注册转换提供基础。
 * @param[in] transition_map 转换映射，用于存储状态和转换信息
 * @param[in] allocator 分配器，用于分配内存
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t _register_primary_states(
    rcl_lifecycle_transition_map_t* transition_map,  //
    const rcutils_allocator_t* allocator) {
  rcl_ret_t ret = RCL_RET_ERROR;

  // 当注册状态时使用的默认值
  // 所有状态都没有附加转换
  // 在注册转换后填充每个状态的有效转换
  rcl_lifecycle_transition_t* valid_transitions = NULL;
  unsigned int valid_transition_size = 0;

  {  // 注册未知状态
    rcl_lifecycle_state_t rcl_state_unknown = {
        "unknown", lifecycle_msgs__msg__State__PRIMARY_STATE_UNKNOWN, valid_transitions,
        valid_transition_size};
    ret = rcl_lifecycle_register_state(transition_map, rcl_state_unknown, allocator);
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册未配置状态
    rcl_lifecycle_state_t rcl_state_unconfigured = {
        "unconfigured", lifecycle_msgs__msg__State__PRIMARY_STATE_UNCONFIGURED, valid_transitions,
        valid_transition_size};
    ret = rcl_lifecycle_register_state(transition_map, rcl_state_unconfigured, allocator);
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册非活动状态
    rcl_lifecycle_state_t rcl_state_inactive = {
        "inactive", lifecycle_msgs__msg__State__PRIMARY_STATE_INACTIVE, valid_transitions,
        valid_transition_size};
    ret = rcl_lifecycle_register_state(transition_map, rcl_state_inactive, allocator);
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册活动状态
    rcl_lifecycle_state_t rcl_state_active = {
        "active", lifecycle_msgs__msg__State__PRIMARY_STATE_ACTIVE, valid_transitions,
        valid_transition_size};
    ret = rcl_lifecycle_register_state(transition_map, rcl_state_active, allocator);
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册已完成状态
    rcl_lifecycle_state_t rcl_state_finalized = {
        "finalized", lifecycle_msgs__msg__State__PRIMARY_STATE_FINALIZED, valid_transitions,
        valid_transition_size};
    ret = rcl_lifecycle_register_state(transition_map, rcl_state_finalized, allocator);
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  return ret;
}

/**
 * @brief 注册生命周期转换状态
 *
 * @param[in] transition_map 生命周期转换映射表指针
 * @param[in] allocator 分配器指针
 * @return rcl_ret_t 返回注册结果，成功返回RCL_RET_OK，失败返回相应的错误码
 */
rcl_ret_t _register_transition_states(
    rcl_lifecycle_transition_map_t* transition_map, const rcutils_allocator_t* allocator) {
  // 初始化返回值为错误
  rcl_ret_t ret = RCL_RET_ERROR;

  // 当注册状态时使用的默认值
  // 所有状态都以没有附加转换的方式注册
  // 在注册转换后，每个状态的有效转换将被填充
  rcl_lifecycle_transition_t* valid_transitions = NULL;
  unsigned int valid_transition_size = 0;

  {  // 注册配置状态
    rcl_lifecycle_state_t rcl_state_configuring = {
        "configuring", lifecycle_msgs__msg__State__TRANSITION_STATE_CONFIGURING, valid_transitions,
        valid_transition_size};
    ret = rcl_lifecycle_register_state(transition_map, rcl_state_configuring, allocator);
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册清理状态
    rcl_lifecycle_state_t rcl_state_cleaningup = {
        "cleaningup", lifecycle_msgs__msg__State__TRANSITION_STATE_CLEANINGUP, valid_transitions,
        valid_transition_size};
    ret = rcl_lifecycle_register_state(transition_map, rcl_state_cleaningup, allocator);
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册关闭状态
    rcl_lifecycle_state_t rcl_state_shuttingdown = {
        "shuttingdown", lifecycle_msgs__msg__State__TRANSITION_STATE_SHUTTINGDOWN,
        valid_transitions, valid_transition_size};
    ret = rcl_lifecycle_register_state(transition_map, rcl_state_shuttingdown, allocator);
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册激活状态
    rcl_lifecycle_state_t rcl_state_activating = {
        "activating", lifecycle_msgs__msg__State__TRANSITION_STATE_ACTIVATING, valid_transitions,
        valid_transition_size};
    ret = rcl_lifecycle_register_state(transition_map, rcl_state_activating, allocator);
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册停用状态
    rcl_lifecycle_state_t rcl_state_deactivating = {
        "deactivating", lifecycle_msgs__msg__State__TRANSITION_STATE_DEACTIVATING,
        valid_transitions, valid_transition_size};
    ret = rcl_lifecycle_register_state(transition_map, rcl_state_deactivating, allocator);
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册错误处理状态
    rcl_lifecycle_state_t rcl_state_errorprocessing = {
        "errorprocessing", lifecycle_msgs__msg__State__TRANSITION_STATE_ERRORPROCESSING,
        valid_transitions, valid_transition_size};
    ret = rcl_lifecycle_register_state(transition_map, rcl_state_errorprocessing, allocator);
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  return ret;
}

/**
 * @brief 注册生命周期转换映射中的转换状态
 *
 * @param[in] transition_map 生命周期转换映射指针
 * @param[in] allocator 分配器指针
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t _register_transitions(
    rcl_lifecycle_transition_map_t* transition_map, const rcutils_allocator_t* allocator) {
  // 初始化返回值为错误
  rcl_ret_t ret = RCL_RET_ERROR;

  // 从转换映射中获取主要状态和转换状态
  // 注意：这些状态指向映射内部的状态
  // 在此之前创建的状态（参见 _register_primary_states）已复制到映射中
  // 因此，必须再次从映射中检索它们以避免使指针无效。

  // 获取未配置状态
  rcl_lifecycle_state_t* unconfigured_state = rcl_lifecycle_get_state(
      transition_map, lifecycle_msgs__msg__State__PRIMARY_STATE_UNCONFIGURED);
  // 获取非活动状态
  rcl_lifecycle_state_t* inactive_state =
      rcl_lifecycle_get_state(transition_map, lifecycle_msgs__msg__State__PRIMARY_STATE_INACTIVE);
  // 获取活动状态
  rcl_lifecycle_state_t* active_state =
      rcl_lifecycle_get_state(transition_map, lifecycle_msgs__msg__State__PRIMARY_STATE_ACTIVE);
  // 获取最终状态
  rcl_lifecycle_state_t* finalized_state =
      rcl_lifecycle_get_state(transition_map, lifecycle_msgs__msg__State__PRIMARY_STATE_FINALIZED);

  // 获取配置中状态
  rcl_lifecycle_state_t* configuring_state = rcl_lifecycle_get_state(
      transition_map, lifecycle_msgs__msg__State__TRANSITION_STATE_CONFIGURING);
  // 获取激活中状态
  rcl_lifecycle_state_t* activating_state = rcl_lifecycle_get_state(
      transition_map, lifecycle_msgs__msg__State__TRANSITION_STATE_ACTIVATING);
  // 获取停用中状态
  rcl_lifecycle_state_t* deactivating_state = rcl_lifecycle_get_state(
      transition_map, lifecycle_msgs__msg__State__TRANSITION_STATE_DEACTIVATING);
  // 获取清理中状态
  rcl_lifecycle_state_t* cleaningup_state = rcl_lifecycle_get_state(
      transition_map, lifecycle_msgs__msg__State__TRANSITION_STATE_CLEANINGUP);
  // 获取关闭中状态
  rcl_lifecycle_state_t* shuttingdown_state = rcl_lifecycle_get_state(
      transition_map, lifecycle_msgs__msg__State__TRANSITION_STATE_SHUTTINGDOWN);
  // 获取错误处理状态
  rcl_lifecycle_state_t* errorprocessing_state = rcl_lifecycle_get_state(
      transition_map, lifecycle_msgs__msg__State__TRANSITION_STATE_ERRORPROCESSING);

  {  // 注册从未配置状态到配置状态的转换
    // 定义一个rcl_lifecycle_transition_t类型的变量，用于表示从未配置状态到配置状态的转换
    rcl_lifecycle_transition_t rcl_transition_configure = {
        rcl_lifecycle_configure_label, lifecycle_msgs__msg__Transition__TRANSITION_CONFIGURE,
        unconfigured_state, configuring_state};
    // 将定义的转换注册到transition_map中
    ret = rcl_lifecycle_register_transition(transition_map, rcl_transition_configure, allocator);
    // 如果注册失败，返回错误码
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从配置状态到非活动状态的转换
    // 定义一个rcl_lifecycle_transition_t类型的变量，用于表示从配置状态到非活动状态的转换
    rcl_lifecycle_transition_t rcl_transition_on_configure_success = {
        rcl_lifecycle_transition_success_label,
        lifecycle_msgs__msg__Transition__TRANSITION_ON_CONFIGURE_SUCCESS, configuring_state,
        inactive_state};
    // 将定义的转换注册到transition_map中
    ret = rcl_lifecycle_register_transition(
        transition_map, rcl_transition_on_configure_success, allocator);
    // 如果注册失败，返回错误码
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从配置状态到未配置状态的转换
    // 定义一个rcl_lifecycle_transition_t类型的变量，用于表示从配置状态到未配置状态的转换
    rcl_lifecycle_transition_t rcl_transition_on_configure_failure = {
        rcl_lifecycle_transition_failure_label,
        lifecycle_msgs__msg__Transition__TRANSITION_ON_CONFIGURE_FAILURE, configuring_state,
        unconfigured_state};
    // 将定义的转换注册到transition_map中
    ret = rcl_lifecycle_register_transition(
        transition_map, rcl_transition_on_configure_failure, allocator);
    // 如果注册失败，返回错误码
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从配置状态到错误处理状态的转换
    // 定义一个rcl_lifecycle_transition_t类型的变量，用于表示从配置状态到错误处理状态的转换
    rcl_lifecycle_transition_t rcl_transition_on_configure_error = {
        rcl_lifecycle_transition_error_label,
        lifecycle_msgs__msg__Transition__TRANSITION_ON_CONFIGURE_ERROR, configuring_state,
        errorprocessing_state};
    // 将定义的转换注册到transition_map中
    ret = rcl_lifecycle_register_transition(
        transition_map, rcl_transition_on_configure_error, allocator);
    // 如果注册失败，返回错误码
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从非活动状态到清理状态的转换
    // 定义一个rcl_lifecycle_transition_t类型的变量，用于表示从非活动状态到清理状态的转换
    rcl_lifecycle_transition_t rcl_transition_cleanup = {
        rcl_lifecycle_cleanup_label, lifecycle_msgs__msg__Transition__TRANSITION_CLEANUP,
        inactive_state, cleaningup_state};
    // 将定义的转换注册到transition_map中
    ret = rcl_lifecycle_register_transition(transition_map, rcl_transition_cleanup, allocator);
    // 如果注册失败，返回错误码
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从清理状态到未配置状态的转换
    // 定义一个rcl_lifecycle_transition_t类型的变量，用于表示从清理状态到未配置状态的转换
    rcl_lifecycle_transition_t rcl_transition_on_cleanup_success = {
        rcl_lifecycle_transition_success_label,
        lifecycle_msgs__msg__Transition__TRANSITION_ON_CLEANUP_SUCCESS, cleaningup_state,
        unconfigured_state};
    // 将定义的转换注册到transition_map中
    ret = rcl_lifecycle_register_transition(
        transition_map, rcl_transition_on_cleanup_success, allocator);
    // 如果注册失败，返回错误码
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从清理状态到非活动状态的转换
    // 定义一个rcl_lifecycle_transition_t类型的变量，用于表示从清理状态到非活动状态的转换
    rcl_lifecycle_transition_t rcl_transition_on_cleanup_failure = {
        rcl_lifecycle_transition_failure_label,
        lifecycle_msgs__msg__Transition__TRANSITION_ON_CLEANUP_FAILURE, cleaningup_state,
        inactive_state};
    // 将定义的转换注册到transition_map中
    ret = rcl_lifecycle_register_transition(
        transition_map, rcl_transition_on_cleanup_failure, allocator);
    // 如果注册失败，返回错误码
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从cleaniningup状态到errorprocessing状态的转换
    // 定义一个rcl_lifecycle_transition_t类型的结构体变量，用于存储转换信息
    rcl_lifecycle_transition_t rcl_transition_on_cleanup_error = {
        rcl_lifecycle_transition_error_label,                          // 转换标签
        lifecycle_msgs__msg__Transition__TRANSITION_ON_CLEANUP_ERROR,  // 转换ID
        cleaningup_state,                                              // 起始状态
        errorprocessing_state                                          // 目标状态
    };
    // 将定义的转换注册到transition_map中
    ret = rcl_lifecycle_register_transition(
        transition_map, rcl_transition_on_cleanup_error, allocator);
    // 判断注册是否成功
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从inactive状态到activating状态的转换
    // 定义一个rcl_lifecycle_transition_t类型的结构体变量，用于存储转换信息
    rcl_lifecycle_transition_t rcl_transition_activate = {
        rcl_lifecycle_activate_label,                          // 转换标签
        lifecycle_msgs__msg__Transition__TRANSITION_ACTIVATE,  // 转换ID
        inactive_state,                                        // 起始状态
        activating_state                                       // 目标状态
    };
    // 将定义的转换注册到transition_map中
    ret = rcl_lifecycle_register_transition(transition_map, rcl_transition_activate, allocator);
    // 判断注册是否成功
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从activating状态到active状态的转换
    // 定义一个rcl_lifecycle_transition_t类型的结构体变量，用于存储转换信息
    rcl_lifecycle_transition_t rcl_transition_on_activate_success = {
        rcl_lifecycle_transition_success_label,                           // 转换标签
        lifecycle_msgs__msg__Transition__TRANSITION_ON_ACTIVATE_SUCCESS,  // 转换ID
        activating_state,                                                 // 起始状态
        active_state                                                      // 目标状态
    };
    // 将定义的转换注册到transition_map中
    ret = rcl_lifecycle_register_transition(
        transition_map, rcl_transition_on_activate_success, allocator);
    // 判断注册是否成功
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从activating状态到inactive状态的转换
    // 定义一个rcl_lifecycle_transition_t类型的结构体变量，用于存储转换信息
    rcl_lifecycle_transition_t rcl_transition_on_activate_failure = {
        rcl_lifecycle_transition_failure_label,                           // 转换标签
        lifecycle_msgs__msg__Transition__TRANSITION_ON_ACTIVATE_FAILURE,  // 转换ID
        activating_state,                                                 // 起始状态
        inactive_state                                                    // 目标状态
    };
    // 将定义的转换注册到transition_map中
    ret = rcl_lifecycle_register_transition(
        transition_map, rcl_transition_on_activate_failure, allocator);
    // 判断注册是否成功
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从activating状态到errorprocessing状态的转换
    // 定义一个rcl_lifecycle_transition_t类型的结构体变量，用于存储转换信息
    rcl_lifecycle_transition_t rcl_transition_on_activate_error = {
        rcl_lifecycle_transition_error_label,                           // 转换标签
        lifecycle_msgs__msg__Transition__TRANSITION_ON_ACTIVATE_ERROR,  // 转换ID
        activating_state,                                               // 起始状态
        errorprocessing_state                                           // 目标状态
    };
    // 将定义的转换注册到transition_map中
    ret = rcl_lifecycle_register_transition(
        transition_map, rcl_transition_on_activate_error, allocator);
    // 判断注册是否成功
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从 active 状态到 deactivating 状态的转换
    // 定义 rcl_lifecycle_transition_t 结构体变量 rcl_transition_deactivate
    rcl_lifecycle_transition_t rcl_transition_deactivate = {
        rcl_lifecycle_deactivate_label, lifecycle_msgs__msg__Transition__TRANSITION_DEACTIVATE,
        active_state, deactivating_state};
    // 使用 rcl_lifecycle_register_transition 函数注册状态转换
    ret = rcl_lifecycle_register_transition(transition_map, rcl_transition_deactivate, allocator);
    // 如果注册失败，返回错误代码
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从 deactivating 状态到 inactive 状态的转换
    // 定义 rcl_lifecycle_transition_t 结构体变量 rcl_transition_on_deactivate_success
    rcl_lifecycle_transition_t rcl_transition_on_deactivate_success = {
        rcl_lifecycle_transition_success_label,
        lifecycle_msgs__msg__Transition__TRANSITION_ON_DEACTIVATE_SUCCESS, deactivating_state,
        inactive_state};
    // 使用 rcl_lifecycle_register_transition 函数注册状态转换
    ret = rcl_lifecycle_register_transition(
        transition_map, rcl_transition_on_deactivate_success, allocator);
    // 如果注册失败，返回错误代码
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从 deactivating 状态到 active 状态的转换
    // 定义 rcl_lifecycle_transition_t 结构体变量 rcl_transition_on_deactivate_failure
    rcl_lifecycle_transition_t rcl_transition_on_deactivate_failure = {
        rcl_lifecycle_transition_failure_label,
        lifecycle_msgs__msg__Transition__TRANSITION_ON_DEACTIVATE_FAILURE, deactivating_state,
        active_state};
    // 使用 rcl_lifecycle_register_transition 函数注册状态转换
    ret = rcl_lifecycle_register_transition(
        transition_map, rcl_transition_on_deactivate_failure, allocator);
    // 如果注册失败，返回错误代码
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从 deactivating 状态到 errorprocessing 状态的转换
    // 定义 rcl_lifecycle_transition_t 结构体变量 rcl_transition_on_deactivate_error
    rcl_lifecycle_transition_t rcl_transition_on_deactivate_error = {
        rcl_lifecycle_transition_error_label,
        lifecycle_msgs__msg__Transition__TRANSITION_ON_DEACTIVATE_ERROR, deactivating_state,
        errorprocessing_state};
    // 使用 rcl_lifecycle_register_transition 函数注册状态转换
    ret = rcl_lifecycle_register_transition(
        transition_map, rcl_transition_on_deactivate_error, allocator);
    // 如果注册失败，返回错误代码
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从 unconfigured 状态到 shuttingdown 状态的转换
    // 定义 rcl_lifecycle_transition_t 结构体变量 rcl_transition_unconfigured_shutdown
    rcl_lifecycle_transition_t rcl_transition_unconfigured_shutdown = {
        rcl_lifecycle_shutdown_label,
        lifecycle_msgs__msg__Transition__TRANSITION_UNCONFIGURED_SHUTDOWN, unconfigured_state,
        shuttingdown_state};
    // 使用 rcl_lifecycle_register_transition 函数注册状态转换
    ret = rcl_lifecycle_register_transition(
        transition_map, rcl_transition_unconfigured_shutdown, allocator);
    // 如果注册失败，返回错误代码
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从非活动状态到关闭状态的转换
    // 定义从非活动状态到关闭状态的转换结构体
    rcl_lifecycle_transition_t rcl_transition_inactive_shutdown = {
        rcl_lifecycle_shutdown_label, lifecycle_msgs__msg__Transition__TRANSITION_INACTIVE_SHUTDOWN,
        inactive_state, shuttingdown_state};
    // 将转换注册到转换映射中
    ret = rcl_lifecycle_register_transition(
        transition_map, rcl_transition_inactive_shutdown, allocator);
    // 检查注册结果
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从活动状态到关闭状态的转换
    // 定义从活动状态到关闭状态的转换结构体
    rcl_lifecycle_transition_t rcl_transition_active_shutdown = {
        rcl_lifecycle_shutdown_label, lifecycle_msgs__msg__Transition__TRANSITION_ACTIVE_SHUTDOWN,
        active_state, shuttingdown_state};
    // 将转换注册到转换映射中
    ret = rcl_lifecycle_register_transition(
        transition_map, rcl_transition_active_shutdown, allocator);
    // 检查注册结果
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从关闭状态到最终状态的成功转换
    // 定义从关闭状态到最终状态的成功转换结构体
    rcl_lifecycle_transition_t rcl_transition_on_shutdown_success = {
        rcl_lifecycle_transition_success_label,
        lifecycle_msgs__msg__Transition__TRANSITION_ON_SHUTDOWN_SUCCESS, shuttingdown_state,
        finalized_state};
    // 将转换注册到转换映射中
    ret = rcl_lifecycle_register_transition(
        transition_map, rcl_transition_on_shutdown_success, allocator);
    // 检查注册结果
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从关闭状态到最终状态的失败转换
    // 定义从关闭状态到最终状态的失败转换结构体
    rcl_lifecycle_transition_t rcl_transition_on_shutdown_failure = {
        rcl_lifecycle_transition_failure_label,
        lifecycle_msgs__msg__Transition__TRANSITION_ON_SHUTDOWN_FAILURE, shuttingdown_state,
        finalized_state};
    // 将转换注册到转换映射中
    ret = rcl_lifecycle_register_transition(
        transition_map, rcl_transition_on_shutdown_failure, allocator);
    // 检查注册结果
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从关闭状态到错误处理状态的转换
    // 定义从关闭状态到错误处理状态的转换结构体
    rcl_lifecycle_transition_t rcl_transition_on_shutdown_error = {
        rcl_lifecycle_transition_error_label,
        lifecycle_msgs__msg__Transition__TRANSITION_ON_SHUTDOWN_ERROR, shuttingdown_state,
        errorprocessing_state};
    // 将转换注册到转换映射中
    ret = rcl_lifecycle_register_transition(
        transition_map, rcl_transition_on_shutdown_error, allocator);
    // 检查注册结果
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从 errorprocessing 状态到 unconfigured 状态的转换
    // 定义 rcl_transition_on_error_success 结构体变量
    rcl_lifecycle_transition_t rcl_transition_on_error_success = {
        rcl_lifecycle_transition_success_label,                        // 转换成功标签
        lifecycle_msgs__msg__Transition__TRANSITION_ON_ERROR_SUCCESS,  // 转换类型
        errorprocessing_state,  // 当前状态：errorprocessing
        unconfigured_state      // 目标状态：unconfigured
    };
    // 注册转换
    ret = rcl_lifecycle_register_transition(
        transition_map, rcl_transition_on_error_success, allocator);
    // 检查注册结果
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从 errorprocessing 状态到 finalized 状态的转换（失败情况）
    // 定义 rcl_transition_on_error_failure 结构体变量
    rcl_lifecycle_transition_t rcl_transition_on_error_failure = {
        rcl_lifecycle_transition_failure_label,                        // 转换失败标签
        lifecycle_msgs__msg__Transition__TRANSITION_ON_ERROR_FAILURE,  // 转换类型
        errorprocessing_state,  // 当前状态：errorprocessing
        finalized_state         // 目标状态：finalized
    };
    // 注册转换
    ret = rcl_lifecycle_register_transition(
        transition_map, rcl_transition_on_error_failure, allocator);
    // 检查注册结果
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  {  // 注册从 errorprocessing 状态到 finalized 状态的转换（错误情况）
    // 定义 rcl_transition_on_error_error 结构体变量
    rcl_lifecycle_transition_t rcl_transition_on_error_error = {
        rcl_lifecycle_transition_error_label,                        // 转换错误标签
        lifecycle_msgs__msg__Transition__TRANSITION_ON_ERROR_ERROR,  // 转换类型
        errorprocessing_state,  // 当前状态：errorprocessing
        finalized_state         // 目标状态：finalized
    };
    // 注册转换
    ret =
        rcl_lifecycle_register_transition(transition_map, rcl_transition_on_error_error, allocator);
    // 检查注册结果
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  return ret;
}

/**
 * @brief 初始化默认生命周期状态机
 *
 * @param[out] state_machine 生命周期状态机指针
 * @param[in] allocator 内存分配器指针
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_lifecycle_init_default_state_machine(
    rcl_lifecycle_state_machine_t* state_machine, const rcutils_allocator_t* allocator) {
  rcl_ret_t fcn_ret = RCL_RET_ERROR;
  // 用于在 fail: 块中连接错误消息。
  // 导致跳转到 fail: 的原因或错误
  char* fail_error_message = NULL;
  // 在 fail: 中发生的错误
  char* fini_error_message = NULL;
  rcl_allocator_t default_allocator;

  // ***************************
  // 注册所有主要状态
  // ***************************
  fcn_ret = _register_primary_states(&state_machine->transition_map, allocator);
  if (fcn_ret != RCL_RET_OK) {
    goto fail;
  }

  // ******************************
  // 注册所有过渡状态
  // ******************************
  fcn_ret = _register_transition_states(&state_machine->transition_map, allocator);
  if (fcn_ret != RCL_RET_OK) {
    goto fail;
  }

  // ************************
  // 注册所有过渡
  // ************************
  fcn_ret = _register_transitions(&state_machine->transition_map, allocator);
  if (fcn_ret != RCL_RET_OK) {
    goto fail;
  }

  // *************************************
  // 将初始状态设置为未配置
  // *************************************
  state_machine->current_state = rcl_lifecycle_get_state(
      &state_machine->transition_map, lifecycle_msgs__msg__State__PRIMARY_STATE_UNCONFIGURED);

  return fcn_ret;

fail:
  // 如果 rcl_lifecycle_transition_map_fini() 失败，它将在此处覆盖错误字符串。
  // 如果发生这种情况，请连接错误字符串
  default_allocator = rcl_get_default_allocator();

  // 如果有错误设置
  if (rcl_error_is_set()) {
    // 复制错误信息字符串
    fail_error_message = rcutils_strdup(rcl_get_error_string().str, default_allocator);
    // 重置错误
    rcl_reset_error();
  }

  // 如果释放转换映射失败
  if (rcl_lifecycle_transition_map_fini(&state_machine->transition_map, allocator) != RCL_RET_OK) {
    // 如果有错误设置
    if (rcl_error_is_set()) {
      // 复制错误信息字符串
      fini_error_message = rcutils_strdup(rcl_get_error_string().str, default_allocator);
      // 重置错误
      rcl_reset_error();
    }
    // 设置错误消息，包括原始错误和在释放过程中遇到的错误
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Freeing transition map failed while handling a previous error. Leaking memory!"
        "\nOriginal error:\n\t%s\nError encountered in "
        "rcl_lifecycle_transition_map_fini():\n\t%s\n",
        fail_error_message != NULL ? fail_error_message
                                   : "Failed to duplicate error while init state machine !",
        fini_error_message != NULL ? fini_error_message
                                   : "Failed to duplicate error while fini transition map !");
  }

  // 如果没有设置错误
  if (!rcl_error_is_set()) {
    // 设置错误消息
    RCL_SET_ERROR_MSG(
        (fail_error_message != NULL)
            ? fail_error_message
            : "Unspecified error in rcl_lifecycle_init_default_state_machine() !");
  }

  // 如果有初始化失败的错误信息
  if (fail_error_message != NULL) {
    // 释放错误信息内存
    default_allocator.deallocate(fail_error_message, default_allocator.state);
  }
  // 如果有释放资源失败的错误信息
  if (fini_error_message != NULL) {
    // 释放错误信息内存
    default_allocator.deallocate(fini_error_message, default_allocator.state);
  }

  return RCL_RET_ERROR;
}

#ifdef __cplusplus
}
#endif  // extern "C"
