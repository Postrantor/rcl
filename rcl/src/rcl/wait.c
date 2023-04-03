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

#include "rcl/wait.h"

#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

#include "./context_impl.h"
#include "rcl/error_handling.h"
#include "rcl/time.h"
#include "rcutils/logging_macros.h"
#include "rmw/error_handling.h"
#include "rmw/event.h"
#include "rmw/rmw.h"

/**
 * @struct rcl_wait_set_impl_s
 * @brief ROS2中rcl相关的结构体，用于存储等待集合的实现细节。
 */
struct rcl_wait_set_impl_s {
  // 已添加到等待集合的订阅数量
  size_t subscription_index;
  rmw_subscriptions_t rmw_subscriptions;
  // 已添加到等待集合的守护条件数量
  size_t guard_condition_index;
  rmw_guard_conditions_t rmw_guard_conditions;
  // 已添加到等待集合的客户端数量
  size_t client_index;
  rmw_clients_t rmw_clients;
  // 已添加到等待集合的服务数量
  size_t service_index;
  rmw_services_t rmw_services;
  // 已添加到等待集合的事件数量
  size_t event_index;
  rmw_events_t rmw_events;

  rmw_wait_set_t *rmw_wait_set;
  // 已添加到等待集合的定时器数量
  size_t timer_index;
  // 与等待集合关联的上下文
  rcl_context_t *context;
  // 等待集合中使用的分配器
  rcl_allocator_t allocator;
};

/**
 * @brief 获取一个零初始化的等待集合。
 *
 * @return 返回一个零初始化的rcl_wait_set_t结构体。
 */
rcl_wait_set_t rcl_get_zero_initialized_wait_set() {
  static rcl_wait_set_t null_wait_set = {
      .subscriptions = NULL,
      .size_of_subscriptions = 0,
      .guard_conditions = NULL,
      .size_of_guard_conditions = 0,
      .clients = NULL,
      .size_of_clients = 0,
      .services = NULL,
      .size_of_services = 0,
      .timers = NULL,
      .size_of_timers = 0,
      .events = NULL,
      .size_of_events = 0,
      .impl = NULL,
  };
  return null_wait_set;
}

/**
 * @brief 检查 wait_set 是否有效
 *
 * @param[in] wait_set 要检查的 rcl_wait_set_t 结构体指针
 * @return 如果 wait_set 有效，则返回 true，否则返回 false
 */
bool rcl_wait_set_is_valid(const rcl_wait_set_t *wait_set) { return wait_set && wait_set->impl; }

/**
 * @brief 清理 wait_set 结构体
 *
 * @param[in,out] wait_set 要清理的 rcl_wait_set_t 结构体指针
 */
static void __wait_set_clean_up(rcl_wait_set_t *wait_set) {
  // 调整 wait_set 的大小为 0
  rcl_ret_t ret = rcl_wait_set_resize(wait_set, 0, 0, 0, 0, 0, 0);
  (void)ret;                  // NO LINT
  assert(RCL_RET_OK == ret);  // Defensive, shouldn't fail with size 0.

  // 释放 wait_set 的内存
  if (wait_set->impl) {
    wait_set->impl->allocator.deallocate(wait_set->impl, wait_set->impl->allocator.state);
    wait_set->impl = NULL;
  }
}

/**
 * @brief 初始化 wait_set 结构体
 *
 * @param[out] wait_set 要初始化的 rcl_wait_set_t 结构体指针
 * @param[in] number_of_subscriptions 订阅数量
 * @param[in] number_of_guard_conditions 守护条件数量
 * @param[in] number_of_timers 定时器数量
 * @param[in] number_of_clients 客户端数量
 * @param[in] number_of_services 服务数量
 * @param[in] number_of_events 事件数量
 * @param[in] context rcl_context_t 结构体指针
 * @param[in] allocator 分配器
 * @return 返回 RCL_RET_OK 表示成功，其他值表示失败
 */
