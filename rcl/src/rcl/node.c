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

#include "rcl/node.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./context_impl.h"
#include "rcl/arguments.h"
#include "rcl/error_handling.h"
#include "rcl/init_options.h"
#include "rcl/localhost.h"
#include "rcl/logging.h"
#include "rcl/logging_rosout.h"
#include "rcl/rcl.h"
#include "rcl/remap.h"
#include "rcl/security.h"
#include "rcutils/env.h"
#include "rcutils/filesystem.h"
#include "rcutils/find.h"
#include "rcutils/format_string.h"
#include "rcutils/logging_macros.h"
#include "rcutils/macros.h"
#include "rcutils/repl_str.h"
#include "rcutils/snprintf.h"
#include "rcutils/strdup.h"
#include "rmw/error_handling.h"
#include "rmw/rmw.h"
#include "rmw/security_options.h"
#include "rmw/validate_namespace.h"
#include "rmw/validate_node_name.h"
#include "tracetools/tracetools.h"

// 定义一个环境变量，用于禁用借出的消息功能
// Define an environment variable to disable the loaned messages feature
const char *const RCL_DISABLE_LOANED_MESSAGES_ENV_VAR = "ROS_DISABLE_LOANED_MESSAGES";

// 定义一个 rcl_node_impl_s 结构体，包含 ROS2 节点的实现细节
// Define a structure rcl_node_impl_s containing the implementation details of a ROS2 node
struct rcl_node_impl_s {
  // 节点选项，包括分配器、域 ID 等
  rcl_node_options_t options;
  // rmw 节点句柄，用于与底层中间件通信
  rmw_node_t *rmw_node_handle;
  // 图形保护条件，用于触发图形事件（如节点、主题等的变化）
  rcl_guard_condition_t *graph_guard_condition;
  // 记录器名称，用于识别节点产生的日志
  const char *logger_name;
  // 节点的完全限定名称，包括命名空间和节点名称
  const char *fq_name;
};

/// 返回与给定验证节点名称和命名空间关联的记录器名称。
/// Return the logger name associated with a node given the validated node name and namespace.
/**
 * 例如，对于命名空间 "/a/b" 中名为 "c" 的节点，记录器名称将是 "a.b.c"，假设记录器名称分隔符为 "."。
 * E.g. for a node named "c" in namespace "/a/b", the logger name will be "a.b.c", assuming logger
 * name separator of ".".
 *
 * \param[in] node_name 已验证的节点名称（单个标记）
 * \param[in] node_namespace 已验证的绝对命名空间（以 "/" 开头）
 * \param[in] allocator 用于分配的分配器
 * \returns 复制的字符串或出错时为空
 */
const char *rcl_create_node_logger_name(
    const char *node_name, const char *node_namespace, const rcl_allocator_t *allocator) {
  // 如果命名空间是根命名空间 ("/")，则记录器名称只是节点名称。
  // If the namespace is the root namespace ("/"), the logger name is just the node name.
  if (strlen(node_namespace) == 1) {
    return rcutils_strdup(node_name, *allocator);
  }

  // 将命名空间中的正斜杠转换为记录器名称使用的分隔符。
  // Convert the forward slashes in the namespace to the separator used for logger names.
  // 输入命名空间已经扩展，因此始终是绝对的，即它将以正斜杠开头，我们希望忽略它。
  // The input namespace has already been expanded and therefore will always be absolute,
  // i.e. it will start with a forward slash, which we want to ignore.
  const char *ns_with_separators = rcutils_repl_str(
      node_namespace + 1,  // 忽略前导正斜杠。
      "/", RCUTILS_LOGGING_SEPARATOR_STRING, allocator);
  if (NULL == ns_with_separators) {
    return NULL;
  }

  // 将命名空间和节点名称连接起来以创建记录器名称。
  // Join the namespace and node name to create the logger name.
  char *node_logger_name = rcutils_format_string(
      *allocator, "%s%s%s", ns_with_separators, RCUTILS_LOGGING_SEPARATOR_STRING, node_name);
  allocator->deallocate((char *)ns_with_separators, allocator->state);
  return node_logger_name;
}

