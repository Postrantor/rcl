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

#include "rcl/type_hash.h"

#include <stdio.h>
#include <yaml.h>

#include "rcl/allocator.h"
#include "rcl/error_handling.h"
#include "rcutils/sha256.h"
#include "rcutils/types/char_array.h"
#include "type_description_interfaces/msg/type_description.h"

/**
 * @brief yaml写入处理函数 (YAML write handler function)
 *
 * @param[in] ext 指向rcutils_char_array_t结构体的指针 (Pointer to rcutils_char_array_t structure)
 * @param[in] buffer 要写入的数据缓冲区 (Data buffer to be written)
 * @param[in] size 缓冲区大小 (Size of the buffer)
 * @return 成功返回1，失败返回0 (Returns 1 on success, 0 on failure)
 */
static int yaml_write_handler(void *ext, uint8_t *buffer, size_t size) {
  // 将ext转换为rcutils_char_array_t类型的指针 (Cast ext to a pointer of type rcutils_char_array_t)
  rcutils_char_array_t *repr = (rcutils_char_array_t *)ext;
  // 将buffer中的数据追加到repr中，并检查结果 (Append data from buffer to repr and check the result)
  rcutils_ret_t res = rcutils_char_array_strncat(repr, (char *)buffer, size);
  // 如果结果为RCL_RET_OK，则返回1，否则返回0 (Return 1 if the result is RCL_RET_OK, otherwise
  // return 0)
  return res == RCL_RET_OK ? 1 : 0;
}

/**
 * @brief 开始序列化 (Start sequence)
 *
 * @param[in] emitter 指向yaml_emitter_t结构体的指针 (Pointer to yaml_emitter_t structure)
 * @return 成功返回1，失败返回0 (Returns 1 on success, 0 on failure)
 */
static inline int start_sequence(yaml_emitter_t *emitter) {
  yaml_event_t event;
  // 初始化序列开始事件并检查结果 (Initialize sequence start event and check the result)
  return yaml_sequence_start_event_initialize(&event, NULL, NULL, 1, YAML_FLOW_SEQUENCE_STYLE) &&
         yaml_emitter_emit(emitter, &event);
}

/**
 * @brief 结束序列化 (End sequence)
 *
 * @param[in] emitter 指向yaml_emitter_t结构体的指针 (Pointer to yaml_emitter_t structure)
 * @return 成功返回1，失败返回0 (Returns 1 on success, 0 on failure)
 */
static inline int end_sequence(yaml_emitter_t *emitter) {
  yaml_event_t event;
  // 初始化序列结束事件并检查结果 (Initialize sequence end event and check the result)
  return yaml_sequence_end_event_initialize(&event) && yaml_emitter_emit(emitter, &event);
}

/**
 * @brief 开始映射 (Start mapping)
 *
 * @param[in] emitter 指向yaml_emitter_t结构体的指针 (Pointer to yaml_emitter_t structure)
 * @return 成功返回1，失败返回0 (Returns 1 on success, 0 on failure)
 */
static inline int start_mapping(yaml_emitter_t *emitter) {
  yaml_event_t event;
  // 初始化映射开始事件并检查结果 (Initialize mapping start event and check the result)
  return yaml_mapping_start_event_initialize(&event, NULL, NULL, 1, YAML_FLOW_MAPPING_STYLE) &&
         yaml_emitter_emit(emitter, &event);
}

/**
 * @brief 结束映射 (End mapping)
 *
 * @param[in] emitter 指向yaml_emitter_t结构体的指针 (Pointer to yaml_emitter_t structure)
 * @return 成功返回1，失败返回0 (Returns 1 on success, 0 on failure)
 */
static inline int end_mapping(yaml_emitter_t *emitter) {
  yaml_event_t event;
  // 初始化映射结束事件并检查结果 (Initialize mapping end event and check the result)
  return yaml_mapping_end_event_initialize(&event) && yaml_emitter_emit(emitter, &event);
}

