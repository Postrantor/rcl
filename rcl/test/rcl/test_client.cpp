// Copyright 2016 Open Source Robotics Foundation, Inc.
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

#include <gtest/gtest.h>

#include "./failing_allocator_functions.hpp"
#include "osrf_testing_tools_cpp/scope_exit.hpp"
#include "rcl/client.h"
#include "rcl/error_handling.h"
#include "rcl/rcl.h"
#include "rcutils/testing/fault_injection.h"
#include "test_msgs/srv/basic_types.h"

/**
 * @class TestClientFixture
 * @brief 测试客户端夹具类 (Test client fixture class)
 */
class TestClientFixture : public ::testing::Test
{
public:
  rcl_context_t * context_ptr;  ///< 上下文指针 (Context pointer)
  rcl_node_t * node_ptr;        ///< 节点指针 (Node pointer)

  /**
   * @brief 设置测试环境 (Set up the test environment)
   */
  void SetUp()
  {
    rcl_ret_t ret;

    // 初始化选项 (Initialize options)
    {
      rcl_init_options_t init_options = rcl_get_zero_initialized_init_options();
      ret = rcl_init_options_init(&init_options, rcl_get_default_allocator());
      ASSERT_EQ(RCL_RET_OK, ret) << rcl_get_error_string().str;

      // 清理初始化选项 (Clean up initialization options)
      OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT({
        EXPECT_EQ(RCL_RET_OK, rcl_init_options_fini(&init_options)) << rcl_get_error_string().str;
      });

      this->context_ptr = new rcl_context_t;
      *this->context_ptr = rcl_get_zero_initialized_context();

      // 初始化上下文 (Initialize context)
      ret = rcl_init(0, nullptr, &init_options, this->context_ptr);
      ASSERT_EQ(RCL_RET_OK, ret) << rcl_get_error_string().str;
    }

    this->node_ptr = new rcl_node_t;
    *this->node_ptr = rcl_get_zero_initialized_node();
    const char * name = "test_client_node";
    rcl_node_options_t node_options = rcl_node_get_default_options();

    // 初始化节点 (Initialize node)
    ret = rcl_node_init(this->node_ptr, name, "", this->context_ptr, &node_options);
    ASSERT_EQ(RCL_RET_OK, ret) << rcl_get_error_string().str;
  }

  /**
   * @brief 拆除测试环境 (Tear down the test environment)
   */
  void TearDown()
  {
    // 清理节点 (Clean up node)
    rcl_ret_t ret = rcl_node_fini(this->node_ptr);
    delete this->node_ptr;
    EXPECT_EQ(RCL_RET_OK, ret) << rcl_get_error_string().str;

    // 关闭上下文 (Shutdown context)
    ret = rcl_shutdown(this->context_ptr);
    EXPECT_EQ(ret, RCL_RET_OK);

    // 清理上下文 (Clean up context)
    ret = rcl_context_fini(this->context_ptr);
    EXPECT_EQ(ret, RCL_RET_OK);
    delete this->context_ptr;
    EXPECT_EQ(RCL_RET_OK, ret) << rcl_get_error_string().str;
  }
};

/**
 * @brief 基本的客户端测试。完整功能在 test_service.cpp 中进行测试。
 *        Basic nominal test of a client. Complete functionality tested at test_service.cpp.
 */