rcl_ret_t rcl_wait_set_init(
    rcl_wait_set_t *wait_set,
    size_t number_of_subscriptions,
    size_t number_of_guard_conditions,
    size_t number_of_timers,
    size_t number_of_clients,
    size_t number_of_services,
    size_t number_of_events,
    rcl_context_t *context,
    rcl_allocator_t allocator) {
  // 打印调试信息
  RCUTILS_LOG_DEBUG_NAMED(
      ROS_PACKAGE_NAME,
      "Initializing wait set with "
      "'%zu' subscriptions, '%zu' guard conditions, '%zu' timers, '%zu' clients, '%zu' services",
      number_of_subscriptions, number_of_guard_conditions, number_of_timers, number_of_clients,
      number_of_services);
  rcl_ret_t fail_ret = RCL_RET_ERROR;

  // 检查分配器是否有效
  RCL_CHECK_ALLOCATOR_WITH_MSG(&allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);
  // 检查 wait_set 是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(wait_set, RCL_RET_INVALID_ARGUMENT);
  // 检查 wait_set 是否已经初始化
  if (rcl_wait_set_is_valid(wait_set)) {
    RCL_SET_ERROR_MSG("wait_set already initialized, or memory was uninitialized.");
    return RCL_RET_ALREADY_INIT;
  }
  // 确保 rcl 已经初始化
  RCL_CHECK_ARGUMENT_FOR_NULL(context, RCL_RET_INVALID_ARGUMENT);
  if (!rcl_context_is_valid(context)) {
    RCL_SET_ERROR_MSG(
        "the given context is not valid, "
        "either rcl_init() was not called or rcl_shutdown() was called.");
    return RCL_RET_NOT_INIT;
  }
  // 为实现结构体分配空间
  wait_set->impl =
      (rcl_wait_set_impl_t *)allocator.allocate(sizeof(rcl_wait_set_impl_t), allocator.state);
  RCL_CHECK_FOR_NULL_WITH_MSG(wait_set->impl, "allocating memory failed", return RCL_RET_BAD_ALLOC);
  memset(wait_set->impl, 0, sizeof(rcl_wait_set_impl_t));
  wait_set->impl->rmw_subscriptions.subscribers = NULL;
  wait_set->impl->rmw_subscriptions.subscriber_count = 0;
  wait_set->impl->rmw_guard_conditions.guard_conditions = NULL;
  wait_set->impl->rmw_guard_conditions.guard_condition_count = 0;
  wait_set->impl->rmw_clients.clients = NULL;
  wait_set->impl->rmw_clients.client_count = 0;
  wait_set->impl->rmw_services.services = NULL;
  wait_set->impl->rmw_services.service_count = 0;
  wait_set->impl->rmw_events.events = NULL;
  wait_set->impl->rmw_events.event_count = 0;
  // 设置上下文
  wait_set->impl->context = context;
  // 设置分配器
  wait_set->impl->allocator = allocator;

  size_t num_conditions = (2 * number_of_subscriptions) + number_of_guard_conditions +
                          number_of_clients + number_of_services + number_of_events;

  // 创建 rmw_wait_set
  wait_set->impl->rmw_wait_set = rmw_create_wait_set(&(context->impl->rmw_context), num_conditions);
  if (!wait_set->impl->rmw_wait_set) {
    goto fail;
  }

  // 初始化订阅空间
  rcl_ret_t ret = rcl_wait_set_resize(
      wait_set, number_of_subscriptions, number_of_guard_conditions, number_of_timers,
      number_of_clients, number_of_services, number_of_events);
  if (RCL_RET_OK != ret) {
    fail_ret = ret;
    goto fail;
  }
  return RCL_RET_OK;
fail:
  if (rcl_wait_set_is_valid(wait_set)) {
    rmw_ret_t ret = rmw_destroy_wait_set(wait_set->impl->rmw_wait_set);
    if (ret != RMW_RET_OK) {
      fail_ret = RCL_RET_WAIT_SET_INVALID;
    }
  }
  __wait_set_clean_up(wait_set);
  return fail_ret;
}

/**
 * @brief 销毁一个等待集合 (wait set)
 *
 * @param[in,out] wait_set 指向要销毁的等待集合的指针
 * @return 返回 RCL_RET_OK 表示成功，其他值表示失败
 */
rcl_ret_t rcl_wait_set_fini(rcl_wait_set_t *wait_set) {
  // 初始化结果为 RCL_RET_OK
  rcl_ret_t result = RCL_RET_OK;
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(wait_set, RCL_RET_INVALID_ARGUMENT);

  // 如果等待集合有效，则执行销毁操作
  if (rcl_wait_set_is_valid(wait_set)) {
    // 调用 rmw_destroy_wait_set 销毁底层的等待集合
    rmw_ret_t ret = rmw_destroy_wait_set(wait_set->impl->rmw_wait_set);
    // 如果销毁失败，设置错误信息并更新结果
    if (ret != RMW_RET_OK) {
      RCL_SET_ERROR_MSG(rmw_get_error_string().str);
      result = RCL_RET_WAIT_SET_INVALID;
    }
    // 清理等待集合
    __wait_set_clean_up(wait_set);
  }
  // 返回结果
  return result;
}

/**
 * @brief 获取等待集合的分配器
 *
 * @param[in] wait_set 指向等待集合的指针
 * @param[out] allocator 指向分配器的指针，用于存储获取到的分配器
 * @return 返回 RCL_RET_OK 表示成功，其他值表示失败
 */
