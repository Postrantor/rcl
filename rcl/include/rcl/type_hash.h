// Copyright 2023 Open Source Robotics Foundation, Inc.
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

#ifndef RCL__TYPE_HASH_H_
#define RCL__TYPE_HASH_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/types.h"
#include "rcl/visibility_control.h"
#include "rcutils/sha256.h"
#include "rosidl_runtime_c/type_hash.h"
#include "type_description_interfaces/msg/type_description.h"

/// 给定一个TypeDescription，输出该数据的可哈希JSON表示形式的字符串。
/**
 * 这里的输出通常通过下面的rcl_calculate_type_hash()进行哈希。
 * 将此参考实现与`rosidl_generator_type_description.generate_type_hash`生成的.json输出文件进行比较。
 * 对于相同的类型，两者必须产生相同的输出，为ROS 2类型版本哈希的外部实现提供稳定的参考。
 *
 * JSON表示包含原始消息的所有类型和字段，但不包括：
 * - 默认值
 * - 注释
 * - 生成TypeDescription的输入纯文本文件
 *
 * \param[in] type_description 预填充的要转换的TypeDescription消息
 * \param[out] output_repr 初始化为空的字符数组，将用type_description的JSON表示填充
 *   注意，output_repr将有一个终止空字符，应从哈希中省略。为此，请使用
 *   (output_repr.buffer_length - 1) 或 strlen(output_repr.buffer) 作为要哈希的数据大小。
 * \return 成功时返回 #RCL_RET_OK，或
 * \return 如果在翻译过程中出现任何问题，则返回 #RCL_RET_ERROR。
 */
RCL_PUBLIC
rcl_ret_t rcl_type_description_to_hashable_json(
  const type_description_interfaces__msg__TypeDescription * type_description,
  rcutils_char_array_t * output_repr);

/// 计算给定TypeDescription的类型版本哈希。
/**
 * 此函数为ROS通信接口类型生成稳定的哈希值。
 * 有关导致此实现的设计动机，请参阅REP-2011。
 *
 * 这个便捷包装器调用rcl_type_description_to_hashable_json，
 * 然后对结果运行sha256哈希。
 *
 * \param[in] msg 预填充的描述要哈希的类型的TypeDescription消息
 * \param[out] message_digest 预分配的缓冲区，将填充计算出的校验和
 * \return 成功时返回 #RCL_RET_OK，或
 * \return 如果在哈希过程中出现任何问题，则返回 #RCL_RET_ERROR。
 */
RCL_PUBLIC
rcl_ret_t rcl_calculate_type_hash(
  const type_description_interfaces__msg__TypeDescription * type_description,
  rosidl_type_hash_t * out_type_hash);

#ifdef __cplusplus
}
#endif

#endif  // RCL__TYPE_HASH_H_
