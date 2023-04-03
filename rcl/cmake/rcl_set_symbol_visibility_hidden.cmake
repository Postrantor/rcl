# Copyright 2019 Open Source Robotics Foundation, Inc.
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
# Configures ros client library with custom settings. The custom settings are
# all related to library symbol visibility, see:
# https://gcc.gnu.org/wiki/Visibility
# http://www.ibm.com/developerworks/aix/library/au-aix-symbol-visibility/
#
# Below code is heavily referenced from a similar functionality in rmw:
# https://github.com/ros2/rmw/blob/master/rmw/cmake/configure_rmw_library.cmake
#
# :param library_target: the library target :type library_target: string :param
# LANGUAGE: Optional flag for the language of the library. Allowed values are
# "C" and "CXX". The default is "CXX". :type LANGUAGE: string
#
# @public
#

# @brief 设置库目标的符号可见性为隐藏
#
# @param library_target 库目标名称 @param LANGUAGE 可选参数，指定编程语言，默认为 "CXX"
#
# @note 如果可能，将符号可见性设置为隐藏。这在 Windows 上已经是默认设置。
function(rcl_set_symbol_visibility_hidden library_target)
  # 解析传入的参数
  cmake_parse_arguments(ARG "" "LANGUAGE" "" ${ARGN})

  # 如果有未解析的参数，报错并退出
  if(ARG_UNPARSED_ARGUMENTS)
    message(
      FATAL_ERROR
        "rcl_set_symbol_visibility_hidden() called with unused arguments: ${ARG_UNPARSED_ARGUMENTS}"
    )
  endif()

  # 如果没有指定编程语言，则默认为 CXX
  if(NOT ARG_LANGUAGE)
    set(ARG_LANGUAGE "CXX")
  endif()

  # 如果编程语言为 C
  if(ARG_LANGUAGE STREQUAL "C")
    # 如果可能，将符号可见性设置为隐藏
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID MATCHES
                                             "Clang")
      # 对于 gcc 和 clang，将符号可见性默认设置为隐藏（Windows 上已经是默认设置）
      set_target_properties(${library_target} PROPERTIES COMPILE_FLAGS
                                                         "-fvisibility=hidden")
    endif()

    # 如果编程语言为 CXX
  elseif(ARG_LANGUAGE STREQUAL "CXX")
    # 如果可能，将符号可见性设置为隐藏
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES
                                               "Clang")
      # 对于 gcc 和 clang，将符号可见性默认设置为隐藏（Windows 上已经是默认设置）
      set_target_properties(
        ${library_target}
        PROPERTIES COMPILE_FLAGS
                   "-fvisibility=hidden -fvisibility-inlines-hidden")
    endif()

    # 如果编程语言不支持，报错并退出
  else()
    message(
      FATAL_ERROR
        "rcl_set_symbol_visibility_hidden() called with unsupported LANGUAGE: '${ARG_LANGUAGE}'"
    )
  endif()
endfunction()
