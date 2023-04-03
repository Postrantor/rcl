// Copyright 2018 Apex.AI, Inc.
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

#include "./impl/namespace.h"

#include <string.h>

#include "./impl/types.h"
#include "rcutils/allocator.h"
#include "rcutils/error_handling.h"
#include "rcutils/strdup.h"
#include "rcutils/types/rcutils_ret.h"

/**
 * @brief 向命名空间跟踪器中添加名称
 *
 * @param[in] ns_tracker 命名空间跟踪器指针
 * @param[in] name 要添加的名称字符串
 * @param[in] namespace_type 命名空间类型（节点或参数）
 * @param[in] allocator 分配器用于分配内存
 * @return rcutils_ret_t 返回操作结果
 */
rcutils_ret_t add_name_to_ns(
  namespace_tracker_t * ns_tracker, const char * name, const namespace_type_t namespace_type,
  rcutils_allocator_t allocator)
{
  // 当前命名空间字符串
  char * cur_ns;
  // 当前命名空间计数
  uint32_t * cur_count;
  // 分隔符字符串
  char * sep_str;
  // 名称长度
  size_t name_len;
  // 命名空间长度
  size_t ns_len;
  // 分隔符长度
  size_t sep_len;
  // 总长度
  size_t tot_len;

  // 根据命名空间类型设置变量值
  switch (namespace_type) {
    case NS_TYPE_NODE:
      cur_ns = ns_tracker->node_ns;
      cur_count = &(ns_tracker->num_node_ns);
      sep_str = NODE_NS_SEPERATOR;
      break;
    case NS_TYPE_PARAM:
      cur_ns = ns_tracker->parameter_ns;
      cur_count = &(ns_tracker->num_parameter_ns);
      sep_str = PARAMETER_NS_SEPERATOR;
      break;
    default:
      return RCUTILS_RET_ERROR;
  }

  // 添加名称到命名空间
  if (NULL == name) {
    return RCUTILS_RET_INVALID_ARGUMENT;
  }
  if (0U == *cur_count) {
    cur_ns = rcutils_strdup(name, allocator);
    if (NULL == cur_ns) {
      return RCUTILS_RET_BAD_ALLOC;
    }
  } else {
    ns_len = strlen(cur_ns);
    name_len = strlen(name);
    sep_len = strlen(sep_str);
    // 检查当前命名空间的最后几个字符是否与分隔符字符串相同
    if (strcmp(cur_ns + ns_len - sep_len, sep_str) == 0) {
      // 当前命名空间已经以分隔符结尾：不要再添加分隔符
      sep_len = 0;
      sep_str = "";
    }

    tot_len = ns_len + sep_len + name_len + 1U;

    char * tmp_ns_ptr = allocator.reallocate(cur_ns, tot_len, allocator.state);
    if (NULL == tmp_ns_ptr) {
      return RCUTILS_RET_BAD_ALLOC;
    }
    cur_ns = tmp_ns_ptr;
    memcpy((cur_ns + ns_len), sep_str, sep_len);
    memcpy((cur_ns + ns_len + sep_len), name, name_len);
    cur_ns[tot_len - 1U] = '\0';
  }
  *cur_count = (*cur_count + 1U);

  if (NS_TYPE_NODE == namespace_type) {
    ns_tracker->node_ns = cur_ns;
  } else {
    ns_tracker->parameter_ns = cur_ns;
  }
  return RCUTILS_RET_OK;
}

/**
 * @brief 从命名空间中移除名称
 *
 * @param[in] ns_tracker 命名空间跟踪器指针
 * @param[in] namespace_type 命名空间类型（节点或参数）
 * @param[in] allocator 分配器
 * @return rcutils_ret_t 返回操作结果
 */
