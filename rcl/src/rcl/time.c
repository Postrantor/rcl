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

#include "rcl/time.h"

#include <stdbool.h>
#include <stdlib.h>

#include "./common.h"
#include "rcl/allocator.h"
#include "rcl/error_handling.h"
#include "rcutils/macros.h"
#include "rcutils/stdatomic_helper.h"
#include "rcutils/time.h"

/**
 * @file
 * @brief RCL_ROS_TIME 实现的内部存储结构体
 */
typedef struct rcl_ros_clock_storage_s {
  atomic_uint_least64_t current_time;  ///< 当前时间，使用原子操作进行更新
  bool active;                         ///< 指示时钟是否处于活动状态
} rcl_ros_clock_storage_t;

/**
 * @brief 获取稳定时间的实现函数
 *
 * @param[in] data 未使用的参数
 * @param[out] current_time 存储当前稳定时间的指针
 * @return 返回 rcl_ret_t 类型的结果
 */
static rcl_ret_t rcl_get_steady_time(void *data, rcl_time_point_value_t *current_time) {
  (void)data;  // unused
  return rcutils_steady_time_now(current_time);
}

/**
 * @brief 获取系统时间的实现函数
 *
 * @param[in] data 未使用的参数
 * @param[out] current_time 存储当前系统时间的指针
 * @return 返回 rcl_ret_t 类型的结果
 */
static rcl_ret_t rcl_get_system_time(void *data, rcl_time_point_value_t *current_time) {
  (void)data;  // unused
  return rcutils_system_time_now(current_time);
}

/**
 * @brief 初始化通用时钟的内部方法，假设时钟是有效的
 *
 * @param[out] clock 要初始化的时钟指针
 * @param[in] allocator 分配器指针
 */
static void rcl_init_generic_clock(rcl_clock_t *clock, rcl_allocator_t *allocator) {
  clock->type = RCL_CLOCK_UNINITIALIZED;  ///< 设置时钟类型为未初始化
  clock->jump_callbacks = NULL;           ///< 初始化跳转回调为空
  clock->num_jump_callbacks = 0u;         ///< 初始化跳转回调数量为 0
  clock->get_now = NULL;                  ///< 初始化获取当前时间的函数指针为空
  clock->data = NULL;                     ///< 初始化数据指针为空
  clock->allocator = *allocator;          ///< 设置分配器
}

/**
 * @brief 获取当前的 ROS 时间
 *
 * 这个函数仅在实现中使用
 *
 * @param[in] data 指向 rcl_ros_clock_storage_t 结构体的指针
 * @param[out] current_time 存储当前时间的指针
 * @return 返回 rcl_ret_t 类型的状态，成功返回 RCL_RET_OK
 */
static rcl_ret_t rcl_get_ros_time(void *data, rcl_time_point_value_t *current_time) {
  // 将 void 类型的 data 转换为 rcl_ros_clock_storage_t 类型的指针
  rcl_ros_clock_storage_t *t = (rcl_ros_clock_storage_t *)data;

  // 如果时钟未激活，则获取系统时间
  if (!t->active) {
    return rcl_get_system_time(data, current_time);
  }

  // 从原子变量中加载当前时间
  *current_time = rcutils_atomic_load_uint64_t(&(t->current_time));

  // 返回成功状态
  return RCL_RET_OK;
}

/**
 * @brief 判断时钟是否已启动
 *
 * @param[in] clock 指向 rcl_clock_t 结构体的指针
 * @return 返回布尔值，如果时钟已启动则返回 true，否则返回 false
 */
bool rcl_clock_time_started(rcl_clock_t *clock) {
  // 定义一个用于查询当前时间的变量
  rcl_time_point_value_t query_now;

  // 获取当前时间，如果成功则判断时间是否大于 0
  if (rcl_clock_get_now(clock, &query_now) == RCL_RET_OK) {
    return query_now > 0;
  }

  // 获取当前时间失败，返回 false
  return false;
}

