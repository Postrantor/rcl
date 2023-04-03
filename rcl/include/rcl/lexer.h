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

#ifndef RCL__LEXER_H_
#define RCL__LEXER_H_

#include <stddef.h>

#include "rcl/allocator.h"
#include "rcl/macros.h"
#include "rcl/types.h"
#include "rcl/visibility_control.h"

#if __cplusplus
extern "C" {
#endif

/// 词法分析器找到的词素类型。
typedef enum rcl_lexeme_e {
  /// 表示没有找到有效的词素（未达到输入结尾）
  RCL_LEXEME_NONE = 0,
  /// 表示已经到达输入的结尾
  RCL_LEXEME_EOF = 1,
  /// ~/
  RCL_LEXEME_TILDE_SLASH = 2,
  /// rosservice://
  RCL_LEXEME_URL_SERVICE = 3,
  /// rostopic://
  RCL_LEXEME_URL_TOPIC = 4,
  /// :
  RCL_LEXEME_COLON = 5,
  /// __node 或者 __name
  RCL_LEXEME_NODE = 6,
  /// __ns
  RCL_LEXEME_NS = 7,
  /// :=
  RCL_LEXEME_SEPARATOR = 8,
  /// \1
  RCL_LEXEME_BR1 = 9,
  /// \2
  RCL_LEXEME_BR2 = 10,
  /// \3
  RCL_LEXEME_BR3 = 11,
  /// \4
  RCL_LEXEME_BR4 = 12,
  /// \5
  RCL_LEXEME_BR5 = 13,
  /// \6
  RCL_LEXEME_BR6 = 14,
  /// \7
  RCL_LEXEME_BR7 = 15,
  /// \8
  RCL_LEXEME_BR8 = 16,
  /// \9
  RCL_LEXEME_BR9 = 17,
  /// 斜杠之间的名称，必须匹配 (([a-zA-Z](_)?)|_)([0-9a-zA-Z](_)?)*
  RCL_LEXEME_TOKEN = 18,
  /// /
  RCL_LEXEME_FORWARD_SLASH = 19,
  /// *
  RCL_LEXEME_WILD_ONE = 20,
  /// **
  RCL_LEXEME_WILD_MULTI = 21,
  // TODO(hidmic): 当参数名称标准化为使用斜杠代替点时删除
  /// \.
  RCL_LEXEME_DOT = 22,
} rcl_lexeme_t;

/// 对字符串进行词法分析。
/**
 * 此函数分析一个字符串，查看它是否以有效的词素开头。
 * 如果字符串不是以有效的词素开头，则词素将为 RCL_LEXEME_NONE，并且
 * 长度将设置为包含使其无效的字符。
 * 它永远不会超过字符串的长度。
 * 如果第一个字符是 '\0'，那么词素将是 RCL_LEXEME_EOF。
 *
 * <hr>
 * 属性              | 符合性
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 否
 * 无锁              | 是
 *
 * \param[in] text 要分析的字符串。
 * \param[out] lexeme 在字符串中找到的词素类型。
 * \param[out] length 构成找到的词素的字符串文本的长度。
 * \return #RCL_RET_OK 如果分析成功，无论是否找到有效的词素，或者
 * \return #RCL_RET_INVALID_ARGUMENT 如果任何函数参数无效，或者
 * \return #RCL_RET_ERROR 如果检测到内部错误。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_lexer_analyze(const char * text, rcl_lexeme_t * lexeme, size_t * length);

#if __cplusplus
}
#endif

#endif  // RCL__LEXER_H_
