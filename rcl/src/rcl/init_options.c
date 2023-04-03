// Copyright 2015 Open Source Robotics Foundation, Inc.
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

#include "rcl/init_options.h"

#include "./common.h"
#include "./init_options_impl.h"
#include "rcl/error_handling.h"
#include "rcutils/logging_macros.h"
#include "rcutils/macros.h"
#include "rmw/error_handling.h"

/**
 * @brief 获取一个零初始化的 rcl_init_options_t 结构体实例
 *
 * @return 返回一个零初始化的 rcl_init_options_t 结构体实例
 */
rcl_init_options_t rcl_get_zero_initialized_init_options(void) {
  // 使用零初始化的结构体常量来创建并返回一个 rcl_init_options_t 实例
  return (const rcl_init_options_t){
      .impl = 0,
  };  // NOLINT(readability/braces): false positive
}

/**
 * @brief 使用默认值和零初始化实现给定的 init_options
 *
 * @param[in,out] init_options 指向要初始化的 rcl_init_options_t 结构体的指针
 * @param[in] allocator 分配器，用于分配内存
 * @return 返回 RCL_RET_OK 或相应的错误代码
 */
static inline rcl_ret_t _rcl_init_options_zero_init(
    rcl_init_options_t* init_options, rcl_allocator_t allocator) {
  // 为 init_options->impl 分配内存
  init_options->impl = allocator.allocate(sizeof(rcl_init_options_impl_t), allocator.state);

  // 检查分配是否成功，如果失败则返回 RCL_RET_BAD_ALLOC 错误
  RCL_CHECK_FOR_NULL_WITH_MSG(
      init_options->impl, "failed to allocate memory for init options impl",
      return RCL_RET_BAD_ALLOC);

  // 设置 init_options->impl 的分配器
  init_options->impl->allocator = allocator;

  // 设置 init_options->impl 的 rmw_init_options 为零初始化
  init_options->impl->rmw_init_options = rmw_get_zero_initialized_init_options();

  return RCL_RET_OK;
}

/**
 * @brief 初始化 rcl_init_options_t 结构体
 *
 * @param[in,out] init_options 指向要初始化的 rcl_init_options_t 结构体的指针
 * @param[in] allocator 分配器，用于分配内存
 * @return 返回 RCL_RET_OK 或相应的错误代码
 */
rcl_ret_t rcl_init_options_init(rcl_init_options_t* init_options, rcl_allocator_t allocator) {
  // 检查并返回错误代码
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_ALREADY_INIT);
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_BAD_ALLOC);
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_ERROR);

  // 检查 init_options 是否为空，如果为空则返回 RCL_RET_INVALID_ARGUMENT 错误
  RCL_CHECK_ARGUMENT_FOR_NULL(init_options, RCL_RET_INVALID_ARGUMENT);

  // 检查 init_options->impl 是否已经初始化，如果已经初始化则返回 RCL_RET_ALREADY_INIT 错误
  if (NULL != init_options->impl) {
    RCL_SET_ERROR_MSG("given init_options (rcl_init_options_t) is already initialized");
    return RCL_RET_ALREADY_INIT;
  }

  // 检查分配器是否有效，如果无效则返回 RCL_RET_INVALID_ARGUMENT 错误
  RCL_CHECK_ALLOCATOR(&allocator, return RCL_RET_INVALID_ARGUMENT);

  // 调用 _rcl_init_options_zero_init 函数进行初始化
  rcl_ret_t ret = _rcl_init_options_zero_init(init_options, allocator);
  if (RCL_RET_OK != ret) {
    return ret;
  }

  // 调用 rmw_init_options_init 函数进行初始化
  rmw_ret_t rmw_ret = rmw_init_options_init(&(init_options->impl->rmw_init_options), allocator);
  if (RMW_RET_OK != rmw_ret) {
    // 如果初始化失败，释放 init_options->impl 的内存并设置错误消息
    allocator.deallocate(init_options->impl, allocator.state);
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);

    // 返回转换后的 rcl 错误代码
    return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
  }

  return RCL_RET_OK;
}

