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
 *  \file       ca_gateway_client_afferent.cpp
 *  \brief      CA_GatewayClientAfferent implementation
 *  \authors    Guillermo GP-Lenza
 ********************************************************************************************/

#include "as2_ca/ca_gateway_client_afferent.hpp"

#include <chrono>
#include <memory>
#include <string>

#include "rclcpp/executors/single_threaded_executor.hpp"
#include "yaml-cpp/yaml.h"

namespace as2_ca
{

CA_GatewayClientAfferent::CA_GatewayClientAfferent(
  rclcpp_lifecycle::LifecycleNode::SharedPtr parent)
: cs4home_core::Afferent("CA_GatewayClientAfferent", parent)
{
  parent_->declare_parameter(name_ + ".config_file", std::string(""));

  callback_group_ = parent_->create_callback_group(
    rclcpp::CallbackGroupType::MutuallyExclusive);

  const std::string service_name =
    std::string(parent_->get_namespace()) + "/register_module";

  register_module_client_ = parent_->create_client<as2_ca_msgs::srv::RegisterModule>(
    service_name, rmw_qos_profile_services_default, callback_group_);
}

bool CA_GatewayClientAfferent::configure()
{
  using std::chrono_literals::operator""s;

  std::string config_file;
  parent_->get_parameter(name_ + ".config_file", config_file);

  if (config_file.empty()) {
    RCLCPP_ERROR(
      parent_->get_logger(),
      "Parameter '%s.config_file' is not set. Cannot register any modules.",
      name_.c_str());
    return false;
  }

  YAML::Node root;
  try {
    root = YAML::LoadFile(config_file);
  } catch (const YAML::Exception & e) {
    RCLCPP_ERROR(
      parent_->get_logger(),
      "Failed to load module config file '%s': %s",
      config_file.c_str(), e.what());
    return false;
  }

  if (!root["modules"] || !root["modules"].IsSequence()) {
    RCLCPP_ERROR(
      parent_->get_logger(),
      "Config file '%s' must contain a 'modules' sequence.",
      config_file.c_str());
    return false;
  }

  if (!register_module_client_->wait_for_service(5s)) {
    RCLCPP_ERROR(
      parent_->get_logger(),
      "register_module service not available after 5 s");
    return false;
  }

  rclcpp::executors::SingleThreadedExecutor service_exec;
  service_exec.add_callback_group(callback_group_, parent_->get_node_base_interface());

  bool any_registered = false;

  for (const auto & entry : root["modules"]) {
    if (!entry["type"] || !entry["module_name"]) {
      RCLCPP_WARN(
        parent_->get_logger(),
        "Skipping malformed entry in '%s': missing 'type' or 'module_name'.",
        config_file.c_str());
      continue;
    }

    const std::string type = entry["type"].as<std::string>();
    const std::string module_name = entry["module_name"].as<std::string>();

    auto request = std::make_shared<as2_ca_msgs::srv::RegisterModule::Request>();
    request->type = type;
    request->module_name = module_name;

    auto future = register_module_client_->async_send_request(request);

    if (service_exec.spin_until_future_complete(future, 5s) !=
      rclcpp::FutureReturnCode::SUCCESS)
    {
      RCLCPP_ERROR(
        parent_->get_logger(),
        "register_module call timed out for type '%s'. Skipping.",
        type.c_str());
      continue;
    }

    auto response = future.get();
    if (response->topic.empty()) {
      RCLCPP_ERROR(
        parent_->get_logger(),
        "register_module returned an empty topic for type '%s'. Skipping.",
        type.c_str());
      continue;
    }

    RCLCPP_INFO(
      parent_->get_logger(),
      "Module '%s' registered for type '%s', subscribing to '%s'",
      module_name.c_str(), type.c_str(), response->topic.c_str());

    if (create_subscriber(response->topic, "as2_ca_msgs/msg/LocalGenericMessage")) {
      any_registered = true;
    }
  }

  if (!any_registered) {
    RCLCPP_ERROR(
      parent_->get_logger(),
      "No modules could be registered from config file '%s'.",
      config_file.c_str());
  }

  return any_registered;
}

}  // namespace as2_ca