/** @brief 获取一个零初始化的 rcl_node_t 结构体 (Get a zero-initialized rcl_node_t structure)
 */
rcl_node_t rcl_get_zero_initialized_node() {
  // 定义一个静态的零初始化的 rcl_node_t 结构体变量 null_node
  // (Define a static zero-initialized rcl_node_t structure variable null_node)
  static rcl_node_t null_node = {.context = 0, .impl = 0};
  // 返回零初始化的 rcl_node_t 结构体变量 null_node
  // (Return the zero-initialized rcl_node_t structure variable null_node)
  return null_node;
}

/** @brief 初始化节点 (Initialize a node)
 *
 * @param[in,out] node 要初始化的节点指针 (Pointer to the node to be initialized)
 * @param[in] name 节点名称 (Name of the node)
 * @param[in] namespace_ 节点命名空间 (Namespace of the node)
 * @param[in] context 节点上下文 (Context of the node)
 * @param[in] options 节点选项 (Node options)
 * @return rcl_ret_t 返回 RCL_RET_OK 表示成功，其他表示失败 (Return RCL_RET_OK for success, others
 * for failure)
 */
rcl_ret_t rcl_node_init(
    rcl_node_t *node,
    const char *name,
    const char *namespace_,
    rcl_context_t *context,
    const rcl_node_options_t *options) {
  // 定义 rmw 图形保护条件变量 (Define the rmw graph guard condition variable)
  const rmw_guard_condition_t *rmw_graph_guard_condition = NULL;
  // 获取图形保护条件的默认选项 (Get default options for the graph guard condition)
  rcl_guard_condition_options_t graph_guard_condition_options =
      rcl_guard_condition_get_default_options();
  // 定义返回值变量 (Define return value variable)
  rcl_ret_t ret;
  // 定义错误返回值 (Define error return value)
  rcl_ret_t fail_ret = RCL_RET_ERROR;
  // 定义重映射节点名称变量 (Define remapped node name variable)
  char *remapped_node_name = NULL;

  // 检查选项和分配器，以便在出错时使用分配器
  RCL_CHECK_ARGUMENT_FOR_NULL(options, RCL_RET_INVALID_ARGUMENT);
  const rcl_allocator_t *allocator = &options->allocator;
  RCL_CHECK_ALLOCATOR_WITH_MSG(allocator, "invalid allocator", return RCL_RET_INVALID_ARGUMENT);

  // 检查参数是否为空 (Check if name, namespace and node arguments are NULL)
  RCL_CHECK_ARGUMENT_FOR_NULL(name, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(namespace_, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(node, RCL_RET_INVALID_ARGUMENT);
  // 输出调试信息，表示正在初始化节点 (Output debug information indicating that the node is being
  // initialized)
  RCUTILS_LOG_DEBUG_NAMED(
      ROS_PACKAGE_NAME, "Initializing node '%s' in namespace '%s'", name, namespace_);
  // 如果节点已经初始化，则返回错误信息 (If the node is already initialized, return an error
  // message)
  if (node->impl) {
    RCL_SET_ERROR_MSG("node already initialized, or struct memory was unintialized");
    return RCL_RET_ALREADY_INIT;
  }
  // 确保 rcl 已经初始化 (Make sure rcl has been initialized)
  RCL_CHECK_FOR_NULL_WITH_MSG(
      context, "given context in options is NULL", return RCL_RET_INVALID_ARGUMENT);
  if (!rcl_context_is_valid(context)) {
    RCL_SET_ERROR_MSG(
        "the given context is not valid, "
        "either rcl_init() was not called or rcl_shutdown() was called.");
    return RCL_RET_NOT_INIT;
  }
  // 在分配内存之前确保节点名称有效 (Make sure the node name is valid before allocating memory)
  int validation_result = 0;
  ret = rmw_validate_node_name(name, &validation_result, NULL);
  if (ret != RMW_RET_OK) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    return ret;
  }
  if (validation_result != RMW_NODE_NAME_VALID) {
    const char *msg = rmw_node_name_validation_result_string(validation_result);
    RCL_SET_ERROR_MSG(msg);
    return RCL_RET_NODE_INVALID_NAME;
  }

  // 处理命名空间 (Process the namespace)
  size_t namespace_length = strlen(namespace_);
  const char *local_namespace_ = namespace_;
  bool should_free_local_namespace_ = false;
  // 如果命名空间只是一个空字符串，用 "/" 替换
  if (namespace_length == 0) {
    // Have this special case to avoid a memory allocation when "" is passed.
    local_namespace_ = "/";
  }

  // 如果命名空间不以 / 开头，添加一个 (If the namespace does not start with a /, add one)
  if (namespace_length > 0 && namespace_[0] != '/') {
    local_namespace_ = rcutils_format_string(*allocator, "/%s", namespace_);
    RCL_CHECK_FOR_NULL_WITH_MSG(local_namespace_, "failed to format node namespace string",
                                ret = RCL_RET_BAD_ALLOC;
                                goto cleanup);
    should_free_local_namespace_ = true;
  }
  // 确保节点命名空间有效 (Make sure the node namespace is valid)
  validation_result = 0;
  ret = rmw_validate_namespace(local_namespace_, &validation_result, NULL);
  if (ret != RMW_RET_OK) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    goto cleanup;
  }
  if (validation_result != RMW_NAMESPACE_VALID) {
    const char *msg = rmw_namespace_validation_result_string(validation_result);
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING("%s, result: %d", msg, validation_result);

    ret = RCL_RET_NODE_INVALID_NAMESPACE;
    goto cleanup;
  }

  // 为实现结构分配空间 (Allocate space for the implementation struct)
  node->impl = (rcl_node_impl_t *)allocator->allocate(sizeof(rcl_node_impl_t), allocator->state);
  RCL_CHECK_FOR_NULL_WITH_MSG(node->impl, "allocating memory failed", ret = RCL_RET_BAD_ALLOC;
                              goto cleanup);
  node->impl->rmw_node_handle = NULL;
  node->impl->graph_guard_condition = NULL;
  node->impl->logger_name = NULL;
  node->impl->fq_name = NULL;
  node->impl->options = rcl_node_get_default_options();
  node->context = context;
  // 初始化节点实现 (Initialize node impl)
  ret = rcl_node_options_copy(options, &(node->impl->options));
  if (RCL_RET_OK != ret) {
    goto fail;
  }

  // 如果给出了重映射规则，请重映射节点名称和命名空间 (Remap the node name and namespace if remap
  // rules are given)
  rcl_arguments_t *global_args = NULL;
  if (node->impl->options.use_global_arguments) {
    global_args = &(node->context->global_arguments);
  }
  ret = rcl_remap_node_name(
      &(node->impl->options.arguments), global_args, name, *allocator, &remapped_node_name);
  if (RCL_RET_OK != ret) {
    goto fail;
  } else if (NULL != remapped_node_name) {
    name = remapped_node_name;
  }
  char *remapped_namespace = NULL;
  ret = rcl_remap_node_namespace(
      &(node->impl->options.arguments), global_args, name, *allocator, &remapped_namespace);
  if (RCL_RET_OK != ret) {
    goto fail;
  } else if (NULL != remapped_namespace) {
    if (should_free_local_namespace_) {
      allocator->deallocate((char *)local_namespace_, allocator->state);
    }
    should_free_local_namespace_ = true;
    local_namespace_ = remapped_namespace;
  }

  // 计算节点的完全限定名称 (Compute the fully qualified name of the node)
  if ('/' == local_namespace_[strlen(local_namespace_) - 1]) {
    node->impl->fq_name = rcutils_format_string(*allocator, "%s%s", local_namespace_, name);
  } else {
    node->impl->fq_name = rcutils_format_string(*allocator, "%s/%s", local_namespace_, name);
  }

  // 节点日志记录器名称 (Node logger name)
  node->impl->logger_name = rcl_create_node_logger_name(name, local_namespace_, allocator);
  RCL_CHECK_FOR_NULL_WITH_MSG(node->impl->logger_name, "creating logger name failed", goto fail);
  // 输出调试信息，表示使用域 ID (Output debug information indicating the use of domain ID)
  RCUTILS_LOG_DEBUG_NAMED(
      ROS_PACKAGE_NAME, "Using domain ID of '%zu'", context->impl->rmw_context.actual_domain_id);

  /** 这里才是核心逻辑的开始吧！ */
  // 创建 rmw 节点句柄 (Create rmw node handle)
  node->impl->rmw_node_handle =
      rmw_create_node(&(node->context->impl->rmw_context), name, local_namespace_);
  RCL_CHECK_FOR_NULL_WITH_MSG(node->impl->rmw_node_handle, rmw_get_error_string().str, goto fail);

  // 图形保护条件 (Graph guard condition)
  rmw_graph_guard_condition = rmw_node_get_graph_guard_condition(node->impl->rmw_node_handle);
  RCL_CHECK_FOR_NULL_WITH_MSG(rmw_graph_guard_condition, rmw_get_error_string().str, goto fail);
  // 分配图形保护条件内存 (Allocate memory for the graph guard condition)
  node->impl->graph_guard_condition =
      (rcl_guard_condition_t *)allocator->allocate(sizeof(rcl_guard_condition_t), allocator->state);
  RCL_CHECK_FOR_NULL_WITH_MSG(
      node->impl->graph_guard_condition, "allocating memory failed", goto fail);
  *node->impl->graph_guard_condition = rcl_get_zero_initialized_guard_condition();
  graph_guard_condition_options.allocator = *allocator;
  // 初始化图形保护条件 (Initialize the graph guard condition)
  ret = rcl_guard_condition_init_from_rmw(
      node->impl->graph_guard_condition, rmw_graph_guard_condition, context,
      graph_guard_condition_options);
  if (ret != RCL_RET_OK) {
    // error message already set
    goto fail;
  }

  // 初始化 rosout 发布器需要节点初始化到可以创建新主题发布器的程度
  if (rcl_logging_rosout_enabled() && node->impl->options.enable_rosout) {
    // 为节点初始化 rosout 发布器
    // Initialize rosout publisher for the node
    ret = rcl_logging_rosout_init_publisher_for_node(node);
    if (ret != RCL_RET_OK) {
      // 错误信息已设置
      // Error message already set
      goto fail;
    }
  }
  // 记录调试信息，表示节点已初始化
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Node initialized");
  ret = RCL_RET_OK;
  // 添加跟踪点
  TRACEPOINT(
      rcl_node_init, (const void *)node, (const void *)rcl_node_get_rmw_handle(node),
      rcl_node_get_name(node), rcl_node_get_namespace(node));
  goto cleanup;

fail: // 失败标签
  if (node->impl) {
    if (rcl_logging_rosout_enabled() && node->impl->options.enable_rosout &&
        node->impl->logger_name) {
      // 清理节点的 rosout 发布器
      // Clean up the rosout publisher for the node
      ret = rcl_logging_rosout_fini_publisher_for_node(node);
      RCUTILS_LOG_ERROR_EXPRESSION_NAMED(
          (ret != RCL_RET_OK && ret != RCL_RET_NOT_INIT), ROS_PACKAGE_NAME,
          "Failed to fini publisher for node: %i", ret);
      allocator->deallocate((char *)node->impl->logger_name, allocator->state);
    }
    if (node->impl->fq_name) {
      allocator->deallocate((char *)node->impl->fq_name, allocator->state);
    }
    if (node->impl->rmw_node_handle) {
      // 销毁 rmw 节点
      // Destroy the rmw node
      ret = rmw_destroy_node(node->impl->rmw_node_handle);
      if (ret != RMW_RET_OK) {
        RCUTILS_LOG_ERROR_NAMED(
            ROS_PACKAGE_NAME, "failed to fini rmw node in error recovery: %s",
            rmw_get_error_string().str);
      }
    }
    if (node->impl->graph_guard_condition) {
      // 销毁保护条件
      // Destroy the guard condition
      ret = rcl_guard_condition_fini(node->impl->graph_guard_condition);
      if (ret != RCL_RET_OK) {
        RCUTILS_LOG_ERROR_NAMED(
            ROS_PACKAGE_NAME, "failed to fini guard condition in error recovery: %s",
            rcl_get_error_string().str);
      }
      allocator->deallocate(node->impl->graph_guard_condition, allocator->state);
    }
    if (NULL != node->impl->options.arguments.impl) {
      // 清理参数
      // Clean up the arguments
      ret = rcl_arguments_fini(&(node->impl->options.arguments));
      if (ret != RCL_RET_OK) {
        RCUTILS_LOG_ERROR_NAMED(
            ROS_PACKAGE_NAME, "failed to fini arguments in error recovery: %s",
            rcl_get_error_string().str);
      }
    }
    // 释放节点实现的内存
    // Deallocate memory for node implementation
    allocator->deallocate(node->impl, allocator->state);
  }
  // 将节点设置为零初始化状态
  // Set the node to zero-initialized state
  *node = rcl_get_zero_initialized_node();
  ret = fail_ret;

cleanup: // 从失败标签跳转到清理标签
  if (should_free_local_namespace_) {
    allocator->deallocate((char *)local_namespace_, allocator->state);
    local_namespace_ = NULL;
  }
  if (NULL != remapped_node_name) {
    allocator->deallocate(remapped_node_name, allocator->state);
  }
  return ret;
}

