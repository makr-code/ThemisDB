/**
 * @file shard_placement.h
 * @brief Shard placement strategy for distributed tensor operations.
 *
 * Defines placement policies that map logical tensor shards to physical
 * nodes based on capacity, topology, and user-specified constraints.
 */

// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "distributed_tensor/artifact_manifest.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace themis {
namespace distributed_tensor {


struct NodeCapacity {
  std::string node_id;

  uint64_t available_capacity_bytes = 0;

  std::string tier_level = "warm";

  std::string accelerator_type = "CPU";

  float bandwidth_utilization = 0.0f;

  uint32_t network_latency_ms = 0;

  std::string topology_zone;

  float load_factor = 0.0f;

  std::unordered_map<std::string, std::string> custom_metadata;
};

struct FactorizationPlacementHint {
  std::string factorization_type;

  std::vector<uint32_t> factor_dimensions;

  bool prefer_colocated_factors = true;

  bool prefer_hot_tier_for_frequent_factors = true;

  std::unordered_map<std::string, std::string> custom_metadata;
};

enum class PlacementConstraint {
  NONE,

  CROSS_ZONE_REPLICATION,

  LOCAL_AFFINITY,

  PROXIMITY_TO_COMPUTE,

  ACCELERATOR_REQUIRED,

  TIER_AWARE,
};

struct PlacementPlan {
  std::string artifact_id;

  std::vector<ShardPlacement> shard_placements;

  float confidence_score = 0.5f;

  uint64_t estimated_cost = 0;

  std::string placement_rationale;

  bool satisfies_hard_constraints = true;

  bool is_degraded = false;

  std::vector<std::string> placement_warnings;

  std::string computed_at;
};

class ShardPlacementStrategy {
 public:
  ShardPlacementStrategy() = default;

  ShardPlacementStrategy(const ShardPlacementStrategy&) = delete;

  ShardPlacementStrategy(ShardPlacementStrategy&&) noexcept = default;

  ShardPlacementStrategy& operator=(const ShardPlacementStrategy&) = delete;

  ShardPlacementStrategy& operator=(ShardPlacementStrategy&&) noexcept = default;

  /**
   * @brief Shard Placement Strategy.
   * @return Return value.
   */
  virtual ~ShardPlacementStrategy() = default;

  virtual PlacementPlan compute_placement(
      const std::string& artifact_id,
      uint32_t num_shards,
      uint64_t shard_size_bytes,
      const std::vector<NodeCapacity>& available_nodes,
      const std::optional<FactorizationPlacementHint>& factorization_hint =
          std::nullopt,
      PlacementConstraint constraints = PlacementConstraint::NONE) noexcept = 0;

  /**
   * @brief Validate placement.
   * @param[in] plan Input parameter.
   * @param[in] constraints Input parameter.
   * @return True when the operation succeeds.
   * @note Exception safety: noexcept.
   */
  virtual bool validate_placement(const PlacementPlan& plan,
                                   PlacementConstraint constraints) const
      noexcept = 0;

  /**
   * @brief Optimize placement.
   * @param[in] current_plan Input parameter.
   * @param[in] available_nodes Input parameter.
   * @return Return value.
   * @note Exception safety: noexcept.
   */
  virtual PlacementPlan optimize_placement(
      const PlacementPlan& current_plan,
      const std::vector<NodeCapacity>& available_nodes) noexcept = 0;
};

class DefaultShardPlacementStrategy : public ShardPlacementStrategy {
 public:
  DefaultShardPlacementStrategy() = default;

  DefaultShardPlacementStrategy(DefaultShardPlacementStrategy&&) noexcept =
      default;

  DefaultShardPlacementStrategy& operator=(
      DefaultShardPlacementStrategy&&) noexcept = default;

  ~DefaultShardPlacementStrategy() override = default;

  PlacementPlan compute_placement(
      const std::string& artifact_id,
      uint32_t num_shards,
      uint64_t shard_size_bytes,
      const std::vector<NodeCapacity>& available_nodes,
      const std::optional<FactorizationPlacementHint>& factorization_hint =
          std::nullopt,
      PlacementConstraint constraints = PlacementConstraint::NONE) noexcept
      override;

  bool validate_placement(const PlacementPlan& plan,
                          PlacementConstraint constraints) const noexcept
      override;

  PlacementPlan optimize_placement(
      const PlacementPlan& current_plan,
      const std::vector<NodeCapacity>& available_nodes) noexcept override;

 private:
  /**
   * @brief Select best node.
   * @param[in] available_nodes Input parameter.
   * @param[in] shard_size_bytes Input parameter.
   * @param[in] constraints Input parameter.
   * @return Return value.
   * @note Exception safety: noexcept.
   */
  std::string select_best_node(
      const std::vector<NodeCapacity>& available_nodes,
      uint64_t shard_size_bytes,
      PlacementConstraint constraints) noexcept;

  /**
   * @brief Satisfies constraints.
   * @param[in] node Input parameter.
   * @param[in] shard_size_bytes Input parameter.
   * @param[in] constraints Input parameter.
   * @return True when the operation succeeds.
   * @note Exception safety: noexcept.
   */
  bool satisfies_constraints(const NodeCapacity& node,
                             uint64_t shard_size_bytes,
                             PlacementConstraint constraints) const noexcept;
};


}  // namespace distributed_tensor
}  // namespace themis
