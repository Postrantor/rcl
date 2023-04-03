// Copyright 2018-2019 Open Source Robotics Foundation, Inc.
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

#include "rcl/logging_rosout.h"

#include "rcl/allocator.h"
#include "rcl/error_handling.h"
#include "rcl/node.h"
#include "rcl/publisher.h"
#include "rcl/time.h"
#include "rcl/types.h"
#include "rcl/visibility_control.h"
#include "rcl_interfaces/msg/log.h"
#include "rcutils/allocator.h"
#include "rcutils/format_string.h"
#include "rcutils/logging_macros.h"
#include "rcutils/macros.h"
#include "rcutils/types/hash_map.h"
#include "rcutils/types/rcutils_ret.h"
#include "rosidl_runtime_c/string_functions.h"

/**
 * @brief 将 rcutils_ret_t 类型的错误码转换为 rcl_ret_t 类型 (Converts an error code of type
 * rcutils_ret_t to rcl_ret_t)
 *
 * @param[in] rcutils_ret rcutils_ret_t 类型的错误码 (Error code of type rcutils_ret_t)
 * @return 对应的 rcl_ret_t 类型的错误码 (Corresponding error code of type rcl_ret_t)
 */
static rcl_ret_t rcl_ret_from_rcutils_ret(rcutils_ret_t rcutils_ret) {
  // 定义一个 rcl_ret_t 类型的变量，用于存储转换后的错误码
  // Define a variable of type rcl_ret_t to store the converted error code
  rcl_ret_t rcl_ret_var;

  // 如果 rcutils_ret 不等于 RCUTILS_RET_OK
  // If rcutils_ret is not equal to RCUTILS_RET_OK
  if (RCUTILS_RET_OK != rcutils_ret) {
    // 如果 rcutils 错误已设置
    // If rcutils error is set
    if (rcutils_error_is_set()) {
      // 设置 rcl 错误消息
      // Set rcl error message
      RCL_SET_ERROR_MSG(rcutils_get_error_string().str);
    } else {
      // 使用格式化字符串设置 rcl 错误消息
      // Set rcl error message with format string
      RCL_SET_ERROR_MSG_WITH_FORMAT_STRING("rcutils_ret_t code: %i", rcutils_ret);
    }
  }

  // 根据 rcutils_ret 的值进行转换
  // Convert based on the value of rcutils_ret
  switch (rcutils_ret) {
    case RCUTILS_RET_OK:
      rcl_ret_var = RCL_RET_OK;
      break;
    case RCUTILS_RET_ERROR:
      rcl_ret_var = RCL_RET_ERROR;
      break;
    case RCUTILS_RET_BAD_ALLOC:
      rcl_ret_var = RCL_RET_BAD_ALLOC;
      break;
    case RCUTILS_RET_INVALID_ARGUMENT:
      rcl_ret_var = RCL_RET_INVALID_ARGUMENT;
      break;
    case RCUTILS_RET_NOT_INITIALIZED:
      rcl_ret_var = RCL_RET_NOT_INIT;
      break;
    default:
      rcl_ret_var = RCUTILS_RET_ERROR;
  }

  // 返回转换后的 rcl_ret_t 类型的错误码
  // Return the converted error code of type rcl_ret_t
  return rcl_ret_var;
}

/**
 * @struct rosout_map_entry_t
 * @brief 结构体，用于存储 ROS2 节点和对应的发布器 (Structure for storing a ROS2 node and its
 * corresponding publisher)
 */
typedef struct rosout_map_entry_t {
  rcl_node_t *node;           ///< ROS2 节点指针 (Pointer to the ROS2 node)
  rcl_publisher_t publisher;  ///< ROS2 发布器 (ROS2 publisher)
} rosout_map_entry_t;

// 声明一个 rcutils_hash_map_t 类型的全局变量 __logger_map (Declare a global variable of type
// rcutils_hash_map_t named __logger_map)
static rcutils_hash_map_t __logger_map;
// 声明一个布尔类型的全局变量 __is_initialized，用于表示是否已初始化 (Declare a global boolean
// variable named __is_initialized, indicating whether it is initialized or not)
static bool __is_initialized = false;
// 声明一个 rcl_allocator_t 类型的全局变量 __rosout_allocator (Declare a global variable of type
// rcl_allocator_t named __rosout_allocator)

/**
 * @struct rosout_sublogger_entry_t
 * @brief 结构体，用于存储子日志记录器的名称和引用计数 (Structure for storing the name and reference
 * count of a sub-logger)
 */
