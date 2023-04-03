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

#include "./impl/yaml_variant.h"

#include "./impl/types.h"
#include "rcl_yaml_param_parser/types.h"
#include "rcutils/allocator.h"
#include "rcutils/error_handling.h"
#include "rcutils/strdup.h"
#include "rcutils/types/string_array.h"

/** 
 * @brief 宏定义：复制一个变量值到另一个变量
 * @param dest_ptr 目标指针，用于存储复制后的值
 * @param src_ptr 源指针，用于读取需要复制的值
 * @param allocator 分配器，用于分配内存空间
 * @param var_type 变量类型，用于确定分配内存大小
 */
#define RCL_YAML_VARIANT_COPY_VALUE(dest_ptr, src_ptr, allocator, var_type)         \
  do {                                                                              \
    /* 使用分配器为目标指针分配内存 */                                               \
    dest_ptr = allocator.allocate(sizeof(var_type), allocator.state);               \
    /* 判断分配是否成功 */                                                           \
    if (NULL == dest_ptr) {                                                         \
      /* 分配失败时输出错误信息 */                                                   \
      RCUTILS_SAFE_FWRITE_TO_STDERR(                                                \
        "Error allocating variant mem when copying value of type " #var_type "\n"); \
      /* 返回false表示操作失败 */                                                    \
      return false;                                                                 \
    }                                                                               \
    /* 将源指针的值复制到目标指针 */                                                  \
    *(dest_ptr) = *(src_ptr);                                                       \
  } while (0)

/** 
 * @brief 宏定义：复制一个数组值到另一个数组
 * @param dest_array 目标数组，用于存储复制后的数组
 * @param src_array 源数组，用于读取需要复制的数组
 * @param allocator 分配器，用于分配内存空间
 * @param var_array_type 数组类型，用于确定分配内存大小
 * @param var_type 变量类型，用于确定数组元素的内存大小
 */
#define RCL_YAML_VARIANT_COPY_ARRAY_VALUE(                                                       \
  dest_array, src_array, allocator, var_array_type, var_type)                                    \
  do {                                                                                           \
    /* 使用分配器为目标数组分配内存 */                                                            \
    dest_array = allocator.allocate(sizeof(var_array_type), allocator.state);                    \
    /* 判断分配是否成功 */                                                                        \
    if (NULL == dest_array) {                                                                    \
      /* 分配失败时输出错误信息 */                                                                \
      RCUTILS_SAFE_FWRITE_TO_STDERR("Error allocating mem for array of type " #var_array_type    \
                                    "\n");                                                       \
      /* 返回false表示操作失败 */                                                                 \
      return false;                                                                              \
    }                                                                                            \
    /* 判断源数组大小是否大于0 */                                                                 \
    if (0U != src_array->size) {                                                                 \
      /* 为目标数组的值分配内存空间 */                                                             \
      dest_array->values =                                                                       \
        allocator.allocate(sizeof(var_type) * src_array->size, allocator.state);                 \
      /* 判断分配是否成功 */                                                                       \
      if (NULL == dest_array->values) {                                                          \
        /* 分配失败时输出错误信息 */                                                               \
        RCUTILS_SAFE_FWRITE_TO_STDERR("Error allocating mem for array values of type " #var_type \
                                      "\n");                                                     \
        /* 返回false表示操作失败 */                                                                \
        return false;                                                                            \
      }                                                                                          \
      /* 将源数组的值复制到目标数组 */                                                             \
      memcpy(dest_array->values, src_array->values, sizeof(var_type) * src_array->size);         \
    } else {                                                                                     \
      /* 源数组大小为0时，目标数组的值指针设为NULL */                                              \
      dest_array->values = NULL;                                                                 \
    }                                                                                            \
    /* 设置目标数组的大小为源数组的大小 */                                                         \
    dest_array->size = src_array->size;                                                          \
  } while (0)

/**
 * @brief 释放 rcl_variant_t 结构体中的动态内存并将指针置空
 *
 * @param[in,out] param_var 指向 rcl_variant_t 结构体的指针，该结构体包含了需要释放的动态内存
 * @param[in] allocator 用于分配和释放内存的 rcutils_allocator_t 结构体实例
 */
