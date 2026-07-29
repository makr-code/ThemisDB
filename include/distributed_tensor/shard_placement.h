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

/// @defgroup shard_placement Shard Placement Strategy
/// @brief Factorization-aware shard placement for distributed tensor artifacts.
/// @{

/// Node capacity information for placement decisions.
struct NodeCapacity {
  /// Node identifier.
  std::string node_id;

  /// Available storage capacity in bytes.
  uint64_t available_capacity_bytes = 0;

  /// Node tier ("hot", "warm", "cold").
  std::string tier_level = "warm";

  /// Hardware accelerator type available (e.g., "GPU", "TPU", "CPU").
  std::string accelerator_type = "CPU";

  /// Current bandwidth utilization (0.0 to 1.0).
  float bandwidth_utilization = 0.0f;

  /// Network latency to cluster center in milliseconds.
  uint32_t network_latency_ms = 0;

  /// Topology zone or rack identifier for affinity placement.
  std::string topology_zone;

  /// Current load factor (0.0 to 1.0).
  float load_factor = 0.0f;

  /// Custom metadata for placement policies.
  std::unordered_map<std::string, std::string> custom_metadata;
};

/// Factorization-aware placement metadata.
///
/// Describes how tensor factorizations affect shard placement decisions.
struct FactorizationPlacementHint {
  /// Factorization type (e.g., "TensorTrain", "HierarchicalTucker", "none").
  std::string factorization_type;

  /// Core ranks or factor dimensions (e.g., [10, 20, 15] for TT).
  std::vector<uint32_t> factor_dimensions;

  /// Whether factor cores should be co-located for efficient computation.
  bool prefer_colocated_factors = true;

  /// If true, place frequently-accessed factors on higher-tier nodes.
  bool prefer_hot_tier_for_frequent_factors = true;

  /// Custom factorization metadata.
  std::unordered_map<std::string, std::string> custom_metadata;
};

/// Placement constraint enumeration.
///
/// Specifies constraints that must be satisfied during shard placement.
enum class PlacementConstraint {
  /// No special constraints.
  NONE,

  /// Require replication across different zones (availability).
  CROSS_ZONE_REPLICATION,

  /// Require locality: keep shards on same node if possible.
  LOCAL_AFFINITY,

  /// Require proximity to a specific node for data locality.
  PROXIMITY_TO_COMPUTE,

  /// Hardware accelerator required (e.g., GPU placement).
  ACCELERATOR_REQUIRED,

  /// Tier-aware placement (e.g., hot vs. cold tier).
  TIER_AWARE,
};

/// Shard placement strategy result.
///
/// Represents the computed placement plan for an artifact's shards.
struct PlacementPlan {
  /// Artifact ID for which this plan was computed.
  std::string artifact_id;

  /// Computed shard placements.
  std::vector<ShardPlacement> shard_placements;

  /// Placement confidence score (0.0 to 1.0).
  float confidence_score = 0.5f;

  /// Estimated placement cost (arbitrary units).
  uint64_t estimated_cost = 0;

  /// Reason for placement decisions (human-readable description).
  std::string placement_rationale;

  /// If false, the requested placement violates hard constraints and must not run.
  bool satisfies_hard_constraints = true;

  /// If true, the plan is usable only with degraded confidence/coverage.
  bool is_degraded = false;

  /// Human-readable diagnostics for placement callers and tests.
  std::vector<std::string> placement_warnings;

  /// Timestamp when plan was computed (ISO 8601).
  std::string computed_at;
};

/// Shard placement strategy interface.
///
/// Computes factorization-aware shard placements respecting topology,
/// capacity, and rebuild constraints.
class ShardPlacementStrategy {
 public:
  /// Construct a shard placement strategy.
  ShardPlacementStrategy() = default;

  /// Copy constructor deleted.
  ShardPlacementStrategy(const ShardPlacementStrategy&) = delete;

