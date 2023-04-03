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

#include "rcl/lexer_lookahead.h"

#include "rcl/error_handling.h"

/**
 * @struct rcl_lexer_lookahead2_impl_s
 * @brief 用于分析文本中词素的结构体
 */
struct rcl_lexer_lookahead2_impl_s {
  const char* text;           ///< 正在分析的文本
  size_t text_idx;            ///< 当前分析位置

  size_t start[2];            ///< 词素的第一个字符位置
  size_t end[2];              ///< 词素的最后一个字符位置之后的位置
  rcl_lexeme_t type[2];       ///< 词素类型

  rcl_allocator_t allocator;  ///< 发生错误时使用的内存分配器
};

/**
 * @brief 获取一个初始化为零的rcl_lexer_lookahead2_t实例
 *
 * @return 初始化为零的rcl_lexer_lookahead2_t实例
 */
rcl_lexer_lookahead2_t rcl_get_zero_initialized_lexer_lookahead2() {
  static rcl_lexer_lookahead2_t zero_initialized = {
      .impl = NULL,
  };
  return zero_initialized;
}

/**
 * @brief 初始化 rcl_lexer_lookahead2_t 结构体
 *
 * @param buffer 用于存储 lookahead2 结构的指针
 * @param text 要解析的文本字符串
 * @param allocator 分配器，用于分配内存
 * @return 返回 RCL_RET_OK 表示成功，其他值表示失败
 */
