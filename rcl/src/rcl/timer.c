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

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/timer.h"

#include <inttypes.h>

#include "rcl/error_handling.h"
#include "rcutils/logging_macros.h"
#include "rcutils/stdatomic_helper.h"
#include "rcutils/time.h"
#include "tracetools/tracetools.h"

/**
 * @struct rcl_timer_impl_s
 * @brief ROS2中的rcl定时器实现结构体
 */
struct rcl_timer_impl_s {
  /// 提供时间的时钟
  rcl_clock_t *clock;
  /// 关联的上下文
  rcl_context_t *context;
  /// 用于唤醒关联等待集的保护条件，当ROSTime导致计时器过期或计时器重置时触发
  rcl_guard_condition_t guard_condition;
  /// 用户提供的回调函数
  atomic_uintptr_t callback;
  /// 以纳秒为单位的持续时间，初始化为int64_t，用于内部时间计算
  atomic_int_least64_t period;
  /// 自未指定时间以来的纳秒时间
  atomic_int_least64_t last_call_time;
  /// 自未指定时间以来的纳秒时间
  atomic_int_least64_t next_call_time;
  /// 在ROS时间激活或停用之前经过的时间信用
  atomic_int_least64_t time_credit;
  /// 表示计时器是否已取消的标志
  atomic_bool canceled;
  /// 用户提供的分配器
  rcl_allocator_t allocator;
  /// 用户提供的重置回调数据
  rcl_timer_on_reset_callback_data_t callback_data;
};

/**
 * @brief 获取一个零初始化的rcl_timer_t实例
 *
 * @return 返回一个零初始化的rcl_timer_t实例
 */
rcl_timer_t rcl_get_zero_initialized_timer() {
  static rcl_timer_t null_timer = {0};
  return null_timer;
}

/**
 * @brief 处理时间跳变的回调函数，用于在 ROS 时间激活、停用或时间跳变时更新定时器状态。
 *
 * @param[in] time_jump 指向 rcl_time_jump_t 结构体的指针，包含时间跳变的信息。
 * @param[in] before_jump 布尔值，表示回调是否在时间跳变之前触发。若为 true，则在跳变之前触发；若为
 * false，则在跳变之后触发。
 * @param[in] user_data 用户数据，此处为 rcl_timer_t 类型的指针。
 */
void _rcl_timer_time_jump(const rcl_time_jump_t *time_jump, bool before_jump, void *user_data) {
  // 将用户数据转换为 rcl_timer_t 类型的指针
  rcl_timer_t *timer = (rcl_timer_t *)user_data;

  // 如果回调在时间跳变之前触发
  if (before_jump) {
    // 如果 ROS 时间被激活或停用
    if (RCL_ROS_TIME_ACTIVATED == time_jump->clock_change ||
        RCL_ROS_TIME_DEACTIVATED == time_jump->clock_change) {
      rcl_time_point_value_t now;
      // 获取当前时间
      if (RCL_RET_OK != rcl_clock_get_now(timer->impl->clock, &now)) {
        RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Failed to get current time in jump callback");
        return;
      }
      // 时间源发生变化，但定时器已经过了一部分周期
      // 在跳变前保存已经过去的时间，以便在新时代中只等待剩余时间
      if (0 == now) {
        // 如果时钟未初始化，则没有时间补偿
        return;
      }
      const int64_t next_call_time = rcutils_atomic_load_int64_t(&timer->impl->next_call_time);
      rcutils_atomic_store(&timer->impl->time_credit, next_call_time - now);
    }
  } else {
    rcl_time_point_value_t now;
    // 获取当前时间
    if (RCL_RET_OK != rcl_clock_get_now(timer->impl->clock, &now)) {
      RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Failed to get current time in jump callback");
      return;
    }
    const int64_t last_call_time = rcutils_atomic_load_int64_t(&timer->impl->last_call_time);
    const int64_t next_call_time = rcutils_atomic_load_int64_t(&timer->impl->next_call_time);
    const int64_t period = rcutils_atomic_load_int64_t(&timer->impl->period);
    // 如果 ROS 时间被激活或停用
    if (RCL_ROS_TIME_ACTIVATED == time_jump->clock_change ||
        RCL_ROS_TIME_DEACTIVATED == time_jump->clock_change) {
      // ROS 时间激活或停用
      if (0 == now) {
        // 如果时钟未初始化，则无法应用时间补偿
        return;
      }
      int64_t time_credit = rcutils_atomic_exchange_int64_t(&timer->impl->time_credit, 0);
      if (time_credit) {
        // 在新时代设置时间，使定时器只等待剩余周期
        rcutils_atomic_store(&timer->impl->next_call_time, now - time_credit + period);
        rcutils_atomic_store(&timer->impl->last_call_time, now - time_credit);
      }
    } else if (next_call_time <= now) {
      // 跳变后向前跳，且定时器已准备好
      if (RCL_RET_OK != rcl_trigger_guard_condition(&timer->impl->guard_condition)) {
        RCUTILS_LOG_ERROR_NAMED(
            ROS_PACKAGE_NAME, "Failed to get trigger guard condition in jump callback");
      }
    } else if (now < last_call_time) {
      // 向后跳过的时间跳变比一个周期还要长
      // 下一个回调应在一个周期后发生
      rcutils_atomic_store(&timer->impl->next_call_time, now + period);
      rcutils_atomic_store(&timer->impl->last_call_time, now);
      return;
    }
  }
}

