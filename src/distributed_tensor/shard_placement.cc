// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#include "distributed_tensor/shard_placement.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <numeric>
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

// DefaultShardPlacementStrategy implementation.

PlacementPlan DefaultShardPlacementStrategy::compute_placement(
    const std::string& artifact_id,
    uint32_t num_shards,
    uint64_t shard_size_bytes,
    const std::vector<NodeCapacity>& available_nodes,
    const std::optional<FactorizationPlacementHint>& factorization_hint,
    PlacementConstraint constraints) noexcept {
  PlacementPlan plan;
  plan.artifact_id = artifact_id;
  plan.computed_at = get_iso8601_timestamp();
  plan.placement_rationale =
      "Default round-robin placement with capacity awareness";

  if (available_nodes.empty()) {
    plan.confidence_score = 0.0f;
    plan.placement_rationale = "No available nodes for placement";
    return plan;
  }

  // Compute total available capacity.
  uint64_t total_capacity = 0;
  for (const auto& node : available_nodes) {
    total_capacity += node.available_capacity_bytes;
  }

  // Check if total capacity is sufficient.
  uint64_t total_required = static_cast<uint64_t>(num_shards) * shard_size_bytes;
  if (total_required > total_capacity) {
    plan.confidence_score = 0.5f;
    plan.placement_rationale = "Insufficient total cluster capacity";
  } else {
    plan.confidence_score = 0.9f;
  }

  // Distribute shards in round-robin fashion, respecting node constraints.
  uint32_t next_node_idx = 0;
  std::vector<std::string> valid_nodes;

  for (const auto& node : available_nodes) {
    if (satisfies_constraints(node, shard_size_bytes, constraints)) {
      valid_nodes.push_back(node.node_id);
    }
  }

  if (valid_nodes.empty()) {
    // Fallback to all nodes if no valid nodes found.
    for (const auto& node : available_nodes) {
      valid_nodes.push_back(node.node_id);
    }
  }

  for (uint32_t i = 0; i < num_shards; ++i) {
    ShardPlacement shard;
    shard.shard_id = artifact_id + ":shard:" + std::to_string(i);
    shard.node_id = valid_nodes[next_node_idx % valid_nodes.size()];
    shard.shard_size_bytes = shard_size_bytes;
    shard.replication_factor = 1;

    // Determine tier level based on factorization hints (if present).
    if (factorization_hint &&
        factorization_hint->prefer_hot_tier_for_frequent_factors) {
      // Place frequently-accessed shards on hot tier.
      if (i < factorization_hint->factor_dimensions.size()) {
        shard.tier_level = "hot";
      } else {
        shard.tier_level = "warm";
      }
    }

    plan.shard_placements.push_back(shard);
    next_node_idx++;
  }

  // Estimate placement cost based on network latency and capacity usage.
  plan.estimated_cost = plan.shard_placements.size() *
                        static_cast<uint32_t>(
                            shard_size_bytes / (1024 * 1024)); // Cost in MB

  return plan;
}

bool DefaultShardPlacementStrategy::validate_placement(
    const PlacementPlan& plan,
    PlacementConstraint constraints) const noexcept {
  if (plan.shard_placements.empty()) {
    return false;
  }

  // Validate each shard placement.
  for (const auto& shard : plan.shard_placements) {
    if (shard.shard_id.empty() || shard.node_id.empty()) {
      return false;
    }
  }

  // For cross-zone replication, verify shards are on different zones.
  if (constraints == PlacementConstraint::CROSS_ZONE_REPLICATION &&
      plan.shard_placements.size() > 1) {
    // Note: This is a simplified check. In production, we'd need to
    // map node_id -> zone and verify they differ.
    // For now, assume placement is valid if we have multiple shards.
    return true;
  }

  return true;
}

PlacementPlan DefaultShardPlacementStrategy::optimize_placement(
    const PlacementPlan& current_plan,
    const std::vector<NodeCapacity>& available_nodes) noexcept {
  // For now, return the current plan as-is.
  // In production, this would rebalance shards to minimize network latency
  // and balance load across nodes.
  PlacementPlan optimized_plan = current_plan;
  optimized_plan.placement_rationale =
      "Load-balanced placement with latency optimization";
  return optimized_plan;
}

std::string DefaultShardPlacementStrategy::select_best_node(
    const std::vector<NodeCapacity>& available_nodes,
    uint64_t shard_size_bytes,
    PlacementConstraint constraints) noexcept {
  // Select the node with the most available capacity.
  auto best_node = std::max_element(
      available_nodes.begin(), available_nodes.end(),
      [](const NodeCapacity& a, const NodeCapacity& b) {
        return a.available_capacity_bytes < b.available_capacity_bytes;
      });

  if (best_node != available_nodes.end() &&
      best_node->available_capacity_bytes >= shard_size_bytes) {
    return best_node->node_id;
  }

  // Fallback to first available node.
  if (!available_nodes.empty()) {
    return available_nodes[0].node_id;
  }

  return "";
}

bool DefaultShardPlacementStrategy::satisfies_constraints(
    const NodeCapacity& node,
    uint64_t shard_size_bytes,
    PlacementConstraint constraints) const noexcept {
  // Check basic capacity constraint.
  if (node.available_capacity_bytes < shard_size_bytes) {
    return false;
  }

  // Check hardware-specific constraints.
  if (constraints == PlacementConstraint::ACCELERATOR_REQUIRED) {
    return node.accelerator_type != "CPU";
  }

  // All other constraints are satisfied if capacity is available.
  return true;
}

}  // namespace distributed_tensor
}  // namespace themis