/**
 * @brief 判断时钟是否有效
 *
 * @param[in] clock 指向 rcl_clock_t 结构体的指针
 * @return 返回布尔值，如果时钟有效则返回 true，否则返回 false
 */
bool rcl_clock_valid(rcl_clock_t *clock) {
  // 如果 clock 为 NULL 或时钟类型未初始化或 get_now 函数指针为空，则返回 false
  if (clock == NULL || clock->type == RCL_CLOCK_UNINITIALIZED || clock->get_now == NULL) {
    return false;
  }

  // 时钟有效，返回 true
  return true;
}

/**
 * @brief 初始化指定类型的时钟
 *
 * @param[in] clock_type 时钟类型，可以是 RCL_CLOCK_UNINITIALIZED、RCL_ROS_TIME、RCL_SYSTEM_TIME 或
 * RCL_STEADY_TIME
 * @param[out] clock 指向 rcl_clock_t 结构体的指针，用于存储初始化后的时钟信息
 * @param[in] allocator 分配器，用于分配内存
 * @return 返回 rcl_ret_t 类型的结果，成功返回 RCL_RET_OK，失败返回相应的错误码
 */
rcl_ret_t rcl_clock_init(
    rcl_clock_type_t clock_type, rcl_clock_t *clock, rcl_allocator_t *allocator) {
  // 检查分配器是否有效，无效则返回 RCL_RET_INVALID_ARGUMENT 错误
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  switch (clock_type) {
    case RCL_CLOCK_UNINITIALIZED:
      // 检查 clock 参数是否为空，为空则返回 RCL_RET_INVALID_ARGUMENT 错误
      RCL_CHECK_ARGUMENT_FOR_NULL(clock, RCL_RET_INVALID_ARGUMENT);
      // 初始化通用时钟
      rcl_init_generic_clock(clock, allocator);
      return RCL_RET_OK;
    case RCL_ROS_TIME:
      // 初始化 ROS 时钟
      return rcl_ros_clock_init(clock, allocator);
    case RCL_SYSTEM_TIME:
      // 初始化系统时钟
      return rcl_system_clock_init(clock, allocator);
    case RCL_STEADY_TIME:
      // 初始化稳定时钟
      return rcl_steady_clock_init(clock, allocator);
    default:
      // 无效的时钟类型，返回 RCL_RET_INVALID_ARGUMENT 错误
      return RCL_RET_INVALID_ARGUMENT;
  }
}

/**
 * @brief 清理通用时钟
 *
 * @param[in,out] clock 指向 rcl_clock_t 结构体的指针，用于存储需要清理的时钟信息
 */
static void rcl_clock_generic_fini(rcl_clock_t *clock) {
  // 内部函数; 假设调用者已经检查过 clock 是否有效
  if (clock->num_jump_callbacks > 0) {
    // 将跳转回调数量设置为 0
    clock->num_jump_callbacks = 0;
    // 释放跳转回调所占用的内存
    clock->allocator.deallocate(clock->jump_callbacks, clock->allocator.state);
    // 将跳转回调指针设置为 NULL
    clock->jump_callbacks = NULL;
  }
}

