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

#ifndef IMPL__YAML_VARIANT_H_
#define IMPL__YAML_VARIANT_H_

#include "./types.h"
#include "rcl_yaml_param_parser/types.h"
#include "rcl_yaml_param_parser/visibility_control.h"
#include "rcutils/allocator.h"
#include "rcutils/macros.h"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Finalize一个rcl_yaml_variant_t
///
/// 该函数用于释放rcl_variant_t结构体中的内存资源。
///
/// \param[in] param_var 指向需要释放资源的rcl_variant_t结构体指针
/// \param[in] allocator 用于释放内存的rcutils_allocator_t实例
RCL_YAML_PARAM_PARSER_PUBLIC
void rcl_yaml_variant_fini(rcl_variant_t * param_var, const rcutils_allocator_t allocator);

/// \brief 复制一个yaml_variant_t从param_var到out_param_var
///
/// 该函数用于将一个rcl_variant_t结构体的内容复制到另一个rcl_variant_t结构体中。
///
/// \param[out] out_param_var 指向目标rcl_variant_t结构体的指针，用于存储复制的数据
/// \param[in] param_var 指向源rcl_variant_t结构体的指针，包含要复制的数据
/// \param[in] allocator 用于分配内存的rcutils_allocator_t实例
/// \return 如果复制成功，则返回true；否则返回false
RCL_YAML_PARAM_PARSER_PUBLIC
RCUTILS_WARN_UNUSED
bool rcl_yaml_variant_copy(
  rcl_variant_t * out_param_var, const rcl_variant_t * param_var, rcutils_allocator_t allocator);

#ifdef __cplusplus
}
#endif

#endif  // IMPL__YAML_VARIANT_H_