typedef struct rosout_sublogger_entry_t {
  char *name;  ///< 子日志记录器的名称 (Name of the sub-logger)
  // 引用计数，当计数为 0 时移除条目 (Reference count, remove the entry when the count is 0)
  uint64_t *count;
} rosout_sublogger_entry_t;

// 声明一个 rcutils_hash_map_t 类型的全局变量 __sublogger_map (Declare a global variable of type
// rcutils_hash_map_t named __sublogger_map)
static rcutils_hash_map_t __sublogger_map;

/**
 * @brief 初始化 rosout 日志记录功能 (Initialize the rosout logging feature)
 *
 * @param[in] allocator 分配器指针，用于分配内存 (Pointer to the allocator for memory allocation)
 * @return 返回 rcl_ret_t 类型的状态码 (Return an rcl_ret_t type status code)
 */
rcl_ret_t rcl_logging_rosout_init(const rcl_allocator_t *allocator) {
  // 检查传入的分配器参数是否为空 (Check if the passed allocator argument is NULL)
  RCL_CHECK_ARGUMENT_FOR_NULL(allocator, RCL_RET_INVALID_ARGUMENT);

  // 初始化状态为 RCL_RET_OK (Initialize the status as RCL_RET_OK)
  rcl_ret_t status = RCL_RET_OK;

  // 如果已经初始化，则直接返回 RCL_RET_OK (If already initialized, return RCL_RET_OK directly)
  if (__is_initialized) {
    return RCL_RET_OK;
  }

  // 初始化 __logger_map 为空的哈希映射 (Initialize __logger_map as an empty hash map)
  __logger_map = rcutils_get_zero_initialized_hash_map();

  // 使用给定的分配器初始化哈希映射 (Initialize the hash map with the given allocator)
  status = rcl_ret_from_rcutils_ret(rcutils_hash_map_init(
      &__logger_map, 2, sizeof(const char *), sizeof(rosout_map_entry_t),
      rcutils_hash_map_string_hash_func, rcutils_hash_map_string_cmp_func, allocator));

  // 如果状态不是 RCL_RET_OK，则返回当前状态 (If the status is not RCL_RET_OK, return the current
  // status)
  if (RCL_RET_OK != status) {
    return status;
  }

  // 初始化 __sublogger_map 为空的哈希映射 (Initialize __sublogger_map as an empty hash map)
  __sublogger_map = rcutils_get_zero_initialized_hash_map();

  // 使用给定的分配器初始化哈希映射 (Initialize the hash map with the given allocator)
  status = rcl_ret_from_rcutils_ret(rcutils_hash_map_init(
      &__sublogger_map, 2, sizeof(const char *), sizeof(rosout_sublogger_entry_t),
      rcutils_hash_map_string_hash_func, rcutils_hash_map_string_cmp_func, allocator));

  // 如果状态不是 RCL_RET_OK (If the status is not RCL_RET_OK)
  if (RCL_RET_OK != status) {
    // 清理 __logger_map 并获取清理状态 (Clean up __logger_map and get the cleanup status)
    rcl_ret_t fini_status = rcl_ret_from_rcutils_ret(rcutils_hash_map_fini(&__logger_map));

    // 如果清理状态不是 RCL_RET_OK，则输出错误信息 (If the cleanup status is not RCL_RET_OK, output
    // the error message)
    if (RCL_RET_OK != fini_status) {
      RCUTILS_SAFE_FWRITE_TO_STDERR("Failed to finalize the hash map for logger: ");
      RCUTILS_SAFE_FWRITE_TO_STDERR(rcl_get_error_string().str);
      rcl_reset_error();
      RCUTILS_SAFE_FWRITE_TO_STDERR("\n");
    }

    // 返回当前状态 (Return the current status)
    return status;
  }

  // 设置 __rosout_allocator 为传入的分配器 (Set __rosout_allocator to the passed allocator)
  __rosout_allocator = *allocator;

  // 设置已初始化标志为 true (Set the initialized flag to true)
  __is_initialized = true;

  // 返回状态 (Return the status)
  return status;
}

/**
 * @brief 移除与给定节点关联的 logger 映射 (Remove the logger mapping associated with the given
 * node)
 *
 * @param[in] node 要移除的节点指针 (Pointer to the node to be removed)
 * @return rcl_ret_t 返回操作结果 (Return operation result)
 */