/**
 * @brief 销毁一个rcl_clock_t类型的时钟对象
 *
 * @param[in] clock 指向要销毁的时钟对象的指针
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_clock_fini(rcl_clock_t *clock) {
  // 检查clock参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(clock, RCL_RET_INVALID_ARGUMENT);
  // 检查clock的分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(
      &clock->allocator, "clock has invalid allocator", return RCL_RET_ERROR);
  // 根据时钟类型执行相应的销毁操作
  switch (clock->type) {
    case RCL_ROS_TIME:
      return rcl_ros_clock_fini(clock);
    case RCL_SYSTEM_TIME:
      return rcl_system_clock_fini(clock);
    case RCL_STEADY_TIME:
      return rcl_steady_clock_fini(clock);
    case RCL_CLOCK_UNINITIALIZED:
    // fall through
    default:
      return RCL_RET_INVALID_ARGUMENT;
  }
}

/**
 * @brief 初始化一个ROS时钟对象
 *
 * @param[out] clock 指向要初始化的时钟对象的指针
 * @param[in] allocator 分配器用于分配内存
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_ros_clock_init(rcl_clock_t *clock, rcl_allocator_t *allocator) {
  // 检查clock和allocator参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(clock, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(allocator, RCL_RET_INVALID_ARGUMENT);
  // 初始化通用时钟对象
  rcl_init_generic_clock(clock, allocator);
  // 为ROS时钟分配内存
  clock->data = allocator->allocate(sizeof(rcl_ros_clock_storage_t), allocator->state);
  // 检查内存分配是否成功
  if (NULL == clock->data) {
    RCL_SET_ERROR_MSG("allocating memory failed");
    return RCL_RET_BAD_ALLOC;
  }

  // 获取ROS时钟存储结构体指针
  rcl_ros_clock_storage_t *storage = (rcl_ros_clock_storage_t *)clock->data;
  // 初始化当前时间为0，表示时间尚未设置
  atomic_init(&(storage->current_time), 0);
  // 设置时钟为非激活状态
  storage->active = false;
  // 设置获取当前时间的函数指针
  clock->get_now = rcl_get_ros_time;
  // 设置时钟类型为ROS时间
  clock->type = RCL_ROS_TIME;
  return RCL_RET_OK;
}

/**
 * @brief 销毁一个 ROS 时钟
 *
 * @param[in] clock 指向要销毁的 rcl_clock_t 结构体的指针
 * @return 返回 RCL_RET_OK 表示成功，其他值表示失败
 */
rcl_ret_t rcl_ros_clock_fini(rcl_clock_t *clock) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(clock, RCL_RET_INVALID_ARGUMENT);

  // 判断时钟类型是否为 RCL_ROS_TIME
  if (clock->type != RCL_ROS_TIME) {
    RCL_SET_ERROR_MSG("clock not of type RCL_ROS_TIME");
    return RCL_RET_ERROR;
  }

  // 调用通用时钟销毁函数
  rcl_clock_generic_fini(clock);

  // 释放时钟数据内存
  clock->allocator.deallocate(clock->data, clock->allocator.state);
  clock->data = NULL;

  return RCL_RET_OK;
}

/**
 * @brief 初始化一个稳定时钟
 *
 * @param[in] clock 指向要初始化的 rcl_clock_t 结构体的指针
 * @param[in] allocator 指向分配器的指针
 * @return 返回 RCL_RET_OK 表示成功，其他值表示失败
 */
rcl_ret_t rcl_steady_clock_init(rcl_clock_t *clock, rcl_allocator_t *allocator) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(clock, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(allocator, RCL_RET_INVALID_ARGUMENT);

  // 调用通用时钟初始化函数
  rcl_init_generic_clock(clock, allocator);

  // 设置获取当前时间的函数指针
  clock->get_now = rcl_get_steady_time;

  // 设置时钟类型为 RCL_STEADY_TIME
  clock->type = RCL_STEADY_TIME;

  return RCL_RET_OK;
}

/**
 * @brief 销毁一个稳定时钟
 *
 * @param[in] clock 指向要销毁的 rcl_clock_t 结构体的指针
 * @return 返回 RCL_RET_OK 表示成功，其他值表示失败
 */
rcl_ret_t rcl_steady_clock_fini(rcl_clock_t *clock) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(clock, RCL_RET_INVALID_ARGUMENT);

  // 判断时钟类型是否为 RCL_STEADY_TIME
  if (clock->type != RCL_STEADY_TIME) {
    RCL_SET_ERROR_MSG("clock not of type RCL_STEADY_TIME");
    return RCL_RET_ERROR;
  }

  // 调用通用时钟销毁函数
  rcl_clock_generic_fini(clock);

  return RCL_RET_OK;
}