/**
 * @brief 为 YAML emitter 发送一个键值对的键。
 *
 * 这个函数初始化一个标量事件，并将给定的键作为双引号字符串发送到 YAML emitter。
 *
 * @param[in] emitter 指向 yaml_emitter_t 结构体的指针，用于存储 YAML emitter 的状态。
 * @param[in] key 要发送的键的字符串表示形式。
 *
 * @return 如果成功发送键，则返回非零值；否则返回零。
 */
static int emit_key(yaml_emitter_t *emitter, const char *key) {
  // 定义一个 YAML 事件结构体变量
  yaml_event_t event;

  // 使用给定的键初始化一个标量事件，并设置其样式为双引号字符串
  // 返回值为布尔值，表示初始化是否成功
  return yaml_scalar_event_initialize(
             &event, NULL, NULL, (yaml_char_t *)key, (int)strlen(key), 0, 1,
             YAML_DOUBLE_QUOTED_SCALAR_STYLE) &&
         // 将初始化的事件发送到 YAML emitter
         // 返回值为布尔值，表示发送是否成功
         yaml_emitter_emit(emitter, &event);
}

/**
 * @brief 将整数值转换为字符串并使用 YAML emitter 发送事件
 *
 * @param[in] emitter 一个指向 yaml_emitter_t 结构的指针，用于发送 YAML 事件
 * @param[in] val 要转换和发送的整数值
 * @param[in] fmt 用于格式化整数值的格式字符串
 * @return 成功时返回非零值，失败时返回零
 */
static int emit_int(yaml_emitter_t *emitter, size_t val, const char *fmt) {
  // 最长的 uint64 是 20 个十进制数字，再加上一个字节的 \0
  char decimal_buf[21];
  yaml_event_t event;

  // 使用 snprintf 将整数值格式化为字符串
  int ret = snprintf(decimal_buf, sizeof(decimal_buf), fmt, val);
  if (ret < 0) {
    emitter->problem = "Failed expanding integer";
    return 0;
  }

  // 检查是否发生缓冲区溢出
  if ((size_t)ret >= sizeof(decimal_buf)) {
    emitter->problem = "Decimal buffer overflow";
    return 0;
  }

  // 初始化 YAML 标量事件并使用 emitter 发送
  return yaml_scalar_event_initialize(
             &event, NULL, NULL, (yaml_char_t *)decimal_buf, (int)strlen(decimal_buf), 1, 0,
             YAML_PLAIN_SCALAR_STYLE) &&
         yaml_emitter_emit(emitter, &event);
}

/**
 * @brief 使用 YAML emitter 发送 ROSIDL 字符串
 *
 * @param[in] emitter 一个指向 yaml_emitter_t 结构的指针，用于发送 YAML 事件
 * @param[in] val 一个指向 rosidl_runtime_c__String 结构的指针，表示要发送的字符串
 * @return 成功时返回非零值，失败时返回零
 */
static int emit_str(yaml_emitter_t *emitter, const rosidl_runtime_c__String *val) {
  yaml_event_t event;

  // 初始化 YAML 标量事件并使用 emitter 发送
  return yaml_scalar_event_initialize(
             &event, NULL, NULL, (yaml_char_t *)val->data, (int)val->size, 0, 1,
             YAML_DOUBLE_QUOTED_SCALAR_STYLE) &&
         yaml_emitter_emit(emitter, &event);
}

/**
 * @brief 将字段类型信息输出到YAML emitter中
 *
 * @param[in] emitter YAML emitter对象，用于输出YAML格式的数据
 * @param[in] field_type 字段类型信息结构体指针
 * @return 成功返回1，失败返回0
 */
