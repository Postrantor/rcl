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

/** \mainpage rcl: ROS生命周期的通用功能
 *
 * `rcl_lifecycle` 提供了一个纯C语言实现的ROS生命周期概念。
 * 它基于 `rcl` 中的主题和服务的实现。
 *
 * `rcl_lifecycle` 包含以下ROS生命周期实体的函数和结构：
 *
 * - 生命周期状态
 * - 生命周期转换
 * - 生命周期状态机
 * - 生命周期触发器
 *
 * 一些有用的抽象：
 *
 * - 返回代码和其他类型
 *   - rcl_lifecycle/data_types.h
 */

// 防止头文件重复包含
#ifndef RCL_LIFECYCLE__RCL_LIFECYCLE_H_
#define RCL_LIFECYCLE__RCL_LIFECYCLE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "rcl_lifecycle/data_types.h"
#include "rcl_lifecycle/default_state_machine.h"
#include "rcl_lifecycle/visibility_control.h"

/// 返回一个成员设置为`NULL`或0的rcl_lifecycle_state_t结构体。
/**
 * 在传递给 rcl_lifecycle_state_init() 之前，应调用此函数以获取一个空的 rcl_lifecycle_state_t。
 *
 * \return rcl_lifecycle_state_t 初始化后的结构体
 */
RCL_LIFECYCLE_PUBLIC
rcl_lifecycle_state_t rcl_lifecycle_get_zero_initialized_state();

/// 初始化一个 rcl_lifecycle_state_init。
/**
 * 此函数根据 `id` 和 `label` 初始化一个状态。
 *
 * 给定的 `rcl_lifecycle_state_t` 必须使用函数 `rcl_lifecycle_get_zero_initialized_state()` 进行零初始化，
 * 并且不能已经由此函数初始化。分配器将用于分配标签字符串。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[inout] state 指向要初始化的状态结构体的指针
 * \param[in] id 状态的标识符
 * \param[in] label 状态的标签
 * \param[in] allocator 用于初始化生命周期状态的有效分配器
 * \return `RCL_RET_OK` 如果状态成功初始化, 或者
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效, 或者
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_LIFECYCLE_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_lifecycle_state_init(
  rcl_lifecycle_state_t * state, uint8_t id, const char * label, const rcl_allocator_t * allocator);

/// 结束一个 rcl_lifecycle_state_t.
/**
 *
 * 调用此函数将使 rcl_lifecycle_state_t 结构进入未初始化状态，该状态在功能上与调用 rcl_lifecycle_state_init 之前相同。此函数使
 * rcl_lifecycle_state_t 无效。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[inout] state 要结束的结构
 * \param[in] allocator 用于结束生命周期状态的有效分配器
 * \return `RCL_RET_OK` 如果状态成功结束, 或者
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效。
 */
RCL_LIFECYCLE_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_lifecycle_state_fini(
  rcl_lifecycle_state_t * state, const rcl_allocator_t * allocator);

/// 返回一个成员设置为 `NULL` 或 0 的 rcl_lifecycle_transition_t 结构。
/**
 * 在传递给 rcl_lifecycle_transition_init() 之前，应调用此函数以获取空的 rcl_lifecycle_transition_t。
 */
RCL_LIFECYCLE_PUBLIC
rcl_lifecycle_transition_t rcl_lifecycle_get_zero_initialized_transition();