rcl_ret_t rcl_wait_set_get_allocator(const rcl_wait_set_t *wait_set, rcl_allocator_t *allocator) {
  // 检查输入参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(wait_set, RCL_RET_INVALID_ARGUMENT);
  // 如果等待集合无效，设置错误信息并返回
  if (!rcl_wait_set_is_valid(wait_set)) {
    RCL_SET_ERROR_MSG("wait set is invalid");
    return RCL_RET_WAIT_SET_INVALID;
  }
  // 检查输出参数是否为空
  RCL_CHECK_ARGUMENT_FOR_NULL(allocator, RCL_RET_INVALID_ARGUMENT);
  // 获取分配器并存储到输出参数中
  *allocator = wait_set->impl->allocator;
  // 返回成功
  return RCL_RET_OK;
}

/**
 * @brief 宏定义，用于向等待集合添加指定类型的元素
 *
 * @param Type 要添加的元素类型
 * 这里在第一行去除了 `\`，为了避免格式化的问题，不知道会不会影响编译
 */
#define SET_ADD(Type)
/* 检查输入参数是否为空 */
RCL_CHECK_ARGUMENT_FOR_NULL(
    wait_set, RCL_RET_INVALID_ARGUMENT); /* 如果等待集合无效，设置错误信息并返回 */
if (!wait_set->impl) {
  RCL_SET_ERROR_MSG("wait set is invalid");
  return RCL_RET_WAIT_SET_INVALID;
} /* 检查输入参数是否为空 */
RCL_CHECK_ARGUMENT_FOR_NULL(
    Type, RCL_RET_INVALID_ARGUMENT); /* 如果等待集合已满，设置错误信息并返回 */
