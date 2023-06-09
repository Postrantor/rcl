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

#ifndef RCL__CONTEXT_H_
#define RCL__CONTEXT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "rcl/allocator.h"
#include "rcl/arguments.h"
#include "rcl/init_options.h"
#include "rcl/macros.h"
#include "rcl/types.h"
#include "rcl/visibility_control.h"
#include "rmw/init.h"

/// @cond Doxygen_Suppress
#ifdef _MSC_VER
#define RCL_ALIGNAS(N) \
  __declspec(align(N))  // 当编译器为 MSVC 时，定义 RCL_ALIGNAS(N) 为 __declspec(align(N))
#else
#include <stdalign.h>   // 包含 stdalign.h 头文件
#define RCL_ALIGNAS(N) alignas(N)  // 当编译器不是 MSVC 时，定义 RCL_ALIGNAS(N) 为 alignas(N)
#endif
/// @endcond

/// \brief 一个唯一的ID，用于表示每个上下文实例。
///
/// rcl_context_instance_id_t 是一个无符号64位整数类型，用于表示 ROS2 上下文实例的唯一标识符。
typedef uint64_t rcl_context_instance_id_t;

/// \brief rcl_context_impl_s 结构体的前向声明。
///
/// rcl_context_impl_t 是一个指向 rcl_context_impl_s
/// 结构体的指针类型。这里使用前向声明，以便在其他地方定义和使用该结构体。
typedef struct rcl_context_impl_s rcl_context_impl_t;

/// 封装 init/shutdown 周期的非全局状态。
/**
 * 上下文用于创建顶级实体，如节点和保护条件，以及关闭特定实例的初始化。
 *
 * 下面是典型上下文生命周期的示意图：
 *
 * ```
 *    +---------------+
 *    |               |
 * +--> 未初始化状态 +---> rcl_get_zero_initialized_context() +
 * |  |               |                                        |
 * |  +---------------+                                        |
 * |                                                           |
 * |           +-----------------------------------------------+
 * |           |
 * |  +--------v---------+                +-----------------------+
 * |  |                  |                |                       |
 * |  | 零初始化状态 +-> rcl_init() +-> 初始化且有效状态 +-> rcl_shutdown() +
 * |  |                  |                |                       |                  |
 * |  +------------------+                +-----------------------+                  |
 * |                                                                                 |
 * |               +-----------------------------------------------------------------+
 * |               |
 * |  +------------v------------+
 * |  |                         |
 * |  | 初始化但无效状态 +---> 结束所有实体，然后 rcl_context_fini() +
 * |  |                         |                                                    |
 * |  +-------------------------+                                                    |
 * |                                                                                 |
 * +---------------------------------------------------------------------------------+
 * ```
 *
 * 声明但未定义的 rcl_context_t
 * 实例被认为是“未初始化”的，将未初始化的上下文传递给任何函数都会导致未定义的行为。 一些函数，如
 * rcl_init() 在使用前需要上下文实例为零初始化（所有成员设置为“零”状态）。
 *
 * 使用 rcl_get_zero_initialized_context() 对 rcl_context_t
 * 进行零初始化，确保上下文处于安全状态，以便使用 rcl_init() 进行初始化。
 *
 * 使用 rcl_init() 对 rcl_context_t 进行初始化后，上下文被认为已初始化且有效。
 * 初始化后，它可以用于创建其他实体，如节点和保护条件。
 *
 * 在任何时候，都可以通过在 rcl_context_t 上调用 rcl_shutdown()
 * 来使上下文失效，此后，上下文仍然是初始化的，但现在无效。
 *
 * 使无效表示给其他实体指示上下文已关闭，但在清理期间仍可访问。
 *
 * 在失效后，以及使用过它的所有实体都已完成后，应使用 rcl_context_fini() 对上下文进行最终处理。
 *
 * 在还未完成拥有其副本的实体时对上下文进行最终处理是未定义的行为。
 * 因此，上下文的生命周期（在 rcl_init() 和 rcl_context_fini()
 * 调用之间）应超过所有直接（例如节点和保护条件）或间接（例如订阅和主题）使用它的实体。
 */
typedef struct rcl_context_s {
  /// 所有共享此上下文的节点的全局参数。
  /** 通常由 rcl_init() 中的 argc/argv 解析生成。 */
  rcl_arguments_t global_arguments;

  /// 特定于实现的指针。
  rcl_context_impl_t* impl;

  // 确保这对于 atomic_uint_least64_t 足够大的假设是在 context.c 文件中的 static_assert 中确保的。
  // 在大多数情况下，它应该只是一个普通的 uint64_t。
/// @cond Doxygen_Suppress
#if !defined(RCL_CONTEXT_ATOMIC_INSTANCE_ID_STORAGE_SIZE)
#define RCL_CONTEXT_ATOMIC_INSTANCE_ID_STORAGE_SIZE sizeof(uint_least64_t)
#endif
  /// @endcond
  /// 实例 ID 原子的私有存储。
  /**
   * 访问实例 id 应使用函数 rcl_context_get_instance_id()，因为实例 id
   * 的类型是原子的，需要正确访问以确保安全性。
   *
   * 实例 id 不应手动更改 - 这样做是未定义的行为。
   *
   * 实例 id 不能在 `impl` 指针的类型中受到保护，因为即使在上下文为零初始化且 `impl` 为 `NULL`
   * 时，也需要访问它。 具体来说，在 `impl` 中存储实例 id 会在访问它和完成上下文之间引入竞争条件。
   * 此外，在将此头文件包含到 C++ 程序中时，无法直接在此处使用 C11 原子（即 "stdatomic.h"）。
   * 请参阅此论文，了解将来可能实现这一点的努力：
   *   http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0943r1.html
   */
  RCL_ALIGNAS(8) uint8_t instance_id_storage[RCL_CONTEXT_ATOMIC_INSTANCE_ID_STORAGE_SIZE];
} rcl_context_t;

