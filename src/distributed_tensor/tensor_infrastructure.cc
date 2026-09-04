// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#include "distributed_tensor/tensor_infrastructure.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace themis {
namespace distributed_tensor {

// Helper: Format current timestamp as ISO 8601 string.
static std::string get_iso8601_timestamp() noexcept {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);
  std::ostringstream oss;
  oss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
  return oss.str();
}

// DefaultTensorInfrastructureManager implementation.

DefaultTensorInfrastructureManager::DefaultTensorInfrastructureManager()
    : stripe_transport_{
          .protocol = "grpc",
          .compression_enabled = true,
          .compression_algorithm = "zstd",
          .encryption_enabled = true,
          .encryption_algorithm = "AES256",
          .parallel_streams = 4,
          .timeout_ms = 30000,
          .max_retries = 3,
          .backoff_strategy = "exponential",
      } {}

bool DefaultTensorInfrastructureManager::register_node(
    const ClusterNode& node) noexcept {
  if (node.node_id.empty()) {
    return false;
  }

  // Check if node already exists.
  if (nodes_.find(node.node_id) != nodes_.end()) {
    return false;
  }

  // Register the node.
  ClusterNode registered_node = node;
  registered_node.last_heartbeat_at = get_iso8601_timestamp();
  if (registered_node.status == NodeStatus::UNKNOWN) {
    registered_node.status = NodeStatus::HEALTHY;
  }

  nodes_[node.node_id] = registered_node;
  return true;
}

bool DefaultTensorInfrastructureManager::unregister_node(
    const std::string& node_id) noexcept {
  auto it = nodes_.find(node_id);
  if (it == nodes_.end()) {
    return false;
  }

  nodes_.erase(it);
  return true;
}

std::optional<ClusterNode> DefaultTensorInfrastructureManager::get_node(
    const std::string& node_id) const noexcept {
  auto it = nodes_.find(node_id);
  if (it != nodes_.end()) {
    return it->second;
  }
  return std::nullopt;
}

std::vector<ClusterNode> DefaultTensorInfrastructureManager::list_nodes()
    const noexcept {
  std::vector<ClusterNode> all_nodes = {};

  for (const auto& [node_id, node] : nodes_) {
    all_nodes.push_back(node);
  }
  return all_nodes;
}

std::vector<ClusterNode>
DefaultTensorInfrastructureManager::get_healthy_nodes() const noexcept {
  std::vector<ClusterNode> healthy_nodes = {};

  for (const auto& [node_id, node] : nodes_) {
    if (node.status == NodeStatus::HEALTHY ||
        node.status == NodeStatus::DEGRADED) {
      healthy_nodes.push_back(node);
    }
  }
  return healthy_nodes;
}

bool DefaultTensorInfrastructureManager::update_node_status(
    const std::string& node_id,
    NodeStatus status) noexcept {
  auto it = nodes_.find(node_id);
  if (it == nodes_.end()) {
    return false;
  }

  it->second.status = status;
  it->second.last_heartbeat_at = get_iso8601_timestamp();
  return true;
}

const StripeTransport&
DefaultTensorInfrastructureManager::get_stripe_transport() const noexcept {
  return stripe_transport_;
}

void DefaultTensorInfrastructureManager::set_stripe_transport(
    StripeTransport transport) noexcept {
  stripe_transport_ = std::move(transport);
}

bool DefaultTensorInfrastructureManager::is_node_available(
    const std::string& node_id,
    uint64_t required_capacity) const noexcept {
  auto node = get_node(node_id);
  if (!node) {
    return false;
  }

  // Node must be healthy or degraded (not unavailable or offline).
  if (node->status != NodeStatus::HEALTHY &&
      node->status != NodeStatus::DEGRADED) {
    return false;
  }

  // Check available storage capacity.
  uint64_t available_capacity =
      node->total_storage_bytes - node->used_storage_bytes;

  return available_capacity >= required_capacity;
}

}  // namespace distributed_tensor
}  // namespace themis
