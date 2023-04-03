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

#include "./impl/parse.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#include "./impl/add_to_arrays.h"
#include "./impl/namespace.h"
#include "./impl/node_params.h"
#include "rcl_yaml_param_parser/parser.h"
#include "rcl_yaml_param_parser/visibility_control.h"
#include "rcutils/allocator.h"
#include "rcutils/error_handling.h"
#include "rcutils/format_string.h"
#include "rcutils/strdup.h"
#include "rcutils/types/rcutils_ret.h"
#include "rcutils/types/string_array.h"
#include "rmw/error_handling.h"
#include "rmw/validate_namespace.h"
#include "rmw/validate_node_name.h"

/// \brief 检查一个命名空间是否有效
///
/// \param[in] namespace 要检查的命名空间
/// \return RCUTILS_RET_OK 如果命名空间有效，或者
/// \return RCUTILS_RET_INVALID_ARGUMENT 如果命名空间无效，或者
/// \return RCUTILS_RET_ERROR 如果发生了未指定的错误。
RCL_YAML_PARAM_PARSER_LOCAL
rcutils_ret_t _validate_namespace(const char * namespace_);

/// \brief 检查一个节点名称是否有效
///
/// \param[in] name 要检查的节点名称
/// \return RCUTILS_RET_OK 如果节点名称有效，或者
/// \return RCUTILS_RET_INVALID_ARGUMENT 如果节点名称无效，或者
/// \return RCUTILS_RET_ERROR 如果发生了未指定的错误。
RCL_YAML_PARAM_PARSER_LOCAL
rcutils_ret_t _validate_nodename(const char * node_name);

/// \brief 检查一个名称（命名空间/节点名称）是否有效
///
/// \param name 要检查的名称
/// \param allocator 要使用的分配器
/// \return RCUTILS_RET_OK 如果名称有效，或者
/// \return RCUTILS_RET_INVALID_ARGUMENT 如果名称无效，或者
/// \return RCL_RET_BAD_ALLOC 如果分配失败，或者
/// \return RCUTILS_RET_ERROR 如果发生了未指定的错误。
RCL_YAML_PARAM_PARSER_LOCAL
rcutils_ret_t _validate_name(const char * name, rcutils_allocator_t allocator);

/**
 * @brief 确定值的类型并返回转换后的值
 * 注意：目前仅支持规范形式
 *
 * @param[in] value 输入值，需要确定类型并进行转换
 * @param[in] style YAML标量样式
 * @param[in] tag YAML标签
 * @param[out] val_type 输出值的数据类型
 * @param[in] allocator 分配器用于分配内存
 * @return 转换后的值（void* 类型）
 */
void * get_value(
  const char * const value, yaml_scalar_style_t style, const yaml_char_t * const tag,
  data_types_t * val_type, const rcutils_allocator_t allocator)
{
  void * ret_val;
  int64_t ival;
  double dval;
  char * endptr = NULL;

  // 检查输入参数是否为空
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(value, NULL);
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(val_type, NULL);
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(&allocator, "allocator is invalid", return NULL);

  // 检查yaml字符串标签
  if (tag != NULL && strcmp(YAML_STR_TAG, (char *)tag) == 0) {
    *val_type = DATA_TYPE_STRING;
    return rcutils_strdup(value, allocator);
  }

  // 检查是否为布尔值
  if (style != YAML_SINGLE_QUOTED_SCALAR_STYLE && style != YAML_DOUBLE_QUOTED_SCALAR_STYLE) {
    if (
      (0 == strcmp(value, "Y")) || (0 == strcmp(value, "y")) || (0 == strcmp(value, "yes")) ||
      (0 == strcmp(value, "Yes")) || (0 == strcmp(value, "YES")) || (0 == strcmp(value, "true")) ||
      (0 == strcmp(value, "True")) || (0 == strcmp(value, "TRUE")) || (0 == strcmp(value, "on")) ||
      (0 == strcmp(value, "On")) || (0 == strcmp(value, "ON"))) {
      *val_type = DATA_TYPE_BOOL;
      ret_val = allocator.zero_allocate(1U, sizeof(bool), allocator.state);
      if (NULL == ret_val) {
        return NULL;
      }
      *((bool *)ret_val) = true;
      return ret_val;
    }

    if (
      (0 == strcmp(value, "N")) || (0 == strcmp(value, "n")) || (0 == strcmp(value, "no")) ||
      (0 == strcmp(value, "No")) || (0 == strcmp(value, "NO")) || (0 == strcmp(value, "false")) ||
      (0 == strcmp(value, "False")) || (0 == strcmp(value, "FALSE")) ||
      (0 == strcmp(value, "off")) || (0 == strcmp(value, "Off")) || (0 == strcmp(value, "OFF"))) {
      *val_type = DATA_TYPE_BOOL;
      ret_val = allocator.zero_allocate(1U, sizeof(bool), allocator.state);
      if (NULL == ret_val) {
        return NULL;
      }
      *((bool *)ret_val) = false;
      return ret_val;
    }
  }

  // 检查是否为整数
  if (style != YAML_SINGLE_QUOTED_SCALAR_STYLE && style != YAML_DOUBLE_QUOTED_SCALAR_STYLE) {
    errno = 0;
    ival = strtol(value, &endptr, 0);
    if ((0 == errno) && (NULL != endptr)) {
      if ((NULL != endptr) && (endptr != value)) {
        if (('\0' != *value) && ('\0' == *endptr)) {
          *val_type = DATA_TYPE_INT64;
          ret_val = allocator.zero_allocate(1U, sizeof(int64_t), allocator.state);
          if (NULL == ret_val) {
            return NULL;
          }
          *((int64_t *)ret_val) = ival;
          return ret_val;
        }
      }
    }
  }

  // 检查是否为浮点数
  if (style != YAML_SINGLE_QUOTED_SCALAR_STYLE && style != YAML_DOUBLE_QUOTED_SCALAR_STYLE) {
    errno = 0;
    endptr = NULL;
    const char * iter_ptr = NULL;
    if (
      (0 == strcmp(value, ".nan")) || (0 == strcmp(value, ".NaN")) ||
      (0 == strcmp(value, ".NAN")) || (0 == strcmp(value, ".inf")) ||
      (0 == strcmp(value, ".Inf")) || (0 == strcmp(value, ".INF")) ||
      (0 == strcmp(value, "+.inf")) || (0 == strcmp(value, "+.Inf")) ||
      (0 == strcmp(value, "+.INF")) || (0 == strcmp(value, "-.inf")) ||
      (0 == strcmp(value, "-.Inf")) || (0 == strcmp(value, "-.INF"))) {
      for (iter_ptr = value; !isalpha(*iter_ptr);) {
        iter_ptr += 1;
      }
      dval = strtod(iter_ptr, &endptr);
      if (*value == '-') {
        dval = -dval;
      }
    } else {
      dval = strtod(value, &endptr);
    }
    if ((0 == errno) && (NULL != endptr)) {
      if ((NULL != endptr) && (endptr != value)) {
        if (('\0' != *value) && ('\0' == *endptr)) {
          *val_type = DATA_TYPE_DOUBLE;
          ret_val = allocator.zero_allocate(1U, sizeof(double), allocator.state);
          if (NULL == ret_val) {
            return NULL;
          }
          *((double *)ret_val) = dval;
          return ret_val;
        }
      }
    }
    errno = 0;
  }

  // 如果不是以上类型，则为字符串
  *val_type = DATA_TYPE_STRING;
  ret_val = rcutils_strdup(value, allocator);
  return ret_val;
}