static rcl_ret_t _rcl_logging_rosout_remove_logger_map(rcl_node_t *node) {
  // 检查传入的节点是否为空 (Check if the passed node is NULL)
  RCL_CHECK_ARGUMENT_FOR_NULL(node, RCL_RET_INVALID_ARGUMENT);

  // 初始化状态变量 (Initialize status variable)
  rcl_ret_t status = RCL_RET_OK;
  char *previous_key = NULL;
  char *key = NULL;
  rosout_map_entry_t entry;

  // 获取下一个键值对和数据 (Get the next key-value pair and data)
  rcutils_ret_t hashmap_ret =
      rcutils_hash_map_get_next_key_and_data(&__logger_map, NULL, &key, &entry);

  // 当状态为 RCL_RET_OK 且 hashmap_ret 为 RCUTILS_RET_OK 时，继续循环 (Continue looping when the
  // status is RCL_RET_OK and hashmap_ret is RCUTILS_RET_OK)
  while (RCL_RET_OK == status && RCUTILS_RET_OK == hashmap_ret) {
    if (entry.node == node) {
      // 如果找到匹配的节点，从映射中删除它 (If a matching node is found, remove it from the
      // mapping)
      status = rcl_ret_from_rcutils_ret(rcutils_hash_map_unset(&__logger_map, &key));
      previous_key = NULL;
    } else {
      // 否则，将当前键设置为上一个键 (Otherwise, set the current key as the previous key)
      previous_key = key;
    }
    if (RCL_RET_OK == status) {
      // 获取下一个键值对和数据 (Get the next key-value pair and data)
      hashmap_ret = rcutils_hash_map_get_next_key_and_data(
          &__logger_map, previous_key ? &previous_key : NULL, &key, &entry);
    }
  }

  return RCL_RET_OK;
}

/**
 * @brief 清除 logger 映射项中的节点 (Clear the node in the logger mapping item)
 *
 * @param[in] value 要清除的映射项值指针 (Pointer to the mapping item value to be cleared)
 * @return rcl_ret_t 返回操作结果 (Return operation result)
 */
static rcl_ret_t _rcl_logging_rosout_clear_logger_map_item(void *value) {
  // 将传入的值转换为 rosout_map_entry_t 类型 (Convert the passed value to rosout_map_entry_t type)
  rosout_map_entry_t *entry = (rosout_map_entry_t *)value;

  // 销毁发布者 (Tear down publisher)
  rcl_ret_t status = rcl_publisher_fini(&entry->publisher, entry->node);

  if (RCL_RET_OK == status) {
    // 如果状态为 RCL_RET_OK，则删除使用此节点的所有条目 (If the status is RCL_RET_OK, delete all
    // entries using this node)
    status = rcl_ret_from_rcutils_ret(_rcl_logging_rosout_remove_logger_map(entry->node));
  }

  return status;
}

/**
 * @brief 清除子记录器映射项 (Clear a sublogger map item)
 *
 * @param[in] value 子记录器映射项指针 (Pointer to the sublogger map item)
 * @return rcl_ret_t 返回操作状态 (Return the operation status)
 */
static rcl_ret_t _rcl_logging_rosout_clear_sublogger_map_item(void *value) {
  // 将传入的值转换为 rosout_sublogger_entry_t 类型指针 (Convert the input value to a pointer of
  // type rosout_sublogger_entry_t)
  rosout_sublogger_entry_t *entry = (rosout_sublogger_entry_t *)value;

  // 从子记录器映射中删除该项，并获取操作状态 (Remove the item from the sublogger map and get the
  // operation status)
  rcl_ret_t status =
      rcl_ret_from_rcutils_ret(rcutils_hash_map_unset(&__sublogger_map, &entry->name));

  // 释放分配给名称和计数的内存 (Deallocate the memory allocated for name and count)
  __rosout_allocator.deallocate(entry->name, __rosout_allocator.state);
  __rosout_allocator.deallocate(entry->count, __rosout_allocator.state);

  // 返回操作状态 (Return the operation status)
  return status;
}

/**
 * @brief 清除哈希映射 (Clear the hashmap)
 *
 * @param[in] map 哈希映射指针 (Pointer to the hashmap)
 * @param[in] predicate 函数指针，用于处理哈希映射中的每个条目 (Function pointer to process each
 * entry in the hashmap)
 * @param[in] entry 哈希映射中的条目指针 (Pointer to the entry in the hashmap)
 * @return rcl_ret_t 返回操作状态 (Return the operation status)
 */