TEST_F(TestClientFixture, test_client_nominal)
{
  // 定义返回值变量
  // Define the return value variable
  rcl_ret_t ret;

  // 初始化一个空的客户端
  // Initialize an empty client
  rcl_client_t client = rcl_get_zero_initialized_client();

  // 初始化客户端
  // Initialize the client
  const char * topic_name = "add_two_ints";
  const char * expected_topic_name = "/add_two_ints";
  rcl_client_options_t client_options = rcl_client_get_default_options();

  // 获取服务类型支持
  // Get service type support
  const rosidl_service_type_support_t * ts =
    ROSIDL_GET_SRV_TYPE_SUPPORT(test_msgs, srv, BasicTypes);
  ret = rcl_client_init(&client, this->node_ptr, ts, topic_name, &client_options);

  // 测试访问客户端选项
  // Test access to client options
  const rcl_client_options_t * client_internal_options = rcl_client_get_options(&client);
  EXPECT_TRUE(rcutils_allocator_is_valid(&(client_internal_options->allocator)));
  EXPECT_EQ(rmw_qos_profile_services_default.reliability, client_internal_options->qos.reliability);
  EXPECT_EQ(rmw_qos_profile_services_default.history, client_internal_options->qos.history);
  EXPECT_EQ(rmw_qos_profile_services_default.depth, client_internal_options->qos.depth);
  EXPECT_EQ(rmw_qos_profile_services_default.durability, client_internal_options->qos.durability);

  // 获取请求发布者的 QoS 配置
  // Get the QoS settings of the request publisher
  const rmw_qos_profile_t * request_publisher_qos =
    rcl_client_request_publisher_get_actual_qos(&client);
  EXPECT_EQ(rmw_qos_profile_services_default.reliability, request_publisher_qos->reliability);
  EXPECT_EQ(rmw_qos_profile_services_default.history, request_publisher_qos->history);
  EXPECT_EQ(rmw_qos_profile_services_default.depth, request_publisher_qos->depth);
  EXPECT_EQ(rmw_qos_profile_services_default.durability, request_publisher_qos->durability);
  EXPECT_EQ(
    rmw_qos_profile_services_default.avoid_ros_namespace_conventions,
    request_publisher_qos->avoid_ros_namespace_conventions);

  // 获取响应订阅者的 QoS 配置
  // Get the QoS settings of the response subscriber
  const rmw_qos_profile_t * response_subscription_qos =
    rcl_client_response_subscription_get_actual_qos(&client);
  EXPECT_EQ(rmw_qos_profile_services_default.reliability, response_subscription_qos->reliability);
  EXPECT_EQ(rmw_qos_profile_services_default.history, response_subscription_qos->history);
  EXPECT_EQ(rmw_qos_profile_services_default.depth, response_subscription_qos->depth);
  EXPECT_EQ(rmw_qos_profile_services_default.durability, response_subscription_qos->durability);
  EXPECT_EQ(
    rmw_qos_profile_services_default.avoid_ros_namespace_conventions,
    response_subscription_qos->avoid_ros_namespace_conventions);

  // 检查初始化返回码以及服务名称是否与预期一致
  // Check the return code of initialization and that the service name matches what's expected
  ASSERT_EQ(RCL_RET_OK, ret) << rcl_get_error_string().str;
  EXPECT_EQ(strcmp(rcl_client_get_service_name(&client), expected_topic_name), 0);

  // 清理客户端
  // Clean up the client
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT({
    rcl_ret_t ret = rcl_client_fini(&client, this->node_ptr);
    EXPECT_EQ(RCL_RET_OK, ret) << rcl_get_error_string().str;
  });

  // 初始化客户端请求
  // Initialize the client request
  test_msgs__srv__BasicTypes_Request req;
  test_msgs__srv__BasicTypes_Request__init(&req);
  req.uint8_value = 1;
  req.uint32_value = 2;

  // 检查发送请求时是否有错误
  // Check that there were no errors while sending the request
  int64_t sequence_number = 0;
  ret = rcl_send_request(&client, &req, &sequence_number);
  EXPECT_EQ(sequence_number, 1);
  EXPECT_EQ(RCL_RET_OK, ret) << rcl_get_error_string().str;

  // 清理请求
  // Clean up the request
  test_msgs__srv__BasicTypes_Request__fini(&req);
}

/**
 * @brief 测试客户端初始化和终止函数 (Testing the client init and fini functions)
 */