/**
 * @brief 结束节点并释放相关资源 (Finalize a node and release its resources)
 *
 * @param[in,out] node 要结束的节点指针 (Pointer to the node to be finalized)
 * @return 返回 rcl_ret_t 类型的结果，表示成功或失败 (Returns an rcl_ret_t result indicating success
 * or failure)
 */
rcl_ret_t rcl_node_fini(rcl_node_t *node) {
  // 记录调试信息 (Log debug information)
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Finalizing node");

  // 检查传入的节点是否为空 (Check if the passed node is null)
  RCL_CHECK_ARGUMENT_FOR_NULL(node, RCL_RET_NODE_INVALID);

  // 如果节点实现为空，则说明已经结束或者是零初始化的节点 (If the node implementation is null, it
  // means the node has been finalized or is zero-initialized)
  if (!node->impl) {
    // 重复调用 fini 或在零初始化的节点上调用 fini 是可以的 (Repeat calls to fini or calling fini on
    // a zero initialized node is ok)
    return RCL_RET_OK;
  }

  // 获取节点的内存分配器 (Get the allocator of the node)
  rcl_allocator_t allocator = node->impl->options.allocator;

  // 初始化结果变量 (Initialize the result variable)
  rcl_ret_t result = RCL_RET_OK;
  rcl_ret_t rcl_ret = RCL_RET_OK;

  // 如果启用了 rosout 日志记录功能，并且节点也启用了 rosout (If rosout logging is enabled and the
  // node also enables rosout)
  if (rcl_logging_rosout_enabled() && node->impl->options.enable_rosout) {
    // 结束节点的 rosout 发布器 (Finalize the rosout publisher for the node)
    rcl_ret = rcl_logging_rosout_fini_publisher_for_node(node);

    // 如果返回结果不是 RCL_RET_OK 且不是 RCL_RET_NOT_INIT，则设置错误信息并更新结果变量 (If the
    // return result is not RCL_RET_OK and not RCL_RET_NOT_INIT, set the error message and update
    // the result variable)
    if (rcl_ret != RCL_RET_OK && rcl_ret != RCL_RET_NOT_INIT) {
      RCL_SET_ERROR_MSG("Unable to fini publisher for node.");
      result = RCL_RET_ERROR;
    }
  }

  // 销毁节点的 rmw 节点句柄 (Destroy the rmw node handle of the node)
  rmw_ret_t rmw_ret = rmw_destroy_node(node->impl->rmw_node_handle);

  // 如果销毁失败，设置错误信息并更新结果变量 (If the destruction fails, set the error message and
  // update the result variable)
  if (rmw_ret != RMW_RET_OK) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    result = RCL_RET_ERROR;
  }

  // 结束节点的图形保护条件 (Finalize the graph guard condition of the node)
  rcl_ret = rcl_guard_condition_fini(node->impl->graph_guard_condition);

  // 如果结束失败，设置错误信息并更新结果变量 (If the finalization fails, set the error message and
  // update the result variable)
  if (rcl_ret != RCL_RET_OK) {
    RCL_SET_ERROR_MSG(rmw_get_error_string().str);
    result = RCL_RET_ERROR;
  }

  // 释放图形保护条件的内存 (Deallocate the memory of the graph guard condition)
  allocator.deallocate(node->impl->graph_guard_condition, allocator.state);

  // 假设分配和释放操作是正确的，因为它们在 init 中已经检查过了 (Assume allocate and deallocate
  // operations are correct since they have been checked in init)
  allocator.deallocate((char *)node->impl->logger_name, allocator.state);
  allocator.deallocate((char *)node->impl->fq_name, allocator.state);

  // 如果节点的选项参数实现不为空，则结束选项参数 (If the implementation of the node's option
  // arguments is not null, finalize the option arguments)
  if (NULL != node->impl->options.arguments.impl) {
    rcl_ret_t ret = rcl_arguments_fini(&(node->impl->options.arguments));
    if (ret != RCL_RET_OK) {
      return ret;
    }
  }

  // 释放节点实现的内存 (Deallocate the memory of the node implementation)
  allocator.deallocate(node->impl, allocator.state);

  // 将节点实现设置为 NULL (Set the node implementation to NULL)
  node->impl = NULL;

  // 记录调试信息 (Log debug information)
  RCUTILS_LOG_DEBUG_NAMED(ROS_PACKAGE_NAME, "Node finalized");

  // 返回结果 (Return the result)
  return result;
}