static rcl_ret_t _rcl_logging_rosout_clear_hashmap(
    rcutils_hash_map_t *map, rcl_ret_t (*predicate)(void *), void *entry) {
  // 初始化操作状态为 RCL_RET_OK (Initialize the operation status as RCL_RET_OK)
  rcl_ret_t status = RCL_RET_OK;

  // 定义一个 key 指针 (Define a key pointer)
  char *key = NULL;

  // 获取哈希映射中的下一个键和数据，并将其存储在 key 和 entry 中 (Get the next key and data in the
  // hashmap and store them in key and entry)
  rcutils_ret_t hashmap_ret = rcutils_hash_map_get_next_key_and_data(map, NULL, &key, entry);

  // 遍历哈希映射中的所有条目 (Iterate through all entries in the hashmap)
  while (RCUTILS_RET_OK == hashmap_ret) {
    // 使用 predicate 函数处理每个条目，并获取操作状态 (Process each entry using the predicate
    // function and get the operation status)
    status = predicate(entry);

    // 如果操作状态不是 RCL_RET_OK，则跳出循环 (If the operation status is not RCL_RET_OK, break the
    // loop)
    if (RCL_RET_OK != status) {
      break;
    }

    // 继续获取哈希映射中的下一个键和数据 (Continue getting the next key and data in the hashmap)
    hashmap_ret = rcutils_hash_map_get_next_key_and_data(map, NULL, &key, entry);
  }

  // 如果 hashmap_ret 不是 RCUTILS_RET_HASH_MAP_NO_MORE_ENTRIES，则更新操作状态 (If hashmap_ret is
  // not RCUTILS_RET_HASH_MAP_NO_MORE_ENTRIES, update the operation status)
  if (RCUTILS_RET_HASH_MAP_NO_MORE_ENTRIES != hashmap_ret) {
    status = rcl_ret_from_rcutils_ret(hashmap_ret);
  }

  // 如果操作状态为 RCL_RET_OK，则清除哈希映射 (If the operation status is RCL_RET_OK, clear the
  // hashmap)
  if (RCL_RET_OK == status) {
    status = rcl_ret_from_rcutils_ret(rcutils_hash_map_fini(map));
  }

  // 返回操作状态 (Return the operation status)
  return status;
}

/**
 * @brief 结束 rosout 日志记录功能 (Finalize the rosout logging functionality)
 *
 * @return rcl_ret_t 返回操作状态 (Return the operation status)
 */
rcl_ret_t rcl_logging_rosout_fini() {
  // 如果未初始化，则直接返回 RCL_RET_OK (If not initialized, return RCL_RET_OK directly)
  if (!__is_initialized) {
    return RCL_RET_OK;
  }
  rcl_ret_t status = RCL_RET_OK;
  rosout_map_entry_t entry;
  rosout_sublogger_entry_t sublogger_entry;

  // 清除 logger_map，使用 _rcl_logging_rosout_clear_logger_map_item 函数处理每个条目 (Clear the
  // logger_map, using _rcl_logging_rosout_clear_logger_map_item function to process each entry)
  status = _rcl_logging_rosout_clear_hashmap(
      &__logger_map, _rcl_logging_rosout_clear_logger_map_item, &entry);
  // 如果状态不是 RCL_RET_OK，则返回错误状态 (If the status is not RCL_RET_OK, return the error
  // status)
  if (RCL_RET_OK != status) {
    return status;
  }

  // 清除 sublogger_map，使用 _rcl_logging_rosout_clear_sublogger_map_item 函数处理每个条目 (Clear
  // the sublogger_map, using _rcl_logging_rosout_clear_sublogger_map_item function to process each
  // entry)
  status = _rcl_logging_rosout_clear_hashmap(
      &__sublogger_map, _rcl_logging_rosout_clear_sublogger_map_item, &sublogger_entry);
  // 如果状态不是 RCL_RET_OK，则返回错误状态 (If the status is not RCL_RET_OK, return the error
  // status)
  if (RCL_RET_OK != status) {
    return status;
  }

  // 设置 __is_initialized 为 false，表示已结束 (Set __is_initialized to false, indicating that it
  // has ended)
  __is_initialized = false;

  // 返回操作状态 (Return the operation status)
  return status;
}

/**
 * @brief 初始化节点的 rosout 发布器 (Initialize the rosout publisher for a node)
 *
 * @param[in] node 要初始化其 rosout 发布器的节点指针 (Pointer to the node to initialize its rosout
 * publisher)
 * @return rcl_ret_t 返回 RCL_RET_OK 或相应的错误代码 (Returns RCL_RET_OK or an appropriate error
 * code)
 */
