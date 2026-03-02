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
 *  \file       ca_gateway_efferent.hpp
 *  \brief      CA_GatewayEfferent component header
 *  \authors    Guillermo GP-Lenza
 ********************************************************************************************/

#ifndef AS2_CA__CA_GATEWAY_EFFERENT_HPP_
#define AS2_CA__CA_GATEWAY_EFFERENT_HPP_

#include <string>
#include <unordered_map>

#include "cs4home_core/Efferent.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"

namespace as2_ca
{

class CA_GatewayEfferent : public cs4home_core::Efferent
{
public:
  RCLCPP_SMART_PTR_DEFINITIONS(CA_GatewayEfferent)

  explicit CA_GatewayEfferent(rclcpp_lifecycle::LifecycleNode::SharedPtr parent);

  bool configure() override;

  // Creates a publisher for the given type on topic "<type>_in" if not already present.
  // Returns the publisher index for use with publish().
  size_t add_type_publisher(const std::string & type);

  bool has_type(const std::string & type) const;
  size_t get_index_for_type(const std::string & type) const;
  std::string get_topic_for_index(size_t idx) const;

private:
  std::unordered_map<std::string, size_t> type_to_index_;
};

}  // namespace as2_ca

#endif  // AS2_CA__CA_GATEWAY_EFFERENT_HPP_
