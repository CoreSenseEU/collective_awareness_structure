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
 *  \file       ca_gateway_efferent.cpp
 *  \brief      CA_GatewayEfferent component implementation
 *  \authors    Guillermo GP-Lenza
 ********************************************************************************************/

#include "ca_structure/ca_gateway_efferent.hpp"

#include <string>

namespace ca_structure
{

CA_GatewayEfferent::CA_GatewayEfferent(rclcpp_lifecycle::LifecycleNode::SharedPtr parent)
: Efferent("ca_gateway_efferent", parent)
{
}

bool CA_GatewayEfferent::configure()
{
  return true;
}

size_t CA_GatewayEfferent::add_type_publisher(const std::string & type)
{
  auto it = type_to_index_.find(type);
  if (it != type_to_index_.end()) {
    RCLCPP_INFO(
      parent_->get_logger(),
      "[CA_GatewayEfferent] Type '%s' already registered on topic '%s'",
      type.c_str(), get_topic_for_index(it->second).c_str());
    return it->second;
  }

  std::string topic = type + "_in";
  create_publisher(topic, "ca_msgs/msg/LocalGenericMessage");
  size_t idx = pubs_.size() - 1;
  type_to_index_[type] = idx;

  RCLCPP_INFO(
    parent_->get_logger(),
    "[CA_GatewayEfferent] Created publisher for type '%s' on topic '%s'",
    type.c_str(), topic.c_str());

  return idx;
}

bool CA_GatewayEfferent::has_type(const std::string & type) const
{
  return type_to_index_.find(type) != type_to_index_.end();
}

size_t CA_GatewayEfferent::get_index_for_type(const std::string & type) const
{
  return type_to_index_.at(type);
}

std::string CA_GatewayEfferent::get_topic_for_index(size_t idx) const
{
  return pubs_[idx]->get_topic_name();
}

}  // namespace ca_structure
