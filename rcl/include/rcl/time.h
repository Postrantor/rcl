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

#ifndef RCL__TIME_H_
#define RCL__TIME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/allocator.h"
#include "rcl/macros.h"
#include "rcl/types.h"
#include "rcl/visibility_control.h"
#include "rcutils/time.h"

/// 方便的宏，用于将秒转换为纳秒。
#define RCL_S_TO_NS RCUTILS_S_TO_NS
/// 方便的宏，用于将毫秒转换为纳秒。
#define RCL_MS_TO_NS RCUTILS_MS_TO_NS
/// 方便的宏，用于将微秒转换为纳秒。
#define RCL_US_TO_NS RCUTILS_US_TO_NS

/// 方便的宏，用于将纳秒转换为秒。
#define RCL_NS_TO_S RCUTILS_NS_TO_S
/// 方便的宏，用于将纳秒转换为毫秒。
#define RCL_NS_TO_MS RCUTILS_NS_TO_MS
/// 方便的宏，用于将纳秒转换为微秒。
#define RCL_NS_TO_US RCUTILS_NS_TO_US

/// 自 Unix 纪元以来的单个时间点，以纳秒为单位测量。
typedef rcutils_time_point_value_t rcl_time_point_value_t;
/// 以纳秒为单位测量的时间持续时间。
typedef rcutils_duration_value_t rcl_duration_value_t;

/// 时间源类型，用于指示时间测量的来源。
/**
 * RCL_ROS_TIME 将报告 ROS 时间源报告的最新值，或者
 * 如果 ROS 时间源未激活，则报告与 RCL_SYSTEM_TIME 相同的值。
 * 有关 ROS 时间源的更多信息，请参阅设计文档：
 * http://design.ros2.org/articles/clock_and_time.html
 *
 * RCL_SYSTEM_TIME 报告与系统时钟相同的值。
 *
 * RCL_STEADY_TIME 报告来自单调递增时钟的值。
 */
typedef enum rcl_clock_type_e {
  /// 时钟未初始化
  RCL_CLOCK_UNINITIALIZED = 0,
  /// 使用 ROS 时间
  RCL_ROS_TIME,
  /// 使用系统时间
  RCL_SYSTEM_TIME,
  /// 使用稳定时钟时间
  RCL_STEADY_TIME
} rcl_clock_type_t;

/// 以纳秒为单位测量的时间持续时间及其来源。
typedef struct rcl_duration_s
{
  /// 纳秒持续时间及其来源。
  rcl_duration_value_t nanoseconds;
} rcl_duration_t;

/// 用于描述时间跳跃类型的枚举。
typedef enum rcl_clock_change_e {
  /// 跳跃前后的源是 ROS_TIME。
  RCL_ROS_TIME_NO_CHANGE = 1,
  /// 源从 SYSTEM_TIME 切换到 ROS_TIME。
  RCL_ROS_TIME_ACTIVATED = 2,
  /// 源从 ROS_TIME 切换到 SYSTEM_TIME。
  RCL_ROS_TIME_DEACTIVATED = 3,
  /// 跳跃前后的源是 SYSTEM_TIME。
  RCL_SYSTEM_TIME_NO_CHANGE = 4
} rcl_clock_change_t;

/// 时间跳跃结构体描述
typedef struct rcl_time_jump_s
{
  /// 表示时间源是否发生了变化
  rcl_clock_change_t clock_change;
  /// 跳跃后的新时间减去跳跃前的最后时间
  rcl_duration_t delta;
} rcl_time_jump_t;

/// 时间跳跃回调函数签名
/// \param[in] time_jump 描述时间跳跃的结构体
/// \param[in] before_jump 每个跳跃回调都会被调用两次：在时钟改变之前和之后。这个参数在第一次调用时为true，第二次调用时为false。
/// \param[in] user_data 在回调注册时给定的指针，将传递给回调函数。
typedef void (*rcl_jump_callback_t)(
  const rcl_time_jump_t * time_jump, bool before_jump, void * user_data);

/// 描述调用时间跳跃回调所需的先决条件的结构体
typedef struct rcl_jump_threshold_s
{
  /// 当时钟类型发生变化时，设置为true以调用回调
  bool on_clock_change;
  /// 表示向前跳跃的最小值，当超过此值时，认为已经超过阈值，如果为零，则禁用。
  rcl_duration_t min_forward;
  /// 表示向后跳跃的最小值，当超过此值时，认为已经超过阈值，如果为零，则禁用。
  rcl_duration_t min_backward;
} rcl_jump_threshold_t;

