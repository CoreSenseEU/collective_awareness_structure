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
 *  \file       ca_gateway_client.cpp
 *  \brief      CA Gateway client implementation
 *  \authors    Guillermo GP-Lenza
 ********************************************************************************************/

#include "ca_structure/ca_gateway_client.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace ca_structure
{

CAGatewayClient::~CAGatewayClient()
{
  clear();
}

void CAGatewayClient::clear()
{
  local_generic_subscribers_.clear();
}

int CAGatewayClient::get_subscriber_count() const
{
  return static_cast<int>(local_generic_subscribers_.size());
}

std::vector<std::string> CAGatewayClient::get_known_peers() const
{
  if (!get_node_names_fn_) {
    return {};
  }
  std::vector<std::string> peers;
  for (const auto & node_name : get_node_names_fn_()) {
    // Node names are "/namespace/node_name"; extract the namespace part.
    const size_t last_slash = node_name.rfind('/');
    std::string ns = (last_slash != std::string::npos && last_slash > 0) ?
      node_name.substr(0, last_slash) : node_name;

    if (!ns.empty() && ns.front() == '/') {
      ns = ns.substr(1);
    }
    if (ns.empty() || ns == agent_id_) {
      continue;
    }
    if (std::find(peers.begin(), peers.end(), ns) == peers.end()) {
      peers.push_back(ns);
    }
  }
  return peers;
}

}  // namespace ca_structure
