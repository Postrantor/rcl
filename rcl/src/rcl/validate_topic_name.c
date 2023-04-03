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

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/validate_topic_name.h"

#include <ctype.h>
#include <string.h>

#include "rcl/allocator.h"
#include "rcl/error_handling.h"
#include "rcutils/isalnum_no_locale.h"

/**
 * @brief 验证ROS2主题名称是否有效
 *
 * 此函数检查给定的主题名称是否符合ROS2命名规范。如果主题名称无效，它还将返回无效字符的索引。
 *
 * @param[in] topic_name 要验证的主题名称字符串
 * @param[out] validation_result 验证结果，指示主题名称是否有效
 * @param[out] invalid_index 如果主题名称无效，此参数将包含无效字符的索引
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_validate_topic_name(
    const char* topic_name, int* validation_result, size_t* invalid_index) {
  // 检查topic_name是否为空，如果为空则返回RCL_RET_INVALID_ARGUMENT错误
  RCL_CHECK_ARGUMENT_FOR_NULL(topic_name, RCL_RET_INVALID_ARGUMENT);

  // 使用给定的主题名称长度调用rcl_validate_topic_name_with_size进行验证
  return rcl_validate_topic_name_with_size(
      topic_name, strlen(topic_name), validation_result, invalid_index);
}

/**
 * @brief 验证给定长度的主题名称是否有效，并返回验证结果和无效索引（如果有）
 *
 * @param[in] topic_name 要验证的主题名称
 * @param[in] topic_name_length 主题名称的长度
 * @param[out] validation_result 验证结果，指示主题名称是否有效
 * @param[out] invalid_index 如果主题名称无效，此参数将包含无效字符的索引
 * @return rcl_ret_t 返回RCL_RET_OK，如果函数执行成功，否则返回相应的错误代码
 */
rcl_ret_t rcl_validate_topic_name_with_size(
    const char* topic_name,
    size_t topic_name_length,
    int* validation_result,
    size_t* invalid_index) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(topic_name, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(validation_result, RCL_RET_INVALID_ARGUMENT);

  // 如果主题名称长度为0，则设置验证结果为无效且为空字符串
  if (topic_name_length == 0) {
    *validation_result = RCL_TOPIC_NAME_INVALID_IS_EMPTY_STRING;
    if (invalid_index) {
      *invalid_index = 0;
    }
    return RCL_RET_OK;
  }
  // 检查第一个字符是否为数字
  if (isdigit(topic_name[0]) != 0) {
    // 如果主题是相对的，且第一个标记以数字开头，则无效，例如：7foo/bar
    *validation_result = RCL_TOPIC_NAME_INVALID_NAME_TOKEN_STARTS_WITH_NUMBER;
    if (invalid_index) {
      *invalid_index = 0;
    }
    return RCL_RET_OK;
  }
  // 此时，topic_name_length >= 1
  if (topic_name[topic_name_length - 1] == '/') {
    // 捕获以"/"结尾的情况，例如："/foo/" 和 "/"
    *validation_result = RCL_TOPIC_NAME_INVALID_ENDS_WITH_FORWARD_SLASH;
    if (invalid_index) {
      *invalid_index = topic_name_length - 1;
    }
    return RCL_RET_OK;
  }
  // 检查不允许的字符、嵌套和不匹配的{}
  bool in_open_curly_brace = false;
  size_t opening_curly_brace_index = 0;
  for (size_t i = 0; i < topic_name_length; ++i) {
    if (rcutils_isalnum_no_locale(topic_name[i])) {
      // 如果在大括号内且第一个字符是数字，则报错，例如：foo/{4bar}
      if (isdigit(topic_name[i]) != 0 && in_open_curly_brace && i > 0 &&
          (i - 1 == opening_curly_brace_index)) {
        *validation_result = RCL_TOPIC_NAME_INVALID_SUBSTITUTION_STARTS_WITH_NUMBER;
        if (invalid_index) {
          *invalid_index = i;
        }
        return RCL_RET_OK;
      }
      // 如果是字母数字字符，即[0-9|A-Z|a-z]，则继续
      continue;
    } else if (topic_name[i] == '_') {
      // 如果是下划线，则继续
      continue;
    } else if (topic_name[i] == '/') {
      // 如果在{}内有正斜杠，则报错
      if (in_open_curly_brace) {
        *validation_result = RCL_TOPIC_NAME_INVALID_SUBSTITUTION_CONTAINS_UNALLOWED_CHARACTERS;
        if (invalid_index) {
          *invalid_index = i;
        }
        return RCL_RET_OK;
      }
      // 如果在{}外有正斜杠，则继续
      continue;
    } else if (topic_name[i] == '~') {
      // 如果波浪号不在第一个位置，则验证失败
      if (i != 0) {
        *validation_result = RCL_TOPIC_NAME_INVALID_MISPLACED_TILDE;
        if (invalid_index) {
          *invalid_index = i;
        }
        return RCL_RET_OK;
      }
      // 如果波浪号在第一个位置，则继续
      continue;
    } else if (topic_name[i] == '{') {
      opening_curly_brace_index = i;
      // 如果开始嵌套大括号，则报错，例如：foo/{{bar}_baz}
      if (in_open_curly_brace) {
        *validation_result = RCL_TOPIC_NAME_INVALID_SUBSTITUTION_CONTAINS_UNALLOWED_CHARACTERS;
        if (invalid_index) {
          *invalid_index = i;
        }
        return RCL_RET_OK;
      }
      in_open_curly_brace = true;
      // 如果是新的、打开的大括号，则继续
      continue;
    } else if (topic_name[i] == '}') {
      // 如果没有前置的{，则报错
      if (!in_open_curly_brace) {
        *validation_result = RCL_TOPIC_NAME_INVALID_UNMATCHED_CURLY_BRACE;
        if (invalid_index) {
          *invalid_index = i;
        }
        return RCL_RET_OK;
      }
      in_open_curly_brace = false;
      // 如果是闭合的大括号，则继续
      continue;
    } else {
      // 如果不是这些字符，则主题名称中包含不允许的字符
      if (in_open_curly_brace) {
        *validation_result = RCL_TOPIC_NAME_INVALID_SUBSTITUTION_CONTAINS_UNALLOWED_CHARACTERS;
      } else {
        *validation_result = RCL_TOPIC_NAME_INVALID_CONTAINS_UNALLOWED_CHARACTERS;
      }
      if (invalid_index) {
        *invalid_index = i;
      }
      return RCL_RET_OK;
    }
  }
  // 检查替换是否正确关闭
  if (in_open_curly_brace) {
    // 替换未关闭的情况，例如：'foo/{bar'
    *validation_result = RCL_TOPIC_NAME_INVALID_UNMATCHED_CURLY_BRACE;
    if (invalid_index) {
      *invalid_index = opening_curly_brace_index;
    }
    return RCL_RET_OK;
  }
  // 检查以数字开头的标记（除第一个之外）
  for (size_t i = 0; i < topic_name_length; ++i) {
    if (i == topic_name_length - 1) {
      // 如果这是最后一个字符，则无需检查
      continue;
    }
    // 过了这一点，假设i+1是有效索引
    if (topic_name[i] == '/') {
      if (isdigit(topic_name[i + 1]) != 0) {
        // 如果'/'后面跟着数字，即[0-9]
        *validation_result = RCL_TOPIC_NAME_INVALID_NAME_TOKEN_STARTS_WITH_NUMBER;
        if (invalid_index) {
          *invalid_index = i + 1;
        }
        return RCL_RET_OK;
      }
    } else if (i == 1 && topic_name[0] == '~') {
      // 特殊情况：第一个字符是~，但第二个字符不是/，例如：~foo无效
      *validation_result = RCL_TOPIC_NAME_INVALID_TILDE_NOT_FOLLOWED_BY_FORWARD_SLASH;
      if (invalid_index) {
        *invalid_index = 1;
      }
      return RCL_RET_OK;
    }
  }
  // 一切正常，将结果设置为有效主题，避免设置无效索引，并返回
  *validation_result = RCL_TOPIC_NAME_VALID;
  return RCL_RET_OK;
}

