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

/// \cond INTERNAL  // Internal Doxygen documentation

#include "rcl/arguments.h"

#include <assert.h>
#include <string.h>

#include "./arguments_impl.h"
#include "./remap_impl.h"
#include "rcl/error_handling.h"
#include "rcl/lexer_lookahead.h"
#include "rcl/validate_topic_name.h"
#include "rcl_yaml_param_parser/parser.h"
#include "rcl_yaml_param_parser/types.h"
#include "rcutils/allocator.h"
#include "rcutils/error_handling.h"
#include "rcutils/format_string.h"
#include "rcutils/logging.h"
#include "rcutils/logging_macros.h"
#include "rcutils/strdup.h"
#include "rmw/validate_namespace.h"
#include "rmw/validate_node_name.h"

#ifdef __cplusplus
extern "C" {
#endif

/// 解析可能是重映射规则的参数。
/**
 * \param[in] arg 要解析的参数
 * \param[in] allocator 要使用的分配器
 * \param[in,out] output_rule 输入一个零初始化的规则，输出一个完全初始化的规则
 * \return RCL_RET_OK 如果解析出有效的规则，或者
 * \return RCL_RET_INVALID_REMAP_RULE 如果参数不是有效的规则，或者
 * \return RCL_RET_BAD_ALLOC 如果分配失败，或者
 * \return RLC_RET_ERROR 如果发生未指定的错误。
 */
RCL_LOCAL
rcl_ret_t _rcl_parse_remap_rule(
    const char *arg, rcl_allocator_t allocator, rcl_remap_t *output_rule);

/// 解析可能是参数规则的参数。
/**
 * \param[in] arg 要解析的参数
 * \param[in,out] params 要填充的参数覆盖结构。
 *     此结构必须由调用者初始化。
 * \return RCL_RET_OK 如果解析出有效的规则，或者
 * \return RCL_RET_INVALID_ARGUMENT 如果参数无效，或者
 * \return RCL_RET_INVALID_PARAM_RULE 如果参数不是有效的规则，或者
 * \return RCL_RET_BAD_ALLOC 如果分配失败，或者
 * \return RLC_RET_ERROR 如果发生未指定的错误。
 */
rcl_ret_t _rcl_parse_param_rule(const char *arg, rcl_params_t *params);

/**
 * @brief 获取参数文件列表
 *
 * @param[in] arguments 输入的 rcl_arguments_t 结构体指针
 * @param[in] allocator 分配器，用于分配内存
 * @param[out] parameter_files 输出参数文件列表的二级指针
 * @return 返回 rcl_ret_t 类型的状态码
 */
rcl_ret_t rcl_arguments_get_param_files(
    const rcl_arguments_t *arguments, rcl_allocator_t allocator, char ***parameter_files) {
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(&allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(arguments, RCL_RET_INVALID_ARGUMENT);
  // 检查实现结构体是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(arguments->impl, RCL_RET_INVALID_ARGUMENT);
  // 检查输出参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(parameter_files, RCL_RET_INVALID_ARGUMENT);

  // 为参数文件列表分配内存
  *(parameter_files) =
      allocator.allocate(sizeof(char *) * arguments->impl->num_param_files_args, allocator.state);
  // 判断内存分配是否成功
  if (NULL == *parameter_files) {
    return RCL_RET_BAD_ALLOC;
  }

  // 遍历参数文件列表并复制到输出参数中
  for (int i = 0; i < arguments->impl->num_param_files_args; ++i) {
    (*parameter_files)[i] = rcutils_strdup(arguments->impl->parameter_files[i], allocator);
    // 判断字符串复制是否成功
    if (NULL == (*parameter_files)[i]) {
      // 释放已分配的内存
      for (int r = i; r >= 0; --r) {
        allocator.deallocate((*parameter_files)[r], allocator.state);
      }
      // 释放参数文件列表内存
      allocator.deallocate((*parameter_files), allocator.state);
      (*parameter_files) = NULL;
      return RCL_RET_BAD_ALLOC;
    }
  }

  return RCL_RET_OK;
}

/**
 * @brief 获取参数文件数量
 *
 * @param[in] args 输入的 rcl_arguments_t 结构体指针
 * @return 返回参数文件数量，如果输入参数为空或实现结构体为空，则返回 -1
 */
int rcl_arguments_get_param_files_count(const rcl_arguments_t *args) {
  // 检查输入参数是否为空以及实现结构体是否为空
  if (NULL == args || NULL == args->impl) {
    return -1;
  }
  // 返回参数文件数量
  return args->impl->num_param_files_args;
}

/**
 * @brief 获取参数覆盖值
 *
 * @param[in] arguments 输入的 rcl_arguments_t 结构体指针
 * @param[out] parameter_overrides 输出的 rcl_params_t 结构体指针的指针
 * @return rcl_ret_t 返回状态码，成功返回 RCL_RET_OK
 */
rcl_ret_t rcl_arguments_get_param_overrides(
    const rcl_arguments_t *arguments, rcl_params_t **parameter_overrides) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(arguments, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(arguments->impl, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(parameter_overrides, RCL_RET_INVALID_ARGUMENT);

  // 检查输出参数是否已经分配内存
  if (NULL != *parameter_overrides) {
    RCL_SET_ERROR_MSG("Output parameter override pointer is not null. May leak memory.");
    return RCL_RET_INVALID_ARGUMENT;
  }
  *parameter_overrides = NULL;

  // 如果存在参数覆盖值，则复制到输出参数中
  if (NULL != arguments->impl->parameter_overrides) {
    *parameter_overrides = rcl_yaml_node_struct_copy(arguments->impl->parameter_overrides);
    if (NULL == *parameter_overrides) {
      return RCL_RET_BAD_ALLOC;
    }
  }
  return RCL_RET_OK;
}

/**
 * @brief 获取日志等级
 *
 * @param[in] arguments 输入的 rcl_arguments_t 结构体指针
 * @param[out] log_levels 输出的 rcl_log_levels_t 结构体指针
 * @return rcl_ret_t 返回状态码，成功返回 RCL_RET_OK
 */
rcl_ret_t rcl_arguments_get_log_levels(
    const rcl_arguments_t *arguments, rcl_log_levels_t *log_levels) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(arguments, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(arguments->impl, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(log_levels, RCL_RET_INVALID_ARGUMENT);

  // 获取分配器
  const rcl_allocator_t *allocator = &arguments->impl->allocator;
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);

  // 复制日志等级到输出参数中
  return rcl_log_levels_copy(&arguments->impl->log_levels, log_levels);
}

/// 解析可能是日志级别规则的参数。
/**
 * \param[in] arg 要解析的参数
 * \param[in,out] log_levels 解析出的默认日志级别或日志设置
 * \return RCL_RET_OK 如果解析出有效的日志级别，或者
 * \return RCL_RET_INVALID_LOG_LEVEL_RULE 如果参数不是有效的规则，或者
 * \return RCL_RET_BAD_ALLOC 如果分配失败，或者
 * \return RLC_RET_ERROR 如果发生未指定的错误。
 */
RCL_LOCAL
rcl_ret_t _rcl_parse_log_level(const char *arg, rcl_log_levels_t *log_levels);

/// 解析可能是日志配置文件的参数。
/**
 * \param[in] arg 要解析的参数
 * \param[in] allocator 要使用的分配器
 * \param[in,out] log_config_file 解析出的日志配置文件
 * \return RCL_RET_OK 如果解析出有效的日志配置文件，或者
 * \return RCL_RET_BAD_ALLOC 如果分配失败，或者
 * \return RLC_RET_ERROR 如果发生未指定的错误。
 */
RCL_LOCAL
rcl_ret_t _rcl_parse_external_log_config_file(
    const char *arg, rcl_allocator_t allocator, char **log_config_file);

/// 解析可能是参数文件的参数。
/**
 * 文件名的语法不进行验证。
 * \param[in] arg 要解析的参数
 * \param[in] allocator 要使用的分配器
 * \param[in] params 指向填充的参数结构
 * \param[in,out] param_file 可能是参数文件名的字符串
 * \return RCL_RET_OK 如果规则解析正确，或者
 * \return RCL_RET_BAD_ALLOC 如果分配失败，或者
 * \return RLC_RET_ERROR 如果发生未指定的错误。
 */
RCL_LOCAL
rcl_ret_t _rcl_parse_param_file(
    const char *arg, rcl_allocator_t allocator, rcl_params_t *params, char **param_file);

/// 解析安全领域参数。
/**
 * \param[in] arg 要解析的参数
 * \param[in] allocator 使用的分配器
 * \param[in,out] enclave 解析后的安全领域
 * \return RCL_RET_OK 如果成功解析了有效的安全领域，或者
 * \return RCL_RET_BAD_ALLOC 如果分配失败，或者
 * \return RLC_RET_ERROR 如果发生未指定的错误。
 */
RCL_LOCAL
rcl_ret_t _rcl_parse_enclave(const char *arg, rcl_allocator_t allocator, char **enclave);

#define RCL_ENABLE_FLAG_PREFIX "--enable-"    // 定义启用标志前缀
#define RCL_DISABLE_FLAG_PREFIX "--disable-"  // 定义禁用标志前缀

/// 解析可能针对提供的键规则的布尔参数。
/**
 * \param[in] arg 要解析的参数
 * \param[in] key 要解析的参数的键。应该是一个以空字符结尾的字符串
 * \param[in,out] value 解析后的布尔值
 * \return RCL_RET_OK 如果成功解析了布尔参数，或者
 * \return RLC_RET_ERROR 如果发生未指定的错误。
 */
RCL_LOCAL
rcl_ret_t _rcl_parse_disabling_flag(const char *arg, const char *key, bool *value);

/// 为目标参数分配并初始化 impl。
/**
 * \param[out] args 要设置 impl 的目标参数
 * \param[in] allocator 要使用的分配器
 * \return RCL_RET_OK 如果解析了有效的规则，或者
 * \return RCL_RET_BAD_ALLOC 如果分配失败
 */
rcl_ret_t _rcl_allocate_initialized_arguments_impl(
    rcl_arguments_t *args, rcl_allocator_t *allocator);

// 解析命令行参数
rcl_ret_t rcl_parse_arguments(
    int argc, const char *const *argv, rcl_allocator_t allocator, rcl_arguments_t *args_output) {
  // 检查分配器是否有效，如果无效则返回错误信息
  RCL_CHECK_ALLOCATOR_WITH_MSG(&allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);

  // 如果参数数量小于0，则设置错误信息并返回无效参数错误
  if (argc < 0) {
    RCL_SET_ERROR_MSG("Argument count cannot be negative");
    return RCL_RET_INVALID_ARGUMENT;
  } else if (argc > 0) {
    // 如果参数数量大于0，检查参数是否为空，如果为空则返回无效参数错误
    RCL_CHECK_ARGUMENT_FOR_NULL(argv, RCL_RET_INVALID_ARGUMENT);
  }

  // 检查输出参数是否为空，如果为空则返回无效参数错误
  RCL_CHECK_ARGUMENT_FOR_NULL(args_output, RCL_RET_INVALID_ARGUMENT);

  // 如果输出参数的 impl 不为空，则设置错误信息并返回无效参数错误
  if (args_output->impl != NULL) {
    RCL_SET_ERROR_MSG("Parse output is not zero-initialized");
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 定义返回值变量和失败时的返回值变量
  rcl_ret_t ret;
  rcl_ret_t fail_ret;

  /**
   * @brief 初始化参数列表并分配内存
   *
   * @param[in] args_output 参数输出结构体指针
   * @param[in] allocator 分配器
   * @return 返回rcl_ret_t类型的状态值
   */
  ret = _rcl_allocate_initialized_arguments_impl(args_output, &allocator);
  if (RCL_RET_OK != ret) {
    return ret;
  }

  // 获取参数实现结构体指针
  rcl_arguments_impl_t *args_impl = args_output->impl;

  // 如果参数数量为0，表示没有参数需要解析
  if (argc == 0) {
    // 没有参数需要解析
    return RCL_RET_OK;
  }

  // 根据参数数量分配足够的内存空间给数组
  args_impl->remap_rules = allocator.allocate(sizeof(rcl_remap_t) * argc, allocator.state);
  if (NULL == args_impl->remap_rules) {
    ret = RCL_RET_BAD_ALLOC;
    goto fail;
  }

  // 初始化参数覆盖结构体
  args_impl->parameter_overrides = rcl_yaml_node_struct_init(allocator);
  if (NULL == args_impl->parameter_overrides) {
    ret = RCL_RET_BAD_ALLOC;
    goto fail;
  }

  // 为参数文件分配内存空间
  args_impl->parameter_files = allocator.allocate(sizeof(char *) * argc, allocator.state);
  if (NULL == args_impl->parameter_files) {
    ret = RCL_RET_BAD_ALLOC;
    goto fail;
  }

  // 为未解析的ROS参数分配内存空间
  args_impl->unparsed_ros_args = allocator.allocate(sizeof(int) * argc, allocator.state);
  if (NULL == args_impl->unparsed_ros_args) {
    ret = RCL_RET_BAD_ALLOC;
    goto fail;
  }

  // 为未解析的参数分配内存空间
  args_impl->unparsed_args = allocator.allocate(sizeof(int) * argc, allocator.state);
  if (NULL == args_impl->unparsed_args) {
    ret = RCL_RET_BAD_ALLOC;
    goto fail;
  }

  // 初始化日志级别结构体
  rcl_log_levels_t log_levels = rcl_get_zero_initialized_log_levels();
  ret = rcl_log_levels_init(&log_levels, &allocator, argc);
  if (ret != RCL_RET_OK) {
    goto fail;
  }

  // 将初始化后的日志级别结构体赋值给参数实现结构体
  args_impl->log_levels = log_levels;

  bool parsing_ros_args = false;  // 定义一个布尔变量，用于判断是否正在解析ROS参数

  // 遍历命令行参数
  for (int i = 0; i < argc; ++i) {
    if (parsing_ros_args) {  // 如果正在解析ROS参数
      // 忽略ROS特定的参数标志
      if (strcmp(RCL_ROS_ARGS_FLAG, argv[i]) == 0) {
        continue;
      }

      // 检查ROS特定参数的显式结束标记
      if (strcmp(RCL_ROS_ARGS_EXPLICIT_END_TOKEN, argv[i]) == 0) {
        parsing_ros_args = false;
        continue;
      }

      // 尝试将参数解析为参数覆盖标志
      if (strcmp(RCL_PARAM_FLAG, argv[i]) == 0 || strcmp(RCL_SHORT_PARAM_FLAG, argv[i]) == 0) {
        if (i + 1 < argc) {
          // 尝试将下一个参数解析为参数覆盖规则
          if (RCL_RET_OK == _rcl_parse_param_rule(argv[i + 1], args_impl->parameter_overrides)) {
            RCUTILS_LOG_DEBUG_NAMED(
                ROS_PACKAGE_NAME, "Got param override rule : %s\n", argv[i + 1]);
            ++i;  // 跳过此处的标志，循环将跳过规则。
            continue;
          }
          rcl_error_string_t prev_error_string = rcl_get_error_string();
          rcl_reset_error();
          RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
              "Couldn't parse parameter override rule: '%s %s'. Error: %s", argv[i], argv[i + 1],
              prev_error_string.str);
        } else {
          RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
              "Couldn't parse trailing %s flag. No parameter override rule found.", argv[i]);
        }
        ret = RCL_RET_INVALID_ROS_ARGS;
        goto fail;
      }
      RCUTILS_LOG_DEBUG_NAMED(
          ROS_PACKAGE_NAME, "Arg %d (%s) is not a %s nor a %s flag.", i, argv[i], RCL_PARAM_FLAG,
          RCL_SHORT_PARAM_FLAG);

      // 尝试将参数解析为重映射规则标志
      if (strcmp(RCL_REMAP_FLAG, argv[i]) == 0 || strcmp(RCL_SHORT_REMAP_FLAG, argv[i]) == 0) {
        if (i + 1 < argc) {
          // 尝试将下一个参数解析为重映射规则
          rcl_remap_t *rule = &(args_impl->remap_rules[args_impl->num_remap_rules]);
          *rule = rcl_get_zero_initialized_remap();
          if (RCL_RET_OK == _rcl_parse_remap_rule(argv[i + 1], allocator, rule)) {
            ++(args_impl->num_remap_rules);
            RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Got remap rule : %s\n", argv[i + 1]);
            ++i;  // 跳过此处的标志，循环将跳过规则。
            continue;
          }
          rcl_error_string_t prev_error_string = rcl_get_error_string();
          rcl_reset_error();
          RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
              "Couldn't parse remap rule: '%s %s'. Error: %s", argv[i], argv[i + 1],
              prev_error_string.str);
        } else {
          RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
              "Couldn't parse trailing %s flag. No remap rule found.", argv[i]);
        }
        ret = RCL_RET_INVALID_ROS_ARGS;
        goto fail;
      }
      RCUTILS_LOG_DEBUG_NAMED(
          ROS_PACKAGE_NAME, "Arg %d (%s) is not a %s nor a %s flag.", i, argv[i], RCL_REMAP_FLAG,
          RCL_SHORT_REMAP_FLAG);

      // 尝试将参数解析为参数文件规则
      if (strcmp(RCL_PARAM_FILE_FLAG, argv[i]) == 0) {
        if (i + 1 < argc) {
          // 尝试将下一个参数解析为参数文件规则
          args_impl->parameter_files[args_impl->num_param_files_args] = NULL;
          if (RCL_RET_OK == _rcl_parse_param_file(
                                argv[i + 1], allocator, args_impl->parameter_overrides,
                                &args_impl->parameter_files[args_impl->num_param_files_args])) {
            ++(args_impl->num_param_files_args);
            RCUTILS_LOG_DEBUG_NAMED(
                ROS_PACKAGE_NAME, "Got params file : %s\ntotal num param files %d",
                args_impl->parameter_files[args_impl->num_param_files_args - 1],
                args_impl->num_param_files_args);
            ++i;  // 跳过此处的标志，循环将跳过规则。
            continue;
          }
          rcl_error_string_t prev_error_string = rcl_get_error_string();
          rcl_reset_error();
          RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
              "Couldn't parse params file: '%s %s'. Error: %s", argv[i], argv[i + 1],
              prev_error_string.str);
        } else {
          RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
              "Couldn't parse trailing %s flag. No file path provided.", argv[i]);
        }
        ret = RCL_RET_INVALID_ROS_ARGS;
        goto fail;
      }
      RCUTILS_LOG_DEBUG_NAMED(
          ROS_PACKAGE_NAME, "Arg %d (%s) is not a %s flag.", i, argv[i], RCL_PARAM_FILE_FLAG);

      // 尝试将参数解析为日志级别配置
      if (strcmp(RCL_LOG_LEVEL_FLAG, argv[i]) == 0) {
        if (i + 1 < argc) {
          if (RCL_RET_OK == _rcl_parse_log_level(argv[i + 1], &args_impl->log_levels)) {
            RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Got log level: %s\n", argv[i + 1]);
            ++i;  // 跳过此处的标志，循环将跳过值。
            continue;
          }
          rcl_error_string_t prev_error_string = rcl_get_error_string();
          rcl_reset_error();
          RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
              "Couldn't parse log level: '%s %s'. Error: %s", argv[i], argv[i + 1],
              prev_error_string.str);
        } else {
          RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
              "Couldn't parse trailing log level flag: '%s'. No log level provided.", argv[i]);
        }
        ret = RCL_RET_INVALID_ROS_ARGS;
        goto fail;
      }
      RCUTILS_LOG_DEBUG_NAMED(
          ROS_PACKAGE_NAME, "Arg %d (%s) is not a %s flag.", i, argv[i], RCL_LOG_LEVEL_FLAG);

      // 尝试将参数解析为日志配置文件
      if (strcmp(RCL_EXTERNAL_LOG_CONFIG_FLAG, argv[i]) == 0) {
        if (i + 1 < argc) {
          if (NULL != args_impl->external_log_config_file) {
            RCUTILS_LOG_DEBUG_NAMED(
                ROS_PACKAGE_NAME, "Overriding log configuration file : %s\n",
                args_impl->external_log_config_file);
            allocator.deallocate(args_impl->external_log_config_file, allocator.state);
            args_impl->external_log_config_file = NULL;
          }
          if (RCL_RET_OK == _rcl_parse_external_log_config_file(
                                argv[i + 1], allocator, &args_impl->external_log_config_file)) {
            RCUTILS_LOG_DEBUG_NAMED(
                ROS_PACKAGE_NAME, "Got log configuration file : %s\n",
                args_impl->external_log_config_file);
            ++i;  // 跳过此处的标志，循环将跳过值。
            continue;
          }
          rcl_error_string_t prev_error_string = rcl_get_error_string();
          rcl_reset_error();
          RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
              "Couldn't parse log configuration file: '%s %s'. Error: %s", argv[i], argv[i + 1],
              prev_error_string.str);
        } else {
          RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
              "Couldn't parse trailing %s flag. No file path provided.", argv[i]);
        }
        ret = RCL_RET_INVALID_ROS_ARGS;
        goto fail;
      }

      // 尝试将参数解析为安全领域
      if (strcmp(RCL_ENCLAVE_FLAG, argv[i]) == 0 || strcmp(RCL_SHORT_ENCLAVE_FLAG, argv[i]) == 0) {
        if (i + 1 < argc) {
          if (NULL != args_impl->enclave) {
            RCUTILS_LOG_DEBUG_NAMED(
                ROS_PACKAGE_NAME, "Overriding security enclave : %s\n", args_impl->enclave);
            allocator.deallocate(args_impl->enclave, allocator.state);
            args_impl->enclave = NULL;
          }
          if (RCL_RET_OK == _rcl_parse_enclave(argv[i + 1], allocator, &args_impl->enclave)) {
            RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Got enclave: %s\n", args_impl->enclave);
            ++i;  // 跳过此处的标志，循环将跳过值。
            continue;
          }
          rcl_error_string_t prev_error_string = rcl_get_error_string();
          rcl_reset_error();
          RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
              "Couldn't parse enclave name: '%s %s'. Error: %s", argv[i], argv[i + 1],
              prev_error_string.str);
        } else {
          RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
              "Couldn't parse trailing %s flag. No enclave path provided.", argv[i]);
        }
        ret = RCL_RET_INVALID_ROS_ARGS;
        goto fail;
      }

      RCUTILS_LOG_DEBUG_NAMED(
          ROS_PACKAGE_NAME, "Arg %d (%s) is not a %s flag.", i, argv[i],
          RCL_EXTERNAL_LOG_CONFIG_FLAG);

      // 尝试解析 --enable/disable-stdout-logs 标志
      ret = _rcl_parse_disabling_flag(
          argv[i], RCL_LOG_STDOUT_FLAG_SUFFIX, &args_impl->log_stdout_disabled);
      if (RCL_RET_OK == ret) {
        RCUTILS_LOG_DEBUG_NAMED(
            ROS_PACKAGE_NAME, "Disable log stdout ? %s\n",
            args_impl->log_stdout_disabled ? "true" : "false");
        continue;
      }
      RCUTILS_LOG_DEBUG_NAMED(
          ROS_PACKAGE_NAME, "Couldn't parse arg %d (%s) as %s%s or %s%s flag. Error: %s", i,
          argv[i], RCL_ENABLE_FLAG_PREFIX, RCL_LOG_STDOUT_FLAG_SUFFIX, RCL_DISABLE_FLAG_PREFIX,
          RCL_LOG_STDOUT_FLAG_SUFFIX, rcl_get_error_string().str);
      rcl_reset_error();

      /**
       * @brief 尝试解析 --enable/disable-rosout-logs 标志
       *
       * @param[in] argv[i] 命令行参数
       * @param[out] args_impl->log_rosout_disabled 是否禁用 rosout 日志的标志
       * @return 返回 RCL_RET_OK 表示解析成功，其他值表示解析失败
       */
      ret = _rcl_parse_disabling_flag(
          argv[i], RCL_LOG_ROSOUT_FLAG_SUFFIX, &args_impl->log_rosout_disabled);
      if (RCL_RET_OK == ret) {
        // 输出解析结果
        RCUTILS_LOG_DEBUG_NAMED(
            ROS_PACKAGE_NAME, "Disable log rosout ? %s\n",
            args_impl->log_rosout_disabled ? "true" : "false");
        continue;
      }
      // 解析失败，输出错误信息
      RCUTILS_LOG_DEBUG_NAMED(
          ROS_PACKAGE_NAME, "Couldn't parse arg %d (%s) as %s%s or %s%s flag. Error: %s", i,
          argv[i], RCL_ENABLE_FLAG_PREFIX, RCL_LOG_ROSOUT_FLAG_SUFFIX, RCL_DISABLE_FLAG_PREFIX,
          RCL_LOG_ROSOUT_FLAG_SUFFIX, rcl_get_error_string().str);
      rcl_reset_error();

      // 尝试解析 --enable/disable-external-lib-logs 标志
      ret = _rcl_parse_disabling_flag(
          argv[i], RCL_LOG_EXT_LIB_FLAG_SUFFIX, &args_impl->log_ext_lib_disabled);
      if (RCL_RET_OK == ret) {
        // 输出解析结果
        RCUTILS_LOG_DEBUG_NAMED(
            ROS_PACKAGE_NAME, "Disable log external lib ? %s\n",
            args_impl->log_ext_lib_disabled ? "true" : "false");
        continue;
      }
      // 解析失败，输出错误信息
      RCUTILS_LOG_DEBUG_NAMED(
          ROS_PACKAGE_NAME, "Couldn't parse arg %d (%s) as %s%s or %s%s flag. Error: %s", i,
          argv[i], RCL_ENABLE_FLAG_PREFIX, RCL_LOG_EXT_LIB_FLAG_SUFFIX, RCL_DISABLE_FLAG_PREFIX,
          RCL_LOG_EXT_LIB_FLAG_SUFFIX, rcl_get_error_string().str);
      rcl_reset_error();

      // 参数是一个未知的 ROS 特定参数
      args_impl->unparsed_ros_args[args_impl->num_unparsed_ros_args] = i;
      ++(args_impl->num_unparsed_ros_args);
    } else {
      // Check for ROS specific arguments flags
      if (strcmp(RCL_ROS_ARGS_FLAG, argv[i]) == 0) {
        parsing_ros_args = true;
        continue;
      }

      // 尝试以其已弃用的形式解析参数作为重映射规则
      rcl_remap_t *rule = &(args_impl->remap_rules[args_impl->num_remap_rules]);
      // 初始化重映射规则结构体
      *rule = rcl_get_zero_initialized_remap();
      // 如果成功解析重映射规则
      if (RCL_RET_OK == _rcl_parse_remap_rule(argv[i], allocator, rule)) {
        // 输出警告信息，提示找到了已弃用的重映射规则语法
        RCUTILS_LOG_WARN_NAMED(
            ROS_PACKAGE_NAME,
            "Found remap rule '%s'. This syntax is deprecated. Use '%s %s %s' instead.", argv[i],
            RCL_ROS_ARGS_FLAG, RCL_REMAP_FLAG, argv[i]);
        // 输出调试信息，显示获取到的重映射规则
        RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Got remap rule : %s\n", argv[i + 1]);
        // 增加重映射规则计数
        ++(args_impl->num_remap_rules);
        continue;
      }
      // 如果无法解析参数作为已弃用的重映射规则
      RCUTILS_LOG_DEBUG_NAMED(
          ROS_PACKAGE_NAME,
          "Couldn't parse arg %d (%s) as a remap rule in its deprecated form. Error: %s", i,
          argv[i], rcl_get_error_string().str);
      // 重置错误信息
      rcl_reset_error();

      // 参数不是特定于ROS的参数
      args_impl->unparsed_args[args_impl->num_unparsed_args] = i;
      // 增加未解析参数计数
      ++(args_impl->num_unparsed_args);
    }
  }

  // 缩小 remap_rules 数组以匹配成功解析的规则数量
  if (0 == args_impl->num_remap_rules) {
    // 没有重映射规则
    allocator.deallocate(args_impl->remap_rules, allocator.state);
    args_impl->remap_rules = NULL;
  } else if (args_impl->num_remap_rules < argc) {
    rcl_remap_t *new_remap_rules = allocator.reallocate(
        args_impl->remap_rules, sizeof(rcl_remap_t) * args_impl->num_remap_rules, &allocator);
    if (NULL == new_remap_rules) {
      ret = RCL_RET_BAD_ALLOC;
      goto fail;
    }
    args_impl->remap_rules = new_remap_rules;
  }

  // 缩小参数文件数组
  if (0 == args_impl->num_param_files_args) {
    allocator.deallocate(args_impl->parameter_files, allocator.state);
    args_impl->parameter_files = NULL;
  } else if (args_impl->num_param_files_args < argc) {
    char **new_parameter_files = allocator.reallocate(
        args_impl->parameter_files, sizeof(char *) * args_impl->num_param_files_args, &allocator);
    if (NULL == new_parameter_files) {
      ret = RCL_RET_BAD_ALLOC;
      goto fail;
    }
    args_impl->parameter_files = new_parameter_files;
  }

  // 如果没有找到参数覆盖，删除参数覆盖
  if (0U == args_impl->parameter_overrides->num_nodes) {
    rcl_yaml_node_struct_fini(args_impl->parameter_overrides);
    args_impl->parameter_overrides = NULL;
  }

  // 缩小未解析的 ROS 参数数组
  if (0 == args_impl->num_unparsed_ros_args) {
    // 没有未解析的 ROS 参数
    allocator.deallocate(args_impl->unparsed_ros_args, allocator.state);
    args_impl->unparsed_ros_args = NULL;
  } else if (args_impl->num_unparsed_ros_args < argc) {
    args_impl->unparsed_ros_args = rcutils_reallocf(
        args_impl->unparsed_ros_args, sizeof(int) * args_impl->num_unparsed_ros_args, &allocator);
    if (NULL == args_impl->unparsed_ros_args) {
      ret = RCL_RET_BAD_ALLOC;
      goto fail;
    }
  }

  // 缩小未解析的参数数组
  if (0 == args_impl->num_unparsed_args) {
    // 没有未解析的参数
    allocator.deallocate(args_impl->unparsed_args, allocator.state);
    args_impl->unparsed_args = NULL;
  } else if (args_impl->num_unparsed_args < argc) {
    args_impl->unparsed_args = rcutils_reallocf(
        args_impl->unparsed_args, sizeof(int) * args_impl->num_unparsed_args, &allocator);
    if (NULL == args_impl->unparsed_args) {
      ret = RCL_RET_BAD_ALLOC;
      goto fail;
    }
  }

  // 缩小日志级别设置
  ret = rcl_log_levels_shrink_to_size(&args_impl->log_levels);
  if (ret != RCL_RET_OK) {
    goto fail;
  }

  return RCL_RET_OK;

