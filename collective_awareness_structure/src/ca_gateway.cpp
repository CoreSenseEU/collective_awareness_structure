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
 *  \file       ca_gateway.cpp
 *  \brief      CA_Gateway node implementation
 *  \authors    Guillermo GP-Lenza
 ********************************************************************************************/

#include "ca_structure/ca_gateway.hpp"

#include <memory>
#include <string>

using std::placeholders::_1;

namespace ca_structure
{

CA_Gateway::CA_Gateway()
: cs4home_core::CognitiveModule("ca_gateway")
{
}

CA_Gateway::CallbackReturnT CA_Gateway::on_configure(const rclcpp_lifecycle::State & state)
{
  (void)state;

  // Declare CA Gateway-specific parameters
  declare_parameter("inter_agent_topic", "/agent_to_agent");
  declare_parameter("register_module_service_name", "/register_module");
  declare_parameter("out_messages_topic", "/gateway_out");
  declare_parameter("agent_id", "");

  inter_agent_topic_ = get_parameter("inter_agent_topic").as_string();
  register_module_service_name_ = get_parameter("register_module_service_name").as_string();
  out_messages_topic_ = get_parameter("out_messages_topic").as_string();
  agent_id_ = get_parameter("agent_id").as_string();

  // shared_from_this() is valid here: the node is managed by a shared_ptr at this point
  auto self = std::dynamic_pointer_cast<rclcpp_lifecycle::LifecycleNode>(shared_from_this());

  // --- Efferent: publishes LocalGenericMessage to per-type local topics ---
  auto gw_efferent = std::make_shared<CA_GatewayEfferent>(self);
  efferent_ = gw_efferent;
  efferent_->configure();

  // --- Afferent: subscribes to the inter-agent topic ---
  afferent_ = std::make_shared<CA_GatewayAfferent>(self);
  afferent_->configure();

  // --- Core: processes incoming InterAgentMessages and routes them via the efferent ---
  core_ = std::make_shared<CA_GatewayCore>(self);
  core_->set_afferent(afferent_);
  core_->set_efferent(efferent_);
  core_->configure();   // registers ia_message_callback on the afferent

  // --- Coupling: exposes the register_module service ---
  coupling_ = std::make_shared<CA_GatewayCoupling>(self, gw_efferent);
  coupling_->configure();

  // --- Outgoing path: local modules → inter-agent network ---
  inter_agent_pub_ = create_publisher<ca_msgs::msg::InterAgentMessage>(
    inter_agent_topic_, rclcpp::QoS(10));

  outgoing_ca_sub_ = create_subscription<ca_msgs::msg::LocalGenericMessage>(
    out_messages_topic_, rclcpp::QoS(10),
    std::bind(&CA_Gateway::forward_local_message, this, _1));

  RCLCPP_INFO(get_logger(), "[CA_Gateway] Configured successfully");
  return CallbackReturnT::SUCCESS;
}

void CA_Gateway::forward_local_message(const ca_msgs::msg::LocalGenericMessage::SharedPtr msg)
{
  ca_msgs::msg::InterAgentMessage ia_msg;
  ia_msg.sender = agent_id_;
  ia_msg.receiver = msg->agent;
  ia_msg.type = msg->type;
  ia_msg.data = msg->data;
  inter_agent_pub_->publish(ia_msg);
}

}  // namespace ca_structure
