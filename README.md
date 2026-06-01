# collective_awareness_structure (ca_structure)

Inter-agent communication infrastructure for multi-robot systems, built on the [cs4home architecture](https://github.com/CoreSenseEU/cs4home_architecture). Provides the `CA_Gateway_Node` broker and two C++ client classes — a standalone `CAGatewayClient` and a cs4home-native `CA_GatewayClientAfferent` — so behavior modules can exchange typed messages across isolated ROS 2 agent graphs without shared memory or a central coordinator.

## Overview

In a multi-agent system, each agent runs an independent ROS 2 graph. `ca_structure` bridges those isolated graphs through a shared inter-agent topic, allowing agents to exchange typed messages without knowing each other's internal topics. Local modules register with the gateway to declare the message types they care about; the gateway routes incoming inter-agent messages to the appropriate local topic.

```
Agent A                                   Agent B
┌──────────────────────────┐             ┌──────────────────────────┐
│  LocalModule             │             │  LocalModule             │
│  (planning)              │             │  (planning)              │
│       │                  │             │       ▲                  │
│  gateway_out             │  /agent_to  │  planning_in             │
│       ▼                  │  _agent     │       │                  │
│  CA_Gateway_Node ────────┼────────────►│  CA_Gateway_Node         │
│                          │             │                          │
└──────────────────────────┘             └──────────────────────────┘
```

## Installation

```bash
cd ~/ros2_ws/src
git clone <this-repository>
cd ~/ros2_ws
colcon build --symlink-install --packages-select ca_msgs ca_structure
```

Dependencies:

```bash
sudo apt install libyaml-cpp-dev
sudo apt install ros-${ROS_DISTRO}-rclcpp-lifecycle
```

`ca_structure` also depends on `cs4home_core` for the `CA_GatewayClientAfferent` component. Build it first if it is not already installed:

```bash
cd ~/ros2_ws/src
git clone https://github.com/CoreSenseEU/cs4home_architecture.git
cd ~/ros2_ws
colcon build --packages-select cs4home_core
```

## CA_Gateway_Node

The gateway node is the central broker on each agent. It exposes a registration service that local modules call to announce themselves, and it forwards incoming inter-agent messages to the correct local topic.

### Running the node

```bash
ros2 run ca_structure ca_gateway_node --ros-args \
  -p agent_id:=drone0 \
  -p inter_agent_topic:=/agent_to_agent \
  -p out_messages_topic:=gateway_out
```

### Parameters

| Parameter | Default | Description |
|---|---|---|
| `agent_id` | `drone0` | Identifier used as the sender field in outgoing inter-agent messages |
| `inter_agent_topic` | `/agent_to_agent` | Shared topic all agents publish and subscribe to |
| `out_messages_topic` | `gateway_out` | Local topic where modules publish messages to be forwarded |
| `register_module_service_name` | `register_module` | Name of the registration service |

### Topics

| Topic | Type | Direction | Description |
|---|---|---|---|
| `/agent_to_agent` | `ca_msgs/msg/InterAgentMessage` | Sub + Pub | Shared inter-agent channel |
| `<type>_in` | `ca_msgs/msg/LocalGenericMessage` | Pub | Created on demand for each registered type |
| `gateway_out` | `ca_msgs/msg/LocalGenericMessage` | Sub | Outgoing messages from local modules |

### Services

| Service | Type | Description |
|---|---|---|
| `register_module` | `ca_msgs/srv/RegisterModule` | Registers a local module for a given type; returns the local topic to subscribe to |

### Data flow

**Incoming** — an inter-agent message arrives and is forwarded to the matching local topic:

```
/agent_to_agent  →  [CA_Gateway]  →  <type>_in
(InterAgentMessage)                  (LocalGenericMessage)
```

**Outgoing** — a local module publishes to `gateway_out` and the gateway wraps and forwards it:

```
gateway_out       →  [CA_Gateway]  →  /agent_to_agent
(LocalGenericMessage)                 (InterAgentMessage)
```

## CAGatewayClient

`ca_structure::CAGatewayClient` is a C++ helper class that hides the registration protocol. It works with any `rclcpp::Node` or `rclcpp_lifecycle::LifecycleNode` and is the recommended way for cs4home `Core` components to communicate through the gateway.

### Usage

```cpp
#include "ca_structure/ca_gateway_client.hpp"

// Attach to any node (regular or lifecycle)
auto client = std::make_shared<ca_structure::CAGatewayClient>(node);

// Register for a message type and provide a typed callback.
// The client calls the register_module service, subscribes to the returned
// topic, and deserialises the payload automatically.
client->register_module<std_msgs::msg::String>(
  "my_type",        // message type key used for routing
  "my_module",      // module name (for logging / registration)
  [](const std_msgs::msg::String & msg, const std::string & sender_agent) {
    RCLCPP_INFO(rclcpp::get_logger("demo"), "Got '%s' from %s",
      msg.data.c_str(), sender_agent.c_str());
  });
```

### Sending a message

Outgoing messages are published to `gateway_out` as a `ca_msgs::msg::LocalGenericMessage` with the payload serialised into the `data` field:

```cpp
#include "ca_msgs/msg/local_generic_message.hpp"

std_msgs::msg::String payload;
payload.data = "hello";

rclcpp::Serialization<std_msgs::msg::String> ser;
rclcpp::SerializedMessage serialized;
ser.serialize_message(&payload, &serialized);

ca_msgs::msg::LocalGenericMessage out;
out.type     = "my_type";
out.receiver = "drone1";           // empty string → broadcast to all agents
out.data.assign(
  serialized.get_rcl_serialized_message().buffer,
  serialized.get_rcl_serialized_message().buffer +
  serialized.get_rcl_serialized_message().buffer_length);

gateway_out_pub_->publish(out);
```

### API summary

| Method | Description |
|---|---|
| `CAGatewayClient(node)` | Connects to the `register_module` service derived from the node namespace |
| `register_module<T>(type, name, cb)` | Registers the module and installs a typed deserialization callback |
| `get_subscriber_count()` | Returns the number of active local subscriptions |
| `clear()` | Removes all active subscriptions |

### cs4home usage pattern

In a cs4home behavior, `CAGatewayClient` lives inside the `Core` component and is initialised in `configure()`:

```cpp
// my_behavior_core.hpp
#include "ca_structure/ca_gateway_client.hpp"
#include "cs4home_core/Core.hpp"

class MyBehaviorCore : public cs4home_core::Core {
  ca_structure::CAGatewayClient ca_client_;
  ...
};

// my_behavior_core.cpp
bool MyBehaviorCore::configure() {
  ca_client_ = ca_structure::CAGatewayClient(parent_);

  ca_client_.register_module<MyMsg>(
    "my_msg_type", "my_behavior",
    [this](const MyMsg & msg, const std::string & sender) {
      handle_peer_message(msg, sender);
    });
  return true;
}
```

## CA_GatewayClientAfferent

`ca_structure::CA_GatewayClientAfferent` is the cs4home-native alternative to `CAGatewayClient`. It extends `cs4home_core::Afferent` and integrates with the cognitive module lifecycle. Module registrations are driven entirely by a YAML config file, so no code changes are needed to add or remove subscribed message types.

### 1. Define the modules file

`config/client_modules.yaml`:

```yaml
modules:
  - type: "perception"
    module_name: "drone0_perception"
  - type: "planning"
    module_name: "drone0_planning"
```

Fields:

- `type` — message type string used by the gateway for routing
- `module_name` — human-readable identifier logged during registration

### 2. Set the parameter on the parent node

```yaml
my_cognitive_module:
  ros__parameters:
    CA_GatewayClientAfferent.config_file: "/path/to/config/client_modules.yaml"
```

### 3. Instantiate inside a CognitiveModule

```cpp
#include "ca_structure/ca_gateway_client_afferent.hpp"
#include "cs4home_core/CognitiveModule.hpp"

class MyBehavior : public cs4home_core::CognitiveModule {
  CallbackReturnT on_configure(const rclcpp_lifecycle::State &) override {
    auto self = std::dynamic_pointer_cast<rclcpp_lifecycle::LifecycleNode>(shared_from_this());

    // CA_GatewayClientAfferent reads the YAML file, calls register_module
    // for each entry, and subscribes to the returned local topics.
    afferent_ = std::make_shared<ca_structure::CA_GatewayClientAfferent>(self);
    afferent_->configure();

    // Receive messages on the first registered topic via CALLBACK mode
    afferent_->set_mode(
      0, cs4home_core::Afferent::CALLBACK,
      [this](std::shared_ptr<rclcpp::SerializedMessage> raw) {
        auto msg = afferent_->get_msg<ca_msgs::msg::LocalGenericMessage>(raw);
        // deserialise msg->data into the actual payload type
      });

    core_ = std::make_shared<MyCore>(self);
    core_->set_afferent(afferent_);
    core_->configure();
    return CallbackReturnT::SUCCESS;
  }
};
```

### Lifecycle behaviour

`configure()` blocks until every `register_module` service call completes (or times out after 5 s per entry). It uses a dedicated callback group and a scoped `SingleThreadedExecutor` so it never deadlocks the parent executor.

### When to use each client

| | `CAGatewayClient` | `CA_GatewayClientAfferent` |
|---|---|---|
| Extends | — (plain C++ class) | `cs4home_core::Afferent` |
| Registration config | Code (calls to `register_module<T>()`) | YAML file |
| Best for | `Core` components needing typed callbacks | `Afferent` components driven by config |
| Lifecycle integration | Manual | Automatic via CognitiveModule |

## Message types

| Message / Service | Fields | Description |
|---|---|---|
| `InterAgentMessage` | `sender`, `receiver`, `type`, `data[]` | Message exchanged on the shared inter-agent topic |
| `LocalGenericMessage` | `agent`, `type`, `data[]` | Message delivered to a local module; `data` carries a serialized ROS 2 message |
| `RegisterModule` (srv) | req: `type`, `module_name` — resp: `topic`, `success` | Registers a module and returns its dedicated local topic |