/**
 * @brief 检查节点是否有效，但不检查上下文 (Check if the node is valid without checking the context)
 *
 * @param[in] node 要检查的 rcl_node_t 结构指针 (Pointer to the rcl_node_t structure to be checked)
 * @return 如果节点有效，则返回 true，否则返回 false (Returns true if the node is valid, otherwise
 * returns false)
 */
bool rcl_node_is_valid_except_context(const rcl_node_t *node) {
  // 检查节点指针是否为空 (Check if the node pointer is NULL)
  RCL_CHECK_FOR_NULL_WITH_MSG(node, "rcl node pointer is invalid", return false);

  // 检查节点实现是否为空 (Check if the node implementation is NULL)
  RCL_CHECK_FOR_NULL_WITH_MSG(node->impl, "rcl node implementation is invalid", return false);

  // 检查节点的 rmw 句柄是否为空 (Check if the node's rmw handle is NULL)
  RCL_CHECK_FOR_NULL_WITH_MSG(
      node->impl->rmw_node_handle, "rcl node's rmw handle is invalid", return false);

  return true;
}

/**
 * @brief 检查节点是否有效，包括上下文 (Check if the node is valid including the context)
 *
 * @param[in] node 要检查的 rcl_node_t 结构指针 (Pointer to the rcl_node_t structure to be checked)
 * @return 如果节点有效，则返回 true，否则返回 false (Returns true if the node is valid, otherwise
 * returns false)
 */