static int emit_field_type(
    yaml_emitter_t *emitter, const type_description_interfaces__msg__FieldType *field_type) {
  // 开始一个映射(mapping)结构
  return start_mapping(emitter) &&

         // 输出"type_id"键
         emit_key(emitter, "type_id") &&
         // 输出对应的整数值
         emit_int(emitter, field_type->type_id, "%d") &&

         // 输出"capacity"键
         emit_key(emitter, "capacity") &&
         // 输出对应的整数值
         emit_int(emitter, field_type->capacity, "%zu") &&

         // 输出"string_capacity"键
         emit_key(emitter, "string_capacity") &&
         // 输出对应的整数值
         emit_int(emitter, field_type->string_capacity, "%zu") &&

         // 输出"nested_type_name"键
         emit_key(emitter, "nested_type_name") &&
         // 输出对应的字符串值
         emit_str(emitter, &field_type->nested_type_name) &&

         // 结束映射(mapping)结构
         end_mapping(emitter);
}

/**
 * @brief 将字段信息输出到YAML emitter中
 *
 * @param[in] emitter YAML emitter对象，用于输出YAML格式的数据
 * @param[in] field 字段信息结构体指针
 * @return 成功返回1，失败返回0
 */
static int emit_field(
    yaml_emitter_t *emitter, const type_description_interfaces__msg__Field *field) {
  // 开始一个映射(mapping)结构
  return start_mapping(emitter) &&

         // 输出"name"键
         emit_key(emitter, "name") &&
         // 输出对应的字符串值
         emit_str(emitter, &field->name) &&

         // 输出"type"键
         emit_key(emitter, "type") &&
         // 输出字段类型信息
         emit_field_type(emitter, &field->type) &&

         // 结束映射(mapping)结构
         end_mapping(emitter);
}

/**
 * @brief 为单个类型描述生成 YAML 映射
 *
 * @param[in] emitter YAML emitter 对象
 * @param[in] individual_type_description 单个类型描述对象
 * @return 成功返回 1，失败返回 0
 */
static int emit_individual_type_description(
    yaml_emitter_t *emitter,
    const type_description_interfaces__msg__IndividualTypeDescription
        *individual_type_description) {
  // 开始映射并添加 "type_name" 键和对应的值，然后添加 "fields" 键并开始序列
  if (!(start_mapping(emitter) &&

        emit_key(emitter, "type_name") &&
        emit_str(emitter, &individual_type_description->type_name) &&

        emit_key(emitter, "fields") && start_sequence(emitter))) {
    return 0;
  }
  // 遍历 fields 并生成 YAML 映射
  for (size_t i = 0; i < individual_type_description->fields.size; i++) {
    if (!emit_field(emitter, &individual_type_description->fields.data[i])) {
      return 0;
    }
  }
  // 结束序列和映射
  return end_sequence(emitter) && end_mapping(emitter);
}

/**
 * @brief 为类型描述生成 YAML 映射
 *
 * @param[in] emitter YAML emitter 对象
 * @param[in] type_description 类型描述对象
 * @return 成功返回 1，失败返回 0
 */
static int emit_type_description(
    yaml_emitter_t *emitter,
    const type_description_interfaces__msg__TypeDescription *type_description) {
  // 开始映射并添加 "type_description" 键和对应的值，然后添加 "referenced_type_descriptions"
  // 键并开始序列
  if (!(start_mapping(emitter) &&

        emit_key(emitter, "type_description") &&
        emit_individual_type_description(emitter, &type_description->type_description) &&

        emit_key(emitter, "referenced_type_descriptions") && start_sequence(emitter))) {
    return 0;
  }
  // 遍历 referenced_type_descriptions 并生成 YAML 映射
  for (size_t i = 0; i < type_description->referenced_type_descriptions.size; i++) {
    if (!emit_individual_type_description(
            emitter, &type_description->referenced_type_descriptions.data[i])) {
      return 0;
    }
  }
  // 结束序列和映射
  return end_sequence(emitter) && end_mapping(emitter);
}

