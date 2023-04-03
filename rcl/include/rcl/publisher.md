## struct

```c
typedef struct rcl_publisher_impl_s rcl_publisher_impl_t;
typedef struct rcl_publisher_s {
  rcl_publisher_impl_t * impl;
} rcl_publisher_t;
typedef struct rcl_publisher_options_s {
  rmw_qos_profile_t qos;
  rcl_allocator_t allocator;
  rmw_publisher_options_t rmw_publisher_options;
  bool disable_loaned_message;
} rcl_publisher_options_t;
```

1. **rcl_publisher_impl_s**：这是一个不完全定义的结构体类型，用于表示发布者的实现细节。具体的实现将在其他地方进行定义。

2. **rcl_publisher_t**：这是一个包含指向 `rcl_publisher_impl_t` 类型的指针的结构体。它代表了一个发布者对象，用于在 ROS2 中发布消息。

3. **rcl_publisher_options_s**：这个结构体包含了创建发布者时所需的选项和配置信息。
   - **rmw_qos_profile_t qos**：这是一个质量服务（Quality of Service, QoS）配置，用于指定发布者的 QoS 策略，如可靠性、持久性等。
   - **rcl_allocator_t allocator**：这是一个内存分配器，用于管理发布者对象的内存分配和释放。
   - **rmw_publisher_options_t rmw_publisher_options**：这是一个与底层中间件相关的发布者选项，用于传递给底层实现。
   - **bool disable_loaned_message**：这是一个布尔值，用于指示是否禁用借用消息功能。如果为 true，则禁用；否则启用。

通过这些结构体定义，我们可以更好地理解 ROS2 中发布者的组成和配置方式。在实际使用中，用户会创建一个 `rcl_publisher_t` 类型的对象，并根据需要设置 `rcl_publisher_options_t` 结构体中的选项，以便在 ROS2 系统中发布消息。

##

```c
rcl_publisher_t rcl_get_zero_initialized_publisher(void);
rcl_ret_t rcl_publisher_init(
  rcl_publisher_t * publisher, const rcl_node_t * node,
  const rosidl_message_type_support_t * type_support, const char * topic_name,
  const rcl_publisher_options_t * options);
rcl_ret_t rcl_publisher_fini(rcl_publisher_t * publisher, rcl_node_t * node);
rcl_publisher_options_t rcl_publisher_get_default_options(void);
rcl_ret_t rcl_borrow_loaned_message(
  const rcl_publisher_t * publisher, const rosidl_message_type_support_t * type_support,
  void ** ros_message);
rcl_ret_t rcl_return_loaned_message_from_publisher(
  const rcl_publisher_t * publisher, void * loaned_message);
rcl_ret_t rcl_publish(
  const rcl_publisher_t * publisher, const void * ros_message,
  rmw_publisher_allocation_t * allocation);
rcl_ret_t rcl_publish_serialized_message(
  const rcl_publisher_t * publisher, const rcl_serialized_message_t * serialized_message,
  rmw_publisher_allocation_t * allocation);
rcl_ret_t rcl_publish_loaned_message(
  const rcl_publisher_t * publisher, void * ros_message, rmw_publisher_allocation_t * allocation);
rcl_ret_t rcl_publisher_assert_liveliness(const rcl_publisher_t * publisher);
rcl_ret_t rcl_publisher_wait_for_all_acked(
  const rcl_publisher_t * publisher, rcl_duration_value_t timeout);
const char * rcl_publisher_get_topic_name(const rcl_publisher_t * publisher);
const rcl_publisher_options_t * rcl_publisher_get_options(const rcl_publisher_t * publisher);
rmw_publisher_t * rcl_publisher_get_rmw_handle(const rcl_publisher_t * publisher);
rcl_context_t * rcl_publisher_get_context(const rcl_publisher_t * publisher);
bool rcl_publisher_is_valid(const rcl_publisher_t * publisher);
bool rcl_publisher_is_valid_except_context(const rcl_publisher_t * publisher);
rcl_ret_t rcl_publisher_get_subscription_count(
  const rcl_publisher_t * publisher, size_t * subscription_count);
const rmw_qos_profile_t * rcl_publisher_get_actual_qos(const rcl_publisher_t * publisher);
bool rcl_publisher_can_loan_messages(const rcl_publisher_t * publisher);
```

1. `rcl_get_zero_initialized_publisher`：创建并返回一个零初始化的发布者对象，为后续的初始化操作做准备。
2. `rcl_publisher_init`：根据提供的节点、消息类型支持、主题名称和选项来初始化发布者。初始化后，发布者就可以用于发布消息了。
3. `rcl_publisher_fini`：销毁发布者并释放其资源。在不再需要发布者时调用此函数。
4. `rcl_publisher_get_default_options`：获取默认的发布者选项，包括 QoS 配置等。用户可以根据需要修改这些选项。
5. `rcl_borrow_loaned_message`：从发布者借用一条消息，用于后续发布。这可以避免额外的内存分配。
6. `rcl_return_loaned_message_from_publisher`：归还之前借用的消息给发布者。在消息发布完成后调用此函数。
7. `rcl_publish`：发布一条 ROS 消息。消息会被发送到发布者关联的主题上。
8. `rcl_publish_serialized_message`：发布一条序列化后的消息。这对于跨语言通信或者避免反序列化开销很有用。
9. `rcl_publish_loaned_message`：发布一条借用的消息。这可以减少内存分配和拷贝操作。
10. `rcl_publisher_assert_liveliness`：声明发布者的活跃状态。这对于某些 QoS 配置（如 LIVELINESS）很重要。
11. `rcl_publisher_wait_for_all_acked`：等待所有已发布的消息被确认。这对于可靠传输的 QoS 配置很有用。
12. `rcl_publisher_get_topic_name`：获取发布者关联的主题名称。
13. `rcl_publisher_get_options`：获取发布者的选项，包括 QoS 配置等。
14. `rcl_publisher_get_rmw_handle`：获取发布者的 RMW（ROS Middleware）句柄。这对于底层中间件操作很有用。
15. `rcl_publisher_get_context`：获取发布者所在的上下文。上下文包含了 ROS2 初始化时的配置信息。
16. `rcl_publisher_is_valid`：检查发布者是否有效。如果有效，则表示发布者可以正常使用。
17. `rcl_publisher_is_valid_except_context`：检查发布者是否有效，但不包括上下文。这对于某些特殊场景下的验证很有用。
18. `rcl_publisher_get_subscription_count`：获取发布者关联主题的订阅数量。这可以帮助了解当前有多少订阅者正在接收消息。
19. `rcl_publisher_get_actual_qos`：获取发布者的实际 QoS（Quality of Service）配置。这包括可靠性、持久性等选项。
20. `rcl_publisher_can_loan_messages`：检查发布者是否支持消息借用功能。如果支持，则可以使用借用消息的相关函数。

这些函数之间的关系主要是围绕创建、初始化、销毁发布者以及发布消息等操作展开。例如，首先使用 `rcl_publisher_init` 初始化一个发布者，然后可以通过 `rcl_publish` 发布消息，最后在不再需要发布者时，使用 `rcl_publisher_fini` 销毁它。