bool rcl_node_is_valid(const rcl_node_t *node) {
  bool result = rcl_node_is_valid_except_context(node);

  // 如果节点无效，直接返回结果 (If the node is invalid, return the result directly)
  if (!result) {
    return result;
  }

  // 检查节点的上下文是否有效 (Check if the node's context is valid)
  if (!rcl_context_is_valid(node->context)) {
    RCL_SET_ERROR_MSG("rcl node's context is invalid");
    return false;
  }

  return true;
}

/**
 * @brief 获取节点的名称 (Get the name of the node)
 *
 * @param[in] node 要获取名称的 rcl_node_t 结构指针 (Pointer to the rcl_node_t structure to get the
 * name)
 * @return 返回节点的名称，如果节点无效，则返回 NULL (Returns the name of the node, or NULL if the
 * node is invalid)
 */
const char *rcl_node_get_name(const rcl_node_t *node) {
  // 检查节点是否有效，但不检查上下文 (Check if the node is valid without checking the context)
  if (!rcl_node_is_valid_except_context(node)) {
    return NULL;  // error already set
  }

  // 返回节点的名称 (Return the name of the node)
  return node->impl->rmw_node_handle->name;
}

/** \brief 获取节点的命名空间 (Get the namespace of a node)
 *  \param[in] node 指向 rcl_node_t 结构体的指针 (Pointer to an rcl_node_t structure)
 *  \return 节点的命名空间字符串 (The namespace string of the node)
 */