/**
 * @brief 初始化系统时钟
 * @param clock 指向rcl_clock_t结构体的指针，用于存储初始化后的时钟信息
 * @param allocator 指向rcl_allocator_t结构体的指针，用于分配内存
 * @return 返回rcl_ret_t类型的结果，表示操作成功或失败
 */
rcl_ret_t rcl_system_clock_init(rcl_clock_t *clock, rcl_allocator_t *allocator) {
  // 检查clock参数是否为空，如果为空则返回错误码RCL_RET_INVALID_ARGUMENT
  RCL_CHECK_ARGUMENT_FOR_NULL(clock, RCL_RET_INVALID_ARGUMENT);
  // 检查allocator参数是否为空，如果为空则返回错误码RCL_RET_INVALID_ARGUMENT
  RCL_CHECK_ARGUMENT_FOR_NULL(allocator, RCL_RET_INVALID_ARGUMENT);
  // 初始化通用时钟
  rcl_init_generic_clock(clock, allocator);
  // 设置获取当前时间的函数指针为rcl_get_system_time
  clock->get_now = rcl_get_system_time;
  // 设置时钟类型为RCL_SYSTEM_TIME
  clock->type = RCL_SYSTEM_TIME;
  // 返回操作成功的结果码RCL_RET_OK
  return RCL_RET_OK;
}

/**
 * @brief 终止系统时钟
 * @param clock 指向rcl_clock_t结构体的指针，用于存储需要终止的时钟信息
 * @return 返回rcl_ret_t类型的结果，表示操作成功或失败
 */
rcl_ret_t rcl_system_clock_fini(rcl_clock_t *clock) {
  // 检查clock参数是否为空，如果为空则返回错误码RCL_RET_INVALID_ARGUMENT
  RCL_CHECK_ARGUMENT_FOR_NULL(clock, RCL_RET_INVALID_ARGUMENT);
  // 检查时钟类型是否为RCL_SYSTEM_TIME，如果不是则设置错误信息并返回错误码RCL_RET_ERROR
  if (clock->type != RCL_SYSTEM_TIME) {
    RCL_SET_ERROR_MSG("clock not of type RCL_SYSTEM_TIME");
    return RCL_RET_ERROR;
  }
  // 终止通用时钟
  rcl_clock_generic_fini(clock);
  // 返回操作成功的结果码RCL_RET_OK
  return RCL_RET_OK;
}

/**
 * @brief 计算两个时间点之间的差值
 * @param start 指向rcl_time_point_t结构体的指针，表示起始时间点
 * @param finish 指向rcl_time_point_t结构体的指针，表示结束时间点
 * @param delta 指向rcl_duration_t结构体的指针，用于存储计算出的时间差
 * @return 返回rcl_ret_t类型的结果，表示操作成功或失败
 */
rcl_ret_t rcl_difference_times(
    const rcl_time_point_t *start, const rcl_time_point_t *finish, rcl_duration_t *delta) {
  // 检查start参数是否为空，如果为空则返回错误码RCL_RET_INVALID_ARGUMENT
  RCL_CHECK_ARGUMENT_FOR_NULL(start, RCL_RET_INVALID_ARGUMENT);
  // 检查finish参数是否为空，如果为空则返回错误码RCL_RET_INVALID_ARGUMENT
  RCL_CHECK_ARGUMENT_FOR_NULL(finish, RCL_RET_INVALID_ARGUMENT);
  // 检查delta参数是否为空，如果为空则返回错误码RCL_RET_INVALID_ARGUMENT
  RCL_CHECK_ARGUMENT_FOR_NULL(delta, RCL_RET_INVALID_ARGUMENT);
  // 检查两个时间点的时钟类型是否相同，如果不同则设置错误信息并返回错误码RCL_RET_ERROR
  if (start->clock_type != finish->clock_type) {
    RCL_SET_ERROR_MSG("Cannot difference between time points with clocks types.");
    return RCL_RET_ERROR;
  }
  // 计算时间差，并将结果存储在delta中
  if (finish->nanoseconds < start->nanoseconds) {
    rcl_time_point_value_t intermediate = start->nanoseconds - finish->nanoseconds;
    delta->nanoseconds = -1 * (int64_t)intermediate;
  } else {
    delta->nanoseconds = (int64_t)(finish->nanoseconds - start->nanoseconds);
  }
  // 返回操作成功的结果码RCL_RET_OK
  return RCL_RET_OK;
}