/// 从开始状态初始化到目标状态的过渡。
/**
 * 给定的 `rcl_lifecycle_transition_t` 必须使用函数 `rcl_lifecycle_get_zero_initialized_transition()` 进行零初始化，并且不能已经由此函数初始化。
 * 分配器将用于分配标签字符串和 rcl_lifecycle_state_t 结构。
 *
 * 注意：过渡指针将拥有开始和目标状态。当调用
 * rcl_lifecycle_transition_fini() 时，这两个状态
 * 将被释放。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] transition 指向预分配的、零初始化的过渡结构
 *    要初始化。
 * \param[in] id 过渡的标识符
 * \param[in] label 过渡的标签
 * \param[in] start 过渡初始化的值
 * \param[in] goal 过渡的目标
 * \param[in] allocator 用于结束生命周期状态的有效分配器
 * \return `RCL_RET_OK` 如果过渡成功初始化, 或者
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效, 或者
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_LIFECYCLE_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_lifecycle_transition_init(
  rcl_lifecycle_transition_t * transition, unsigned int id, const char * label,
  rcl_lifecycle_state_t * start, rcl_lifecycle_state_t * goal, const rcl_allocator_t * allocator);

/// 结束一个 rcl_lifecycle_transition_t.
/**
 * 调用此函数将使 rcl_lifecycle_transition_t 结构进入未初始化状态，该状态在功能上与调用 rcl_lifecycle_transition_init 之前相同。此函数使
 * rcl_lifecycle_transition_t 无效。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[inout] transition 要完成的结构
 * \param[in] allocator 用于完成过渡的有效分配器
 * \return `RCL_RET_OK` 如果状态成功完成, 或者
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效, 或者
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_LIFECYCLE_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_lifecycle_transition_fini(
  rcl_lifecycle_transition_t * transition, const rcl_allocator_t * allocator);

/// 返回默认初始化的状态机选项结构。
RCL_LIFECYCLE_PUBLIC
rcl_lifecycle_state_machine_options_t rcl_lifecycle_get_default_state_machine_options();

/// 返回一个成员设置为 `NULL` 或 0 的 rcl_lifecycle_state_machine_t 结构。
/**
 * 在传递给 rcl_lifecycle_state_machine_init() 之前，应该调用它以获取空的 rcl_lifecycle_state_machine_t。
 */
RCL_LIFECYCLE_PUBLIC
rcl_lifecycle_state_machine_t rcl_lifecycle_get_zero_initialized_state_machine();

/// 初始化状态机
/**
 * 此函数初始化状态机：一个发布器用于发布过渡消息和一组服务以获取有关状态和过渡的信息。
 * 如果 `default_state` 为 `true`，则初始化一个新的默认状态机。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[inout] state_machine 要初始化的结构
 * \param[in] node_handle 有效的（未完成）节点句柄，用于创建发布器和服务
 * \param[in] ts_pub_notify 过渡发布器指针，用于发布过渡
 * \param[in] ts_srv_change_state 允许触发状态更改的服务指针
 * \param[in] ts_srv_get_state 允许获取当前状态的服务指针
 * \param[in] ts_srv_get_available_states 允许获取可用状态的服务指针
 * \param[in] ts_srv_get_available_transitions 允许获取可用过渡的服务指针
 * \param[in] ts_srv_get_transition_graph 允许从图中获取过渡的服务指针
 * \param[in] state_machine_options 初始化状态机的配置选项集合
 * \return `RCL_RET_OK` 如果状态机成功初始化, 或者
 * \return `RCL_RET_INVALID_ARGUMENT` 如果输入参数为 NULL, 或者
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_LIFECYCLE_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_lifecycle_state_machine_init(
  rcl_lifecycle_state_machine_t * state_machine, rcl_node_t * node_handle,
  const rosidl_message_type_support_t * ts_pub_notify,
  const rosidl_service_type_support_t * ts_srv_change_state,
  const rosidl_service_type_support_t * ts_srv_get_state,
  const rosidl_service_type_support_t * ts_srv_get_available_states,
  const rosidl_service_type_support_t * ts_srv_get_available_transitions,
  const rosidl_service_type_support_t * ts_srv_get_transition_graph,
  const rcl_lifecycle_state_machine_options_t * state_machine_options);

/// 结束一个 rcl_lifecycle_state_machine_t.
/**
 * 调用此函数将使 rcl_lifecycle_state_machine_t 结构进入未初始化状态，该状态在功能上与调用 rcl_lifecycle_state_machine_init 之前相同。此函数使
 * rcl_lifecycle_state_machine_t 无效。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[inout] state_machine 要完成的结构
 * \param[in] node_handle 有效的（未完成）节点句柄
 * \return `RCL_RET_OK` 如果状态成功完成, 或者
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效, 或者
 * \return `RCL_RET_ERROR` 如果发生未指定的错误。
 */
RCL_LIFECYCLE_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_lifecycle_state_machine_fini(
  rcl_lifecycle_state_machine_t * state_machine, rcl_node_t * node_handle);