const char *rcl_node_get_namespace(const rcl_node_t *node) {
  // 检查节点是否有效，但不检查上下文 (Check if the node is valid, but not the context)
  if (!rcl_node_is_valid_except_context(node)) {
    return NULL;  // 错误已设置 (error already set)
  }
  // 返回节点的命名空间 (Return the node's namespace)
  return node->impl->rmw_node_handle->namespace_;
}

/** \brief 获取节点的完全限定名称 (Get the fully qualified name of a node)
 *  \param[in] node 指向 rcl_node_t 结构体的指针 (Pointer to an rcl_node_t structure)
 *  \return 节点的完全限定名称字符串 (The fully qualified name string of the node)
 */
const char *rcl_node_get_fully_qualified_name(const rcl_node_t *node) {
  // 检查节点是否有效，但不检查上下文 (Check if the node is valid, but not the context)
  if (!rcl_node_is_valid_except_context(node)) {
    return NULL;  // 错误已设置 (error already set)
  }
  // 返回节点的完全限定名称 (Return the node's fully qualified name)
  return node->impl->fq_name;
}

/** \brief 获取节点的选项 (Get the options of a node)
 *  \param[in] node 指向 rcl_node_t 结构体的指针 (Pointer to an rcl_node_t structure)
 *  \return 节点选项的指针 (Pointer to the node's options)
 */