/**
 * @brief 初始化一个定时器
 *
 * @param[in] timer 指向要初始化的定时器的指针
 * @param[in] clock 用于计时的时钟
 * @param[in] context rcl上下文
 * @param[in] period 定时器周期，以纳秒为单位
 * @param[in] callback 定时器回调函数
 * @param[in] allocator 分配器用于分配内存
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_timer_init(
    rcl_timer_t *timer,
    rcl_clock_t *clock,
    rcl_context_t *context,
    int64_t period,
    const rcl_timer_callback_t callback,
    rcl_allocator_t allocator) {
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(&allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  // 检查timer和clock参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(timer, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(clock, RCL_RET_INVALID_ARGUMENT);
  // 检查周期是否为非负数
  if (period < 0) {
    RCL_SET_ERROR_MSG("timer period must be non-negative");
    return RCL_RET_INVALID_ARGUMENT;
  }
  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(
      ROS_PACKAGE_NAME, "Initializing timer with period: %" PRIu64 "ns", period);
  // 检查定时器是否已经初始化
  if (timer->impl) {
    RCL_SET_ERROR_MSG("timer already initialized, or memory was uninitialized");
    return RCL_RET_ALREADY_INIT;
  }
  // 获取当前时间
  rcl_time_point_value_t now;
  rcl_ret_t now_ret = rcl_clock_get_now(clock, &now);
  if (now_ret != RCL_RET_OK) {
    return now_ret;  // rcl error state should already be set.
  }
  // 初始化定时器实现结构体
  rcl_timer_impl_t impl;
  impl.clock = clock;
  impl.context = context;
  impl.guard_condition = rcl_get_zero_initialized_guard_condition();
  rcl_guard_condition_options_t options = rcl_guard_condition_get_default_options();
  rcl_ret_t ret = rcl_guard_condition_init(&(impl.guard_condition), context, options);
  if (RCL_RET_OK != ret) {
    return ret;
  }
  // 如果时钟类型为ROS_TIME，添加跳跃回调
  if (RCL_ROS_TIME == impl.clock->type) {
    rcl_jump_threshold_t threshold;
    threshold.on_clock_change = true;
    threshold.min_forward.nanoseconds = 1;
    threshold.min_backward.nanoseconds = -1;
    ret = rcl_clock_add_jump_callback(clock, threshold, _rcl_timer_time_jump, timer);
    if (RCL_RET_OK != ret) {
      if (RCL_RET_OK != rcl_guard_condition_fini(&(impl.guard_condition))) {
        // Should be impossible
        RCUTILS_LOG_ERROR_NAMED(
            ROS_PACKAGE_NAME, "Failed to fini guard condition after failing to add jump callback");
      }
      return ret;
    }
  }
  // 初始化原子变量
  atomic_init(&impl.callback, (uintptr_t)callback);
  atomic_init(&impl.period, period);
  atomic_init(&impl.time_credit, 0);
  atomic_init(&impl.last_call_time, now);
  atomic_init(&impl.next_call_time, now + period);
  atomic_init(&impl.canceled, false);
  impl.allocator = allocator;

  // 初始化回调数据
  impl.callback_data.on_reset_callback = NULL;
  impl.callback_data.user_data = NULL;
  impl.callback_data.reset_counter = 0;

  // 分配内存并检查是否成功
  timer->impl = (rcl_timer_impl_t *)allocator.allocate(sizeof(rcl_timer_impl_t), allocator.state);
  if (NULL == timer->impl) {
    if (RCL_RET_OK != rcl_guard_condition_fini(&(impl.guard_condition))) {
      // Should be impossible
      RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Failed to fini guard condition after bad alloc");
    }
    if (RCL_RET_OK != rcl_clock_remove_jump_callback(clock, _rcl_timer_time_jump, timer)) {
      // Should be impossible
      RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Failed to remove callback after bad alloc");
    }

    RCL_SET_ERROR_MSG("allocating memory failed");
    return RCL_RET_BAD_ALLOC;
  }
  // 将初始化的结构体赋值给timer->impl
  *timer->impl = impl;
  TRACEPOINT(rcl_timer_init, (const void *)timer, period);
  return RCL_RET_OK;
}

/**
 * @brief 销毁一个rcl_timer_t类型的timer。
 *
 * @param[in,out] timer 指向要销毁的rcl_timer_t类型的timer的指针。
 * @return 返回RCL_RET_OK表示成功，其他值表示失败。
 */