/**
 * @brief 处理失败情况并释放资源
 * @param[in] args_impl 参数实现结构体指针
 * @param[in] args_output 参数输出结构体指针
 * @param[in] ret 当前错误代码
 * @return 返回 fail_ret 错误代码
 */
fail:
  // 设置失败返回值
  fail_ret = ret;
  if (NULL != args_impl) {
    // 如果参数实现不为空，尝试清理参数输出
    ret = rcl_arguments_fini(args_output);
    if (RCL_RET_OK != ret) {
      // 如果清理失败，记录错误日志
      RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Failed to fini arguments after earlier failure");
    }
  }
  // 返回失败的错误代码
  return fail_ret;
}

/**
 * @brief 获取未解析参数的数量
 *
 * @param[in] args 指向 rcl_arguments_t 结构体的指针
 * @return int 返回未解析参数的数量，如果输入参数为 NULL，则返回 -1
 */
int rcl_arguments_get_count_unparsed(const rcl_arguments_t *args) {
  // 检查输入参数是否为 NULL
  if (NULL == args || NULL == args->impl) {
    return -1;
  }
  // 返回未解析参数的数量
  return args->impl->num_unparsed_args;
}

/**
 * @brief 获取未解析参数的索引列表
 *
 * @param[in] args 指向 rcl_arguments_t 结构体的指针
 * @param[in] allocator 分配器，用于分配内存
 * @param[out] output_unparsed_indices 输出未解析参数的索引列表
 * @return rcl_ret_t 返回操作结果，成功则返回 RCL_RET_OK
 */
