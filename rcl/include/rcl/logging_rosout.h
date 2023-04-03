// Copyright 2018-2019 Open Source Robotics Foundation, Inc.
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

#ifndef RCL__LOGGING_ROSOUT_H_
#define RCL__LOGGING_ROSOUT_H_

#include "rcl/allocator.h"
#include "rcl/error_handling.h"
#include "rcl/node.h"
#include "rcl/types.h"
#include "rcl/visibility_control.h"

#ifdef __cplusplus
extern "C" {
#endif

/// 默认的 /rosout 主题的 QoS 配置设置
/**
 * - depth = 1000
 * - durability = RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL
 * - lifespan = {10, 0}
 */
static const rmw_qos_profile_t rcl_qos_profile_rosout_default = {
  RMW_QOS_POLICY_HISTORY_KEEP_LAST,
  1000,
  RMW_QOS_POLICY_RELIABILITY_RELIABLE,
  RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL,
  RMW_QOS_DEADLINE_DEFAULT,
  {10, 0},
  RMW_QOS_POLICY_LIVELINESS_SYSTEM_DEFAULT,
  RMW_QOS_LIVELINESS_LEASE_DURATION_DEFAULT,
  false};

/// 初始化 rcl_logging_rosout 功能
/**
 * 调用此函数将初始化 rcl_logging_rosout 功能。在调用任何其他 rcl_logging_rosout_* 函数之前，必须先调用此函数。
 *
 * <hr>
 * 属性              | 符合性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] allocator 用于与 rcl_logging_rosout 功能相关的元数据的分配器
 * \return #RCL_RET_OK 如果 rcl_logging_rosout 功能成功初始化，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_logging_rosout_init(const rcl_allocator_t * allocator);

/// 反初始化 rcl_logging_rosout 功能
/**
 * 调用此函数将使 rcl_logging_rosout 功能处于未初始化状态，该状态在功能上与调用 rcl_logging_rosout_init 之前相同。
 *
 * <hr>
 * 属性              | 符合性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \return #RCL_RET_OK 如果 rcl_logging_rosout 功能成功反初始化，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_logging_rosout_fini();

/// 为节点创建 rosout 发布者并将其注册到日志系统中
/**
 * 对 rcl_node_t 调用此函数将在该节点上创建一个新的发布者，该发布者将由日志系统用于发布来自该节点的 logger 的所有日志消息。
 *
 * 如果此节点已存在发布者，则不会创建新的发布者。
 *
 * 预期在使用此函数创建 rosout 发布者后，将在节点仍然有效时为节点调用 rcl_logging_destroy_rosout_publisher_for_node() 来清理发布者。
 *
 *
 * <hr>
 * 属性              | 符合性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] node 将在其上创建发布者的有效 rcl_node_t
 * \return #RCL_RET_OK 如果日志发布者成功创建，或
 * \return #RCL_RET_NODE_INVALID 如果参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_logging_rosout_init_publisher_for_node(rcl_node_t * node);

/// 取消注册节点的 rosout 发布者并清理分配的资源
/**
 * 对 rcl_node_t 调用此函数将销毁该节点上的 rosout 发布者，并从日志系统中删除它，以便不再向此函数发布日志消息。
 *
 *
 * <hr>
 * 属性              | 符合性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] node 将在其上创建发布者的有效 rcl_node_t
 * \return #RCL_RET_OK 如果日志发布者成功完成，或
 * \return #RCL_RET_NODE_INVALID 如果任何参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_logging_rosout_fini_publisher_for_node(rcl_node_t * node);

/// 输出处理程序将日志消息输出到 rosout 主题。
/**
 * 当使用 logger 名称和日志消息调用时，此函数将尝试查找与 logger 名称相关的 rosout 发布者，并通过该发布者发布 Log 消息。
 * 如果没有与 logger 直接相关的发布者，则不会执行任何操作。
 *
 * 此函数旨在与 rcutils 的日志功能一起注册，不应在此上下文之外使用。
 * 另外，像 args 这样的参数应为非空且已正确初始化，否则其行为是未定义的。
 *
 * <hr>
 * 属性              | 符合性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] location 指向位置结构的指针或 NULL
 * \param[in] severity 严重级别
 * \param[in] name logger 的名称，必须是以空字符结尾的 c 字符串
 * \param[in] timestamp 创建日志消息时的时间戳
 * \param[in] format 要插入格式化日志消息的参数列表
 * \param[in] args 字符串格式的参数
 */
RCL_PUBLIC
void rcl_logging_rosout_output_handler(
  const rcutils_log_location_t * location, int severity, const char * name,
  rcutils_time_point_value_t timestamp, const char * format, va_list * args);

/// 基于 logger 添加一个从属 logger
/**
 * 调用此函数将使用节点上 `logger_name` 的现有发布者创建一个从属 logger，该 logger 将由日志系统用于发布来自该节点的 logger 的所有日志消息。
 *
 * 如果已存在从属 logger，则不会创建它。
 *
 * 预期在使用此函数创建从属 logger 后，将在 `logger_name` 的发布者仍然有效时为节点调用 rcl_logging_rosout_remove_sublogger() 来清理从属 logger。
 *
 *
 * <hr>
 * 属性              | 符合性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] logger_name 具有相应 rosout 发布者的节点上的 logger_name
 * \param[in] sublogger_name 从属 logger 名称
 * \return #RCL_RET_OK 如果从属 logger 成功创建，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_SUBLOGGER_ALREADY_EXIST 如果从属 logger 已经存在，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_logging_rosout_add_sublogger(const char * logger_name, const char * sublogger_name);

/// 删除从属 logger 并清理分配的资源
/**
 * 调用此函数将销毁基于 `logger_name+RCUTILS_LOGGING_SEPARATOR_STRING+sublogger_name` 的从属 logger，并从日志系统中删除它，以便不再向此函数发布日志消息。
 *
 *
 * <hr>
 * 属性              | 符合性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] logger_name 具有相应 rosout 发布者的节点上的 logger_name
 * \param[in] sublogger_name 从属 logger 名称
 * \return #RCL_RET_OK 如果从属 logger 成功完成，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_logging_rosout_remove_sublogger(
  const char * logger_name, const char * sublogger_name);

#ifdef __cplusplus
}
#endif

#endif  // RCL__LOGGING_ROSOUT_H_