/// \brief 解析 <key:value> 对中的值部分
///
/// \param[in] event yaml_event_t 事件，包含要解析的值信息
/// \param[in] is_seq 布尔值，表示当前值是否属于序列
/// \param[in] node_idx 节点索引，用于更新参数
/// \param[in] parameter_idx 参数索引，用于更新参数
/// \param[out] seq_data_type 指向数据类型的指针，用于存储序列的数据类型
/// \param[in,out] params_st rcl_params_t 结构体指针，用于存储解析后的参数
/// \return rcutils_ret_t 返回操作结果
rcutils_ret_t parse_value(
  const yaml_event_t event, const bool is_seq, const size_t node_idx, const size_t parameter_idx,
  data_types_t * seq_data_type, rcl_params_t * params_st)
{
  // 检查 seq_data_type 是否为空
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(seq_data_type, RCUTILS_RET_INVALID_ARGUMENT);
  // 检查 params_st 是否为空
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(params_st, RCUTILS_RET_INVALID_ARGUMENT);

  // 获取分配器
  rcutils_allocator_t allocator = params_st->allocator;
  // 检查分配器是否有效
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(
    &allocator, "invalid allocator", return RCUTILS_RET_INVALID_ARGUMENT);

  // 如果没有节点可供更新，则返回错误
  if (0U == params_st->num_nodes) {
    RCUTILS_SET_ERROR_MSG("No node to update");
    return RCUTILS_RET_INVALID_ARGUMENT;
  }

  // 获取值的大小、值、样式、标签和行号
  const size_t val_size = event.data.scalar.length;
  const char * value = (char *)event.data.scalar.value;
  yaml_scalar_style_t style = event.data.scalar.style;
  const yaml_char_t * const tag = event.data.scalar.tag;
  const uint32_t line_num = ((uint32_t)(event.start_mark.line) + 1U);

  // 检查值是否为空
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    value, "event argument has no value", return RCUTILS_RET_INVALID_ARGUMENT);

  // 如果值的样式不是单引号或双引号，并且值的大小为0，则返回错误
  if (
    style != YAML_SINGLE_QUOTED_SCALAR_STYLE && style != YAML_DOUBLE_QUOTED_SCALAR_STYLE &&
    0U == val_size) {
    RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING("No value at line %d", line_num);
    return RCUTILS_RET_ERROR;
  }

  // 检查参数值数组是否为空，如果为空则返回内存分配错误
  if (NULL == params_st->params[node_idx].parameter_values) {
    RCUTILS_SET_ERROR_MSG("Internal error: Invalid mem");
    return RCUTILS_RET_BAD_ALLOC;
  }

  // 获取参数值并将其存储在 param_value 中
  rcl_variant_t * param_value = &(params_st->params[node_idx].parameter_values[parameter_idx]);
  // 定义一个变量来存储值的数据类型
  data_types_t val_type;
  // 调用 get_value 函数以获取值、数据类型和分配内存
  // value: 参数值字符串
  // style: 参数值的风格
  // tag: 参数值的标签
  // &val_type: 用于存储解析出的数据类型的指针
  // allocator: 分配器用于分配内存
  void * ret_val = get_value(value, style, tag, &val_type, allocator);

  // 检查返回值是否为 NULL，如果为 NULL，则表示解析过程中出现错误
  if (NULL == ret_val) {
    // 设置错误消息，并附加格式化字符串，包括值和行号
    RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING("Error parsing value %s at line %d", value, line_num);
    // 返回错误代码
    return RCUTILS_RET_ERROR;
  }

  /**
 * @brief 根据值类型处理参数值并分配内存
 *
 * @param[in] val_type 参数值的数据类型
 * @param[in] is_seq 是否为序列类型
 * @param[in,out] param_value 参数值结构体指针
 * @param[in] ret_val 返回值指针
 * @param[in,out] seq_data_type 序列数据类型指针
 * @param[in] line_num 当前行号
 * @param[in] allocator 分配器
 * @return rcutils_ret_t 返回状态码
 */
  rcutils_ret_t ret = RCUTILS_RET_OK;
  switch (val_type) {
    case DATA_TYPE_UNKNOWN:
      // 设置错误信息，未知数据类型
      RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Unknown data type of value %s at line %d\n", value, line_num);
      ret = RCUTILS_RET_ERROR;
      break;
    case DATA_TYPE_BOOL:
      if (!is_seq) {
        if (NULL != param_value->bool_value) {
          // 覆盖原始值，释放原始内存
          allocator.deallocate(param_value->bool_value, allocator.state);
        }
        // 分配布尔值内存
        param_value->bool_value = (bool *)ret_val;
      } else {
        if (DATA_TYPE_UNKNOWN == *seq_data_type) {
          // 设置序列数据类型
          *seq_data_type = val_type;
          if (NULL != param_value->bool_array_value) {
            // 释放原始布尔数组内存
            allocator.deallocate(param_value->bool_array_value->values, allocator.state);
            allocator.deallocate(param_value->bool_array_value, allocator.state);
            param_value->bool_array_value = NULL;
          }
          // 分配布尔数组内存
          param_value->bool_array_value =
            allocator.zero_allocate(1U, sizeof(rcl_bool_array_t), allocator.state);
          if (NULL == param_value->bool_array_value) {
            // 分配失败，释放内存并设置错误信息
            allocator.deallocate(ret_val, allocator.state);
            RCUTILS_SAFE_FWRITE_TO_STDERR("Error allocating mem\n");
            ret = RCUTILS_RET_BAD_ALLOC;
            break;
          }
        } else {
          if (*seq_data_type != val_type) {
            // 序列中的数据类型不一致，设置错误信息并释放内存
            RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
              "Sequence should be of same type. Value type 'bool' do not belong at line_num %d",
              line_num);
            allocator.deallocate(ret_val, allocator.state);
            ret = RCUTILS_RET_ERROR;
            break;
          }
        }
        // 将值添加到布尔数组中
        ret = add_val_to_bool_arr(param_value->bool_array_value, ret_val, allocator);
        if (RCUTILS_RET_OK != ret) {
          if (NULL != ret_val) {
            // 添加失败，释放内存
            allocator.deallocate(ret_val, allocator.state);
          }
        }
      }
      break;
    /**
     * @brief 处理 DATA_TYPE_INT64 类型的参数值。
     *
     * @param[in] is_seq 是否为序列类型。
     * @param[in,out] param_value 参数值结构体指针，用于存储解析后的参数值。
     * @param[in] ret_val 解析后的 int64_t 类型值或数组。
     * @param[in,out] seq_data_type 序列数据类型，用于检查序列中的数据类型是否一致。
     * @param[in] val_type 当前解析到的值的数据类型。
     * @param[in] line_num 当前解析到的行号。
     * @param[in] allocator 分配器，用于分配和释放内存。
     * @return 返回 RCUTILS_RET_OK 或错误代码。
     */
    case DATA_TYPE_INT64:
      if (!is_seq) {
        // 如果不是序列类型
        if (NULL != param_value->integer_value) {
          // 如果原始值非空，释放原始值内存
          allocator.deallocate(param_value->integer_value, allocator.state);
        }
        // 将解析后的值赋给参数值结构体
        param_value->integer_value = (int64_t *)ret_val;
      } else {
        // 如果是序列类型
        if (DATA_TYPE_UNKNOWN == *seq_data_type) {
          // 如果序列数据类型未知
          if (NULL != param_value->integer_array_value) {
            // 释放原始数组值内存
            allocator.deallocate(param_value->integer_array_value->values, allocator.state);
            allocator.deallocate(param_value->integer_array_value, allocator.state);
            param_value->integer_array_value = NULL;
          }
          // 设置序列数据类型
          *seq_data_type = val_type;
          // 为整数数组分配内存
          param_value->integer_array_value =
            allocator.zero_allocate(1U, sizeof(rcl_int64_array_t), allocator.state);
          if (NULL == param_value->integer_array_value) {
            // 如果分配失败，释放内存并返回错误
            allocator.deallocate(ret_val, allocator.state);
            RCUTILS_SAFE_FWRITE_TO_STDERR("Error allocating mem\n");
            ret = RCUTILS_RET_BAD_ALLOC;
            break;
          }
        } else {
          // 如果序列数据类型已知
          if (*seq_data_type != val_type) {
            // 如果当前值的数据类型与序列数据类型不一致，返回错误
            RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
              "Sequence should be of same type. Value type 'integer' do not belong at line_num %d",
              line_num);
            allocator.deallocate(ret_val, allocator.state);
            ret = RCUTILS_RET_ERROR;
            break;
          }
        }
        // 将解析后的值添加到整数数组中
        ret = add_val_to_int_arr(param_value->integer_array_value, ret_val, allocator);
        if (RCUTILS_RET_OK != ret) {
          // 如果添加失败，释放内存
          if (NULL != ret_val) {
            allocator.deallocate(ret_val, allocator.state);
          }
        }
      }
      break;
    /**
     * @brief 处理 DATA_TYPE_DOUBLE 类型的参数值。
     *
     * @param[in] is_seq 是否为序列类型。
     * @param[in,out] param_value 参数值结构体指针，用于存储解析后的参数值。
     * @param[in] ret_val 解析后的参数值。
     * @param[in,out] seq_data_type 序列数据类型，用于检查序列中的数据类型是否一致。
     * @param[in] val_type 当前解析到的数据类型。
     * @param[in] line_num 当前解析到的行号。
     * @param[in] allocator 分配器，用于内存分配和释放。
     *
     * @return 返回 RCUTILS_RET_OK 表示成功，其他值表示失败。
     */
    case DATA_TYPE_DOUBLE:
      if (!is_seq) {
        // 如果不是序列类型
        if (NULL != param_value->double_value) {
          // 如果原始值不为空，释放原始值内存
          allocator.deallocate(param_value->double_value, allocator.state);
        }
        // 将解析后的值赋给参数值结构体
        param_value->double_value = (double *)ret_val;
      } else {
        // 如果是序列类型
        if (DATA_TYPE_UNKNOWN == *seq_data_type) {
          // 如果序列数据类型未知
          if (NULL != param_value->double_array_value) {
            // 释放原始数组值内存
            allocator.deallocate(param_value->double_array_value->values, allocator.state);
            allocator.deallocate(param_value->double_array_value, allocator.state);
            param_value->double_array_value = NULL;
          }
          // 设置序列数据类型
          *seq_data_type = val_type;
          // 为参数值结构体分配内存
          param_value->double_array_value =
            allocator.zero_allocate(1U, sizeof(rcl_double_array_t), allocator.state);
          if (NULL == param_value->double_array_value) {
            // 如果分配失败，释放内存并返回错误
            allocator.deallocate(ret_val, allocator.state);
            RCUTILS_SAFE_FWRITE_TO_STDERR("Error allocating mem\n");
            ret = RCUTILS_RET_BAD_ALLOC;
            break;
          }
        } else {
          // 如果序列数据类型已知
          if (*seq_data_type != val_type) {
            // 如果当前值的数据类型与序列数据类型不一致，返回错误
            RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
              "Sequence should be of same type. Value type 'double' do not belong at line_num %d",
              line_num);
            allocator.deallocate(ret_val, allocator.state);
            ret = RCUTILS_RET_ERROR;
            break;
          }
        }
        // 将解析后的值添加到 double 数组中
        ret = add_val_to_double_arr(param_value->double_array_value, ret_val, allocator);
        if (RCUTILS_RET_OK != ret) {
          // 如果添加失败，释放内存
          if (NULL != ret_val) {
            allocator.deallocate(ret_val, allocator.state);
          }
        }
      }
      break;
    /**
     * @brief 处理字符串类型的参数值
     *
     * @param[in] is_seq 是否为序列类型
     * @param[in,out] param_value 参数值结构体指针，用于存储解析后的参数值
     * @param[in] ret_val 解析得到的字符串值
     * @param[in,out] seq_data_type 序列数据类型，用于检查序列中的数据类型是否一致
     * @param[in] val_type 当前解析到的值的数据类型
     * @param[in] line_num 当前解析到的行号，用于错误提示
     * @param[in] allocator 分配器，用于内存分配和释放
     * @return rcutils_ret_t 返回处理结果，成功返回RCUTILS_RET_OK，失败返回相应的错误码
     */
    case DATA_TYPE_STRING:
      if (!is_seq) {
        // 如果不是序列类型
        if (NULL != param_value->string_value) {
          // 如果原始字符串值不为空，释放原始字符串值
          allocator.deallocate(param_value->string_value, allocator.state);
        }
        // 将解析得到的字符串值赋给参数值结构体
        param_value->string_value = (char *)ret_val;
      } else {
        // 如果是序列类型
        if (DATA_TYPE_UNKNOWN == *seq_data_type) {
          // 如果序列数据类型未知
          if (NULL != param_value->string_array_value) {
            // 如果字符串数组值不为空，释放字符串数组值
            if (RCUTILS_RET_OK != rcutils_string_array_fini(param_value->string_array_value)) {
              // 如果释放失败，记录日志并继续
              RCUTILS_SAFE_FWRITE_TO_STDERR("Error deallocating string array");
            }
            allocator.deallocate(param_value->string_array_value, allocator.state);
            param_value->string_array_value = NULL;
          }
          // 设置序列数据类型为当前值的数据类型
          *seq_data_type = val_type;
          // 为字符串数组值分配内存
          param_value->string_array_value =
            allocator.zero_allocate(1U, sizeof(rcutils_string_array_t), allocator.state);
          if (NULL == param_value->string_array_value) {
            // 如果分配失败，释放已分配的内存并返回错误
            allocator.deallocate(ret_val, allocator.state);
            RCUTILS_SAFE_FWRITE_TO_STDERR("Error allocating mem\n");
            ret = RCUTILS_RET_BAD_ALLOC;
            break;
          }
        } else {
          // 如果序列数据类型已知
          if (*seq_data_type != val_type) {
            // 如果当前值的数据类型与序列数据类型不一致，返回错误
            RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
              "Sequence should be of same type. Value type 'string' do not belong at line_num %d",
              line_num);
            allocator.deallocate(ret_val, allocator.state);
            ret = RCUTILS_RET_ERROR;
            break;
          }
        }
        // 将解析得到的字符串值添加到字符串数组中
        ret = add_val_to_string_arr(param_value->string_array_value, ret_val, allocator);
        if (RCUTILS_RET_OK != ret) {
          // 如果添加失败，释放已分配的内存
          if (NULL != ret_val) {
            allocator.deallocate(ret_val, allocator.state);
          }
        }
      }
      break;
    default:
      // 设置错误消息，提示未知的数据类型
      RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Unknown data type of value %s at line %d", value, line_num);
      // 返回错误代码
      ret = RCUTILS_RET_ERROR;
      // 使用分配器释放内存
      allocator.deallocate(ret_val, allocator.state);
      break;
  }
  return ret;
}