TEST_F(TestClientFixture, test_client_init_fini)
{
  rcl_ret_t ret;
  // 设置有效输入 (Setup valid inputs)
  rcl_client_t client;

  const rosidl_service_type_support_t * ts =
    ROSIDL_GET_SRV_TYPE_SUPPORT(test_msgs, srv, BasicTypes);
  const char * topic_name = "chatter";
  rcl_client_options_t default_client_options = rcl_client_get_default_options();

  // 尝试在初始化时传递空的客户端指针 (Try passing null for client in init)
  ret = rcl_client_init(nullptr, this->node_ptr, ts, topic_name, &default_client_options);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, ret) << rcl_get_error_string().str;
  rcl_reset_error();

  // 尝试在初始化时传递空的节点指针 (Try passing null for a node pointer in init)
  client = rcl_get_zero_initialized_client();
  ret = rcl_client_init(&client, nullptr, ts, topic_name, &default_client_options);
  EXPECT_EQ(RCL_RET_NODE_INVALID, ret) << rcl_get_error_string().str;
  rcl_reset_error();

  // 检查空发布者是否有效 (Check if null publisher is valid)
  EXPECT_FALSE(rcl_client_is_valid(nullptr));
  rcl_reset_error();

  // 检查零初始化的客户端是否有效 (Check if zero initialized client is valid)
  client = rcl_get_zero_initialized_client();
  EXPECT_FALSE(rcl_client_is_valid(&client));
  rcl_reset_error();

  // 检查有效的客户端是否有效 (Check that a valid client is valid)
  client = rcl_get_zero_initialized_client();
  ret = rcl_client_init(&client, this->node_ptr, ts, topic_name, &default_client_options);
  EXPECT_EQ(RCL_RET_OK, ret) << rcl_get_error_string().str;
  EXPECT_TRUE(rcl_client_is_valid(&client));
  ret = rcl_client_init(&client, this->node_ptr, ts, topic_name, &default_client_options);
  EXPECT_EQ(RCL_RET_ALREADY_INIT, ret) << rcl_get_error_string().str;
  ret = rcl_client_fini(&client, this->node_ptr);
  EXPECT_EQ(RCL_RET_OK, ret) << rcl_get_error_string().str;
  rcl_reset_error();

  // 尝试在初始化时传递无效（未初始化）的节点 (Try passing an invalid (uninitialized) node in init)
  client = rcl_get_zero_initialized_client();
  rcl_node_t invalid_node = rcl_get_zero_initialized_node();
  ret = rcl_client_init(&client, &invalid_node, ts, topic_name, &default_client_options);
  EXPECT_EQ(RCL_RET_NODE_INVALID, ret) << rcl_get_error_string().str;
  rcl_reset_error();

  // 尝试在初始化时传递空的类型支持 (Try passing null for the type support in init)
  client = rcl_get_zero_initialized_client();
  ret = rcl_client_init(&client, this->node_ptr, nullptr, topic_name, &default_client_options);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, ret) << rcl_get_error_string().str;
  rcl_reset_error();

  // 尝试在初始化时传递空的主题名称 (Try passing null for the topic name in init)
  client = rcl_get_zero_initialized_client();
  ret = rcl_client_init(&client, this->node_ptr, ts, nullptr, &default_client_options);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, ret) << rcl_get_error_string().str;
  rcl_reset_error();

  // 尝试在初始化时传递空的选项 (Try passing null for the options in init)
  client = rcl_get_zero_initialized_client();
  ret = rcl_client_init(&client, this->node_ptr, ts, topic_name, nullptr);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, ret) << rcl_get_error_string().str;
  rcl_reset_error();

  // 尝试在初始化时传递具有无效分配器的选项 (Try passing options with an invalid allocate in allocator with init)
  client = rcl_get_zero_initialized_client();
  rcl_client_options_t client_options_with_invalid_allocator;
  client_options_with_invalid_allocator = rcl_client_get_default_options();
  client_options_with_invalid_allocator.allocator.allocate = nullptr;
  ret = rcl_client_init(
    &client, this->node_ptr, ts, topic_name, &client_options_with_invalid_allocator);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, ret) << rcl_get_error_string().str;
  rcl_reset_error();

  // 尝试在初始化时传递具有无效 deallocate 的分配器的选项 (Try passing options with an invalid deallocate in allocator with init)
  client = rcl_get_zero_initialized_client();
  client_options_with_invalid_allocator = rcl_client_get_default_options();
  client_options_with_invalid_allocator.allocator.deallocate = nullptr;
  ret = rcl_client_init(
    &client, this->node_ptr, ts, topic_name, &client_options_with_invalid_allocator);
  EXPECT_EQ(RCL_RET_INVALID_ARGUMENT, ret) << rcl_get_error_string().str;
  rcl_reset_error();

  // 具有无效 realloc 的分配器可能会工作（因此我们不会测试它）(An allocator with an invalid realloc will probably work (so we will not test it))

  // 尝试在初始化时传递具有失败分配器的选项 (Try passing options with a failing allocator with init)
  client = rcl_get_zero_initialized_client();
  rcl_client_options_t client_options_with_failing_allocator;
  client_options_with_failing_allocator = rcl_client_get_default_options();
  client_options_with_failing_allocator.allocator.allocate = failing_malloc;
  client_options_with_failing_allocator.allocator.reallocate = failing_realloc;
  ret = rcl_client_init(
    &client, this->node_ptr, ts, topic_name, &client_options_with_failing_allocator);
  EXPECT_EQ(RCL_RET_BAD_ALLOC, ret) << rcl_get_error_string().str;
  rcl_reset_error();
}

