// Copyright 2018-2020 Open Source Robotics Foundation, Inc.
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

#include "rcl/security.h"

#include <stdbool.h>

#include "rcl/error_handling.h"
#include "rcutils/env.h"
#include "rcutils/filesystem.h"
#include "rcutils/logging_macros.h"
#include "rcutils/strdup.h"
#include "rmw/security_options.h"

/**
 * @brief 从环境变量中获取安全选项
 *
 * @param[in] name 节点名称
 * @param[in] allocator 分配器，用于分配内存
 * @param[out] security_options 用于存储从环境变量中获取的安全选项
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_get_security_options_from_environment(
    const char* name,
    const rcutils_allocator_t* allocator,
    rmw_security_options_t* security_options) {
  // 定义一个布尔值，表示是否使用安全功能
  bool use_security = false;
  // 检查安全功能是否启用，并将结果存储在use_security中
  rcl_ret_t ret = rcl_security_enabled(&use_security);
  // 如果返回值不是RCL_RET_OK，则返回错误代码
  if (RCL_RET_OK != ret) {
    return ret;
  }

  // 记录调试信息，显示是否使用安全功能
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Using security: %s", use_security ? "true" : "false");

  // 如果不使用安全功能，设置安全选项为宽松模式并返回RCL_RET_OK
  if (!use_security) {
    security_options->enforce_security = RMW_SECURITY_ENFORCEMENT_PERMISSIVE;
    return RCL_RET_OK;
  }

  // 获取安全策略并将其存储在security_options中
  ret = rcl_get_enforcement_policy(&security_options->enforce_security);
  // 如果返回值不是RCL_RET_OK，则返回错误代码
  if (RCL_RET_OK != ret) {
    return ret;
  }

  // 在这里进行文件发现
  char* secure_root = rcl_get_secure_root(name, allocator);
  // 如果找到了安全目录，将其存储在security_options中并记录信息
  if (secure_root) {
    RCUTILS_LOG_INFO_NAMED(ROS_PACKAGE_NAME, "Found security directory: %s", secure_root);
    security_options->security_root_path = secure_root;
  } else {
    // 如果没有找到安全目录且安全策略为强制模式，则返回错误代码
    if (RMW_SECURITY_ENFORCEMENT_ENFORCE == security_options->enforce_security) {
      return RCL_RET_ERROR;
    }
  }
  // 返回操作成功的结果
  return RCL_RET_OK;
}

/**
 * @brief 检查 ROS 2 安全功能是否启用。
 *
 * @param[out] use_security 返回安全功能是否启用的布尔值。
 * @return rcl_ret_t 返回 RCL_RET_OK 表示成功，其他值表示失败。
 */
rcl_ret_t rcl_security_enabled(bool* use_security) {
  // 获取环境变量 ROS_SECURITY_ENABLE_VAR_NAME 的值
  const char* ros_security_enable = NULL;
  const char* get_env_error_str = NULL;

  // 检查 use_security 参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(use_security, RCL_RET_INVALID_ARGUMENT);

  // 从环境变量中获取 ROS_SECURITY_ENABLE_VAR_NAME 的值
  get_env_error_str = rcutils_get_env(ROS_SECURITY_ENABLE_VAR_NAME, &ros_security_enable);
  if (NULL != get_env_error_str) {
    // 如果获取环境变量出错，设置错误消息并返回 RCL_RET_ERROR
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Error getting env var '" RCUTILS_STRINGIFY(ROS_SECURITY_ENABLE_VAR_NAME) "': %s\n",
        get_env_error_str);
    return RCL_RET_ERROR;
  }

  // 根据环境变量的值设置 use_security
  *use_security = (0 == strcmp(ros_security_enable, "true"));
  return RCL_RET_OK;
}

/**
 * @brief 获取 ROS 2 安全策略。
 *
 * @param[out] policy 返回安全策略的枚举值。
 * @return rcl_ret_t 返回 RCL_RET_OK 表示成功，其他值表示失败。
 */
rcl_ret_t rcl_get_enforcement_policy(rmw_security_enforcement_policy_t* policy) {
  // 获取环境变量 ROS_SECURITY_STRATEGY_VAR_NAME 的值
  const char* ros_enforce_security = NULL;
  const char* get_env_error_str = NULL;

  // 检查 policy 参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(policy, RCL_RET_INVALID_ARGUMENT);

  // 从环境变量中获取 ROS_SECURITY_STRATEGY_VAR_NAME 的值
  get_env_error_str = rcutils_get_env(ROS_SECURITY_STRATEGY_VAR_NAME, &ros_enforce_security);
  if (NULL != get_env_error_str) {
    // 如果获取环境变量出错，设置错误消息并返回 RCL_RET_ERROR
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Error getting env var '" RCUTILS_STRINGIFY(ROS_SECURITY_STRATEGY_VAR_NAME) "': %s\n",
        get_env_error_str);
    return RCL_RET_ERROR;
  }

  // 根据环境变量的值设置 policy
  *policy = (0 == strcmp(ros_enforce_security, "Enforce")) ? RMW_SECURITY_ENFORCEMENT_ENFORCE
                                                           : RMW_SECURITY_ENFORCEMENT_PERMISSIVE;
  return RCL_RET_OK;
}