const rcl_node_options_t *rcl_node_get_options(const rcl_node_t *node) {
  // 检查节点是否有效，但不检查上下文 (Check if the node is valid, but not the context)
  if (!rcl_node_is_valid_except_context(node)) {
    return NULL;  // 错误已设置 (error already set)
  }
  // 返回节点的选项 (Return the node's options)
  return &node->impl->options;
}

/** \brief 获取节点的域 ID (Get the domain ID of a node)
 *  \param[in] node 指向 rcl_node_t 结构体的指针 (Pointer to an rcl_node_t structure)
 *  \param[out] domain_id 存储域 ID 的指针 (Pointer to store the domain ID)
 *  \return RCL_RET_OK 或相应的错误代码 (RCL_RET_OK or corresponding error code)
 */
rcl_ret_t rcl_node_get_domain_id(const rcl_node_t *node, size_t *domain_id) {
  // 检查节点是否有效 (Check if the node is valid)
  if (!rcl_node_is_valid(node)) {
    return RCL_RET_NODE_INVALID;
  }
  // 检查 domain_id 参数是否为空 (Check if the domain_id argument is NULL)
  RCL_CHECK_ARGUMENT_FOR_NULL(domain_id, RCL_RET_INVALID_ARGUMENT);
  // 获取节点上下文的域 ID (Get the domain ID of the node's context)
  rcl_ret_t ret = rcl_context_get_domain_id(node->context, domain_id);
  // 检查返回值是否为 RCL_RET_OK (Check if the return value is RCL_RET_OK)
  if (RCL_RET_OK != ret) {
    return ret;
  }
  // 返回成功状态 (Return success status)
  return RCL_RET_OK;
}

/** \brief 获取节点的 rmw 句柄
 *
 * \param[in] node 指向 rcl_node_t 结构体的指针
 * \return 如果节点有效，则返回 rmw_node_handle，否则返回 NULL
 */
rmw_node_t *rcl_node_get_rmw_handle(const rcl_node_t *node) {
  // 检查节点是否有效，除了上下文 (Check if the node is valid, except for the context)
  if (!rcl_node_is_valid_except_context(node)) {
    return NULL;  // 错误已设置 (error already set)
  }
  // 返回节点的 rmw_node_handle
  return node->impl->rmw_node_handle;
}