/**
 * @brief 测试客户端错误参数的情况 (Test cases for passing bad/invalid arguments to client functions)
 */
TEST_F(TestClientFixture, test_client_bad_arguments)
{
  // 初始化一个空的客户端对象 (Initialize an empty client object)
  rcl_client_t client = rcl_get_zero_initialized_client();
  // 获取服务类型支持 (Get service type support)
  const rosidl_service_type_support_t * ts =
    ROSIDL_GET_SRV_TYPE_SUPPORT(test_msgs, srv, BasicTypes);
  // 获取默认的客户端选项 (Get default client options)
  rcl_client_options_t default_client_options = rcl_client_get_default_options();

  // 测试无效的服务名称 (Test invalid service name)
  EXPECT_EQ(
    RCL_RET_SERVICE_NAME_INVALID,
    rcl_client_init(&client, this->node_ptr, ts, "invalid name", &default_client_options))
    << rcl_get_error_string().str;
  rcl_reset_error();

  // 测试无效的节点 (Test invalid node)
  EXPECT_EQ(RCL_RET_NODE_INVALID, rcl_client_fini(&client, nullptr));
  rcl_reset_error();
  rcl_node_t not_valid_node = rcl_get_zero_initialized_node();
  EXPECT_EQ(RCL_RET_NODE_INVALID, rcl_client_fini(&client, &not_valid_node));
  rcl_reset_error();

  // 初始化请求和响应对象 (Initialize request and response objects)
  rmw_service_info_t header;
  int64_t sequence_number = 24;
  test_msgs__srv__BasicTypes_Response client_response;
  test_msgs__srv__BasicTypes_Request client_request;
  test_msgs__srv__BasicTypes_Request__init(&client_request);
  test_msgs__srv__BasicTypes_Response__init(&client_response);
  // 清理请求和响应对象 (Clean up request and response objects)
  OSRF_TESTING_TOOLS_CPP_SCOPE_EXIT({
    test_msgs__srv__BasicTypes_Response__fini(&client_response);
    test_msgs__srv__BasicTypes_Request__fini(&client_request);
  });

  // 测试空指针参数 (Test null pointer arguments)
  EXPECT_EQ(nullptr, rcl_client_get_rmw_handle(nullptr));
  rcl_reset_error();
  EXPECT_EQ(nullptr, rcl_client_get_service_name(nullptr));
  rcl_reset_error();
  EXPECT_EQ(nullptr, rcl_client_get_service_name(nullptr));
  rcl_reset_error();
  EXPECT_EQ(nullptr, rcl_client_get_options(nullptr));
  EXPECT_EQ(RCL_RET_CLIENT_INVALID, rcl_take_response_with_info(nullptr, &header, &client_response))
    << rcl_get_error_string().str;
  rcl_reset_error();
  EXPECT_EQ(
    RCL_RET_CLIENT_INVALID, rcl_take_response(nullptr, &(header.request_id), &client_response))
    << rcl_get_error_string().str;
  rcl_reset_error();
  EXPECT_EQ(RCL_RET_CLIENT_INVALID, rcl_send_request(nullptr, &client_request, &sequence_number))
    << rcl_get_error_string().str;
  rcl_reset_error();
  EXPECT_EQ(24, sequence_number);
  EXPECT_EQ(nullptr, rcl_client_request_publisher_get_actual_qos(nullptr));
  rcl_reset_error();
  EXPECT_EQ(nullptr, rcl_client_response_subscription_get_actual_qos(nullptr));
  rcl_reset_error();

  // 测试未初始化的客户端 (Test not initialized client)
  EXPECT_EQ(nullptr, rcl_client_get_rmw_handle(&client));
  rcl_reset_error();
  EXPECT_EQ(nullptr, rcl_client_get_service_name(&client));
  rcl_reset_error();
  EXPECT_EQ(nullptr, rcl_client_get_service_name(&client));
  rcl_reset_error();
  EXPECT_EQ(nullptr, rcl_client_get_options(&client));
  EXPECT_EQ(RCL_RET_CLIENT_INVALID, rcl_take_response_with_info(&client, &header, &client_response))
    << rcl_get_error_string().str;
  rcl_reset_error();
  EXPECT_EQ(
    RCL_RET_CLIENT_INVALID, rcl_take_response(&client, &(header.request_id), &client_response))
    << rcl_get_error_string().str;
  rcl_reset_error();
  EXPECT_EQ(RCL_RET_CLIENT_INVALID, rcl_send_request(&client, &client_request, &sequence_number))
    << rcl_get_error_string().str;
  rcl_reset_error();
  EXPECT_EQ(24, sequence_number);
  EXPECT_EQ(nullptr, rcl_client_request_publisher_get_actual_qos(&client));
  rcl_reset_error();
  EXPECT_EQ(nullptr, rcl_client_response_subscription_get_actual_qos(&client));
  rcl_reset_error();
}

