# as2_ca

Collective Awareness (CA) package for the [cs4home architecture](https://github.com/CoreSenseEU/cs4home_architecture), providing inter-agent communication infrastructure for multi-robot systems built with [Aerostack2](https://github.com/aerostack2/aerostack2) in the Inspection Testbed.

## Overview

In a multi-agent system, each agent runs an independent ROS 2 graph. `as2_ca` bridges those isolated graphs through a shared inter-agent topic, allowing agents to exchange typed messages without knowing each other's internal topics. Local modules register with the gateway to declare the message types they care about; the gateway routes incoming inter-agent messages to the appropriate local topic.

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
colcon build --symlink-install --packages-select as2_ca as2_ca_msgs
```

Dependencies:

```bash
sudo apt install libyaml-cpp-dev
sudo apt install ros-humble-rclcpp-lifecycle
```

## CA_Gateway_Node

The gateway node is the central broker on each agent. It exposes a registration service that local modules call to announce themselves, and it forwards incoming inter-agent messages to the correct local topic.

### Running the node

```bash
ros2 run as2_ca ca_gateway_node --ros-args \
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
| `/agent_to_agent` | `as2_ca_msgs/msg/InterAgentMessage` | Sub + Pub | Shared inter-agent channel |
| `<type>_in` | `as2_ca_msgs/msg/LocalGenericMessage` | Pub | Created on demand for each registered type |
| `gateway_out` | `as2_ca_msgs/msg/LocalGenericMessage` | Sub | Outgoing messages from local modules |

### Services

| Service | Type | Description |
|---|---|---|
| `register_module` | `as2_ca_msgs/srv/RegisterModule` | Registers a local module for a given type; returns the local topic to subscribe to |

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

## CA_Gateway_Client

`CAGatewayClient` is a C++ helper class that hides the registration protocol. It is intended for modules that need a straightforward way to communicate through the gateway. It is a special type of afferent.

### Usage

```cpp
#include "as2_ca/ca_gateway_client.hpp"

// Attach to any rclcpp::Node
auto client = std::make_shared<as2_ca::CAGatewayClient>(node);

// Register for a message type and provide a typed callback.
// The client calls the register_module service, then subscribes to
// the returned topic and deserialises the payload automatically.
client->register_module<std_msgs::msg::String>(
  "my_type",        // message type to register for
  "my_module",      // module name (for logging)
  [](const std_msgs::msg::String & msg, const std::string & sender_agent) {
    RCLCPP_INFO(rclcpp::get_logger("demo"), "Got '%s' from %s",
      msg.data.c_str(), sender_agent.c_str());
  });

// Send a message to another agent
std_msgs::msg::String outgoing;
outgoing.data = "hello";
rclcpp::Serialization<std_msgs::msg::String> serializer;
rclcpp::SerializedMessage serialized;
serializer.serialize_message(&outgoing, &serialized);
// publish via gateway_out as a LocalGenericMessage ...
```

### API summary

| Method | Description |
|---|---|
| `CAGatewayClient(node)` | Connects to the `register_module` service derived from the node namespace |
| `register_module<T>(type, name, cb)` | Registers the module and installs a typed deserialization callback |
| `get_subscriber_count()` | Returns the number of active local subscriptions |
| `clear()` | Removes all active subscriptions |

## CA_GatewayClientAfferent

`CA_GatewayClientAfferent` is the cs4home-native alternative to `CAGatewayClient`. It integrates with the cognitive module lifecycle and reads its registration list from a YAML file so that the module's type subscriptions are fully data-driven.

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

### 3. Instantiate inside a cognitive module

```cpp
#include "as2_ca/ca_gateway_client_afferent.hpp"

class MyCore : public cs4home_core::Core {
  bool configure() override {
    // configure() reads the file, calls register_module for each entry,
    // and creates a subscription to every returned topic.
    afferent_->configure();

    // Receive LocalGenericMessage on the first registered topic
    afferent_->set_mode(
      0, cs4home_core::Afferent::CALLBACK,
      [this](std::shared_ptr<rclcpp::SerializedMessage> msg) {
        auto local_msg =
          afferent_->get_msg<as2_ca_msgs::msg::LocalGenericMessage>(msg);
        // deserialise local_msg->data into the actual payload type
      });
    return true;
  }
};

// In the CognitiveModule on_configure():
afferent_ = std::make_shared<as2_ca::CA_GatewayClientAfferent>(
  shared_from_this());
core_->set_afferent(afferent_);
```

### Lifecycle behaviour

`configure()` blocks until every `register_module` service call completes (or times out after 5 s per entry). It uses a dedicated callback group and a scoped `SingleThreadedExecutor` so it never deadlocks the parent executor.

## Message types

| Message / Service | Fields | Description |
|---|---|---|
| `InterAgentMessage` | `sender`, `receiver`, `type`, `data[]` | Message exchanged on the shared inter-agent topic |
| `LocalGenericMessage` | `agent`, `type`, `data[]` | Message delivered to a local module; `data` carries a serialized ROS 2 message |
| `RegisterModule` (srv) | req: `type`, `module_name` — resp: `topic` | Registers a module and returns its dedicated local topic |