/// 检查状态机是否处于活动状态。
/**
 * 如果状态已初始化，则返回 `RCL_RET_OK`，否则返回 `RCL_RET_ERROR`
 * 在要返回 `RCL_RET_ERROR` 的情况下，设置错误消息。
 * 此功能不能失败。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] state_machine 指向状态机结构的指针
 * \return `RCL_RET_OK` 如果状态已初始化, 或者
 * \return `RCL_RET_INVALID_ARGUMENT` 如果任何参数无效。
 */
RCL_LIFECYCLE_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_lifecycle_state_machine_is_initialized(
  const rcl_lifecycle_state_machine_t * state_machine);

/// 通过id获取状态。
/**
 * 根据`id`返回指向内部转换结构的指针。
 * 如果状态中未设置`id`，则返回NULL。
 *
 * <hr>
 * 属性                | 遵循性
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] state 指向状态结构的指针
 * \param[in] id 要在有效转换中查找的标识符
 * \return 如果存在`id`，则返回指向生命周期转换的指针，否则返回NULL
 */
RCL_LIFECYCLE_PUBLIC
RCL_WARN_UNUSED
const rcl_lifecycle_transition_t * rcl_lifecycle_get_transition_by_id(
  const rcl_lifecycle_state_t * state, uint8_t id);

/// 通过id获取状态。
/**
 * 根据`label`返回指向内部转换结构的指针。
 * 如果状态中未设置`label`，则返回NULL。
 *
 * <hr>
 * 属性                | 遵循性
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] state 指向状态结构的指针
 * \param[in] label 要在有效转换中查找的标签
 * \return 如果存在标签，则返回指向生命周期转换的指针，否则返回NULL
 */
RCL_LIFECYCLE_PUBLIC
RCL_WARN_UNUSED
const rcl_lifecycle_transition_t * rcl_lifecycle_get_transition_by_label(
  const rcl_lifecycle_state_t * state, const char * label);

/// 通过id触发状态。
/**
 * 此函数将根据`id`触发转换。如果参数
 * `publish_notification`为`true`，则将在
 * ROS 2网络中发布通知转换的消息，如果为`false`，则不会发布消息。
 *
 * <hr>
 * 属性                | 遵循性
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] state_machine 指向状态机结构的指针
 * \param[in] id 要触发的转换的标识符
 * \param[in] publish_notification 如果值为`true`，将发布通知转换的消息，
 *    否则不会发布消息
 * \return 如果成功触发转换，则返回`RCL_RET_OK`，或者
 * \return 如果任何参数无效，则返回`RCL_RET_INVALID_ARGUMENT`，或者
 * \return 如果发生未指定的错误，则返回`RCL_RET_ERROR`。
 */
RCL_LIFECYCLE_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_lifecycle_trigger_transition_by_id(
  rcl_lifecycle_state_machine_t * state_machine, uint8_t id, bool publish_notification);

/// 通过标签触发状态。
/**
 * 此函数将根据`label`触发转换。如果参数
 * `publish_notification`为`true`，则将在
 * ROS 2网络中发布通知转换的消息，如果为`false`，则不会发布消息。
 *
 * <hr>
 * 属性                | 遵循性
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] state_machine 指向状态机结构的指针
 * \param[in] label 要触发的转换的标签
 * \param[in] publish_notification 如果值为`true`，将发布通知转换的消息，
 *    否则不会发布消息
 * \return 如果成功触发转换，则返回`RCL_RET_OK`，或者
 * \return 如果任何参数无效，则返回`RCL_RET_INVALID_ARGUMENT`，或者
 * \return 如果发生未指定的错误，则返回`RCL_RET_ERROR`。
 */
RCL_LIFECYCLE_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_lifecycle_trigger_transition_by_label(
  rcl_lifecycle_state_machine_t * state_machine, const char * label, bool publish_notification);

/// 打印状态机数据
/**
 * 此函数将在标准输出中打印
 * rcl_lifecycle_state_machine_t结构中的数据。
 *
 * <hr>
 * 属性                | 遵循性
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] state_machine 要打印的状态机结构的指针
 */
RCL_LIFECYCLE_PUBLIC
void rcl_print_state_machine(const rcl_lifecycle_state_machine_t * state_machine);

#ifdef __cplusplus
}
#endif  // extern "C"

#endif  // RCL_LIFECYCLE__RCL_LIFECYCLE_H_