/**
 * @brief 测试客户端初始化和终止函数可能失败的情况 (Test the case where client initialization and finalization functions may fail)
 *
 * @param TestClientFixture 测试客户端固件 (Test client fixture)
 * @param test_client_init_fini_maybe_fail 测试名称 (Test name)
 */
TEST_F(TestClientFixture, test_client_init_fini_maybe_fail)
{
  // 获取服务类型支持信息 (Get service type support information)
  const rosidl_service_type_support_t * ts =
    ROSIDL_GET_SRV_TYPE_SUPPORT(test_msgs, srv, BasicTypes);

  // 定义话题名称 (Define topic name)
  constexpr char topic_name[] = "chatter";

  // 获取默认客户端选项 (Get default client options)
  rcl_client_options_t default_client_options = rcl_client_get_default_options();

  // 使用故障注入测试 (Using fault injection test)
  RCUTILS_FAULT_INJECTION_TEST({
    // 初始化一个零值客户端 (Initialize a zero-value client)
    rcl_client_t client = rcl_get_zero_initialized_client();

    // 尝试初始化客户端 (Try to initialize the client)
    rcl_ret_t ret =
      rcl_client_init(&client, this->node_ptr, ts, topic_name, &default_client_options);

    // 如果初始化成功 (If the initialization is successful)
    if (RCL_RET_OK == ret) {
      // 检查客户端是否有效 (Check if the client is valid)
      EXPECT_TRUE(rcl_client_is_valid(&client));

      // 尝试终止客户端 (Try to finalize the client)
      ret = rcl_client_fini(&client, this->node_ptr);

      // 如果终止失败 (If the finalization fails)
      if (RCL_RET_OK != ret) {
        // 如果故障注入导致终止失败，我们应该再次尝试 (If fault injection caused the finalization to fail, we should try again)
        EXPECT_EQ(RCL_RET_OK, rcl_client_fini(&client, this->node_ptr));
      }
    } else {
      // 如果初始化失败，检查错误是否已设置 (If the initialization fails, check if the error is set)
      EXPECT_TRUE(rcl_error_is_set());

      // 重置错误 (Reset the error)
      rcl_reset_error();
    }
  });
}
