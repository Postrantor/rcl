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

#include "rcl/node_options.h"

#include "rcl/arguments.h"
#include "rcl/domain_id.h"
#include "rcl/error_handling.h"
#include "rcl/logging_rosout.h"
#include "rcutils/macros.h"

/**
 * \brief 获取默认的节点选项
 *
 * 返回一个具有默认值的 rcl_node_options_t 结构体。
 *
 * \return 默认的节点选项
 */
rcl_node_options_t rcl_node_get_default_options() {
  // !!! 确保这些默认值的更改反映在头文件文档字符串中
  rcl_node_options_t default_options = {
      .allocator = rcl_get_default_allocator(),           ///< 默认分配器
      .use_global_arguments = true,                       ///< 使用全局参数
      .arguments = rcl_get_zero_initialized_arguments(),  ///< 初始化为零的参数
      .enable_rosout = true,                              ///< 启用 rosout
      .rosout_qos = rcl_qos_profile_rosout_default,       ///< rosout 的 QoS 配置
  };
  return default_options;
}

/**
 * \brief 复制节点选项
 *
 * 将给定的节点选项复制到另一个节点选项结构体中。
 *
 * \param[in] options 源节点选项
 * \param[out] options_out 目标节点选项
 * \return RCL_RET_OK 或相应的错误代码
 */
rcl_ret_t rcl_node_options_copy(
    const rcl_node_options_t* options, rcl_node_options_t* options_out) {
  RCUTILS_CAN_SET_MSG_AND_RETURN_WITH_ERROR_OF(RCL_RET_INVALID_ARGUMENT);

  RCL_CHECK_ARGUMENT_FOR_NULL(options, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(options_out, RCL_RET_INVALID_ARGUMENT);
  if (options_out == options) {
    RCL_SET_ERROR_MSG("Attempted to copy options into itself");
    return RCL_RET_INVALID_ARGUMENT;
  }
  if (NULL != options_out->arguments.impl) {
    RCL_SET_ERROR_MSG("Options out must be zero initialized");
    return RCL_RET_INVALID_ARGUMENT;
  }
  options_out->allocator = options->allocator;                        ///< 复制分配器
  options_out->use_global_arguments = options->use_global_arguments;  ///< 复制全局参数设置
  options_out->enable_rosout = options->enable_rosout;                ///< 复制 rosout 设置
  options_out->rosout_qos = options->rosout_qos;  ///< 复制 rosout 的 QoS 配置
  if (NULL != options->arguments.impl) {
    return rcl_arguments_copy(&(options->arguments), &(options_out->arguments));  ///< 复制参数
  }
  return RCL_RET_OK;
}

/**
 * \brief 销毁节点选项
 *
 * 清理并释放与给定的节点选项相关的资源。
 *
 * \param[in] options 要销毁的节点选项
 * \return RCL_RET_OK 或相应的错误代码
 */
rcl_ret_t rcl_node_options_fini(rcl_node_options_t* options) {
  RCL_CHECK_ARGUMENT_FOR_NULL(options, RCL_RET_INVALID_ARGUMENT);
  rcl_allocator_t allocator = options->allocator;
  RCL_CHECK_ALLOCATOR(&allocator, return RCL_RET_INVALID_ARGUMENT);

  if (options->arguments.impl) {
    rcl_ret_t ret = rcl_arguments_fini(&options->arguments);  ///< 销毁参数
    if (RCL_RET_OK != ret) {
      RCL_SET_ERROR_MSG("Failed to fini rcl arguments");
      return ret;
    }
  }

  return RCL_RET_OK;
}

#ifdef __cplusplus
}
#endif