rcl_ret_t rcl_timer_fini(rcl_timer_t *timer) {
  // 如果timer为空或者timer的实现为空，则返回RCL_RET_OK
  if (!timer || !timer->impl) {
    return RCL_RET_OK;
  }
  // 由于timer有效，将返回RCL_RET_OK或RCL_RET_ERROR
  rcl_ret_t result = rcl_timer_cancel(timer);
  rcl_allocator_t allocator = timer->impl->allocator;
  rcl_ret_t fail_ret;
  // 如果时钟类型为RCL_ROS_TIME
  if (RCL_ROS_TIME == timer->impl->clock->type) {
    // 跳跃回调使用保护条件，因此在释放下面的保护条件之前，我们必须先移除它
    fail_ret = rcl_clock_remove_jump_callback(timer->impl->clock, _rcl_timer_time_jump, timer);
    if (RCL_RET_OK != fail_ret) {
      RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Failed to remove timer jump callback");
    }
  }
  // 结束保护条件
  fail_ret = rcl_guard_condition_fini(&(timer->impl->guard_condition));
  if (RCL_RET_OK != fail_ret) {
    RCL_SET_ERROR_MSG("Failure to fini guard condition");
  }
  // 释放timer的实现内存
  allocator.deallocate(timer->impl, allocator.state);
  timer->impl = NULL;
  return result;
}

/**
 * @brief 获取与rcl_timer_t类型的timer关联的时钟。
 *
 * @param[in] timer 指向要查询的rcl_timer_t类型的timer的指针。
 * @param[out] clock 返回与timer关联的rcl_clock_t类型的时钟的指针。
 * @return 返回RCL_RET_OK表示成功，其他值表示失败。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_timer_clock(rcl_timer_t *timer, rcl_clock_t **clock) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(timer, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(clock, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(timer->impl, RCL_RET_TIMER_INVALID);

  // 设置clock为timer的实现中的时钟
  *clock = timer->impl->clock;

  return RCL_RET_OK;
}

/**
 * @brief 调用定时器回调函数并更新下一次调用时间。
 *
 * @param[in] timer 指向 rcl_timer_t 结构体的指针。
 * @return 返回 rcl_ret_t 类型的状态，表示操作成功或失败。
 */
