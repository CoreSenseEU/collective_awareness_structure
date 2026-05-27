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
 *  \file       ca_gateway.hpp
 *  \brief      CA_Gateway node header file
 *  \authors    Guillermo GP-Lenza
 ********************************************************************************************/

#ifndef CA_STRUCTURE__CA_GATEWAY_HPP_
#define CA_STRUCTURE__CA_GATEWAY_HPP_

#include <memory>
#include <string>

#include "cs4home_core/CognitiveModule.hpp"

#include "ca_msgs/msg/inter_agent_message.hpp"
#include "ca_msgs/msg/local_generic_message.hpp"

#include "ca_structure/ca_gateway_afferent.hpp"
#include "ca_structure/ca_gateway_coupling.hpp"
#include "ca_structure/ca_gateway_core.hpp"
#include "ca_structure/ca_gateway_efferent.hpp"

#include "rclcpp/rclcpp.hpp"

namespace ca_structure
{

class CA_Gateway : public cs4home_core::CognitiveModule
{
public:
  RCLCPP_SMART_PTR_DEFINITIONS(CA_Gateway)
  using CallbackReturnT = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

  CA_Gateway();

  CallbackReturnT on_configure(const rclcpp_lifecycle::State & state) override;

protected:
  // Outgoing path: local modules → inter-agent network
  rclcpp::Subscription<ca_msgs::msg::LocalGenericMessage>::SharedPtr outgoing_ca_sub_;
  rclcpp::Publisher<ca_msgs::msg::InterAgentMessage>::SharedPtr inter_agent_pub_;

  std::string inter_agent_topic_;
  std::string register_module_service_name_;
  std::string out_messages_topic_;
  std::string agent_id_;

private:
  void forward_local_message(const ca_msgs::msg::LocalGenericMessage::SharedPtr msg);
};

}  // namespace ca_structure

#endif  // CA_STRUCTURE__CA_GATEWAY_HPP_