/**
 * @brief 验证命名空间是否有效
 *
 * @param[in] namespace_ 要验证的命名空间字符串
 * @return rcutils_ret_t 返回验证结果，包括 RCUTILS_RET_OK, RCUTILS_RET_ERROR 和 RCUTILS_RET_INVALID_ARGUMENT
 */
rcutils_ret_t _validate_namespace(const char * namespace_)
{
  int validation_result = 0;  // 初始化验证结果变量
  rmw_ret_t ret;              // 定义rmw返回类型变量
  ret = rmw_validate_namespace(namespace_, &validation_result, NULL);  // 调用rmw验证命名空间函数
  if (RMW_RET_OK != ret) {  // 如果rmw验证函数返回值不是RMW_RET_OK
    RCUTILS_SET_ERROR_MSG(rmw_get_error_string().str);  // 设置错误信息
    return RCUTILS_RET_ERROR;                           // 返回RCUTILS_RET_ERROR
  }
  if (RMW_NAMESPACE_VALID != validation_result) {  // 如果验证结果不是RMW_NAMESPACE_VALID
    RCUTILS_SET_ERROR_MSG(
      rmw_namespace_validation_result_string(validation_result));  // 设置错误信息
    return RCUTILS_RET_INVALID_ARGUMENT;  // 返回RCUTILS_RET_INVALID_ARGUMENT
  }

  return RCUTILS_RET_OK;  // 返回RCUTILS_RET_OK表示验证成功
}