/**
 * @brief 根据验证结果返回相应的错误信息字符串
 * @param validation_result 验证结果，是一个整数值
 * @return 如果验证通过，则返回NULL；否则返回对应的错误信息字符串
 */
const char* rcl_topic_name_validation_result_string(int validation_result) {
  switch (validation_result) {
    case RCL_TOPIC_NAME_VALID:
      // 主题名称有效，返回NULL
      return NULL;
    case RCL_TOPIC_NAME_INVALID_IS_EMPTY_STRING:
      // 主题名称不能为空字符串
      return "topic name must not be empty string";
    case RCL_TOPIC_NAME_INVALID_ENDS_WITH_FORWARD_SLASH:
      // 主题名称不能以正斜杠结尾
      return "topic name must not end with a forward slash";
    case RCL_TOPIC_NAME_INVALID_CONTAINS_UNALLOWED_CHARACTERS:
      // 主题名称不能包含除字母数字、下划线、波浪线、大括号之外的字符
      return "topic name must not contain characters other than alphanumerics, '_', '~', '{', or "
             "'}'";
    case RCL_TOPIC_NAME_INVALID_NAME_TOKEN_STARTS_WITH_NUMBER:
      // 主题名称的标记不能以数字开头
      return "topic name token must not start with a number";
    case RCL_TOPIC_NAME_INVALID_UNMATCHED_CURLY_BRACE:
      // 主题名称不能有不匹配（不平衡）的大括号
      return "topic name must not have unmatched (unbalanced) curly braces '{}'";
    case RCL_TOPIC_NAME_INVALID_MISPLACED_TILDE:
      // 主题名称不能包含波浪线'~'，除非它是第一个字符
      return "topic name must not have tilde '~' unless it is the first character";
    case RCL_TOPIC_NAME_INVALID_TILDE_NOT_FOLLOWED_BY_FORWARD_SLASH:
      // 主题名称不能包含波浪线'~'，除非它后面紧跟正斜杠'/'
      return "topic name must not have a tilde '~' that is not followed by a forward slash '/'";
    case RCL_TOPIC_NAME_INVALID_SUBSTITUTION_CONTAINS_UNALLOWED_CHARACTERS:
      // 替换名称不能包含除字母数字和下划线之外的字符
      return "substitution name must not contain characters other than alphanumerics or '_'";
    case RCL_TOPIC_NAME_INVALID_SUBSTITUTION_STARTS_WITH_NUMBER:
      // 替换名称不能以数字开头
      return "substitution name must not start with a number";
    default:
      // 未知的rcl主题名称验证结果代码
      return "unknown result code for rcl topic name validation";
  }
}

#ifdef __cplusplus
}
#endif