rcutils_ret_t rem_name_from_ns(
  namespace_tracker_t * ns_tracker, const namespace_type_t namespace_type,
  rcutils_allocator_t allocator)
{
  // 当前命名空间字符串
  char * cur_ns;
  // 当前命名空间计数
  uint32_t * cur_count;
  // 分隔符字符串
  char * sep_str;
  // 命名空间长度
  size_t ns_len;
  // 总长度
  size_t tot_len;

  // 根据命名空间类型设置变量值
  switch (namespace_type) {
    case NS_TYPE_NODE:
      cur_ns = ns_tracker->node_ns;
      cur_count = &(ns_tracker->num_node_ns);
      sep_str = NODE_NS_SEPERATOR;
      break;
    case NS_TYPE_PARAM:
      cur_ns = ns_tracker->parameter_ns;
      cur_count = &(ns_tracker->num_parameter_ns);
      sep_str = PARAMETER_NS_SEPERATOR;
      break;
    default:
      return RCUTILS_RET_ERROR;
  }

  // 从命名空间中移除最后一个名称
  if (*cur_count > 0U) {
    if (1U == *cur_count) {
      allocator.deallocate(cur_ns, allocator.state);
      cur_ns = NULL;
    } else {
      ns_len = strlen(cur_ns);
      char * last_idx = NULL;
      char * next_str = NULL;
      const char * end_ptr = (cur_ns + ns_len);

      // 查找最后一个分隔符
      next_str = strstr(cur_ns, sep_str);
      while (NULL != next_str) {
        if (next_str > end_ptr) {
          RCUTILS_SET_ERROR_MSG("Internal error. Crossing array boundary");
          return RCUTILS_RET_ERROR;
        }
        last_idx = next_str;
        next_str = (next_str + strlen(sep_str));
        next_str = strstr(next_str, sep_str);
      }
      // 移除最后一个名称
      if (NULL != last_idx) {
        tot_len = ((size_t)(last_idx - cur_ns) + 1U);
        char * tmp_ns_ptr = allocator.reallocate(cur_ns, tot_len, allocator.state);
        if (NULL == tmp_ns_ptr) {
          return RCUTILS_RET_BAD_ALLOC;
        }
        cur_ns = tmp_ns_ptr;
        cur_ns[tot_len - 1U] = '\0';
      }
    }
    *cur_count = (*cur_count - 1U);
  }
  // 更新命名空间跟踪器中的命名空间字符串
  if (NS_TYPE_NODE == namespace_type) {
    ns_tracker->node_ns = cur_ns;
  } else {
    ns_tracker->parameter_ns = cur_ns;
  }
  return RCUTILS_RET_OK;
}

/**
 * @brief 替换命名空间
 *
 * @param[in] ns_tracker 命名空间跟踪器指针
 * @param[in] new_ns 新的命名空间字符串
 * @param[in] new_ns_count 新的命名空间计数
 * @param[in] namespace_type 命名空间类型（节点或参数）
 * @param[in] allocator 分配器
 * @return rcutils_ret_t 返回操作结果
 */
rcutils_ret_t replace_ns(
  namespace_tracker_t * ns_tracker, char * const new_ns, const uint32_t new_ns_count,
  const namespace_type_t namespace_type, rcutils_allocator_t allocator)
{
  // 初始化返回值为成功
  rcutils_ret_t res = RCUTILS_RET_OK;

  // 根据命名空间类型移除旧的命名空间并指向新的命名空间
  switch (namespace_type) {
    case NS_TYPE_NODE:
      // 如果节点命名空间不为空，则释放内存
      if (NULL != ns_tracker->node_ns) {
        allocator.deallocate(ns_tracker->node_ns, allocator.state);
      }
      // 复制新的命名空间到节点命名空间
      ns_tracker->node_ns = rcutils_strdup(new_ns, allocator);
      // 如果复制失败，返回内存分配错误
      if (NULL == ns_tracker->node_ns) {
        return RCUTILS_RET_BAD_ALLOC;
      }
      // 更新节点命名空间计数
      ns_tracker->num_node_ns = new_ns_count;
      break;
    case NS_TYPE_PARAM:
      // 如果参数命名空间不为空，则释放内存
      if (NULL != ns_tracker->parameter_ns) {
        allocator.deallocate(ns_tracker->parameter_ns, allocator.state);
      }
      // 复制新的命名空间到参数命名空间
      ns_tracker->parameter_ns = rcutils_strdup(new_ns, allocator);
      // 如果复制失败，返回内存分配错误
      if (NULL == ns_tracker->parameter_ns) {
        return RCUTILS_RET_BAD_ALLOC;
      }
      // 更新参数命名空间计数
      ns_tracker->num_parameter_ns = new_ns_count;
      break;
    default:
      // 如果命名空间类型不匹配，返回错误
      res = RCUTILS_RET_ERROR;
      break;
  }
  // 返回操作结果
  return res;
}
