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

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/logging.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>

#include "./arguments_impl.h"
#include "rcl/allocator.h"
#include "rcl/error_handling.h"
#include "rcl/logging_rosout.h"
#include "rcl/macros.h"
#include "rcl_logging_interface/rcl_logging_interface.h"
#include "rcutils/logging.h"
#include "rcutils/time.h"

// 定义最大日志输出函数数量
#define RCL_LOGGING_MAX_OUTPUT_FUNCS (4)

// 初始化日志输出处理程序数组，大小为RCL_LOGGING_MAX_OUTPUT_FUNCS
static rcutils_logging_output_handler_t g_rcl_logging_out_handlers[RCL_LOGGING_MAX_OUTPUT_FUNCS] = {
    0};
// 初始化当前已注册的日志输出处理程序数量
static uint8_t g_rcl_logging_num_out_handlers = 0;
// 初始化日志分配器
static rcl_allocator_t g_logging_allocator;
// 初始化标准输出日志功能的启用状态
static bool g_rcl_logging_stdout_enabled = false;
// 初始化ROS输出日志功能的启用状态
static bool g_rcl_logging_rosout_enabled = false;
// 初始化外部库日志功能的启用状态
static bool g_rcl_logging_ext_lib_enabled = false;

/**
 * @brief 一个输出函数，将日志发送到外部日志库。
 *
 * @param location 日志发生的位置，包含文件名、函数名和行号。
 * @param severity 日志的严重程度，如 RCUTILS_LOG_SEVERITY_DEBUG, RCUTILS_LOG_SEVERITY_INFO 等。
 * @param name 记录日志的节点名称。
 * @param timestamp 日志记录的时间戳。
 * @param format 日志消息的格式字符串。
 * @param args 格式字符串中对应的参数列表。
 */
static void rcl_logging_ext_lib_output_handler(
    const rcutils_log_location_t* location,
    int severity,
    const char* name,
    rcutils_time_point_value_t timestamp,
    const char* format,
    va_list* args);

/**
 * @brief 使用指定的输出处理器配置日志系统。
 *
 * @param[in] global_args 全局命令行参数。
 * @param[in] allocator 分配器，用于分配内存。
 * @param[in] output_handler 输出处理器，用于处理日志输出。
 *
 * @return 成功时返回 RCL_RET_OK，失败时返回相应的错误代码。
 */
