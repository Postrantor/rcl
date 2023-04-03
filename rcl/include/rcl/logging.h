// Copyright 2018 Open Source Robotics Foundation, Inc.
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

#ifndef RCL__LOGGING_H_
#define RCL__LOGGING_H_

#include "rcl/allocator.h"
#include "rcl/arguments.h"
#include "rcl/macros.h"
#include "rcl/types.h"
#include "rcl/visibility_control.h"
#include "rcutils/logging.h"

#ifdef __cplusplus
extern "C" {
#endif

/// 日志消息的函数签名。
typedef rcutils_logging_output_handler_t rcl_logging_output_handler_t;

/// 配置日志系统。
/**
 * 该函数应在 ROS 初始化过程中调用。
 * 它将把启用的日志输出追加器添加到根记录器。
 *
 * <hr>
 * 属性              | 符合性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] global_args 系统的全局参数
 * \param[in] allocator 用于分配日志系统使用的内存的分配器
 * \return #RCL_RET_OK 如果成功，或者
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或者
 * \return #RCL_RET_ERROR 如果发生一般错误
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_logging_configure(
  const rcl_arguments_t * global_args, const rcl_allocator_t * allocator);

/// 使用提供的输出处理程序配置日志系统。
/**
 * 类似于 rcl_logging_configure，但它使用提供的输出处理程序。
 * \sa rcl_logging_configure
 *
 * <hr>
 * 属性              | 符合性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] global_args 系统的全局参数
 * \param[in] allocator 用于分配日志系统使用的内存的分配器
 * \param[in] output_handler 要安装的输出处理程序
 * \return #RCL_RET_OK 如果成功，或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或者
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或者
 * \return #RCL_RET_ERROR 如果发生一般错误
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_logging_configure_with_output_handler(
  const rcl_arguments_t * global_args, const rcl_allocator_t * allocator,
  rcl_logging_output_handler_t output_handler);

/**
 * 应调用此函数来拆除由 configure 函数设置的日志。
 *
 * <hr>
 * 属性              | 符合性
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \return #RCL_RET_OK 如果成功。
 * \return #RCL_RET_ERROR 如果发生一般错误
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_logging_fini(void);

/// 查看是否启用了日志记录 rosout。
/**
 * 此功能旨在用于检查是否启用了日志记录 rosout。
 *
 * <hr>
 * 属性              | 符合性
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \return `TRUE` 如果启用了日志记录 rosout，或者
 * \return `FALSE` 如果禁用了日志记录 rosout。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
bool rcl_logging_rosout_enabled(void);

/// rcl 使用的默认输出处理程序。
/**
 * 此函数可以包装在特定语言的客户端库中，
 * 在那里添加必要的互斥保护，然后使用
 * rcl_logging_configure_with_output_handler() 而不是
 * rcl_logging_configure()。
 *
 * <hr>
 * 属性              | 符合性
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] location 指向位置结构的指针或 NULL
 * \param[in] severity 严重级别
 * \param[in] name 记录器的名称，必须是以空字符结尾的 c 字符串
 * \param[in] timestamp 创建日志消息时的时间戳
 * \param[in] format 要插入格式化日志消息的参数列表
 * \param[in] args 字符串格式的参数
 */
RCL_PUBLIC
void rcl_logging_multiple_output_handler(
  const rcutils_log_location_t * location, int severity, const char * name,
  rcutils_time_point_value_t timestamp, const char * format, va_list * args);

#ifdef __cplusplus
}
#endif

#endif  // RCL__LOGGING_H_