/**
 * @brief 验证节点名称是否有效
 *
 * @param[in] node_name 要验证的节点名称字符串
 * @return rcutils_ret_t 返回验证结果，包括 RCUTILS_RET_OK, RCUTILS_RET_ERROR 和 RCUTILS_RET_INVALID_ARGUMENT
 */
rcutils_ret_t _validate_nodename(const char * node_name)
{
  int validation_result = 0;                                          // 初始化验证结果变量
  rmw_ret_t ret;                                                      // 定义rmw返回类型变量
  ret = rmw_validate_node_name(node_name, &validation_result, NULL);  // 调用rmw验证节点名称函数
  if (RMW_RET_OK != ret) {  // 如果rmw验证函数返回值不是RMW_RET_OK
    RCUTILS_SET_ERROR_MSG(rmw_get_error_string().str);  // 设置错误信息
    return RCUTILS_RET_ERROR;                           // 返回RCUTILS_RET_ERROR
  }
  if (RMW_NODE_NAME_VALID != validation_result) {  // 如果验证结果不是RMW_NODE_NAME_VALID
    RCUTILS_SET_ERROR_MSG(
      rmw_node_name_validation_result_string(validation_result));  // 设置错误信息
    return RCUTILS_RET_INVALID_ARGUMENT;  // 返回RCUTILS_RET_INVALID_ARGUMENT
  }

  return RCUTILS_RET_OK;  // 返回RCUTILS_RET_OK表示验证成功
}

