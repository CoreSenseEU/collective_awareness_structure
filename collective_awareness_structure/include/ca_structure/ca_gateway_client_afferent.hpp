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
 *  \file       ca_gateway_client_afferent.hpp
 *  \brief      Afferent component that reads module registrations from a YAML
 *              file and subscribes to the gateway-provided local topics.
 *  \authors    Guillermo GP-Lenza
 ********************************************************************************************/

#ifndef AS2_CA__CA_GATEWAY_CLIENT_AFFERENT_HPP_
#define AS2_CA__CA_GATEWAY_CLIENT_AFFERENT_HPP_

#include <memory>
#include <string>

#include "cs4home_core/Afferent.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "ca_msgs/srv/register_module.hpp"

namespace ca_structure
{

/**
 * @class CA_GatewayClientAfferent
 * @brief Afferent that reads {type, module_name} pairs from a YAML file
 *        (parameter CA_GatewayClientAfferent.config_file), calls the
 *        register_module service for each entry, and subscribes to the
 *        returned LocalGenericMessage topics.
 */
class CA_GatewayClientAfferent : public cs4home_core::Afferent
{
public:
  RCLCPP_SMART_PTR_DEFINITIONS(CA_GatewayClientAfferent)

  explicit CA_GatewayClientAfferent(rclcpp_lifecycle::LifecycleNode::SharedPtr parent);

  bool configure() override;

private:
  rclcpp::CallbackGroup::SharedPtr callback_group_;
  rclcpp::Client<ca_msgs::srv::RegisterModule>::SharedPtr register_module_client_;
};

}  // namespace ca_structure

#endif  // AS2_CA__CA_GATEWAY_CLIENT_AFFERENT_HPP_