rcl_ret_t rcl_logging_rosout_init_publisher_for_node(rcl_node_t *node) {
  // 如果未初始化，则直接返回 RCL_RET_OK (If not initialized, return RCL_RET_OK directly)
  if (!__is_initialized) {
    return RCL_RET_OK;
  }

  const char *logger_name = NULL;
  rosout_map_entry_t new_entry;
  rcl_ret_t status = RCL_RET_OK;

  // 验证输入并确保尚未初始化 (Verify input and make sure it's not already initialized)
  RCL_CHECK_ARGUMENT_FOR_NULL(node, RCL_RET_NODE_INVALID);
  logger_name = rcl_node_get_logger_name(node);
  if (NULL == logger_name) {
    RCL_SET_ERROR_MSG("Logger name was null.");
    return RCL_RET_ERROR;
  }
  if (rcutils_hash_map_key_exists(&__logger_map, &logger_name)) {
    // 根据此处的结果更新行为，以强制使用唯一名称或使用非唯一名称 (Update behavior to either enforce
    // unique names or work with non-unique names based on the outcome here)
    // https://github.com/ros2/design/issues/187
    RCUTILS_LOG_WARN_NAMED(
        "rcl.logging_rosout",
        "Publisher already registered for provided node name. If this is due to multiple nodes "
        "with the same name then all logs for that logger name will go out over the existing "
        "publisher. As soon as any node with that name is destructed it will unregister the "
        "publisher, preventing any further logs for that name from being published on the rosout "
        "topic.");
    return RCL_RET_OK;
  }

  // 在节点上创建一个新的 Log 消息发布器 (Create a new Log message publisher on the node)
  const rosidl_message_type_support_t *type_support =
      rosidl_typesupport_c__get_message_type_support_handle__rcl_interfaces__msg__Log();
  rcl_publisher_options_t options = rcl_publisher_get_default_options();

  // Late joining subscriptions get the user's setting of rosout qos options.
  const rcl_node_options_t *node_options = rcl_node_get_options(node);
  RCL_CHECK_FOR_NULL_WITH_MSG(node_options, "Node options was null.", return RCL_RET_ERROR);

  options.qos = node_options->rosout_qos;
  options.allocator = node_options->allocator;
  new_entry.publisher = rcl_get_zero_initialized_publisher();
  status =
      rcl_publisher_init(&new_entry.publisher, node, type_support, ROSOUT_TOPIC_NAME, &options);

  // 将新发布器添加到映射中 (Add the new publisher to the map)
  if (RCL_RET_OK == status) {
    new_entry.node = node;
    status =
        rcl_ret_from_rcutils_ret(rcutils_hash_map_set(&__logger_map, &logger_name, &new_entry));
    if (RCL_RET_OK != status) {
      RCL_SET_ERROR_MSG("Failed to add publisher to map.");
      // 我们未能将其添加到映射中，因此销毁我们创建的发布器 (We failed to add to the map so destroy
      // the publisher that we created)
      rcl_ret_t fini_status = rcl_publisher_fini(&new_entry.publisher, new_entry.node);
      // 忽略返回状态，以支持 set 的失败 (ignore the return status in favor of the failure from set)
      RCL_UNUSED(fini_status);
    }
  }

  return status;
}

/**
 * @brief 终止节点的 rosout 发布器
 * @param[in] node 要终止其 rosout 发布器的节点指针
 * @return rcl_ret_t 返回 RCL_RET_OK 或相应的错误代码
 *
 * Terminate the rosout publisher for a given node.
 * @param[in] node Pointer to the node whose rosout publisher is to be terminated.
 * @return rcl_ret_t Returns RCL_RET_OK or an appropriate error code.
 */
rcl_ret_t rcl_logging_rosout_fini_publisher_for_node(rcl_node_t *node) {
  // 检查是否已初始化，如果没有则直接返回 RCL_RET_OK
  // Check if already initialized, if not return RCL_RET_OK directly
  if (!__is_initialized) {
    return RCL_RET_OK;
  }

  rosout_map_entry_t entry;
  const char *logger_name = NULL;
  rcl_ret_t status = RCL_RET_OK;

  // 验证输入并确保已初始化
  // Verify input and make sure it's initialized
  RCL_CHECK_ARGUMENT_FOR_NULL(node, RCL_RET_NODE_INVALID);
  logger_name = rcl_node_get_logger_name(node);
  if (NULL == logger_name) {
    return RCL_RET_ERROR;
  }
  if (!rcutils_hash_map_key_exists(&__logger_map, &logger_name)) {
    return RCL_RET_OK;
  }

  // 终止发布器并从映射中删除条目
  // Finalize the publisher and remove the entry from the map
  status = rcl_ret_from_rcutils_ret(rcutils_hash_map_get(&__logger_map, &logger_name, &entry));
  if (RCL_RET_OK == status && node == entry.node) {
    status = rcl_publisher_fini(&entry.publisher, entry.node);
  }
  if (RCL_RET_OK == status) {
    // 删除使用此节点的所有条目
    // Delete all entries using this node
    status = rcl_ret_from_rcutils_ret(_rcl_logging_rosout_remove_logger_map(entry.node));
  }

  return status;
}