rcl_ret_t rcl_timer_call(rcl_timer_t *timer) {
  // 打印调试信息
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Calling timer");

  // 检查 timer 和 timer->impl 是否为空，如果为空则返回错误
  RCL_CHECK_ARGUMENT_FOR_NULL(timer, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(timer->impl, RCL_RET_TIMER_INVALID);

  // 如果定时器已取消，则返回错误
  if (rcutils_atomic_load_bool(&timer->impl->canceled)) {
    RCL_SET_ERROR_MSG("timer is canceled");
    return RCL_RET_TIMER_CANCELED;
  }

  // 获取当前时间
  rcl_time_point_value_t now;
  rcl_ret_t now_ret = rcl_clock_get_now(timer->impl->clock, &now);
  if (now_ret != RCL_RET_OK) {
    return now_ret;  // rcl error state should already be set.
  }

  // 如果当前时间小于0，则返回错误
  if (now < 0) {
    RCL_SET_ERROR_MSG("clock now returned negative time point value");
    return RCL_RET_ERROR;
  }

  // 更新上次调用时间
  rcl_time_point_value_t previous_ns =
      rcutils_atomic_exchange_int64_t(&timer->impl->last_call_time, now);

  // 获取回调函数
  rcl_timer_callback_t typed_callback =
      (rcl_timer_callback_t)rcutils_atomic_load_uintptr_t(&timer->impl->callback);

  // 获取下一次调用时间和周期
  int64_t next_call_time = rcutils_atomic_load_int64_t(&timer->impl->next_call_time);
  int64_t period = rcutils_atomic_load_int64_t(&timer->impl->period);

  // 将下一次调用时间向前移动一个周期
  next_call_time += period;

  // 如果定时器至少错过了一个周期
  if (next_call_time < now) {
    if (0 == period) {
      // 周期为零的定时器始终准备好
      next_call_time = now;
    } else {
      // 将下一次调用时间向前移动所需的周期数
      int64_t now_ahead = now - next_call_time;
      // 向上取整，避免溢出
      int64_t periods_ahead = 1 + (now_ahead - 1) / period;
      next_call_time += periods_ahead * period;
    }
  }

  // 更新下一次调用时间
  rcutils_atomic_store(&timer->impl->next_call_time, next_call_time);

  // 如果回调函数不为空，则执行回调函数
  if (typed_callback != NULL) {
    int64_t since_last_call = now - previous_ns;
    typed_callback(timer, since_last_call);
  }

  // 返回成功状态
  return RCL_RET_OK;
}

/**
 * @brief 检查定时器是否准备好触发
 *
 * @param[in] timer 指向 rcl_timer_t 结构体的指针
 * @param[out] is_ready 用于存储结果的布尔值指针，如果定时器准备好触发，则为 true，否则为 false
 * @return rcl_ret_t 返回 RCL_RET_OK 表示成功，其他值表示失败
 */
rcl_ret_t rcl_timer_is_ready(const rcl_timer_t *timer, bool *is_ready) {
  // 检查 timer 参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(timer, RCL_RET_INVALID_ARGUMENT);
  // 检查 timer->impl 是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(timer->impl, RCL_RET_TIMER_INVALID);
  // 检查 is_ready 参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(is_ready, RCL_RET_INVALID_ARGUMENT);

  int64_t time_until_next_call;
  // 获取下次调用之前的剩余时间
  rcl_ret_t ret = rcl_timer_get_time_until_next_call(timer, &time_until_next_call);
  if (ret == RCL_RET_TIMER_CANCELED) {
    // 如果定时器已取消，则设置 is_ready 为 false
    *is_ready = false;
    return RCL_RET_OK;
  } else if (ret != RCL_RET_OK) {
    // 如果返回值不是 RCL_RET_OK，则直接返回错误码
    return ret;
  }
  // 如果剩余时间小于等于 0，则设置 is_ready 为 true，否则为 false
  *is_ready = (time_until_next_call <= 0);
  return RCL_RET_OK;
}

/**
 * @brief 获取定时器下次调用之前的剩余时间
 *
 * @param[in] timer 指向 rcl_timer_t 结构体的指针
 * @param[out] time_until_next_call 用于存储结果的 int64_t 类型指针
 * @return rcl_ret_t 返回 RCL_RET_OK 表示成功，其他值表示失败
 */
rcl_ret_t rcl_timer_get_time_until_next_call(
    const rcl_timer_t *timer, int64_t *time_until_next_call) {
  // 检查 timer 参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(timer, RCL_RET_INVALID_ARGUMENT);
  // 检查 timer->impl 是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(timer->impl, RCL_RET_TIMER_INVALID);
  // 检查 time_until_next_call 参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(time_until_next_call, RCL_RET_INVALID_ARGUMENT);

  // 如果定时器已取消，则返回 RCL_RET_TIMER_CANCELED
  if (rcutils_atomic_load_bool(&timer->impl->canceled)) {
    return RCL_RET_TIMER_CANCELED;
  }

  rcl_time_point_value_t now;
  // 获取当前时间
  rcl_ret_t ret = rcl_clock_get_now(timer->impl->clock, &now);
  if (ret != RCL_RET_OK) {
    // 如果返回值不是 RCL_RET_OK，则直接返回错误码
    return ret;
  }
  // 计算并设置下次调用之前的剩余时间
  *time_until_next_call = rcutils_atomic_load_int64_t(&timer->impl->next_call_time) - now;
  return RCL_RET_OK;
}

/**
 * @brief 获取自上次调用以来的时间
 *
 * @param[in] timer 指向 rcl_timer_t 结构体的指针
 * @param[out] time_since_last_call 自上次调用以来的时间（纳秒）
 * @return rcl_ret_t 返回 RCL_RET_OK 表示成功，其他值表示失败
 */
rcl_ret_t rcl_timer_get_time_since_last_call(
    const rcl_timer_t *timer, rcl_time_point_value_t *time_since_last_call) {
  // 检查参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(timer, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(timer->impl, RCL_RET_TIMER_INVALID);
  RCL_CHECK_ARGUMENT_FOR_NULL(time_since_last_call, RCL_RET_INVALID_ARGUMENT);

  // 获取当前时间
  rcl_time_point_value_t now;
  rcl_ret_t ret = rcl_clock_get_now(timer->impl->clock, &now);
  if (ret != RCL_RET_OK) {
    return ret;  // rcl error state should already be set.
  }

  // 计算自上次调用以来的时间
  *time_since_last_call = now - rcutils_atomic_load_int64_t(&timer->impl->last_call_time);
  return RCL_RET_OK;
}

/**
 * @brief 获取定时器周期
 *
 * @param[in] timer 指向 rcl_timer_t 结构体的指针
 * @param[out] period 定时器周期（纳秒）
 * @return rcl_ret_t 返回 RCL_RET_OK 表示成功，其他值表示失败
 */
rcl_ret_t rcl_timer_get_period(const rcl_timer_t *timer, int64_t *period) {
  // 检查参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(timer, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(timer->impl, RCL_RET_TIMER_INVALID);
  RCL_CHECK_ARGUMENT_FOR_NULL(period, RCL_RET_INVALID_ARGUMENT);

  // 获取定时器周期
  *period = rcutils_atomic_load_int64_t(&timer->impl->period);
  return RCL_RET_OK;
}

/**
 * @brief 交换定时器周期
 *
 * @param[in] timer 指向 rcl_timer_t 结构体的指针
 * @param[in] new_period 新的定时器周期（纳秒）
 * @param[out] old_period 旧的定时器周期（纳秒）
 * @return rcl_ret_t 返回 RCL_RET_OK 表示成功，其他值表示失败
 */
rcl_ret_t rcl_timer_exchange_period(
    const rcl_timer_t *timer, int64_t new_period, int64_t *old_period) {
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);

  // 检查参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(timer, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(timer->impl, RCL_RET_TIMER_INVALID);
  RCL_CHECK_ARGUMENT_FOR_NULL(old_period, RCL_RET_INVALID_ARGUMENT);

  // 交换定时器周期
  *old_period = rcutils_atomic_exchange_int64_t(&timer->impl->period, new_period);

  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(
      ROS_PACKAGE_NAME, "Updated timer period from '%" PRIu64 "ns' to '%" PRIu64 "ns'", *old_period,
      new_period);
  return RCL_RET_OK;
}