/**
 * @brief 获取当前时间点的值
 *
 * @param[in] clock 指向rcl_clock_t类型的指针，表示一个时钟实例
 * @param[out] time_point_value 用于存储当前时间点值的指针
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_clock_get_now(rcl_clock_t *clock, rcl_time_point_value_t *time_point_value) {
  // 可以返回RCL_RET_INVALID_ARGUMENT错误
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  // 可以返回RCL_RET_ERROR错误
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_ERROR);

  // 检查clock参数是否为空，为空则返回RCL_RET_INVALID_ARGUMENT错误
  RCL_CHECK_ARGUMENT_FOR_NULL(clock, RCL_RET_INVALID_ARGUMENT);
  // 检查time_point_value参数是否为空，为空则返回RCL_RET_INVALID_ARGUMENT错误
  RCL_CHECK_ARGUMENT_FOR_NULL(time_point_value, RCL_RET_INVALID_ARGUMENT);
  if (clock->type && clock->get_now) {
    // 如果clock已初始化且get_now函数已注册，则调用get_now函数获取当前时间点值
    return clock->get_now(clock->data, time_point_value);
  }
  // 设置错误信息为"Clock is not initialized or does not have get_now registered."
  RCL_SET_ERROR_MSG("Clock is not initialized or does not have get_now registered.");
  // 返回RCL_RET_ERROR错误
  return RCL_RET_ERROR;
}

/**
 * @brief 调用时钟跳变回调函数
 *
 * @param[in] clock 指向rcl_clock_t类型的指针，表示一个时钟实例
 * @param[in] time_jump 指向rcl_time_jump_t类型的指针，表示时间跳变信息
 * @param[in] before_jump 布尔值，表示是否在跳变之前调用回调函数
 */
static void rcl_clock_call_callbacks(
    rcl_clock_t *clock, const rcl_time_jump_t *time_jump, bool before_jump) {
  // 内部函数; 假设参数是有效的
  // 判断是否为时钟状态改变（激活或停用）
  bool is_clock_change = time_jump->clock_change == RCL_ROS_TIME_ACTIVATED ||
                         time_jump->clock_change == RCL_ROS_TIME_DEACTIVATED;
  // 遍历所有注册的跳变回调函数
  for (size_t cb_idx = 0; cb_idx < clock->num_jump_callbacks; ++cb_idx) {
    rcl_jump_callback_info_t *info = &(clock->jump_callbacks[cb_idx]);
    if (
        // 如果是时钟状态改变且回调阈值设置了on_clock_change
        (is_clock_change && info->threshold.on_clock_change) ||
        // 或者回调阈值的min_backward小于0且时间跳变的delta小于等于min_backward
        (info->threshold.min_backward.nanoseconds < 0 &&
         time_jump->delta.nanoseconds <= info->threshold.min_backward.nanoseconds) ||
        // 或者回调阈值的min_forward大于0且时间跳变的delta大于等于min_forward
        (info->threshold.min_forward.nanoseconds > 0 &&
         time_jump->delta.nanoseconds >= info->threshold.min_forward.nanoseconds)) {
      // 调用回调函数
      info->callback(time_jump, before_jump, info->user_data);
    }
  }
}

