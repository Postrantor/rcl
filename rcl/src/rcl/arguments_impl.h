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

#ifndef RCL__ARGUMENTS_IMPL_H_
#define RCL__ARGUMENTS_IMPL_H_

#include "./remap_impl.h"
#include "rcl/arguments.h"
#include "rcl/log_level.h"
#include "rcl_yaml_param_parser/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \internal
/// \brief 结构体 rcl_arguments_impl_s，用于存储解析后的命令行参数信息。
struct rcl_arguments_impl_s
{
  /// \brief 未解析的 ROS 特定参数的索引数组。
  int * unparsed_ros_args;
  /// \brief unparsed_ros_args 的长度。
  int num_unparsed_ros_args;

  /// \brief 非 ROS 参数的索引数组。
  int * unparsed_args;
  /// \brief unparsed_args 的长度。
  int num_unparsed_args;

  /// \brief 从参数中解析出的参数覆盖规则。
  rcl_params_t * parameter_overrides;

  /// \brief yaml 参数文件路径数组。
  char ** parameter_files;
  /// \brief parameter_files 的长度。
  int num_param_files_args;

  /// \brief 名称重映射规则数组。
  rcl_remap_t * remap_rules;
  /// \brief remap_rules 的长度。
  int num_remap_rules;

  /// \brief 从参数中解析出的日志级别。
  rcl_log_levels_t log_levels;
  /// \brief 用于配置外部日志库的文件。
  char * external_log_config_file;
  /// \brief 布尔值，表示是否禁用标准输出处理程序进行日志输出。
  bool log_stdout_disabled;
  /// \brief 布尔值，表示是否禁用 rosout 主题处理程序进行日志输出。
  bool log_rosout_disabled;
  /// \brief 布尔值，表示是否禁用外部库处理程序进行日志输出。
  bool log_ext_lib_disabled;

  /// \brief 要使用的 Enclave。
  char * enclave;

  /// \brief 用于在此结构中分配对象的分配器。
  rcl_allocator_t allocator;
};

#ifdef __cplusplus
}
#endif

#endif  // RCL__ARGUMENTS_IMPL_H_
