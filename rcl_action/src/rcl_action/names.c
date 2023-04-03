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

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl_action/names.h"

#include <string.h>

#include "rcl/error_handling.h"
#include "rcutils/format_string.h"

/**
 * @brief 获取目标服务名称
 * 
 * @param action_name 动作名称
 * @param allocator 分配器
 * @param goal_service_name 目标服务名称的指针
 * @return rcl_ret_t 返回状态码
 */
rcl_ret_t rcl_action_get_goal_service_name(
  const char * action_name, rcl_allocator_t allocator, char ** goal_service_name)
{
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(&allocator, "allocator is invalid", return RCL_RET_INVALID_ARGUMENT);
  // 检查动作名称是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(action_name, RCL_RET_INVALID_ARGUMENT);
  // 检查动作名称长度是否为0
  if (0 == strlen(action_name)) {
    RCL_SET_ERROR_MSG("invalid empty action name");
    return RCL_RET_ACTION_NAME_INVALID;
  }
  // 检查目标服务名称是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(goal_service_name, RCL_RET_INVALID_ARGUMENT);
  // 检查目标服务名称是否已经被分配
  if (NULL != *goal_service_name) {
    RCL_SET_ERROR_MSG("writing action goal service name may leak memory");
    return RCL_RET_INVALID_ARGUMENT;
  }
  // 格式化目标服务名称字符串
  *goal_service_name = rcutils_format_string(allocator, "%s/_action/send_goal", action_name);
  // 检查目标服务名称是否分配成功
  if (NULL == *goal_service_name) {
    RCL_SET_ERROR_MSG("failed to allocate memory for action goal service name");
    return RCL_RET_BAD_ALLOC;
  }
  return RCL_RET_OK;
}

/**
 * @brief 获取取消服务名称
 * 
 * @param action_name 动作名称
 * @param allocator 分配器
 * @param cancel_service_name 取消服务名称的指针
 * @return rcl_ret_t 返回状态码
 */
rcl_ret_t rcl_action_get_cancel_service_name(
  const char * action_name, rcl_allocator_t allocator, char ** cancel_service_name)
{
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(&allocator, "allocator is invalid", return RCL_RET_INVALID_ARGUMENT);
  // 检查动作名称是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(action_name, RCL_RET_INVALID_ARGUMENT);
  // 检查动作名称长度是否为0
  if (0 == strlen(action_name)) {
    RCL_SET_ERROR_MSG("invalid empty action name");
    return RCL_RET_ACTION_NAME_INVALID;
  }
  // 检查取消服务名称是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(cancel_service_name, RCL_RET_INVALID_ARGUMENT);
  // 检查取消服务名称是否已经被分配
  if (NULL != *cancel_service_name) {
    RCL_SET_ERROR_MSG("writing action cancel service name may leak memory");
    return RCL_RET_INVALID_ARGUMENT;
  }
  // 格式化取消服务名称字符串
  *cancel_service_name = rcutils_format_string(allocator, "%s/_action/cancel_goal", action_name);
  // 检查取消服务名称是否分配成功
  if (NULL == *cancel_service_name) {
    RCL_SET_ERROR_MSG("failed to allocate memory for action cancel service name");
    return RCL_RET_BAD_ALLOC;
  }
  return RCL_RET_OK;
}

/**
 * @brief 获取结果服务名称
 * 
 * @param action_name 动作名称
 * @param allocator 分配器
 * @param result_service_name 结果服务名称的指针
 * @return rcl_ret_t 返回状态码
 */