rcl_ret_t rcl_logging_configure_with_output_handler(
    const rcl_arguments_t* global_args,
    const rcl_allocator_t* allocator,
    rcl_logging_output_handler_t output_handler) {
  // 检查 global_args 是否为空，如果为空则返回无效参数错误
  RCL_CHECK_ARGUMENT_FOR_NULL(global_args, RCL_RET_INVALID_ARGUMENT);
  // 检查分配器是否有效，如果无效则返回无效参数错误
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  // 检查 output_handler 是否为空，如果为空则返回无效参数错误
  RCL_CHECK_ARGUMENT_FOR_NULL(output_handler, RCL_RET_INVALID_ARGUMENT);
  // 使用指定的分配器自动初始化日志系统
  RCUTILS_LOGGING_AUTOINIT_WITH_ALLOCATOR(*allocator);
  // 设置全局日志分配器
  g_logging_allocator = *allocator;
  // 初始化默认日志级别为 -1
  int default_level = -1;
  // 获取命令行参数中的日志级别设置
  rcl_log_levels_t* log_levels = &global_args->impl->log_levels;
  // 获取外部日志配置文件路径
  const char* config_file = global_args->impl->external_log_config_file;
  // 根据命令行参数设置是否启用标准输出日志
  g_rcl_logging_stdout_enabled = !global_args->impl->log_stdout_disabled;
  // 根据命令行参数设置是否启用 rosout 日志
  g_rcl_logging_rosout_enabled = !global_args->impl->log_rosout_disabled;
  // 根据命令行参数设置是否启用外部日志库
  g_rcl_logging_ext_lib_enabled = !global_args->impl->log_ext_lib_disabled;
  // 初始化状态为 RCL_RET_OK
  rcl_ret_t status = RCL_RET_OK;
  // 初始化输出处理器数量为 0
  g_rcl_logging_num_out_handlers = 0;

  // 如果有日志级别设置
  if (log_levels) {
    // 如果默认日志级别不是未设置状态，则更新默认日志级别并设置到 rcutils
    if (log_levels->default_logger_level != RCUTILS_LOG_SEVERITY_UNSET) {
      default_level = (int)log_levels->default_logger_level;
      rcutils_logging_set_default_logger_level(default_level);
    }

    // 遍历所有日志设置，为每个 logger 设置对应的日志级别
    for (size_t i = 0; i < log_levels->num_logger_settings; ++i) {
      rcutils_ret_t rcutils_status = rcutils_logging_set_logger_level(
          log_levels->logger_settings[i].name, (int)log_levels->logger_settings[i].level);
      // 如果设置失败，则返回错误
      if (RCUTILS_RET_OK != rcutils_status) {
        return RCL_RET_ERROR;
      }
    }
  }
  // 如果启用了标准输出日志，则将控制台输出处理器添加到输出处理器数组中
  if (g_rcl_logging_stdout_enabled) {
    g_rcl_logging_out_handlers[g_rcl_logging_num_out_handlers++] =
        rcutils_logging_console_output_handler;
  }
  // 如果启用了 rosout 日志，则初始化 rosout 日志系统并将其输出处理器添加到输出处理器数组中
  if (g_rcl_logging_rosout_enabled) {
    status = rcl_logging_rosout_init(allocator);
    if (RCL_RET_OK == status) {
      g_rcl_logging_out_handlers[g_rcl_logging_num_out_handlers++] =
          rcl_logging_rosout_output_handler;
    }
  }
  // 如果启用了外部日志库，则初始化外部日志库并将其输出处理器添加到输出处理器数组中
  if (g_rcl_logging_ext_lib_enabled) {
    status = rcl_logging_external_initialize(config_file, g_logging_allocator);
    if (RCL_RET_OK == status) {
      rcl_logging_ret_t logging_status = rcl_logging_external_set_logger_level(NULL, default_level);
      // 如果设置默认日志级别失败，则返回错误
      if (RCL_LOGGING_RET_OK != logging_status) {
        status = RCL_RET_ERROR;
      }
      g_rcl_logging_out_handlers[g_rcl_logging_num_out_handlers++] =
          rcl_logging_ext_lib_output_handler;
    }
  }
  // 设置 rcutils 的输出处理器为指定的输出处理器
  rcutils_logging_set_output_handler(output_handler);
  // 返回状态
  return status;
}

/**
 * @brief 配置日志系统
 *
 * @param[in] global_args 全局参数，用于配置日志系统
 * @param[in] allocator 分配器，用于分配内存
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_logging_configure(
    const rcl_arguments_t* global_args, const rcl_allocator_t* allocator) {
  // 调用 rcl_logging_configure_with_output_handler 函数进行日志系统配置
  return rcl_logging_configure_with_output_handler(
      global_args, allocator, &rcl_logging_multiple_output_handler);
}

/**
 * @brief 关闭日志系统
 *
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_logging_fini(void) {
  // 初始化状态变量为 RCL_RET_OK
  rcl_ret_t status = RCL_RET_OK;

  // 设置输出处理器为 rcutils_logging_console_output_handler
  rcutils_logging_set_output_handler(rcutils_logging_console_output_handler);

  // 更新 g_rcl_logging_num_out_handlers 和 g_rcl_logging_out_handlers，
  // 以便在调用 rcl_logging_fini 后不再调用 rcl_logging_ext_lib_output_handler
  g_rcl_logging_num_out_handlers = 1;
  g_rcl_logging_out_handlers[0] = rcutils_logging_console_output_handler;

  // 如果启用了 g_rcl_logging_rosout_enabled，则关闭 rosout 日志
  if (g_rcl_logging_rosout_enabled) {
    status = rcl_logging_rosout_fini();
  }

  // 如果状态为 RCL_RET_OK 且启用了 g_rcl_logging_ext_lib_enabled，则关闭外部日志库
  if (RCL_RET_OK == status && g_rcl_logging_ext_lib_enabled) {
    status = rcl_logging_external_shutdown();
  }

  // 返回操作结果
  return status;
}

/**
 * @brief 检查ROSout日志功能是否启用
 *
 * @return 如果启用了ROSout日志功能，则返回true，否则返回false
 */
bool rcl_logging_rosout_enabled(void) {
  return g_rcl_logging_rosout_enabled;  // 返回全局变量g_rcl_logging_rosout_enabled的值
}

