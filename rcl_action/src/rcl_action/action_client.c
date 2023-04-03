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

#include "rcl_action/action_client.h"

#include "./action_client_impl.h"
#include "rcl/client.h"
#include "rcl/error_handling.h"
#include "rcl/graph.h"
#include "rcl/subscription.h"
#include "rcl/types.h"
#include "rcl/wait.h"
#include "rcl_action/default_qos.h"
#include "rcl_action/names.h"
#include "rcl_action/types.h"
#include "rcl_action/wait.h"
#include "rcutils/logging_macros.h"
#include "rcutils/strdup.h"
#include "rmw/qos_profiles.h"
#include "rmw/types.h"

/**
 * @brief 获取一个初始化为零的rcl_action_client_t实例
 *
 * 这个函数返回一个静态的、初始化为零的rcl_action_client_t实例。
 *
 * @return 初始化为零的rcl_action_client_t实例
 */
rcl_action_client_t rcl_action_get_zero_initialized_client(void)
{
  // 定义并初始化一个静态的空动作客户端
  static rcl_action_client_t null_action_client = {0};
  // 返回这个空动作客户端
  return null_action_client;
}

/**
 * @brief 获取一个初始化为零的rcl_action_client_impl_t实例
 *
 * 这个函数返回一个初始化为零的rcl_action_client_impl_t实例。
 *
 * @return 初始化为零的rcl_action_client_impl_t实例
 */
rcl_action_client_impl_t _rcl_action_get_zero_initialized_client_impl(void)
{
  // 获取并初始化一个空客户端
  rcl_client_t null_client = rcl_get_zero_initialized_client();
  // 获取并初始化一个空订阅
  rcl_subscription_t null_subscription = rcl_get_zero_initialized_subscription();
  // 定义并初始化一个空动作客户端实现
  rcl_action_client_impl_t null_action_client = {
    null_client,
    null_client,
    null_client,
    null_subscription,
    null_subscription,
    rcl_action_client_get_default_options(),
    NULL,
    0,
    0,
    0,
    0,
    0};
  // 返回这个空动作客户端实现
  return null_action_client;
}

/**
 * @brief 销毁一个rcl_action_client_t实例
 *
 * 这个函数销毁一个rcl_action_client_t实例，释放其相关资源。
 *
 * @param[in] action_client 要销毁的动作客户端指针
 * @param[in] node 动作客户端所属的节点指针
 * @param[in] allocator 用于分配内存的分配器
 * @return 返回RCL_RET_OK表示成功，其他值表示失败
 */
rcl_ret_t _rcl_action_client_fini_impl(
  rcl_action_client_t * action_client, rcl_node_t * node, rcl_allocator_t allocator)
{
  // 如果动作客户端实现为空，则直接返回成功
  if (NULL == action_client->impl) {
    return RCL_RET_OK;
  }
  // 初始化返回值为成功
  rcl_ret_t ret = RCL_RET_OK;
  // 销毁目标客户端，如果失败则设置返回值为错误
  if (RCL_RET_OK != rcl_client_fini(&action_client->impl->goal_client, node)) {
    ret = RCL_RET_ERROR;
  }
  // 销毁取消客户端，如果失败则设置返回值为错误
  if (RCL_RET_OK != rcl_client_fini(&action_client->impl->cancel_client, node)) {
    ret = RCL_RET_ERROR;
  }
  // 销毁结果客户端，如果失败则设置返回值为错误
  if (RCL_RET_OK != rcl_client_fini(&action_client->impl->result_client, node)) {
    ret = RCL_RET_ERROR;
  }
  // 销毁反馈订阅，如果失败则设置返回值为错误
  if (RCL_RET_OK != rcl_subscription_fini(&action_client->impl->feedback_subscription, node)) {
    ret = RCL_RET_ERROR;
  }
  // 销毁状态订阅，如果失败则设置返回值为错误
  if (RCL_RET_OK != rcl_subscription_fini(&action_client->impl->status_subscription, node)) {
    ret = RCL_RET_ERROR;
  }
  // 释放动作名称和动作客户端实现的内存
  allocator.deallocate(action_client->impl->action_name, allocator.state);
  allocator.deallocate(action_client->impl, allocator.state);
  // 将动作客户端实现设置为空
  action_client->impl = NULL;
  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Action client finalized");
  // 返回结果
  return ret;
}