void rcl_yaml_variant_fini(rcl_variant_t * param_var, const rcutils_allocator_t allocator)
{
  // 如果 param_var 为空，则直接返回
  if (NULL == param_var) {
    return;
  }

  // 如果 bool_value 不为空，则释放其内存并将指针置空
  if (NULL != param_var->bool_value) {
    allocator.deallocate(param_var->bool_value, allocator.state);
    param_var->bool_value = NULL;
  }
  // 如果 integer_value 不为空，则释放其内存并将指针置空
  else if (NULL != param_var->integer_value) {
    allocator.deallocate(param_var->integer_value, allocator.state);
    param_var->integer_value = NULL;
  }
  // 如果 double_value 不为空，则释放其内存并将指针置空
  else if (NULL != param_var->double_value) {
    allocator.deallocate(param_var->double_value, allocator.state);
    param_var->double_value = NULL;
  }
  // 如果 string_value 不为空，则释放其内存并将指针置空
  else if (NULL != param_var->string_value) {
    allocator.deallocate(param_var->string_value, allocator.state);
    param_var->string_value = NULL;
  }
  // 如果 bool_array_value 不为空，则释放其内存并将指针置空
  else if (NULL != param_var->bool_array_value) {
    if (NULL != param_var->bool_array_value->values) {
      allocator.deallocate(param_var->bool_array_value->values, allocator.state);
    }
    allocator.deallocate(param_var->bool_array_value, allocator.state);
    param_var->bool_array_value = NULL;
  }
  // 如果 integer_array_value 不为空，则释放其内存并将指针置空
  else if (NULL != param_var->integer_array_value) {
    if (NULL != param_var->integer_array_value->values) {
      allocator.deallocate(param_var->integer_array_value->values, allocator.state);
    }
    allocator.deallocate(param_var->integer_array_value, allocator.state);
    param_var->integer_array_value = NULL;
  }
  // 如果 double_array_value 不为空，则释放其内存并将指针置空
  else if (NULL != param_var->double_array_value) {
    if (NULL != param_var->double_array_value->values) {
      allocator.deallocate(param_var->double_array_value->values, allocator.state);
    }
    allocator.deallocate(param_var->double_array_value, allocator.state);
    param_var->double_array_value = NULL;
  }
  // 如果 string_array_value 不为空，则释放其内存并将指针置空
  else if (NULL != param_var->string_array_value) {
    if (RCUTILS_RET_OK != rcutils_string_array_fini(param_var->string_array_value)) {
      // Log and continue ...
      RCUTILS_SAFE_FWRITE_TO_STDERR("Error deallocating string array");
    }
    allocator.deallocate(param_var->string_array_value, allocator.state);
    param_var->string_array_value = NULL;
  } else {
    // 如果没有需要释放的内存，则什么都不做
  }
}

/**
 * @brief 复制 rcl_variant_t 类型的参数值
 *
 * 该函数用于复制 rcl_variant_t 类型的参数值，支持多种数据类型。
 *
 * @param[out] out_param_var 输出参数，用于存储复制后的参数值
 * @param[in] param_var 输入参数，需要被复制的参数值
 * @param[in] allocator 分配器，用于分配内存
 * @return 成功返回 true，失败返回 false
 */
bool rcl_yaml_variant_copy(
  rcl_variant_t * out_param_var, const rcl_variant_t * param_var, rcutils_allocator_t allocator)
{
  // 检查输入参数和输出参数是否为空
  if (NULL == param_var || NULL == out_param_var) {
    return false;
  }
  // 复制布尔值
  if (NULL != param_var->bool_value) {
    RCL_YAML_VARIANT_COPY_VALUE(out_param_var->bool_value, param_var->bool_value, allocator, bool);
  // 复制整数值
  } else if (NULL != param_var->integer_value) {
    RCL_YAML_VARIANT_COPY_VALUE(
      out_param_var->integer_value, param_var->integer_value, allocator, int64_t);
  // 复制浮点数值
  } else if (NULL != param_var->double_value) {
    RCL_YAML_VARIANT_COPY_VALUE(
      out_param_var->double_value, param_var->double_value, allocator, double);
  // 复制字符串值
  } else if (NULL != param_var->string_value) {
    out_param_var->string_value = rcutils_strdup(param_var->string_value, allocator);
    if (NULL == out_param_var->string_value) {
      RCUTILS_SAFE_FWRITE_TO_STDERR("Error allocating variant mem when copying string_value\n");
      return false;
    }
  // 复制布尔数组值
  } else if (NULL != param_var->bool_array_value) {
    RCL_YAML_VARIANT_COPY_ARRAY_VALUE(
      out_param_var->bool_array_value, param_var->bool_array_value, allocator, rcl_bool_array_t,
      bool);
  // 复制整数数组值
  } else if (NULL != param_var->integer_array_value) {
    RCL_YAML_VARIANT_COPY_ARRAY_VALUE(
      out_param_var->integer_array_value, param_var->integer_array_value, allocator,
      rcl_int64_array_t, int64_t);
  // 复制浮点数数组值
  } else if (NULL != param_var->double_array_value) {
    RCL_YAML_VARIANT_COPY_ARRAY_VALUE(
      out_param_var->double_array_value, param_var->double_array_value, allocator,
      rcl_double_array_t, double);
  // 复制字符串数组值
  } else if (NULL != param_var->string_array_value) {
    out_param_var->string_array_value =
      allocator.allocate(sizeof(rcutils_string_array_t), allocator.state);
    if (NULL == out_param_var->string_array_value) {
      RCUTILS_SAFE_FWRITE_TO_STDERR("Error allocating mem\n");
      return false;
    }
    *(out_param_var->string_array_value) = rcutils_get_zero_initialized_string_array();
    rcutils_ret_t ret = rcutils_string_array_init(
      out_param_var->string_array_value, param_var->string_array_value->size,
      &(param_var->string_array_value->allocator));
    if (RCUTILS_RET_OK != ret) {
      if (RCUTILS_RET_BAD_ALLOC == ret) {
        RCUTILS_SAFE_FWRITE_TO_STDERR("Error allocating mem for string array\n");
      }
      return false;
    }
    // 复制字符串数组中的每个字符串
    for (size_t str_idx = 0U; str_idx < param_var->string_array_value->size; ++str_idx) {
      out_param_var->string_array_value->data[str_idx] = rcutils_strdup(
        param_var->string_array_value->data[str_idx], out_param_var->string_array_value->allocator);
      if (NULL == out_param_var->string_array_value->data[str_idx]) {
        RCUTILS_SAFE_FWRITE_TO_STDERR("Error allocating mem for string array values\n");
        return false;
      }
    }
  } else {
    /// Nothing to do to keep pclint happy
  }
  return true;
}
