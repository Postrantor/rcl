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

#ifndef RCL__REMAP_H_
#define RCL__REMAP_H_

#include "rcl/allocator.h"
#include "rcl/arguments.h"
#include "rcl/macros.h"
#include "rcl/types.h"
#include "rcl/visibility_control.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct rcl_remap_impl_s rcl_remap_impl_t;

/// 保存重映射规则的结构体.
typedef struct rcl_remap_s
{
  /// 私有实现指针.
  rcl_remap_impl_t * impl;
} rcl_remap_t;

/// 返回一个成员初始化为 `NULL` 的 rcl_remap_t 结构体.
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_remap_t rcl_get_zero_initialized_remap(void);

// TODO(sloretz) 当支持 rostopic:// 时添加文档
/// 根据给定的规则重映射主题名称.
/**
 * 提供的主题名称必须已经扩展为完全限定名称。
 * \sa rcl_expand_topic_name()
 *
 * 如果 `local_arguments` 不是 NULL 且不是零初始化，则首先检查其重映射规则。
 * 如果没有规则匹配且 `global_arguments` 不是 NULL 且不是零初始化，则接下来检查其规则。
 * 如果 `local_arguments` 和 global_arguments 都是 NULL 或零初始化，则函数将返回 RCL_RET_INVALID_ARGUMENT。
 *
 * `global_arguments` 通常是传递给 rcl_init() 的参数。
 * \sa rcl_init()
 * \sa rcl_get_global_arguments()
 *
 * 按照给定的顺序检查重映射规则。
 * 对于传递给 rcl_init() 的规则，这通常是它们在命令行上的顺序。
 * \sa rcl_parse_arguments()
 *
 * 只有第一个匹配的重映射规则才用于重映射名称。
 * 例如，如果命令行参数是 `foo:=bar bar:=baz`，主题 `foo` 被重映射为 `bar` 而不是 `baz`。
 *
 * 使用 `node_name` 和 `node_namespace` 将匹配和替换扩展为完全限定名称。
 * 给定节点名 `trudy`、命名空间 `/ns` 和规则 `foo:=~/bar`，规则中的名称将扩展为 `/ns/foo:=/ns/trudy/bar`。
 * 只有在给定的主题名称为 `/ns/foo` 时，规则才会生效。
 *
 * `node_name` 也用于匹配特定节点的规则。
 * 给定规则 `alice:foo:=bar foo:=baz`、节点名 `alice` 和主题 `foo`，重映射的主题名称将是 `bar`。
 * 如果给定节点名 `bob` 和主题 `foo`，重映射的主题名称将是 `baz`。
 * 请注意，即使后面有更具体的规则，处理过程也总是在第一个匹配的规则处停止。
 * 给定 `foo:=bar alice:foo:=baz` 和主题名 `foo`，重映射的主题名始终为 `bar`，而与给定的节点名无关。
 *
 * <hr>
 * 属性                | 遵循
 * ------------------ | -------------
 * 分配内存            | 是
 * 线程安全            | 否
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] local_arguments 在全局参数之前使用的命令行参数，如果为 NULL 或零初始化，则只使用全局参数。
 * \param[in] global_arguments 如果没有本地规则匹配，则使用的命令行参数，或者 `NULL` 或零初始化以忽略全局参数。
 * \param[in] topic_name 要重映射的完全限定和扩展主题名称。
 * \param[in] node_name 名称所属节点的名称。
 * \param[in] node_namespace 名称所属节点的命名空间。
 * \param[in] allocator 要使用的有效分配器。
 * \param[out] output_name 分配的重映射名称字符串，或者如果没有重映射规则匹配名称，则为 `NULL`。
 * \return #RCL_RET_OK 如果主题名称被重映射或没有规则匹配，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_TOPIC_NAME_INVALID 如果给定的主题名称无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_remap_topic_name(
  const rcl_arguments_t * local_arguments, const rcl_arguments_t * global_arguments,
  const char * topic_name, const char * node_name, const char * node_namespace,
  rcl_allocator_t allocator, char ** output_name);

// TODO(sloretz) add documentation about rosservice:// when it is supported
/// 根据给定的规则重新映射服务名称。
/**
 * 提供的服务名称必须已经扩展为完全限定名称。
 *
 * 该函数的行为与 rcl_expand_topic_name() 相同，只是它适用于服务名称而不是主题名称。
 * \sa rcl_expand_topic_name()
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] local_arguments 在全局参数之前使用的命令行参数，或
 *   如果为 NULL 或零初始化，则仅使用全局参数。
 * \param[in] global_arguments 如果没有匹配的本地规则，则使用的命令行参数，或
 *   `NULL` 或零初始化以忽略全局参数。
 * \param[in] service_name 要重新映射的完全限定和扩展服务名称。
 * \param[in] node_name 名称所属节点的名称。
 * \param[in] node_namespace 名称所属节点的命名空间。
 * \param[in] allocator 要使用的有效分配器。
 * \param[out] output_name 分配的字符串，包含重新映射的名称，或
 *   如果没有重新映射规则匹配名称，则为 `NULL`。
 * \return #RCL_RET_OK 如果名称被重新映射或没有规则匹配，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_SERVICE_NAME_INVALID 如果给定名称无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_remap_service_name(
  const rcl_arguments_t * local_arguments, const rcl_arguments_t * global_arguments,
  const char * service_name, const char * node_name, const char * node_namespace,
  rcl_allocator_t allocator, char ** output_name);

/// 根据给定的规则重新映射节点名称。
/**
 * 此函数返回具有给定名称的节点将被重新映射到的节点名称。
 * 当节点的名称被重新映射时，它会更改其记录器名称和扩展相对主题和服务名称的输出。
 *
 * 在组合节点时，请确保最终使用的节点名称在每个进程中都是唯一的。
 * 目前还没有一种方法可以独立地重新映射两个具有相同节点名称并手动组合到一个进程中的节点的名称。
 *
 * `local_arguments`、`global_arguments`、`node_name` 的行为，以及重新映射规则的应用顺序和节点特定规则与 rcl_remap_topic_name() 相同。
 * \sa rcl_remap_topic_name()
 *
 * <hr>
 * 属性              | 遵循性
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] local_arguments 在全局参数之前使用的参数。
 * \param[in] global_arguments 如果没有匹配的本地规则，则使用的命令行参数，或
 *   `NULL` 或零初始化以忽略全局参数。
 * \param[in] node_name 当前节点的名称。
 * \param[in] allocator 要使用的有效分配器。
 * \param[out] output_name 分配的字符串，包含重新映射的名称，或
 *   如果没有重新映射规则匹配名称，则为 `NULL`。
 * \return #RCL_RET_OK 如果名称被重新映射或没有规则匹配，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_NODE_INVALID_NAME 如果名称无效，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_remap_node_name(
  const rcl_arguments_t * local_arguments, const rcl_arguments_t * global_arguments,
  const char * node_name, rcl_allocator_t allocator, char ** output_name);

/// 基于给定规则重新映射命名空间。
/**
 * 此函数返回具有给定名称的节点将被重新映射到的命名空间。
 * 当节点的命名空间被重新映射时，它会更改其记录器名称和扩展相对主题和服务名称的输出。
 *
 * `local_arguments`、`global_arguments`、`node_name` 的行为，以及重新映射规则的应用顺序和节点特定规则与 rcl_remap_topic_name() 相同。
 * \sa rcl_remap_topic_name()
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] local_arguments 在全局参数之前使用的参数。
 * \param[in] global_arguments 如果没有匹配的本地规则，则使用的命令行参数，或者
 *   `NULL` 或零初始化以忽略全局参数。
 * \param[in] node_name 被重新映射的命名空间的节点名称。
 * \param[in] allocator 要使用的有效分配器。
 * \param[out] output_namespace 分配的字符串，其中包含重新映射的命名空间，或者
 *   如果没有重新映射规则匹配名称，则为 `NULL`。
 * \return #RCL_RET_OK 如果节点名称被重新映射或没有规则匹配，或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或者
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或者
 * \return #RCL_RET_NODE_INVALID_NAMESPACE 如果重新映射的命名空间无效，或者
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_remap_node_namespace(
  const rcl_arguments_t * local_arguments, const rcl_arguments_t * global_arguments,
  const char * node_name, rcl_allocator_t allocator, char ** output_namespace);

/// 将一个重新映射结构复制到另一个。
/**
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] rule 要复制的结构。
 *  其分配器用于将内存复制到新结构中。
 * \param[out] rule_out 要复制到的零初始化 rcl_remap_t 结构。
 * \return #RCL_RET_OK 如果结构成功复制，或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何函数参数无效，或者
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或者
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_remap_copy(const rcl_remap_t * rule, rcl_remap_t * rule_out);

/// 回收 rcl_remap_t 结构内部持有的资源。
/**
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] remap 要释放的结构。
 * \return #RCL_RET_OK 如果内存成功释放，或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何函数参数无效，或者
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_remap_fini(rcl_remap_t * remap);

#ifdef __cplusplus
}
#endif

#endif  // RCL__REMAP_H_