/// 描述已添加回调的结构体
typedef struct rcl_jump_callback_info_s
{
  /// 回调函数指针
  rcl_jump_callback_t callback;
  /// 决定何时调用回调的阈值
  rcl_jump_threshold_t threshold;
  /// 传递给回调函数的指针
  void * user_data;
} rcl_jump_callback_info_t;

/// 时间源封装结构体
typedef struct rcl_clock_s
{
  /// 时钟类型
  rcl_clock_type_t type;
  /// 添加的跳跃回调数组
  rcl_jump_callback_info_t * jump_callbacks;
  /// jump_callbacks中的回调数量
  size_t num_jump_callbacks;
  /// 获取当前时间的函数指针
  rcl_ret_t (*get_now)(void * data, rcl_time_point_value_t * now);
  // 设置当前时间的函数指针（未使用）
  // void (*set_now) (rcl_time_point_value_t);
  /// 时钟存储
  void * data;
  /// 用于内部分配的自定义分配器
  rcl_allocator_t allocator;
} rcl_clock_t;

/// 单个时间点，以纳秒为单位，参考点基于源
typedef struct rcl_time_point_s
{
  /// 时间点的纳秒数
  rcl_time_point_value_t nanoseconds;
  /// 时间点的时钟类型
  rcl_clock_type_t clock_type;
} rcl_time_point_t;

// typedef struct rcl_rate_t
// {
//   rcl_time_point_value_t trigger_time;  ///< 触发时间点的值
//   int64_t period;                       ///< 周期，以纳秒为单位
//   rcl_clock_type_t clock;               ///< 时钟类型
// } rcl_rate_t;
// TODO(tfoote) integrate rate and timer implementations