rcl_ret_t rcl_lexer_lookahead2_init(
    rcl_lexer_lookahead2_t* buffer, const char* text, rcl_allocator_t allocator) {
  // 检查参数是否有效并返回错误
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_BAD_ALLOC);

  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(&allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  // 检查 buffer 和 text 参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(buffer, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(text, RCL_RET_INVALID_ARGUMENT);
  // 检查 buffer 是否已经初始化
  if (NULL != buffer->impl) {
    RCL_SET_ERROR_MSG("buffer must be zero initialized");
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 为 lookahead2 结构分配内存
  buffer->impl = allocator.allocate(sizeof(rcl_lexer_lookahead2_impl_t), allocator.state);
  // 检查内存分配是否成功
  RCL_CHECK_FOR_NULL_WITH_MSG(
      buffer->impl, "Failed to allocate lookahead impl", return RCL_RET_BAD_ALLOC);

  // 初始化 lookahead2 结构的成员变量
  buffer->impl->text = text;
  buffer->impl->text_idx = 0u;
  buffer->impl->start[0] = 0u;
  buffer->impl->start[1] = 0u;
  buffer->impl->end[0] = 0u;
  buffer->impl->end[1] = 0u;
  buffer->impl->type[0] = RCL_LEXEME_NONE;
  buffer->impl->type[1] = RCL_LEXEME_NONE;
  buffer->impl->allocator = allocator;

  // 返回成功
  return RCL_RET_OK;
}

/**
 * @brief 释放 rcl_lexer_lookahead2_t 结构体的内存
 *
 * @param buffer 要释放的 lookahead2 结构的指针
 * @return 返回 RCL_RET_OK 表示成功，其他值表示失败
 */
rcl_ret_t rcl_lexer_lookahead2_fini(rcl_lexer_lookahead2_t* buffer) {
  // 检查 buffer 参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(buffer, RCL_RET_INVALID_ARGUMENT);
  // 检查 buffer 是否已经被释放
  RCL_CHECK_FOR_NULL_WITH_MSG(
      buffer->impl, "buffer finalized twice", return RCL_RET_INVALID_ARGUMENT);
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(
      &(buffer->impl->allocator), "invalid allocator", return RCL_RET_INVALID_ARGUMENT);

  // 释放 lookahead2 结构的内存
  buffer->impl->allocator.deallocate(buffer->impl, buffer->impl->allocator.state);
  buffer->impl = NULL;

  // 返回成功
  return RCL_RET_OK;
}

/**
 * @brief 从缓冲区中预览下一个词法单元类型
 *
 * @param[in] buffer 指向rcl_lexer_lookahead2_t结构体的指针
 * @param[out] next_type 存储下一个词法单元类型的指针
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_lexer_lookahead2_peek(rcl_lexer_lookahead2_t* buffer, rcl_lexeme_t* next_type) {
  // 检查参数是否有效，如果无效则返回错误
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);

  // 检查buffer是否为空，如果为空则返回错误
  RCL_CHECK_ARGUMENT_FOR_NULL(buffer, RCL_RET_INVALID_ARGUMENT);
  // 检查buffer->impl是否为空，如果为空则返回错误
  RCL_CHECK_FOR_NULL_WITH_MSG(
      buffer->impl, "buffer not initialized", return RCL_RET_INVALID_ARGUMENT);
  // 检查next_type是否为空，如果为空则返回错误
  RCL_CHECK_ARGUMENT_FOR_NULL(next_type, RCL_RET_INVALID_ARGUMENT);

  rcl_ret_t ret;
  size_t length;

  // 如果文本索引大于等于第一个词法单元的结束位置
  if (buffer->impl->text_idx >= buffer->impl->end[0]) {
    // 没有缓冲的词法单元；获取一个
    ret =
        rcl_lexer_analyze(rcl_lexer_lookahead2_get_text(buffer), &(buffer->impl->type[0]), &length);

    // 如果返回值不是RCL_RET_OK，则返回错误
    if (RCL_RET_OK != ret) {
      return ret;
    }

    // 设置第一个词法单元的开始和结束位置
    buffer->impl->start[0] = buffer->impl->text_idx;
    buffer->impl->end[0] = buffer->impl->start[0] + length;
  }

  // 将第一个词法单元类型赋值给next_type
  *next_type = buffer->impl->type[0];
  // 返回成功
  return RCL_RET_OK;
}

/**
 * @brief 从缓冲区中预览两个连续的词法单元类型
 *
 * @param[in] buffer 指向rcl_lexer_lookahead2_t结构体的指针，用于存储词法分析器的状态
 * @param[out] next_type1 指向第一个词法单元类型的指针
 * @param[out] next_type2 指向第二个词法单元类型的指针
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_lexer_lookahead2_peek2(
    rcl_lexer_lookahead2_t* buffer, rcl_lexeme_t* next_type1, rcl_lexeme_t* next_type2) {
  // 检查参数是否有效，如果无效则返回错误
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);

  rcl_ret_t ret;
  // 首先预览第一个词法单元（重用其对buffer和next_type1的错误检查）
  ret = rcl_lexer_lookahead2_peek(buffer, next_type1);
  if (RCL_RET_OK != ret) {
    return ret;
  }
  // 检查next_type2参数是否为空，如果为空则返回错误
  RCL_CHECK_ARGUMENT_FOR_NULL(next_type2, RCL_RET_INVALID_ARGUMENT);

  // 如果第一个词法单元是NONE或EOF，则不需要进一步预览
  if (RCL_LEXEME_NONE == *next_type1 || RCL_LEXEME_EOF == *next_type1) {
    *next_type2 = *next_type1;
    return ret;
  }

  size_t length;

  // 如果缓冲区中没有词法单元，则获取一个
  if (buffer->impl->text_idx >= buffer->impl->end[1]) {
    ret = rcl_lexer_analyze(
        &(buffer->impl->text[buffer->impl->end[0]]), &(buffer->impl->type[1]), &length);

    if (RCL_RET_OK != ret) {
      return ret;
    }

    // 更新缓冲区的起始和结束位置
    buffer->impl->start[1] = buffer->impl->end[0];
    buffer->impl->end[1] = buffer->impl->start[1] + length;
  }

  // 设置第二个词法单元类型
  *next_type2 = buffer->impl->type[1];
  return RCL_RET_OK;
}

/**
 * @brief 接受并处理 lookahead2 缓冲区中的词素
 *
 * @param[in] buffer 指向 rcl_lexer_lookahead2_t 结构体的指针
 * @param[out] lexeme_text 词素文本的指针，可以为 NULL
 * @param[out] lexeme_text_length 词素文本长度的指针，可以为 NULL
 * @return rcl_ret_t 返回 RCL_RET_OK 或相应的错误代码
 */
rcl_ret_t rcl_lexer_lookahead2_accept(
    rcl_lexer_lookahead2_t* buffer, const char** lexeme_text, size_t* lexeme_text_length) {
  // 检查参数是否有效，设置错误消息并返回错误代码
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_ERROR);

  // 检查 buffer 是否为空，如果为空则返回无效参数错误
  RCL_CHECK_ARGUMENT_FOR_NULL(buffer, RCL_RET_INVALID_ARGUMENT);
  // 检查 buffer->impl 是否为空，如果为空则设置错误消息并返回无效参数错误
  RCL_CHECK_FOR_NULL_WITH_MSG(
      buffer->impl, "buffer not initialized", return RCL_RET_INVALID_ARGUMENT);
  // 检查 lexeme_text 和 lexeme_text_length 是否同时为 NULL 或非
  // NULL，否则设置错误消息并返回无效参数错误
  if ((NULL == lexeme_text && NULL != lexeme_text_length) ||
      (NULL != lexeme_text && NULL == lexeme_text_length)) {
    RCL_SET_ERROR_MSG("text and length must both be set or both be NULL");
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 如果缓冲区中的第一个词素类型为 RCL_LEXEME_EOF（已到达文件末尾），则不接受任何词素
  if (RCL_LEXEME_EOF == buffer->impl->type[0]) {
    // 设置 lexeme_text 和 lexeme_text_length 的值
    if (NULL != lexeme_text && NULL != lexeme_text_length) {
      *lexeme_text = rcl_lexer_lookahead2_get_text(buffer);
      *lexeme_text_length = 0u;
    }
    return RCL_RET_OK;
  }

  // 如果文本索引大于等于第一个词素的结束位置，设置错误消息并返回错误代码
  if (buffer->impl->text_idx >= buffer->impl->end[0]) {
    RCL_SET_ERROR_MSG("no lexeme to accept");
    return RCL_RET_ERROR;
  }

  // 设置 lexeme_text 和 lexeme_text_length 的值
  if (NULL != lexeme_text && NULL != lexeme_text_length) {
    *lexeme_text = &(buffer->impl->text[buffer->impl->start[0]]);
    *lexeme_text_length = buffer->impl->end[0] - buffer->impl->start[0];
  }

  // 推进词法分析器的位置
  buffer->impl->text_idx = buffer->impl->end[0];

  // 将缓冲区中的第二个词素移动到第一个位置
  buffer->impl->start[0] = buffer->impl->start[1];
  buffer->impl->end[0] = buffer->impl->end[1];
  buffer->impl->type[0] = buffer->impl->type[1];

  return RCL_RET_OK;
}

/**
 * @brief 期望的词法单元类型与缓冲区中的词法单元类型进行比较，如果匹配则接受该词法单元。
 *
 * @param[in] buffer 指向rcl_lexer_lookahead2_t结构体的指针，用于存储词法分析器的状态。
 * @param[in] type 预期的词法单元类型。
 * @param[out] lexeme_text 如果函数成功执行，此参数将指向接受的词法单元文本。
 * @param[out] lexeme_text_length 如果函数成功执行，此参数将包含接受的词法单元文本的长度。
 * @return 返回RCL_RET_OK表示成功，其他值表示失败。
 */
rcl_ret_t rcl_lexer_lookahead2_expect(
    rcl_lexer_lookahead2_t* buffer,
    rcl_lexeme_t type,
    const char** lexeme_text,
    size_t* lexeme_text_length) {
  // 设置错误消息并返回错误代码
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_WRONG_LEXEME);

  // 定义返回值和词法单元变量
  rcl_ret_t ret;
  rcl_lexeme_t lexeme;

  // 获取缓冲区中的下一个词法单元
  ret = rcl_lexer_lookahead2_peek(buffer, &lexeme);
  // 如果获取失败，则返回错误代码
  if (RCL_RET_OK != ret) {
    return ret;
  }
  // 如果预期的词法单元类型与实际的词法单元类型不匹配
  if (type != lexeme) {
    // 如果实际词法单元类型为NONE或EOF，设置错误消息并返回错误代码
    if (RCL_LEXEME_NONE == lexeme || RCL_LEXEME_EOF == lexeme) {
      RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
          "Expected lexeme type (%d) not found, search ended at index %zu", type,
          buffer->impl->text_idx);
      return RCL_RET_WRONG_LEXEME;
    }
    // 设置错误消息并返回错误代码
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Expected lexeme type %d, got %d at index %zu", type, lexeme, buffer->impl->text_idx);
    return RCL_RET_WRONG_LEXEME;
  }
  // 如果预期的词法单元类型与实际的词法单元类型匹配，则接受该词法单元
  return rcl_lexer_lookahead2_accept(buffer, lexeme_text, lexeme_text_length);
}

/**
 * @brief 获取缓冲区中当前词法单元的文本。
 *
 * @param[in] buffer 指向rcl_lexer_lookahead2_t结构体的指针，用于存储词法分析器的状态。
 * @return 返回指向当前词法单元文本的指针。
 */
const char* rcl_lexer_lookahead2_get_text(const rcl_lexer_lookahead2_t* buffer) {
  // 返回指向当前词法单元文本的指针
  return &(buffer->impl->text[buffer->impl->text_idx]);
}