/**
 * @brief 验证给定的名称是否符合ROS2命名规范
 *
 * @param[in] name 要验证的名称字符串
 * @param[in] allocator 用于分配内存的rcutils_allocator_t实例
 * @return rcutils_ret_t 返回RCUTILS_RET_OK表示验证通过，其他值表示验证失败
 */
rcutils_ret_t _validate_name(const char * name, rcutils_allocator_t allocator)
{
  // 特殊规则：如果名称为"/**"或"/*"，则直接返回RCUTILS_RET_OK
  if (0 == strcmp(name, "/**") || 0 == strcmp(name, "/*")) {
    return RCUTILS_RET_OK;
  }

  rcutils_ret_t ret = RCUTILS_RET_OK;
  // 查找名称中最后一个'/'的位置
  char * separator_pos = strrchr(name, '/');
  char * node_name = NULL;
  char * absolute_namespace = NULL;
  // 如果没有找到'/'，则将整个名称作为节点名称
  if (NULL == separator_pos) {
    node_name = rcutils_strdup(name, allocator);
    if (NULL == node_name) {
      ret = RCUTILS_RET_BAD_ALLOC;
      goto clean;
    }
  } else {
    // 包含最后一个'/'的子字符串命名空间
    char * namespace_ = rcutils_strndup(name, ((size_t)(separator_pos - name)) + 1, allocator);
    if (NULL == namespace_) {
      ret = RCUTILS_RET_BAD_ALLOC;
      goto clean;
    }
    // 如果命名空间不是以'/'开头，则添加'/'使其成为绝对命名空间
    if (namespace_[0] != '/') {
      absolute_namespace = rcutils_format_string(allocator, "/%s", namespace_);
      allocator.deallocate(namespace_, allocator.state);
      if (NULL == absolute_namespace) {
        ret = RCUTILS_RET_BAD_ALLOC;
        goto clean;
      }
    } else {
      absolute_namespace = namespace_;
    }

    // 将'/'之后的部分作为节点名称
    node_name = rcutils_strdup(separator_pos + 1, allocator);
    if (NULL == node_name) {
      ret = RCUTILS_RET_BAD_ALLOC;
      goto clean;
    }
  }

  // 如果存在绝对命名空间，则验证其有效性
  if (absolute_namespace) {
    size_t i = 0;
    separator_pos = strchr(absolute_namespace + i + 1, '/');
    if (NULL == separator_pos) {
      ret = _validate_namespace(absolute_namespace);
      if (RCUTILS_RET_OK != ret) {
        goto clean;
      }
    } else {
      do {
        size_t len = ((size_t)(separator_pos - absolute_namespace)) - i;
        char * namespace_ = rcutils_strndup(absolute_namespace + i, len, allocator);
        if (NULL == namespace_) {
          ret = RCUTILS_RET_BAD_ALLOC;
          goto clean;
        }
        // 检查是否存在重复的'/'
        if (0 == strcmp(namespace_, "/")) {
          RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
            "%s contains repeated forward slash", absolute_namespace);
          allocator.deallocate(namespace_, allocator.state);
          ret = RCUTILS_RET_INVALID_ARGUMENT;
          goto clean;
        }
        // 验证子命名空间的有效性，除非它是"/**"或"/*"
        if (0 != strcmp(namespace_, "/**") && 0 != strcmp(namespace_, "/*")) {
          ret = _validate_namespace(namespace_);
          if (RCUTILS_RET_OK != ret) {
            allocator.deallocate(namespace_, allocator.state);
            goto clean;
          }
        }
        allocator.deallocate(namespace_, allocator.state);
        i += len;
      } while (NULL != (separator_pos = strchr(absolute_namespace + i + 1, '/')));
    }
  }

  if (0 != strcmp(node_name, "*") && 0 != strcmp(node_name, "**")) {
    ret = _validate_nodename(node_name);
    if (RCUTILS_RET_OK != ret) {
      goto clean;
    }
  }

/**
 * @brief 清理内存并返回结果
 *
 * 该函数用于释放分配给 absolute_namespace 和 node_name 的内存，并返回结果。
 *
 * @param[in] absolute_namespace 指向绝对命名空间的指针，如果已分配内存，则需要释放。
 * @param[in] node_name 指向节点名称的指针，如果已分配内存，则需要释放。
 * @param[in] allocator 分配器对象，用于处理内存分配和释放。
 * @param[out] ret 返回结果，通常为 RCL_RET_OK 或其他错误代码。
 *
 * @return 返回 ret 参数的值。
 */
clean:
  // 如果 absolute_namespace 已分配内存，则释放它
  if (absolute_namespace) {
    allocator.deallocate(absolute_namespace, allocator.state);
  }
  // 如果 node_name 已分配内存，则释放它
  if (node_name) {
    allocator.deallocate(node_name, allocator.state);
  }
  // 返回结果
  return ret;
}

/**
 * @brief 解析 <key:value> 对中的 key 部分
 *
 * @param[in] event yaml_event_t 事件
 * @param[in,out] map_level 当前映射层级
 * @param[in,out] is_new_map 是否为新映射
 * @param[in,out] node_idx 节点索引
 * @param[in,out] parameter_idx 参数索引
 * @param[in,out] ns_tracker 命名空间跟踪器
 * @param[in,out] params_st 参数存储结构体
 * @return rcutils_ret_t 返回状态
 */
