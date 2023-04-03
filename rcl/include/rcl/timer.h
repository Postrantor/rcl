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

/// @file

#ifndef RCL__TIMER_H_
#define RCL__TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "rcl/allocator.h"
#include "rcl/context.h"
#include "rcl/event_callback.h"
#include "rcl/guard_condition.h"
#include "rcl/macros.h"
#include "rcl/time.h"
#include "rcl/types.h"
#include "rmw/rmw.h"

/// rcl_timer_impl_s 结构体的类型定义
typedef struct rcl_timer_impl_s rcl_timer_impl_t;

/// 封装 ROS 计时器的结构体
typedef struct rcl_timer_s
{
  /// 私有实现指针
  rcl_timer_impl_t * impl;
} rcl_timer_t;

/// 封装重置回调数据的结构体
typedef struct rcl_timer_on_reset_callback_data_s
{
  rcl_event_callback_t on_reset_callback;  ///< 重置回调函数
  const void * user_data;                  ///< 用户数据
  size_t reset_counter;                    ///< 重置计数器
} rcl_timer_on_reset_callback_data_t;

/// 计时器的用户回调签名
/**
 * 回调接收到的第一个参数是指向计时器的指针。
 * 这可以用于取消计时器、查询下一个计时器回调的时间、将回调替换为其他回调等。
 *
 * 唯一的注意事项是，函数 rcl_timer_get_time_since_last_call()
 * 将返回自上次调用此回调之前的时间，而不是上次调用的时间。
 * 因此，给定的第二个参数是自上次回调被调用以来的时间，因为该信息已经无法通过计时器获得。
 * 自上次回调调用以来的时间以纳秒为单位给出。
 */
typedef void (*rcl_timer_callback_t)(rcl_timer_t *, int64_t);

/// 返回一个零初始化的计时器
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_timer_t rcl_get_zero_initialized_timer(void);