/**
 * @brief 将类型描述转换为可哈希的 JSON 字符串。
 *
 * @param[in] type_description 指向 type_description_interfaces__msg__TypeDescription 结构体的指针。
 * @param[out] output_repr 用于存储生成的 JSON 字符串的 rcutils_char_array_t 结构体指针。
 * @return 返回 rcl_ret_t 类型的结果，成功返回 RCL_RET_OK，否则返回相应的错误代码。
 */
rcl_ret_t rcl_type_description_to_hashable_json(
    const type_description_interfaces__msg__TypeDescription *type_description,
    rcutils_char_array_t *output_repr) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(type_description, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(output_repr, RCL_RET_INVALID_ARGUMENT);

  // 定义 YAML emitter 和 event 变量
  yaml_emitter_t emitter;
  yaml_event_t event;

  // 初始化 YAML emitter
  if (!yaml_emitter_initialize(&emitter)) {
    goto error;
  }

  // 禁用基于行长度的换行
  yaml_emitter_set_width(&emitter, -1);
  // 通过提供无效的换行样式来避免 EOF 换行
  yaml_emitter_set_break(&emitter, -1);
  // 设置 YAML emitter 的输出处理函数和输出对象
  yaml_emitter_set_output(&emitter, yaml_write_handler, output_repr);

  // 初始化并发射 YAML 事件以生成 JSON 字符串
  if (!(yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING) &&
        yaml_emitter_emit(&emitter, &event) &&

        yaml_document_start_event_initialize(&event, NULL, NULL, NULL, 1) &&
        yaml_emitter_emit(&emitter, &event) &&

        emit_type_description(&emitter, type_description) &&

        yaml_document_end_event_initialize(&event, 1) && yaml_emitter_emit(&emitter, &event) &&

        yaml_stream_end_event_initialize(&event) && yaml_emitter_emit(&emitter, &event))) {
    goto error;
  }

  // 删除 YAML emitter 并返回成功
  yaml_emitter_delete(&emitter);
  return RCL_RET_OK;

// 错误处理
error:
  // 设置错误状态并删除 YAML emitter
  rcl_set_error_state(emitter.problem, __FILE__, __LINE__);
  yaml_emitter_delete(&emitter);
  return RCL_RET_ERROR;
}

/**
 * @brief 计算类型哈希值
 *
 * 该函数根据给定的类型描述计算其哈希值。
 *
 * @param[in] type_description 指向类型描述结构体的指针
 * @param[out] output_hash 用于存储计算出的哈希值的指针
 * @return 返回 rcl_ret_t 类型的结果，成功返回 RCL_RET_OK，否则返回相应的错误代码
 */
rcl_ret_t rcl_calculate_type_hash(
    const type_description_interfaces__msg__TypeDescription *type_description,
    rosidl_type_hash_t *output_hash) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(type_description, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(output_hash, RCL_RET_INVALID_ARGUMENT);

  // 初始化结果变量
  rcl_ret_t result = RCL_RET_OK;
  // 初始化字符数组用于存储类型描述的 JSON 表示
  rcutils_char_array_t msg_repr = rcutils_get_zero_initialized_char_array();
  // 设置字符数组的内存分配器
  msg_repr.allocator = rcl_get_default_allocator();

  // 设置输出哈希值的版本
  output_hash->version = 1;
  // 将类型描述转换为可哈希的 JSON 字符串
  result = rcl_type_description_to_hashable_json(type_description, &msg_repr);
  if (result == RCL_RET_OK) {
    // 初始化 SHA-256 上下文
    rcutils_sha256_ctx_t sha_ctx;
    rcutils_sha256_init(&sha_ctx);
    // 更新 SHA-256 上下文，注意不包括字符数组的空终止符
    rcutils_sha256_update(&sha_ctx, (const uint8_t *)msg_repr.buffer, msg_repr.buffer_length - 1);
    // 计算哈希值并存储到 output_hash 中
    rcutils_sha256_final(&sha_ctx, output_hash->value);
  }
  // 释放字符数组占用的内存
  result = rcutils_char_array_fini(&msg_repr);
  // 返回结果
  return result;
}
