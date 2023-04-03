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

#ifndef RCL__LEXER_LOOKAHEAD_H_
#define RCL__LEXER_LOOKAHEAD_H_

#include <stddef.h>

#include "rcl/allocator.h"
#include "rcl/lexer.h"
#include "rcl/macros.h"
#include "rcl/types.h"
#include "rcl/visibility_control.h"

#if __cplusplus
extern "C" {
#endif

// 前向声明
typedef struct rcl_lexer_lookahead2_impl_s rcl_lexer_lookahead2_impl_t;

/// 跟踪词法分析并允许查看2个词素。
typedef struct rcl_lexer_lookahead2_s
{
  /// 指向 lexer look ahead2 实现的指针
  rcl_lexer_lookahead2_impl_t * impl;
} rcl_lexer_lookahead2_t;

/// 获取一个零初始化的 rcl_lexer_lookahead2_t 实例。
/**
 * \sa rcl_lexer_lookahead2_init()
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | No
 * Thread-Safe        | Yes
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \return 零初始化的 lookahead2 缓冲区。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_lexer_lookahead2_t rcl_get_zero_initialized_lexer_lookahead2();

/// 初始化 rcl_lexer_lookahead2_t 实例。
/**
 * lookahead2 缓冲区借用对提供的文本的引用。
 * 在缓冲区完成之前，文本不能被释放。
 * 只有当此函数返回 RCL_RET_OK 时，lookahead2 缓冲区才需要完成。
 * \sa rcl_lexer_lookahead2_fini()
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 *
 * \param[in] buffer 零初始化的缓冲区。
 * \sa rcl_get_zero_initialized_lexer_lookahead2()
 * \param[in] text 要分析的字符串。
 * \param[in] allocator 发生错误时使用的分配器。
 * \return 如果缓冲区成功初始化，则返回 #RCL_RET_OK，或
 * \return 如果任何函数参数无效，则返回 #RCL_RET_INVALID_ARGUMENT，或
 * \return 如果分配内存失败，则返回 #RCL_RET_BAD_ALLOC，或
 * \return 如果发生未指定的错误，则返回 #RCL_RET_ERROR。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_lexer_lookahead2_init(
  rcl_lexer_lookahead2_t * buffer, const char * text, rcl_allocator_t allocator);

/// 完成 rcl_lexer_lookahead2_t 结构的实例。
/**
 * \sa rcl_lexer_lookahead2_init()
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes [1]
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 * <i>[1] 只有在参数无效时才分配。</i>
 *
 * \param[in] buffer 要释放的结构。
 * \return 如果结构成功完成，则返回 #RCL_RET_OK，或
 * \return 如果任何函数参数无效，则返回 #RCL_RET_INVALID_ARGUMENT，或
 * \return 如果发生未指定的错误，则返回 #RCL_RET_ERROR。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_lexer_lookahead2_fini(rcl_lexer_lookahead2_t * buffer);

/// 查看字符串中的下一个词素。
/**
 * 对 peek 的重复调用将返回相同的词素。
 * 将下一个词素视为有效的解析器必须接受它以推进词法分析。
 * \sa rcl_lexer_lookahead2_accept()
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes [1]
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 * <i>[1] 只有在参数无效或检测到内部错误时才分配。</i>
 *
 * \param[in] buffer 正在用于分析字符串的 lookahead2 缓冲区。
 * \param[out] next_type 字符串中下一个词素的输出变量。
 * \return 如果查看成功，则返回 #RCL_RET_OK，或
 * \return 如果任何函数参数无效，则返回 #RCL_RET_INVALID_ARGUMENT，或
 * \return 如果发生未指定的错误，则返回 #RCL_RET_ERROR。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_lexer_lookahead2_peek(rcl_lexer_lookahead2_t * buffer, rcl_lexeme_t * next_type);

/// 查看字符串中的下两个词素。
/**
 * 对 peek2 的重复调用将返回相同的两个词素。
 * 将下两个词素视为有效的解析器必须接受两次以推进词法分析。
 * \sa rcl_lexer_lookahead2_accept()
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes [1]
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 * <i>[1] 只有在参数无效或检测到内部错误时才分配。</i>
 *
 * \param[in] buffer 正在用于分析字符串的 lookahead2 缓冲区。
 * \param[out] next_type1 字符串中下一个词素的输出变量。
 * \param[out] next_type2 字符串中下一个词素之后的词素的输出变量。
 * \return 如果查看成功，则返回 #RCL_RET_OK，或
 * \return 如果任何函数参数无效，则返回 #RCL_RET_INVALID_ARGUMENT，或
 * \return 如果发生未指定的错误，则返回 #RCL_RET_ERROR。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_lexer_lookahead2_peek2(
  rcl_lexer_lookahead2_t * buffer, rcl_lexeme_t * next_type1, rcl_lexeme_t * next_type2);

/// 接受词素并推进分析。
/**
 * 必须先查看令牌才能接受它。
 * \sa rcl_lexer_lookahead2_peek()
 * \sa rcl_lexer_lookahead2_peek2()
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes [1]
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 * <i>[1] 只有在参数无效或发生错误时才分配。</i>
 *
 * \param[in] buffer 正在用于分析字符串的 lookahead2 缓冲区。
 * \param[out] lexeme_text 词素在字符串中开始的指针。
 * \param[out] lexeme_text_length lexeme_text 的长度。
 * \return 如果查看成功，则返回 #RCL_RET_OK，或
 * \return 如果任何函数参数无效，则返回 #RCL_RET_INVALID_ARGUMENT，或
 * \return 如果发生未指定的错误，则返回 #RCL_RET_ERROR。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_lexer_lookahead2_accept(
  rcl_lexer_lookahead2_t * buffer, const char ** lexeme_text, size_t * lexeme_text_length);

/// 要求下一个词素为某种类型并推进分析。
/**
 * 此方法是查看和接受词素的快捷方式。
 * 当只有一个有效的词素可能接下来时，解析器应使用它。
 * \sa rcl_lexer_lookahead2_peek()
 * \sa rcl_lexer_lookahead2_accept()
 *
 * <hr>
 * Attribute          | Adherence
 * ------------------ | -------------
 * Allocates Memory   | Yes [1]
 * Thread-Safe        | No
 * Uses Atomics       | No
 * Lock-Free          | Yes
 * <i>[1] 只有在参数无效或发生错误时才分配。</i>
 *
 * \param[in] buffer 正在用于分析字符串的 lookahead2 缓冲区。
 * \param[in] type 下一个词素必须是的类型。
 * \param[out] lexeme_text 词素在字符串中开始的指针。
 * \param[out] lexeme_text_length lexeme_text 的长度。
 * \return 如果下一个词素是预期的，则返回 #RCL_RET_OK，或
 * \return 如果下一个词素不是预期的，则返回 #RCL_RET_WRONG_LEXEME，或
 * \return 如果任何函数参数无效，则返回 #RCL_RET_INVALID_ARGUMENT，或
 * \return 如果发生未指定的错误，则返回 #RCL_RET_ERROR。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_lexer_lookahead2_expect(
  rcl_lexer_lookahead2_t * buffer, rcl_lexeme_t type, const char ** lexeme_text,
  size_t * lexeme_text_length);

/// 获取当前正在分析的位置的文本。
/**
 * <hr>
 * 属性                | 符合性
 * ------------------ | -------------
 * 分配内存            | 否
 * 线程安全            | 是
 * 使用原子操作        | 否
 * 无锁                | 是
 *
 * \param[in] buffer 正在用于分析字符串的lookahead2缓冲区。
 * \return 如果buffer非空且已初始化，返回指向原始文本中正在分析的位置的指针；
 * \return 如果buffer为空或零初始化，返回`NULL`；
 * \return 如果buffer未初始化或已完成，返回未定义的值。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const char * rcl_lexer_lookahead2_get_text(const rcl_lexer_lookahead2_t * buffer);

#if __cplusplus
}
#endif

#endif  // RCL__LEXER_LOOKAHEAD_H_