/**
 * @brief 获取定时器的回调函数
 *
 * @param[in] timer 指向 rcl_timer_t 结构体的指针
 * @return rcl_timer_callback_t 返回定时器的回调函数指针
 */
rcl_timer_callback_t rcl_timer_get_callback(const rcl_timer_t *timer) {
  // 检查 timer 参数是否为空，如果为空则返回 NULL
  RCL_CHECK_ARGUMENT_FOR_NULL(timer, NULL);
  // 检查 timer->impl 是否为空，如果为空则输出错误信息并返回 NULL
  RCL_CHECK_FOR_NULL_WITH_MSG(timer->impl, "timer is invalid", return NULL);
  // 返回 timer 的回调函数指针
  return (rcl_timer_callback_t)rcutils_atomic_load_uintptr_t(&timer->impl->callback);
}

/**
 * @brief 交换定时器的回调函数
 *
 * @param[in,out] timer 指向 rcl_timer_t 结构体的指针
 * @param[in] new_callback 新的回调函数指针
 * @return rcl_timer_callback_t 返回旧的回调函数指针
 */
rcl_timer_callback_t rcl_timer_exchange_callback(
    rcl_timer_t *timer, const rcl_timer_callback_t new_callback) {
  // 输出调试信息
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Updating timer callback");
  // 检查 timer 参数是否为空，如果为空则返回 NULL
  RCL_CHECK_ARGUMENT_FOR_NULL(timer, NULL);
  // 检查 timer->impl 是否为空，如果为空则输出错误信息并返回 NULL
  RCL_CHECK_FOR_NULL_WITH_MSG(timer->impl, "timer is invalid", return NULL);
  // 交换 timer 的回调函数指针，并返回旧的回调函数指针
  return (rcl_timer_callback_t)rcutils_atomic_exchange_uintptr_t(
      &timer->impl->callback, (uintptr_t)new_callback);
}

