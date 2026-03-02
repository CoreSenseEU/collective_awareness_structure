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
 *  \file       ca_gateway_coupling.cpp
 *  \brief      CA_GatewayCoupling component implementation
 *  \authors    Guillermo GP-Lenza
 ********************************************************************************************/

#include "as2_ca/ca_gateway_coupling.hpp"

#include <memory>
#include <string>

using std::placeholders::_1;
using std::placeholders::_2;

namespace as2_ca
{

CA_GatewayCoupling::CA_GatewayCoupling(
  rclcpp_lifecycle::LifecycleNode::SharedPtr parent,
  CA_GatewayEfferent::SharedPtr efferent)
: Coupling("ca_gateway_coupling", parent),
  efferent_(efferent)
{
}

bool CA_GatewayCoupling::configure()
{
  std::string service_name;
  parent_->get_parameter("register_module_service_name", service_name);

  register_module_srv_ = parent_->create_service<as2_ca_msgs::srv::RegisterModule>(
    service_name,
    std::bind(&CA_GatewayCoupling::register_module_cb, this, _1, _2));

  RCLCPP_INFO(
    parent_->get_logger(),
    "[CA_GatewayCoupling] register_module service ready on '%s'", service_name.c_str());

  return true;
}

void CA_GatewayCoupling::register_module_cb(
  const std::shared_ptr<as2_ca_msgs::srv::RegisterModule::Request> request,
  std::shared_ptr<as2_ca_msgs::srv::RegisterModule::Response> response)
{
  RCLCPP_INFO(
    parent_->get_logger(),
    "[CA_GatewayCoupling] Registration request from module '%s' for type '%s'",
    request->module_name.c_str(), request->type.c_str());

  size_t idx = efferent_->add_type_publisher(request->type);
  response->topic = efferent_->get_topic_for_index(idx);
}

}  // namespace as2_ca
