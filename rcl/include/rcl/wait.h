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

#ifndef RCL__WAIT_H_
#define RCL__WAIT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "rcl/client.h"
#include "rcl/event.h"
#include "rcl/guard_condition.h"
#include "rcl/macros.h"
#include "rcl/service.h"
#include "rcl/subscription.h"
#include "rcl/timer.h"
#include "rcl/types.h"
#include "rcl/visibility_control.h"

/// \file rcl_wait_set.h

typedef struct rcl_wait_set_impl_s rcl_wait_set_impl_t;

/// 用于等待订阅、保护条件等的容器。
typedef struct rcl_wait_set_s
{
  /// 订阅指针的存储。
  const rcl_subscription_t ** subscriptions;
  /// 订阅数量
  size_t size_of_subscriptions;
  /// 保护条件指针的存储。
  const rcl_guard_condition_t ** guard_conditions;
  /// 保护条件数量
  size_t size_of_guard_conditions;
  /// 定时器指针的存储。
  const rcl_timer_t ** timers;
  /// 定时器数量
  size_t size_of_timers;
  /// 客户端指针的存储。
  const rcl_client_t ** clients;
  /// 客户端数量
  size_t size_of_clients;
  /// 服务指针的存储。
  const rcl_service_t ** services;
  /// 服务数量
  size_t size_of_services;
  /// 事件指针的存储。
  const rcl_event_t ** events;
  /// 事件数量
  size_t size_of_events;
  /// 特定于实现的存储。
  rcl_wait_set_impl_t * impl;
} rcl_wait_set_t;

/// 返回一个成员设置为`NULL`的rcl_wait_set_t结构体。
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_wait_set_t rcl_get_zero_initialized_wait_set(void);