rcutils_ret_t parse_key(
  const yaml_event_t event, uint32_t * map_level, bool * is_new_map, size_t * node_idx,
  size_t * parameter_idx, namespace_tracker_t * ns_tracker, rcl_params_t * params_st)
{
  // 检查 map_level 和 params_st 是否为空
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(map_level, RCUTILS_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(params_st, RCUTILS_RET_INVALID_ARGUMENT);

  // 获取分配器
  rcutils_allocator_t allocator = params_st->allocator;

  // 检查分配器是否有效
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(
    &allocator, "invalid allocator", return RCUTILS_RET_INVALID_ARGUMENT);

  // 获取值的大小和值
  const size_t val_size = event.data.scalar.length;
  const char * value = (char *)event.data.scalar.value;
  const uint32_t line_num = ((uint32_t)(event.start_mark.line) + 1U);

  // 检查值是否为空
  RCUTILS_CHECK_FOR_NULL_WITH_MSG(
    value, "event argument has no value", return RCUTILS_RET_INVALID_ARGUMENT);

  // 检查值的大小是否为0
  if (0U == val_size) {
    RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING("No key at line %d", line_num);
    return RCUTILS_RET_ERROR;
  }

  rcutils_ret_t ret = RCUTILS_RET_OK;

  // 根据映射层级处理
  switch (*map_level) {
    case MAP_UNINIT_LVL:
      // 未初始化的映射层级
      RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING("Unintialized map level at line %d", line_num);
      ret = RCUTILS_RET_ERROR;
      break;
    case MAP_NODE_NAME_LVL: {
      // 处理节点名称层级

      // 在获取 PARAMS_KEY 之前，将名称添加到节点命名空间
      if (0 != strncmp(PARAMS_KEY, value, strlen(PARAMS_KEY))) {
        ret = add_name_to_ns(ns_tracker, value, NS_TYPE_NODE, allocator);
        if (RCUTILS_RET_OK != ret) {
          RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
            "Internal error adding node namespace at line %d", line_num);
          break;
        }
      } else {
        // 检查是否有节点名称
        if (0U == ns_tracker->num_node_ns) {
          RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
            "There are no node names before %s at line %d", PARAMS_KEY, line_num);
          ret = RCUTILS_RET_ERROR;
          break;
        }
        // 上一个键（命名空间中的最后一个名称）是节点名称。从命名空间中删除它
        char * node_name_ns = rcutils_strdup(ns_tracker->node_ns, allocator);
        if (NULL == node_name_ns) {
          ret = RCUTILS_RET_BAD_ALLOC;
          break;
        }

        // 验证名称
        ret = _validate_name(node_name_ns, allocator);
        if (RCUTILS_RET_OK != ret) {
          allocator.deallocate(node_name_ns, allocator.state);
          break;
        }

        // 查找节点
        ret = find_node(node_name_ns, params_st, node_idx);
        allocator.deallocate(node_name_ns, allocator.state);
        if (RCUTILS_RET_OK != ret) {
          break;
        }

        // 从命名空间中删除名称
        ret = rem_name_from_ns(ns_tracker, NS_TYPE_NODE, allocator);
        if (RCUTILS_RET_OK != ret) {
          RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
            "Internal error adding node namespace at line %d", line_num);
          break;
        }
        // 将映射层级提升到 PARAMS
        (*map_level)++;
      }
    } break;
    case MAP_PARAMS_LVL: {
      // 处理参数层级

      char * parameter_ns = NULL;
      char * param_name = NULL;

      // 如果是新映射，上一个键是参数命名空间
      if (*is_new_map) {
        parameter_ns = params_st->params[*node_idx].parameter_names[*parameter_idx];
        if (NULL == parameter_ns) {
          RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
            "Internal error creating param namespace at line %d", line_num);
          ret = RCUTILS_RET_ERROR;
          break;
        }
        ret = replace_ns(
          ns_tracker, parameter_ns, (ns_tracker->num_parameter_ns + 1U), NS_TYPE_PARAM, allocator);
        if (RCUTILS_RET_OK != ret) {
          RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
            "Internal error replacing namespace at line %d", line_num);
          ret = RCUTILS_RET_ERROR;
          break;
        }
        *is_new_map = false;
      }

      // 将参数名称添加到节点参数中
      parameter_ns = ns_tracker->parameter_ns;
      if (NULL == parameter_ns) {
        ret = find_parameter(*node_idx, value, params_st, parameter_idx);
        if (ret != RCUTILS_RET_OK) {
          break;
        }
      } else {
        ret = find_parameter(*node_idx, parameter_ns, params_st, parameter_idx);
        if (ret != RCUTILS_RET_OK) {
          break;
        }

        const size_t params_ns_len = strlen(parameter_ns);
        const size_t param_name_len = strlen(value);
        const size_t tot_len = (params_ns_len + param_name_len + 2U);

        param_name = allocator.zero_allocate(1U, tot_len, allocator.state);
        if (NULL == param_name) {
          ret = RCUTILS_RET_BAD_ALLOC;
          break;
        }

        memcpy(param_name, parameter_ns, params_ns_len);
        param_name[params_ns_len] = '.';
        memcpy((param_name + params_ns_len + 1U), value, param_name_len);
        param_name[tot_len - 1U] = '\0';

        if (NULL != params_st->params[*node_idx].parameter_names[*parameter_idx]) {
          // 这个内存是在 find_parameter() 中分配的，现在指针被覆盖了
          allocator.deallocate(
            params_st->params[*node_idx].parameter_names[*parameter_idx], allocator.state);
        }
        params_st->params[*node_idx].parameter_names[*parameter_idx] = param_name;
      }
    } break;
    default:
      // 未知的映射层级
      RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING("Unknown map level at line %d", line_num);
      ret = RCUTILS_RET_ERROR;
      break;
  }
  return ret;
}

/**
 * @brief 从解析参数YAML文件中获取事件并处理它们
 *
 * @param[in] parser 指向yaml_parser_t结构体的指针，用于解析YAML文件
 * @param[in] ns_tracker 指向namespace_tracker_t结构体的指针，用于跟踪命名空间
 * @param[out] params_st 指向rcl_params_t结构体的指针，用于存储解析后的参数信息
 * @return rcutils_ret_t 返回RCUTILS_RET_OK表示成功，其他值表示失败
 */