/**
 * @brief 复制rcl_init_options_t结构体的内容
 *
 * 该函数将源rcl_init_options_t结构体的内容复制到目标rcl_init_options_t结构体中。
 *
 * @param[in] src 指向源rcl_init_options_t结构体的指针
 * @param[out] dst 指向目标rcl_init_options_t结构体的指针
 * @return 返回rcl_ret_t类型的结果，成功返回RCL_RET_OK，否则返回相应的错误代码
 */
rcl_ret_t rcl_init_options_copy(const rcl_init_options_t* src, rcl_init_options_t* dst) {
  // 检查输入参数是否有效，并设置相应的错误消息
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_ALREADY_INIT);
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_BAD_ALLOC);
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_ERROR);

  // 检查src和src->impl是否为空，为空则返回RCL_RET_INVALID_ARGUMENT错误
  RCL_CHECK_ARGUMENT_FOR_NULL(src, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(src->impl, RCL_RET_INVALID_ARGUMENT);
  // 检查分配器是否有效，无效则返回RCL_RET_INVALID_ARGUMENT错误
  RCL_CHECK_ALLOCATOR(&src->impl->allocator, return RCL_RET_INVALID_ARGUMENT);
  // 检查dst是否为空，为空则返回RCL_RET_INVALID_ARGUMENT错误
  RCL_CHECK_ARGUMENT_FOR_NULL(dst, RCL_RET_INVALID_ARGUMENT);
  // 检查dst->impl是否已经初始化，如果已经初始化则返回RCL_RET_ALREADY_INIT错误
  if (NULL != dst->impl) {
    RCL_SET_ERROR_MSG("given dst (rcl_init_options_t) is already initialized");
    return RCL_RET_ALREADY_INIT;
  }

  // 初始化dst（因为我们知道它处于零初始化状态）
  rcl_ret_t ret = _rcl_init_options_zero_init(dst, src->impl->allocator);
  // 如果初始化失败，则返回相应的错误代码
  if (RCL_RET_OK != ret) {
    return ret;  // error already set
  }

  // 将src的信息复制到dst中
  rmw_ret_t rmw_ret =
      rmw_init_options_copy(&(src->impl->rmw_init_options), &(dst->impl->rmw_init_options));
  // 如果复制失败，则处理错误并返回相应的错误代码
  if (RMW_RET_OK != rmw_ret) {
    rmw_error_string_t error_string = rmw_get_error_string();
    rmw_reset_error();
    ret = rcl_init_options_fini(dst);
    if (RCL_RET_OK != ret) {
      RCUTILS_LOG_ERROR_NAMED(
          "rcl",
          "failed to finalize dst rcl_init_options while handling failure to "
          "copy rmw_init_options, original ret '%d' and error: %s",
          rmw_ret, error_string.str);
      return ret;  // error already set
    }
    RCL_SET_ERROR_MSG(error_string.str);
    return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
  }

  // 成功完成复制操作，返回RCL_RET_OK
  return RCL_RET_OK;
}