rcl_ret_t rcl_action_get_result_service_name(
  const char * action_name, rcl_allocator_t allocator, char ** result_service_name)
{
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(&allocator, "allocator is invalid", return RCL_RET_INVALID_ARGUMENT);
  // 检查动作名称是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(action_name, RCL_RET_INVALID_ARGUMENT);
  // 检查动作名称长度是否为0
  if (0 == strlen(action_name)) {
    RCL_SET_ERROR_MSG("invalid empty action name");
    return RCL_RET_ACTION_NAME_INVALID;
  }
  // 检查结果服务名称是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(result_service_name, RCL_RET_INVALID_ARGUMENT);
  // 检查结果服务名称是否已经被分配
  if (NULL != *result_service_name) {
    RCL_SET_ERROR_MSG("writing action result service name may leak memory");
    return RCL_RET_INVALID_ARGUMENT;
  }
  // 格式化结果服务名称字符串
  *result_service_name = rcutils_format_string(allocator, "%s/_action/get_result", action_name);
  // 检查结果服务名称是否分配成功
  if (NULL == *result_service_name) {
    RCL_SET_ERROR_MSG("failed to allocate memory for action result service name");
    return RCL_RET_BAD_ALLOC;
  }
  return RCL_RET_OK;
}

/**
 * @brief 获取反馈主题名称
 * 
 * @param action_name 动作名称
 * @param allocator 分配器
 * @param feedback_topic_name 反馈主题名称的指针
 * @return rcl_ret_t 返回状态码
 */
rcl_ret_t rcl_action_get_feedback_topic_name(
  const char * action_name, rcl_allocator_t allocator, char ** feedback_topic_name)
{
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(&allocator, "allocator is invalid", return RCL_RET_INVALID_ARGUMENT);
  // 检查动作名称是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(action_name, RCL_RET_INVALID_ARGUMENT);
  // 检查动作名称长度是否为0
  if (0 == strlen(action_name)) {
    RCL_SET_ERROR_MSG("invalid empty action name");
    return RCL_RET_ACTION_NAME_INVALID;
  }
  // 检查反馈主题名称是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(feedback_topic_name, RCL_RET_INVALID_ARGUMENT);
  // 检查反馈主题名称是否已经被分配
  if (NULL != *feedback_topic_name) {
    RCL_SET_ERROR_MSG("writing action feedback topic name may leak memory");
    return RCL_RET_INVALID_ARGUMENT;
  }
  // 格式化反馈主题名称字符串
  *feedback_topic_name = rcutils_format_string(allocator, "%s/_action/feedback", action_name);
  // 检查反馈主题名称是否分配成功
  if (NULL == *feedback_topic_name) {
    RCL_SET_ERROR_MSG("failed to allocate memory for action feedback topic name");
    return RCL_RET_BAD_ALLOC;
  }
  return RCL_RET_OK;
}

/**
 * @brief 获取状态主题名称
 * 
 * @param action_name 动作名称
 * @param allocator 分配器
 * @param status_topic_name 状态主题名称的指针
 * @return rcl_ret_t 返回状态码
 */
rcl_ret_t rcl_action_get_status_topic_name(
  const char * action_name, rcl_allocator_t allocator, char ** status_topic_name)
{
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(&allocator, "allocator is invalid", return RCL_RET_INVALID_ARGUMENT);
  // 检查动作名称是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(action_name, RCL_RET_INVALID_ARGUMENT);
  // 检查动作名称长度是否为0
  if (0 == strlen(action_name)) {
    RCL_SET_ERROR_MSG("invalid empty action name");
    return RCL_RET_ACTION_NAME_INVALID;
  }
  // 检查状态主题名称是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(status_topic_name, RCL_RET_INVALID_ARGUMENT);
  // 检查状态主题名称是否已经被分配
  if (NULL != *status_topic_name) {
    RCL_SET_ERROR_MSG("writing action status topic name may leak memory");
    return RCL_RET_INVALID_ARGUMENT;
  }
  // 格式化状态主题名称字符串
  *status_topic_name = rcutils_format_string(allocator, "%s/_action/status", action_name);
  // 检查状态主题名称是否分配成功
  if (NULL == *status_topic_name) {
    RCL_SET_ERROR_MSG("failed to allocate memory for action status topic name");
    return RCL_RET_BAD_ALLOC;
  }
  return RCL_RET_OK;
}

#ifdef __cplusplus
}
#endif
