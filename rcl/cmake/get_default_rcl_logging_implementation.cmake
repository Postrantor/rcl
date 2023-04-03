# Copyright 2018 Open Source Robotics Foundation, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

#
# Get the package name of the default logging implementation.
#
# Either selecting it using the variable RCL_LOGGING_IMPLEMENTATION or choosing
# a default from the available implementations.
#
# :param var: the output variable name containing the package name :type var:
# string
#

# @brief 获取默认的 RCL 日志实现。
#
# 如果已经指定了日志实现或设置了 RCL_LOGGING_IMPLEMENTATION 环境变量， 则使用它们，否则默认使用
# rcl_logging_noop。
#
# @param[in] var 用于存储选定的日志实现的变量名。
macro(get_default_rcl_logging_implementation var)

  # 如果已经指定了日志实现或设置了 RCL_LOGGING_IMPLEMENTATION 环境变量，则使用它们
  if(NOT "${RCL_LOGGING_IMPLEMENTATION}" STREQUAL "")
    set(_logging_implementation "${RCL_LOGGING_IMPLEMENTATION}")
  elseif(NOT "$ENV{RCL_LOGGING_IMPLEMENTATION}" STREQUAL "")
    set(_logging_implementation "$ENV{RCL_LOGGING_IMPLEMENTATION}")
  else()
    # 否则，默认使用 rcl_logging_spdlog
    set(_logging_implementation rcl_logging_spdlog)
  endif()

  # 如果实现决策不是动态确定的，则将其持久化到缓存中
  set(RCL_LOGGING_IMPLEMENTATION
      "${_logging_implementation}"
      CACHE STRING "select rcl logging implementation to use" FORCE)

  # 查找所需的日志实现包
  find_package("${_logging_implementation}" REQUIRED)

  # 将选定的日志实现设置为传入的变量
  set(${var} ${_logging_implementation})
endmacro()