rcutils_ret_t parse_file_events(
  yaml_parser_t * parser, namespace_tracker_t * ns_tracker, rcl_params_t * params_st)
{
  // 定义变量
  int32_t done_parsing = 0;
  bool is_key = true;
  bool is_seq = false;
  uint32_t line_num = 0;
  data_types_t seq_data_type = DATA_TYPE_UNKNOWN;
  uint32_t map_level = 1U;
  uint32_t map_depth = 0U;
  bool is_new_map = false;

  // 检查输入参数是否为空
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(parser, RCUTILS_RET_INVALID_ARGUMENT);
  RCUTILS_CHECK_ARGUMENT_FOR_NULL(params_st, RCUTILS_RET_INVALID_ARGUMENT);

  // 获取分配器并检查其有效性
  rcutils_allocator_t allocator = params_st->allocator;
  RCUTILS_CHECK_ALLOCATOR_WITH_MSG(
    &allocator, "invalid allocator", return RCUTILS_RET_INVALID_ARGUMENT);

  // 定义YAML事件变量
  yaml_event_t event;
  size_t node_idx = 0;
  size_t parameter_idx = 0;
  rcutils_ret_t ret = RCUTILS_RET_OK;

  // 循环解析YAML文件中的事件
  while (0 == done_parsing) {
    if (RCUTILS_RET_OK != ret) {
      break;
    }
    int success = yaml_parser_parse(parser, &event);
    if (0 == success) {
      RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING("Error parsing a event near line %d", line_num);
      ret = RCUTILS_RET_ERROR;
      break;
    }
    line_num = ((uint32_t)(event.start_mark.line) + 1U);

    // 根据事件类型进行处理
    switch (event.type) {
      case YAML_STREAM_END_EVENT:
        done_parsing = 1;
        yaml_event_delete(&event);
        break;
      case YAML_SCALAR_EVENT: {
        // 在参数级别之间切换键和值
        if (is_key) {
          ret = parse_key(
            event, &map_level, &is_new_map, &node_idx, &parameter_idx, ns_tracker, params_st);
          if (RCUTILS_RET_OK != ret) {
            break;
          }
          is_key = false;
        } else {
          // 它是一个值
          if (map_level < (uint32_t)(MAP_PARAMS_LVL)) {
            RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
              "Cannot have a value before %s at line %d", PARAMS_KEY, line_num);
            ret = RCUTILS_RET_ERROR;
            break;
          }
          if (0U == params_st->num_nodes) {
            RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
              "Cannot have a value before %s at line %d", PARAMS_KEY, line_num);
            yaml_event_delete(&event);
            return RCUTILS_RET_ERROR;
          }
          if (0U == params_st->params[node_idx].num_params) {
            RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
              "Cannot have a value before %s at line %d", PARAMS_KEY, line_num);
            yaml_event_delete(&event);
            return RCUTILS_RET_ERROR;
          }
          ret = parse_value(event, is_seq, node_idx, parameter_idx, &seq_data_type, params_st);
          if (RCUTILS_RET_OK != ret) {
            break;
          }
          if (!is_seq) {
            is_key = true;
          }
        }
      } break;
      case YAML_SEQUENCE_START_EVENT:
        if (is_key) {
          RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING("Sequences cannot be key at line %d", line_num);
          ret = RCUTILS_RET_ERROR;
          break;
        }
        if (map_level < (uint32_t)(MAP_PARAMS_LVL)) {
          RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
            "Sequences can only be values and not keys in params. Error at line %d\n", line_num);
          ret = RCUTILS_RET_ERROR;
          break;
        }
        is_seq = true;
        seq_data_type = DATA_TYPE_UNKNOWN;
        break;
      case YAML_SEQUENCE_END_EVENT:
        is_seq = false;
        is_key = true;
        break;
      case YAML_MAPPING_START_EVENT:
        map_depth++;
        is_new_map = true;
        is_key = true;
        // 如果是PARAMS_KEY映射，则禁用新映射
        if ((MAP_PARAMS_LVL == map_level) && ((map_depth - (ns_tracker->num_node_ns + 1U)) == 2U)) {
          is_new_map = false;
        }
        break;
      case YAML_MAPPING_END_EVENT:
        if (MAP_PARAMS_LVL == map_level) {
          if (ns_tracker->num_parameter_ns > 0U) {
            // 删除参数命名空间
            ret = rem_name_from_ns(ns_tracker, NS_TYPE_PARAM, allocator);
            if (RCUTILS_RET_OK != ret) {
              RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
                "Internal error removing parameter namespace at line %d", line_num);
              break;
            }
          } else {
            map_level--;
          }
        } else {
          if ((MAP_NODE_NAME_LVL == map_level) && (map_depth == (ns_tracker->num_node_ns + 1U))) {
            // 删除节点命名空间
            ret = rem_name_from_ns(ns_tracker, NS_TYPE_NODE, allocator);
            if (RCUTILS_RET_OK != ret) {
              RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
                "Internal error removing node namespace at line %d", line_num);
              break;
            }
          }
        }
        map_depth--;
        break;
      case YAML_ALIAS_EVENT:
        RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING(
          "Will not support aliasing at line %d\n", line_num);
        ret = RCUTILS_RET_ERROR;
        break;
      case YAML_STREAM_START_EVENT:
        break;
      case YAML_DOCUMENT_START_EVENT:
        break;
      case YAML_DOCUMENT_END_EVENT:
        break;
      case YAML_NO_EVENT:
        RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING("Received an empty event at line %d", line_num);
        ret = RCUTILS_RET_ERROR;
        break;
      default:
        RCUTILS_SET_ERROR_MSG_WITH_FORMAT_STRING("Unknown YAML event at line %d", line_num);
        ret = RCUTILS_RET_ERROR;
        break;
    }
    yaml_event_delete(&event);
  }
  return ret;
}

/// \brief 从解析参数YAML值字符串中获取事件并处理它们
///
/// \param[in] parser 指向yaml_parser_t结构体的指针，用于解析YAML文件
/// \param[in] node_idx 当前节点的索引
/// \param[in] parameter_idx 当前参数的索引
/// \param[out] params_st 指向rcl_params_t结构体的指针，存储解析后的参数信息
/// \return 返回rcutils_ret_t类型的结果，表示解析过程是否成功
///
rcutils_ret_t parse_value_events(
  yaml_parser_t * parser, const size_t node_idx, const size_t parameter_idx,
  rcl_params_t * params_st)
{
  bool is_seq = false;                             // 是否为序列类型的标志
  data_types_t seq_data_type = DATA_TYPE_UNKNOWN;  // 序列数据类型初始化为未知
  rcutils_ret_t ret = RCUTILS_RET_OK;              // 初始化返回值为成功
  bool done_parsing = false;                       // 完成解析的标志
  while (RCUTILS_RET_OK == ret && !done_parsing) {  // 当解析成功且未完成解析时，继续循环
    yaml_event_t event;                             // 定义一个YAML事件变量
    int success = yaml_parser_parse(parser, &event);    // 解析YAML事件
    if (0 == success) {                                 // 如果解析失败
      RCUTILS_SET_ERROR_MSG("Error parsing an event");  // 设置错误消息
      ret = RCUTILS_RET_ERROR;                          // 返回值设为错误
      break;
    }
    switch (event.type) {          // 根据事件类型进行处理
      case YAML_STREAM_END_EVENT:  // 流结束事件
        done_parsing = true;       // 设置完成解析标志为真
        break;
      case YAML_SCALAR_EVENT:  // 标量事件
        ret =
          parse_value(event, is_seq, node_idx, parameter_idx, &seq_data_type, params_st);  // 解析值
        break;
      case YAML_SEQUENCE_START_EVENT:       // 序列开始事件
        is_seq = true;                      // 设置序列标志为真
        seq_data_type = DATA_TYPE_UNKNOWN;  // 重置序列数据类型为未知
        break;
      case YAML_SEQUENCE_END_EVENT:  // 序列结束事件
        is_seq = false;              // 设置序列标志为假
        break;
      case YAML_STREAM_START_EVENT:  // 流开始事件
        break;
      case YAML_DOCUMENT_START_EVENT:  // 文档开始事件
        break;
      case YAML_DOCUMENT_END_EVENT:  // 文档结束事件
        break;
      case YAML_NO_EVENT:                                  // 空事件
        RCUTILS_SET_ERROR_MSG("Received an empty event");  // 设置错误消息
        ret = RCUTILS_RET_ERROR;                           // 返回值设为错误
        break;
      default:                                        // 未知事件
        RCUTILS_SET_ERROR_MSG("Unknown YAML event");  // 设置错误消息
        ret = RCUTILS_RET_ERROR;                      // 返回值设为错误
        break;
    }
    yaml_event_delete(&event);  // 删除已处理的YAML事件
  }
  return ret;  // 返回解析结果
}

