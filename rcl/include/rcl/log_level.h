// Copyright 2020 Open Source Robotics Foundation, Inc.
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

#ifndef RCL__LOG_LEVEL_H_
#define RCL__LOG_LEVEL_H_

#include "rcl/allocator.h"
#include "rcl/macros.h"
#include "rcl/types.h"
#include "rcl/visibility_control.h"

#ifdef __cplusplus
extern "C" {
#endif

/// 定义 RCUTILS_LOG_SEVERITY 的类型别名；
typedef enum RCUTILS_LOG_SEVERITY rcl_log_severity_t;

/// 用于指定名称和日志级别的 logger 项。
typedef struct rcl_logger_setting_s {
  /// logger 的名称。
  const char* name;
  /// logger 的最低日志级别严重性。
  rcl_log_severity_t level;
} rcl_logger_setting_t;

/// 保存默认 logger 级别和其他 logger 设置。
typedef struct rcl_log_levels_s {
  /// 最低默认 logger 级别严重性。
  rcl_log_severity_t default_logger_level;
  /// logger 设置数组。
  rcl_logger_setting_t* logger_settings;
  /// logger 设置数量。
  size_t num_logger_settings;
  /// logger 设置容量。
  size_t capacity_logger_settings;
  /// 用于在此结构中分配对象的分配器。
  rcl_allocator_t allocator;
} rcl_log_levels_t;

/// 返回一个成员初始化为零值的 rcl_log_levels_t 结构体。
/**
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | Yes
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \return 成员初始化为零值的 rcl_log_levels_t 结构体。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_log_levels_t rcl_get_zero_initialized_log_levels();

/// 初始化日志级别结构体。
/**
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param[in] log_levels 要初始化的结构体。
 * \param[in] allocator 用于分配并分配到 log_levels 的内存分配器。
 * \param[in] logger_count 要分配的 logger 设置数量。
 *  此操作为 logger_settings 预留内存，但不进行初始化。
 * \return 如果结构成功初始化，则返回 #RCL_RET_OK，或
 * \return 如果 log_levels 为 NULL，则返回 #RCL_RET_INVALID_ARGUMENT，或
 * \return 如果 log_levels 包含已初始化的内存，则返回 #RCL_RET_INVALID_ARGUMENT，或
 * \return 如果分配器无效，则返回 #RCL_RET_INVALID_ARGUMENT，或
 * \return 如果分配内存失败，则返回 #RCL_RET_BAD_ALLOC。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_log_levels_init(
    rcl_log_levels_t* log_levels, const rcl_allocator_t* allocator, size_t logger_count);

/// 将一个日志级别结构体复制到另一个。
/**
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param[in] src 要复制的结构体。
 *  其分配器用于将内存复制到新结构中。
 * \param[out] dst 要复制到的日志级别结构体。
 * \return 如果结构成功复制，则返回 #RCL_RET_OK，或
 * \return 如果 src 为 NULL，则返回 #RCL_RET_INVALID_ARGUMENT，或
 * \return 如果 src 分配器无效，则返回 #RCL_RET_INVALID_ARGUMENT，或
 * \return 如果 dst 为 NULL，则返回 #RCL_RET_INVALID_ARGUMENT，或
 * \return 如果 dst 包含已分配的内存，则返回 #RCL_RET_INVALID_ARGUMENT，或
 * \return 如果分配内存失败，则返回 #RCL_RET_BAD_ALLOC。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_log_levels_copy(const rcl_log_levels_t* src, rcl_log_levels_t* dst);

/// 回收 rcl_log_levels_t 结构体内部持有的资源。
/**
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param[in] log_levels 要释放其资源的结构体。
 * \return 如果内存成功释放，则返回 #RCL_RET_OK，或
 * \return 如果 log_levels 为 NULL，则返回 #RCL_RET_INVALID_ARGUMENT，或
 * \return 如果 log_levels 分配器无效且结构包含已初始化的内存，则返回 #RCL_RET_INVALID_ARGUMENT。
 */
RCL_PUBLIC
rcl_ret_t rcl_log_levels_fini(rcl_log_levels_t* log_levels);

/// 缩小日志级别结构体。
/**
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param[in] log_levels 要缩小的结构体。
 * \return 如果内存成功缩小，则返回 #RCL_RET_OK，或
 * \return 如果 log_levels 为 NULL 或其分配器无效，则返回 #RCL_RET_INVALID_ARGUMENT，或
 * \return 如果重新分配内存失败，则返回 #RCL_RET_BAD_ALLOC。
 */
RCL_PUBLIC
rcl_ret_t rcl_log_levels_shrink_to_size(rcl_log_levels_t* log_levels);

/// 添加具有名称和级别的 logger 设置。
/**
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param[in] log_levels 要设置 logger 日志级别的结构体。
 * \param[in] logger_name logger 的名称，将在结构中存储其副本。
 * \param[in] log_level 要为 logger_name 设置的最低日志级别严重性。
 * \return 如果成功添加 logger 设置，则返回 #RCL_RET_OK，或
 * \return 如果分配内存失败，则返回 #RCL_RET_BAD_ALLOC，或
 * \return 如果 log_levels 为 NULL，则返回 #RCL_RET_INVALID_ARGUMENT，或
 * \return 如果 log_levels 未初始化，则返回 #RCL_RET_INVALID_ARGUMENT，或
 * \return 如果 log_levels 分配器无效，则返回 #RCL_RET_INVALID_ARGUMENT，或
 * \return 如果 logger_name 为 NULL，则返回 #RCL_RET_INVALID_ARGUMENT，或
 * \return 如果 log_levels 结构已满，则返回 #RCL_RET_ERROR。
 */
RCL_PUBLIC
rcl_ret_t rcl_log_levels_add_logger_setting(
    rcl_log_levels_t* log_levels, const char* logger_name, rcl_log_severity_t log_level);

#ifdef __cplusplus
}
#endif

#endif  // RCL__LOG_LEVEL_H_