rcl_ret_t rcl_arguments_get_unparsed(
    const rcl_arguments_t *args, rcl_allocator_t allocator, int **output_unparsed_indices) {
  // 检查输入参数是否为 NULL
  RCL_CHECK_ARGUMENT_FOR_NULL(args, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(args->impl, RCL_RET_INVALID_ARGUMENT);
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(&allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  // 检查输出参数是否为 NULL
  RCL_CHECK_ARGUMENT_FOR_NULL(output_unparsed_indices, RCL_RET_INVALID_ARGUMENT);

  // 初始化输出参数
  *output_unparsed_indices = NULL;
  // 如果存在未解析参数
  if (args->impl->num_unparsed_args) {
    // 分配内存空间
    *output_unparsed_indices =
        allocator.allocate(sizeof(int) * args->impl->num_unparsed_args, allocator.state);
    // 检查内存分配是否成功
    if (NULL == *output_unparsed_indices) {
      return RCL_RET_BAD_ALLOC;
    }
    // 将未解析参数的索引复制到输出参数中
    for (int i = 0; i < args->impl->num_unparsed_args; ++i) {
      (*output_unparsed_indices)[i] = args->impl->unparsed_args[i];
    }
  }
  // 返回操作结果
  return RCL_RET_OK;
}

/**
 * @brief 获取未解析的ROS参数数量
 *
 * @param[in] args 指向rcl_arguments_t结构体的指针
 * @return int 返回未解析的ROS参数数量，如果输入参数为NULL，则返回-1
 */
int rcl_arguments_get_count_unparsed_ros(const rcl_arguments_t *args) {
  // 检查输入参数是否为NULL
  if (NULL == args || NULL == args->impl) {
    return -1;
  }
  // 返回未解析的ROS参数数量
  return args->impl->num_unparsed_ros_args;
}

/**
 * @brief 获取未解析的ROS参数索引
 *
 * @param[in] args 指向rcl_arguments_t结构体的指针
 * @param[in] allocator 分配器
 * @param[out] output_unparsed_ros_indices 输出未解析的ROS参数索引数组
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_arguments_get_unparsed_ros(
    const rcl_arguments_t *args, rcl_allocator_t allocator, int **output_unparsed_ros_indices) {
  // 检查输入参数是否为NULL
  RCL_CHECK_ARGUMENT_FOR_NULL(args, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(args->impl, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ALLOCATOR_WITH_MSG(&allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(output_unparsed_ros_indices, RCL_RET_INVALID_ARGUMENT);

  // 初始化输出参数
  *output_unparsed_ros_indices = NULL;
  // 如果存在未解析的ROS参数
  if (args->impl->num_unparsed_ros_args) {
    // 为输出参数分配内存
    *output_unparsed_ros_indices =
        allocator.allocate(sizeof(int) * args->impl->num_unparsed_ros_args, allocator.state);
    // 检查内存分配是否成功
    if (NULL == *output_unparsed_ros_indices) {
      return RCL_RET_BAD_ALLOC;
    }
    // 将未解析的ROS参数索引复制到输出参数中
    for (int i = 0; i < args->impl->num_unparsed_ros_args; ++i) {
      (*output_unparsed_ros_indices)[i] = args->impl->unparsed_ros_args[i];
    }
  }
  // 返回成功状态
  return RCL_RET_OK;
}

/**
 * @brief 获取零初始化的rcl_arguments_t结构体
 *
 * @return rcl_arguments_t 返回零初始化的rcl_arguments_t结构体
 */
rcl_arguments_t rcl_get_zero_initialized_arguments(void) {
  // 定义并初始化一个默认的rcl_arguments_t结构体
  static rcl_arguments_t default_arguments = {.impl = NULL};
  // 返回默认的rcl_arguments_t结构体
  return default_arguments;
}

/**
 * @brief 移除 ROS 参数并返回非 ROS 参数列表
 *
 * @param[in] argv 命令行参数列表
 * @param[in] args rcl_arguments_t 结构体，包含已解析的 ROS 参数信息
 * @param[in] allocator 分配器，用于分配和释放内存
 * @param[out] nonros_argc 非 ROS 参数的数量
 * @param[out] nonros_argv 非 ROS 参数列表
 * @return rcl_ret_t 返回 RCL_RET_OK 表示成功，其他值表示失败
 */
rcl_ret_t rcl_remove_ros_arguments(
    const char *const *argv,
    const rcl_arguments_t *args,
    rcl_allocator_t allocator,
    int *nonros_argc,
    const char ***nonros_argv) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(args, RCL_RET_INVALID_ARGUMENT);
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(&allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  // 检查输出参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(nonros_argc, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(nonros_argv, RCL_RET_INVALID_ARGUMENT);

  // 检查 nonros_argv 是否已经被分配内存
  if (NULL != *nonros_argv) {
    RCL_SET_ERROR_MSG("Output nonros_argv pointer is not null. May leak memory.");
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 获取未解析的非 ROS 参数数量
  *nonros_argc = rcl_arguments_get_count_unparsed(args);
  if (*nonros_argc < 0) {
    RCL_SET_ERROR_MSG("Failed to get unparsed non ROS specific arguments count.");
    return RCL_RET_INVALID_ARGUMENT;
  } else if (*nonros_argc > 0) {
    // 检查输入参数是否为空
    RCL_CHECK_ARGUMENT_FOR_NULL(argv, RCL_RET_INVALID_ARGUMENT);
  }

  // 初始化非 ROS 参数列表
  *nonros_argv = NULL;
  if (0 == *nonros_argc) {
    return RCL_RET_OK;
  }

  // 获取未解析参数的索引
  int *unparsed_indices = NULL;
  rcl_ret_t ret = rcl_arguments_get_unparsed(args, allocator, &unparsed_indices);

  // 如果获取失败，返回错误代码
  if (RCL_RET_OK != ret) {
    return ret;
  }

  // 分配内存给非 ROS 参数列表
  size_t alloc_size = sizeof(char *) * *nonros_argc;
  *nonros_argv = allocator.allocate(alloc_size, allocator.state);
  if (NULL == *nonros_argv) {
    // 如果分配失败，释放内存并返回错误代码
    allocator.deallocate(unparsed_indices, allocator.state);
    return RCL_RET_BAD_ALLOC;
  }

  // 将未解析的非 ROS 参数添加到非 ROS 参数列表中
  for (int i = 0; i < *nonros_argc; ++i) {
    (*nonros_argv)[i] = argv[unparsed_indices[i]];
  }

  // 释放未解析参数索引的内存
  allocator.deallocate(unparsed_indices, allocator.state);
  return RCL_RET_OK;
}

/**
 * @brief 复制给定的 rcl_arguments_t 结构体，并将结果存储在 args_out 中。
 *
 * @param[in] args 指向要复制的 rcl_arguments_t 结构体的指针。
 * @param[out] args_out 指向用于存储复制结果的 rcl_arguments_t 结构体的指针。
 * @return 返回 RCL_RET_OK 表示成功，其他值表示失败。
 */
rcl_ret_t rcl_arguments_copy(const rcl_arguments_t *args, rcl_arguments_t *args_out) {
  // 检查输入参数是否有效并返回相应错误
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_BAD_ALLOC);

  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(args, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(args->impl, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(args_out, RCL_RET_INVALID_ARGUMENT);
  if (NULL != args_out->impl) {
    RCL_SET_ERROR_MSG("args_out must be zero initialized");
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 获取分配器
  rcl_allocator_t allocator = args->impl->allocator;

  // 分配并初始化 args_out 的内部实现
  rcl_ret_t ret = _rcl_allocate_initialized_arguments_impl(args_out, &allocator);
  if (RCL_RET_OK != ret) {
    return ret;
  }

  // 如果存在未解析的参数，则复制它们
  if (args->impl->num_unparsed_args) {
    args_out->impl->unparsed_args =
        allocator.allocate(sizeof(int) * args->impl->num_unparsed_args, allocator.state);
    if (NULL == args_out->impl->unparsed_args) {
      if (RCL_RET_OK != rcl_arguments_fini(args_out)) {
        RCL_SET_ERROR_MSG("Error while finalizing arguments due to another error");
      }
      return RCL_RET_BAD_ALLOC;
    }
    for (int i = 0; i < args->impl->num_unparsed_args; ++i) {
      args_out->impl->unparsed_args[i] = args->impl->unparsed_args[i];
    }
    args_out->impl->num_unparsed_args = args->impl->num_unparsed_args;
  }

  // 如果存在未解析的 ROS 参数，则复制它们
  if (args->impl->num_unparsed_ros_args) {
    args_out->impl->unparsed_ros_args =
        allocator.allocate(sizeof(int) * args->impl->num_unparsed_ros_args, allocator.state);
    if (NULL == args_out->impl->unparsed_ros_args) {
      if (RCL_RET_OK != rcl_arguments_fini(args_out)) {
        RCL_SET_ERROR_MSG("Error while finalizing arguments due to another error");
      }
      return RCL_RET_BAD_ALLOC;
    }
    for (int i = 0; i < args->impl->num_unparsed_ros_args; ++i) {
      args_out->impl->unparsed_ros_args[i] = args->impl->unparsed_ros_args[i];
    }
    args_out->impl->num_unparsed_ros_args = args->impl->num_unparsed_ros_args;
  }

  // 如果存在重映射规则，则复制它们
  if (args->impl->num_remap_rules) {
    args_out->impl->remap_rules =
        allocator.allocate(sizeof(rcl_remap_t) * args->impl->num_remap_rules, allocator.state);
    if (NULL == args_out->impl->remap_rules) {
      if (RCL_RET_OK != rcl_arguments_fini(args_out)) {
        RCL_SET_ERROR_MSG("Error while finalizing arguments due to another error");
      }
      return RCL_RET_BAD_ALLOC;
    }
    for (int i = 0; i < args->impl->num_remap_rules; ++i) {
      args_out->impl->remap_rules[i] = rcl_get_zero_initialized_remap();
      ret = rcl_remap_copy(&(args->impl->remap_rules[i]), &(args_out->impl->remap_rules[i]));
      if (RCL_RET_OK != ret) {
        if (RCL_RET_OK != rcl_arguments_fini(args_out)) {
          RCL_SET_ERROR_MSG("Error while finalizing arguments due to another error");
        }
        return ret;
      }
      ++(args_out->impl->num_remap_rules);
    }
  }

  // 复制参数规则
  if (args->impl->parameter_overrides) {
    args_out->impl->parameter_overrides =
        rcl_yaml_node_struct_copy(args->impl->parameter_overrides);
  }

  // 复制参数文件
  if (args->impl->num_param_files_args) {
    args_out->impl->parameter_files =
        allocator.zero_allocate(args->impl->num_param_files_args, sizeof(char *), allocator.state);
    if (NULL == args_out->impl->parameter_files) {
      if (RCL_RET_OK != rcl_arguments_fini(args_out)) {
        RCL_SET_ERROR_MSG("Error while finalizing arguments due to another error");
      }
      return RCL_RET_BAD_ALLOC;
    }
    for (int i = 0; i < args->impl->num_param_files_args; ++i) {
      args_out->impl->parameter_files[i] =
          rcutils_strdup(args->impl->parameter_files[i], allocator);
      if (NULL == args_out->impl->parameter_files[i]) {
        if (RCL_RET_OK != rcl_arguments_fini(args_out)) {
          RCL_SET_ERROR_MSG("Error while finalizing arguments due to another error");
        }
        return RCL_RET_BAD_ALLOC;
      }
      ++(args_out->impl->num_param_files_args);
    }
  }

  // 复制 enclave 参数
  char *enclave_copy = rcutils_strdup(args->impl->enclave, allocator);
  if (args->impl->enclave && !enclave_copy) {
    if (RCL_RET_OK != rcl_arguments_fini(args_out)) {
      RCL_SET_ERROR_MSG("Error while finalizing arguments due to another error");
    } else {
      RCL_SET_ERROR_MSG("Error while copying enclave argument");
    }
    return RCL_RET_BAD_ALLOC;
  }
  args_out->impl->enclave = enclave_copy;

  // 返回成功
  return RCL_RET_OK;
}

/**
 * @brief 释放rcl_arguments_t结构体中的资源
 *
 * @param[in,out] args 指向要释放的rcl_arguments_t结构体的指针
 * @return 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_arguments_fini(rcl_arguments_t *args) {
  // 检查输入参数是否为空，如果为空则返回错误码
  RCL_CHECK_ARGUMENT_FOR_NULL(args, RCL_RET_INVALID_ARGUMENT);

  // 如果args->impl不为空，则进行资源释放操作
  if (args->impl) {
    // 初始化返回值为RCL_RET_OK
    rcl_ret_t ret = RCL_RET_OK;

    // 如果存在重映射规则，则释放相关资源
    if (args->impl->remap_rules) {
      // 遍历所有重映射规则并逐个释放
      for (int i = 0; i < args->impl->num_remap_rules; ++i) {
        rcl_ret_t remap_ret = rcl_remap_fini(&(args->impl->remap_rules[i]));
        // 如果释放失败，则记录错误信息并继续处理其他资源
        if (remap_ret != RCL_RET_OK) {
          ret = remap_ret;
          RCUTILS_LOG_ERROR_NAMED(
              ROS_PACKAGE_NAME,
              "Failed to finalize remap rule while finalizing arguments. Continuing...");
        }
      }
      // 释放重映射规则数组
      args->impl->allocator.deallocate(args->impl->remap_rules, args->impl->allocator.state);
      args->impl->remap_rules = NULL;
      args->impl->num_remap_rules = 0;
    }

    // 释放日志级别资源
    rcl_ret_t log_levels_ret = rcl_log_levels_fini(&args->impl->log_levels);
    // 如果释放失败，则记录错误信息并继续处理其他资源
    if (log_levels_ret != RCL_RET_OK) {
      ret = log_levels_ret;
      RCUTILS_LOG_ERROR_NAMED(
          ROS_PACKAGE_NAME,
          "Failed to finalize log levels while finalizing arguments. Continuing...");
    }

    // 释放未解析参数数组
    args->impl->allocator.deallocate(args->impl->unparsed_args, args->impl->allocator.state);
    args->impl->num_unparsed_args = 0;
    args->impl->unparsed_args = NULL;

    // 释放未解析的ROS参数数组
    args->impl->allocator.deallocate(args->impl->unparsed_ros_args, args->impl->allocator.state);
    args->impl->num_unparsed_ros_args = 0;
    args->impl->unparsed_ros_args = NULL;

    // 如果存在参数覆盖，则释放相关资源
    if (args->impl->parameter_overrides) {
      rcl_yaml_node_struct_fini(args->impl->parameter_overrides);
      args->impl->parameter_overrides = NULL;
    }

    // 如果存在参数文件，则释放相关资源
    if (args->impl->parameter_files) {
      // 遍历所有参数文件并逐个释放
      for (int p = 0; p < args->impl->num_param_files_args; ++p) {
        args->impl->allocator.deallocate(
            args->impl->parameter_files[p], args->impl->allocator.state);
      }
      // 释放参数文件数组
      args->impl->allocator.deallocate(args->impl->parameter_files, args->impl->allocator.state);
      args->impl->num_param_files_args = 0;
      args->impl->parameter_files = NULL;
    }

    // 释放enclave资源
    args->impl->allocator.deallocate(args->impl->enclave, args->impl->allocator.state);

    // 如果存在外部日志配置文件，则释放相关资源
    if (NULL != args->impl->external_log_config_file) {
      args->impl->allocator.deallocate(
          args->impl->external_log_config_file, args->impl->allocator.state);
      args->impl->external_log_config_file = NULL;
    }

    // 释放args->impl资源
    args->impl->allocator.deallocate(args->impl, args->impl->allocator.state);
    args->impl = NULL;

    // 返回处理结果
    return ret;
  }

  // 如果args->impl为空，表示已经被释放过，返回错误信息
  RCL_SET_ERROR_MSG("rcl_arguments_t finalized twice");
  return RCL_RET_ERROR;
}

/// 解析一个完全限定的命名空间，用于命名空间替换规则（例如：`/foo/bar`）
/**
 * \param lex_lookahead 一个指向 rcl_lexer_lookahead2_t 结构体的指针
 * \return 返回 rcl_ret_t 类型的结果，表示解析操作的成功或失败
 * \sa _rcl_parse_remap_begin_remap_rule()
 */
RCL_LOCAL
rcl_ret_t _rcl_parse_remap_fully_qualified_namespace(rcl_lexer_lookahead2_t *lex_lookahead) {
  rcl_ret_t ret;

  // 检查参数的合理性
  assert(NULL != lex_lookahead);

  // 至少有一个正斜杠 /
  ret = rcl_lexer_lookahead2_expect(lex_lookahead, RCL_LEXEME_FORWARD_SLASH, NULL, NULL);
  if (RCL_RET_WRONG_LEXEME == ret) {
    return RCL_RET_INVALID_REMAP_RULE;
  }

  // 重复的 tokens 和斜杠（允许尾部斜杠，但不强制要求）
  while (true) {
    ret = rcl_lexer_lookahead2_expect(lex_lookahead, RCL_LEXEME_TOKEN, NULL, NULL);
    if (RCL_RET_WRONG_LEXEME == ret) {
      rcl_reset_error();
      break;
    }
    ret = rcl_lexer_lookahead2_expect(lex_lookahead, RCL_LEXEME_FORWARD_SLASH, NULL, NULL);
    if (RCL_RET_WRONG_LEXEME == ret) {
      rcl_reset_error();
      break;
    }
  }
  return RCL_RET_OK;
}

/// 解析一个 token 或者一个反向引用（例如：`bar`，或者 `\7`）。
/**
 * \sa _rcl_parse_remap_begin_remap_rule()
 * \param lex_lookahead 一个指向 rcl_lexer_lookahead2_t 结构体的指针
 * \return 返回 rcl_ret_t 类型的结果，表示解析操作是否成功
 */
RCL_LOCAL
rcl_ret_t _rcl_parse_remap_replacement_token(rcl_lexer_lookahead2_t *lex_lookahead) {
  rcl_ret_t ret;
  rcl_lexeme_t lexeme;

  // 检查参数的合法性
  assert(NULL != lex_lookahead);

  // 预览下一个词素
  ret = rcl_lexer_lookahead2_peek(lex_lookahead, &lexeme);
  if (RCL_RET_OK != ret) {
    return ret;
  }

  // 判断词素是否为反向引用
  if (RCL_LEXEME_BR1 == lexeme || RCL_LEXEME_BR2 == lexeme || RCL_LEXEME_BR3 == lexeme ||
      RCL_LEXEME_BR4 == lexeme || RCL_LEXEME_BR5 == lexeme || RCL_LEXEME_BR6 == lexeme ||
      RCL_LEXEME_BR7 == lexeme || RCL_LEXEME_BR8 == lexeme || RCL_LEXEME_BR9 == lexeme) {
    RCL_SET_ERROR_MSG("Backreferences are not implemented");
    return RCL_RET_ERROR;
  } else if (RCL_LEXEME_TOKEN == lexeme) {  // 判断词素是否为 token
    // 接受当前词素
    ret = rcl_lexer_lookahead2_accept(lex_lookahead, NULL, NULL);
  } else {
    // 其他情况，返回无效的重映射规则错误
    ret = RCL_RET_INVALID_REMAP_RULE;
  }

  return ret;
}

/// 解析名称重映射规则的替换部分（例如：`bar/\1/foo`）。
/**
 * \sa _rcl_parse_remap_begin_remap_rule()
 */
RCL_LOCAL
rcl_ret_t _rcl_parse_remap_replacement_name(
    rcl_lexer_lookahead2_t *lex_lookahead,  ///< [in] 词法分析器 lookahead 结构体指针
    rcl_remap_t *rule)                      ///< [in,out] 重映射规则结构体指针
{
  rcl_ret_t ret;
  rcl_lexeme_t lexeme;

  // 检查参数合理性
  assert(NULL != lex_lookahead);
  assert(NULL != rule);

  // 获取替换起始位置
  const char *replacement_start = rcl_lexer_lookahead2_get_text(lex_lookahead);
  if (NULL == replacement_start) {
    RCL_SET_ERROR_MSG("failed to get start of replacement");
    return RCL_RET_ERROR;
  }

  // 私有名称（~/...）还是完全限定名称（/...）？
  ret = rcl_lexer_lookahead2_peek(lex_lookahead, &lexeme);
  if (RCL_RET_OK != ret) {
    return ret;
  }
  if (RCL_LEXEME_TILDE_SLASH == lexeme || RCL_LEXEME_FORWARD_SLASH == lexeme) {
    ret = rcl_lexer_lookahead2_accept(lex_lookahead, NULL, NULL);
  }
  if (RCL_RET_OK != ret) {
    return ret;
  }

  // token ( '/' token )*
  ret = _rcl_parse_remap_replacement_token(lex_lookahead);
  if (RCL_RET_OK != ret) {
    return ret;
  }
  ret = rcl_lexer_lookahead2_peek(lex_lookahead, &lexeme);
  if (RCL_RET_OK != ret) {
    return ret;
  }
  // 当不是文件结束符时，继续解析
  while (RCL_LEXEME_EOF != lexeme) {
    ret = rcl_lexer_lookahead2_expect(lex_lookahead, RCL_LEXEME_FORWARD_SLASH, NULL, NULL);
    if (RCL_RET_WRONG_LEXEME == ret) {
      return RCL_RET_INVALID_REMAP_RULE;
    }
    ret = _rcl_parse_remap_replacement_token(lex_lookahead);
    if (RCL_RET_OK != ret) {
      return ret;
    }
    ret = rcl_lexer_lookahead2_peek(lex_lookahead, &lexeme);
    if (RCL_RET_OK != ret) {
      return ret;
    }
  }

  // 将替换内容复制到规则中
  const char *replacement_end = rcl_lexer_lookahead2_get_text(lex_lookahead);
  size_t length = (size_t)(replacement_end - replacement_start);
  rule->impl->replacement = rcutils_strndup(replacement_start, length, rule->impl->allocator);
  if (NULL == rule->impl->replacement) {
    RCL_SET_ERROR_MSG("failed to copy replacement");
    return RCL_RET_BAD_ALLOC;
  }

  return RCL_RET_OK;
}

/// 解析资源名称标记或通配符（例如：`foobar`，`*`，`**`）。
/**
 * \sa _rcl_parse_resource_match()
 * \param[in] lex_lookahead 一个指向 rcl_lexer_lookahead2_t 结构体的指针
 * \return 返回 rcl_ret_t 类型的结果，表示解析操作是否成功
 */
RCL_LOCAL
rcl_ret_t _rcl_parse_resource_match_token(rcl_lexer_lookahead2_t *lex_lookahead) {
  rcl_ret_t ret;
  rcl_lexeme_t lexeme;

  // 检查参数的合理性
  assert(NULL != lex_lookahead);

  // 预览下一个词素
  ret = rcl_lexer_lookahead2_peek(lex_lookahead, &lexeme);
  if (RCL_RET_OK != ret) {
    return ret;
  }

  // 判断词素类型
  if (RCL_LEXEME_TOKEN == lexeme) {
    // 接受当前词素并前进
    ret = rcl_lexer_lookahead2_accept(lex_lookahead, NULL, NULL);
  } else if (RCL_LEXEME_WILD_ONE == lexeme) {
    // 设置错误消息，表示单字符通配符 '*' 尚未实现
    RCL_SET_ERROR_MSG("Wildcard '*' is not implemented");
    return RCL_RET_ERROR;
  } else if (RCL_LEXEME_WILD_MULTI == lexeme) {
    // 设置错误消息，表示多字符通配符 '**' 尚未实现
    RCL_SET_ERROR_MSG("Wildcard '**' is not implemented");
    return RCL_RET_ERROR;
  } else {
    // 设置错误消息，表示期望的词素类型为标记或通配符
    RCL_SET_ERROR_MSG("Expecting token or wildcard");
    ret = RCL_RET_WRONG_LEXEME;
  }

  return ret;
}

/// 解析规则中的资源名称匹配部分（例如：`rostopic://foo`）
/**
 * \sa _rcl_parse_remap_match_name()
 *
 * @param[in] lex_lookahead 词法分析器，用于解析输入文本
 * @param[in] allocator 分配器，用于分配内存
 * @param[out] resource_match 输出参数，用于存储解析后的资源名称匹配结果
 * @return 返回 rcl_ret_t 类型的状态码，表示函数执行结果
 */
RCL_LOCAL
rcl_ret_t _rcl_parse_resource_match(
    rcl_lexer_lookahead2_t *lex_lookahead, rcl_allocator_t allocator, char **resource_match) {
  rcl_ret_t ret;
  rcl_lexeme_t lexeme;

  // 检查参数的合理性
  assert(NULL != lex_lookahead);
  assert(rcutils_allocator_is_valid(&allocator));
  assert(NULL != resource_match);
  assert(NULL == *resource_match);

  const char *match_start = rcl_lexer_lookahead2_get_text(lex_lookahead);
  if (NULL == match_start) {
    RCL_SET_ERROR_MSG("failed to get start of match");
    return RCL_RET_ERROR;
  }

  // 判断是否为私有名称（~/...）或完全限定名称（/...）
  ret = rcl_lexer_lookahead2_peek(lex_lookahead, &lexeme);
  if (RCL_RET_OK != ret) {
    return ret;
  }
  if (RCL_LEXEME_TILDE_SLASH == lexeme || RCL_LEXEME_FORWARD_SLASH == lexeme) {
    ret = rcl_lexer_lookahead2_accept(lex_lookahead, NULL, NULL);
    if (RCL_RET_OK != ret) {
      return ret;
    }
  }

  // 解析 token（'/' token）*
  ret = _rcl_parse_resource_match_token(lex_lookahead);
  if (RCL_RET_OK != ret) {
    return ret;
  }
  ret = rcl_lexer_lookahead2_peek(lex_lookahead, &lexeme);
  if (RCL_RET_OK != ret) {
    return ret;
  }
  while (RCL_LEXEME_SEPARATOR != lexeme) {
    ret = rcl_lexer_lookahead2_expect(lex_lookahead, RCL_LEXEME_FORWARD_SLASH, NULL, NULL);
    if (RCL_RET_WRONG_LEXEME == ret) {
      return RCL_RET_INVALID_REMAP_RULE;
    }
    ret = _rcl_parse_resource_match_token(lex_lookahead);
    if (RCL_RET_OK != ret) {
      return ret;
    }
    ret = rcl_lexer_lookahead2_peek(lex_lookahead, &lexeme);
    if (RCL_RET_OK != ret) {
      return ret;
    }
  }

  // 将匹配结果复制到规则中
  const char *match_end = rcl_lexer_lookahead2_get_text(lex_lookahead);
  const size_t length = (size_t)(match_end - match_start);
  *resource_match = rcutils_strndup(match_start, length, allocator);
  if (NULL == *resource_match) {
    RCL_SET_ERROR_MSG("failed to copy match");
    return RCL_RET_BAD_ALLOC;
  }

  return RCL_RET_OK;
}

/**
 * @brief 解析参数名称的 token
 *
 * @param[in] lex_lookahead 一个指向 rcl_lexer_lookahead2_t 结构体的指针
 * @return rcl_ret_t 返回 RCL_RET_OK 表示成功，其他值表示失败
 *
 * 解析参数名称的 token，并检查是否包含通配符。如果遇到未实现的通配符，将返回错误。
 */
RCL_LOCAL
rcl_ret_t _rcl_parse_param_name_token(rcl_lexer_lookahead2_t *lex_lookahead) {
  rcl_ret_t ret;
  rcl_lexeme_t lexeme;

  // 检查参数的合法性
  assert(NULL != lex_lookahead);

  // 获取下一个词素
  ret = rcl_lexer_lookahead2_peek(lex_lookahead, &lexeme);
  if (RCL_RET_OK != ret) {
    return ret;
  }
  // 检查词素是否为 TOKEN 或 FORWARD_SLASH
  if (RCL_LEXEME_TOKEN != lexeme && RCL_LEXEME_FORWARD_SLASH != lexeme) {
    // 检查词素是否为 WILD_ONE 或 WILD_MULTI
    if (RCL_LEXEME_WILD_ONE == lexeme) {
      RCL_SET_ERROR_MSG("Wildcard '*' is not implemented");
      return RCL_RET_ERROR;
    } else if (RCL_LEXEME_WILD_MULTI == lexeme) {
      RCL_SET_ERROR_MSG("Wildcard '**' is not implemented");
      return RCL_RET_ERROR;
    } else {
      RCL_SET_ERROR_MSG("Expecting token or wildcard");
      return RCL_RET_WRONG_LEXEME;
    }
  }
  // 循环处理 TOKEN 和 FORWARD_SLASH
  do {
    ret = rcl_lexer_lookahead2_accept(lex_lookahead, NULL, NULL);
    if (RCL_RET_OK != ret) {
      return ret;
    }
    ret = rcl_lexer_lookahead2_peek(lex_lookahead, &lexeme);
    if (RCL_RET_OK != ret) {
      return ret;
    }
  } while (RCL_LEXEME_TOKEN == lexeme || RCL_LEXEME_FORWARD_SLASH == lexeme);
  return RCL_RET_OK;
}

/// 解析参数覆盖规则中的参数名称（例如：`foo.bar`）
/**
 * \sa _rcl_parse_param_rule()
 */
// TODO(hidmic): 当参数名称标准化为使用斜杠代替点时删除
//               。
RCL_LOCAL
rcl_ret_t _rcl_parse_param_name(
    rcl_lexer_lookahead2_t *lex_lookahead,  ///< 输入参数，指向词法分析器的指针
    rcl_allocator_t allocator,              ///< 输入参数，内存分配器
    char **param_name)                      ///< 输出参数，解析后的参数名称
{
  rcl_ret_t ret;
  rcl_lexeme_t lexeme;

  // 检查参数合理性
  assert(NULL != lex_lookahead);
  assert(rcutils_allocator_is_valid(&allocator));
  assert(NULL != param_name);
  assert(NULL == *param_name);

  const char *name_start = rcl_lexer_lookahead2_get_text(lex_lookahead);
  if (NULL == name_start) {
    RCL_SET_ERROR_MSG("failed to get start of param name");
    return RCL_RET_ERROR;
  }

  // token ( '.' token )*
  ret = _rcl_parse_param_name_token(lex_lookahead);
  if (RCL_RET_OK != ret) {
    return ret;
  }
  ret = rcl_lexer_lookahead2_peek(lex_lookahead, &lexeme);
  if (RCL_RET_OK != ret) {
    return ret;
  }
  while (RCL_LEXEME_SEPARATOR != lexeme) {
    ret = rcl_lexer_lookahead2_expect(lex_lookahead, RCL_LEXEME_DOT, NULL, NULL);
    if (RCL_RET_WRONG_LEXEME == ret) {
      return RCL_RET_INVALID_REMAP_RULE;
    }
    ret = _rcl_parse_param_name_token(lex_lookahead);
    if (RCL_RET_OK != ret) {
      return ret;
    }
    ret = rcl_lexer_lookahead2_peek(lex_lookahead, &lexeme);
    if (RCL_RET_OK != ret) {
      return ret;
    }
  }

  // 复制参数名称
  const char *name_end = rcl_lexer_lookahead2_get_text(lex_lookahead);
  const size_t length = (size_t)(name_end - name_start);
  *param_name = rcutils_strndup(name_start, length, allocator);
  if (NULL == *param_name) {
    RCL_SET_ERROR_MSG("failed to copy param name");
    return RCL_RET_BAD_ALLOC;
  }

  return RCL_RET_OK;
}

/// 解析名称重映射规则的匹配部分（例如：`rostopic://foo`）
/**
 * \param[in] lex_lookahead 一个指向 rcl_lexer_lookahead2_t 结构体的指针，用于解析输入字符串
 * \param[out] rule 一个指向 rcl_remap_t 结构体的指针，用于存储解析结果
 * \return 返回 rcl_ret_t 类型的状态码，表示函数执行成功或失败
 * \sa _rcl_parse_remap_begin_remap_rule()
 */
RCL_LOCAL
rcl_ret_t _rcl_parse_remap_match_name(rcl_lexer_lookahead2_t *lex_lookahead, rcl_remap_t *rule) {
  rcl_ret_t ret;
  rcl_lexeme_t lexeme;

  // 检查参数的合法性
  assert(NULL != lex_lookahead);
  assert(NULL != rule);

  // rostopic:// rosservice://
  ret = rcl_lexer_lookahead2_peek(lex_lookahead, &lexeme);
  if (RCL_RET_OK != ret) {
    return ret;
  }
  // 判断是服务还是主题
  if (RCL_LEXEME_URL_SERVICE == lexeme) {
    rule->impl->type = RCL_SERVICE_REMAP;
    ret = rcl_lexer_lookahead2_accept(lex_lookahead, NULL, NULL);
  } else if (RCL_LEXEME_URL_TOPIC == lexeme) {
    rule->impl->type = RCL_TOPIC_REMAP;
    ret = rcl_lexer_lookahead2_accept(lex_lookahead, NULL, NULL);
  } else {
    rule->impl->type = (RCL_TOPIC_REMAP | RCL_SERVICE_REMAP);
  }
  if (RCL_RET_OK != ret) {
    return ret;
  }

  // 解析资源匹配
  ret = _rcl_parse_resource_match(lex_lookahead, rule->impl->allocator, &rule->impl->match);
  if (RCL_RET_WRONG_LEXEME == ret) {
    ret = RCL_RET_INVALID_REMAP_RULE;
  }
  return ret;
}

/// 解析名称重映射规则（例如：`rostopic:///foo:=bar`）
/**
 * \param[in] lex_lookahead 一个指向 rcl_lexer_lookahead2_t 结构体的指针，用于解析输入字符串
 * \param[out] rule 一个指向 rcl_remap_t 结构体的指针，用于存储解析结果
 * \return 返回 rcl_ret_t 类型的状态码，表示函数执行成功或失败
 * \sa _rcl_parse_remap_begin_remap_rule()
 */
RCL_LOCAL
rcl_ret_t _rcl_parse_remap_name_remap(rcl_lexer_lookahead2_t *lex_lookahead, rcl_remap_t *rule) {
  rcl_ret_t ret;

  // 检查参数的合法性
  assert(NULL != lex_lookahead);
  assert(NULL != rule);

  // 解析匹配名称
  ret = _rcl_parse_remap_match_name(lex_lookahead, rule);
  if (RCL_RET_OK != ret) {
    return ret;
  }
  // 解析分隔符 :=
  ret = rcl_lexer_lookahead2_expect(lex_lookahead, RCL_LEXEME_SEPARATOR, NULL, NULL);
  if (RCL_RET_WRONG_LEXEME == ret) {
    return RCL_RET_INVALID_REMAP_RULE;
  }
  // 解析替换名称
  ret = _rcl_parse_remap_replacement_name(lex_lookahead, rule);
  if (RCL_RET_OK != ret) {
    return ret;
  }

  return RCL_RET_OK;
}

/// 解析命名空间替换规则（例如：`__ns:=/new/ns`）
/**
 * \param[in] lex_lookahead 一个指向 rcl_lexer_lookahead2_t 结构体的指针，用于解析输入字符串
 * \param[out] rule 一个指向 rcl_remap_t 结构体的指针，用于存储解析结果
 * \return 返回 rcl_ret_t 类型的状态码，表示函数执行成功或失败
 * \sa _rcl_parse_remap_begin_remap_rule()
 */
RCL_LOCAL
rcl_ret_t _rcl_parse_remap_namespace_replacement(
    rcl_lexer_lookahead2_t *lex_lookahead, rcl_remap_t *rule) {
  rcl_ret_t ret;

  // 检查参数的合法性
  assert(NULL != lex_lookahead);
  assert(NULL != rule);

  // 解析 __ns
  ret = rcl_lexer_lookahead2_expect(lex_lookahead, RCL_LEXEME_NS, NULL, NULL);
  if (RCL_RET_WRONG_LEXEME == ret) {
    return RCL_RET_INVALID_REMAP_RULE;
  }
  // 解析分隔符 :=
  ret = rcl_lexer_lookahead2_expect(lex_lookahead, RCL_LEXEME_SEPARATOR, NULL, NULL);
  if (RCL_RET_WRONG_LEXEME == ret) {
    return RCL_RET_INVALID_REMAP_RULE;
  }
  // 解析命名空间 /foo/bar
  const char *ns_start = rcl_lexer_lookahead2_get_text(lex_lookahead);
  if (NULL == ns_start) {
    RCL_SET_ERROR_MSG("failed to get start of namespace");
    return RCL_RET_ERROR;
  }
  ret = _rcl_parse_remap_fully_qualified_namespace(lex_lookahead);
  if (RCL_RET_OK != ret) {
    if (RCL_RET_INVALID_REMAP_RULE == ret) {
      // 名称没有以前导斜杠开始
      RCUTILS_LOG_WARN_NAMED(
          ROS_PACKAGE_NAME, "Namespace not remapped to a fully qualified name (found: %s)",
          ns_start);
    }
    return ret;
  }
  // 应该没有剩余内容
  ret = rcl_lexer_lookahead2_expect(lex_lookahead, RCL_LEXEME_EOF, NULL, NULL);
  if (RCL_RET_OK != ret) {
    // 名称必须以前导斜杠开始，但格式可能无效
    RCUTILS_LOG_WARN_NAMED(
        ROS_PACKAGE_NAME, "Namespace not remapped to a fully qualified name (found: %s)", ns_start);
    return ret;
  }

  // 将命名空间复制到规则中
  const char *ns_end = rcl_lexer_lookahead2_get_text(lex_lookahead);
  size_t length = (size_t)(ns_end - ns_start);
  rule->impl->replacement = rcutils_strndup(ns_start, length, rule->impl->allocator);
  if (NULL == rule->impl->replacement) {
    RCL_SET_ERROR_MSG("failed to copy namespace");
    return RCL_RET_BAD_ALLOC;
  }

  rule->impl->type = RCL_NAMESPACE_REMAP;
  return RCL_RET_OK;
}

/// 解析节点名替换规则（例如：`__node:=new_name` 或 `__name:=new_name`）。
/**
 * \sa _rcl_parse_remap_begin_remap_rule()
 * \param[in] lex_lookahead 词法分析器，用于解析输入的字符串
 * \param[out] rule 存储解析后的重映射规则
 * \return 返回 rcl_ret_t 类型的结果，表示解析过程是否成功
 */
RCL_LOCAL
rcl_ret_t _rcl_parse_remap_nodename_replacement(
    rcl_lexer_lookahead2_t *lex_lookahead, rcl_remap_t *rule) {
  rcl_ret_t ret;
  const char *node_name;
  size_t length;

  // 检查参数的合理性
  assert(NULL != lex_lookahead);
  assert(NULL != rule);

  // __node
  // 预期下一个词素是 RCL_LEXEME_NODE
  ret = rcl_lexer_lookahead2_expect(lex_lookahead, RCL_LEXEME_NODE, NULL, NULL);
  if (RCL_RET_WRONG_LEXEME == ret) {
    return RCL_RET_INVALID_REMAP_RULE;
  }
  // :=
  // 预期下一个词素是 RCL_LEXEME_SEPARATOR
  ret = rcl_lexer_lookahead2_expect(lex_lookahead, RCL_LEXEME_SEPARATOR, NULL, NULL);
  if (RCL_RET_WRONG_LEXEME == ret) {
    return RCL_RET_INVALID_REMAP_RULE;
  }
  // new_node_name
  // 预期下一个词素是 RCL_LEXEME_TOKEN，并获取节点名及其长度
  ret = rcl_lexer_lookahead2_expect(lex_lookahead, RCL_LEXEME_TOKEN, &node_name, &length);
  if (RCL_RET_WRONG_LEXEME == ret) {
    node_name = rcl_lexer_lookahead2_get_text(lex_lookahead);
    // 记录无效节点名的警告日志
    RCUTILS_LOG_WARN_NAMED(
        ROS_PACKAGE_NAME, "Node name not remapped to invalid name: '%s'", node_name);
    return RCL_RET_INVALID_REMAP_RULE;
  }
  if (RCL_RET_OK != ret) {
    return ret;
  }
  // 将节点名复制到规则的替换部分
  rule->impl->replacement = rcutils_strndup(node_name, length, rule->impl->allocator);
  if (NULL == rule->impl->replacement) {
    RCL_SET_ERROR_MSG("failed to allocate node name");
    return RCL_RET_BAD_ALLOC;
  }

  // 设置规则类型为 RCL_NODENAME_REMAP
  rule->impl->type = RCL_NODENAME_REMAP;
  return RCL_RET_OK;
}

/// 解析包含尾随冒号的节点名前缀（例如：`node_name:`）。
/**
 * \param[in] lex_lookahead 词法分析器 lookahead2 的指针
 * \param[in] allocator 分配器
 * \param[out] node_name 存储解析出的节点名的指针
 * \return 返回 rcl_ret_t 类型的结果，成功返回 RCL_RET_OK
 */
RCL_LOCAL
rcl_ret_t _rcl_parse_nodename_prefix(
    rcl_lexer_lookahead2_t *lex_lookahead, rcl_allocator_t allocator, char **node_name) {
  size_t length = 0;
  const char *token = NULL;

  // 检查参数的合理性
  assert(NULL != lex_lookahead);
  assert(rcutils_allocator_is_valid(&allocator));
  assert(NULL != node_name);
  assert(NULL == *node_name);

  // 预期一个标记和一个冒号
  rcl_ret_t ret = rcl_lexer_lookahead2_expect(lex_lookahead, RCL_LEXEME_TOKEN, &token, &length);
  if (RCL_RET_OK != ret) {
    return ret;
  }
  ret = rcl_lexer_lookahead2_expect(lex_lookahead, RCL_LEXEME_COLON, NULL, NULL);
  if (RCL_RET_OK != ret) {
    return ret;
  }

  // 复制节点名
  *node_name = rcutils_strndup(token, length, allocator);
  if (NULL == *node_name) {
    RCL_SET_ERROR_MSG("failed to allocate node name");
    return RCL_RET_BAD_ALLOC;
  }

  return RCL_RET_OK;
}

/// 为重映射规则解析节点名前缀。
/**
 * \sa _rcl_parse_nodename_prefix()
 * \sa _rcl_parse_remap_begin_remap_rule()
 * \param[in] lex_lookahead 词法分析器 lookahead2 的指针
 * \param[out] rule 存储解析出的重映射规则的指针
 * \return 返回 rcl_ret_t 类型的结果，成功返回 RCL_RET_OK
 */
RCL_LOCAL
rcl_ret_t _rcl_parse_remap_nodename_prefix(
    rcl_lexer_lookahead2_t *lex_lookahead, rcl_remap_t *rule) {
  // 检查参数的合理性
  assert(NULL != lex_lookahead);
  assert(NULL != rule);

  rcl_ret_t ret =
      _rcl_parse_nodename_prefix(lex_lookahead, rule->impl->allocator, &rule->impl->node_name);
  if (RCL_RET_WRONG_LEXEME == ret) {
    ret = RCL_RET_INVALID_REMAP_RULE;
  }
  return ret;
}

/// 开始递归下降解析 remap 规则。
/**
 * \param[in] lex_lookahead 供解析器使用的 lookahead(2) 缓冲区。
 * \param[in,out] rule 输入一个零初始化的规则，输出一个完全初始化的规则。
 * \return RCL_RET_OK 如果解析了有效的规则，或者
 * \return RCL_RET_INVALID_REMAP_RULE 如果参数不是有效的规则，或者
 * \return RCL_RET_BAD_ALLOC 如果分配失败，或者
 * \return RLC_RET_ERROR 如果发生未指定的错误。
 */
RCL_LOCAL
rcl_ret_t _rcl_parse_remap_begin_remap_rule(
    rcl_lexer_lookahead2_t *lex_lookahead, rcl_remap_t *rule) {
  rcl_ret_t ret;
  rcl_lexeme_t lexeme1;
  rcl_lexeme_t lexeme2;

  // 检查参数的合理性
  assert(NULL != lex_lookahead);
  assert(NULL != rule);

  // 检查可选的节点名前缀
  ret = rcl_lexer_lookahead2_peek2(lex_lookahead, &lexeme1, &lexeme2);
  if (RCL_RET_OK != ret) {
    return ret;
  }
  if (RCL_LEXEME_TOKEN == lexeme1 && RCL_LEXEME_COLON == lexeme2) {
    ret = _rcl_parse_remap_nodename_prefix(lex_lookahead, rule);
    if (RCL_RET_OK != ret) {
      return ret;
    }
  }

  ret = rcl_lexer_lookahead2_peek(lex_lookahead, &lexeme1);
  if (RCL_RET_OK != ret) {
    return ret;
  }

  // 这是什么类型的规则（节点名替换、命名空间替换还是名称 remap）？
  if (RCL_LEXEME_NODE == lexeme1) {
    ret = _rcl_parse_remap_nodename_replacement(lex_lookahead, rule);
    if (RCL_RET_OK != ret) {
      return ret;
    }
  } else if (RCL_LEXEME_NS == lexeme1) {
    ret = _rcl_parse_remap_namespace_replacement(lex_lookahead, rule);
    if (RCL_RET_OK != ret) {
      return ret;
    }
  } else {
    ret = _rcl_parse_remap_name_remap(lex_lookahead, rule);
    if (RCL_RET_OK != ret) {
      return ret;
    }
  }

  // 确保字符串中的所有字符都已被消耗
  ret = rcl_lexer_lookahead2_expect(lex_lookahead, RCL_LEXEME_EOF, NULL, NULL);
  if (RCL_RET_WRONG_LEXEME == ret) {
    return RCL_RET_INVALID_REMAP_RULE;
  }
  return ret;
}

/**
 * @brief 解析日志级别名称
 *
 * @param[in] lex_lookahead 词法分析器lookahead2对象的指针
 * @param[in] allocator 分配器的指针
 * @param[out] logger_name 解析出的日志记录器名称的指针
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
RCL_LOCAL
rcl_ret_t _rcl_parse_log_level_name(
    rcl_lexer_lookahead2_t *lex_lookahead, rcl_allocator_t *allocator, char **logger_name) {
  rcl_lexeme_t lexeme;

  // 检查参数合法性
  assert(NULL != lex_lookahead);
  assert(rcutils_allocator_is_valid(allocator));
  assert(NULL != logger_name);
  assert(NULL == *logger_name);

  // 获取名称起始位置
  const char *name_start = rcl_lexer_lookahead2_get_text(lex_lookahead);
  if (NULL == name_start) {
    RCL_SET_ERROR_MSG("failed to get start of logger name");
    return RCL_RET_ERROR;
  }

  // 预览下一个词素
  rcl_ret_t ret = rcl_lexer_lookahead2_peek(lex_lookahead, &lexeme);
  if (RCL_RET_OK != ret) {
    return ret;
  }

  // 当词素不是分隔符时，继续处理
  while (RCL_LEXEME_SEPARATOR != lexeme) {
    ret = rcl_lexer_lookahead2_expect(lex_lookahead, lexeme, NULL, NULL);
    if (RCL_RET_OK != ret) {
      return ret;
    }

    // 预览下一个词素
    ret = rcl_lexer_lookahead2_peek(lex_lookahead, &lexeme);
    if (RCL_RET_OK != ret) {
      return ret;
    }

    // 如果词素是文件结束符，返回无效的日志级别规则错误
    if (lexeme == RCL_LEXEME_EOF) {
      ret = RCL_RET_INVALID_LOG_LEVEL_RULE;
      return ret;
    }
  }

  // 复制日志记录器名称
  const char *name_end = rcl_lexer_lookahead2_get_text(lex_lookahead);
  const size_t length = (size_t)(name_end - name_start);
  *logger_name = rcutils_strndup(name_start, length, *allocator);
  if (NULL == *logger_name) {
    RCL_SET_ERROR_MSG("failed to copy logger name");
    return RCL_RET_BAD_ALLOC;
  }

  return RCL_RET_OK;
}

/**
 * @brief 解析日志级别参数并设置相应的日志级别
 *
 * @param[in] arg 输入的命令行参数字符串
 * @param[out] log_levels 用于存储解析后的日志级别设置
 * @return 返回 rcl_ret_t 类型的结果，成功返回 RCL_RET_OK，否则返回相应的错误代码
 */
rcl_ret_t _rcl_parse_log_level(const char *arg, rcl_log_levels_t *log_levels) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(arg, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(log_levels, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(log_levels->logger_settings, RCL_RET_INVALID_ARGUMENT);

  // 获取分配器
  rcl_allocator_t *allocator = &log_levels->allocator;
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);

  rcl_ret_t ret = RCL_RET_OK;
  char *logger_name = NULL;
  int level = 0;
  rcutils_ret_t rcutils_ret = RCUTILS_RET_OK;

  // 初始化词法分析器
  rcl_lexer_lookahead2_t lex_lookahead = rcl_get_zero_initialized_lexer_lookahead2();

  ret = rcl_lexer_lookahead2_init(&lex_lookahead, arg, *allocator);
  if (RCL_RET_OK != ret) {
    return ret;
  }

  // 解析日志级别名称
  ret = _rcl_parse_log_level_name(&lex_lookahead, allocator, &logger_name);
  if (RCL_RET_OK == ret) {
    // 检查日志级别名称是否为空
    if (strlen(logger_name) == 0) {
      RCL_SET_ERROR_MSG("Argument has an invalid logger item that name is empty");
      ret = RCL_RET_INVALID_LOG_LEVEL_RULE;
      goto cleanup;
    }

    // 检查分隔符
    ret = rcl_lexer_lookahead2_expect(&lex_lookahead, RCL_LEXEME_SEPARATOR, NULL, NULL);
    if (RCL_RET_WRONG_LEXEME == ret) {
      ret = RCL_RET_INVALID_LOG_LEVEL_RULE;
      goto cleanup;
    }

    // 解析日志级别值
    const char *level_token;
    size_t level_token_length;
    ret = rcl_lexer_lookahead2_expect(
        &lex_lookahead, RCL_LEXEME_TOKEN, &level_token, &level_token_length);
    if (RCL_RET_WRONG_LEXEME == ret) {
      ret = RCL_RET_INVALID_LOG_LEVEL_RULE;
      goto cleanup;
    }

    // 检查词法分析器是否到达末尾
    ret = rcl_lexer_lookahead2_expect(&lex_lookahead, RCL_LEXEME_EOF, NULL, NULL);
    if (RCL_RET_OK != ret) {
      ret = RCL_RET_INVALID_LOG_LEVEL_RULE;
      goto cleanup;
    }

    // 将解析出的日志级别字符串转换为整数
    rcutils_ret = rcutils_logging_severity_level_from_string(level_token, *allocator, &level);
    if (RCUTILS_RET_OK == rcutils_ret) {
      // 添加日志设置
      ret = rcl_log_levels_add_logger_setting(log_levels, logger_name, (rcl_log_severity_t)level);
      if (ret != RCL_RET_OK) {
        goto cleanup;
      }
    }
  } else {
    // 将解析出的日志级别字符串转换为整数
    rcutils_ret = rcutils_logging_severity_level_from_string(arg, *allocator, &level);
    if (RCUTILS_RET_OK == rcutils_ret) {
      // 设置默认日志级别
      if (log_levels->default_logger_level != (rcl_log_severity_t)level) {
        if (log_levels->default_logger_level != RCUTILS_LOG_SEVERITY_UNSET) {
          RCUTILS_LOG_DEBUG_NAMED(
              ROS_PACKAGE_NAME, "Minimum default log level will be replaced from %d to %d",
              log_levels->default_logger_level, level);
        }
        log_levels->default_logger_level = (rcl_log_severity_t)level;
      }
      ret = RCL_RET_OK;
    }
  }

  // 检查是否成功解析日志级别
  if (RCUTILS_RET_OK != rcutils_ret) {
    RCL_SET_ERROR_MSG("Argument does not use a valid severity level");
    ret = RCL_RET_ERROR;
  }

cleanup:
  // 清理分配的内存
  if (logger_name) {
    allocator->deallocate(logger_name, allocator->state);
  }
  // 销毁词法分析器
  rcl_ret_t rv = rcl_lexer_lookahead2_fini(&lex_lookahead);
  if (RCL_RET_OK != rv) {
    if (RCL_RET_OK != ret) {
      RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Failed to fini lookahead2 after error occurred");
    } else {
      ret = rv;
    }
  }

  return ret;
}

/**
 * @brief 解析重映射规则
 *
 * @param[in] arg 输入的参数字符串
 * @param[in] allocator 分配器，用于分配内存
 * @param[out] output_rule 输出解析后的重映射规则
 * @return rcl_ret_t 返回状态码
 */
rcl_ret_t _rcl_parse_remap_rule(
    const char *arg, rcl_allocator_t allocator, rcl_remap_t *output_rule) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(arg, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(output_rule, RCL_RET_INVALID_ARGUMENT);

  // 为输出规则分配内存
  output_rule->impl = allocator.allocate(sizeof(rcl_remap_impl_t), allocator.state);
  if (NULL == output_rule->impl) {
    return RCL_RET_BAD_ALLOC;
  }
  // 初始化输出规则的属性
  output_rule->impl->allocator = allocator;
  output_rule->impl->type = RCL_UNKNOWN_REMAP;
  output_rule->impl->node_name = NULL;
  output_rule->impl->match = NULL;
  output_rule->impl->replacement = NULL;

  // 初始化词法分析器
  rcl_lexer_lookahead2_t lex_lookahead = rcl_get_zero_initialized_lexer_lookahead2();
  rcl_ret_t ret = rcl_lexer_lookahead2_init(&lex_lookahead, arg, allocator);

  if (RCL_RET_OK == ret) {
    // 开始解析重映射规则
    ret = _rcl_parse_remap_begin_remap_rule(&lex_lookahead, output_rule);

    // 结束词法分析器
    rcl_ret_t fini_ret = rcl_lexer_lookahead2_fini(&lex_lookahead);
    if (RCL_RET_OK != ret) {
      if (RCL_RET_OK != fini_ret) {
        RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Failed to fini lookahead2 after error occurred");
      }
    } else {
      if (RCL_RET_OK == fini_ret) {
        return RCL_RET_OK;
      }
      ret = fini_ret;
    }
  }

  // 清理输出规则，但保留第一个错误返回码
  if (RCL_RET_OK != rcl_remap_fini(output_rule)) {
    RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Failed to fini remap rule after error occurred");
  }

  return ret;
}

/**
 * @brief 解析参数规则
 *
 * @param[in] arg 输入的参数字符串
 * @param[out] params 参数结构体指针，用于存储解析结果
 * @return rcl_ret_t 返回解析结果状态
 */
rcl_ret_t _rcl_parse_param_rule(const char *arg, rcl_params_t *params) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(arg, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(params, RCL_RET_INVALID_ARGUMENT);

  // 初始化词法分析器
  rcl_lexer_lookahead2_t lex_lookahead = rcl_get_zero_initialized_lexer_lookahead2();

  // 配置词法分析器
  rcl_ret_t ret = rcl_lexer_lookahead2_init(&lex_lookahead, arg, params->allocator);
  if (RCL_RET_OK != ret) {
    return ret;
  }

  rcl_lexeme_t lexeme1;
  rcl_lexeme_t lexeme2;
  char *node_name = NULL;
  char *param_name = NULL;

  // 检查可选的节点名前缀
  ret = rcl_lexer_lookahead2_peek2(&lex_lookahead, &lexeme1, &lexeme2);
  if (RCL_RET_OK != ret) {
    goto cleanup;
  }

  if (RCL_LEXEME_TOKEN == lexeme1 && RCL_LEXEME_COLON == lexeme2) {
    ret = _rcl_parse_nodename_prefix(&lex_lookahead, params->allocator, &node_name);
    if (RCL_RET_OK != ret) {
      if (RCL_RET_WRONG_LEXEME == ret) {
        ret = RCL_RET_INVALID_PARAM_RULE;
      }
      goto cleanup;
    }
  } else {
    node_name = rcutils_strdup("/**", params->allocator);
    if (NULL == node_name) {
      ret = RCL_RET_BAD_ALLOC;
      goto cleanup;
    }
  }

  // 解析参数名
  ret = _rcl_parse_param_name(&lex_lookahead, params->allocator, &param_name);
  if (RCL_RET_OK != ret) {
    if (RCL_RET_WRONG_LEXEME == ret) {
      ret = RCL_RET_INVALID_PARAM_RULE;
    }
    goto cleanup;
  }

  // 检查分隔符
  ret = rcl_lexer_lookahead2_expect(&lex_lookahead, RCL_LEXEME_SEPARATOR, NULL, NULL);
  if (RCL_RET_WRONG_LEXEME == ret) {
    ret = RCL_RET_INVALID_PARAM_RULE;
    goto cleanup;
  }

  // 获取YAML值并解析
  const char *yaml_value = rcl_lexer_lookahead2_get_text(&lex_lookahead);
  if (!rcl_parse_yaml_value(node_name, param_name, yaml_value, params)) {
    ret = RCL_RET_INVALID_PARAM_RULE;
    goto cleanup;
  }

cleanup:
  // 清理内存
  params->allocator.deallocate(param_name, params->allocator.state);
  params->allocator.deallocate(node_name, params->allocator.state);
  if (RCL_RET_OK != ret) {
    if (RCL_RET_OK != rcl_lexer_lookahead2_fini(&lex_lookahead)) {
      RCUTILS_LOG_ERROR_NAMED(ROS_PACKAGE_NAME, "Failed to fini lookahead2 after error occurred");
    }
  } else {
    ret = rcl_lexer_lookahead2_fini(&lex_lookahead);
  }
  return ret;
}

/**
 * @brief 解析参数文件
 *
 * @param[in] arg 输入的参数文件路径
 * @param[in] allocator 分配器
 * @param[out] params 参数结构体指针，用于存储解析结果
 * @param[out] param_file 参数文件路径指针
 * @return rcl_ret_t 返回解析结果状态
 */
rcl_ret_t _rcl_parse_param_file(
    const char *arg, rcl_allocator_t allocator, rcl_params_t *params, char **param_file) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(arg, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(params, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(param_file, RCL_RET_INVALID_ARGUMENT);

  // 复制参数文件路径
  *param_file = rcutils_strdup(arg, allocator);
  if (NULL == *param_file) {
    RCL_SET_ERROR_MSG("Failed to allocate memory for parameters file path");
    return RCL_RET_BAD_ALLOC;
  }

  // 解析YAML文件
  if (!rcl_parse_yaml_file(*param_file, params)) {
    allocator.deallocate(*param_file, allocator.state);
    *param_file = NULL;
    // Error message already set.
    return RCL_RET_ERROR;
  }
  return RCL_RET_OK;
}

/**
 * @brief 解析外部日志配置文件参数
 *
 * @param[in] arg 输入的命令行参数
 * @param[in] allocator 分配器用于分配内存
 * @param[out] log_config_file 用于存储解析后的日志配置文件路径
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t _rcl_parse_external_log_config_file(
    const char *arg, rcl_allocator_t allocator, char **log_config_file) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(arg, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(log_config_file, RCL_RET_INVALID_ARGUMENT);

  // 复制输入参数到log_config_file
  *log_config_file = rcutils_strdup(arg, allocator);
  // TODO(hidmic): add file checks
  // 检查内存分配是否成功
  if (NULL == *log_config_file) {
    RCL_SET_ERROR_MSG("Failed to allocate memory for external log config file");
    return RCL_RET_BAD_ALLOC;
  }
  return RCL_RET_OK;
}

/**
 * @brief 解析enclave参数
 *
 * @param[in] arg 输入的命令行参数
 * @param[in] allocator 分配器用于分配内存
 * @param[out] enclave 用于存储解析后的enclave名称
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t _rcl_parse_enclave(const char *arg, rcl_allocator_t allocator, char **enclave) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(arg, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(enclave, RCL_RET_INVALID_ARGUMENT);

  // 复制输入参数到enclave
  *enclave = rcutils_strdup(arg, allocator);
  // 检查内存分配是否成功
  if (NULL == *enclave) {
    RCL_SET_ERROR_MSG("Failed to allocate memory for enclave name");
    return RCL_RET_BAD_ALLOC;
  }
  return RCL_RET_OK;
}

/**
 * @brief 解析禁用标志参数
 *
 * @param[in] arg 输入的命令行参数
 * @param[in] suffix 禁用标志的后缀
 * @param[out] disable 用于存储解析后的禁用状态
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t _rcl_parse_disabling_flag(const char *arg, const char *suffix, bool *disable) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(arg, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(suffix, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(disable, RCL_RET_INVALID_ARGUMENT);

  // 检查启用前缀和后缀是否匹配
  const size_t enable_prefix_len = strlen(RCL_ENABLE_FLAG_PREFIX);
  if (strncmp(RCL_ENABLE_FLAG_PREFIX, arg, enable_prefix_len) == 0 &&
      strcmp(suffix, arg + enable_prefix_len) == 0) {
    *disable = false;
    return RCL_RET_OK;
  }

  // 检查禁用前缀和后缀是否匹配
  const size_t disable_prefix_len = strlen(RCL_DISABLE_FLAG_PREFIX);
  if (strncmp(RCL_DISABLE_FLAG_PREFIX, arg, disable_prefix_len) == 0 &&
      strcmp(suffix, arg + disable_prefix_len) == 0) {
    *disable = true;
    return RCL_RET_OK;
  }

  // 设置错误消息
  RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
      "Argument is not a %s%s nor a %s%s flag.", RCL_ENABLE_FLAG_PREFIX, suffix,
      RCL_DISABLE_FLAG_PREFIX, suffix);
  return RCL_RET_ERROR;
}

/**
 * @brief 分配并初始化rcl_arguments_t实现结构体
 *
 * @param[out] args rcl_arguments_t结构体指针
 * @param[in] allocator 分配器用于分配内存
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t _rcl_allocate_initialized_arguments_impl(
    rcl_arguments_t *args, rcl_allocator_t *allocator) {
  // 分配内存给args->impl
  args->impl = allocator->allocate(sizeof(rcl_arguments_impl_t), allocator->state);
  // 检查内存分配是否成功
  if (NULL == args->impl) {
    return RCL_RET_BAD_ALLOC;
  }

  // 初始化rcl_arguments_impl_t结构体成员
  rcl_arguments_impl_t *args_impl = args->impl;
  args_impl->num_remap_rules = 0;
  args_impl->remap_rules = NULL;
  args_impl->log_levels = rcl_get_zero_initialized_log_levels();
  args_impl->external_log_config_file = NULL;
  args_impl->unparsed_args = NULL;
  args_impl->num_unparsed_args = 0;
  args_impl->unparsed_ros_args = NULL;
  args_impl->num_unparsed_ros_args = 0;
  args_impl->parameter_overrides = NULL;
  args_impl->parameter_files = NULL;
  args_impl->num_param_files_args = 0;
  args_impl->log_stdout_disabled = false;
  args_impl->log_rosout_disabled = false;
  args_impl->log_ext_lib_disabled = false;
  args_impl->enclave = NULL;
  args_impl->allocator = *allocator;

  return RCL_RET_OK;
}

#ifdef __cplusplus
}
#endif

/// \endcond  // Internal Doxygen documentation