/// 使用可等待项目的空间初始化rcl wait set。
/**
 * 此函数为wait set中可以存储的订阅和其他可等待实体分配空间。
 * 它还将分配器设置为给定的分配器，并将修剪成员初始化为false。
 *
 * wait_set结构体应该被分配并初始化为`NULL`。
 * 如果wait_set已分配但内存未初始化，则行为未定义。
 * 在已经初始化的wait set上调用此函数将导致错误。
 * 如果在wait set上调用了rcl_wait_set_fini()，则可以重新初始化wait set。
 *
 * 要使用默认分配器，请使用rcl_get_default_allocator()。
 *
 * 预期用法：
 *
 * ```c
 * #include <rcl/wait.h>
 *
 * rcl_wait_set_t wait_set = rcl_get_zero_initialized_wait_set();
 * rcl_ret_t ret =
 *   rcl_wait_set_init(&wait_set, 42, 42, 42, 42, 42, &context, rcl_get_default_allocator());
 * // ... 错误处理，然后使用它，然后调用匹配的fini：
 * ret = rcl_wait_set_fini(&wait_set);
 * // ... 错误处理
 * ```
 *
 * <hr>
 * 属性          | 符合性
 * ------------------ | -------------
 * 分配内存   | 是
 * 线程安全        | 否
 * 使用原子       | 否
 * 无锁          | 是
 *
 * \param[inout] wait_set 要初始化的wait set结构体
 * \param[in] number_of_subscriptions 订阅集的非零大小
 * \param[in] number_of_guard_conditions 保护条件集的非零大小
 * \param[in] number_of_timers 定时器集的非零大小
 * \param[in] number_of_clients 客户端集的非零大小
 * \param[in] number_of_services 服务集的非零大小
 * \param[in] number_of_events 事件集的非零大小
 * \param[in] context wait set应与之关联的上下文
 * \param[in] allocator 在集合中分配空间时使用的分配器
 * \return #RCL_RET_OK 如果wait set成功初始化，或
 * \return #RCL_RET_ALREADY_INIT 如果wait set未初始化为零，或
 * \return #RCL_RET_NOT_INIT 如果给定的上下文无效，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_WAIT_SET_INVALID 如果wait set没有正确销毁，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_wait_set_init(
  rcl_wait_set_t * wait_set, size_t number_of_subscriptions, size_t number_of_guard_conditions,
  size_t number_of_timers, size_t number_of_clients, size_t number_of_services,
  size_t number_of_events, rcl_context_t * context, rcl_allocator_t allocator);

/// 结束一个 rcl 等待集。
/**
 * 使用初始化时给定的分配器释放在
 * rcl_wait_set_init() 中分配的等待集中的任何内存。
 *
 * 在零初始化的等待集上调用此函数将不执行任何操作并返回 RCL_RET_OK。
 * 在未初始化的内存上调用此函数会导致未定义的行为。
 * 调用此函数后，等待集将再次被零初始化，
 * 因此在此之后立即调用此函数或 rcl_wait_set_init() 将成功。
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[inout] wait_set 要结束的等待集结构体。
 * \return #RCL_RET_OK 如果结束成功，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_WAIT_SET_INVALID 如果等待集没有正确销毁，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_wait_set_fini(rcl_wait_set_t * wait_set);

/// 获取等待集的分配器。
/**
 * 分配器必须是已分配的 rcl_allocator_t 结构体，因为结果将复制到此变量中。
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] wait_set 等待集的句柄
 * \param[out] allocator 将结果复制到的 rcl_allocator_t 结构体
 * \return #RCL_RET_OK 如果成功获取分配器，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_WAIT_SET_INVALID 如果等待集无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_wait_set_get_allocator(const rcl_wait_set_t * wait_set, rcl_allocator_t * allocator);

/// 在集合中的下一个空位置存储指向给定订阅的指针。
/**
 * 此函数不保证订阅不已经在等待集中。
 *
 * 还将 rmw 表示添加到底层 rmw 数组并递增 rmw 数组计数。
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[inout] wait_set 要存储订阅的结构体
 * \param[in] subscription 要添加到等待集的订阅
 * \param[out] index 存储容器中添加的订阅的索引。
 *   此参数是可选的，可以设置为 `NULL` 以忽略。
 * \return #RCL_RET_OK 如果成功添加，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_WAIT_SET_INVALID 如果等待集为零初始化，或
 * \return #RCL_RET_WAIT_SET_FULL 如果订阅集已满，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_wait_set_add_subscription(
  rcl_wait_set_t * wait_set, const rcl_subscription_t * subscription, size_t * index);

/// 删除（设置为 `NULL`）等待集中的所有实体。
/**
 * 在使用 rcl_wait 之后但在向集合添加新实体之前应使用此函数。
 * 将底层 rmw 数组中的所有条目设置为 `NULL`，并将 rmw 数组的计数设置为 `0`。
 *
 * 在未初始化（零初始化）的等待集上调用此操作将失败。
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[inout] wait_set 要清除其实体的结构体
 * \return #RCL_RET_OK 如果成功清除，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_WAIT_SET_INVALID 如果等待集为零初始化，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_wait_set_clear(rcl_wait_set_t * wait_set);

/// 重新分配等待集中实体的空间。
/**
 * 此函数将为所有实体集释放和重新分配内存。
 *
 * 大小为0将只释放内存并将`NULL`分配给数组。
 *
 * 使用在等待集初始化期间给定的分配器进行分配和释放。
 *
 * 调用此函数后，集合中的所有值都将设置为`NULL`，
 * 实际上与调用rcl_wait_set_clear()相同。
 * 类似地，底层rmw表示也会重新分配和重置：
 * 所有条目都设置为`NULL`，计数设置为零。
 *
 * 如果请求的大小与当前大小匹配，则不会进行分配。
 *
 * 可以在未初始化（零初始化）的等待集上调用此函数。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[inout] wait_set 要调整大小的结构
 * \param[in] subscriptions_size 新订阅集的大小
 * \param[in] guard_conditions_size 新保护条件集的大小
 * \param[in] timers_size 新计时器集的大小
 * \param[in] clients_size 新客户端集的大小
 * \param[in] services_size 新服务集的大小
 * \param[in] events_size 新事件集的大小
 * \return #RCL_RET_OK 如果成功调整大小，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_wait_set_resize(
  rcl_wait_set_t * wait_set, size_t subscriptions_size, size_t guard_conditions_size,
  size_t timers_size, size_t clients_size, size_t services_size, size_t events_size);

/// 将保护条件的指针存储在集合中的下一个空位置。
/**
 * 此函数的行为与订阅完全相同。
 * \see rcl_wait_set_add_subscription
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_wait_set_add_guard_condition(
  rcl_wait_set_t * wait_set, const rcl_guard_condition_t * guard_condition, size_t * index);

/// 将计时器的指针存储在集合中的下一个空位置。
/**
 * 此函数的行为与订阅完全相同。
 * \see rcl_wait_set_add_subscription
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_wait_set_add_timer(
  rcl_wait_set_t * wait_set, const rcl_timer_t * timer, size_t * index);

/// 将客户端的指针存储在集合中的下一个空位置。
/**
 * 此函数的行为与订阅完全相同。
 * \see rcl_wait_set_add_subscription
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_wait_set_add_client(
  rcl_wait_set_t * wait_set, const rcl_client_t * client, size_t * index);

/// 将服务的指针存储在集合中的下一个空位置。
/**
 * 此函数的行为与订阅完全相同。
 * \see rcl_wait_set_add_subscription
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_wait_set_add_service(
  rcl_wait_set_t * wait_set, const rcl_service_t * service, size_t * index);

/// 将事件的指针存储在集合中的下一个空位置。
/**
 * 此函数的行为与订阅完全相同。
 * \see rcl_wait_set_add_subscription
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_wait_set_add_event(
  rcl_wait_set_t * wait_set, const rcl_event_t * event, size_t * index);

/// 阻塞，直到等待集准备好或超时。
/**
 * 此函数将收集 rcl_wait_set_t 中的项目，并将它们传递给底层的 rmw_wait 函数。
 *
 * 此函数返回后，等待集中的项目将保持不变或设置为 `NULL`。
 * 不是 `NULL` 的项目已准备好，其中根据项目类型，准备好意味着不同的事情。
 * 对于订阅，这意味着可能有可以获取的消息，或者订阅的状态已更改，在这种情况下，
 * rcl_take 可能会成功，但返回 taken == false。
 * 对于 guard conditions，这意味着触发了 guard condition。
 *
 * 预期用法：
 *
 * ```c
 * #include <rcl/rcl.h>
 *
 * // 在此之前成功调用 rcl_init()
 * rcl_node_t node;  // 初始化此节点，请参阅 rcl_node_init()
 * rcl_subscription_t sub1;  // 初始化此订阅，请参阅 rcl_subscription_init()
 * rcl_subscription_t sub2;  // 初始化此订阅，请参阅 rcl_subscription_init()
 * rcl_guard_condition_t gc1;  // 初始化此 guard condition，请参阅 rcl_guard_condition_init()
 * rcl_wait_set_t wait_set = rcl_get_zero_initialized_wait_set();
 * rcl_ret_t ret = rcl_wait_set_init(&wait_set, 2, 1, 0, 0, 0, rcl_get_default_allocator());
 * // ... 错误处理
 * do {
 *   ret = rcl_wait_set_clear(&wait_set);
 *   // ... 错误处理
 *   ret = rcl_wait_set_add_subscription(&wait_set, &sub1);
 *   // ... 错误处理
 *   ret = rcl_wait_set_add_subscription(&wait_set, &sub2);
 *   // ... 错误处理
 *   ret = rcl_wait_set_add_guard_condition(&wait_set, &gc1);
 *   // ... 错误处理
 *   ret = rcl_wait(&wait_set, RCL_MS_TO_NS(1000));  // 1000ms == 1s, 以纳秒为单位传递
 *   if (ret == RCL_RET_TIMEOUT) {
 *     continue;
 *   }
 *   for (int i = 0; i < wait_set.size_of_subscriptions; ++i) {
 *     if (wait_set.subscriptions[i]) {
 *       // 订阅已准备好...
 *     }
 *   }
 *   for (int i = 0; i < wait_set.size_of_guard_conditions; ++i) {
 *     if (wait_set.guard_conditions[i]) {
 *       // 订阅已准备好...
 *     }
 *   }
 * } while(check_some_condition());
 * // ... 结束节点、订阅和 guard conditions...
 * ret = rcl_wait_set_fini(&wait_set);
 * // ... 错误处理
 * ```
 *
 * 等待集结构必须分配、初始化，并且应该已经清除并填充了项目，例如订阅和 guard conditions。
 * 传递一个没有等待项的等待集将失败。
 * 集合中的 `NULL` 项目将被忽略，例如，输入有效：
 *  - `subscriptions[0]` = 有效指针
 *  - `subscriptions[1]` = `NULL`
 *  - `subscriptions[2]` = 有效指针
 *  - `size_of_subscriptions` = 3
 * 传递未初始化（零初始化）的等待集结构将失败。
 * 传递具有未初始化内存的等待集结构是未定义的行为。
 *
 * 超时的单位是纳秒。
 * 如果超时为负，则此函数将无限期阻塞，直到等待集中的某个内容有效或被中断。
 * 如果超时为 0，则此函数将非阻塞；检查现在已准备好的内容，但如果还没有准备好则不等待。
 * 如果超时大于 0，则此函数将在经过该时间段后返回，或者等待集变为准备好，以先到者为准。
 * 传递具有未初始化内存的超时结构是未定义的行为。
 *
 * 对于唯一等待集和唯一内容，此函数是线程安全的。
 * 此函数不能在多个线程上操作相同的等待集，等待集也不能共享内容。
 * 例如，在两个线程上调用 rcl_wait() 在两个不同的等待集上，这两个等待集都包含一个共享的 guard condition 是未定义的行为。
 *
 * \param[inout] wait_set 要等待的事物集合，如果没有准备好，则进行修剪
 * \param[in] timeout 等待等待集准备好的持续时间，以纳秒为单位
 * \return #RCL_RET_OK 等待集中的某个内容已准备好，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_WAIT_SET_INVALID 如果等待集为零初始化，或
 * \return #RCL_RET_WAIT_SET_EMPTY 如果等待集不包含项目，或
 * \return #RCL_RET_TIMEOUT 如果超时在某个内容准备好之前过期，或
 * \return #RCL_RET_ERROR 发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_wait(rcl_wait_set_t * wait_set, int64_t timeout);

/// 如果等待集有效，则返回 `true`，否则返回 `false`。
/**
 * 等待集无效，如果：
 *   - 实现是 `NULL`（未调用 rcl_wait_set_init 或失败）
 *   - 使用 rcl_wait_set_fini 对等待集进行了最终处理
 *
 * 如果等待集指针为 `NULL`，也返回 `false`。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存           | 否
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * \param[in] wait_set 要验证的 rcl_wait_set_t
 * \return 如果 wait_set 有效，则为 `true`，否则为 `false`。
 */
RCL_PUBLIC
bool rcl_wait_set_is_valid(const rcl_wait_set_t * wait_set);

#ifdef __cplusplus
}
#endif

#endif  // RCL__WAIT_H_