// \internal 初始化一个特定于动作客户端的服务客户端。
// \param Type 服务类型
#define CLIENT_INIT(Type)                                                                   \
  /* 定义一个指向Type服务名称的字符指针 */                                   \
  char * Type##_service_name = NULL;                                                        \
  /* 获取Type服务名称 */                                                              \
  ret = rcl_action_get_##Type##_service_name(action_name, allocator, &Type##_service_name); \
  /* 检查返回值是否为RCL_RET_OK */                                                  \
  if (RCL_RET_OK != ret) {                                                                  \
    /* 重置错误信息 */                                                                \
    rcl_reset_error();                                                                      \
    /* 设置错误消息 */                                                                \
    RCL_SET_ERROR_MSG("failed to get " #Type " service name");                              \
    /* 判断返回值是否为RCL_RET_BAD_ALLOC */                                         \
    if (RCL_RET_BAD_ALLOC == ret) {                                                         \
      ret = RCL_RET_BAD_ALLOC;                                                              \
    } else {                                                                                \
      ret = RCL_RET_ERROR;                                                                  \
    }                                                                                       \
    /* 跳转到fail标签 */                                                               \
    goto fail;                                                                              \
  }                                                                                         \
  /* 定义Type服务客户端选项 */                                                     \
  rcl_client_options_t Type##_service_client_options = {                                    \
    .qos = options->Type##_service_qos, .allocator = allocator};                            \
  /* 初始化Type客户端 */                                                              \
  action_client->impl->Type##_client = rcl_get_zero_initialized_client();                   \
  /* 初始化Type服务客户端 */                                                        \
  ret = rcl_client_init(                                                                    \
    &action_client->impl->Type##_client, node, type_support->Type##_service_type_support,   \
    Type##_service_name, &Type##_service_client_options);                                   \
  /* 释放Type服务名称内存 */                                                        \
  allocator.deallocate(Type##_service_name, allocator.state);                               \
  /* 检查返回值是否为RCL_RET_OK */                                                  \
  if (RCL_RET_OK != ret) {                                                                  \
    /* 判断返回值是否为RCL_RET_BAD_ALLOC */                                         \
    if (RCL_RET_BAD_ALLOC == ret) {                                                         \
      ret = RCL_RET_BAD_ALLOC;                                                              \
    } else if (RCL_RET_SERVICE_NAME_INVALID == ret) {                                       \
      ret = RCL_RET_ACTION_NAME_INVALID;                                                    \
    } else {                                                                                \
      ret = RCL_RET_ERROR;                                                                  \
    }                                                                                       \
    /* 跳转到fail标签 */                                                               \
    goto fail;                                                                              \
  }

// \internal 初始化一个特定于动作客户端的主题订阅。
// \param Type 主题类型
#define SUBSCRIPTION_INIT(Type)                                                                 \
  /* 定义一个指向Type主题名称的字符指针 */                                       \
  char * Type##_topic_name = NULL;                                                              \
  /* 获取Type主题名称 */                                                                  \
  ret = rcl_action_get_##Type##_topic_name(action_name, allocator, &Type##_topic_name);         \
  /* 检查返回值是否为RCL_RET_OK */                                                      \
  if (RCL_RET_OK != ret) {                                                                      \
    /* 重置错误信息 */                                                                    \
    rcl_reset_error();                                                                          \
    /* 设置错误消息 */                                                                    \
    RCL_SET_ERROR_MSG("failed to get " #Type " topic name");                                    \
    /* 判断返回值是否为RCL_RET_BAD_ALLOC */                                             \
    if (RCL_RET_BAD_ALLOC == ret) {                                                             \
      ret = RCL_RET_BAD_ALLOC;                                                                  \
    } else {                                                                                    \
      ret = RCL_RET_ERROR;                                                                      \
    }                                                                                           \
    /* 跳转到fail标签 */                                                                   \
    goto fail;                                                                                  \
  }                                                                                             \
  /* 定义Type主题订阅选项 */                                                            \
  rcl_subscription_options_t Type##_topic_subscription_options =                                \
    rcl_subscription_get_default_options();                                                     \
  /* 设置Type主题订阅的QoS和分配器 */                                                \
  Type##_topic_subscription_options.qos = options->Type##_topic_qos;                            \
  Type##_topic_subscription_options.allocator = allocator;                                      \
  /* 初始化Type订阅 */                                                                     \
  action_client->impl->Type##_subscription = rcl_get_zero_initialized_subscription();           \
  /* 初始化Type主题订阅 */                                                               \
  ret = rcl_subscription_init(                                                                  \
    &action_client->impl->Type##_subscription, node, type_support->Type##_message_type_support, \
    Type##_topic_name, &Type##_topic_subscription_options);                                     \
  /* 释放Type主题名称内存 */                                                            \
  allocator.deallocate(Type##_topic_name, allocator.state);                                     \
  /* 检查返回值是否为RCL_RET_OK */                                                      \
  if (RCL_RET_OK != ret) {                                                                      \
    /* 判断返回值是否为RCL_RET_BAD_ALLOC */                                             \
    if (RCL_RET_BAD_ALLOC == ret) {                                                             \
      ret = RCL_RET_BAD_ALLOC;                                                                  \
    } else if (RCL_RET_TOPIC_NAME_INVALID == ret) {                                             \
      ret = RCL_RET_ACTION_NAME_INVALID;                                                        \
    } else {                                                                                    \
      ret = RCL_RET_ERROR;                                                                      \
    }                                                                                           \
    /* 跳转到fail标签 */                                                                   \
    goto fail;                                                                                  \
  }

/**
 * @brief 初始化动作客户端
 *
 * @param[in] action_client 动作客户端指针
 * @param[in] node 节点指针
 * @param[in] type_support 类型支持结构体指针
 * @param[in] action_name 动作名称字符串
 * @param[in] options 动作客户端选项指针
 * @return rcl_ret_t 返回状态码
 */
rcl_ret_t rcl_action_client_init(
  rcl_action_client_t * action_client, rcl_node_t * node,
  const rosidl_action_type_support_t * type_support, const char * action_name,
  const rcl_action_client_options_t * options)
{
  // 检查参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(action_client, RCL_RET_INVALID_ARGUMENT);
  // 检查节点是否有效
  if (!rcl_node_is_valid(node)) {
    return RCL_RET_NODE_INVALID;
  }
  // 检查类型支持、动作名称和选项是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(type_support, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(action_name, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(options, RCL_RET_INVALID_ARGUMENT);
  // 获取分配器
  rcl_allocator_t allocator = options->allocator;
  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(&allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);

  rcl_ret_t ret = RCL_RET_OK;
  rcl_ret_t fini_ret = RCL_RET_OK;
  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(
    ROS_PACKAGE_NAME, "Initializing client for action name '%s'", action_name);
  // 检查动作客户端是否已初始化
  if (NULL != action_client->impl) {
    RCL_SET_ERROR_MSG("action client already initialized, or memory was uninitialized");
    return RCL_RET_ALREADY_INIT;
  }
  // 为实现结构体分配空间
  action_client->impl = allocator.allocate(sizeof(rcl_action_client_impl_t), allocator.state);
  // 检查分配结果
  RCL_CHECK_FOR_NULL_WITH_MSG(
    action_client->impl, "allocating memory failed", return RCL_RET_BAD_ALLOC);

  // 避免未初始化的指针导致初始化失败
  *action_client->impl = _rcl_action_get_zero_initialized_client_impl();
  // 复制动作客户端名称和选项
  action_client->impl->action_name = rcutils_strdup(action_name, allocator);
  // 检查复制结果
  if (NULL == action_client->impl->action_name) {
    RCL_SET_ERROR_MSG("failed to duplicate action name");
    ret = RCL_RET_BAD_ALLOC;
    goto fail;
  }
  action_client->impl->options = *options;

  // 初始化动作服务客户端
  CLIENT_INIT(goal);
  CLIENT_INIT(cancel);
  CLIENT_INIT(result);

  // 初始化动作主题订阅
  SUBSCRIPTION_INIT(feedback);
  SUBSCRIPTION_INIT(status);

  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Action client initialized");
  return ret;
fail:
  // 清理动作客户端
  fini_ret = _rcl_action_client_fini_impl(action_client, node, allocator);
  if (RCL_RET_OK != fini_ret) {
    RCL_SET_ERROR_MSG("failed to cleanup action client");
    ret = RCL_RET_ERROR;
  }
  return ret;
}

/**
 * @brief 销毁动作客户端
 *
 * @param[in] action_client 动作客户端指针
 * @param[in] node 节点指针
 * @return rcl_ret_t 返回状态码
 */
rcl_ret_t rcl_action_client_fini(rcl_action_client_t * action_client, rcl_node_t * node)
{
  // 记录调试信息
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Finalizing action client");
  // 检查动作客户端是否有效
  if (!rcl_action_client_is_valid(action_client)) {
    return RCL_RET_ACTION_CLIENT_INVALID;  // error already set
  }
  // 检查节点是否有效（除上下文外）
  if (!rcl_node_is_valid_except_context(node)) {
    return RCL_RET_NODE_INVALID;  // error already set
  }
  // 销毁动作客户端实现
  return _rcl_action_client_fini_impl(action_client, node, action_client->impl->options.allocator);
}

/**
 * @brief 获取默认的rcl_action_client选项
 *
 * @return rcl_action_client_options_t 默认选项
 */
rcl_action_client_options_t rcl_action_client_get_default_options(void)
{
  // 定义一个静态的默认选项变量
  static rcl_action_client_options_t default_options;
  // 设置目标服务的QoS配置为默认服务QoS配置
  default_options.goal_service_qos = rmw_qos_profile_services_default;
  // 设置取消服务的QoS配置为默认服务QoS配置
  default_options.cancel_service_qos = rmw_qos_profile_services_default;
  // 设置结果服务的QoS配置为默认服务QoS配置
  default_options.result_service_qos = rmw_qos_profile_services_default;
  // 设置反馈主题的QoS配置为默认QoS配置
  default_options.feedback_topic_qos = rmw_qos_profile_default;
  // 设置状态主题的QoS配置为默认动作QoS配置
  default_options.status_topic_qos = rcl_action_qos_profile_status_default;
  // 设置分配器为默认分配器
  default_options.allocator = rcl_get_default_allocator();
  // 返回默认选项
  return default_options;
}

/**
 * @brief 检查动作服务器是否可用
 *
 * @param[in] node 节点指针
 * @param[in] client 动作客户端指针
 * @param[out] is_available 用于存储服务器是否可用的布尔值
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_action_server_is_available(
  const rcl_node_t * node, const rcl_action_client_t * client, bool * is_available)
{
  // 检查节点是否有效
  if (!rcl_node_is_valid(node)) {
    return RCL_RET_NODE_INVALID;  // 错误已设置
  }
  // 检查动作客户端是否有效
  if (!rcl_action_client_is_valid(client)) {
    return RCL_RET_ACTION_CLIENT_INVALID;  // 错误已设置
  }
  // 检查is_available参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(is_available, RCL_RET_INVALID_ARGUMENT);

  bool temp;
  rcl_ret_t ret;
  *is_available = true;

  // 检查目标服务服务器是否可用
  ret = rcl_service_server_is_available(node, &(client->impl->goal_client), &temp);
  if (RCL_RET_OK != ret) {
    return ret;  // 错误已设置
  }
  *is_available = (*is_available && temp);

  // 检查取消服务服务器是否可用
  ret = rcl_service_server_is_available(node, &(client->impl->cancel_client), &temp);
  if (RCL_RET_OK != ret) {
    return ret;  // 错误已设置
  }
  *is_available = (*is_available && temp);

  // 检查结果服务服务器是否可用
  ret = rcl_service_server_is_available(node, &(client->impl->result_client), &temp);
  if (RCL_RET_OK != ret) {
    return ret;  // 错误已设置
  }
  *is_available = (*is_available && temp);

  size_t number_of_publishers;

  // 获取反馈订阅的发布者数量
  ret = rcl_subscription_get_publisher_count(
    &(client->impl->feedback_subscription), &number_of_publishers);
  if (RCL_RET_OK != ret) {
    return ret;  // 错误已设置
  }
  *is_available = *is_available && (number_of_publishers != 0);

  // 获取状态订阅的发布者数量
  ret = rcl_subscription_get_publisher_count(
    &(client->impl->status_subscription), &number_of_publishers);
  if (RCL_RET_OK != ret) {
    return ret;  // 错误已设置
  }
  *is_available = *is_available && (number_of_publishers != 0);

  // 返回操作结果
  return RCL_RET_OK;
}

/*!
 * \internal
 * \brief 发送一个特定于动作客户端的服务请求。
 * \param Type 请求类型
 * \param request 请求数据
 * \param sequence_number 序列号
 */
#define SEND_SERVICE_REQUEST(Type, request, sequence_number)                                       \
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Sending action " #Type " request");                   \
  if (!rcl_action_client_is_valid(action_client)) {                                                \
    return RCL_RET_ACTION_CLIENT_INVALID; /* error already set */                                  \
  }                                                                                                \
  RCL_CHECK_ARGUMENT_FOR_NULL(request, RCL_RET_INVALID_ARGUMENT);                                  \
  RCL_CHECK_ARGUMENT_FOR_NULL(sequence_number, RCL_RET_INVALID_ARGUMENT);                          \
  rcl_ret_t ret = rcl_send_request(&action_client->impl->Type##_client, request, sequence_number); \
  if (RCL_RET_OK != ret) {                                                                         \
    return RCL_RET_ERROR; /* error already set */                                                  \
  }                                                                                                \
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Action " #Type " request sent");                      \
  return RCL_RET_OK;

/*!
 * \internal
 * \brief 接收一个特定于动作客户端的服务响应。
 * \param Type 响应类型
 * \param response_header 响应头
 * \param response 响应数据
 */
#define TAKE_SERVICE_RESPONSE(Type, response_header, response)                         \
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Taking action " #Type " response");       \
  if (!rcl_action_client_is_valid(action_client)) {                                    \
    return RCL_RET_ACTION_CLIENT_INVALID; /* error already set */                      \
  }                                                                                    \
  RCL_CHECK_ARGUMENT_FOR_NULL(response_header, RCL_RET_INVALID_ARGUMENT);              \
  RCL_CHECK_ARGUMENT_FOR_NULL(response, RCL_RET_INVALID_ARGUMENT);                     \
  rcl_ret_t ret =                                                                      \
    rcl_take_response(&action_client->impl->Type##_client, response_header, response); \
  if (RCL_RET_OK != ret) {                                                             \
    if (RCL_RET_BAD_ALLOC == ret) {                                                    \
      return RCL_RET_BAD_ALLOC; /* error already set */                                \
    }                                                                                  \
    if (RCL_RET_CLIENT_TAKE_FAILED == ret) {                                           \
      return RCL_RET_ACTION_CLIENT_TAKE_FAILED;                                        \
    }                                                                                  \
    return RCL_RET_ERROR; /* error already set */                                      \
  }                                                                                    \
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Action " #Type " response taken");        \
  return RCL_RET_OK;

/**
 * @brief 发送目标请求
 *
 * @param[in] action_client 动作客户端指针
 * @param[in] ros_goal_request 目标请求数据
 * @param[out] sequence_number 序列号指针
 * @return rcl_ret_t 返回执行结果
 */
rcl_ret_t rcl_action_send_goal_request(
  const rcl_action_client_t * action_client, const void * ros_goal_request,
  int64_t * sequence_number)
{
  // 发送服务请求
  SEND_SERVICE_REQUEST(goal, ros_goal_request, sequence_number);
}

/**
 * @brief 接收目标响应
 *
 * @param[in] action_client 动作客户端指针
 * @param[out] response_header 响应头指针
 * @param[out] ros_goal_response 目标响应数据
 * @return rcl_ret_t 返回执行结果
 */
rcl_ret_t rcl_action_take_goal_response(
  const rcl_action_client_t * action_client, rmw_request_id_t * response_header,
  void * ros_goal_response)
{
  // 接收服务响应
  TAKE_SERVICE_RESPONSE(goal, response_header, ros_goal_response);
}

/**
 * @brief 发送结果请求
 *
 * @param[in] action_client 动作客户端指针
 * @param[in] ros_result_request 结果请求数据
 * @param[out] sequence_number 序列号指针
 * @return rcl_ret_t 返回执行结果
 */
rcl_ret_t rcl_action_send_result_request(
  const rcl_action_client_t * action_client, const void * ros_result_request,
  int64_t * sequence_number)
{
  // 发送服务请求
  SEND_SERVICE_REQUEST(result, ros_result_request, sequence_number);
}

/**
 * @brief 接收结果响应
 *
 * @param[in] action_client 动作客户端指针
 * @param[out] response_header 响应头指针
 * @param[out] ros_result_response 结果响应数据
 * @return rcl_ret_t 返回执行结果
 */
rcl_ret_t rcl_action_take_result_response(
  const rcl_action_client_t * action_client, rmw_request_id_t * response_header,
  void * ros_result_response)
{
  // 接收服务响应
  TAKE_SERVICE_RESPONSE(result, response_header, ros_result_response);
}

/**
 * @brief 发送取消请求
 *
 * @param[in] action_client 动作客户端指针
 * @param[in] ros_cancel_request 取消请求数据
 * @param[out] sequence_number 序列号指针
 * @return rcl_ret_t 返回执行结果
 */
rcl_ret_t rcl_action_send_cancel_request(
  const rcl_action_client_t * action_client, const void * ros_cancel_request,
  int64_t * sequence_number)
{
  // 发送服务请求
  SEND_SERVICE_REQUEST(cancel, ros_cancel_request, sequence_number);
}

/**
 * @brief 接收取消响应
 *
 * @param[in] action_client 动作客户端指针
 * @param[out] response_header 响应头指针
 * @param[out] ros_cancel_response 取消响应数据
 * @return rcl_ret_t 返回执行结果
 */
rcl_ret_t rcl_action_take_cancel_response(
  const rcl_action_client_t * action_client, rmw_request_id_t * response_header,
  void * ros_cancel_response)
{
  // 接收服务响应
  TAKE_SERVICE_RESPONSE(cancel, response_header, ros_cancel_response);
}

/** \internal 接收一个特定的 action client 主题消息。
 *  \brief 定义 TAKE_MESSAGE(Type) 宏，用于接收特定类型的 action 消息。
 */
#define TAKE_MESSAGE(Type)                                                                \
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Taking action " #Type);                      \
  /* 检查 action_client 是否有效 */                                                 \
  if (!rcl_action_client_is_valid(action_client)) {                                       \
    return RCL_RET_ACTION_CLIENT_INVALID; /* error already set */                         \
  }                                                                                       \
  /* 检查 ros_##Type 参数是否为空 */                                              \
  RCL_CHECK_ARGUMENT_FOR_NULL(ros_##Type, RCL_RET_INVALID_ARGUMENT);                      \
  rmw_message_info_t message_info; /* ignored */                                          \
  /* 调用 rcl_take 函数接收指定类型的消息 */                                 \
  rcl_ret_t ret =                                                                         \
    rcl_take(&action_client->impl->Type##_subscription, ros_##Type, &message_info, NULL); \
  /* 判断 rcl_take 的返回值 */                                                      \
  if (RCL_RET_OK != ret) {                                                                \
    if (RCL_RET_SUBSCRIPTION_TAKE_FAILED == ret) {                                        \
      return RCL_RET_ACTION_CLIENT_TAKE_FAILED;                                           \
    }                                                                                     \
    if (RCL_RET_BAD_ALLOC == ret) {                                                       \
      return RCL_RET_BAD_ALLOC;                                                           \
    }                                                                                     \
    return RCL_RET_ERROR;                                                                 \
  }                                                                                       \
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Action " #Type " taken");                    \
  return RCL_RET_OK;

/**
 *  \brief 接收反馈消息
 *  \param[in] action_client 指向 rcl_action_client_t 结构体的指针
 *  \param[out] ros_feedback 存储接收到的反馈消息的指针
 *  \return 返回操作结果，成功返回 RCL_RET_OK
 */
rcl_ret_t rcl_action_take_feedback(const rcl_action_client_t * action_client, void * ros_feedback)
{
  TAKE_MESSAGE(feedback);
}

/**
 *  \brief 接收状态消息
 *  \param[in] action_client 指向 rcl_action_client_t 结构体的指针
 *  \param[out] ros_status 存储接收到的状态消息的指针
 *  \return 返回操作结果，成功返回 RCL_RET_OK
 */
rcl_ret_t rcl_action_take_status(const rcl_action_client_t * action_client, void * ros_status)
{
  TAKE_MESSAGE(status);
}

/**
 *  \brief 获取 action 客户端的 action 名称
 *  \param[in] action_client 指向 rcl_action_client_t 结构体的指针
 *  \return 返回 action 名称字符串，如果 action_client 无效则返回 NULL
 */
const char * rcl_action_client_get_action_name(const rcl_action_client_t * action_client)
{
  if (!rcl_action_client_is_valid(action_client)) {
    return NULL;
  }
  return action_client->impl->action_name;
}

/**
 *  \brief 获取 action 客户端的选项
 *  \param[in] action_client 指向 rcl_action_client_t 结构体的指针
 *  \return 返回指向 rcl_action_client_options_t 结构体的指针，如果 action_client 无效则返回 NULL
 */
const rcl_action_client_options_t * rcl_action_client_get_options(
  const rcl_action_client_t * action_client)
{
  if (!rcl_action_client_is_valid(action_client)) {
    return NULL;
  }
  return &action_client->impl->options;
}

/**
 *  \brief 检查 action 客户端是否有效
 *  \param[in] action_client 指向 rcl_action_client_t 结构体的指针
 *  \return 如果 action_client 有效则返回 true，否则返回 false
 */
bool rcl_action_client_is_valid(const rcl_action_client_t * action_client)
{
  RCL_CHECK_FOR_NULL_WITH_MSG(action_client, "action client pointer is invalid", return false);
  RCL_CHECK_FOR_NULL_WITH_MSG(
    action_client->impl, "action client implementation is invalid", return false);
  if (!rcl_client_is_valid(&action_client->impl->goal_client)) {
    rcl_reset_error();
    RCL_SET_ERROR_MSG("goal client is invalid");
    return false;
  }
  if (!rcl_client_is_valid(&action_client->impl->cancel_client)) {
    rcl_reset_error();
    RCL_SET_ERROR_MSG("cancel client is invalid");
    return false;
  }
  if (!rcl_client_is_valid(&action_client->impl->result_client)) {
    rcl_reset_error();
    RCL_SET_ERROR_MSG("result client is invalid");
    return false;
  }
  if (!rcl_subscription_is_valid(&action_client->impl->feedback_subscription)) {
    rcl_reset_error();
    RCL_SET_ERROR_MSG("feedback subscription is invalid");
    return false;
  }
  if (!rcl_subscription_is_valid(&action_client->impl->status_subscription)) {
    rcl_reset_error();
    RCL_SET_ERROR_MSG("status subscription is invalid");
    return false;
  }
  return true;
}

/**
 * @brief 将动作客户端添加到等待集合中
 *
 * @param wait_set 指向要操作的rcl_wait_set_t结构体的指针
 * @param action_client 指向要添加的rcl_action_client_t结构体的指针
 * @param client_index 返回添加的动作客户端在等待集合中的客户端索引
 * @param subscription_index 返回添加的动作客户端在等待集合中的订阅索引
 * @return rcl_ret_t 返回操作结果，成功返回RCL_RET_OK
 */
rcl_ret_t rcl_action_wait_set_add_action_client(
  rcl_wait_set_t * wait_set, const rcl_action_client_t * action_client, size_t * client_index,
  size_t * subscription_index)
{
  rcl_ret_t ret;
  // 检查wait_set参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(wait_set, RCL_RET_WAIT_SET_INVALID);
  // 检查action_client是否有效
  if (!rcl_action_client_is_valid(action_client)) {
    return RCL_RET_ACTION_CLIENT_INVALID;  // error already set
  }

  // 等待动作目标服务响应消息
  ret = rcl_wait_set_add_client(
    wait_set, &action_client->impl->goal_client, &action_client->impl->wait_set_goal_client_index);
  if (RCL_RET_OK != ret) {
    return ret;
  }
  // 等待动作取消服务响应消息
  ret = rcl_wait_set_add_client(
    wait_set, &action_client->impl->cancel_client,
    &action_client->impl->wait_set_cancel_client_index);
  if (RCL_RET_OK != ret) {
    return ret;
  }
  // 等待动作结果服务响应消息
  ret = rcl_wait_set_add_client(
    wait_set, &action_client->impl->result_client,
    &action_client->impl->wait_set_result_client_index);
  if (RCL_RET_OK != ret) {
    return ret;
  }
  // 等待动作反馈消息
  ret = rcl_wait_set_add_subscription(
    wait_set, &action_client->impl->feedback_subscription,
    &action_client->impl->wait_set_feedback_subscription_index);
  if (RCL_RET_OK != ret) {
    return ret;
  }
  // 等待动作状态消息
  ret = rcl_wait_set_add_subscription(
    wait_set, &action_client->impl->status_subscription,
    &action_client->impl->wait_set_status_subscription_index);
  if (RCL_RET_OK != ret) {
    return ret;
  }

  if (NULL != client_index) {
    // 目标客户端是第一个添加的
    *client_index = action_client->impl->wait_set_goal_client_index;
  }
  if (NULL != subscription_index) {
    // 反馈订阅是第一个添加的
    *subscription_index = action_client->impl->wait_set_feedback_subscription_index;
  }
  return RCL_RET_OK;
}

/**
 * @brief 获取动作客户端在等待集合中的实体数量
 *
 * @param action_client 指向要查询的rcl_action_client_t结构体的指针
 * @param num_subscriptions 返回订阅实体的数量
 * @param num_guard_conditions 返回保护条件实体的数量
 * @param num_timers 返回定时器实体的数量
 * @param num_clients 返回客户端实体的数量
 * @param num_services 返回服务实体的数量
 * @return rcl_ret_t 返回操作结果，成功返回RCL_RET_OK
 */
rcl_ret_t rcl_action_client_wait_set_get_num_entities(
  const rcl_action_client_t * action_client, size_t * num_subscriptions,
  size_t * num_guard_conditions, size_t * num_timers, size_t * num_clients, size_t * num_services)
{
  // 检查action_client是否有效
  if (!rcl_action_client_is_valid(action_client)) {
    return RCL_RET_ACTION_CLIENT_INVALID;  // error already set
  }
  // 检查参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(num_subscriptions, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(num_guard_conditions, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(num_timers, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(num_clients, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(num_services, RCL_RET_INVALID_ARGUMENT);
  // 设置实体数量
  *num_subscriptions = 2;
  *num_guard_conditions = 0;
  *num_timers = 0;
  *num_clients = 3;
  *num_services = 0;
  return RCL_RET_OK;
}

/**
 * @brief 获取与动作客户端相关的实体是否准备就绪
 *
 * @param[in] wait_set 等待集，用于检查订阅、服务和客户端是否有新事件
 * @param[in] action_client 动作客户端，用于与动作服务器进行通信
 * @param[out] is_feedback_ready 反馈订阅是否准备就绪的布尔值指针
 * @param[out] is_status_ready 状态订阅是否准备就绪的布尔值指针
 * @param[out] is_goal_response_ready 目标响应客户端是否准备就绪的布尔值指针
 * @param[out] is_cancel_response_ready 取消响应客户端是否准备就绪的布尔值指针
 * @param[out] is_result_response_ready 结果响应客户端是否准备就绪的布尔值指针
 * @return rcl_ret_t 返回操作结果，成功返回RCL_RET_OK，否则返回相应的错误代码
 */
rcl_ret_t rcl_action_client_wait_set_get_entities_ready(
  const rcl_wait_set_t * wait_set, const rcl_action_client_t * action_client,
  bool * is_feedback_ready, bool * is_status_ready, bool * is_goal_response_ready,
  bool * is_cancel_response_ready, bool * is_result_response_ready)
{
  // 检查wait_set参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(wait_set, RCL_RET_WAIT_SET_INVALID);
  // 检查action_client是否有效
  if (!rcl_action_client_is_valid(action_client)) {
    return RCL_RET_ACTION_CLIENT_INVALID;  // error already set
  }
  // 检查is_feedback_ready参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(is_feedback_ready, RCL_RET_INVALID_ARGUMENT);
  // 检查is_status_ready参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(is_status_ready, RCL_RET_INVALID_ARGUMENT);
  // 检查is_goal_response_ready参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(is_goal_response_ready, RCL_RET_INVALID_ARGUMENT);
  // 检查is_cancel_response_ready参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(is_cancel_response_ready, RCL_RET_INVALID_ARGUMENT);
  // 检查is_result_response_ready参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(is_result_response_ready, RCL_RET_INVALID_ARGUMENT);

  // 获取动作客户端的实现细节
  const rcl_action_client_impl_t * impl = action_client->impl;
  // 获取各个实体在wait_set中的索引
  const size_t feedback_index = impl->wait_set_feedback_subscription_index;
  const size_t status_index = impl->wait_set_status_subscription_index;
  const size_t goal_index = impl->wait_set_goal_client_index;
  const size_t cancel_index = impl->wait_set_cancel_client_index;
  const size_t result_index = impl->wait_set_result_client_index;
  // 检查各个实体的索引是否越界
  if (feedback_index >= wait_set->size_of_subscriptions) {
    RCL_SET_ERROR_MSG("wait set index for feedback subscription is out of bounds");
    return RCL_RET_ERROR;
  }
  if (status_index >= wait_set->size_of_subscriptions) {
    RCL_SET_ERROR_MSG("wait set index for status subscription is out of bounds");
    return RCL_RET_ERROR;
  }
  if (goal_index >= wait_set->size_of_clients) {
    RCL_SET_ERROR_MSG("wait set index for goal client is out of bounds");
    return RCL_RET_ERROR;
  }
  if (cancel_index >= wait_set->size_of_clients) {
    RCL_SET_ERROR_MSG("wait set index for cancel client is out of bounds");
    return RCL_RET_ERROR;
  }
  if (result_index >= wait_set->size_of_clients) {
    RCL_SET_ERROR_MSG("wait set index for result client is out of bounds");
    return RCL_RET_ERROR;
  }

  // 获取各个实体的指针
  const rcl_subscription_t * feedback_subscription = wait_set->subscriptions[feedback_index];
  const rcl_subscription_t * status_subscription = wait_set->subscriptions[status_index];
  const rcl_client_t * goal_client = wait_set->clients[goal_index];
  const rcl_client_t * cancel_client = wait_set->clients[cancel_index];
  const rcl_client_t * result_client = wait_set->clients[result_index];
  // 检查各个实体是否准备就绪
  *is_feedback_ready = (&impl->feedback_subscription == feedback_subscription);
  *is_status_ready = (&impl->status_subscription == status_subscription);
  *is_goal_response_ready = (&impl->goal_client == goal_client);
  *is_cancel_response_ready = (&impl->cancel_client == cancel_client);
  *is_result_response_ready = (&impl->result_client == result_client);
  // 返回操作成功
  return RCL_RET_OK;
}

/**
 * @brief 设置目标客户端回调函数
 * 
 * @param action_client 指向rcl_action_client_t结构体的指针
 * @param callback rcl_event_callback_t类型的回调函数
 * @param user_data 用户数据，将传递给回调函数
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_action_client_set_goal_client_callback(
  const rcl_action_client_t * action_client, rcl_event_callback_t callback, const void * user_data)
{
  // 检查action_client是否有效
  if (!rcl_action_client_is_valid(action_client)) {
    return RCL_RET_ACTION_CLIENT_INVALID;
  }

  // 设置新响应回调函数
  return rcl_client_set_on_new_response_callback(
    &action_client->impl->goal_client, callback, user_data);
}

/**
 * @brief 设置取消客户端回调函数
 * 
 * @param action_client 指向rcl_action_client_t结构体的指针
 * @param callback rcl_event_callback_t类型的回调函数
 * @param user_data 用户数据，将传递给回调函数
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_action_client_set_cancel_client_callback(
  const rcl_action_client_t * action_client, rcl_event_callback_t callback, const void * user_data)
{
  // 检查action_client是否有效
  if (!rcl_action_client_is_valid(action_client)) {
    return RCL_RET_ACTION_CLIENT_INVALID;
  }

  // 设置新响应回调函数
  return rcl_client_set_on_new_response_callback(
    &action_client->impl->cancel_client, callback, user_data);
}

/**
 * @brief 设置结果客户端回调函数
 * 
 * @param action_client 指向rcl_action_client_t结构体的指针
 * @param callback rcl_event_callback_t类型的回调函数
 * @param user_data 用户数据，将传递给回调函数
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_action_client_set_result_client_callback(
  const rcl_action_client_t * action_client, rcl_event_callback_t callback, const void * user_data)
{
  // 检查action_client是否有效
  if (!rcl_action_client_is_valid(action_client)) {
    return RCL_RET_ACTION_CLIENT_INVALID;
  }

  // 设置新响应回调函数
  return rcl_client_set_on_new_response_callback(
    &action_client->impl->result_client, callback, user_data);
}

/**
 * @brief 设置反馈订阅回调函数
 * 
 * @param action_client 指向rcl_action_client_t结构体的指针
 * @param callback rcl_event_callback_t类型的回调函数
 * @param user_data 用户数据，将传递给回调函数
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_action_client_set_feedback_subscription_callback(
  const rcl_action_client_t * action_client, rcl_event_callback_t callback, const void * user_data)
{
  // 检查action_client是否有效
  if (!rcl_action_client_is_valid(action_client)) {
    return RCL_RET_ACTION_CLIENT_INVALID;
  }

  // 设置新消息回调函数
  return rcl_subscription_set_on_new_message_callback(
    &action_client->impl->feedback_subscription, callback, user_data);
}

/**
 * @brief 设置状态订阅回调函数
 * 
 * @param action_client 指向rcl_action_client_t结构体的指针
 * @param callback rcl_event_callback_t类型的回调函数
 * @param user_data 用户数据，将传递给回调函数
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_action_client_set_status_subscription_callback(
  const rcl_action_client_t * action_client, rcl_event_callback_t callback, const void * user_data)
{
  // 检查action_client是否有效
  if (!rcl_action_client_is_valid(action_client)) {
    return RCL_RET_ACTION_CLIENT_INVALID;
  }

  // 设置新消息回调函数
  return rcl_subscription_set_on_new_message_callback(
    &action_client->impl->status_subscription, callback, user_data);
}

#ifdef __cplusplus
}
#endif
