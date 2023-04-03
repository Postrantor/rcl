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

#ifndef RCL__ARGUMENTS_H_
#define RCL__ARGUMENTS_H_

#include "rcl/allocator.h"
#include "rcl/log_level.h"
#include "rcl/macros.h"
#include "rcl/types.h"
#include "rcl/visibility_control.h"
#include "rcl_yaml_param_parser/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \typedef rcl_arguments_impl_s rcl_arguments_impl_t
/// \brief 私有实现结构体类型定义

/// \struct rcl_arguments_s
/// \brief 保存解析命令行参数的输出结果
typedef struct rcl_arguments_s {
  /// \var rcl_arguments_impl_t * impl
  /// \brief 私有实现指针
  rcl_arguments_impl_t* impl;
} rcl_arguments_t;

/// \def RCL_ROS_ARGS_FLAG
/// \brief 命令行标志，表示 ROS 参数的开始
#define RCL_ROS_ARGS_FLAG "--ros-args"

/// \def RCL_ROS_ARGS_EXPLICIT_END_TOKEN
/// \brief 标记，表示 ROS 参数的显式结束
#define RCL_ROS_ARGS_EXPLICIT_END_TOKEN "--"

/// \def RCL_PARAM_FLAG
/// \brief ROS 标志，表示设置 ROS 参数
#define RCL_PARAM_FLAG "--param"

/// \def RCL_SHORT_PARAM_FLAG
/// \brief ROS 短标志，表示设置 ROS 参数
#define RCL_SHORT_PARAM_FLAG "-p"

/// \def RCL_PARAM_FILE_FLAG
/// \brief ROS 标志，表示包含 ROS 参数的文件路径
#define RCL_PARAM_FILE_FLAG "--params-file"

/// \def RCL_REMAP_FLAG
/// \brief ROS 标志，表示 ROS 重映射规则
#define RCL_REMAP_FLAG "--remap"

/// \def RCL_SHORT_REMAP_FLAG
/// \brief ROS 短标志，表示 ROS 重映射规则
#define RCL_SHORT_REMAP_FLAG "-r"

/// \def RCL_ENCLAVE_FLAG
/// \brief ROS 标志，表示 ROS 安全领域的名称
#define RCL_ENCLAVE_FLAG "--enclave"

/// \def RCL_SHORT_ENCLAVE_FLAG
/// \brief ROS 短标志，表示 ROS 安全领域的名称
#define RCL_SHORT_ENCLAVE_FLAG "-e"

/// \def RCL_LOG_LEVEL_FLAG
/// \brief ROS 标志，表示设置 ROS 日志级别
#define RCL_LOG_LEVEL_FLAG "--log-level"

/// \def RCL_EXTERNAL_LOG_CONFIG_FLAG
/// \brief ROS 标志，表示配置日志的配置文件名称
#define RCL_EXTERNAL_LOG_CONFIG_FLAG "--log-config-file"

/// \def RCL_LOG_STDOUT_FLAG_SUFFIX
/// \brief ROS 标志后缀，用于启用或禁用 stdout 日志（必须以 --enable- 或 --disable- 开头）
#define RCL_LOG_STDOUT_FLAG_SUFFIX "stdout-logs"

/// \def RCL_LOG_ROSOUT_FLAG_SUFFIX
/// \brief ROS 标志后缀，用于启用或禁用 rosout 日志（必须以 --enable- 或 --disable- 开头）
#define RCL_LOG_ROSOUT_FLAG_SUFFIX "rosout-logs"

/// \def RCL_LOG_EXT_LIB_FLAG_SUFFIX
/// \brief ROS 标志后缀，用于启用或禁用外部库日志（必须以 --enable- 或 --disable- 开头）
#define RCL_LOG_EXT_LIB_FLAG_SUFFIX "external-lib-logs"

/// \brief 返回一个 rcl_arguments_t 结构体，其成员初始化为 `NULL`
/// \return 初始化为 `NULL` 的 rcl_arguments_t 结构体
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_arguments_t rcl_get_zero_initialized_arguments(void);