if (!(wait_set->impl->Type##_index < wait_set->size_of_##Type##s)) {
  RCL_SET_ERROR_MSG(#Type "s set is full");
  return RCL_RET_WAIT_SET_FULL;
} /* 获取当前索引并递增 */
size_t current_index = wait_set->impl->Type##_index++; /* 将元素添加到等待集合中 */
wait_set->Type##s[current_index] = Type;               /* 设置可选的输出参数 */
if (NULL != index) {
  *index = current_index;
}

/**
 * @brief 为给定的类型设置RMW存储和计数，同时将其放入rmw存储中。
 *
 * @param Type 类型名称，例如：subscription、client等。
 * @param RMWStorage RMW存储变量名，例如：subscriptions、clients等。
 * @param RMWCount RMW计数变量名，例如：number_of_subscriptions、number_of_clients等。
 */
#define SET_ADD_RMW(Type, RMWStorage, RMWCount)                                                   \
  /* 将其放入rmw存储中。 */                                                               \
  rmw_##Type##_t *rmw_handle = rcl_##Type##_get_rmw_handle(Type);                                 \
  /* 检查rmw_handle是否为空，如果为空则返回错误信息并返回RCL_RET_ERROR。 */ \
  RCL_CHECK_FOR_NULL_WITH_MSG(rmw_handle, rcl_get_error_string().str, return RCL_RET_ERROR);      \
  /* 将rmw_handle的数据存储到wait_set的impl->RMWStorage[current_index]中。 */           \
  wait_set->impl->RMWStorage[current_index] = rmw_handle->data;                                   \
  /* 增加wait_set的impl->RMWCount计数。 */                                                  \
  wait_set->impl->RMWCount++;

/**
 * @brief 清除给定类型的wait_set存储，并将索引重置为0。
 *
 * @param Type 类型名称，例如：subscription、client等。
 */
#define SET_CLEAR(Type)                                                                          \
  do {                                                                                           \
    /* 如果wait_set的Type##s不为空，则执行清除操作。 */                           \
    if (NULL != wait_set->Type##s) {                                                             \
      /* 使用memset将wait_set的Type##s内存区域设置为0。 */                           \
      memset(                                                                                    \
          (void *)wait_set->Type##s, 0, sizeof(rcl_##Type##_t *) * wait_set->size_of_##Type##s); \
      /* 将wait_set的impl->Type##_index重置为0。 */                                        \
      wait_set->impl->Type##_index = 0;                                                          \
    }                                                                                            \
  } while (false)

/**
 * @brief 清除给定类型的RMW存储，并将计数重置为0。
 *
 * @param Type 类型名称，例如：subscription、client等。
 * @param RMWStorage RMW存储变量名，例如：subscriptions、clients等。
 * @param RMWCount RMW计数变量名，例如：number_of_subscriptions、number_of_clients等。
 */
#define SET_CLEAR_RMW(Type, RMWStorage, RMWCount)                                       \
  do {                                                                                  \
    /* 如果wait_set的impl->RMWStorage不为空，则执行清除操作。 */         \
    if (NULL != wait_set->impl->RMWStorage) {                                           \
      /* 使用memset将wait_set的impl->RMWStorage内存区域设置为0。 */         \
      memset(wait_set->impl->RMWStorage, 0, sizeof(void *) * wait_set->impl->RMWCount); \
      /* 将wait_set的impl->RMWCount重置为0。 */                                   \
      wait_set->impl->RMWCount = 0;                                                     \
    }                                                                                   \
  } while (false)

/**
 * @brief 宏函数，用于调整wait_set中Type类型的大小。
 *
 * @param Type 类型名称，如：subscription、guard_condition等。
 * @param ExtraDealloc 当Type##s_size为0时，需要额外执行的释放操作。
 * @param ExtraRealloc 当Type##s_size不为0时，需要额外执行的重新分配操作。
 */
#define SET_RESIZE(Type, ExtraDealloc, ExtraRealloc)                                                   \
  do {                                                                                                 \
    rcl_allocator_t allocator = wait_set->impl->allocator; /* 获取内存分配器 */                 \
    wait_set->size_of_##Type##s = 0;                       /* 将Type类型的数量置为0 */         \
    wait_set->impl->Type##_index = 0;                      /* 将Type类型的索引置为0 */         \
    if (0 == Type##s_size) {                               /* 如果Type类型的大小为0 */         \
      if (wait_set->Type##s) {                             /* 如果Type类型存在 */                \
        allocator.deallocate((void *)wait_set->Type##s, allocator.state); /* 释放Type类型内存 */ \
        wait_set->Type##s = NULL; /* 将Type类型指针置为空 */                                   \
      }                                                                                                \
      ExtraDealloc                /* 执行额外的释放操作 */                                    \
    } else {                      /* 如果Type类型的大小不为0 */                               \
      wait_set->Type##s = (const rcl_##Type##_t **)allocator.reallocate(                               \
          (void *)wait_set->Type##s, sizeof(rcl_##Type##_t *) * Type##s_size,                          \
          allocator.state); /* 重新分配Type类型内存 */                                         \
      RCL_CHECK_FOR_NULL_WITH_MSG(                                                                     \
          wait_set->Type##s, "allocating memory failed",                                               \
          return RCL_RET_BAD_ALLOC); /* 检查内存分配是否成功 */                              \
      memset(                                                                                          \
          (void *)wait_set->Type##s, 0,                                                                \
          sizeof(rcl_##Type##_t *) * Type##s_size); /* 将分配的内存初始化为0 */              \
      wait_set->size_of_##Type##s = Type##s_size;   /* 设置Type类型的数量 */                    \
      ExtraRealloc                                  /* 执行额外的重新分配操作 */            \
    }                                                                                                  \
  } while (false)

/**
 * @brief 宏函数，用于释放RMW相关的存储空间。
 *
 * @param RMWStorage RMW相关的存储空间变量名。
 * @param RMWCount RMW相关的计数变量名。
 */
#define SET_RESIZE_RMW_DEALLOC(RMWStorage, RMWCount)                                            \
  /* Also deallocate the rmw storage. */                                                        \
  if (wait_set->impl->RMWStorage) {                           /* 如果RMW存储空间存在 */ \
    allocator.deallocate(                                                                       \
        (void *)wait_set->impl->RMWStorage, allocator.state); /* 释放RMW存储空间 */       \
    wait_set->impl->RMWStorage = NULL; /* 将RMW存储空间指针置为空 */                  \
    wait_set->impl->RMWCount = 0;      /* 将RMW计数置为0 */                                \
  }

/**
 * @brief 宏函数，用于调整RMW相关的存储空间大小。
 *
 * @param Type 类型名称，如：subscription、guard_condition等。
 * @param RMWStorage RMW相关的存储空间变量名。
 * @param RMWCount RMW相关的计数变量名。
 */
#define SET_RESIZE_RMW_REALLOC(Type, RMWStorage, RMWCount)                                         \
  /* Also resize the rmw storage. */                                                               \
  wait_set->impl->RMWCount = 0; /* 将RMW计数置为0 */                                          \
  wait_set->impl->RMWStorage = (void **)allocator.reallocate(                                      \
      wait_set->impl->RMWStorage, sizeof(void *) * Type##s_size,                                   \
      allocator.state);              /* 重新分配RMW存储空间大小 */                       \
  if (!wait_set->impl->RMWStorage) { /* 如果RMW存储空间分配失败 */                       \
    allocator.deallocate((void *)wait_set->Type##s, allocator.state); /* 释放Type类型内存 */ \
    wait_set->Type##s = NULL;                      /* 将Type类型指针置为空 */              \
    wait_set->size_of_##Type##s = 0;               /* 将Type类型的数量置为0 */             \
    RCL_SET_ERROR_MSG("allocating memory failed"); /* 设置错误信息 */                        \
    return RCL_RET_BAD_ALLOC;                      /* 返回内存分配失败错误码 */         \
  }                                                                                                \
  memset(                                                                                          \
      wait_set->impl->RMWStorage, 0,                                                               \
      sizeof(void *) * Type##s_size); /* 将分配的RMW存储空间初始化为0 */

/**
 * @brief 添加订阅者到等待集合中
 *
 * 将 rmw 表示添加到底层的 rmw 数组并增加 rmw 数组计数。
 *
 * @param[in,out] wait_set 指向 rcl_wait_set_t 结构体的指针，用于存储等待集合
 * @param[in] subscription 指向 rcl_subscription_t 结构体的指针，表示要添加的订阅者
 * @param[out] index 返回添加订阅者在等待集合中的索引位置
 * @return 返回 rcl_ret_t 类型的结果，成功返回 RCL_RET_OK
 */
rcl_ret_t rcl_wait_set_add_subscription(
    rcl_wait_set_t *wait_set, const rcl_subscription_t *subscription, size_t *index) {
  // 调用 SET_ADD 宏，将订阅者添加到等待集合中
  SET_ADD(subscription)
  // 调用 SET_ADD_RMW 宏，将订阅者添加到底层的 rmw 数组，并增加 rmw 数组计数
  SET_ADD_RMW(subscription, rmw_subscriptions.subscribers, rmw_subscriptions.subscriber_count)
  // 返回操作成功的结果
  return RCL_RET_OK;
}

/**
 * @brief 清除等待集合中的所有条目
 *
 * 将底层 rmw 数组中的所有条目设置为 null，并将 rmw 数组的计数设置为 0。
 *
 * @param[in,out] wait_set 指向 rcl_wait_set_t 结构体的指针，用于存储等待集合
 * @return 返回 rcl_ret_t 类型的结果，成功返回 RCL_RET_OK
 */
rcl_ret_t rcl_wait_set_clear(rcl_wait_set_t *wait_set) {
  // 检查 wait_set 参数是否为 null，如果是则返回 RCL_RET_INVALID_ARGUMENT 错误
  RCL_CHECK_ARGUMENT_FOR_NULL(wait_set, RCL_RET_INVALID_ARGUMENT);
  // 检查 wait_set->impl 是否为 null，如果是则返回 RCL_RET_WAIT_SET_INVALID 错误
  RCL_CHECK_ARGUMENT_FOR_NULL(wait_set->impl, RCL_RET_WAIT_SET_INVALID);

  // 调用 SET_CLEAR 宏，清除订阅者、守护条件、客户端、服务、事件和定时器
  SET_CLEAR(subscription);
  SET_CLEAR(guard_condition);
  SET_CLEAR(client);
  SET_CLEAR(service);
  SET_CLEAR(event);
  SET_CLEAR(timer);

  // 调用 SET_CLEAR_RMW 宏，将底层 rmw 数组中的所有条目设置为 null，并将 rmw 数组的计数设置为 0
  SET_CLEAR_RMW(subscription, rmw_subscriptions.subscribers, rmw_subscriptions.subscriber_count);
  SET_CLEAR_RMW(
      guard_condition, rmw_guard_conditions.guard_conditions,
      rmw_guard_conditions.guard_condition_count);
  SET_CLEAR_RMW(clients, rmw_clients.clients, rmw_clients.client_count);
  SET_CLEAR_RMW(services, rmw_services.services, rmw_services.service_count);
  SET_CLEAR_RMW(events, rmw_events.events, rmw_events.event_count);

  // 返回操作成功的结果
  return RCL_RET_OK;
}

/**
 * @brief 调整 rcl_wait_set_t 结构体的大小，重新分配内存并重置相关字段。
 *
 * 实现特定说明：
 * 类似地，底层的 rmw 表示也会被重新分配和重置：所有条目都设置为 null，计数设置为零。
 *
 * @param[in,out] wait_set 指向要调整大小的 rcl_wait_set_t 结构体的指针。
 * @param[in] subscriptions_size 新的订阅数组大小。
 * @param[in] guard_conditions_size 新的保护条件数组大小。
 * @param[in] timers_size 新的定时器数组大小。
 * @param[in] clients_size 新的客户端数组大小。
 * @param[in] services_size 新的服务数组大小。
 * @param[in] events_size 新的事件数组大小。
 * @return 返回 RCL_RET_OK 如果成功，否则返回相应的错误代码。
 */
rcl_ret_t rcl_wait_set_resize(
    rcl_wait_set_t *wait_set,
    size_t subscriptions_size,
    size_t guard_conditions_size,
    size_t timers_size,
    size_t clients_size,
    size_t services_size,
    size_t events_size) {
  // 检查 wait_set 和 wait_set->impl 是否为空，如果为空则返回 RCL_RET_INVALID_ARGUMENT 错误
  RCL_CHECK_ARGUMENT_FOR_NULL(wait_set, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(wait_set->impl, RCL_RET_WAIT_SET_INVALID);

  // 调整订阅数组的大小，并重新分配内存
  SET_RESIZE(
      subscription,
      SET_RESIZE_RMW_DEALLOC(rmw_subscriptions.subscribers, rmw_subscriptions.subscriber_count),
      SET_RESIZE_RMW_REALLOC(
          subscription, rmw_subscriptions.subscribers, rmw_subscriptions.subscriber_count));

  // 调整保护条件数组的大小，不需要重新分配内存
  SET_RESIZE(guard_condition, ;, ;);  // NOLINT

  // 调整 RMW 保护条件数组的大小，需要考虑保护条件和定时器的数量
  rmw_guard_conditions_t *rmw_gcs = &(wait_set->impl->rmw_guard_conditions);
  const size_t num_rmw_gc = guard_conditions_size + timers_size;

  // 清除已添加的保护条件
  rmw_gcs->guard_condition_count = 0u;
  if (0u == num_rmw_gc) {
    if (rmw_gcs->guard_conditions) {
      wait_set->impl->allocator.deallocate(
          (void *)rmw_gcs->guard_conditions, wait_set->impl->allocator.state);
      rmw_gcs->guard_conditions = NULL;
    }
  } else {
    // 重新分配内存并调整保护条件数组的大小
    rmw_gcs->guard_conditions = (void **)wait_set->impl->allocator.reallocate(
        rmw_gcs->guard_conditions, sizeof(void *) * num_rmw_gc, wait_set->impl->allocator.state);

    // 如果重新分配失败，释放相关资源并返回 RCL_RET_BAD_ALLOC 错误
    if (!rmw_gcs->guard_conditions) {
      wait_set->impl->allocator.deallocate(
          (void *)wait_set->guard_conditions, wait_set->impl->allocator.state);
      wait_set->size_of_guard_conditions = 0u;
      wait_set->guard_conditions = NULL;
      wait_set->impl->allocator.deallocate(
          (void *)wait_set->timers, wait_set->impl->allocator.state);
      wait_set->size_of_timers = 0u;
      wait_set->timers = NULL;
      RCL_SET_ERROR_MSG("allocating memory failed");
      return RCL_RET_BAD_ALLOC;
    }
    // 将重新分配的内存设置为零
    memset(rmw_gcs->guard_conditions, 0, sizeof(void *) * num_rmw_gc);
  }

  // 调整定时器数组的大小，不需要重新分配内存
  SET_RESIZE(timer, ;, ;);  // NOLINT

  // 调整客户端数组的大小，并重新分配内存
  SET_RESIZE(
      client, SET_RESIZE_RMW_DEALLOC(rmw_clients.clients, rmw_clients.client_count),
      SET_RESIZE_RMW_REALLOC(client, rmw_clients.clients, rmw_clients.client_count));

  // 调整服务数组的大小，并重新分配内存
  SET_RESIZE(
      service, SET_RESIZE_RMW_DEALLOC(rmw_services.services, rmw_services.service_count),
      SET_RESIZE_RMW_REALLOC(service, rmw_services.services, rmw_services.service_count));

  // 调整事件数组的大小，并重新分配内存
  SET_RESIZE(
      event, SET_RESIZE_RMW_DEALLOC(rmw_events.events, rmw_events.event_count),
      SET_RESIZE_RMW_REALLOC(event, rmw_events.events, rmw_events.event_count));

  // 返回成功
  return RCL_RET_OK;
}

/**
 * @brief 添加一个守护条件到等待集合中。
 *
 * @param[in] wait_set 指向要添加守护条件的等待集合的指针。
 * @param[in] guard_condition 要添加到等待集合中的守护条件。
 * @param[out] index 守护条件在等待集合中的索引。
 * @return 返回 RCL_RET_OK 表示成功，其他值表示失败。
 */
rcl_ret_t rcl_wait_set_add_guard_condition(
    rcl_wait_set_t *wait_set, const rcl_guard_condition_t *guard_condition, size_t *index) {
  // 使用 SET_ADD 宏添加守护条件
  SET_ADD(guard_condition)

  // 使用 SET_ADD_RMW 宏将守护条件添加到 rmw_guard_conditions 中
  SET_ADD_RMW(
      guard_condition, rmw_guard_conditions.guard_conditions,
      rmw_guard_conditions.guard_condition_count)

  return RCL_RET_OK;
}

/**
 * @brief 添加一个定时器到等待集合中。
 *
 * @param[in] wait_set 指向要添加定时器的等待集合的指针。
 * @param[in] timer 要添加到等待集合中的定时器。
 * @param[out] index 定时器在等待集合中的索引。
 * @return 返回 RCL_RET_OK 表示成功，其他值表示失败。
 */
rcl_ret_t rcl_wait_set_add_timer(
    rcl_wait_set_t *wait_set, const rcl_timer_t *timer, size_t *index) {
  // 使用 SET_ADD 宏添加定时器
  SET_ADD(timer)

  // 将定时器的守护条件添加到 rmw 守护条件集合的末尾
  rcl_guard_condition_t *guard_condition = rcl_timer_get_guard_condition(timer);
  if (NULL != guard_condition) {
    // rcl_wait() 将负责将这些向后移动并设置 guard_condition_count。
    const size_t index = wait_set->size_of_guard_conditions + (wait_set->impl->timer_index - 1);
    rmw_guard_condition_t *rmw_handle = rcl_guard_condition_get_rmw_handle(guard_condition);
    RCL_CHECK_FOR_NULL_WITH_MSG(rmw_handle, rcl_get_error_string().str, return RCL_RET_ERROR);
    wait_set->impl->rmw_guard_conditions.guard_conditions[index] = rmw_handle->data;
  }
  return RCL_RET_OK;
}

/**
 * @brief 添加一个客户端到等待集合中。
 *
 * @param[in] wait_set 指向要添加客户端的等待集合的指针。
 * @param[in] client 要添加到等待集合中的客户端。
 * @param[out] index 客户端在等待集合中的索引。
 * @return 返回 RCL_RET_OK 表示成功，其他值表示失败。
 */
rcl_ret_t rcl_wait_set_add_client(
    rcl_wait_set_t *wait_set, const rcl_client_t *client, size_t *index) {
  // 使用 SET_ADD 宏添加客户端
  SET_ADD(client)

  // 使用 SET_ADD_RMW 宏将客户端添加到 rmw_clients 中
  SET_ADD_RMW(client, rmw_clients.clients, rmw_clients.client_count)

  return RCL_RET_OK;
}

/**
 * @brief 添加一个服务到等待集合中。
 *
 * @param[in] wait_set 指向要添加服务的等待集合的指针。
 * @param[in] service 要添加到等待集合中的服务。
 * @param[out] index 服务在等待集合中的索引。
 * @return 返回 RCL_RET_OK 表示成功，其他值表示失败。
 */
rcl_ret_t rcl_wait_set_add_service(
    rcl_wait_set_t *wait_set, const rcl_service_t *service, size_t *index) {
  // 使用 SET_ADD 宏添加服务
  SET_ADD(service)

  // 使用 SET_ADD_RMW 宏将服务添加到 rmw_services 中
  SET_ADD_RMW(service, rmw_services.services, rmw_services.service_count)

  return RCL_RET_OK;
}

/**
 * @brief 添加一个事件到等待集合中。
 *
 * @param[in] wait_set 指向要添加事件的等待集合的指针。
 * @param[in] event 要添加到等待集合中的事件。
 * @param[out] index 事件在等待集合中的索引。
 * @return 返回 RCL_RET_OK 表示成功，其他值表示失败。
 */
rcl_ret_t rcl_wait_set_add_event(
    rcl_wait_set_t *wait_set, const rcl_event_t *event, size_t *index) {
  // 使用 SET_ADD 宏添加事件
  SET_ADD(event)

  // 使用 SET_ADD_RMW 宏将事件添加到 rmw_events 中
  SET_ADD_RMW(event, rmw_events.events, rmw_events.event_count)

  // 将 rmw_handle 添加到等待集合的实现中
  wait_set->impl->rmw_events.events[current_index] = rmw_handle;

  return RCL_RET_OK;
}

/**
 * @brief 等待wait_set中的事件，直到超时或有事件触发。
 *
 * @param[in] wait_set 指向rcl_wait_set_t类型的指针，用于存储等待的事件。
 * @param[in] timeout
 * 超时时间（以纳秒为单位），如果为负数，则阻塞等待；如果为0，则立即返回；如果为正数，则等待指定的纳秒数。
 * @return rcl_ret_t 返回RCL_RET_OK表示成功，其他值表示失败。
 */
rcl_ret_t rcl_wait(rcl_wait_set_t *wait_set, int64_t timeout) {
  // 检查wait_set是否为空，如果为空则返回错误
  RCL_CHECK_ARGUMENT_FOR_NULL(wait_set, RCL_RET_INVALID_ARGUMENT);
  // 检查wait_set是否有效，如果无效则设置错误消息并返回错误
  if (!rcl_wait_set_is_valid(wait_set)) {
    RCL_SET_ERROR_MSG("wait set is invalid");
    return RCL_RET_WAIT_SET_INVALID;
  }
  // 检查wait_set是否为空，如果为空则设置错误消息并返回错误
  if (wait_set->size_of_subscriptions == 0 && wait_set->size_of_guard_conditions == 0 &&
      wait_set->size_of_timers == 0 && wait_set->size_of_clients == 0 &&
      wait_set->size_of_services == 0 && wait_set->size_of_events == 0) {
    RCL_SET_ERROR_MSG("wait set is empty");
    return RCL_RET_WAIT_SET_EMPTY;
  }
  // 计算超时参数
  // 默认情况下，如果不满足以下条件，则将计时器设置为无限期阻塞。
  rmw_time_t *timeout_argument = NULL;
  rmw_time_t temporary_timeout_storage;

  bool is_timer_timeout = false;
  int64_t min_timeout = timeout > 0 ? timeout : INT64_MAX;
  {  // 作用域，以防止下面的i冲突
    uint64_t i = 0;
    for (i = 0; i < wait_set->impl->timer_index; ++i) {
      if (!wait_set->timers[i]) {
        continue;  // 跳过空计时器
      }
      rmw_guard_conditions_t *rmw_gcs = &(wait_set->impl->rmw_guard_conditions);
      size_t gc_idx = wait_set->size_of_guard_conditions + i;
      if (NULL != rmw_gcs->guard_conditions[gc_idx]) {
        // 此计时器具有保护条件，因此将其移动以创建合法的等待集。
        rmw_gcs->guard_conditions[rmw_gcs->guard_condition_count] =
            rmw_gcs->guard_conditions[gc_idx];
        ++(rmw_gcs->guard_condition_count);
      }
      // 使用计时器时间来设置rmw_wait超时
      // TODO(sloretz) 在启用ROS_TIME的情况下修复ROS_TIME计时器上的虚假唤醒
      int64_t timer_timeout = INT64_MAX;
      rcl_ret_t ret = rcl_timer_get_time_until_next_call(wait_set->timers[i], &timer_timeout);
      if (ret == RCL_RET_TIMER_CANCELED) {
        wait_set->timers[i] = NULL;
        continue;
      }
      if (ret != RCL_RET_OK) {
        return ret;  // rcl错误状态应已设置。
      }
      if (timer_timeout < min_timeout) {
        is_timer_timeout = true;
        min_timeout = timer_timeout;
      }
    }
  }

  if (timeout == 0) {
    // 那么它是非阻塞的，因此将临时存储设置为0,0并传递它。
    temporary_timeout_storage.sec = 0;
    temporary_timeout_storage.nsec = 0;
    timeout_argument = &temporary_timeout_storage;
  } else if (timeout > 0 || is_timer_timeout) {
    // 如果min_timeout为负数，我们需要立即唤醒。
    if (min_timeout < 0) {
      min_timeout = 0;
    }
    temporary_timeout_storage.sec = RCL_NS_TO_S(min_timeout);
    temporary_timeout_storage.nsec = min_timeout % 1000000000;
    timeout_argument = &temporary_timeout_storage;
  }

  // 等待
  rmw_ret_t ret = rmw_wait(
      &wait_set->impl->rmw_subscriptions, &wait_set->impl->rmw_guard_conditions,
      &wait_set->impl->rmw_services, &wait_set->impl->rmw_clients, &wait_set->impl->rmw_events,
      wait_set->impl->rmw_wait_set, timeout_argument);

  // rmw_wait将未准备好的项目设置为NULL。
  // 我们现在相应地更新我们的句柄。

  // 检查就绪计时器
  // 并将未准备好的计时器（包括取消的计时器）设置为NULL。
  size_t i;
  for (i = 0; i < wait_set->impl->timer_index; ++i) {
    if (!wait_set->timers[i]) {
      continue;
    }
    bool is_ready = false;
    rcl_ret_t ret = rcl_timer_is_ready(wait_set->timers[i], &is_ready);
    if (ret != RCL_RET_OK) {
      return ret;  // rcl错误状态应已设置。
    }
    if (!is_ready) {
      wait_set->timers[i] = NULL;
    }
  }
  // 检查超时，如果不是计时器，则返回RCL_RET_TIMEOUT。
  if (ret != RMW_RET_OK && ret != RMW_RET_TIMEOUT) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    return RCL_RET_ERROR;
  }
  // 将相应的rcl订阅句柄设置为NULL。
  for (i = 0; i < wait_set->size_of_subscriptions; ++i) {
    bool is_ready = wait_set->impl->rmw_subscriptions.subscribers[i] != NULL;
    if (!is_ready) {
      wait_set->subscriptions[i] = NULL;
    }
  }
  // 将相应的rcl guard_condition句柄设置为NULL。
  for (i = 0; i < wait_set->size_of_guard_conditions; ++i) {
    bool is_ready = wait_set->impl->rmw_guard_conditions.guard_conditions[i] != NULL;
    if (!is_ready) {
      wait_set->guard_conditions[i] = NULL;
    }
  }
  // 将相应的rcl客户端句柄设置为NULL。
  for (i = 0; i < wait_set->size_of_clients; ++i) {
    bool is_ready = wait_set->impl->rmw_clients.clients[i] != NULL;
    if (!is_ready) {
      wait_set->clients[i] = NULL;
    }
  }
  // 将相应的rcl服务句柄设置为NULL。
  for (i = 0; i < wait_set->size_of_services; ++i) {
    bool is_ready = wait_set->impl->rmw_services.services[i] != NULL;
    if (!is_ready) {
      wait_set->services[i] = NULL;
    }
  }
  // 将相应的rcl事件句柄设置为NULL。
  for (i = 0; i < wait_set->size_of_events; ++i) {
    bool is_ready = wait_set->impl->rmw_events.events[i] != NULL;
    if (!is_ready) {
      wait_set->events[i] = NULL;
    }
  }

  if (RMW_RET_TIMEOUT == ret && !is_timer_timeout) {
    return RCL_RET_TIMEOUT;
  }
  return RCL_RET_OK;
}

#ifdef __cplusplus
}
#endif