/// 检查时钟是否已启动。
/**
 * 如果时钟包含非零的时间点值，则此函数返回 true。
 * 请注意，如果数据未初始化，可能会出现误报。
 *
 * 此功能主要用于检查使用 ROS 时间的时钟是否已启动。这是因为模拟器可能在暂停时初始化，
 * 导致 ROS 时间为 0，直到它恢复运行。
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | Yes
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param[in] clock 正在查询的时钟句柄
 * \return 如果时钟已启动，则返回 true，否则返回 false。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
bool rcl_clock_time_started(rcl_clock_t * clock);

/// 检查时钟是否具有有效值。
/**
 * 如果时间源看起来有效，则此函数返回 true。
 * 它将检查类型是否未初始化，并检查指针是否无效。
 * 请注意，如果数据未初始化，可能会出现误报。
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | Yes
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param[in] clock 正在查询的时钟句柄
 * \return 如果源被认为是有效的，则返回 true，否则返回 false。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
bool rcl_clock_valid(rcl_clock_t * clock);

/// 根据传递的类型初始化时钟。
/**
 * 这将分配所有必要的内部结构，并初始化变量。
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes [1]
 * Thread-Safe        | No [2]
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * <i>[1] 如果 `clock_type` 是 #RCL_ROS_TIME</i>
 * <i>[2] 函数是可重入的，但对同一 `clock` 对象的并发调用是不安全的。
 *        线程安全性还受到 `allocator` 对象的影响。</i>
 *
 * \param[in] clock_type 用于提供时间源的类型标识符
 * \param[in] clock 正在初始化的时钟句柄
 * \param[in] allocator 用于分配的分配器
 * \return 如果时间源成功初始化，则返回 #RCL_RET_OK，或者
 * \return 如果任何参数无效，则返回 #RCL_RET_INVALID_ARGUMENT，或者
 * \return 如果发生未指定的错误，则返回 #RCL_RET_ERROR。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_clock_init(
  rcl_clock_type_t clock_type, rcl_clock_t * clock, rcl_allocator_t * allocator);

/// 结束时钟。
/**
 * 这将释放所有必要的内部结构，并清理任何变量。
 * 它可以与任何 init 函数组合使用。
 *
 * 传递类型为 #RCL_CLOCK_UNINITIALIZED 的时钟将导致返回 #RCL_RET_INVALID_ARGUMENT。
 *
 * 此函数与对同一时钟对象进行操作的任何其他函数都不是线程安全的。
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No [1]
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * <i>[1] 函数是可重入的，但对同一 `clock` 对象的并发调用是不安全的。
 *        线程安全性还受到与 `clock` 对象关联的 `allocator` 对象的影响。</i>
 *
 * \param[in] clock 正在结束的时钟句柄
 * \return 如果时间源成功结束，则返回 #RCL_RET_OK，或者
 * \return 如果任何参数无效，则返回 #RCL_RET_INVALID_ARGUMENT，或者
 * \return 如果发生未指定的错误，则返回 #RCL_RET_ERROR。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_clock_fini(rcl_clock_t * clock);

/// 初始化一个 #RCL_ROS_TIME 时间源的时钟。
/**
 * 这将分配所有必要的内部结构，并初始化变量。
 * 它专门设置了一个 #RCL_ROS_TIME 时间源。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 是
 * 线程安全            | 否 [1]
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * <i>[2] 函数是可重入的，但在同一个 `clock` 对象上的并发调用是不安全的。
 *        线程安全性还受到 `allocator` 对象的影响。</i>
 *
 * \param[in] clock 正在初始化的时钟句柄
 * \param[in] allocator 用于分配的分配器
 * \return #RCL_RET_OK 如果时间源成功初始化，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_ERROR 发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_ros_clock_init(rcl_clock_t * clock, rcl_allocator_t * allocator);

/// 结束一个 #RCL_ROS_TIME 时间源的时钟。
/**
 * 这将释放所有必要的内部结构，并清理任何变量。
 * 它专门设置了一个 #RCL_ROS_TIME 时间源。它预计
 * 与 init 函数配对使用。
 *
 * 此函数与在同一时钟对象上操作的任何其他函数都不是线程安全的。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否 [1]
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * <i>[1] 函数是可重入的，但在同一个 `clock` 对象上的并发调用是不安全的。
 *        线程安全性还受到与 `clock` 对象关联的 `allocator` 对象的影响。</i>
 *
 * \param[in] clock 正在初始化的时钟句柄
 * \return #RCL_RET_OK 如果时间源成功结束，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_ros_clock_fini(rcl_clock_t * clock);

/// 初始化一个 #RCL_STEADY_TIME 时间源的时钟。
/**
 * 这将分配所有必要的内部结构，并初始化变量。
 * 它专门设置了一个 #RCL_STEADY_TIME 时间源。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否 [1]
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * <i>[1] 函数是可重入的，但在同一个 `clock` 对象上的并发调用是不安全的。
 *        线程安全性还受到 `allocator` 对象的影响。</i>
 *
 * \param[in] clock 正在初始化的时钟句柄
 * \param[in] allocator 用于分配的分配器
 * \return #RCL_RET_OK 如果时间源成功初始化，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_steady_clock_init(rcl_clock_t * clock, rcl_allocator_t * allocator);

/// 结束一个 #RCL_STEADY_TIME 时间源的时钟。
/**
 * 将时钟作为 #RCL_STEADY_TIME 时间源结束。
 *
 * 这将释放所有必要的内部结构，并清理任何变量。
 * 它专门设置了一个稳定的时间源。预计与 init 函数配对使用。
 *
 * 此函数与在同一时钟对象上操作的任何其他函数都不是线程安全的。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否 [1]
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * <i>[1] 函数是可重入的，但在同一个 `clock` 对象上的并发调用是不安全的。
 *        线程安全性还受到与 `clock` 对象关联的 `allocator` 对象的影响。</i>
 *
 * \param[in] clock 正在初始化的时钟句柄
 * \return #RCL_RET_OK 如果时间源成功结束，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_steady_clock_fini(rcl_clock_t * clock);

/// 初始化一个 #RCL_SYSTEM_TIME 时间源的时钟。
/**
 * 将时钟作为 #RCL_SYSTEM_TIME 时间源初始化。
 *
 * 这将分配所有必要的内部结构，并初始化变量。
 * 它专门设置了一个系统时间源。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否 [1]
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * <i>[1] 函数是可重入的，但在同一个 `clock` 对象上的并发调用是不安全的。
 *        线程安全性还受到与 `clock` 对象关联的 `allocator` 对象的影响。</i>
 *
 * \param[in] clock 正在初始化的时钟句柄
 * \param[in] allocator 用于分配的分配器
 * \return #RCL_RET_OK 如果时间源成功初始化，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_system_clock_init(rcl_clock_t * clock, rcl_allocator_t * allocator);

/// 作为 #RCL_SYSTEM_TIME 时间源，完成对时钟的初始化。
/**
 * 将时钟初始化为 #RCL_SYSTEM_TIME 时间源。
 *
 * 这将释放所有必要的内部结构，并清理任何变量。
 * 它专门设置系统时间源。预计与初始化函数配对使用。
 *
 * 此函数与对同一时钟对象进行操作的任何函数都不是线程安全的。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 否
 * 线程安全           | 否 [1]
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * <i>[1] 函数是可重入的，但在同一个 `clock` 对象上的并发调用是不安全的。
 *        线程安全性还受到与 `clock` 对象关联的 `allocator` 对象的影响。</i>
 *
 * \param[in] clock 正在初始化的时钟句柄。
 * \return #RCL_RET_OK 如果时间源成功完成，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_system_clock_fini(rcl_clock_t * clock);

/// 计算两个时间点之间的差值
/**
 * 此函数接受两个时间点，并计算它们之间的持续时间。
 * 两个时间点必须使用相同的时间抽象，结果持续时间也将具有相同的抽象。
 *
 * 值将计算为 duration = finish - start。如果开始在结束之后，持续时间将为负数。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 否
 * 线程安全           | 是
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * \param[in] start 持续时间的开始时间点。
 * \param[in] finish 持续时间的结束时间点。
 * \param[out] delta 开始和结束之间的持续时间。
 * \return #RCL_RET_OK 如果差值计算成功，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_difference_times(
  const rcl_time_point_t * start, const rcl_time_point_t * finish, rcl_duration_t * delta);

/// 使用关联时钟的当前值填充 time_point_value。
/**
 * 此函数将使用与其关联的时间抽象的当前值填充 time_point_value 对象的数据。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 否
 * 线程安全           | 是
 * 使用原子操作       | 是 [1]
 * 无锁               | 是
 *
 * <i>[1] 如果 `clock` 是 #RCL_ROS_TIME 类型。</i>
 *
 * \param[in] clock 用于设置值的时间源。
 * \param[out] time_point_value 要填充的 time_point 值。
 * \return #RCL_RET_OK 如果成功检索到上次调用时间，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_clock_get_now(rcl_clock_t * clock, rcl_time_point_value_t * time_point_value);

/// 启用 ROS 时间抽象覆盖。
/**
 * 此方法将启用 ROS 时间抽象覆盖值，
 * 使得时间源将报告设置的值，而不是回退到系统时间。
 *
 * 此函数与在同一时钟对象上使用的 rcl_clock_add_jump_callback() 函数和
 * rcl_clock_remove_jump_callback() 函数不是线程安全的。
 *
 * <hr>
 * 属性              | 遵循 [1]
 * ------------------ | -------------
 * 分配内存           | 否
 * 线程安全           | 否 [2]
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * <i>[1] 仅适用于函数本身，因为跳转回调可能不遵循它。</i>
 * <i>[2] 函数是可重入的，但在同一个 `clock` 对象上的并发调用是不安全的。</i>
 *
 * \param[in] clock 要启用的时钟。
 * \return #RCL_RET_OK 如果时间源成功启用，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_enable_ros_time_override(rcl_clock_t * clock);

/// 禁用 ROS 时间抽象覆盖。
/**
 * 此方法将禁用 #RCL_ROS_TIME 时间抽象覆盖值，
 * 使得时间源将报告系统时间，即使已设置自定义值。
 *
 * 此函数与在同一时钟对象上使用的 rcl_clock_add_jump_callback() 函数和
 * rcl_clock_remove_jump_callback() 函数不是线程安全的。
 *
 * <hr>
 * 属性              | 遵循 [1]
 * ------------------ | -------------
 * 分配内存           | 否
 * 线程安全           | 否 [2]
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * <i>[1] 仅适用于函数本身，因为跳转回调可能不遵循它。</i>
 * <i>[2] 函数是可重入的，但在同一个 `clock` 对象上的并发调用是不安全的。</i>
 *
 * \param[in] clock 要禁用的时钟。
 * \return #RCL_RET_OK 如果时间源成功禁用，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_disable_ros_time_override(rcl_clock_t * clock);

/// 检查 #RCL_ROS_TIME 时间源是否启用了覆盖功能。
/**
 * 此函数将填充 is_enabled 对象，以指示是否启用了时间覆盖功能。如果启用了该功能，则返回设置的值。
 * 否则，此时间源将返回与系统时间抽象相当的值。
 *
 * 与 rcl_enable_ros_time_override() 和 rcl_disable_ros_time_override() 函数在同一个时钟对象上使用时，此函数不是线程安全的。
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否 [1]
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * <i>[1] 函数是可重入的，但在同一个 `clock` 对象上的并发调用是不安全的。</i>
 *
 * \param[in] clock 要查询的时钟。
 * \param[out] is_enabled 是否启用了覆盖功能。
 * \return #RCL_RET_OK 如果时间源查询成功，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_is_enabled_ros_time_override(rcl_clock_t * clock, bool * is_enabled);

/// 为此 #RCL_ROS_TIME 时间源设置当前时间。
/**
 * 此函数将更新 #RCL_ROS_TIME 时间源的内部存储。
 * 如果查询并启用覆盖功能，时间源将返回此值，否则将返回系统时间。
 *
 * 与 rcl_clock_add_jump_callback() 和 rcl_clock_remove_jump_callback() 函数在同一个时钟对象上使用时，此函数不是线程安全的。
 *
 * <hr>
 * 属性              | 遵循性 [1]
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否 [2]
 * 使用原子操作      | 是
 * 无锁              | 是
 *
 * <i>[1] 仅适用于函数本身，因为跳转回调可能不遵循它。</i>
 * <i>[2] 函数是可重入的，但在同一个 `clock` 对象上的并发调用是不安全的。</i>
 *
 * \param[in] clock 要更新的时钟。
 * \param[in] time_value 新的当前时间。
 * \return #RCL_RET_OK 如果时间源设置成功，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_set_ros_time_override(rcl_clock_t * clock, rcl_time_point_value_t time_value);

/// 添加一个回调，当时间跳跃超过阈值时调用。
/**
 * 当阈值超过时，回调被调用两次：一次在时钟更新之前，一次在时钟更新之后。
 * user_data 指针作为最后一个参数传递给回调。
 * 在时钟中添加的回调和 user_data 对必须是唯一的。
 *
 * 与 rcl_clock_remove_jump_callback()、rcl_enable_ros_time_override()、rcl_disable_ros_time_override() 和
 * rcl_set_ros_time_override() 函数在同一个时钟对象上使用时，此函数不是线程安全的。
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否 [1]
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * <i>[1] 函数是可重入的，但在同一个 `clock` 对象上的并发调用是不安全的。线程安全性还受到与 `clock` 对象关联的 `allocator` 对象的影响。</i>
 *
 * \param[in] clock 要添加跳转回调的时钟。
 * \param[in] threshold 指示何时调用回调的条件。
 * \param[in] callback 要调用的回调。
 * \param[in] user_data 要传递给回调的指针。
 * \return #RCL_RET_OK 如果回调成功添加，或
 * \return #RCL_RET_BAD_ALLOC 如果内存分配失败，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_clock_add_jump_callback(
  rcl_clock_t * clock, rcl_jump_threshold_t threshold, rcl_jump_callback_t callback,
  void * user_data);

/// 删除先前添加的时间跳转回调。
/**
 * 与 rcl_clock_add_jump_callback()、rcl_enable_ros_time_override()、rcl_disable_ros_time_override() 和
 * rcl_set_ros_time_override() 函数在同一个时钟对象上使用时，此函数不是线程安全的。
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否 [1]
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * <i>[1] 函数是可重入的，但在同一个 `clock` 对象上的并发调用是不安全的。线程安全性还受到与 `clock` 对象关联的 `allocator` 对象的影响。</i>
 *
 * \param[in] clock 要从中删除跳转回调的时钟。
 * \param[in] callback 要调用的回调。
 * \param[in] user_data 要传递给回调的指针。
 * \return #RCL_RET_OK 如果回调成功添加，或
 * \return #RCL_RET_BAD_ALLOC 如果内存分配失败，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_ERROR 回调未找到或发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_clock_remove_jump_callback(
  rcl_clock_t * clock, rcl_jump_callback_t callback, void * user_data);

#ifdef __cplusplus
}
#endif

#endif  // RCL__TIME_H_
