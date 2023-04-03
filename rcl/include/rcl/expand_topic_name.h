// Copyright 2017 Open Source Robotics Foundation, Inc.
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

#ifndef RCL__EXPAND_TOPIC_NAME_H_
#define RCL__EXPAND_TOPIC_NAME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/allocator.h"
#include "rcl/macros.h"
#include "rcl/types.h"
#include "rcl/visibility_control.h"
#include "rcutils/types/string_map.h"

/// 将给定的主题名称扩展为完全限定的主题名称。
/**
 * input_topic_name、node_name 和 node_namespace 参数必须都是有效的、以空字符结尾的 C 字符串。
 * 如果发生错误，output_topic_name 不会被赋值。
 *
 * output_topic_name 将以空字符结尾。
 * 它也是分配的，因此在不再需要时，需要使用传递给此函数的相同分配器进行释放。
 * 在调用此函数之前，请确保传递给 output_topic_name 的 `char *` 不指向已分配的内存，
 * 因为如果此函数成功，它将被覆盖并因此泄漏。
 *
 * 预期用法：
 *
 * ```c
 * rcl_allocator_t allocator = rcl_get_default_allocator();
 * rcutils_allocator_t rcutils_allocator = rcutils_get_default_allocator();
 * rcutils_string_map_t substitutions_map = rcutils_get_zero_initialized_string_map();
 * rcutils_ret_t rcutils_ret = rcutils_string_map_init(&substitutions_map, 0, rcutils_allocator);
 * if (rcutils_ret != RCUTILS_RET_OK) {
 *   // ... 错误处理
 * }
 * rcl_ret_t ret = rcl_get_default_topic_name_substitutions(&substitutions_map);
 * if (ret != RCL_RET_OK) {
 *   // ... 错误处理
 * }
 * char * expanded_topic_name = NULL;
 * ret = rcl_expand_topic_name(
 *   "some/topic",
 *   "my_node",
 *   "my_ns",
 *   &substitutions_map,
 *   allocator,
 *   &expanded_topic_name);
 * if (ret != RCL_RET_OK) {
 *   // ... 错误处理
 * } else {
 *   RCUTILS_LOG_INFO_NAMED(ROS_PACKAGE_NAME, "Expanded topic name: %s", expanded_topic_name)
 *   // ... 完成后需要释放输出主题名称：
 *   allocator.deallocate(expanded_topic_name, allocator.state);
 * }
 * ```
 *
 * 输入的主题名称使用 rcl_validate_topic_name() 进行验证，
 * 但如果验证失败，则返回 RCL_RET_TOPIC_NAME_INVALID。
 *
 * 输入的节点名称使用 rmw_validate_node_name() 进行验证，
 * 但如果验证失败，则返回 RCL_RET_NODE_INVALID_NAME。
 *
 * 输入的节点命名空间使用 rmw_validate_namespace() 进行验证，
 * 但如果验证失败，则返回 RCL_RET_NODE_INVALID_NAMESPACE。
 *
 * 除了由 rcl_get_default_topic_name_substitutions() 给出的内容外，
 * 还有以下这些替换：
 *
 * - {node} -> 节点的名称
 * - {namespace} -> 节点的命名空间
 * - {ns} -> 节点的命名空间
 *
 * 如果使用未知的替换，将返回 RCL_RET_UNKNOWN_SUBSTITUTION。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] input_topic_name 要扩展的主题名称
 * \param[in] node_name 与主题关联的节点名称
 * \param[in] node_namespace 与主题关联的节点命名空间
 * \param[in] substitutions 可能替换的字符串映射
 * \param[in] allocator 创建输出主题时要使用的分配器
 * \param[out] output_topic_name 输出 char * 指针
 * \return #RCL_RET_OK 如果主题名称扩展成功，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_TOPIC_NAME_INVALID 如果给定的主题名称无效，或
 * \return #RCL_RET_NODE_INVALID_NAME 如果名称无效，或
 * \return #RCL_RET_NODE_INVALID_NAMESPACE 如果命名空间无效，或
 * \return #RCL_RET_UNKNOWN_SUBSTITUTION 对于名称中未知的替换，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_expand_topic_name(
  const char * input_topic_name, const char * node_name, const char * node_namespace,
  const rcutils_string_map_t * substitutions, rcl_allocator_t allocator, char ** output_topic_name);

/// 使用默认的替换对填充给定的字符串映射。
/**
 * 如果字符串映射未初始化，则返回 RCL_RET_INVALID_ARGUMENT。
 *
 * \param[inout] string_map 要用对填充的 rcutils_string_map_t 映射
 * \return #RCL_RET_OK 如果成功，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何参数无效，或
 * \return #RCL_RET_BAD_ALLOC 如果分配内存失败，或
 * \return #RCL_RET_ERROR 如果发生未指定的错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_get_default_topic_name_substitutions(rcutils_string_map_t * string_map);

#ifdef __cplusplus
}
#endif

#endif  // RCL__EXPAND_TOPIC_NAME_H_
