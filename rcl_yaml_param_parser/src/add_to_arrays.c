// Copyright 2018 Apex.AI, Inc.
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

#include "./impl/add_to_arrays.h"

#include "rcutils/allocator.h"
#include "rcutils/error_handling.h"
#include "rcutils/types/rcutils_ret.h"
#include "rcutils/types/string_array.h"

/**
 * @brief 将值添加到简单数组中
 *
 * 此宏用于将给定值添加到指定的简单数组中。如果数组为空，则初始化数组并设置大小为1。
 * 如果数组非空，则增加数组大小并将新值添加到数组末尾。
 *
 * @param[in] val_array      要修改的简单数组
 * @param[in] value          要添加到数组的值
 * @param[in] value_type     数组中值的类型
 * @param[in] allocator      用于分配和释放内存的分配器
 *
 * @return RCUTILS_RET_OK 成功添加值，或者 RCUTILS_RET_BAD_ALLOC 分配内存失败
 */
#define ADD_VALUE_TO_SIMPLE_ARRAY(val_array, value, value_type, allocator)                  \
  do {                                                                                      \
    if (NULL == val_array->values) {                                                        \
      /* 如果数组为空，初始化数组并设置大小为1 */                         \
      val_array->values = value;                                                            \
      val_array->size = 1;                                                                  \
    } else {                                                                                \
      /* 增加数组大小并添加新值 */                                               \
      value_type * tmp_arr = val_array->values;                                             \
      val_array->values =                                                                   \
        allocator.zero_allocate(val_array->size + 1U, sizeof(value_type), allocator.state); \
      if (NULL == val_array->values) {                                                      \
        /* 如果分配内存失败，恢复原始数组并返回错误 */                  \
        val_array->values = tmp_arr;                                                        \
        RCUTILS_SAFE_FWRITE_TO_STDERR("Error allocating mem\n");                            \
        return RCUTILS_RET_BAD_ALLOC;                                                       \
      }                                                                                     \
      /* 将原始数组的内容复制到新数组 */                                      \
      memcpy(val_array->values, tmp_arr, (val_array->size * sizeof(value_type)));           \
      /* 将新值添加到新数组末尾 */                                               \
      val_array->values[val_array->size] = *value;                                          \
      /* 更新数组大小 */                                                              \
      val_array->size++;                                                                    \
      /* 释放分配的内存 */                                                           \
      allocator.deallocate(value, allocator.state);                                         \
      allocator.deallocate(tmp_arr, allocator.state);                                       \
    }                                                                                       \
    return RCUTILS_RET_OK;                                                                  \
  } while (0)

/**
 * @brief 向布尔数组中添加值。如果数组不存在，则创建数组
 *
 * @param[in,out] val_array 指向布尔数组的指针
 * @param[in] value 要添加到数组的布尔值指针
 * @param[in] allocator 分配器
 * @return rcutils_ret_t 返回操作结果
 */
rcutils_ret_t add_val_to_bool_arr(
  rcl_bool_array_t * const val_array, bool * value, const rcutils_allocator_t allocator)
{
  // 检查val_array是否为空，如果为空则返回无效参数错误
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(val_array, RCUTILS_RET_INVALID_ARGUMENT);
  // 检查value是否为空，如果为空则返回无效参数错误
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(value, RCUTILS_RET_INVALID_ARGUMENT);
  // 检查分配器是否有效，如果无效则返回无效参数错误
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(
    &allocator, "invalid allocator", return RCUTILS_RET_INVALID_ARGUMENT);

  // 将值添加到简单数组中
  ADD_VALUE_TO_SIMPLE_ARRAY(val_array, value, bool, allocator);
}

/**
 * @brief 向整数数组中添加值。如果数组不存在，则创建数组
 *
 * @param[in,out] val_array 指向整数数组的指针
 * @param[in] value 要添加到数组的整数值指针
 * @param[in] allocator 分配器
 * @return rcutils_ret_t 返回操作结果
 */
rcutils_ret_t add_val_to_int_arr(
  rcl_int64_array_t * const val_array, int64_t * value, const rcutils_allocator_t allocator)
{
  // 检查val_array是否为空，如果为空则返回无效参数错误
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(val_array, RCUTILS_RET_INVALID_ARGUMENT);
  // 检查value是否为空，如果为空则返回无效参数错误
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(value, RCUTILS_RET_INVALID_ARGUMENT);
  // 检查分配器是否有效，如果无效则返回无效参数错误
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(
    &allocator, "invalid allocator", return RCUTILS_RET_INVALID_ARGUMENT);

  // 将值添加到简单数组中
  ADD_VALUE_TO_SIMPLE_ARRAY(val_array, value, int64_t, allocator);
}

/**
 * @brief 向双精度浮点数数组中添加值。如果数组不存在，则创建数组
 *
 * @param[in,out] val_array 指向双精度浮点数数组的指针
 * @param[in] value 要添加到数组的双精度浮点数值指针
 * @param[in] allocator 分配器
 * @return rcutils_ret_t 返回操作结果
 */
rcutils_ret_t add_val_to_double_arr(
  rcl_double_array_t * const val_array, double * value, const rcutils_allocator_t allocator)
{
  // 检查val_array是否为空，如果为空则返回无效参数错误
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(val_array, RCUTILS_RET_INVALID_ARGUMENT);
  // 检查value是否为空，如果为空则返回无效参数错误
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(value, RCUTILS_RET_INVALID_ARGUMENT);
  // 检查分配器是否有效，如果无效则返回无效参数错误
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(
    &allocator, "invalid allocator", return RCUTILS_RET_INVALID_ARGUMENT);

  // 将值添加到简单数组中
  ADD_VALUE_TO_SIMPLE_ARRAY(val_array, value, double, allocator);
}

/**
 * @brief 向字符串数组中添加值。如果数组不存在，则创建数组
 *
 * @param[in,out] val_array 指向字符串数组的指针
 * @param[in] value 要添加到数组的字符串值指针
 * @param[in] allocator 分配器
 * @return rcutils_ret_t 返回操作结果
 */
rcutils_ret_t add_val_to_string_arr(
  rcutils_string_array_t * const val_array, char * value, const rcutils_allocator_t allocator)
{
  // 检查val_array是否为空，如果为空则返回无效参数错误
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(val_array, RCUTILS_RET_INVALID_ARGUMENT);
  // 检查value是否为空，如果为空则返回无效参数错误
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(value, RCUTILS_RET_INVALID_ARGUMENT);
  // 检查分配器是否有效，如果无效则返回无效参数错误
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(
    &allocator, "invalid allocator", return RCUTILS_RET_INVALID_ARGUMENT);

  // 如果数组数据为空，则初始化字符串数组并将值添加到数组中
  if (NULL == val_array->data) {
    rcutils_ret_t ret = rcutils_string_array_init(val_array, 1, &allocator);
    if (RCUTILS_RET_OK != ret) {
      return ret;
    }
    val_array->data[0U] = value;
  } else {
    // 增加数组大小并添加新值
    char ** new_string_arr_ptr = allocator.reallocate(
      val_array->data, ((val_array->size + 1U) * sizeof(char *)), allocator.state);
    if (NULL == new_string_arr_ptr) {
      RCUTILS_SAFE_FWRITE_TO_STDERR("Error allocating mem\n");
      return RCUTILS_RET_BAD_ALLOC;
    }
    val_array->data = new_string_arr_ptr;
    val_array->data[val_array->size] = value;
    val_array->size++;
  }
  return RCUTILS_RET_OK;
}