/**
 * @brief 在指定的目录中查找与给定名称完全匹配的安全根路径。
 *
 * @param[in] name 要查找的名称。
 * @param[in] ros_secure_keystore_env ROS2 安全密钥库环境变量。
 * @param[in] allocator 用于分配内存的 rcl_allocator_t 结构体指针。
 * @return 返回找到的安全根路径，如果没有找到则返回 NULL。
 */
char* exact_match_lookup(
    const char* name, const char* ros_secure_keystore_env, const rcl_allocator_t* allocator) {
  // 在 <root dir> 目录中执行 enclave 名称的精确匹配。
  char* secure_root = NULL;
  char* enclaves_dir = NULL;
  enclaves_dir = rcutils_join_path(ros_secure_keystore_env, "enclaves", *allocator);

  // 当根命名空间被显式传入时，处理 "/" 情况。
  if (0 == strcmp(name, "/")) {
    secure_root = enclaves_dir;
  } else {
    char* relative_path = NULL;

    // 获取本地路径，忽略前导斜杠。
    // TODO(ros2team): 删除硬编码长度，使用根命名空间的长度代替。
    relative_path = rcutils_to_native_path(name + 1, *allocator);
    secure_root = rcutils_join_path(enclaves_dir, relative_path, *allocator);
    allocator->deallocate(relative_path, allocator->state);
    allocator->deallocate(enclaves_dir, allocator->state);
  }
  return secure_root;
}

/**
 * @brief 复制环境变量的值。
 *
 * @param[in] name 要复制的环境变量名称。
 * @param[in] allocator 用于分配内存的 rcl_allocator_t 结构体指针。
 * @param[out] value 存储复制后的环境变量值的指针。
 * @return 如果成功，则返回 NULL，否则返回错误信息。
 */
static const char* dupenv(const char* name, const rcl_allocator_t* allocator, char** value) {
  const char* buffer = NULL;
  const char* error = rcutils_get_env(name, &buffer);
  if (NULL != error) {
    return error;
  }
  *value = NULL;

  // 如果缓冲区不为空，则复制字符串。
  if (0 != strcmp("", buffer)) {
    *value = rcutils_strdup(buffer, *allocator);
    if (NULL == *value) {
      return "string duplication failed";
    }
  }
  return NULL;
}

/**
 * @brief 获取安全根目录的路径
 *
 * @param[in] name 要查找的节点名称
 * @param[in] allocator 分配器，用于分配内存
 * @return 返回安全根目录的路径，如果未找到或出错，则返回 NULL
 */
char* rcl_get_secure_root(const char* name, const rcl_allocator_t* allocator) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(name, NULL);
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "allocator is invalid", return NULL);

  char* secure_root = NULL;
  char* ros_secure_keystore_env = NULL;
  char* ros_secure_enclave_override_env = NULL;

  // 检查密钥库环境变量
  const char* error = dupenv(ROS_SECURITY_KEYSTORE_VAR_NAME, allocator, &ros_secure_keystore_env);
  if (NULL != error) {
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "failed to get %s: %s", ROS_SECURITY_KEYSTORE_VAR_NAME, error);
    return NULL;
  }

  if (NULL == ros_secure_keystore_env) {
    return NULL;  // 环境变量为空
  }

  // 检查领域覆盖环境变量
  error = dupenv(ROS_SECURITY_ENCLAVE_OVERRIDE, allocator, &ros_secure_enclave_override_env);
  if (NULL != error) {
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "failed to get %s: %s", ROS_SECURITY_ENCLAVE_OVERRIDE, error);
    goto leave_rcl_get_secure_root;
  }

  // 如果有可用的环境变量，使用下一个查找覆盖
  if (NULL != ros_secure_enclave_override_env) {
    secure_root =
        exact_match_lookup(ros_secure_enclave_override_env, ros_secure_keystore_env, allocator);
  } else {
    secure_root = exact_match_lookup(name, ros_secure_keystore_env, allocator);
  }

  if (NULL == secure_root) {
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "SECURITY ERROR: unable to find a folder matching the name '%s' in '%s'. ", name,
        ros_secure_keystore_env);
    goto leave_rcl_get_secure_root;
  }

  // 检查 secure_root 是否为目录
  if (!rcutils_is_directory(secure_root)) {
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "SECURITY ERROR: directory '%s' does not exist.", secure_root);
    allocator->deallocate(secure_root, allocator->state);
    secure_root = NULL;
  }

leave_rcl_get_secure_root:
  // 释放分配的内存
  allocator->deallocate(ros_secure_enclave_override_env, allocator->state);
  allocator->deallocate(ros_secure_keystore_env, allocator->state);
  return secure_root;
}
