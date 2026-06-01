// Copyright 2026 Universidad Politécnica de Madrid
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of the Universidad Politécnica de Madrid nor the names of its
//      contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

/*!*******************************************************************************************
 *  \file       ca_gateway_client.hpp
 *  \brief      CA Gateway client — works with rclcpp::Node and lifecycle nodes.
 *  \authors    Guillermo GP-Lenza
 ********************************************************************************************/

#ifndef CA_STRUCTURE__CA_GATEWAY_CLIENT_HPP_
#define CA_STRUCTURE__CA_GATEWAY_CLIENT_HPP_

#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ca_msgs/msg/local_generic_message.hpp"
#include "ca_msgs/srv/register_module.hpp"

#include "rclcpp/rclcpp.hpp"
#include "rclcpp/serialization.hpp"

namespace ca_structure
{

class CAGatewayClient
{
public:
  CAGatewayClient() = default;

  // Template constructor: accepts rclcpp::Node::SharedPtr or
  // rclcpp_lifecycle::LifecycleNode::SharedPtr — both expose the same pub/sub/client API.
  template<typename NodeT>
  explicit CAGatewayClient(std::shared_ptr<NodeT> node)
  : logger_(node->get_logger()),
    agent_id_(node->get_namespace()),
    own_namespace_(node->get_namespace())
  {
    if (!agent_id_.empty() && agent_id_.front() == '/') {
      agent_id_ = agent_id_.substr(1);
    }

    const std::string svc  = std::string(node->get_namespace()) + "/register_module";
    const std::string out  = std::string(node->get_namespace()) + "/gateway_out";

    register_module_client_ =
      node->template create_client<ca_msgs::srv::RegisterModule>(svc);

    forwarder_pub_ =
      node->template create_publisher<ca_msgs::msg::LocalGenericMessage>(out, 10);

    // Type-erased factory so subscribe_to_local_generic<T> doesn't capture the node type.
    sub_factory_ = [node](
      const std::string & topic,
      std::function<void(ca_msgs::msg::LocalGenericMessage::SharedPtr)> cb)
    {
      return node->template create_subscription<ca_msgs::msg::LocalGenericMessage>(
        topic, 10, cb);
    };

    get_node_names_fn_ = [node]() {return node->get_node_names();};

    while (!register_module_client_->wait_for_service(std::chrono::seconds(1))) {
      if (!rclcpp::ok()) {
        RCLCPP_ERROR(logger_, "Interrupted while waiting for register_module service.");
        return;
      }
      RCLCPP_INFO(logger_, "Waiting for register_module service...");
    }
    RCLCPP_INFO(logger_, "Connected to register_module service.");
  }

  ~CAGatewayClient();
  void clear();

  // Asynchronously register for messages of the given type; callback receives deserialized T
  // plus the sender agent id.
  template<typename T>
  void register_module(
    const std::string & type,
    const std::string & module_name,
    std::function<void(const T &, const std::string &)> callback)
  {
    auto request = std::make_shared<ca_msgs::srv::RegisterModule::Request>();
    request->type = type;
    request->module_name = module_name;

    RCLCPP_INFO(logger_, "Registering module '%s' for type '%s'",
      module_name.c_str(), type.c_str());

    assert(register_module_client_ != nullptr);
    register_module_client_->async_send_request(
      request,
      [this, module_name, callback](
        rclcpp::Client<ca_msgs::srv::RegisterModule>::SharedFuture future)
      {
        auto response = future.get();
        if (response->topic.empty()) {
          RCLCPP_ERROR(logger_, "Failed to register module '%s'", module_name.c_str());
          return;
        }
        RCLCPP_INFO(logger_, "Module '%s' registered on topic '%s'",
          module_name.c_str(), response->topic.c_str());
        subscribe_to_local_generic<T>(response->topic, callback);
      });
  }

  // Serialize msg and forward to each receiver via the CA gateway out topic.
  template<typename T>
  void forward_IA_msg(
    const T & msg,
    const std::string & type,
    const std::vector<std::string> & receivers)
  {
    rclcpp::Serialization<T> serializer;
    rclcpp::SerializedMessage serialized;
    serializer.serialize_message(&msg, &serialized);
    const auto & rcl = serialized.get_rcl_serialized_message();
    const std::vector<uint8_t> data(rcl.buffer, rcl.buffer + rcl.buffer_length);

    for (const auto & receiver : receivers) {
      ca_msgs::msg::LocalGenericMessage generic_msg;
      generic_msg.agent = receiver;
      generic_msg.type  = type;
      generic_msg.data  = data;
      forwarder_pub_->publish(generic_msg);
    }
  }

  // Discover peers by inspecting the ROS 2 graph for namespaces other than own.
  std::vector<std::string> get_known_peers() const;

  int get_subscriber_count() const;

private:
  using LocalSub = rclcpp::Subscription<ca_msgs::msg::LocalGenericMessage>::SharedPtr;
  using SubFactory = std::function<
    LocalSub(
      const std::string &,
      std::function<void(ca_msgs::msg::LocalGenericMessage::SharedPtr)>)>;

  template<typename T>
  void subscribe_to_local_generic(
    const std::string & topic,
    std::function<void(const T &, const std::string &)> callback)
  {
    auto sub = sub_factory_(
      topic,
      [callback](ca_msgs::msg::LocalGenericMessage::SharedPtr msg)
      {
        T deserialized;
        rclcpp::SerializedMessage serialized(msg->data.size());
        auto & rcl = serialized.get_rcl_serialized_message();
        std::memcpy(rcl.buffer, msg->data.data(), msg->data.size());
        rcl.buffer_length = msg->data.size();
        rclcpp::Serialization<T> s;
        s.deserialize_message(&serialized, &deserialized);
        callback(deserialized, msg->agent);
      });
    local_generic_subscribers_.push_back(sub);
  }

  std::vector<LocalSub> local_generic_subscribers_;
  rclcpp::Client<ca_msgs::srv::RegisterModule>::SharedPtr register_module_client_;
  rclcpp::Publisher<ca_msgs::msg::LocalGenericMessage>::SharedPtr forwarder_pub_;
  SubFactory sub_factory_;
  std::function<std::vector<std::string>()> get_node_names_fn_;
  rclcpp::Logger logger_{rclcpp::get_logger("ca_gateway_client")};
  std::string agent_id_;
  std::string own_namespace_;
};

}  // namespace ca_structure

#endif  // CA_STRUCTURE__CA_GATEWAY_CLIENT_HPP_