/**
 * @brief 启用ROS时间覆盖功能
 *
 * @param[in] clock 指向rcl_clock_t结构体的指针
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_enable_ros_time_override(rcl_clock_t *clock) {
  // 检查clock参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(clock, RCL_RET_INVALID_ARGUMENT);

  // 判断时钟类型是否为RCL_ROS_TIME
  if (clock->type != RCL_ROS_TIME) {
    RCL_SET_ERROR_MSG("Clock is not of type RCL_ROS_TIME, cannot enable override.");
    return RCL_RET_ERROR;
  }

  // 获取时钟存储数据
  rcl_ros_clock_storage_t *storage = (rcl_ros_clock_storage_t *)clock->data;

  // 检查存储数据是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(
      storage, "Clock storage is not initialized, cannot enable override.", return RCL_RET_ERROR);

  // 如果时钟未激活，则激活并调用回调函数
  if (!storage->active) {
    rcl_time_jump_t time_jump;
    time_jump.delta.nanoseconds = 0;
    time_jump.clock_change = RCL_ROS_TIME_ACTIVATED;
    rcl_clock_call_callbacks(clock, &time_jump, true);
    storage->active = true;
    rcl_clock_call_callbacks(clock, &time_jump, false);
  }

  return RCL_RET_OK;
}

/**
 * @brief 禁用ROS时间覆盖功能
 *
 * @param[in] clock 指向rcl_clock_t结构体的指针
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_disable_ros_time_override(rcl_clock_t *clock) {
  // 检查clock参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(clock, RCL_RET_INVALID_ARGUMENT);

  // 判断时钟类型是否为RCL_ROS_TIME
  if (clock->type != RCL_ROS_TIME) {
    RCL_SET_ERROR_MSG("Clock is not of type RCL_ROS_TIME, cannot disable override.");
    return RCL_RET_ERROR;
  }

  // 获取时钟存储数据
  rcl_ros_clock_storage_t *storage = (rcl_ros_clock_storage_t *)clock->data;

  // 检查存储数据是否为空
  RCL_CHECK_FOR_NULL_WITH_MSG(
      storage, "Clock storage is not initialized, cannot enable override.", return RCL_RET_ERROR);

  // 如果时钟已激活，则禁用并调用回调函数
  if (storage->active) {
    rcl_time_jump_t time_jump;
    time_jump.delta.nanoseconds = 0;
    time_jump.clock_change = RCL_ROS_TIME_DEACTIVATED;
    rcl_clock_call_callbacks(clock, &time_jump, true);
    storage->active = false;
    rcl_clock_call_callbacks(clock, &time_jump, false);
  }

  return RCL_RET_OK;
}

/**
 * @brief 检查 ROS 时间覆盖是否启用
 *
 * @param[in] clock 指向 rcl_clock_t 结构体的指针
 * @param[out] is_enabled 用于存储结果的布尔指针，如果启用了时间覆盖，则为 true，否则为 false
 * @return rcl_ret_t 返回 RCL_RET_OK 表示成功，其他值表示失败
 */
rcl_ret_t rcl_is_enabled_ros_time_override(rcl_clock_t *clock, bool *is_enabled) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(clock, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(is_enabled, RCL_RET_INVALID_ARGUMENT);

  // 检查时钟类型是否为 RCL_ROS_TIME
  if (clock->type != RCL_ROS_TIME) {
    RCL_SET_ERROR_MSG("Clock is not of type RCL_ROS_TIME, cannot query override state.");
    return RCL_RET_ERROR;
  }

  // 获取时钟存储
  rcl_ros_clock_storage_t *storage = (rcl_ros_clock_storage_t *)clock->data;

  // 检查时钟存储是否已初始化
  RCL_CHECK_FOR_NULL_WITH_MSG(
      storage, "Clock storage is not initialized, cannot enable override.", return RCL_RET_ERROR);

  // 设置 is_enabled 的值
  *is_enabled = storage->active;

  return RCL_RET_OK;
}

/**
 * @brief 设置 ROS 时间覆盖
 *
 * @param[in] clock 指向 rcl_clock_t 结构体的指针
 * @param[in] time_value 要设置的时间值
 * @return rcl_ret_t 返回 RCL_RET_OK 表示成功，其他值表示失败
 */