/**
 * @brief 取消定时器
 *
 * @param[in,out] timer 指向 rcl_timer_t 结构体的指针
 * @return rcl_ret_t 返回操作结果，成功则返回 RCL_RET_OK
 */
rcl_ret_t rcl_timer_cancel(rcl_timer_t *timer) {
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_TIMER_INVALID);

  // 检查 timer 参数是否为空，如果为空则返回 RCL_RET_INVALID_ARGUMENT
  RCL_CHECK_ARGUMENT_FOR_NULL(timer, RCL_RET_INVALID_ARGUMENT);
  // 检查 timer->impl 是否为空，如果为空则输出错误信息并返回 RCL_RET_TIMER_INVALID
  RCL_CHECK_FOR_NULL_WITH_MSG(timer->impl, "timer is invalid", return RCL_RET_TIMER_INVALID);
  // 将 timer 的 canceled 标志设置为 true
  rcutils_atomic_store(&timer->impl->canceled, true);
  // 输出调试信息
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Timer canceled");
  // 返回操作成功
  return RCL_RET_OK;
}

/**
 * @brief 判断定时器是否已取消
 *
 * @param[in] timer 指向 rcl_timer_t 结构体的指针
 * @param[out] is_canceled 用于存储判断结果的布尔值指针
 * @return rcl_ret_t 返回操作结果，成功则返回 RCL_RET_OK
 */
rcl_ret_t rcl_timer_is_canceled(const rcl_timer_t *timer, bool *is_canceled) {
  // 检查 timer 参数是否为空，如果为空则返回 RCL_RET_INVALID_ARGUMENT
  RCL_CHECK_ARGUMENT_FOR_NULL(timer, RCL_RET_INVALID_ARGUMENT);
  // 检查 timer->impl 是否为空，如果为空则返回 RCL_RET_TIMER_INVALID
  RCL_CHECK_ARGUMENT_FOR_NULL(timer->impl, RCL_RET_TIMER_INVALID);
  // 检查 is_canceled 参数是否为空，如果为空则返回 RCL_RET_INVALID_ARGUMENT
  RCL_CHECK_ARGUMENT_FOR_NULL(is_canceled, RCL_RET_INVALID_ARGUMENT);
  // 获取 timer 的 canceled 标志并存储到 is_canceled 中
  *is_canceled = rcutils_atomic_load_bool(&timer->impl->canceled);
  // 返回操作成功
  return RCL_RET_OK;
}

/**
 * @brief 重置定时器
 *
 * @param[in,out] timer 指向要重置的rcl_timer_t结构体的指针
 * @return 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_timer_reset(rcl_timer_t *timer) {
  // 检查参数是否有效，如果无效则返回错误
  RCUTILS_CAN_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);

  // 检查timer是否为空，如果为空则返回错误
  RCL_CHECK_ARGUMENT_FOR_NULL(timer, RCL_RET_INVALID_ARGUMENT);
  // 检查timer->impl是否为空，如果为空则返回错误
  RCL_CHECK_FOR_NULL_WITH_MSG(timer->impl, "timer is invalid", return RCL_RET_TIMER_INVALID);

  // 获取当前时间
  rcl_time_point_value_t now;
  rcl_ret_t now_ret = rcl_clock_get_now(timer->impl->clock, &now);
  if (now_ret != RCL_RET_OK) {
    return now_ret;  // rcl error state should already be set.
  }

  // 获取定时器周期
  int64_t period = rcutils_atomic_load_int64_t(&timer->impl->period);
  // 更新下一次调用时间
  rcutils_atomic_store(&timer->impl->next_call_time, now + period);
  // 设置定时器未取消状态
  rcutils_atomic_store(&timer->impl->canceled, false);
  // 触发保护条件
  rcl_ret_t ret = rcl_trigger_guard_condition(&timer->impl->guard_condition);

  // 获取回调数据
  rcl_timer_on_reset_callback_data_t *cb_data = &timer->impl->callback_data;

  // 如果存在回调函数，则执行回调
  if (cb_data->on_reset_callback) {
    cb_data->on_reset_callback(cb_data->user_data, 1);
  } else {
    // 否则，增加重置计数器
    cb_data->reset_counter++;
  }

  // 如果触发保护条件失败，记录错误日志
  if (ret != RCL_RET_OK) {
    RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Failed to trigger timer guard condition");
  }

  // 记录定时器重置成功的调试日志
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Timer successfully reset");

  return RCL_RET_OK;
}

/**
 * @brief 获取定时器的分配器
 *
 * @param[in] timer 指向rcl_timer_t结构体的指针
 * @return 返回指向rcl_allocator_t结构体的指针，如果出错则返回NULL
 */