/**
 * @brief ROS2 rcl 日志输出处理函数 (ROS2 rcl logging output handler function)
 *
 * @param[in] location 日志位置（包括文件名、行号和函数名）(Log location, including file name, line
 * number and function name)
 * @param[in] severity 日志级别 (Log severity level)
 * @param[in] name 记录器名称 (Logger name)
 * @param[in] timestamp 日志时间戳 (Log timestamp)
 * @param[in] format 日志格式 (Log format)
 * @param[in] args 可变参数列表 (Variable argument list)
 */
void rcl_logging_rosout_output_handler(
    const rcutils_log_location_t *location,
    int severity,
    const char *name,
    rcutils_time_point_value_t timestamp,
    const char *format,
    va_list *args) {
  rosout_map_entry_t entry;
  rcl_ret_t status = RCL_RET_OK;

  // 检查是否已初始化 (Check if initialized)
  if (!__is_initialized) {
    return;
  }

  // 从哈希映射中获取记录器 (Get logger from hash map)
  rcutils_ret_t rcutils_ret = rcutils_hash_map_get(&__logger_map, &name, &entry);
  if (RCUTILS_RET_OK == rcutils_ret) {
    char msg_buf[1024] = "";
    rcutils_char_array_t msg_array = {
        .buffer = msg_buf,
        .owns_buffer = false,
        .buffer_length = 0u,
        .buffer_capacity = sizeof(msg_buf),
        .allocator = __rosout_allocator};

    // 格式化日志字符串 (Format log string)
    status = rcl_ret_from_rcutils_ret(rcutils_char_array_vsprintf(&msg_array, format, *args));
    if (RCL_RET_OK != status) {
      RCUTILS_SAFE_FWRITE_TO_STDERR("Failed to format log string: ");
      RCUTILS_SAFE_FWRITE_TO_STDERR(rcl_get_error_string().str);
      rcl_reset_error();
      RCUTILS_SAFE_FWRITE_TO_STDERR("\n");
    } else {
      // 创建 Log 消息 (Create Log message)
      rcl_interfaces__msg__Log *log_message = rcl_interfaces__msg__Log__create();
      if (NULL != log_message) {
        // 设置 Log 消息属性 (Set Log message attributes)
        log_message->stamp.sec = (int32_t)RCL_NS_TO_S(timestamp);
        log_message->stamp.nanosec = (timestamp % RCL_S_TO_NS(1));
        log_message->level = severity;
        log_message->line = (int32_t)location->line_number;
        rosidl_runtime_c__String__assign(&log_message->name, name);
        rosidl_runtime_c__String__assign(&log_message->msg, msg_array.buffer);
        rosidl_runtime_c__String__assign(&log_message->file, location->file_name);
        rosidl_runtime_c__String__assign(&log_message->function, location->function_name);

        // 发布 Log 消息到 rosout (Publish Log message to rosout)
        status = rcl_publish(&entry.publisher, log_message, NULL);
        if (RCL_RET_OK != status) {
          RCUTILS_SAFE_FWRITE_TO_STDERR("Failed to publish log message to rosout: ");
          RCUTILS_SAFE_FWRITE_TO_STDERR(rcl_get_error_string().str);
          rcl_reset_error();
          RCUTILS_SAFE_FWRITE_TO_STDERR("\n");
        }

        // 销毁 Log 消息 (Destroy Log message)
        rcl_interfaces__msg__Log__destroy(log_message);
      }
    }

    // 清理 msg_array (Clean up msg_array)
    status = rcl_ret_from_rcutils_ret(rcutils_char_array_fini(&msg_array));
    if (RCL_RET_OK != status) {
      RCUTILS_SAFE_FWRITE_TO_STDERR("failed to fini char_array: ");
      RCUTILS_SAFE_FWRITE_TO_STDERR(rcl_get_error_string().str);
      rcl_reset_error();
      RCUTILS_SAFE_FWRITE_TO_STDERR("\n");
    }
  }
}

