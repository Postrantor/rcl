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

/// @file

#ifndef RCL__SECURITY_H_
#define RCL__SECURITY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "rcl/allocator.h"
#include "rcl/types.h"
#include "rcl/visibility_control.h"
#include "rmw/security_options.h"

#ifndef ROS_SECURITY_ENCLAVE_OVERRIDE
/// \brief 环境变量中包含安全领域覆盖的名称。
#define ROS_SECURITY_ENCLAVE_OVERRIDE "ROS_SECURITY_ENCLAVE_OVERRIDE"
#endif

#ifndef ROS_SECURITY_KEYSTORE_VAR_NAME
/// \brief 环境变量中包含密钥库路径的名称。
#define ROS_SECURITY_KEYSTORE_VAR_NAME "ROS_SECURITY_KEYSTORE"
#endif

#ifndef ROS_SECURITY_STRATEGY_VAR_NAME
/// \brief 环境变量中包含安全策略的名称。
#define ROS_SECURITY_STRATEGY_VAR_NAME "ROS_SECURITY_STRATEGY"
#endif

#ifndef ROS_SECURITY_ENABLE_VAR_NAME
/// \brief 控制是否启用安全性的环境变量名称。
#define ROS_SECURITY_ENABLE_VAR_NAME "ROS_SECURITY_ENABLE"
#endif

/// \brief 从环境变量和给定名称中初始化安全选项。
/**
 * 根据环境初始化给定的安全选项。
 * 更多细节：
 *  \sa rcl_security_enabled()
 *  \sa rcl_get_enforcement_policy()
 *  \sa rcl_get_secure_root()
 *
 * \param[in] name 用于查找安全根路径的名称。
 * \param[in] allocator 用于分配的分配器。
 * \param[out] security_options 将根据环境配置的安全选项。
 * \return #RCL_RET_OK 如果安全选项正确返回，或
 * \return #RCL_RET_INVALID_ARGUMENT 如果参数无效，或
 * \return #RCL_RET_ERROR 如果发生意外错误
 */
RCL_PUBLIC
rcl_ret_t rcl_get_security_options_from_environment(
  const char * name, const rcutils_allocator_t * allocator,
  rmw_security_options_t * security_options);

/// \brief 根据环境检查是否需要使用安全性。
/**
 * 如果 `ROS_SECURITY_ENABLE` 环境变量设置为 "true"，则 `use_security` 将设置为 true。
 *
 * \param[out] use_security 不能为空。
 * \return #RCL_RET_INVALID_ARGUMENT 如果参数无效，或
 * \return #RCL_RET_ERROR 如果发生意外错误，或
 * \return #RCL_RET_OK。
 */
RCL_PUBLIC
rcl_ret_t rcl_security_enabled(bool * use_security);

/// \brief 从环境中获取安全执行策略。
/**
 * 根据 `ROS_SECURITY_STRATEGY` 环境变量的值设置 `policy`。
 * 如果 `ROS_SECURITY_STRATEGY` 是 "Enforce"，则 `policy` 将是 `RMW_SECURITY_ENFORCEMENT_ENFORCE`。
 * 如果不是，则 `policy` 将是 `RMW_SECURITY_ENFORCEMENT_PERMISSIVE`。
 *
 * \param[out] policy 不能为空。
 * \return #RCL_RET_INVALID_ARGUMENT 如果参数无效，或
 * \return #RCL_RET_ERROR 如果发生意外错误，或
 * \return #RCL_RET_OK。
 */
RCL_PUBLIC
rcl_ret_t rcl_get_enforcement_policy(rmw_security_enforcement_policy_t * policy);

/// \brief 给定领域名称返回安全根。
/**
 * 返回与领域名称关联的安全目录。
 *
 * 使用环境变量 `ROS_SECURITY_KEYSTORE` 的值作为根。
 * 使用传递的 `name` 从该根查找要使用的特定目录。
 * 例如，对于名为 "/a/b/c" 的上下文和根 "/r"，安全根路径将是
 * "/r/a/b/c"，其中分隔符 "/" 是针对目标文件系统的本地分隔符（例如，在 _WIN32 上为 "\"）。
 *
 * 但是，可以通过设置安全领域覆盖环境变量（`ROS_SECURITY_ENCLAVE_OVERRIDE`）来覆盖此扩展，
 * 允许用户显式指定要使用的确切领域 `name`。
 * 对于在运行时之前领域不确定的应用程序或在测试和使用其他可能无法轻松配置的工具时，
 * 这种覆盖非常有用。
 *
 * \param[in] name 经过验证的名称（单个令牌）
 * \param[in] allocator 用于分配的分配器
 * \return 机器特定（绝对）领域目录路径或失败时返回 NULL。
 *  返回的指针必须由此函数的调用者释放
 */
RCL_PUBLIC
char * rcl_get_secure_root(const char * name, const rcl_allocator_t * allocator);

#ifdef __cplusplus
}
#endif

#endif  // RCL__SECURITY_H_