/// @brief 在节点参数结构中查找参数条目索引
///
/// @param[in] node_idx 节点索引
/// @param[in] parameter_name 参数名称
/// @param[in,out] param_st 指向 rcl_params_t 结构的指针，用于存储参数信息
/// @param[out] parameter_idx 存储找到的参数索引
/// @return 返回 rcutils_ret_t 类型的结果，表示查找参数是否成功
rcutils_ret_t find_parameter(
  const size_t node_idx, const char * parameter_name, rcl_params_t * param_st,
  size_t * parameter_idx)
{
  // 断言检查输入参数是否有效
  assert(NULL != parameter_name);
  assert(NULL != param_st);
  assert(NULL != parameter_idx);

  // 断言检查节点索引是否在有效范围内
  assert(node_idx < param_st->num_nodes);

  // 获取节点参数结构的指针
  rcl_node_params_t * node_param_st = &(param_st->params[node_idx]);
  // 遍历节点参数结构，查找参数名称
  for (*parameter_idx = 0U; *parameter_idx < node_param_st->num_params; (*parameter_idx)++) {
    if (0 == strcmp(node_param_st->parameter_names[*parameter_idx], parameter_name)) {
      // 找到参数
      return RCUTILS_RET_OK;
    }
  }
  // 参数未找到，添加它
  rcutils_allocator_t allocator = param_st->allocator;
  // 如果需要，重新分配内存
  if (node_param_st->num_params >= node_param_st->capacity_params) {
    if (
      RCUTILS_RET_OK !=
      node_params_reallocate(node_param_st, node_param_st->capacity_params * 2, allocator)) {
      return RCUTILS_RET_BAD_ALLOC;
    }
  }
  // 检查参数名称是否已经存在
  if (NULL != node_param_st->parameter_names[*parameter_idx]) {
    param_st->allocator.deallocate(
      node_param_st->parameter_names[*parameter_idx], param_st->allocator.state);
  }
  // 分配内存并复制参数名称
  node_param_st->parameter_names[*parameter_idx] = rcutils_strdup(parameter_name, allocator);
  // 检查分配的内存是否有效
  if (NULL == node_param_st->parameter_names[*parameter_idx]) {
    return RCUTILS_RET_BAD_ALLOC;
  }
  // 增加节点参数结构中的参数数量
  node_param_st->num_params++;
  return RCUTILS_RET_OK;
}

/**
 * @brief 在参数结构中查找节点条目索引 (Find node entry index in parameters' structure)
 *
 * @param[in] node_name 要查找的节点名称 (The name of the node to find)
 * @param[in,out] param_st 参数结构指针，用于存储节点信息 (Pointer to the parameter structure, used to store node information)
 * @param[out] node_idx 查找到的节点索引将存储在此处 (The found node index will be stored here)
 * @return rcutils_ret_t 返回查找结果状态 (Return the result status of the search)
 */
rcutils_ret_t find_node(const char * node_name, rcl_params_t * param_st, size_t * node_idx)
{
  // 检查输入参数是否为非空 (Check if input arguments are not NULL)
  assert(NULL != node_name);
  assert(NULL != param_st);
  assert(NULL != node_idx);

  // 遍历参数结构中的所有节点 (Iterate through all nodes in the parameter structure)
  for (*node_idx = 0U; *node_idx < param_st->num_nodes; (*node_idx)++) {
    // 如果找到匹配的节点名称 (If a matching node name is found)
    if (0 == strcmp(param_st->node_names[*node_idx], node_name)) {
      // 节点已找到 (Node found)
      return RCUTILS_RET_OK;
    }
  }
  // 节点未找到，添加它 (Node not found, add it)
  rcutils_allocator_t allocator = param_st->allocator;
  // 如果需要，重新分配内存 (Reallocate memory if necessary)
  if (param_st->num_nodes >= param_st->capacity_nodes) {
    if (
      RCUTILS_RET_OK !=
      rcl_yaml_node_struct_reallocate(param_st, param_st->capacity_nodes * 2, allocator)) {
      return RCUTILS_RET_BAD_ALLOC;
    }
  }
  // 分配内存并复制节点名称 (Allocate memory and copy the node name)
  param_st->node_names[*node_idx] = rcutils_strdup(node_name, allocator);
  if (NULL == param_st->node_names[*node_idx]) {
    return RCUTILS_RET_BAD_ALLOC;
  }
  // 初始化新节点的参数结构 (Initialize the parameter structure of the new node)
  rcutils_ret_t ret = node_params_init(&(param_st->params[*node_idx]), allocator);
  if (RCUTILS_RET_OK != ret) {
    // 如果初始化失败，释放已分配的内存 (If initialization fails, release the allocated memory)
    allocator.deallocate(param_st->node_names[*node_idx], allocator.state);
    param_st->node_names[*node_idx] = NULL;
    return ret;
  }
  // 更新节点数量 (Update the number of nodes)
  param_st->num_nodes++;
  return RCUTILS_RET_OK;
}