/**
 * @brief 处理多个输出日志的函数
 *
 * @param[in] location 日志发生的位置信息
 * @param[in] severity 日志的严重程度
 * @param[in] name 日志记录器的名称
 * @param[in] timestamp 日志发生的时间戳
 * @param[in] format 日志消息的格式字符串
 * @param[in] args 格式化日志消息所需的参数列表
 */
void rcl_logging_multiple_output_handler(
    const rcutils_log_location_t* location,
    int severity,
    const char* name,
    rcutils_time_point_value_t timestamp,
    const char* format,
    va_list* args) {
  // 遍历所有已注册的日志处理函数
  for (uint8_t i = 0; i < g_rcl_logging_num_out_handlers && NULL != g_rcl_logging_out_handlers[i];
       ++i) {
    // 调用当前遍历到的日志处理函数，传入相应的参数
    g_rcl_logging_out_handlers[i](location, severity, name, timestamp, format, args);
  }
}

/**
 * @brief 处理日志输出的函数
 *
 * @param[in] location 日志发生的位置，包含文件名、函数名和行号
 * @param[in] severity 日志的严重程度
 * @param[in] name 日志记录器的名称
 * @param[in] timestamp 日志发生的时间戳
 * @param[in] format 日志消息的格式字符串
 * @param[in] args 格式化参数列表
 */
static void rcl_logging_ext_lib_output_handler(
    const rcutils_log_location_t* location,
    int severity,
    const char* name,
    rcutils_time_point_value_t timestamp,
    const char* format,
    va_list* args) {
  // 定义返回状态变量
  rcl_ret_t status;

  // 初始化消息缓冲区
  char msg_buf[1024] = "";
  rcutils_char_array_t msg_array = {
      .buffer = msg_buf,
      .owns_buffer = false,
      .buffer_length = 0u,
      .buffer_capacity = sizeof(msg_buf),
      .allocator = g_logging_allocator};

  // 初始化输出缓冲区
  char output_buf[1024] = "";
  rcutils_char_array_t output_array = {
      .buffer = output_buf,
      .owns_buffer = false,
      .buffer_length = 0u,
      .buffer_capacity = sizeof(output_buf),
      .allocator = g_logging_allocator};

  // 将格式化参数应用于格式字符串并将结果存储在msg_array中
  status = rcutils_char_array_vsprintf(&msg_array, format, *args);

  // 检查格式化是否成功
  if (RCL_RET_OK == status) {
    // 格式化日志消息
    status = rcutils_logging_format_message(
        location, severity, name, timestamp, msg_array.buffer, &output_array);
    // 检查格式化是否成功
    if (RCL_RET_OK != status) {
      RCUTILS_SAFE_FWRITE_TO_STDERR("failed to format log message: ");
      RCUTILS_SAFE_FWRITE_TO_STDERR(rcl_get_error_string().str);
      rcl_reset_error();
      RCUTILS_SAFE_FWRITE_TO_STDERR("\n");
    }
  } else {
    RCUTILS_SAFE_FWRITE_TO_STDERR("failed to format user log message: ");
    RCUTILS_SAFE_FWRITE_TO_STDERR(rcl_get_error_string().str);
    rcl_reset_error();
    RCUTILS_SAFE_FWRITE_TO_STDERR("\n");
  }

  // 将格式化后的日志消息发送到外部日志记录器
  rcl_logging_external_log(severity, name, output_array.buffer);

  // 清理msg_array资源
  status = rcutils_char_array_fini(&msg_array);
  if (RCL_RET_OK != status) {
    RCUTILS_SAFE_FWRITE_TO_STDERR("failed to finalize char array: ");
    RCUTILS_SAFE_FWRITE_TO_STDERR(rcl_get_error_string().str);
    rcl_reset_error();
    RCUTILS_SAFE_FWRITE_TO_STDERR("\n");
  }

  // 清理output_array资源
  status = rcutils_char_array_fini(&output_array);
  if (RCL_RET_OK != status) {
    RCUTILS_SAFE_FWRITE_TO_STDERR("failed to finalize char array: ");
    RCUTILS_SAFE_FWRITE_TO_STDERR(rcl_get_error_string().str);
    rcl_reset_error();
    RCUTILS_SAFE_FWRITE_TO_STDERR("\n");
  }
}

#ifdef __cplusplus
}
#endif
