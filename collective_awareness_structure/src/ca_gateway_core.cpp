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
 *  \file       ca_gateway_core.cpp
 *  \brief      CA_GatewayCore component implementation
 *  \authors    Guillermo GP-Lenza
 ********************************************************************************************/

#include "as2_ca/ca_gateway_core.hpp"
#include "as2_ca/ca_gateway_efferent.hpp"

#include <memory>
#include <string>

#include "as2_ca_msgs/msg/inter_agent_message.hpp"
#include "as2_ca_msgs/msg/local_generic_message.hpp"

using std::placeholders::_1;

namespace as2_ca
{

CA_GatewayCore::CA_GatewayCore(rclcpp_lifecycle::LifecycleNode::SharedPtr parent)
: Core("ca_gateway_core", parent)
{
}

bool CA_GatewayCore::configure()
{
  std::string inter_agent_topic;
  parent_->get_parameter("inter_agent_topic", inter_agent_topic);

  afferent_->set_mode(
    inter_agent_topic,
    cs4home_core::Afferent::CALLBACK,
    std::bind(&CA_GatewayCore::ia_message_callback, this, _1));

  return true;
}

bool CA_GatewayCore::activate()
{
  return true;
}

bool CA_GatewayCore::deactivate()
{
  return true;
}

void CA_GatewayCore::ia_message_callback(std::shared_ptr<rclcpp::SerializedMessage> serialized_msg)
{
  auto msg = afferent_->get_msg<as2_ca_msgs::msg::InterAgentMessage>(serialized_msg);

  auto gw_efferent = std::dynamic_pointer_cast<CA_GatewayEfferent>(efferent_);
  if (!gw_efferent->has_type(msg->type)) {
    RCLCPP_WARN(
      parent_->get_logger(),
      "[CA_GatewayCore] No publisher registered for type '%s'. Dropping message.",
      msg->type.c_str());
    return;
  }

  size_t idx = gw_efferent->get_index_for_type(msg->type);

  auto local_msg = std::make_shared<as2_ca_msgs::msg::LocalGenericMessage>();
  local_msg->agent = msg->sender;
  local_msg->type = msg->type;
  local_msg->data = msg->data;

  efferent_->publish(idx, local_msg);

  RCLCPP_DEBUG(
    parent_->get_logger(),
    "[CA_GatewayCore] Forwarded message of type '%s' from '%s' to topic '%s'",
    msg->type.c_str(), msg->sender.c_str(),
    gw_efferent->get_topic_for_index(idx).c_str());
}

}  // namespace as2_ca
