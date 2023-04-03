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

#include "rcl_action/action_server.h"

#include "./action_server_impl.h"
#include "rcl/error_handling.h"
#include "rcl/rcl.h"
#include "rcl/time.h"
#include "rcl_action/default_qos.h"
#include "rcl_action/goal_handle.h"
#include "rcl_action/names.h"
#include "rcl_action/types.h"
#include "rcl_action/wait.h"
#include "rcutils/logging_macros.h"
#include "rcutils/strdup.h"
#include "rmw/rmw.h"

/**
 * @brief 获取一个初始化为零的rcl_action_server_t实例
 *
 * @return 返回一个初始化为零的rcl_action_server_t实例
 */
rcl_action_server_t rcl_action_get_zero_initialized_server(void)
{
  // 定义一个静态的空操作服务器并初始化为0
  static rcl_action_server_t null_action_server = {0};
  // 返回空操作服务器
  return null_action_server;
}

// 定义一个宏，用于初始化服务
#define SERVICE_INIT(Type)                                                                  \
  /* 定义一个指向Type服务名的字符指针，并初始化为NULL */                \
  char * Type##_service_name = NULL;                                                        \
  /* 获取Type服务名 */                                                                 \
  ret = rcl_action_get_##Type##_service_name(action_name, allocator, &Type##_service_name); \
  /* 检查返回值是否为RCL_RET_OK */                                                  \
  if (RCL_RET_OK != ret) {                                                                  \
    /* 根据不同的错误类型设置返回值 */                                        \
    if (RCL_RET_BAD_ALLOC == ret) {                                                         \
      ret = RCL_RET_BAD_ALLOC;                                                              \
    } else if (RCL_RET_ACTION_NAME_INVALID == ret) {                                        \
      ret = RCL_RET_ACTION_NAME_INVALID;                                                    \
    } else {                                                                                \
      ret = RCL_RET_ERROR;                                                                  \
    }                                                                                       \
    /* 跳转到fail标签 */                                                               \
    goto fail;                                                                              \
  }                                                                                         \
  /* 初始化Type服务选项 */                                                           \
  rcl_service_options_t Type##_service_options = {                                          \
    .qos = options->Type##_service_qos, .allocator = allocator};                            \
  /* 初始化Type服务 */                                                                 \
  ret = rcl_service_init(                                                                   \
    &action_server->impl->Type##_service, node, type_support->Type##_service_type_support,  \
    Type##_service_name, &Type##_service_options);                                          \
  /* 释放Type服务名内存 */                                                           \
  allocator.deallocate(Type##_service_name, allocator.state);                               \
  /* 检查返回值是否为RCL_RET_OK */                                                  \
  if (RCL_RET_OK != ret) {                                                                  \
    /* 根据不同的错误类型设置返回值 */                                        \
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

// 定义一个宏，用于初始化发布者
#define PUBLISHER_INIT(Type)                                                                 \
  /* 定义一个指向Type主题名的字符指针，并初始化为NULL */                 \
  char * Type##_topic_name = NULL;                                                           \
  /* 获取Type主题名 */                                                                  \
  ret = rcl_action_get_##Type##_topic_name(action_name, allocator, &Type##_topic_name);      \
  /* 检查返回值是否为RCL_RET_OK */                                                   \
  if (RCL_RET_OK != ret) {                                                                   \
    /* 根据不同的错误类型设置返回值 */                                         \
    if (RCL_RET_BAD_ALLOC == ret) {                                                          \
      ret = RCL_RET_BAD_ALLOC;                                                               \
    } else if (RCL_RET_ACTION_NAME_INVALID == ret) {                                         \
      ret = RCL_RET_ACTION_NAME_INVALID;                                                     \
    } else {                                                                                 \
      ret = RCL_RET_ERROR;                                                                   \
    }                                                                                        \
    /* 跳转到fail标签 */                                                                \
    goto fail;                                                                               \
  }                                                                                          \
  /* 初始化Type发布者选项 */                                                         \
  rcl_publisher_options_t Type##_publisher_options = {                                       \
    .qos = options->Type##_topic_qos, .allocator = allocator};                               \
  /* 初始化Type发布者 */                                                               \
  ret = rcl_publisher_init(                                                                  \
    &action_server->impl->Type##_publisher, node, type_support->Type##_message_type_support, \
    Type##_topic_name, &Type##_publisher_options);                                           \
  /* 释放Type主题名内存 */                                                            \
  allocator.deallocate(Type##_topic_name, allocator.state);                                  \
  /* 检查返回值是否为RCL_RET_OK */                                                   \
  if (RCL_RET_OK != ret) {                                                                   \
    /* 根据不同的错误类型设置返回值 */                                         \
    if (RCL_RET_BAD_ALLOC == ret) {                                                          \
      ret = RCL_RET_BAD_ALLOC;                                                               \
    } else if (RCL_RET_TOPIC_NAME_INVALID == ret) {                                          \
      ret = RCL_RET_ACTION_NAME_INVALID;                                                     \
    } else {                                                                                 \
      ret = RCL_RET_ERROR;                                                                   \
    }                                                                                        \
    /* 跳转到fail标签 */                                                                \
    goto fail;                                                                               \
  }

/**
 * @brief 初始化一个rcl_action_server_t实例。
 *
 * @param action_server 指向要初始化的rcl_action_server_t结构体的指针。
 * @param node 与此操作服务器关联的节点。
 * @param clock 用于时间管理的时钟。
 * @param type_support 操作类型支持。
 * @param action_name 操作名称。
 * @param options 操作服务器选项。
 * @return 成功时返回RCL_RET_OK，否则返回相应的错误代码。
 */
rcl_ret_t rcl_action_server_init(
  rcl_action_server_t * action_server, rcl_node_t * node, rcl_clock_t * clock,
  const rosidl_action_type_support_t * type_support, const char * action_name,
  const rcl_action_server_options_t * options)
{
  // 检查参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(action_server, RCL_RET_INVALID_ARGUMENT);
  if (!rcl_node_is_valid(node)) {
    return RCL_RET_NODE_INVALID;  // 错误已设置
  }
  if (!rcl_clock_valid(clock)) {
    RCL_SET_ERROR_MSG("invalid clock");
    return RCL_RET_INVALID_ARGUMENT;
  }
  RCL_CHECK_ARGUMENT_FOR_NULL(type_support, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(action_name, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(options, RCL_RET_INVALID_ARGUMENT);
  rcl_allocator_t allocator = options->allocator;
  RCL_CHECK_ALLOCATOR_WITH_MSG(&allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);

  // 初始化操作服务器日志
  RCUTILS_LOG_DEBUG_NAMED(
    ROS_PACKAGE_NAME, "Initializing action server for action name '%s'", action_name);
  if (action_server->impl) {
    RCL_SET_ERROR_MSG("action server already initialized, or memory was unintialized");
    return RCL_RET_ALREADY_INIT;
  }

  // 为操作服务器分配内存
  action_server->impl = (rcl_action_server_impl_t *)allocator.allocate(
    sizeof(rcl_action_server_impl_t), allocator.state);
  RCL_CHECK_FOR_NULL_WITH_MSG(
    action_server->impl, "allocating memory failed", return RCL_RET_BAD_ALLOC);

  // 零初始化
  action_server->impl->goal_service = rcl_get_zero_initialized_service();
  action_server->impl->cancel_service = rcl_get_zero_initialized_service();
  action_server->impl->result_service = rcl_get_zero_initialized_service();
  action_server->impl->expire_timer = rcl_get_zero_initialized_timer();
  action_server->impl->feedback_publisher = rcl_get_zero_initialized_publisher();
  action_server->impl->status_publisher = rcl_get_zero_initialized_publisher();
  action_server->impl->action_name = NULL;
  action_server->impl->options = *options;  // 复制选项
  action_server->impl->goal_handles = NULL;
  action_server->impl->num_goal_handles = 0u;
  action_server->impl->clock = NULL;

  rcl_ret_t ret = RCL_RET_OK;
  // 初始化服务
  SERVICE_INIT(goal);
  SERVICE_INIT(cancel);
  SERVICE_INIT(result);

  // 初始化发布器
  PUBLISHER_INIT(feedback);
  PUBLISHER_INIT(status);

  // 存储时钟引用
  action_server->impl->clock = clock;

  // 初始化定时器
  ret = rcl_timer_init(
    &action_server->impl->expire_timer, action_server->impl->clock, node->context,
    options->result_timeout.nanoseconds, NULL, allocator);
  if (RCL_RET_OK != ret) {
    goto fail;
  }
  // 取消定时器，以免开始触发
  ret = rcl_timer_cancel(&action_server->impl->expire_timer);
  if (RCL_RET_OK != ret) {
    goto fail;
  }

  // 复制操作名称
  action_server->impl->action_name = rcutils_strdup(action_name, allocator);
  if (NULL == action_server->impl->action_name) {
    ret = RCL_RET_BAD_ALLOC;
    goto fail;
  }
  return ret;
fail : {
  // 终止已初始化的服务/发布器并释放action_server->impl内存
  rcl_ret_t ret_throwaway = rcl_action_server_fini(action_server, node);
  // 由于已经有一个失败，因此在一个或多个操作服务器成员上终止可能会出错
  (void)ret_throwaway;
}
  return ret;
}

/**
 * @brief 销毁一个rcl_action_server_t实例。
 *
 * @param[in] action_server 要销毁的动作服务器指针。
 * @param[in] node 与动作服务器关联的节点指针。
 * @return 返回RCL_RET_OK，如果成功销毁动作服务器；否则返回相应的错误代码。
 */
rcl_ret_t rcl_action_server_fini(rcl_action_server_t * action_server, rcl_node_t * node)
{
  // 检查action_server参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(action_server, RCL_RET_ACTION_SERVER_INVALID);
  // 检查节点是否有效
  if (!rcl_node_is_valid_except_context(node)) {
    return RCL_RET_NODE_INVALID;  // error already set
  }

  rcl_ret_t ret = RCL_RET_OK;
  if (action_server->impl) {
    // 结束服务
    if (rcl_service_fini(&action_server->impl->goal_service, node) != RCL_RET_OK) {
      ret = RCL_RET_ERROR;
    }
    if (rcl_service_fini(&action_server->impl->cancel_service, node) != RCL_RET_OK) {
      ret = RCL_RET_ERROR;
    }
    if (rcl_service_fini(&action_server->impl->result_service, node) != RCL_RET_OK) {
      ret = RCL_RET_ERROR;
    }
    // 结束发布者
    if (rcl_publisher_fini(&action_server->impl->feedback_publisher, node) != RCL_RET_OK) {
      ret = RCL_RET_ERROR;
    }
    if (rcl_publisher_fini(&action_server->impl->status_publisher, node) != RCL_RET_OK) {
      ret = RCL_RET_ERROR;
    }
    // 结束定时器
    if (rcl_timer_fini(&action_server->impl->expire_timer) != RCL_RET_OK) {
      ret = RCL_RET_ERROR;
    }
    // 丢弃时钟引用
    action_server->impl->clock = NULL;
    // 释放动作名称内存
    rcl_allocator_t allocator = action_server->impl->options.allocator;
    if (action_server->impl->action_name) {
      allocator.deallocate(action_server->impl->action_name, allocator.state);
      action_server->impl->action_name = NULL;
    }
    // 释放目标句柄存储，但不结束它们。
    for (size_t i = 0; i < action_server->impl->num_goal_handles; ++i) {
      allocator.deallocate(action_server->impl->goal_handles[i], allocator.state);
    }
    allocator.deallocate(action_server->impl->goal_handles, allocator.state);
    action_server->impl->goal_handles = NULL;
    // 释放结构体内存
    allocator.deallocate(action_server->impl, allocator.state);
    action_server->impl = NULL;
  }
  return ret;
}

/**
 * @brief 获取rcl_action_server_t的默认选项。
 *
 * @return 返回一个包含默认选项的rcl_action_server_options_t实例。
 */
rcl_action_server_options_t rcl_action_server_get_default_options(void)
{
  // !!! 确保这些默认值的更改反映在头文件文档字符串中
  static rcl_action_server_options_t default_options;
  default_options.goal_service_qos = rmw_qos_profile_services_default;
  default_options.cancel_service_qos = rmw_qos_profile_services_default;
  default_options.result_service_qos = rmw_qos_profile_services_default;
  default_options.feedback_topic_qos = rmw_qos_profile_default;
  default_options.status_topic_qos = rcl_action_qos_profile_status_default;
  default_options.allocator = rcl_get_default_allocator();
  default_options.result_timeout.nanoseconds = RCUTILS_S_TO_NS(10);  // 10 seconds
  return default_options;
}

/**
 * @brief 宏定义：处理服务请求
 *
 * @param Type 服务类型
 * @param request_header 请求头
 * @param request 请求内容
 */
#define TAKE_SERVICE_REQUEST(Type, request_header, request)                                        \
  if (!rcl_action_server_is_valid(action_server)) {                                                \
    return RCL_RET_ACTION_SERVER_INVALID; /* 错误已设置 */                                    \
  }                                                                                                \
  RCL_CHECK_ARGUMENT_FOR_NULL(request_header, RCL_RET_INVALID_ARGUMENT);                           \
  RCL_CHECK_ARGUMENT_FOR_NULL(request, RCL_RET_INVALID_ARGUMENT);                                  \
  rcl_ret_t ret = rcl_take_request(&action_server->impl->Type##_service, request_header, request); \
  if (RCL_RET_OK != ret) {                                                                         \
    if (RCL_RET_BAD_ALLOC == ret) {                                                                \
      return RCL_RET_BAD_ALLOC; /* 错误已设置 */                                              \
    }                                                                                              \
    if (RCL_RET_SERVICE_TAKE_FAILED == ret) {                                                      \
      return RCL_RET_ACTION_SERVER_TAKE_FAILED;                                                    \
    }                                                                                              \
    return RCL_RET_ERROR; /* 错误已设置 */                                                    \
  }                                                                                                \
  return RCL_RET_OK;

/**
 * @brief 宏定义：发送服务响应
 *
 * @param Type 服务类型
 * @param response_header 响应头
 * @param response 响应内容
 */
#define SEND_SERVICE_RESPONSE(Type, response_header, response)                          \
  if (!rcl_action_server_is_valid(action_server)) {                                     \
    return RCL_RET_ACTION_SERVER_INVALID; /* 错误已设置 */                         \
  }                                                                                     \
  RCL_CHECK_ARGUMENT_FOR_NULL(response_header, RCL_RET_INVALID_ARGUMENT);               \
  RCL_CHECK_ARGUMENT_FOR_NULL(response, RCL_RET_INVALID_ARGUMENT);                      \
  rcl_ret_t ret =                                                                       \
    rcl_send_response(&action_server->impl->Type##_service, response_header, response); \
  if (RCL_RET_OK != ret) {                                                              \
    return RCL_RET_ERROR; /* 错误已设置 */                                         \
  }                                                                                     \
  return RCL_RET_OK;

/**
 * @brief 接收目标请求
 *
 * @param action_server 动作服务器
 * @param request_header 请求头
 * @param ros_goal_request ROS目标请求
 * @return rcl_ret_t 返回结果
 */
rcl_ret_t rcl_action_take_goal_request(
  const rcl_action_server_t * action_server, rmw_request_id_t * request_header,
  void * ros_goal_request)
{
  TAKE_SERVICE_REQUEST(goal, request_header, ros_goal_request);
}

/**
 * @brief 发送目标响应
 *
 * @param action_server 动作服务器
 * @param response_header 响应头
 * @param ros_goal_response ROS目标响应
 * @return rcl_ret_t 返回结果
 */
rcl_ret_t rcl_action_send_goal_response(
  const rcl_action_server_t * action_server, rmw_request_id_t * response_header,
  void * ros_goal_response)
{
  SEND_SERVICE_RESPONSE(goal, response_header, ros_goal_response);
}

// 仅实现部分
static int64_t _goal_info_stamp_to_nanosec(const rcl_action_goal_info_t * goal_info)
{
  assert(goal_info);
  return RCUTILS_S_TO_NS(goal_info->stamp.sec) + (int64_t)goal_info->stamp.nanosec;
}

// 仅实现部分
static void _nanosec_to_goal_info_stamp(const int64_t * nanosec, rcl_action_goal_info_t * goal_info)
{
  assert(nanosec);
  assert(goal_info);
  goal_info->stamp.sec = (int32_t)RCUTILS_NS_TO_S(*nanosec);
  goal_info->stamp.nanosec = *nanosec % RCUTILS_S_TO_NS(1);
}

/**
 * @brief 接受一个新的目标并为其分配内存空间
 *
 * @param action_server 指向rcl_action_server_t结构体的指针，用于处理动作服务器
 * @param goal_info 指向rcl_action_goal_info_t结构体的指针，包含新目标的信息
 * @return rcl_action_goal_handle_t* 返回新分配的目标句柄指针，如果失败则返回NULL
 */
rcl_action_goal_handle_t * rcl_action_accept_new_goal(
  rcl_action_server_t * action_server, const rcl_action_goal_info_t * goal_info)
{
  // 检查动作服务器是否有效
  if (!rcl_action_server_is_valid(action_server)) {
    return NULL;  // 错误已设置
  }
  // 检查goal_info参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(goal_info, NULL);

  // 检查具有相同ID的目标是否已存在
  if (rcl_action_server_goal_exists(action_server, goal_info)) {
    RCL_SET_ERROR_MSG("goal ID already exists");
    return NULL;
  }

  // 在目标句柄指针数组中分配空间
  rcl_allocator_t allocator = action_server->impl->options.allocator;
  rcl_action_goal_handle_t ** goal_handles = action_server->impl->goal_handles;
  const size_t num_goal_handles = action_server->impl->num_goal_handles;
  // TODO(jacobperron): 不要为每个接受的目标句柄分配空间，
  //                    如果需要，将分配的内存加倍。
  const size_t new_num_goal_handles = num_goal_handles + 1u;
  void * tmp_ptr = allocator.reallocate(
    goal_handles, new_num_goal_handles * sizeof(rcl_action_goal_handle_t *), allocator.state);
  if (!tmp_ptr) {
    RCL_SET_ERROR_MSG("memory allocation failed for goal handle pointer");
    return NULL;
  }
  goal_handles = (rcl_action_goal_handle_t **)tmp_ptr;

  // 为新的目标句柄分配空间
  tmp_ptr = allocator.allocate(sizeof(rcl_action_goal_handle_t), allocator.state);
  if (!tmp_ptr) {
    RCL_SET_ERROR_MSG("memory allocation failed for new goal handle");
    return NULL;
  }
  goal_handles[num_goal_handles] = (rcl_action_goal_handle_t *)tmp_ptr;

  // 使用当前时间重新设置目标信息
  rcl_action_goal_info_t goal_info_stamp_now = rcl_action_get_zero_initialized_goal_info();
  goal_info_stamp_now = *goal_info;
  rcl_time_point_value_t now_time_point;
  rcl_ret_t ret = rcl_clock_get_now(action_server->impl->clock, &now_time_point);
  if (RCL_RET_OK != ret) {
    return NULL;  // 错误已设置
  }
  _nanosec_to_goal_info_stamp(&now_time_point, &goal_info_stamp_now);

  // 创建一个新的目标句柄
  *goal_handles[num_goal_handles] = rcl_action_get_zero_initialized_goal_handle();
  ret =
    rcl_action_goal_handle_init(goal_handles[num_goal_handles], &goal_info_stamp_now, allocator);
  if (RCL_RET_OK != ret) {
    RCL_SET_ERROR_MSG("failed to initialize goal handle");
    return NULL;
  }

  action_server->impl->goal_handles = goal_handles;
  action_server->impl->num_goal_handles = new_num_goal_handles;
  return goal_handles[num_goal_handles];
}

/**
 * @brief 重新计算过期定时器的值
 *
 * @param[in] expire_timer 指向过期定时器的指针
 * @param[in] timeout 超时时间（纳秒）
 * @param[in] goal_handles 目标句柄数组
 * @param[in] num_goal_handles 目标句柄数组的大小
 * @param[in] clock 时钟对象指针
 * @return rcl_ret_t 返回操作结果
 */
static rcl_ret_t _recalculate_expire_timer(
  rcl_timer_t * expire_timer, const int64_t timeout, rcl_action_goal_handle_t ** goal_handles,
  size_t num_goal_handles, rcl_clock_t * clock)
{
  // 初始化非活动目标数量为0
  size_t num_inactive_goals = 0u;
  // 设置最小周期为超时时间
  int64_t minimum_period = timeout;

  // 获取当前时间（纳秒）
  int64_t current_time;
  rcl_ret_t ret = rcl_clock_get_now(clock, &current_time);
  if (RCL_RET_OK != ret) {
    return RCL_RET_ERROR;
  }

  // 遍历目标句柄数组
  for (size_t i = 0; i < num_goal_handles; ++i) {
    rcl_action_goal_handle_t * goal_handle = goal_handles[i];
    // 如果目标句柄不是活动状态
    if (!rcl_action_goal_handle_is_active(goal_handle)) {
      // 增加非活动目标数量
      ++num_inactive_goals;

      // 获取目标信息
      rcl_action_goal_info_t goal_info;
      ret = rcl_action_goal_handle_get_info(goal_handle, &goal_info);
      if (RCL_RET_OK != ret) {
        return RCL_RET_ERROR;
      }

      // 计算时间差
      int64_t delta = timeout - (current_time - _goal_info_stamp_to_nanosec(&goal_info));
      // 更新最小周期
      if (delta < minimum_period) {
        minimum_period = delta;
      }
    }
  }

  // 如果没有目标句柄或者没有非活动目标
  if (0u == num_goal_handles || 0u == num_inactive_goals) {
    // 取消定时器
    return rcl_timer_cancel(expire_timer);
  } else {
    // 如果最小周期为负数，说明时间向后跳跃
    if (minimum_period < 0) {
      minimum_period = 0;
    }
    // 重置定时器
    ret = rcl_timer_reset(expire_timer);
    if (RCL_RET_OK != ret) {
      return ret;
    }
    // 设置定时器在下一个目标过期时触发
    int64_t old_period;
    ret = rcl_timer_exchange_period(expire_timer, minimum_period, &old_period);
    if (RCL_RET_OK != ret) {
      return ret;
    }
  }
  return RCL_RET_OK;
}

/**
 * @brief 发布反馈信息
 *
 * @param[in] action_server 动作服务器对象指针
 * @param[in] ros_feedback 反馈信息指针
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_action_publish_feedback(
  const rcl_action_server_t * action_server, void * ros_feedback)
{
  // 检查动作服务器是否有效
  if (!rcl_action_server_is_valid(action_server)) {
    return RCL_RET_ACTION_SERVER_INVALID;  // error already set
  }
  // 检查反馈信息参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(ros_feedback, RCL_RET_INVALID_ARGUMENT);

  // 发布反馈信息
  rcl_ret_t ret = rcl_publish(&action_server->impl->feedback_publisher, ros_feedback, NULL);
  if (RCL_RET_OK != ret) {
    return RCL_RET_ERROR;  // error already set
  }
  return RCL_RET_OK;
}

/**
 * @brief 获取目标状态数组
 *
 * @param[in] action_server 指向有效的rcl_action_server_t结构体的指针
 * @param[out] status_message 用于存储目标状态数组的指针
 * @return rcl_ret_t 返回操作结果，成功返回RCL_RET_OK，否则返回相应的错误代码
 *
 * 获取给定动作服务器的目标状态数组。
 */
rcl_ret_t rcl_action_get_goal_status_array(
  const rcl_action_server_t * action_server, rcl_action_goal_status_array_t * status_message)
{
  // 检查动作服务器是否有效
  if (!rcl_action_server_is_valid(action_server)) {
    return RCL_RET_ACTION_SERVER_INVALID;  // 错误已设置
  }
  // 检查status_message参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(status_message, RCL_RET_INVALID_ARGUMENT);

  // 如果目标数量为零，则无需分配任何空间
  size_t num_goals = action_server->impl->num_goal_handles;
  if (0u == num_goals) {
    status_message->msg.status_list.size = 0u;
    return RCL_RET_OK;
  }

  // 分配状态数组
  rcl_allocator_t allocator = action_server->impl->options.allocator;
  rcl_ret_t ret = rcl_action_goal_status_array_init(status_message, num_goals, allocator);
  if (RCL_RET_OK != ret) {
    if (RCL_RET_BAD_ALLOC == ret) {
      return RCL_RET_BAD_ALLOC;
    }
    return RCL_RET_ERROR;
  }

  // 填充状态数组
  for (size_t i = 0u; i < num_goals; ++i) {
    ret = rcl_action_goal_handle_get_info(
      action_server->impl->goal_handles[i], &status_message->msg.status_list.data[i].goal_info);
    if (RCL_RET_OK != ret) {
      ret = RCL_RET_ERROR;
      goto fail;
    }
    ret = rcl_action_goal_handle_get_status(
      action_server->impl->goal_handles[i], &status_message->msg.status_list.data[i].status);
    if (RCL_RET_OK != ret) {
      ret = RCL_RET_ERROR;
      goto fail;
    }
  }
  return RCL_RET_OK;
fail : {
  rcl_ret_t ret_throwaway = rcl_action_goal_status_array_fini(status_message);
  (void)ret_throwaway;
  return ret;
}
}

/**
 * @brief 发布状态
 *
 * @param[in] action_server 指向有效的rcl_action_server_t结构体的指针
 * @param[in] status_message 要发布的状态消息的指针
 * @return rcl_ret_t 返回操作结果，成功返回RCL_RET_OK，否则返回相应的错误代码
 *
 * 发布给定动作服务器的状态消息。
 */
rcl_ret_t rcl_action_publish_status(
  const rcl_action_server_t * action_server, const void * status_message)
{
  // 检查动作服务器是否有效
  if (!rcl_action_server_is_valid(action_server)) {
    return RCL_RET_ACTION_SERVER_INVALID;  // 错误已设置
  }
  // 检查status_message参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(status_message, RCL_RET_INVALID_ARGUMENT);

  // 发布状态消息
  rcl_ret_t ret = rcl_publish(&action_server->impl->status_publisher, status_message, NULL);
  if (RCL_RET_OK != ret) {
    return RCL_RET_ERROR;  // 错误已设置
  }
  return RCL_RET_OK;
}

/**
 * @brief 接收结果请求
 *
 * @param[in] action_server 指向有效的rcl_action_server_t结构体的指针
 * @param[out] request_header 请求头的指针
 * @param[out] ros_result_request 存储结果请求的指针
 * @return rcl_ret_t 返回操作结果，成功返回RCL_RET_OK，否则返回相应的错误代码
 *
 * 从给定动作服务器接收结果请求。
 */
rcl_ret_t rcl_action_take_result_request(
  const rcl_action_server_t * action_server, rmw_request_id_t * request_header,
  void * ros_result_request)
{
  TAKE_SERVICE_REQUEST(result, request_header, ros_result_request);
}

/**
 * @brief 发送结果响应给客户端。
 *
 * @param[in] action_server 指向动作服务器的指针。
 * @param[in] response_header 指向响应头的指针。
 * @param[in] ros_result_response 指向结果响应的指针。
 * @return 返回 rcl_ret_t 类型的结果状态。
 */
rcl_ret_t rcl_action_send_result_response(
  const rcl_action_server_t * action_server, rmw_request_id_t * response_header,
  void * ros_result_response)
{
  // 调用宏发送服务响应
  SEND_SERVICE_RESPONSE(result, response_header, ros_result_response);
}

/**
 * @brief 过期目标处理函数。
 *
 * @param[in] action_server 指向动作服务器的指针。
 * @param[out] expired_goals 存储过期目标信息的数组。
 * @param[in] expired_goals_capacity 过期目标数组的容量。
 * @param[out] num_expired 存储过期目标数量的指针。
 * @return 返回 rcl_ret_t 类型的结果状态。
 */
rcl_ret_t rcl_action_expire_goals(
  const rcl_action_server_t * action_server, rcl_action_goal_info_t * expired_goals,
  size_t expired_goals_capacity, size_t * num_expired)
{
  // 检查动作服务器是否有效
  if (!rcl_action_server_is_valid(action_server)) {
    return RCL_RET_ACTION_SERVER_INVALID;
  }
  // 判断输出参数是否有效
  const bool output_expired =
    NULL != expired_goals && NULL != num_expired && expired_goals_capacity > 0u;
  if (
    !output_expired &&
    (NULL != expired_goals || NULL != num_expired || expired_goals_capacity != 0u)) {
    RCL_SET_ERROR_MSG("expired_goals, expired_goals_capacity, and num_expired inconsistent");
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 获取当前时间（纳秒）
  int64_t current_time;
  rcl_ret_t ret = rcl_clock_get_now(action_server->impl->clock, &current_time);
  if (RCL_RET_OK != ret) {
    return RCL_RET_ERROR;
  }

  // 用于缩小目标句柄数组的分配器
  rcl_allocator_t allocator = action_server->impl->options.allocator;

  size_t num_goals_expired = 0u;
  rcl_ret_t ret_final = RCL_RET_OK;
  const int64_t timeout = (int64_t)action_server->impl->options.result_timeout.nanoseconds;
  rcl_action_goal_handle_t * goal_handle;
  rcl_action_goal_info_t goal_info;
  int64_t goal_time;
  size_t num_goal_handles = action_server->impl->num_goal_handles;
  for (size_t i = 0u; i < num_goal_handles; ++i) {
    // 如果输出过期并且过期目标数量大于等于过期目标容量，则停止处理过期目标
    if (output_expired && num_goals_expired >= expired_goals_capacity) {
      break;
    }
    goal_handle = action_server->impl->goal_handles[i];
    // 过期仅适用于已终止的目标
    if (rcl_action_goal_handle_is_active(goal_handle)) {
      continue;
    }
    rcl_action_goal_info_t * info_ptr = &goal_info;
    if (output_expired) {
      info_ptr = &(expired_goals[num_goals_expired]);
    }
    ret = rcl_action_goal_handle_get_info(goal_handle, info_ptr);
    if (RCL_RET_OK != ret) {
      ret_final = RCL_RET_ERROR;
      continue;
    }
    goal_time = _goal_info_stamp_to_nanosec(info_ptr);
    if ((current_time - goal_time) > timeout) {
      // 释放用于存储指向目标句柄的指针的空间
      allocator.deallocate(action_server->impl->goal_handles[i], allocator.state);
      action_server->impl->goal_handles[i] = NULL;
      // 将后面的所有指针向后移动一个位置以填补空缺
      for (size_t post_i = i; (post_i + 1) < num_goal_handles; ++post_i) {
        action_server->impl->goal_handles[post_i] = action_server->impl->goal_handles[post_i + 1];
      }
      // 减少 i 以便再次检查具有新目标句柄的相同索引
      --i;
      --num_goal_handles;
      ++num_goals_expired;
    }
  }

  if (num_goals_expired > 0u) {
    // 如果有一些目标过期，则缩小目标句柄数组
    if (0u == num_goal_handles) {
      allocator.deallocate(action_server->impl->goal_handles, allocator.state);
      action_server->impl->goal_handles = NULL;
      action_server->impl->num_goal_handles = num_goal_handles;
    } else {
      void * tmp_ptr = allocator.reallocate(
        action_server->impl->goal_handles, num_goal_handles * sizeof(rcl_action_goal_handle_t *),
        allocator.state);
      if (!tmp_ptr) {
        RCL_SET_ERROR_MSG("failed to shrink size of goal handle array");
        ret_final = RCL_RET_ERROR;
      } else {
        action_server->impl->goal_handles = (rcl_action_goal_handle_t **)tmp_ptr;
        action_server->impl->num_goal_handles = num_goal_handles;
      }
    }
  }
  // 重新计算过期定时器
  rcl_ret_t expire_timer_ret = _recalculate_expire_timer(
    &action_server->impl->expire_timer, action_server->impl->options.result_timeout.nanoseconds,
    action_server->impl->goal_handles, action_server->impl->num_goal_handles,
    action_server->impl->clock);

  if (RCL_RET_OK != expire_timer_ret) {
    ret_final = expire_timer_ret;
  }

  // 如果参数不为空，则设置它
  if (NULL != num_expired) {
    (*num_expired) = num_goals_expired;
  }
  return ret_final;
}

/**
 * @brief 通知 action server 目标已完成
 *
 * @param[in] action_server 指向 rcl_action_server_t 结构体的指针
 * @return 返回 rcl_ret_t 类型的结果，表示操作是否成功
 */
rcl_ret_t rcl_action_notify_goal_done(const rcl_action_server_t * action_server)
{
  // 检查 action server 是否有效
  if (!rcl_action_server_is_valid(action_server)) {
    return RCL_RET_ACTION_SERVER_INVALID;
  }
  // 重新计算过期定时器
  return _recalculate_expire_timer(
    &action_server->impl->expire_timer, action_server->impl->options.result_timeout.nanoseconds,
    action_server->impl->goal_handles, action_server->impl->num_goal_handles,
    action_server->impl->clock);
}

/**
 * @brief 接收取消请求
 *
 * @param[in] action_server 指向 rcl_action_server_t 结构体的指针
 * @param[out] request_header 请求头
 * @param[out] ros_cancel_request 取消请求
 * @return 返回 rcl_ret_t 类型的结果，表示操作是否成功
 */
rcl_ret_t rcl_action_take_cancel_request(
  const rcl_action_server_t * action_server, rmw_request_id_t * request_header,
  void * ros_cancel_request)
{
  TAKE_SERVICE_REQUEST(cancel, request_header, ros_cancel_request);
}

/**
 * @brief 处理取消请求
 *
 * @param[in] action_server 指向 rcl_action_server_t 结构体的指针
 * @param[in] cancel_request 指向 rcl_action_cancel_request_t 结构体的指针，表示取消请求
 * @param[out] cancel_response 指向 rcl_action_cancel_response_t 结构体的指针，表示取消响应
 * @return 返回 rcl_ret_t 类型的结果，表示操作是否成功
 */
rcl_ret_t rcl_action_process_cancel_request(
  const rcl_action_server_t * action_server, const rcl_action_cancel_request_t * cancel_request,
  rcl_action_cancel_response_t * cancel_response)
{
  // 检查 action server 是否有效
  if (!rcl_action_server_is_valid(action_server)) {
    return RCL_RET_ACTION_SERVER_INVALID;  // error already set
  }
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(cancel_request, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(cancel_response, RCL_RET_INVALID_ARGUMENT);

  // 获取分配器
  rcl_allocator_t allocator = action_server->impl->options.allocator;
  const size_t total_num_goals = action_server->impl->num_goal_handles;

  // 存储将要转换为取消状态的活动目标句柄的指针
  rcl_action_goal_handle_t ** goal_handles_to_cancel =
    (rcl_action_goal_handle_t **)allocator.allocate(
      sizeof(rcl_action_goal_handle_t *) * total_num_goals, allocator.state);
  if (!goal_handles_to_cancel) {
    RCL_SET_ERROR_MSG("allocation failed for temporary goal handle array");
    return RCL_RET_BAD_ALLOC;
  }
  size_t num_goals_to_cancel = 0u;

  // 请求数据
  const rcl_action_goal_info_t * request_goal_info = &cancel_request->goal_info;
  const uint8_t * request_uuid = request_goal_info->goal_id.uuid;
  int64_t request_nanosec = _goal_info_stamp_to_nanosec(request_goal_info);

  rcl_ret_t ret_final = RCL_RET_OK;
  // 确定应该转换为取消状态的目标数量
  if (!uuidcmpzero(request_uuid) && (0u == request_nanosec)) {
    // UUID 不为零且时间戳为零；取消一个目标（如果存在）
    rcl_action_goal_info_t goal_info = rcl_action_get_zero_initialized_goal_info();
    cancel_response->msg.return_code = action_msgs__srv__CancelGoal_Response__ERROR_UNKNOWN_GOAL_ID;
    rcl_action_goal_handle_t * goal_handle;
    for (size_t i = 0u; i < total_num_goals; ++i) {
      goal_handle = action_server->impl->goal_handles[i];
      rcl_ret_t ret = rcl_action_goal_handle_get_info(goal_handle, &goal_info);
      if (RCL_RET_OK != ret) {
        ret_final = RCL_RET_ERROR;
        continue;
      }

      if (uuidcmp(request_uuid, goal_info.goal_id.uuid)) {
        if (rcl_action_goal_handle_is_cancelable(goal_handle)) {
          goal_handles_to_cancel[num_goals_to_cancel++] = goal_handle;
          cancel_response->msg.return_code = action_msgs__srv__CancelGoal_Response__ERROR_NONE;
        } else {
          // 如果目标不可取消，则必须是因为它处于终止状态
          cancel_response->msg.return_code =
            action_msgs__srv__CancelGoal_Response__ERROR_GOAL_TERMINATED;
        }
        break;
      }
    }
  } else {
    // 请求者可以稍后更新响应代码以拒绝请求（如果需要）
    cancel_response->msg.return_code = action_msgs__srv__CancelGoal_Response__ERROR_NONE;
    if (uuidcmpzero(request_uuid) && (0u == request_nanosec)) {
      // UUID 和时间戳都为零；取消所有目标
      // 将时间戳设置为最大值，以便在以下 for 循环中取消所有活动目标
      request_nanosec = INT64_MAX;
    }

    // 取消在时间戳之前或等于时间戳的所有活动目标
    // 还要取消与取消请求中的 UUID 匹配的任何目标
    rcl_action_goal_info_t goal_info = rcl_action_get_zero_initialized_goal_info();
    rcl_action_goal_handle_t * goal_handle;
    for (size_t i = 0u; i < total_num_goals; ++i) {
      goal_handle = action_server->impl->goal_handles[i];
      rcl_ret_t ret = rcl_action_goal_handle_get_info(goal_handle, &goal_info);
      if (RCL_RET_OK != ret) {
        ret_final = RCL_RET_ERROR;
        continue;
      }

      const int64_t goal_nanosec = _goal_info_stamp_to_nanosec(&goal_info);
      if (
        rcl_action_goal_handle_is_cancelable(goal_handle) &&
        ((goal_nanosec <= request_nanosec) || uuidcmp(request_uuid, goal_info.goal_id.uuid))) {
        goal_handles_to_cancel[num_goals_to_cancel++] = goal_handle;
      }
    }
  }

  if (0u == num_goals_to_cancel) {
    cancel_response->msg.goals_canceling.data = NULL;
    cancel_response->msg.goals_canceling.size = 0u;
    goto cleanup;
  }

  // 在响应中分配空间
  rcl_ret_t ret = rcl_action_cancel_response_init(cancel_response, num_goals_to_cancel, allocator);
  if (RCL_RET_OK != ret) {
    if (RCL_RET_BAD_ALLOC == ret) {
      ret_final = RCL_RET_BAD_ALLOC;  // error already set
    } else {
      ret_final = RCL_RET_ERROR;  // error already set
    }
    goto cleanup;
  }

  // 将目标信息添加到输出结构中
  rcl_action_goal_handle_t * goal_handle;
  for (size_t i = 0u; i < num_goals_to_cancel; ++i) {
    goal_handle = goal_handles_to_cancel[i];
    ret =
      rcl_action_goal_handle_get_info(goal_handle, &cancel_response->msg.goals_canceling.data[i]);
    if (RCL_RET_OK != ret) {
      ret_final = RCL_RET_ERROR;  // error already set
    }
  }
cleanup:
  allocator.deallocate(goal_handles_to_cancel, allocator.state);
  return ret_final;
}

/**
 * @brief 发送取消响应
 *
 * @param[in] action_server 动作服务器指针
 * @param[in] response_header 响应头指针
 * @param[in] ros_cancel_response ROS取消响应指针
 * @return rcl_ret_t 返回RCL操作状态
 */
rcl_ret_t rcl_action_send_cancel_response(
  const rcl_action_server_t * action_server, rmw_request_id_t * response_header,
  void * ros_cancel_response)
{
  // 发送服务响应
  SEND_SERVICE_RESPONSE(cancel, response_header, ros_cancel_response);
}

/**
 * @brief 获取动作服务器的动作名称
 *
 * @param[in] action_server 动作服务器指针
 * @return const char* 返回动作名称字符串
 */
const char * rcl_action_server_get_action_name(const rcl_action_server_t * action_server)
{
  if (!rcl_action_server_is_valid(action_server)) {
    return NULL;  // 错误已设置
  }
  return action_server->impl->action_name;
}

/**
 * @brief 获取动作服务器的选项
 *
 * @param[in] action_server 动作服务器指针
 * @return const rcl_action_server_options_t* 返回动作服务器选项指针
 */
const rcl_action_server_options_t * rcl_action_server_get_options(
  const rcl_action_server_t * action_server)
{
  if (!rcl_action_server_is_valid(action_server)) {
    return NULL;  // 错误已设置
  }
  return &action_server->impl->options;
}

/**
 * @brief 获取目标句柄
 *
 * @param[in] action_server 动作服务器指针
 * @param[out] goal_handles 目标句柄指针的指针
 * @param[out] num_goals 目标数量指针
 * @return rcl_ret_t 返回RCL操作状态
 */
rcl_ret_t rcl_action_server_get_goal_handles(
  const rcl_action_server_t * action_server, rcl_action_goal_handle_t *** goal_handles,
  size_t * num_goals)
{
  if (!rcl_action_server_is_valid(action_server)) {
    return RCL_RET_ACTION_SERVER_INVALID;  // 错误已设置
  }
  RCL_CHECK_ARGUMENT_FOR_NULL(goal_handles, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(num_goals, RCL_RET_INVALID_ARGUMENT);
  *goal_handles = action_server->impl->goal_handles;
  *num_goals = action_server->impl->num_goal_handles;
  return RCL_RET_OK;
}

/**
 * @brief 检查动作服务器中目标是否存在
 *
 * @param[in] action_server 动作服务器指针
 * @param[in] goal_info 目标信息指针
 * @return bool 如果目标存在则返回true，否则返回false
 */
bool rcl_action_server_goal_exists(
  const rcl_action_server_t * action_server, const rcl_action_goal_info_t * goal_info)
{
  if (!rcl_action_server_is_valid(action_server)) {
    return false;  // 错误已设置
  }
  RCL_CHECK_ARGUMENT_FOR_NULL(goal_info, false);

  rcl_action_goal_info_t gh_goal_info = rcl_action_get_zero_initialized_goal_info();
  rcl_ret_t ret;
  for (size_t i = 0u; i < action_server->impl->num_goal_handles; ++i) {
    ret = rcl_action_goal_handle_get_info(action_server->impl->goal_handles[i], &gh_goal_info);
    if (RCL_RET_OK != ret) {
      RCL_SET_ERROR_MSG("failed to get info for goal handle");
      return false;
    }
    // 比较UUIDs
    if (uuidcmp(gh_goal_info.goal_id.uuid, goal_info->goal_id.uuid)) {
      return true;
    }
  }
  return false;
}

/**
 * @brief 检查给定的rcl_action_server_t是否有效
 *
 * @param action_server 要检查的rcl_action_server_t指针
 * @return 如果有效则返回true，否则返回false
 */
bool rcl_action_server_is_valid(const rcl_action_server_t * action_server)
{
  // 检查action_server指针是否为空，如果为空则返回错误信息并返回false
  RCL_CHECK_FOR_NULL_WITH_MSG(action_server, "action server pointer is invalid", return false);
  // 检查action_server的实现是否为空，如果为空则返回错误信息并返回false
  RCL_CHECK_FOR_NULL_WITH_MSG(
    action_server->impl, "action server implementation is invalid", return false);
  // 检查goal_service是否有效，如果无效则重置错误并设置错误消息，然后返回false
  if (!rcl_service_is_valid(&action_server->impl->goal_service)) {
    rcl_reset_error();
    RCL_SET_ERROR_MSG("goal service is invalid");
    return false;
  }
  // 检查cancel_service是否有效，如果无效则重置错误并设置错误消息，然后返回false
  if (!rcl_service_is_valid(&action_server->impl->cancel_service)) {
    rcl_reset_error();
    RCL_SET_ERROR_MSG("cancel service is invalid");
    return false;
  }
  // 检查result_service是否有效，如果无效则重置错误并设置错误消息，然后返回false
  if (!rcl_service_is_valid(&action_server->impl->result_service)) {
    rcl_reset_error();
    RCL_SET_ERROR_MSG("result service is invalid");
    return false;
  }
  // 检查feedback_publisher是否有效，如果无效则重置错误并设置错误消息，然后返回false
  if (!rcl_publisher_is_valid(&action_server->impl->feedback_publisher)) {
    rcl_reset_error();
    RCL_SET_ERROR_MSG("feedback publisher is invalid");
    return false;
  }
  // 检查status_publisher是否有效，如果无效则重置错误并设置错误消息，然后返回false
  if (!rcl_publisher_is_valid(&action_server->impl->status_publisher)) {
    rcl_reset_error();
    RCL_SET_ERROR_MSG("status publisher is invalid");
    return false;
  }
  // 如果所有检查都通过，则返回true
  return true;
}

/**
 * @brief 检查给定的rcl_action_server_t是否有效（不包括上下文）
 *
 * @param action_server 要检查的rcl_action_server_t指针
 * @return 如果有效则返回true，否则返回false
 */
bool rcl_action_server_is_valid_except_context(const rcl_action_server_t * action_server)
{
  // 检查action_server指针是否为空，如果为空则返回错误信息并返回false
  RCL_CHECK_FOR_NULL_WITH_MSG(action_server, "action server pointer is invalid", return false);
  // 检查action_server的实现是否为空，如果为空则返回错误信息并返回false
  RCL_CHECK_FOR_NULL_WITH_MSG(
    action_server->impl, "action server implementation is invalid", return false);
  // 检查goal_service是否有效，如果无效则设置错误消息并返回false
  if (!rcl_service_is_valid(&action_server->impl->goal_service)) {
    RCL_SET_ERROR_MSG("goal service is invalid");
    return false;
  }
  // 检查cancel_service是否有效，如果无效则设置错误消息并返回false
  if (!rcl_service_is_valid(&action_server->impl->cancel_service)) {
    RCL_SET_ERROR_MSG("cancel service is invalid");
    return false;
  }
  // 检查result_service是否有效，如果无效则设置错误消息并返回false
  if (!rcl_service_is_valid(&action_server->impl->result_service)) {
    RCL_SET_ERROR_MSG("result service is invalid");
    return false;
  }
  // 检查feedback_publisher是否有效（不包括上下文），如果无效则设置错误消息并返回false
  if (!rcl_publisher_is_valid_except_context(&action_server->impl->feedback_publisher)) {
    RCL_SET_ERROR_MSG("feedback publisher is invalid");
    return false;
  }
  // 检查status_publisher是否有效（不包括上下文），如果无效则设置错误消息并返回false
  if (!rcl_publisher_is_valid_except_context(&action_server->impl->status_publisher)) {
    RCL_SET_ERROR_MSG("status publisher is invalid");
    return false;
  }
  // 如果所有检查都通过，则返回true
  return true;
}

/**
 * @brief 将rcl_action_server_t添加到rcl_wait_set_t中
 *
 * @param wait_set 要添加到的rcl_wait_set_t指针
 * @param action_server 要添加的rcl_action_server_t指针
 * @param[out] service_index 返回添加的服务索引
 * @return 成功时返回RCL_RET_OK，否则返回相应的错误代码
 */
rcl_ret_t rcl_action_wait_set_add_action_server(
  rcl_wait_set_t * wait_set, const rcl_action_server_t * action_server, size_t * service_index)
{
  // 检查wait_set参数是否为空，如果为空则返回错误代码
  RCL_CHECK_ARGUMENT_FOR_NULL(wait_set, RCL_RET_WAIT_SET_INVALID);
  // 检查action_server是否有效（不包括上下文），如果无效则返回错误代码
  if (!rcl_action_server_is_valid_except_context(action_server)) {
    return RCL_RET_ACTION_SERVER_INVALID;  // error already set
  }

  // 将goal_service添加到wait_set中，并获取索引
  rcl_ret_t ret = rcl_wait_set_add_service(
    wait_set, &action_server->impl->goal_service,
    &action_server->impl->wait_set_goal_service_index);
  // 如果添加失败，则返回错误代码
  if (RCL_RET_OK != ret) {
    return ret;
  }
  // 将cancel_service添加到wait_set中，并获取索引
  ret = rcl_wait_set_add_service(
    wait_set, &action_server->impl->cancel_service,
    &action_server->impl->wait_set_cancel_service_index);
  // 如果添加失败，则返回错误代码
  if (RCL_RET_OK != ret) {
    return ret;
  }
  // 将result_service添加到wait_set中，并获取索引
  ret = rcl_wait_set_add_service(
    wait_set, &action_server->impl->result_service,
    &action_server->impl->wait_set_result_service_index);
  // 如果添加失败，则返回错误代码
  if (RCL_RET_OK != ret) {
    return ret;
  }
  // 将expire_timer添加到wait_set中，并获取索引
  ret = rcl_wait_set_add_timer(
    wait_set, &action_server->impl->expire_timer,
    &action_server->impl->wait_set_expire_timer_index);
  // 如果添加失败，则返回错误代码
  if (RCL_RET_OK != ret) {
    return ret;
  }

  // 如果service_index不为空，则设置goal_service的索引
  if (NULL != service_index) {
    // The goal service was the first added
    *service_index = action_server->impl->wait_set_goal_service_index;
  }
  // 返回成功代码
  return RCL_RET_OK;
}

/**
 * @brief 获取rcl_action_server_t在wait_set中的实体数量
 *
 * @param action_server 要查询的rcl_action_server_t指针
 * @param[out] num_subscriptions 返回订阅数量
 * @param[out] num_guard_conditions 返回守护条件数量
 * @param[out] num_timers 返回定时器数量
 * @param[out] num_clients 返回客户端数量
 * @param[out] num_services 返回服务数量
 * @return 成功时返回RCL_RET_OK，否则返回相应的错误代码
 */
rcl_ret_t rcl_action_server_wait_set_get_num_entities(
  const rcl_action_server_t * action_server, size_t * num_subscriptions,
  size_t * num_guard_conditions, size_t * num_timers, size_t * num_clients, size_t * num_services)
{
  // 检查action_server是否有效（不包括上下文），如果无效则返回错误代码
  if (!rcl_action_server_is_valid_except_context(action_server)) {
    return RCL_RET_ACTION_SERVER_INVALID;  // error already set
  }
  // 检查输出参数是否为空，如果为空则返回错误代码
  RCL_CHECK_ARGUMENT_FOR_NULL(num_subscriptions, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(num_guard_conditions, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(num_timers, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(num_clients, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(num_services, RCL_RET_INVALID_ARGUMENT);
  // 设置实体数量
  *num_subscriptions = 0u;
  *num_guard_conditions = 0u;
  *num_timers = 1u;
  *num_clients = 0u;
  *num_services = 3u;
  // 返回成功代码
  return RCL_RET_OK;
}

/**
 * @brief 获取实体是否准备好的状态
 * 
 * @param[in] wait_set 等待集
 * @param[in] action_server 动作服务器
 * @param[out] is_goal_request_ready 目标请求是否准备好
 * @param[out] is_cancel_request_ready 取消请求是否准备好
 * @param[out] is_result_request_ready 结果请求是否准备好
 * @param[out] is_goal_expired 目标是否过期
 * @return rcl_ret_t 返回操作结果
 */
rcl_ret_t rcl_action_server_wait_set_get_entities_ready(
  const rcl_wait_set_t * wait_set, const rcl_action_server_t * action_server,
  bool * is_goal_request_ready, bool * is_cancel_request_ready, bool * is_result_request_ready,
  bool * is_goal_expired)
{
  // 检查wait_set是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(wait_set, RCL_RET_WAIT_SET_INVALID);
  // 检查action_server是否有效
  if (!rcl_action_server_is_valid_except_context(action_server)) {
    return RCL_RET_ACTION_SERVER_INVALID;  // error already set
  }
  // 检查is_goal_request_ready是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(is_goal_request_ready, RCL_RET_INVALID_ARGUMENT);
  // 检查is_cancel_request_ready是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(is_cancel_request_ready, RCL_RET_INVALID_ARGUMENT);
  // 检查is_result_request_ready是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(is_result_request_ready, RCL_RET_INVALID_ARGUMENT);
  // 检查is_goal_expired是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(is_goal_expired, RCL_RET_INVALID_ARGUMENT);

  // 获取实现细节
  const rcl_action_server_impl_t * impl = action_server->impl;
  // 获取目标服务
  const rcl_service_t * goal_service = wait_set->services[impl->wait_set_goal_service_index];
  // 获取取消服务
  const rcl_service_t * cancel_service = wait_set->services[impl->wait_set_cancel_service_index];
  // 获取结果服务
  const rcl_service_t * result_service = wait_set->services[impl->wait_set_result_service_index];
  // 获取过期计时器
  const rcl_timer_t * expire_timer = wait_set->timers[impl->wait_set_expire_timer_index];
  // 设置目标请求准备状态
  *is_goal_request_ready = (&impl->goal_service == goal_service);
  // 设置取消请求准备状态
  *is_cancel_request_ready = (&impl->cancel_service == cancel_service);
  // 设置结果请求准备状态
  *is_result_request_ready = (&impl->result_service == result_service);
  // 设置目标过期状态
  *is_goal_expired = (&impl->expire_timer == expire_timer);
  // 返回操作成功
  return RCL_RET_OK;
}

/**
 * @brief 设置目标服务回调函数
 * 
 * @param[in] action_server 动作服务器
 * @param[in] callback 回调函数
 * @param[in] user_data 用户数据
 * @return rcl_ret_t 返回操作结果
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_server_set_goal_service_callback(
  const rcl_action_server_t * action_server, rcl_event_callback_t callback, const void * user_data)
{
  // 检查action_server是否有效
  if (!rcl_action_server_is_valid_except_context(action_server)) {
    return RCL_RET_ACTION_SERVER_INVALID;
  }

  // 设置新请求回调函数
  return rcl_service_set_on_new_request_callback(
    &action_server->impl->goal_service, callback, user_data);
}

/**
 * @brief 设置结果服务回调函数
 * 
 * @param[in] action_server 动作服务器
 * @param[in] callback 回调函数
 * @param[in] user_data 用户数据
 * @return rcl_ret_t 返回操作结果
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_server_set_result_service_callback(
  const rcl_action_server_t * action_server, rcl_event_callback_t callback, const void * user_data)
{
  // 检查action_server是否有效
  if (!rcl_action_server_is_valid_except_context(action_server)) {
    return RCL_RET_ACTION_SERVER_INVALID;
  }

  // 设置新请求回调函数
  return rcl_service_set_on_new_request_callback(
    &action_server->impl->result_service, callback, user_data);
}

/**
 * @brief 设置取消服务回调函数
 * 
 * @param[in] action_server 动作服务器
 * @param[in] callback 回调函数
 * @param[in] user_data 用户数据
 * @return rcl_ret_t 返回操作结果
 */
RCL_ACTION_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_action_server_set_cancel_service_callback(
  const rcl_action_server_t * action_server, rcl_event_callback_t callback, const void * user_data)
{
  // 检查action_server是否有效
  if (!rcl_action_server_is_valid_except_context(action_server)) {
    return RCL_RET_ACTION_SERVER_INVALID;
  }

  // 设置新请求回调函数
  return rcl_service_set_on_new_request_callback(
    &action_server->impl->cancel_service, callback, user_data);
}

#ifdef __cplusplus
}
#endif