const rcl_allocator_t *rcl_timer_get_allocator(const rcl_timer_t *timer) {
  // 检查timer是否为空，如果为空则返回NULL
  RCL_CHECK_ARGUMENT_FOR_NULL(timer, NULL);
  // 检查timer->impl是否为空，如果为空则返回NULL
  RCL_CHECK_FOR_NULL_WITH_MSG(timer->impl, "timer is invalid", return NULL);

  // 返回分配器指针
  return &timer->impl->allocator;
}

/**
 * @brief 获取与定时器关联的保护条件 (Get the guard condition associated with the timer)
 *
 * @param[in] timer 指向 rcl_timer_t 结构体的指针 (Pointer to an rcl_timer_t structure)
 * @return 如果成功，则返回指向 rcl_guard_condition_t 结构体的指针，否则返回 NULL (If successful,
 * returns a pointer to an rcl_guard_condition_t structure, otherwise returns NULL)
 */
rcl_guard_condition_t *rcl_timer_get_guard_condition(const rcl_timer_t *timer) {
  // 检查输入参数是否为 NULL (Check if input parameters are NULL)
  if (NULL == timer || NULL == timer->impl || NULL == timer->impl->guard_condition.impl) {
    return NULL;
  }
  // 返回与定时器关联的保护条件 (Return the guard condition associated with the timer)
  return &timer->impl->guard_condition;
}

/**
 * @brief 设置定时器重置回调函数 (Set the timer reset callback function)
 *
 * @param[in] timer 指向 rcl_timer_t 结构体的指针 (Pointer to an rcl_timer_t structure)
 * @param[in] on_reset_callback 定时器重置回调函数 (Timer reset callback function)
 * @param[in] user_data 用户数据，将传递给回调函数 (User data that will be passed to the callback
 * function)
 * @return 成功时返回 RCL_RET_OK，否则返回相应的错误代码 (Returns RCL_RET_OK on success, otherwise
 * returns the corresponding error code)
 */
rcl_ret_t rcl_timer_set_on_reset_callback(
    const rcl_timer_t *timer, rcl_event_callback_t on_reset_callback, const void *user_data) {
  // 检查输入参数是否为 NULL (Check if input parameters are NULL)
  RCL_CHECK_ARGUMENT_FOR_NULL(timer, RCL_RET_INVALID_ARGUMENT);

  // 获取回调数据结构体指针 (Get the pointer to the callback data structure)
  rcl_timer_on_reset_callback_data_t *cb_data = &timer->impl->callback_data;

  // 如果提供了回调函数 (If a callback function is provided)
  if (on_reset_callback) {
    // 设置回调函数和用户数据 (Set the callback function and user data)
    cb_data->on_reset_callback = on_reset_callback;
    cb_data->user_data = user_data;
    // 如果存在重置计数器 (If there is a reset counter)
    if (cb_data->reset_counter) {
      // 调用回调函数并传递用户数据和重置计数器 (Call the callback function with user data and reset
      // counter)
      cb_data->on_reset_callback(user_data, cb_data->reset_counter);
      // 重置计数器 (Reset the counter)
      cb_data->reset_counter = 0;
    }
  } else {
    // 清除回调函数和用户数据 (Clear the callback function and user data)
    cb_data->on_reset_callback = NULL;
    cb_data->user_data = NULL;
  }

  // 返回成功状态 (Return success status)
  return RCL_RET_OK;
}

#ifdef __cplusplus
}
#endif
