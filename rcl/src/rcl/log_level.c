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

#include "rcl/log_level.h"

#include "rcl/error_handling.h"
#include "rcutils/allocator.h"
#include "rcutils/logging_macros.h"
#include "rcutils/strdup.h"

/**
 * @brief 获取一个零初始化的日志级别结构体
 *
 * @return 返回一个零初始化的 rcl_log_levels_t 结构体
 */
rcl_log_levels_t rcl_get_zero_initialized_log_levels() {
  // 定义并初始化一个 rcl_log_levels_t 结构体变量 log_levels
  const rcl_log_levels_t log_levels = {
      .default_logger_level = RCUTILS_LOG_SEVERITY_UNSET,
      .logger_settings = NULL,
      .num_logger_settings = 0,
      .capacity_logger_settings = 0,
      .allocator = {NULL, NULL, NULL, NULL, NULL},
  };
  // 返回初始化后的 log_levels 结构体
  return log_levels;
}

/**
 * @brief 初始化日志级别结构体
 *
 * @param[in,out] log_levels 指向要初始化的 rcl_log_levels_t 结构体指针
 * @param[in] allocator 分配器，用于分配内存
 * @param[in] logger_count 日志记录器数量
 * @return 返回操作结果，成功返回 RCL_RET_OK，否则返回相应的错误代码
 */
rcl_ret_t rcl_log_levels_init(
    rcl_log_levels_t *log_levels, const rcl_allocator_t *allocator, size_t logger_count) {
  // 检查 log_levels 参数是否为空，如果为空则返回 RCL_RET_INVALID_ARGUMENT 错误
  RCL_CHECK_ARGUMENT_FOR_NULL(log_levels, RCL_RET_INVALID_ARGUMENT);
  // 检查分配器是否有效，如果无效则返回 RCL_RET_INVALID_ARGUMENT 错误
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  // 检查 log_levels->logger_settings 是否为空，如果不为空则返回 RCL_RET_INVALID_ARGUMENT 错误
  if (log_levels->logger_settings != NULL) {
    RCL_SET_ERROR_MSG("invalid logger settings");
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 初始化 log_levels 结构体的各个成员变量
  log_levels->default_logger_level = RCUTILS_LOG_SEVERITY_UNSET;
  log_levels->logger_settings = NULL;
  log_levels->num_logger_settings = 0;
  log_levels->capacity_logger_settings = logger_count;
  log_levels->allocator = *allocator;

  // 如果 logger_count 大于 0，则为 log_levels->logger_settings 分配内存
  if (logger_count > 0) {
    log_levels->logger_settings =
        allocator->allocate(sizeof(rcl_logger_setting_t) * logger_count, allocator->state);
    // 如果分配内存失败，则返回 RCL_RET_BAD_ALLOC 错误
    if (NULL == log_levels->logger_settings) {
      RCL_SET_ERROR_MSG("Error allocating memory");
      return RCL_RET_BAD_ALLOC;
    }
  }
  // 返回操作成功的结果代码 RCL_RET_OK
  return RCL_RET_OK;
}

/**
 * @brief 复制源日志级别结构到目标日志级别结构
 *
 * @param[in] src 指向源 rcl_log_levels_t 结构的指针
 * @param[out] dst 指向目标 rcl_log_levels_t 结构的指针
 * @return 返回 rcl_ret_t 类型的结果，成功返回 RCL_RET_OK，否则返回相应的错误代码
 */
rcl_ret_t rcl_log_levels_copy(const rcl_log_levels_t *src, rcl_log_levels_t *dst) {
  // 检查输入参数 src 是否为空，为空则返回无效参数错误
  RCL_CHECK_ARGUMENT_FOR_NULL(src, RCL_RET_INVALID_ARGUMENT);
  // 检查输入参数 dst 是否为空，为空则返回无效参数错误
  RCL_CHECK_ARGUMENT_FOR_NULL(dst, RCL_RET_INVALID_ARGUMENT);
  // 获取分配器
  const rcl_allocator_t *allocator = &src->allocator;
  // 检查分配器是否有效，无效则返回无效参数错误
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  // 检查目标结构中的 logger_settings 是否为空，不为空则返回无效参数错误
  if (dst->logger_settings != NULL) {
    RCL_SET_ERROR_MSG("invalid logger settings");
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 使用分配器为目标结构的 logger_settings 分配内存
  dst->logger_settings = allocator->allocate(
      sizeof(rcl_logger_setting_t) * (src->num_logger_settings), allocator->state);
  // 检查分配的内存是否有效，无效则返回内存分配错误
  if (NULL == dst->logger_settings) {
    RCL_SET_ERROR_MSG("Error allocating memory");
    return RCL_RET_BAD_ALLOC;
  }

  // 复制源结构的默认日志级别到目标结构
  dst->default_logger_level = src->default_logger_level;
  // 复制源结构的 logger_settings 容量到目标结构
  dst->capacity_logger_settings = src->capacity_logger_settings;
  // 复制源结构的分配器到目标结构
  dst->allocator = src->allocator;
  // 遍历源结构的 logger_settings，并复制到目标结构
  for (size_t i = 0; i < src->num_logger_settings; ++i) {
    // 使用分配器为目标结构的 logger_settings[i].name 分配内存并复制内容
    dst->logger_settings[i].name = rcutils_strdup(src->logger_settings[i].name, *allocator);
    // 检查分配的内存是否有效，无效则进行清理并返回内存分配错误
    if (NULL == dst->logger_settings[i].name) {
      dst->num_logger_settings = i;
      if (RCL_RET_OK != rcl_log_levels_fini(dst)) {
        RCL_SET_ERROR_MSG("Error while finalizing log levels due to another error");
      }
      return RCL_RET_BAD_ALLOC;
    }
    // 复制源结构的 logger_settings[i].level 到目标结构
    dst->logger_settings[i].level = src->logger_settings[i].level;
  }
  // 复制源结构的 num_logger_settings 到目标结构
  dst->num_logger_settings = src->num_logger_settings;
  // 返回成功
  return RCL_RET_OK;
}

/**
 * @brief 释放日志级别结构体的内存
 *
 * @param[in,out] log_levels 指向要释放的日志级别结构体的指针
 * @return rcl_ret_t 返回操作结果，成功返回RCL_RET_OK，否则返回相应的错误代码
 */
rcl_ret_t rcl_log_levels_fini(rcl_log_levels_t *log_levels) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(log_levels, RCL_RET_INVALID_ARGUMENT);

  // 获取分配器
  const rcl_allocator_t *allocator = &log_levels->allocator;

  if (log_levels->logger_settings) {
    // 检查分配器，确保可以安全地完成零初始化的rcl_log_levels_t
    RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);

    // 遍历并释放logger_settings中的名称内存
    for (size_t i = 0; i < log_levels->num_logger_settings; ++i) {
      allocator->deallocate((void *)log_levels->logger_settings[i].name, allocator->state);
    }

    // 将num_logger_settings设置为0
    log_levels->num_logger_settings = 0;

    // 释放logger_settings内存
    allocator->deallocate(log_levels->logger_settings, allocator->state);
    log_levels->logger_settings = NULL;
  }

  return RCL_RET_OK;
}