  /// Move constructor.
  ShardPlacementStrategy(ShardPlacementStrategy&&) noexcept = default;

  /// Assignment operator deleted.
  ShardPlacementStrategy& operator=(const ShardPlacementStrategy&) = delete;

  /// Move assignment operator.
  ShardPlacementStrategy& operator=(ShardPlacementStrategy&&) noexcept = default;

  /// Virtual destructor.
  virtual ~ShardPlacementStrategy() = default;

  /// Compute shard placement for an artifact.
  ///
  /// @param artifact_id Artifact to place.
  /// @param num_shards Number of shards to create.
  /// @param shard_size_bytes Size of each shard in bytes.
  /// @param available_nodes Available nodes for placement.
  /// @param factorization_hint Factorization-aware placement hints (optional).
  /// @param constraints Placement constraints to satisfy.
  /// @return Computed placement plan.
  virtual PlacementPlan compute_placement(
      const std::string& artifact_id,
      uint32_t num_shards,
      uint64_t shard_size_bytes,
      const std::vector<NodeCapacity>& available_nodes,
      const std::optional<FactorizationPlacementHint>& factorization_hint =
          std::nullopt,
      PlacementConstraint constraints = PlacementConstraint::NONE) noexcept = 0;

  /// Validate that a placement plan satisfies constraints.
  ///
  /// @param plan Placement plan to validate.
  /// @param constraints Constraints to verify against.
  /// @return true if plan satisfies constraints, false otherwise.
  virtual bool validate_placement(const PlacementPlan& plan,
                                   PlacementConstraint constraints) const
      noexcept = 0;

  /// Optimize an existing placement for better performance or cost.
  ///
  /// @param current_plan Current placement plan.
  /// @param available_nodes Available nodes for rebalancing.
  /// @return Optimized placement plan.
  virtual PlacementPlan optimize_placement(
      const PlacementPlan& current_plan,
      const std::vector<NodeCapacity>& available_nodes) noexcept = 0;
};

/// Default shard placement strategy.
///
/// Implements basic RAID-style placement with round-robin distribution
/// and optional factorization awareness.
class DefaultShardPlacementStrategy : public ShardPlacementStrategy {
 public:
  /// Construct the default placement strategy.
  DefaultShardPlacementStrategy() = default;

  /// Move constructor.
  DefaultShardPlacementStrategy(DefaultShardPlacementStrategy&&) noexcept =
      default;

  /// Move assignment operator.
  DefaultShardPlacementStrategy& operator=(
      DefaultShardPlacementStrategy&&) noexcept = default;

  /// Destructor.
  ~DefaultShardPlacementStrategy() override = default;

  /// Compute shard placement using round-robin with capacity awareness.
  PlacementPlan compute_placement(
      const std::string& artifact_id,
      uint32_t num_shards,
      uint64_t shard_size_bytes,
      const std::vector<NodeCapacity>& available_nodes,
      const std::optional<FactorizationPlacementHint>& factorization_hint =
          std::nullopt,
      PlacementConstraint constraints = PlacementConstraint::NONE) noexcept
      override;

  /// Validate placement plan.
  bool validate_placement(const PlacementPlan& plan,
                          PlacementConstraint constraints) const noexcept
      override;

  /// Optimize placement for load balancing.
  PlacementPlan optimize_placement(
      const PlacementPlan& current_plan,
      const std::vector<NodeCapacity>& available_nodes) noexcept override;

 private:
  /// Select best node for a shard based on capacity and constraints.
  std::string select_best_node(
      const std::vector<NodeCapacity>& available_nodes,
      uint64_t shard_size_bytes,
      PlacementConstraint constraints) noexcept;

  /// Check if a node satisfies placement constraints.
  bool satisfies_constraints(const NodeCapacity& node,
                             uint64_t shard_size_bytes,
                             PlacementConstraint constraints) const noexcept;
};

/// @}

}  // namespace distributed_tensor
}  // namespace themis