/// 返回一个零初始化的上下文对象。
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_context_t rcl_get_zero_initialized_context(void);

/// 结束一个上下文。
/**
 * 要结束的上下文必须先用 rcl_init() 初始化，然后用 rcl_shutdown() 使之无效。
 * 未初始化的零初始化上下文可以被结束。
 * 如果 context 为 `NULL`，则返回 #RCL_RET_INVALID_ARGUMENT。
 * 如果 context 为零初始化，则返回 #RCL_RET_OK。
 * 如果 context 已初始化且有效（未对其调用 rcl_shutdown()），则返回 #RCL_RET_INVALID_ARGUMENT。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 是
 * 线程安全          | 否
 * 使用原子操作      | 是
 * 无锁              | 是 [1]
 * <i>[1] 如果 `atomic_is_lock_free()` 对 `atomic_uint_least64_t` 返回 true</i>
 *
 * \param[inout] 要结束的上下文对象。
 * \return 如果成功完成关闭，则返回 #RCL_RET_OK，或
 * \return 如果任何参数无效，则返回 #RCL_RET_INVALID_ARGUMENT，或
 * \return 如果发生未指定的错误，则返回 #RCL_RET_ERROR。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_context_fini(rcl_context_t* context);

/// 返回此上下文在初始化期间使用的初始化选项。
/**
 * 此函数可能失败并返回 `NULL`：
 *   - context 为 NULL
 *   - context 为零初始化，例如 context->impl 为 `NULL`
 *
 * 如果 context 未初始化，则为未定义行为。
 *
 * 如果返回 `NULL`，则已设置错误消息。
 *
 * 选项仅供参考，因此返回的指针是 const。
 * 更改选项中的值是未定义行为，但可能不会产生任何影响。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 是
 * 无锁              | 是
 *
 * \param[in] 要从中检索初始化选项的上下文对象
 * \return 指向初始化选项的指针，或
 * \return 如果有错误，则返回 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
const rcl_init_options_t* rcl_context_get_init_options(const rcl_context_t* context);

/// 返回给定上下文的唯一无符号整数，如果无效，则返回 `0`。
/**
 * 给定的上下文必须是非 `NULL` 的，但不需要初始化或有效。
 * 如果上下文是 `NULL`，则返回 `0`。
 * 如果上下文未初始化，则为未定义行为。
 *
 * 如果上下文是零初始化的，或者上下文已被 rcl_shutdown() 使无效，
 * 则实例 ID 可能为 `0`。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 是
 * 无锁              | 是 [1]
 * <i>[1] 如果 `atomic_is_lock_free()` 对于 `atomic_uint_least64_t` 返回 true</i>
 *
 * \param[in] context 从中获取实例 id 的对象
 * \return 特定于此上下文实例的唯一 id，或
 * \return `0` 如果无效，或
 * \return `0` 如果上下文是 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_context_instance_id_t rcl_context_get_instance_id(const rcl_context_t* context);

/// 返回上下文域 id。
/**
 * \pre 如果上下文未初始化，则为未定义行为。
 *
 * <hr>
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是 [1]
 * 使用原子操作      | 否
 * 无锁              | 否
 *
 * <i>[1] 异步调用此函数与 rcl_init() 或 rcl_shutdown() 可能导致函数有时成功，有时返回
 * #RCL_RET_INVALID_ARGUMENT。</i>
 *
 * \param[in] context 从中获取域 id 的上下文。
 * \param[out] domain_id 将返回域 id 的输出变量。
 * \return 如果 `context` 无效（参见 rcl_context_is_valid()），则返回 #RCL_RET_INVALID_ARGUMENT，或
 * \return 如果 `domain_id` 是 `NULL`，则返回 #RCL_RET_INVALID_ARGUMENT，或
 * \return 如果正确检索到域 id，则返回 #RCL_RET_OK。
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rcl_ret_t rcl_context_get_domain_id(rcl_context_t* context, size_t* domain_id);

/// 如果给定的上下文当前有效，则返回 `true`，否则返回 `false`。
/**
 * 如果上下文是 `NULL`，则返回 `false`。
 * 如果上下文是零初始化的，则返回 `false`。
 * 如果上下文未初始化，则为未定义行为。
 *
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 是
 * 无锁              | 是 [1]
 * <i>[1] 如果 `atomic_is_lock_free()` 对于 `atomic_uint_least64_t` 返回 true</i>
 *
 * \param[in] context 应检查其有效性的对象
 * \return 如果有效，则返回 `true`，否则返回 `false`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
bool rcl_context_is_valid(const rcl_context_t* context);

/// 如果给定的上下文当前有效，则返回指向 rmw 上下文的指针，否则返回 `NULL`。
/**
 * 如果上下文是 `NULL`，则返回 `NULL`。
 * 如果上下文是零初始化的，则返回 `NULL`。
 * 如果上下文未初始化，则为未定义行为。
 *
 * 属性              | 遵循
 * ------------------ | -------------
 * 分配内存          | 否
 * 线程安全          | 是
 * 使用原子操作      | 是
 * 无锁              | 是 [1]
 * <i>[1] 如果 `atomic_is_lock_free()` 对于 `atomic_uint_least64_t` 返回 true</i>
 *
 * \param[in] context 从中获取 rmw 上下文的对象。
 * \return 如果有效，则返回指向 rmw 上下文的指针，否则返回 `NULL`
 */
RCL_PUBLIC
RCL_WARN_UNUSED
rmw_context_t* rcl_context_get_rmw_context(rcl_context_t* context);

#ifdef __cplusplus
}
#endif

#endif  // RCL__CONTEXT_H_