/**
 * @brief 调整日志级别结构体的大小以适应当前使用的数量
 *
 * @param[in,out] log_levels 指向要调整大小的日志级别结构体的指针
 * @return rcl_ret_t 返回操作结果，成功返回RCL_RET_OK，否则返回相应的错误代码
 */
rcl_ret_t rcl_log_levels_shrink_to_size(rcl_log_levels_t *log_levels) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(log_levels, RCL_RET_INVALID_ARGUMENT);

  // 获取分配器
  rcl_allocator_t *allocator = &log_levels->allocator;

  // 检查分配器
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);

  if (0U == log_levels->num_logger_settings) {
    // 如果没有logger_settings，则释放内存并将容量设置为0
    allocator->deallocate(log_levels->logger_settings, allocator->state);
    log_levels->logger_settings = NULL;
    log_levels->capacity_logger_settings = 0;
  } else if (log_levels->num_logger_settings < log_levels->capacity_logger_settings) {
    // 如果当前使用的logger_settings数量小于容量，则重新分配内存
    rcl_logger_setting_t *new_logger_settings = allocator->reallocate(
        log_levels->logger_settings, sizeof(rcl_logger_setting_t) * log_levels->num_logger_settings,
        allocator->state);

    // 检查重新分配是否成功
    if (NULL == new_logger_settings) {
      return RCL_RET_BAD_ALLOC;
    }

    // 更新logger_settings和容量
    log_levels->logger_settings = new_logger_settings;
    log_levels->capacity_logger_settings = log_levels->num_logger_settings;
  }

  return RCL_RET_OK;
}

/**
 * @brief 添加日志记录器设置
 *
 * @param[in] log_levels 日志级别结构体指针
 * @param[in] logger_name 日志记录器名称
 * @param[in] log_level 日志严重性级别
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_log_levels_add_logger_setting(
    rcl_log_levels_t *log_levels, const char *logger_name, rcl_log_severity_t log_level) {
  // 检查参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(log_levels, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(log_levels->logger_settings, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(logger_name, RCL_RET_INVALID_ARGUMENT);

  // 获取分配器
  rcl_allocator_t *allocator = &log_levels->allocator;
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);

  // 检查是否存在具有相同名称的日志记录器
  rcl_logger_setting_t *logger_setting = NULL;
  for (size_t i = 0; i < log_levels->num_logger_settings; ++i) {
    if (strcmp(log_levels->logger_settings[i].name, logger_name) == 0) {
      logger_setting = &log_levels->logger_settings[i];
      if (logger_setting->level != log_level) {
        // 如果日志级别不同，则替换为新的日志级别
        RCUTILS_LOG_DEBUG_NAMED(
            ROS_PACKAGE_NAME, "Minimum log level of logger [%s] will be replaced from %d to %d",
            logger_name, logger_setting->level, log_level);
        logger_setting->level = log_level;
      }
      return RCL_RET_OK;
    }
  }

  // 检查是否有足够的容量存储日志记录器设置
  if (log_levels->num_logger_settings >= log_levels->capacity_logger_settings) {
    RCL_SET_ERROR_MSG("No capacity to store a logger setting");
    return RCL_RET_ERROR;
  }

  // 复制日志记录器名称
  char *name = rcutils_strdup(logger_name, *allocator);
  if (NULL == name) {
    RCL_SET_ERROR_MSG("failed to copy logger name");
    return RCL_RET_BAD_ALLOC;
  }

  // 添加新的日志记录器设置
  logger_setting = &log_levels->logger_settings[log_levels->num_logger_settings];
  logger_setting->name = name;
  logger_setting->level = log_level;
  log_levels->num_logger_settings += 1;

  return RCL_RET_OK;
}