/**
 * @brief 获取完整的子记录器名称 (Get the full sublogger name)
 *
 * @param[in] logger_name 主记录器名称 (Main logger name)
 * @param[in] sublogger_name 子记录器名称 (Sublogger name)
 * @param[out] full_sublogger_name 完整的子记录器名称 (Full sublogger name)
 * @return rcl_ret_t 返回操作结果 (Return operation result)
 */
static rcl_ret_t _rcl_logging_rosout_get_full_sublogger_name(
    const char *logger_name, const char *sublogger_name, char **full_sublogger_name) {
  // 检查输入参数是否为空 (Check if input arguments are NULL)
  RCL_CHECK_ARGUMENT_FOR_NULL(logger_name, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(sublogger_name, RCL_RET_INVALID_ARGUMENT);
  RCL_CHECK_ARGUMENT_FOR_NULL(full_sublogger_name, RCL_RET_INVALID_ARGUMENT);

  // 检查主记录器名称和子记录器名称是否为空字符串 (Check if main logger name and sublogger name are
  // empty strings)
  if (logger_name[0] == '\0' || sublogger_name[0] == '\0') {
    RCL_SET_ERROR_MSG("logger name or sub-logger name can't be empty.");
    return RCL_RET_INVALID_ARGUMENT;
  }

  // 格式化完整的子记录器名称 (Format the full sublogger name)
  *full_sublogger_name = rcutils_format_string(
      __rosout_allocator, "%s%s%s", logger_name, RCUTILS_LOGGING_SEPARATOR_STRING, sublogger_name);
  // 检查分配是否成功 (Check if allocation was successful)
  if (NULL == *full_sublogger_name) {
    RCL_SET_ERROR_MSG("Failed to allocate a full sublogger name.");
    return RCL_RET_BAD_ALLOC;
  }

  // 返回操作成功 (Return operation success)
  return RCL_RET_OK;
}

/**
 * @brief 添加子记录器到指定的记录器中 (Add a sublogger to the specified logger)
 *
 * @param[in] logger_name 记录器名称 (The name of the logger)
 * @param[in] sublogger_name 子记录器名称 (The name of the sublogger)
 * @return rcl_ret_t 返回状态 (Return status)
 */
rcl_ret_t rcl_logging_rosout_add_sublogger(const char *logger_name, const char *sublogger_name) {
  // 如果未初始化，则返回 RCL_RET_OK (If not initialized, return RCL_RET_OK)
  if (!__is_initialized) {
    return RCL_RET_OK;
  }

  rcl_ret_t status = RCL_RET_OK;
  char *full_sublogger_name = NULL;
  uint64_t *sublogger_count = NULL;
  rosout_map_entry_t entry;
  rosout_sublogger_entry_t sublogger_entry;

  // 获取完整的子记录器名称 (Get the full sublogger name)
  status = _rcl_logging_rosout_get_full_sublogger_name(
      logger_name, sublogger_name, &full_sublogger_name);
  if (RCL_RET_OK != status) {
    // Error already set
    return status;
  }

  // 获取记录器映射中的条目 (Get the entry in the logger map)
  rcutils_ret_t rcutils_ret = rcutils_hash_map_get(&__logger_map, &logger_name, &entry);
  if (RCUTILS_RET_OK != rcutils_ret) {
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING("The entry of logger '%s' not exist.", logger_name);
    status = RCL_RET_ERROR;
    goto cleanup;
  }

  // 检查子记录器是否已存在 (Check if the sublogger already exists)
  if (rcutils_hash_map_key_exists(&__logger_map, &full_sublogger_name)) {
    // 获取条目并增加引用计数 (Get the entry and increase the reference count)
    status = rcl_ret_from_rcutils_ret(
        rcutils_hash_map_get(&__sublogger_map, &full_sublogger_name, &sublogger_entry));
    if (RCL_RET_OK != status) {
      RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
          "Failed to get item from sublogger map for '%s'.", full_sublogger_name);
      goto cleanup;
    }
    *sublogger_entry.count += 1;
    goto cleanup;
  }

  // 将发布者添加到记录器映射中 (Add the publisher to the logger map)
  status =
      rcl_ret_from_rcutils_ret(rcutils_hash_map_set(&__logger_map, &full_sublogger_name, &entry));
  if (RCL_RET_OK != status) {
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Failed to add publisher to map for logger '%s'.", full_sublogger_name);
    goto cleanup;
  }

  // 设置子记录器条目的名称和计数 (Set the name and count of the sublogger entry)
  sublogger_entry.name = full_sublogger_name;
  sublogger_count = __rosout_allocator.allocate(sizeof(uint64_t), __rosout_allocator.state);
  if (!sublogger_count) {
    RCL_SET_ERROR_MSG("Failed to allocate memory for count of sublogger entry.");
    goto cleanup;
  }
  sublogger_entry.count = sublogger_count;
  *sublogger_entry.count = 1;

  // 将子记录器条目添加到子记录器映射中 (Add the sublogger entry to the sublogger map)
  status = rcl_ret_from_rcutils_ret(
      rcutils_hash_map_set(&__sublogger_map, &full_sublogger_name, &sublogger_entry));
  if (RCL_RET_OK != status) {
    // revert the previor set operation for __logger_map
    rcutils_ret_t rcutils_ret = rcutils_hash_map_unset(&__logger_map, &full_sublogger_name);
    if (RCUTILS_RET_OK != rcutils_ret) {
      RCUTILS_SAFE_FWRITE_TO_STDERR("failed to unset hashmap: ");
      RCUTILS_SAFE_FWRITE_TO_STDERR(rcl_get_error_string().str);
      rcl_reset_error();
      RCUTILS_SAFE_FWRITE_TO_STDERR("\n");
    }
    goto cleanup_count;
  }

  return status;

cleanup_count:
  // 清理分配的内存 (Clean up allocated memory)
  __rosout_allocator.deallocate(sublogger_count, __rosout_allocator.state);
cleanup:
  __rosout_allocator.deallocate(full_sublogger_name, __rosout_allocator.state);
  return status;
}