/**
 * @brief 释放初始化选项资源
 *
 * @param init_options 指向要释放的rcl_init_options_t结构体的指针
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_init_options_fini(rcl_init_options_t* init_options) {
  // 检查init_options是否为空，如果为空则返回RCL_RET_INVALID_ARGUMENT
  RCL_CHECK_ARGUMENT_FOR_NULL(init_options, RCL_RET_INVALID_ARGUMENT);
  // 检查init_options->impl是否为空，如果为空则返回RCL_RET_INVALID_ARGUMENT
  RCL_CHECK_ARGUMENT_FOR_NULL(init_options->impl, RCL_RET_INVALID_ARGUMENT);
  // 获取分配器
  rcl_allocator_t allocator = init_options->impl->allocator;
  // 检查分配器是否有效，如果无效则返回RCL_RET_INVALID_ARGUMENT
  RCL_CHECK_ALLOCATOR(&allocator, return RCL_RET_INVALID_ARGUMENT);
  // 调用rmw_init_options_fini函数释放资源
  rmw_ret_t rmw_ret = rmw_init_options_fini(&(init_options->impl->rmw_init_options));
  // 判断rmw_init_options_fini的返回值是否为RMW_RET_OK
  if (RMW_RET_OK != rmw_ret) {
    // 设置错误信息
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    // 转换rmw_ret为rcl_ret并返回
    return rcl_convert_rmw_ret_to_rcl_ret(rmw_ret);
  }
  // 使用分配器释放init_options->impl
  allocator.deallocate(init_options->impl, allocator.state);
  // 返回RCL_RET_OK表示成功
  return RCL_RET_OK;
}

/**
 * @brief 获取域ID
 *
 * @param init_options 指向rcl_init_options_t结构体的指针
 * @param domain_id 存储域ID的指针
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_init_options_get_domain_id(
    const rcl_init_options_t* init_options, size_t* domain_id) {
  // 检查init_options是否为空，如果为空则返回RCL_RET_INVALID_ARGUMENT
  RCL_CHECK_ARGUMENT_FOR_NULL(init_options, RCL_RET_INVALID_ARGUMENT);
  // 检查init_options->impl是否为空，如果为空则返回RCL_RET_INVALID_ARGUMENT
  RCL_CHECK_ARGUMENT_FOR_NULL(init_options->impl, RCL_RET_INVALID_ARGUMENT);
  // 检查domain_id是否为空，如果为空则返回RCL_RET_INVALID_ARGUMENT
  RCL_CHECK_ARGUMENT_FOR_NULL(domain_id, RCL_RET_INVALID_ARGUMENT);
  // 获取域ID并存储到domain_id指向的内存中
  *domain_id = init_options->impl->rmw_init_options.domain_id;
  // 返回RCL_RET_OK表示成功
  return RCL_RET_OK;
}

/**
 * @brief 设置域ID
 *
 * @param init_options 指向rcl_init_options_t结构体的指针
 * @param domain_id 要设置的域ID
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t rcl_init_options_set_domain_id(rcl_init_options_t* init_options, size_t domain_id) {
  // 检查init_options是否为空，如果为空则返回RCL_RET_INVALID_ARGUMENT
  RCL_CHECK_ARGUMENT_FOR_NULL(init_options, RCL_RET_INVALID_ARGUMENT);
  // 检查init_options->impl是否为空，如果为空则返回RCL_RET_INVALID_ARGUMENT
  RCL_CHECK_ARGUMENT_FOR_NULL(init_options->impl, RCL_RET_INVALID_ARGUMENT);
  // 设置域ID
  init_options->impl->rmw_init_options.domain_id = domain_id;
  // 返回RCL_RET_OK表示成功
  return RCL_RET_OK;
}

/**
 * @brief 获取rmw_init_options_t结构体的指针
 *
 * @param init_options 指向rcl_init_options_t结构体的指针
 * @return rmw_init_options_t* 返回指向rmw_init_options_t结构体的指针，如果失败则返回NULL
 */
rmw_init_options_t* rcl_init_options_get_rmw_init_options(rcl_init_options_t* init_options) {
  // 检查init_options是否为空，如果为空则返回NULL
  RCL_CHECK_ARGUMENT_FOR_NULL(init_options, NULL);
  // 检查init_options->impl是否为空，如果为空则返回NULL
  RCL_CHECK_ARGUMENT_FOR_NULL(init_options->impl, NULL);
  // 返回指向rmw_init_options_t结构体的指针
  return &(init_options->impl->rmw_init_options);
}

/**
 * @brief 获取分配器
 *
 * @param init_options 指向rcl_init_options_t结构体的指针
 * @return const rcl_allocator_t* 返回指向rcl_allocator_t结构体的指针，如果失败则返回NULL
 */
const rcl_allocator_t* rcl_init_options_get_allocator(const rcl_init_options_t* init_options) {
  // 检查init_options是否为空，如果为空则返回NULL
  RCL_CHECK_ARGUMENT_FOR_NULL(init_options, NULL);
  // 检查init_options->impl是否为空，如果为空则返回NULL
  RCL_CHECK_ARGUMENT_FOR_NULL(init_options->impl, NULL);
  // 返回指向rcl_allocator_t结构体的指针
  return &(init_options->impl->allocator);
}

#ifdef __cplusplus
}
#endif
