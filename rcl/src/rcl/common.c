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

#include "./common.h"  // NOLINT

#include <stdlib.h>

#include "rcl/allocator.h"
#include "rcl/error_handling.h"

/**
 * @brief 将 rmw_ret_t 类型的返回值转换为 rcl_ret_t 类型的返回值
 *
 * @param[in] rmw_ret rmw_ret_t 类型的返回值
 * @return 对应的 rcl_ret_t 类型的返回值
 */
rcl_ret_t rcl_convert_rmw_ret_to_rcl_ret(rmw_ret_t rmw_ret) {
  // 使用 switch 语句根据输入的 rmw_ret 的值进行匹配
  switch (rmw_ret) {
    // 如果 rmw_ret 的值为 RMW_RET_OK，则返回 RCL_RET_OK
    case RMW_RET_OK:
      return RCL_RET_OK;
    // 如果 rmw_ret 的值为 RMW_RET_INVALID_ARGUMENT，则返回 RCL_RET_INVALID_ARGUMENT
    case RMW_RET_INVALID_ARGUMENT:
      return RCL_RET_INVALID_ARGUMENT;
    // 如果 rmw_ret 的值为 RMW_RET_BAD_ALLOC，则返回 RCL_RET_BAD_ALLOC
    case RMW_RET_BAD_ALLOC:
      return RCL_RET_BAD_ALLOC;
    // 如果 rmw_ret 的值为 RMW_RET_UNSUPPORTED，则返回 RCL_RET_UNSUPPORTED
    case RMW_RET_UNSUPPORTED:
      return RCL_RET_UNSUPPORTED;
    // 如果 rmw_ret 的值为 RMW_RET_NODE_NAME_NON_EXISTENT，则返回 RCL_RET_NODE_NAME_NON_EXISTENT
    case RMW_RET_NODE_NAME_NON_EXISTENT:
      return RCL_RET_NODE_NAME_NON_EXISTENT;
    // 其他情况下，默认返回 RCL_RET_ERROR
    default:
      return RCL_RET_ERROR;
  }
}

#ifdef __cplusplus
}
#endif