/// 解析命令行参数到一个可供代码使用的结构体。
/**
 * \sa rcl_get_zero_initialized_arguments()
 *
 * ROS 参数预期由一个前导 `--ros-args` 标志和一个后置双
 * 破折号标记 `--` 限定，如果在最后一个 `--ros-args` 后没有非 ROS 参数，则可以省略。
 *
 * 支持通过 `-r/--remap` 标志解析重映射规则，例如 `--remap from:=to` 或 `-r from:=to`。
 * 成功解析的重映射规则按照它们在 `argv` 中给出的顺序存储。
 * 如果给定参数 `{"__ns:=/foo", "__ns:=/bar"}`，则此过程中节点使用的命名空间将是 `/foo` 而不是
 * `/bar`。
 *
 * \sa rcl_remap_topic_name()
 * \sa rcl_remap_service_name()
 * \sa rcl_remap_node_name()
 * \sa rcl_remap_node_namespace()
 *
 * 支持通过 `-p/--param` 标志解析参数覆盖规则，例如 `--param name:=value`
 * 或 `-p name:=value`。
 *
 * 默认日志级别将解析为 `--log-level level`，而记录器级别将解析为
 * 多个 `--log-level name:=level`，其中 `level` 是一个名称，表示 `RCUTILS_LOG_SEVERITY`
 * 枚举中的日志级别之一， 例如 `info`、`debug`、`warn`，不区分大小写。
 * 如果找到多个这样的规则，则使用最后解析的一个。
 *
 * 如果参数看起来不是有效的 ROS 参数，例如 `-r/--remap` 标志后面跟着的
 * 不是有效的重映射规则，则解析将立即失败。
 *
 * 如果参数看起来不是已知的 ROS 参数，则跳过它并保持未解析状态。
 *
 * \sa rcl_arguments_get_count_unparsed_ros()
 * \sa rcl_arguments_get_unparsed_ros()
 *
 * 在 `--ros-args ... --` 范围之外找到的所有参数都将被跳过并保持未解析状态。
 *
 * \sa rcl_arguments_get_count_unparsed()
 * \sa rcl_arguments_get_unparsed()
 *
 * <hr>
 * 属性                | 遵循性
 * ------------------ | -------------
 * 分配内存            | 是
 * 线程安全            | 是
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] argc 参数的数量。
 * \param[in] argv 参数的值。
 * \param[in] allocator 有效的分配器。
 * \param[out] args_output 将包含解析结果的结构体。
 *   使用前必须初始化为零。
 * \return #RCL_RET_OK 如果参数解析成功，或
 * \return #RCL_RET_INVALID_ROS_ARGS 如果找到无效的 ROS 参数，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何函数参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_parse_arguments(
    int argc, const char* const* argv, rcl_allocator_t allocator, rcl_arguments_t* args_output);

/// 返回未解析为ROS特定参数的参数数量。
/**
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 是
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] args 已解析的参数结构。
 * \return 未解析参数的数量，或
 * \return -1 如果args为`NULL`或零初始化。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
int rcl_arguments_get_count_unparsed(const rcl_arguments_t* args);

/// 返回非ROS特定参数的索引列表。
/**
 * 可能提供了非ROS特定参数，即'--ros-args'范围之外的参数。
 * 此函数将在原始argv数组中填充这些参数的索引数组。
 * 由于第一个参数总是被认为是进程名，所以列表中总是包含索引0。
 *
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 是
 * 线程安全            | 是
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] args 已解析的参数结构。
 * \param[in] allocator 有效的分配器。
 * \param[out] output_unparsed_indices 分配的原始argv数组中的索引数组。
 *   调用者必须使用给定的分配器释放此数组。
 *   如果没有未解析的参数，则输出将设置为NULL。
 * \return #RCL_RET_OK 如果一切正常，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何函数参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_arguments_get_unparsed(
    const rcl_arguments_t* args, rcl_allocator_t allocator, int** output_unparsed_indices);

/// 返回未成功解析的ROS特定参数的数量。
/**
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 是
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] args 已解析的参数结构。
 * \return 未解析的ROS特定参数的数量，或
 * \return -1 如果args为`NULL`或零初始化。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
int rcl_arguments_get_count_unparsed_ros(const rcl_arguments_t* args);

/// 返回未解析的未知ROS特定参数的索引列表。
/**
 * 一些ROS特定参数可能没有被识别，或者不打算被rcl解析。
 * 此函数将在原始argv数组中填充这些参数的索引数组。
 *
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 是
 * 线程安全            | 是
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] args 已解析的参数结构。
 * \param[in] allocator 有效的分配器。
 * \param[out] output_unparsed_ros_indices 分配的原始argv数组中的索引数组。
 *   调用者必须使用给定的分配器释放此数组。
 *   如果没有未解析的ROS特定参数，则输出将设置为NULL。
 * \return #RCL_RET_OK 如果一切正常，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何函数参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_arguments_get_unparsed_ros(
    const rcl_arguments_t* args, rcl_allocator_t allocator, int** output_unparsed_ros_indices);

/// 返回参数中给出的参数yaml文件的数量。
/**
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存           | 否
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * \param[in] args 已解析的参数结构。
 * \return yaml文件的数量，或
 * \return -1 如果args为`NULL`或零初始化。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
int rcl_arguments_get_param_files_count(const rcl_arguments_t* args);

/// 返回在命令行中指定的yaml参数文件路径列表。
/**
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存           | 是
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * \param[in] arguments 已解析的参数结构。
 * \param[in] allocator 有效的分配器。
 * \param[out] parameter_files 分配的参数文件名数组。
 *   调用者必须使用给定的分配器释放此数组。
 *   如果没有参数文件，则输出为NULL。
 * \return #RCL_RET_OK 如果一切正常，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何函数参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_arguments_get_param_files(
    const rcl_arguments_t* arguments, rcl_allocator_t allocator, char*** parameter_files);

/// 返回从命令行解析的所有参数覆盖。
/**
 * 参数覆盖是直接从命令行参数和命令行中提供的参数文件解析的。
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存           | 是
 * 线程安全           | 否
 * 使用原子操作       | 否
 * 无锁               | 是
 *
 * \param[in] arguments 已解析的参数结构。
 * \param[out] parameter_overrides 从命令行参数解析的参数覆盖。
 *   调用者必须完成此结构。
 *   如果没有解析到参数覆盖，则输出为NULL。
 * \return #RCL_RET_OK 如果一切正常，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何函数参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_arguments_get_param_overrides(
    const rcl_arguments_t* arguments, rcl_params_t** parameter_overrides);

/// 返回一个删除了 ROS 特定参数的参数列表。
/**
 * 一些参数可能不是作为 ROS 参数传递的。
 * 此函数将新的 argv 数组中的参数填充到一个数组中。
 * 由于第一个参数总是被认为是进程名，所以列表中始终包含参数向量的第一个值。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 是
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] argv 参数向量
 * \param[in] args 已解析的参数结构。
 * \param[in] allocator 有效的分配器。
 * \param[out] nonros_argc 不是 ROS 特定的参数计数
 * \param[out] nonros_argv 分配的不是 ROS 特定的参数数组
 *   调用者必须使用给定的分配器释放此数组。
 *   如果没有非 ROS 参数，则输出将设置为 NULL。
 * \return #RCL_RET_OK 如果一切正常，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何函数参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_remove_ros_arguments(
    const char* const* argv,
    const rcl_arguments_t* args,
    rcl_allocator_t allocator,
    int* nonros_argc,
    const char*** nonros_argv);

/// 返回从命令行解析的日志级别。
/**
 * 日志级别直接从命令行参数解析。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] arguments 已解析的参数结构。
 * \param[out] log_levels 从命令行参数解析的日志级别。
 *   如果函数成功，则调用者必须完成输出。
 * \return #RCL_RET_OK 如果一切正常，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何函数参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_arguments_get_log_levels(
    const rcl_arguments_t* arguments, rcl_log_levels_t* log_levels);

/// 将一个参数结构复制到另一个参数结构。
/**
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] args 要复制的结构。
 *  其分配器用于将内存复制到新结构中。
 * \param[out] args_out 要复制到的零初始化参数结构。
 * \return #RCL_RET_OK 如果结构复制成功，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何函数参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_arguments_copy(const rcl_arguments_t* args, rcl_arguments_t* args_out);

/// 回收 rcl_arguments_t 结构内部持有的资源。
/**
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] args 要释放的结构。
 * \return #RCL_RET_OK 如果内存成功释放，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何函数参数无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_arguments_fini(rcl_arguments_t* args);

#ifdef __cplusplus
}
#endif

#endif  // RCL__ARGUMENTS_H_