/** \brief 获取节点的 rcl 实例 ID (Get the rcl instance ID of the node)
 *
 * \param[in] node 指向 rcl_node_t 结构体的指针 (Pointer to the rcl_node_t structure)
 * \return 如果节点有效，则返回实例 ID，否则返回 0 (If the node is valid, return the instance ID,
 * otherwise return 0)
 */
uint64_t rcl_node_get_rcl_instance_id(const rcl_node_t *node) {
  // 检查节点是否有效，除了上下文 (Check if the node is valid, except for the context)
  if (!rcl_node_is_valid_except_context(node)) {
    return 0;  // 错误已设置 (error already set)
  }
  // 返回节点上下文的实例 ID (Return the instance ID of the node's context)
  return rcl_context_get_instance_id(node->context);
}

/** \brief 获取节点的图形保护条件 (Get the graph guard condition of the node)
 *
 * \param[in] node 指向 rcl_node_t 结构体的指针 (Pointer to the rcl_node_t structure)
 * \return 如果节点有效，则返回图形保护条件，否则返回 NULL (If the node is valid, return the graph
 * guard condition, otherwise return NULL)
 */
const rcl_guard_condition_t *rcl_node_get_graph_guard_condition(const rcl_node_t *node) {
  // 检查节点是否有效，除了上下文 (Check if the node is valid, except for the context)
  if (!rcl_node_is_valid_except_context(node)) {
    return NULL;  // 错误已设置 (error already set)
  }
  // 返回节点的图形保护条件 (Return the graph guard condition of the node)
  return node->impl->graph_guard_condition;
}

/**
 * \brief 获取节点的日志记录器名称 (Get the logger name of the node)
 *
 * \param[in] node 指向 rcl_node_t 结构体的指针 (Pointer to the rcl_node_t structure)
 * \return 如果节点有效，则返回日志记录器名称，否则返回 NULL (If the node is valid, return the
 * logger name, otherwise return NULL)
 */
const char *rcl_node_get_logger_name(const rcl_node_t *node) {
  // 检查节点是否有效，除了上下文 (Check if the node is valid, except for the context)
  if (!rcl_node_is_valid_except_context(node)) {
    return NULL;  // 错误已设置 (error already set)
  }
  // 返回节点的日志记录器名称 (Return the logger name of the node)
  return node->impl->logger_name;
}

/**
 * @brief 获取禁用借用消息的设置 (Get the setting for disabling loaned messages)
 *
 * @param[out] disable_loaned_message 用于存储禁用借用消息设置的布尔值指针 (Pointer to a boolean
 * value to store the disable loaned message setting)
 * @return rcl_ret_t 返回操作结果 (Return operation result)
 */
rcl_ret_t rcl_get_disable_loaned_message(bool *disable_loaned_message) {
  // 定义环境变量值和错误字符串 (Define environment variable value and error string)
  const char *env_val = NULL;
  const char *env_error_str = NULL;

  // 检查输入参数是否为空 (Check if input argument is NULL)
  RCL_CHECK_ARGUMENT_FOR_NULL(disable_loaned_message, RCL_RET_INVALID_ARGUMENT);

  // 获取环境变量值 (Get the environment variable value)
  env_error_str = rcutils_get_env(RCL_DISABLE_LOANED_MESSAGES_ENV_VAR, &env_val);
  // 如果获取过程中出现错误 (If an error occurs during retrieval)
  if (NULL != env_error_str) {
    // 设置错误信息 (Set error message)
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Error getting env var: '" RCUTILS_STRINGIFY(RCL_DISABLE_LOANED_MESSAGES_ENV_VAR) "': %s\n",
        env_error_str);
    // 返回错误结果 (Return error result)
    return RCL_RET_ERROR;
  }

  // 根据环境变量值设置禁用借用消息的布尔值 (Set the boolean value for disabling loaned messages
  // based on the environment variable value)
  *disable_loaned_message = (strcmp(env_val, "1") == 0);
  // 返回操作成功 (Return operation success)
  return RCL_RET_OK;
}

#ifdef __cplusplus
}
#endif
