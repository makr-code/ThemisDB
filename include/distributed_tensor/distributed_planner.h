// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "distributed_tensor/artifact_manifest.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace themis {
namespace distributed_tensor {

/// @defgroup distributed_planner Distributed Query Planning
/// @brief Query planner integration for distributed tensor retrieval.
/// @{

/// Retrieval strategy enumeration.
///
/// Specifies how tensor data should be retrieved across the cluster.
enum class RetrievalStrategy {
  /// Summary-first retrieval: fetch summaries first, then exact data on demand.
  SUMMARY_FIRST,

  /// Exact retrieval: fetch complete tensor data.
  EXACT,

  /// Approximate retrieval: fetch quantized or lossy representation.
  APPROXIMATE,

  /// Lazy retrieval: defer loading until first access.
  LAZY,

  /// Cached retrieval: prefer cached copies over primary.
  CACHED,
};

/// Retrieval location preference enumeration.
///
/// Specifies where data should be fetched from.
enum class RetrievalLocation {
  /// Retrieve from local cache if available.
  LOCAL_CACHE,

  /// Retrieve from hot-tier storage.
  HOT_TIER,

  /// Retrieve from warm-tier storage.
  WARM_TIER,

  /// Retrieve from cold-tier storage (may be slow).
  COLD_TIER,

  /// Retrieve from any available location.
  ANY_TIER,
};

/// Distributed tensor retrieval plan.
///
/// Specifies how to retrieve a tensor artifact from distributed storage.
struct DistributedRetrievalPlan {
  /// Artifact ID to retrieve.
  std::string artifact_id;

  /// Retrieval strategy to use.
  RetrievalStrategy retrieval_strategy;

  /// Preferred retrieval location.
  RetrievalLocation retrieval_location;

  /// List of shard IDs to retrieve (empty = all shards).
  std::vector<std::string> shard_ids_to_retrieve;

  /// If true, retrieve in parallel; if false, sequential.
  bool parallel_retrieval = true;

  /// Maximum number of parallel retrieval streams.
  uint32_t max_parallel_streams = 4;

  /// Estimated retrieval time in milliseconds.
  uint64_t estimated_retrieval_time_ms = 0;

  /// Network bandwidth estimate in MB/s.
  float estimated_bandwidth_mbps = 100.0f;

  /// Custom retrieval parameters.
  std::unordered_map<std::string, std::string> custom_parameters;
};

/// Tensor dependency in query execution plan.
///
/// Tracks tensor artifacts required by a query stage.
struct TensorDependency {
  /// Artifact ID of the dependency.
  std::string artifact_id;

  /// Lifecycle stage required (ACTIVE, STALE, etc.).
  ArtifactLifecycleStage required_lifecycle_stage;

  /// Whether this is a required or optional dependency.
  bool is_required = true;

  /// Freshn ess requirement in seconds (0 = any version).
  uint32_t max_staleness_seconds = 0;

  /// Whether dependency can use approximate/quantized version.
  bool allow_approximation = false;
};

/// Distributed tensor planner interface.
///
/// Plans distributed tensor retrieval to integrate with the query engine.
class DistributedTensorPlanner {
 public:
  /// Construct a distributed tensor planner.
  DistributedTensorPlanner() = default;

  /// Copy constructor deleted.
  DistributedTensorPlanner(const DistributedTensorPlanner&) = delete;

  /// Move constructor.
  DistributedTensorPlanner(DistributedTensorPlanner&&) noexcept = default;

  /// Assignment operator deleted.
  DistributedTensorPlanner& operator=(const DistributedTensorPlanner&) = delete;

  /// Move assignment operator.
  DistributedTensorPlanner& operator=(DistributedTensorPlanner&&) noexcept =
      default;

  /// Virtual destructor.
  virtual ~DistributedTensorPlanner() = default;

  /// Plan tensor retrieval for a query stage.
  ///
  /// @param manifest Artifact manifest.
  /// @param dependencies Tensor dependencies for this stage.
  /// @param preferred_location Preferred retrieval location.
  /// @return Distributed retrieval plan.
  virtual DistributedRetrievalPlan plan_tensor_retrieval(
      const ArtifactManifest& manifest,
      const std::vector<TensorDependency>& dependencies,
      RetrievalLocation preferred_location =
          RetrievalLocation::ANY_TIER) const noexcept = 0;

  /// Optimize retrieval plan for performance or cost.
  ///
  /// @param plan Current retrieval plan.
  /// @return Optimized retrieval plan.
  virtual DistributedRetrievalPlan optimize_retrieval_plan(
      const DistributedRetrievalPlan& plan) const noexcept = 0;

  /// Check if tensor is available for retrieval.
  ///
  /// @param manifest Artifact manifest.
  /// @param required_stage Required lifecycle stage.
  /// @return true if tensor is available, false otherwise.
  virtual bool is_tensor_available(
      const ArtifactManifest& manifest,
      ArtifactLifecycleStage required_stage =
          ArtifactLifecycleStage::ACTIVE) const noexcept = 0;

  /// Estimate cost of retrieving a tensor.
  ///
  /// @param manifest Artifact manifest.
  /// @param strategy Retrieval strategy.
  /// @param location Retrieval location.
  /// @return Estimated cost in arbitrary units.
  virtual uint64_t estimate_retrieval_cost(
      const ArtifactManifest& manifest,
      RetrievalStrategy strategy,
      RetrievalLocation location) const noexcept = 0;
};

/// Default distributed tensor planner implementation.
///
/// Provides basic tensor retrieval planning integrated with the query engine.
class DefaultDistributedTensorPlanner : public DistributedTensorPlanner {
 public:
  /// Construct the default distributed tensor planner.
  DefaultDistributedTensorPlanner() = default;

  /// Move constructor.
  DefaultDistributedTensorPlanner(DefaultDistributedTensorPlanner&&) noexcept =
      default;

  /// Move assignment operator.
  DefaultDistributedTensorPlanner& operator=(
      DefaultDistributedTensorPlanner&&) noexcept = default;

  /// Destructor.
  ~DefaultDistributedTensorPlanner() override = default;

  /// Plan tensor retrieval.
  DistributedRetrievalPlan plan_tensor_retrieval(
      const ArtifactManifest& manifest,
      const std::vector<TensorDependency>& dependencies,
      RetrievalLocation preferred_location = RetrievalLocation::ANY_TIER)
      const noexcept override;

  /// Optimize retrieval plan.
  DistributedRetrievalPlan optimize_retrieval_plan(
      const DistributedRetrievalPlan& plan) const noexcept override;

  /// Check tensor availability.
  bool is_tensor_available(
      const ArtifactManifest& manifest,
      ArtifactLifecycleStage required_stage =
          ArtifactLifecycleStage::ACTIVE) const noexcept override;

  /// Estimate retrieval cost.
  uint64_t estimate_retrieval_cost(
      const ArtifactManifest& manifest,
      RetrievalStrategy strategy,
      RetrievalLocation location) const noexcept override;

 private:
  /// Select retrieval strategy based on artifact and dependencies.
  RetrievalStrategy select_retrieval_strategy(
      const ArtifactManifest& manifest,
      const std::vector<TensorDependency>& dependencies) const noexcept;

  /// Calculate estimated retrieval time.
  uint64_t calculate_retrieval_time(
      uint64_t data_size,
      float bandwidth_mbps,
      uint32_t parallel_streams) const noexcept;
};

/// @}

}  // namespace distributed_tensor
}  // namespace themis