/**
 * @brief 移除子记录器 (Remove a sublogger)
 *
 * @param[in] logger_name 主记录器名称 (The name of the main logger)
 * @param[in] sublogger_name 子记录器名称 (The name of the sublogger to be removed)
 * @return rcl_ret_t 返回状态 (Return status)
 */
rcl_ret_t rcl_logging_rosout_remove_sublogger(const char *logger_name, const char *sublogger_name) {
  // 检查是否已初始化 (Check if already initialized)
  if (!__is_initialized) {
    return RCL_RET_OK;
  }

  rcl_ret_t status = RCL_RET_OK;
  char *full_sublogger_name = NULL;
  rosout_sublogger_entry_t sublogger_entry;

  // 获取完整的子记录器名称 (Get the full sublogger name)
  status = _rcl_logging_rosout_get_full_sublogger_name(
      logger_name, sublogger_name, &full_sublogger_name);
  if (RCL_RET_OK != status) {
    // 错误已设置 (Error already set)
    return status;
  }

  // 从映射中移除条目 (Remove the entry from the map)
  if (!rcutils_hash_map_key_exists(&__logger_map, &full_sublogger_name)) {
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING("Sub-logger '%s' not exist.", full_sublogger_name);
    status = RCL_RET_ERROR;
    goto cleanup;
  }

  // 获取子记录器条目 (Get the sublogger entry)
  status = rcl_ret_from_rcutils_ret(
      rcutils_hash_map_get(&__sublogger_map, &full_sublogger_name, &sublogger_entry));
  if (RCL_RET_OK != status) {
    RCL_SET_ERROR_MSG_WITH_FORMAT_STRING(
        "Failed to get item from sublogger map for '%s'.", full_sublogger_name);
    goto cleanup;
  }

  // 减少子记录器计数 (Decrease the sublogger count)
  *sublogger_entry.count -= 1;
  if (*sublogger_entry.count == 0) {
    // 当计数为0时，从映射中删除子记录器 (Remove the sublogger from the map when the count is 0)
    status = rcl_ret_from_rcutils_ret(rcutils_hash_map_unset(&__logger_map, &full_sublogger_name));
    if (RCL_RET_OK == status) {
      status =
          rcl_ret_from_rcutils_ret(rcutils_hash_map_unset(&__sublogger_map, &full_sublogger_name));
      // 释放内存 (Deallocate memory)
      __rosout_allocator.deallocate(sublogger_entry.name, __rosout_allocator.state);
      __rosout_allocator.deallocate(sublogger_entry.count, __rosout_allocator.state);
    }
  }

cleanup:
  // 清理并释放内存 (Clean up and deallocate memory)
  __rosout_allocator.deallocate(full_sublogger_name, __rosout_allocator.state);
  return status;
}