/// 初始化计时器
/**
 * 计时器由时钟、回调函数和周期组成。
 * 计时器可以添加到等待集合中并等待，这样当计时器准备好执行时，等待集合将唤醒。
 *
 * 计时器只保存状态，不会自动调用回调。
 * 它不创建任何线程、注册中断或消耗信号。
 * 对于阻塞行为，它可以与等待集合和 rcl_wait() 结合使用。
 * 当 rcl_timer_is_ready() 返回 true 时，仍然需要使用 rcl_timer_call() 显式调用计时器。
 *
 * 计时器句柄必须是指向已分配且零初始化的 rcl_timer_t 结构体的指针。
 * 在已经初始化的计时器上调用此函数将失败。
 * 在已分配但未零初始化的计时器结构体上调用此函数是未定义的行为。
 *
 * 时钟句柄必须是指向已初始化的 rcl_clock_t 结构体的指针。
 * 时钟的生命周期必须超过计时器的生命周期。
 *
 * 周期是一个非负持续时间（而不是未来的绝对时间）。
 * 如果周期为 `0`，则始终准备好。
 *
 * 回调是一个可选参数。
 * 有效输入是指向函数回调的指针，或者 `NULL` 表示在 rcl 中不存储回调。
 * 如果回调为 `NULL`，则调用者客户端库负责触发计时器回调。
 * 否则，它必须是一个返回 void 并接受两个参数的函数，
 * 第一个参数是指向关联计时器的指针，第二个参数是 int64_t 类型，
 * 表示自上次调用以来的时间，或者如果这是对回调的第一次调用，则表示自计时器创建以来的时间。
 *
 * 预期用法：
 *
 * ```c
 * #include <rcl/rcl.h>
 *
 * void my_timer_callback(rcl_timer_t * timer, int64_t last_call_time)
 * {
 *   // 执行计时器工作...
 *   // 可选地重新配置、取消或重置计时器...
 * }
 *
 * rcl_context_t * context;  // 之前通过 rcl_init() 初始化...
 * rcl_clock_t clock;
 * rcl_allocator_t allocator = rcl_get_default_allocator();
 * rcl_ret_t ret = rcl_clock_init(RCL_STEADY_TIME, &clock, &allocator);
 * // ... 错误处理
 *
 * rcl_timer_t timer = rcl_get_zero_initialized_timer();
 * ret = rcl_timer_init(
 *   &timer, &clock, context, RCL_MS_TO_NS(100), my_timer_callback, allocator);
 * // ... 错误处理，使用等待集合或手动轮询计时器，然后清理
 * ret = rcl_timer_fini(&timer);
 * // ... 错误处理
 * ```
 *
 * <hr>
 * 属性              | 符合性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 是
 * 无锁              | 是 [1][2][3]
 * <i>[1] 如果 `atomic_is_lock_free()` 对于 `atomic_uintptr_t` 返回 true</i>
 *
 * <i>[2] 如果 `atomic_is_lock_free()` 对于 `atomic_uint_least64_t` 返回 true</i>
 *
 * <i>[3] 如果 `atomic_is_lock_free()` 对于 `atomic_bool` 返回 true</i>
 *
 * \param[inout] timer 要初始化的计时器句柄
 * \param[in] clock 提供当前时间的时钟
 * \param[in] context 计时器要关联的上下文
 * \param[in] period 回调之间的持续时间（以纳秒为单位）
 * \param[in] callback 每个周期调用的用户定义函数
 * \param[in] allocator 用于分配的分配器
 * \return #RCL_RET_OK 如果计时器成功初始化，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ALREADY_INIT 如果计时器已经初始化，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_ERROR 发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_timer_init(
  rcl_timer_t * timer, rcl_clock_t * clock, rcl_context_t * context, int64_t period,
  const rcl_timer_callback_t callback, rcl_allocator_t allocator);

/// 结束一个定时器。
/**
 * 此函数将释放任何内存并使定时器无效。
 *
 * 已经无效（零初始化）或 `NULL` 的定时器不会失败。
 *
 * 与在同一定时器对象上使用的任何 rcl_timer_* 函数，此函数都不是线程安全的。
 *
 * <hr>
 * 属性              | 遵循情况
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 是
 * 无锁              | 是 [1][2][3]
 * <i>[1] 如果 `atomic_is_lock_free()` 对于 `atomic_uintptr_t` 返回 true</i>
 *
 * <i>[2] 如果 `atomic_is_lock_free()` 对于 `atomic_uint_least64_t` 返回 true</i>
 *
 * <i>[3] 如果 `atomic_is_lock_free()` 对于 `atomic_bool` 返回 true</i>
 *
 * \param[inout] timer 要结束的定时器句柄。
 * \return #RCL_RET_OK 如果定时器成功结束，或
 * \return #RCL_RET_ERROR 发生未指定错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_timer_fini(rcl_timer_t * timer);

/// 调用定时器的回调并设置最后一次调用时间。
/**
 * 即使定时器的周期尚未过去，此函数也会调用回调并更改最后一次调用时间。
 * 调用代码需要首先调用 rcl_timer_is_ready() 以确保周期已过。
 * 如果回调指针为 `NULL`（在初始化中设置或在初始化后交换），则不触发回调。
 * 但是，客户端库仍应调用此函数以更新定时器的状态。
 * 此命令的操作顺序如下：
 *
 *  - 确保定时器尚未取消。
 *  - 将当前时间获取到一个临时 rcl_steady_time_point_t 中。
 *  - 用定时器的最后一次调用时间交换当前时间。
 *  - 调用回调，传递此定时器和自上次调用以来的时间。
 *  - 回调完成后返回。
 *
 * 在回调期间，可以取消定时器或修改其周期和/或回调。
 *
 * <hr>
 * 属性              | 遵循情况
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是 [1]
 * 使用原子操作      | 是
 * 无锁              | 是 [2]
 * <i>[1] 用户回调可能不是线程安全的</i>
 *
 * <i>[2] 如果 `atomic_is_lock_free()` 对于 `atomic_int_least64_t` 返回 true</i>
 *
 * \param[inout] timer 要调用的定时器句柄
 * \return #RCL_RET_OK 如果定时器成功调用，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_TIMER_INVALID 如果 timer->impl 无效，或
 * \return #RCL_RET_TIMER_CANCELED 如果定时器已取消，或
 * \return #RCL_RET_ERROR 发生未指定错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_timer_call(rcl_timer_t * timer);

/// 获取定时器的时钟。
/**
 * 此函数检索时钟指针并将其复制到给定变量中。
 *
 * clock 参数必须是指向已分配的 rcl_clock_t * 的指针。
 *
 * <hr>
 * 属性              | 遵循情况
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] timer 正在查询的定时器句柄
 * \param[out] clock 存储时钟的 rcl_clock_t *
 * \return #RCL_RET_OK 如果时钟成功检索，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_TIMER_INVALID 如果定时器无效。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_timer_clock(rcl_timer_t * timer, rcl_clock_t ** clock);

/// 计算是否应调用定时器。
/**
 * 如果距离下次调用的时间小于或等于 0 且定时器尚未取消，则结果为 true。
 * 否则，结果为 false，表示不应调用定时器。
 *
 * is_ready 参数必须指向一个已分配的 bool 对象，因为结果会复制到其中。
 *
 * <hr>
 * 属性              | 遵循情况
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 是
 * 无锁              | 是 [1]
 * <i>[1] 如果 `atomic_is_lock_free()` 对于 `atomic_int_least64_t` 返回 true</i>
 *
 * \param[in] timer 正在检查的定时器句柄
 * \param[out] is_ready 用于存储计算结果的布尔值
 * \return #RCL_RET_OK 如果成功检索到上次调用时间，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_TIMER_INVALID 如果 timer->impl 无效，或
 * \return #RCL_RET_ERROR 发生未指定错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_timer_is_ready(const rcl_timer_t * timer, bool * is_ready);

/// 计算并获取下次调用之前的时间（以纳秒为单位）。
/**
 * 此函数通过将定时器的周期添加到上次调用时间，并从当前时间中减去该和来计算下次调用的时间。
 * 下次调用的时间可以是正数，表示尚未准备好调用，因为自上次调用以来尚未过去周期。
 * 下次调用的时间也可以是 0 或负数，表示自上次调用以来已过去周期，应调用定时器。
 * 负值表示定时器调用逾期的时间量。
 *
 * `time_until_next_call` 参数必须指向一个已分配的 int64_t，因为将时间直到复制到该实例中。
 *
 * <hr>
 * 属性              | 遵循情况
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 是
 * 无锁              | 是 [1]
 * <i>[1] 如果 `atomic_is_lock_free()` 对于 `atomic_int_least64_t` 返回 true</i>
 *
 * \param[in] timer 正在查询的定时器句柄
 * \param[out] time_until_next_call 用于存储结果的输出变量
 * \return #RCL_RET_OK 如果成功计算了下次调用的时间，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_TIMER_INVALID 如果 timer->impl 无效，或
 * \return #RCL_RET_TIMER_CANCELED 如果定时器已取消，或
 * \return #RCL_RET_ERROR 发生未指定错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_timer_get_time_until_next_call(
  const rcl_timer_t * timer, int64_t * time_until_next_call);

/// 获取上一次调用 rcl_timer_call() 以来的时间。
/**
 * 此函数计算自上次调用以来的时间，并将其复制到给定的 int64_t 变量中。
 *
 * 在回调函数内部调用此函数不会返回自上次调用以来的时间，而是返回自当前回调被调用以来的时间。
 *
 * time_since_last_call 参数必须是指向已分配的 int64_t 的指针。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 是
 * 无锁              | 是 [1]
 * <i>[1] 如果 `atomic_is_lock_free()` 返回 true，则为 `atomic_int_least64_t`</i>
 *
 * \param[in] timer 正在查询的计时器句柄
 * \param[out] time_since_last_call 存储时间的结构体
 * \return #RCL_RET_OK 如果成功获取了上次调用的时间，或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或者
 * \return #RCL_RET_TIMER_INVALID 如果 timer->impl 无效，或者
 * \return #RCL_RET_ERROR 发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_timer_get_time_since_last_call(
  const rcl_timer_t * timer, int64_t * time_since_last_call);

/// 获取计时器的周期。
/**
 * 此函数检索周期并将其复制到给定的变量中。
 *
 * period 参数必须是指向已分配的 int64_t 的指针。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 是
 * 无锁              | 是 [1]
 * <i>[1] 如果 `atomic_is_lock_free()` 返回 true，则为 `atomic_int_least64_t`</i>
 *
 * \param[in] timer 正在查询的计时器句柄
 * \param[out] period 存储周期的 int64_t
 * \return #RCL_RET_OK 如果成功获取了周期，或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或者
 * \return #RCL_RET_TIMER_INVALID 如果 timer->impl 无效，或者
 * \return #RCL_RET_ERROR 发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_timer_get_period(const rcl_timer_t * timer, int64_t * period);

/// 交换计时器的周期并返回先前的周期。
/**
 * 此函数交换计时器中的周期，并将旧周期复制到给定的变量中。
 *
 * 交换（更改）周期不会影响已经等待的 wait sets。
 *
 * old_period 参数必须是指向已分配的 int64_t 的指针。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 是
 * 无锁              | 是 [1]
 * <i>[1] 如果 `atomic_is_lock_free()` 返回 true，则为 `atomic_int_least64_t`</i>
 *
 * \param[in] timer 正在修改的计时器句柄
 * \param[out] new_period 要交换到计时器中的 int64_t
 * \param[out] old_period 存储先前周期的 int64_t
 * \return #RCL_RET_OK 如果成功获取了周期，或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或者
 * \return #RCL_RET_TIMER_INVALID 如果 timer->impl 无效，或者
 * \return #RCL_RET_ERROR 发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_timer_exchange_period(
  const rcl_timer_t * timer, int64_t new_period, int64_t * old_period);

/// 返回当前计时器回调。
/**
 * 如果以下情况之一发生，此函数可能会失败并返回 `NULL`：
 *   - 计时器为 `NULL`
 *   - 计时器尚未初始化（实现无效）
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 是
 * 无锁              | 是 [1]
 * <i>[1] 如果 `atomic_is_lock_free()` 返回 true，则为 `atomic_int_least64_t`</i>
 *
 * \param[in] timer 应返回回调的计时器句柄
 * \return 指向回调的函数指针，如果发生错误则返回 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_timer_callback_t rcl_timer_get_callback(const rcl_timer_t * timer);

/// 交换当前计时器回调并返回当前回调。
/**
 * 如果以下情况之一发生，此函数可能会失败并返回 `NULL`：
 *   - 计时器为 `NULL`
 *   - 计时器尚未初始化（实现无效）
 *
 * 此函数可以将回调设置为 `NULL`，在这种情况下，当调用 rcl_timer_call 时，回调将被忽略。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 是
 * 无锁              | 是 [1]
 * <i>[1] 如果 `atomic_is_lock_free()` 返回 true，则为 `atomic_int_least64_t`</i>
 *
 * \param[inout] timer 应从中交换回调的计时器句柄
 * \param[in] new_callback 要交换到计时器中的回调
 * \return 指向旧回调的函数指针，如果发生错误则返回 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_timer_callback_t rcl_timer_exchange_callback(
  rcl_timer_t * timer, const rcl_timer_callback_t new_callback);

/// 取消定时器
/**
 * 当定时器被取消时，rcl_timer_is_ready() 将对该定时器返回 false，
 * 而 rcl_timer_call() 将失败并返回 RCL_RET_TIMER_CANCELED。
 *
 * 取消的定时器可以通过 rcl_timer_reset() 重置，然后再次使用。
 * 在已经取消的定时器上调用此函数将成功。
 *
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 是
 * 使用原子操作        | 是
 * 无锁                | 是 [1]
 * <i>[1] 如果 `atomic_is_lock_free()` 对于 `atomic_int_least64_t` 返回 true</i>
 *
 * \param[inout] timer 要取消的定时器
 * \return #RCL_RET_OK 如果定时器成功取消, 或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效, 或者
 * \return #RCL_RET_TIMER_INVALID 如果定时器无效。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_timer_cancel(rcl_timer_t * timer);

/// 获取定时器的取消状态
/**
 * 如果定时器被取消，is_canceled 参数中将存储 true。
 * 否则，is_canceled 参数中将存储 false。
 *
 * is_canceled 参数必须指向一个分配好的 bool，因为结果会复制到这个变量中。
 *
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 是
 * 使用原子操作        | 是
 * 无锁                | 是 [1]
 * <i>[1] 如果 `atomic_is_lock_free()` 对于 `atomic_bool` 返回 true</i>
 *
 * \param[in] timer 要查询的定时器
 * \param[out] is_canceled 存储是否取消的布尔值
 * \return #RCL_RET_OK 如果成功获取上次调用时间, 或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效, 或者
 * \return #RCL_RET_TIMER_INVALID 如果 timer->impl 无效, 或者
 * \return #RCL_RET_ERROR 发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_timer_is_canceled(const rcl_timer_t * timer, bool * is_canceled);

/// 重置定时器
/**
 * 此函数可以在已取消或未取消的定时器上调用。
 * 对于所有定时器，它将把上次调用时间重置为现在。
 * 对于已取消的定时器，它还会使定时器变为未取消状态。
 *
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 是
 * 使用原子操作        | 是
 * 无锁                | 是 [1]
 * <i>[1] 如果 `atomic_is_lock_free()` 对于 `atomic_int_least64_t` 返回 true</i>
 *
 * \param[inout] timer 要重置的定时器
 * \return #RCL_RET_OK 如果定时器成功重置, 或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效, 或者
 * \return #RCL_RET_TIMER_INVALID 如果定时器无效, 或者
 * \return #RCL_RET_ERROR 发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_timer_reset(rcl_timer_t * timer);

/// 返回定时器的分配器
/**
 * 此函数可能失败，因此返回 `NULL`，如果：
 *   - 定时器为 `NULL`
 *   - 定时器尚未初始化（实现无效）
 *
 * 返回的指针只在定时器对象有效期间有效。
 *
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 是
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[inout] timer 定时器对象的句柄
 * \return 指向分配器的指针，或者如果发生错误则返回 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const rcl_allocator_t * rcl_timer_get_allocator(const rcl_timer_t * timer);

/// 获取定时器用于在使用 ROSTime 时唤醒 waitset 的 guard condition
/**
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] timer 要查询的定时器
 * \return `NULL` 如果定时器无效或没有 guard condition, 或者
 * \return guard condition 指针。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_guard_condition_t * rcl_timer_get_guard_condition(const rcl_timer_t * timer);

/// 为定时器设置重置回调函数
/**
 * 此 API 设置在定时器重置时调用的回调函数。
 * 如果定时器已经被重置，回调将被调用。
 *
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 否
 *
 * \param[in] timer 要设置回调的定时器句柄
 * \param[in] on_reset_callback 当定时器重置时调用的回调
 * \param[in] user_data 在以后调用回调时提供，可以为 NULL
 * \return `RCL_RET_OK` 如果成功, 或者
 * \return `RCL_RET_INVALID_ARGUMENT` 如果 `timer` 为 NULL
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_timer_set_on_reset_callback(
  const rcl_timer_t * timer, rcl_event_callback_t on_reset_callback, const void * user_data);

#ifdef __cplusplus
}
#endif

#endif  // RCL__TIMER_H_