rcl_ret_t rcl_set_ros_time_override(rcl_clock_t *clock, rcl_time_point_value_t time_value) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(clock, RCL_RET_INVALID_ARGUMENT);

  // 检查时钟类型是否为 RCL_ROS_TIME
  if (clock->type != RCL_ROS_TIME) {
    RCL_SET_ERROR_MSG("Clock is not of type RCL_ROS_TIME, cannot set time override.");
    return RCL_RET_ERROR;
  }

  // 获取时钟存储
  rcl_ros_clock_storage_t *storage = (rcl_ros_clock_storage_t *)clock->data;

  // 检查时钟存储是否已初始化
  RCL_CHECK_FOR_NULL_WITH_MSG(
      storage, "Clock storage is not initialized, cannot enable override.", return RCL_RET_ERROR);

  rcl_time_jump_t time_jump;

  // 如果时间覆盖已启用
  if (storage->active) {
    time_jump.clock_change = RCL_ROS_TIME_NO_CHANGE;

    // 获取当前 ROS 时间
    rcl_time_point_value_t current_time;
    rcl_ret_t ret = rcl_get_ros_time(storage, &current_time);
    if (RCL_RET_OK != ret) {
      return ret;
    }

    // 计算时间跳跃的差值
    time_jump.delta.nanoseconds = time_value - current_time;

    // 调用回调函数处理时间跳跃
    rcl_clock_call_callbacks(clock, &time_jump, true);

    // 更新当前时间
    rcutils_atomic_store(&(storage->current_time), time_value);

    // 再次调用回调函数处理时间跳跃
    rcl_clock_call_callbacks(clock, &time_jump, false);
  } else {
    // 如果时间覆盖未启用，直接更新当前时间
    rcutils_atomic_store(&(storage->current_time), time_value);
  }

  return RCL_RET_OK;
}

/**
 * @brief 添加跳变回调函数到时钟中
 *
 * @param[in] clock 指向 rcl_clock_t 结构体的指针，用于存储时钟信息
 * @param[in] threshold 跳变阈值，包含正向和反向跳变的最小时间
 * @param[in] callback 跳变回调函数，当时钟发生跳变时将被调用
 * @param[in] user_data 用户数据，将传递给回调函数
 * @return rcl_ret_t 返回 RCL_RET_OK 表示成功，其他值表示失败
 */
rcl_ret_t rcl_clock_add_jump_callback(
    rcl_clock_t *clock,
    rcl_jump_threshold_t threshold,
    rcl_jump_callback_t callback,
    void *user_data) {
  // 确保参数有效
  RCL_CHECK_ARGUMENT_FOR_NULL(clock, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ALLOCATOR_WITH_MSG(
      &(clock->allocator), "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(callback, RCL_RET_INVALID_ARGUMENT);
  if (threshold.min_forward.nanoseconds < 0) {
    RCL_SET_ERROR_MSG("forward jump threshold must be positive or zero");
    return RCL_RET_INVALID_ARGUMENT;
  }
  if (threshold.min_backward.nanoseconds > 0) {
    RCL_SET_ERROR_MSG("backward jump threshold must be negative or zero");
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 回调函数/用户数据对必须是唯一的
  for (size_t cb_idx = 0; cb_idx < clock->num_jump_callbacks; ++cb_idx) {
    const rcl_jump_callback_info_t *info = &(clock->jump_callbacks[cb_idx]);
    if (info->callback == callback && info->user_data == user_data) {
      RCL_SET_ERROR_MSG("callback/user_data are already added to this clock");
      return RCL_RET_ERROR;
    }
  }

  // 添加新的回调函数，增加回调列表的大小
  rcl_jump_callback_info_t *callbacks = clock->allocator.reallocate(
      clock->jump_callbacks, sizeof(rcl_jump_callback_info_t) * (clock->num_jump_callbacks + 1),
      clock->allocator.state);
  if (NULL == callbacks) {
    RCL_SET_ERROR_MSG("Failed to realloc jump callbacks");
    return RCL_RET_BAD_ALLOC;
  }
  clock->jump_callbacks = callbacks;
  clock->jump_callbacks[clock->num_jump_callbacks].callback = callback;
  clock->jump_callbacks[clock->num_jump_callbacks].threshold = threshold;
  clock->jump_callbacks[clock->num_jump_callbacks].user_data = user_data;
  ++(clock->num_jump_callbacks);
  return RCL_RET_OK;
}

/**
 * @brief 删除指定的跳变回调函数
 *
 * @param[in] clock 指向rcl_clock_t类型的指针，表示要操作的时钟对象
 * @param[in] callback 要删除的跳变回调函数
 * @param[in] user_data 与回调函数关联的用户数据
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_clock_remove_jump_callback(
    rcl_clock_t *clock, rcl_jump_callback_t callback, void *user_data) {
  // 确保参数有效
  RCL_CHECK_ARGUMENT_FOR_NULL(clock, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ALLOCATOR_WITH_MSG(
      &(clock->allocator), "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(callback, RCL_RET_INVALID_ARGUMENT);

  /**
   * @brief 在时钟的跳变回调列表中查找并删除指定的回调函数，同时将其后的所有回调向前移动一个位置。
   *
   * @param[in,out] clock 一个指向 rcl_clock_t 结构体的指针，用于存储时钟信息和回调函数列表
   * @param[in] callback 要删除的回调函数指针
   * @param[in] user_data 与要删除的回调函数关联的用户数据指针
   *
   * @return 返回 RCL_RET_ERROR 表示未找到指定的回调函数，否则表示成功
   */
  // 初始化 found_callback 标志为 false
  bool found_callback = false;

  // 遍历时钟的跳变回调列表
  for (size_t cb_idx = 0; cb_idx < clock->num_jump_callbacks; ++cb_idx) {
    // 获取当前遍历到的回调信息
    const rcl_jump_callback_info_t *info = &(clock->jump_callbacks[cb_idx]);

    // 如果已经找到了要删除的回调，则将后续回调向前移动一个位置
    if (found_callback) {
      clock->jump_callbacks[cb_idx - 1] = *info;
    } else if (info->callback == callback && info->user_data == user_data) {
      // 如果当前回调与要删除的回调匹配，则设置 found_callback 标志为 true
      found_callback = true;
    }
  }

  // 如果没有找到指定的回调函数，设置错误消息并返回 RCL_RET_ERROR
  if (!found_callback) {
    RCL_SET_ERROR_MSG("jump callback was not found");
    return RCL_RET_ERROR;
  }

  /**
   * @brief 减少时钟跳变回调的数量，如果为0，则释放内存。否则，重新分配内存以适应减少的回调数量。
   *
   * @param[in,out] clock 一个指向 rcl_clock_t 结构体的指针，用于存储时钟信息和回调函数列表
   *
   * @return 返回 RCL_RET_OK 表示成功，返回 RCL_RET_BAD_ALLOC 表示内存分配失败
   */
  if (--(clock->num_jump_callbacks) == 0) {
    // 当回调数量减少到0时，使用分配器释放 jump_callbacks 的内存，并将其设置为 NULL
    clock->allocator.deallocate(clock->jump_callbacks, clock->allocator.state);
    clock->jump_callbacks = NULL;
  } else {
    // 否则，根据新的回调数量重新分配内存
    rcl_jump_callback_info_t *callbacks = clock->allocator.reallocate(
        clock->jump_callbacks, sizeof(rcl_jump_callback_info_t) * clock->num_jump_callbacks,
        clock->allocator.state);

    // 如果重新分配失败，设置错误消息并返回 RCL_RET_BAD_ALLOC
    if (NULL == callbacks) {
      RCL_SET_ERROR_MSG("Failed to shrink jump callbacks");
      return RCL_RET_BAD_ALLOC;
    }

    // 更新 clock 的 jump_callbacks 指针
    clock->jump_callbacks = callbacks;
  }

  // 执行成功，返回 RCL_RET_OK
  return RCL_RET_OK;
}
