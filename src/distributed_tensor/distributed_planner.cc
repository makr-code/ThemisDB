// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#include "distributed_tensor/distributed_planner.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace themis {
namespace distributed_tensor {

namespace {

bool lifecycle_satisfies_requirement(ArtifactLifecycleStage actual_stage,
                                     ArtifactLifecycleStage required_stage) noexcept {
  switch (required_stage) {
    case ArtifactLifecycleStage::STAGING:
      return actual_stage == ArtifactLifecycleStage::STAGING;
    case ArtifactLifecycleStage::ACTIVE:
      return actual_stage == ArtifactLifecycleStage::ACTIVE;
    case ArtifactLifecycleStage::STALE:
      return actual_stage == ArtifactLifecycleStage::ACTIVE ||
             actual_stage == ArtifactLifecycleStage::STALE;
    case ArtifactLifecycleStage::RECOVERING:
      return actual_stage == ArtifactLifecycleStage::ACTIVE ||
             actual_stage == ArtifactLifecycleStage::STALE ||
             actual_stage == ArtifactLifecycleStage::RECOVERING;
    case ArtifactLifecycleStage::DEPRECATED:
      return actual_stage == ArtifactLifecycleStage::DEPRECATED;
    case ArtifactLifecycleStage::DELETED:
      return actual_stage == ArtifactLifecycleStage::DELETED;
  }

  return false;
}

bool dependencies_allow_degraded_reads(
    const std::vector<TensorDependency>& dependencies) noexcept {
  for (const auto& dependency : dependencies) {
    if (!dependency.is_required || dependency.allow_approximation ||
        dependency.max_staleness_seconds > 0) {
      return true;
    }
  }

  return dependencies.empty();
}

}  // namespace

// DefaultDistributedTensorPlanner implementation.

DistributedRetrievalPlan
DefaultDistributedTensorPlanner::plan_tensor_retrieval(
    const ArtifactManifest& manifest,
    const std::vector<TensorDependency>& dependencies,
    RetrievalLocation preferred_location) const noexcept {
  DistributedRetrievalPlan plan;
  plan.artifact_id = manifest.artifact_id();
  plan.retrieval_location = preferred_location;
  plan.parallel_retrieval = true;
  plan.max_parallel_streams = 4;
  plan.execution_rationale = "Ready for distributed retrieval.";

  if (!manifest.is_complete()) {
    plan.can_execute = false;
    plan.parallel_retrieval = false;
    plan.max_parallel_streams = 0;
    plan.execution_rationale =
        "Manifest is incomplete; retrieval is blocked until placement and metadata are valid.";
    return plan;
  }

  if (manifest.integrity_receipt() && !manifest.integrity_receipt()->is_valid) {
    plan.can_execute = false;
    plan.parallel_retrieval = false;
    plan.max_parallel_streams = 0;
    plan.execution_rationale =
        "Integrity receipt is invalid; retrieval is blocked fail-closed.";
    return plan;
  }

  // Select retrieval strategy based on artifact and dependencies.
  plan.retrieval_strategy = select_retrieval_strategy(manifest, dependencies);

  ArtifactLifecycleStage required_stage = ArtifactLifecycleStage::ACTIVE;
  for (const auto& dependency : dependencies) {
    if (static_cast<int>(dependency.required_lifecycle_stage) >
        static_cast<int>(required_stage)) {
      required_stage = dependency.required_lifecycle_stage;
    }
  }

  const auto actual_stage = manifest.lifecycle_stage();
  if (!lifecycle_satisfies_requirement(actual_stage, required_stage)) {
    const bool allow_degraded_reads = dependencies_allow_degraded_reads(dependencies);
    if (allow_degraded_reads &&
        (actual_stage == ArtifactLifecycleStage::STALE ||
         actual_stage == ArtifactLifecycleStage::RECOVERING)) {
      plan.degraded_mode = true;
      plan.parallel_retrieval = actual_stage != ArtifactLifecycleStage::RECOVERING;
      plan.max_parallel_streams =
          actual_stage == ArtifactLifecycleStage::RECOVERING ? 1 : 2;
      plan.execution_rationale =
          "Retrieval proceeds in degraded mode because the artifact is stale or recovering.";
    } else {
      plan.can_execute = false;
      plan.parallel_retrieval = false;
      plan.max_parallel_streams = 0;
      plan.execution_rationale =
          "Artifact lifecycle stage does not satisfy dependency requirements.";
      return plan;
    }
  }

  // Collect all shard IDs to retrieve.
  for (const auto& shard : manifest.shard_placements()) {
    plan.shard_ids_to_retrieve.push_back(shard.shard_id);
  }

  if (plan.shard_ids_to_retrieve.empty()) {
    plan.can_execute = false;
    plan.parallel_retrieval = false;
    plan.max_parallel_streams = 0;
    plan.execution_rationale =
        "Manifest does not contain any retrievable shards.";
    return plan;
  }

  // Estimate retrieval time.
  plan.estimated_retrieval_time_ms =
      calculate_retrieval_time(manifest.total_size_bytes(),
                               plan.estimated_bandwidth_mbps,
                               plan.max_parallel_streams);

  return plan;
}

DistributedRetrievalPlan DefaultDistributedTensorPlanner::optimize_retrieval_plan(
    const DistributedRetrievalPlan& plan) const noexcept {
  DistributedRetrievalPlan optimized_plan = plan;

  // Optimization heuristic: reduce parallel streams if data is small.
  if (optimized_plan.estimated_retrieval_time_ms < 50) {
    optimized_plan.max_parallel_streams = 1;
  } else if (optimized_plan.estimated_retrieval_time_ms < 200) {
    optimized_plan.max_parallel_streams = 2;
  }

  if (optimized_plan.degraded_mode) {
    optimized_plan.max_parallel_streams =
        std::min<uint32_t>(optimized_plan.max_parallel_streams, 2);
  }

  // Recalculate estimated time with optimized parameters.
  // (In this simplified model, we don't have access to data size,
  // so we'll use the original estimate as a baseline.)

  return optimized_plan;
}

bool DefaultDistributedTensorPlanner::is_tensor_available(
    const ArtifactManifest& manifest,
    ArtifactLifecycleStage required_stage) const noexcept {
  if (!manifest.is_complete() || manifest.num_shards() == 0) {
    return false;
  }

  if (manifest.integrity_receipt() && !manifest.integrity_receipt()->is_valid) {
    return false;
  }

  return lifecycle_satisfies_requirement(manifest.lifecycle_stage(), required_stage);
}

uint64_t DefaultDistributedTensorPlanner::estimate_retrieval_cost(
    const ArtifactManifest& manifest,
    RetrievalStrategy strategy,
    RetrievalLocation location) const noexcept {
  // Cost estimation heuristic:
  // Base cost = data size in MB
  // Strategy multiplier: SUMMARY_FIRST=0.1, EXACT=1.0, APPROXIMATE=0.3, LAZY=0.05, CACHED=0.2
  // Location multiplier: LOCAL_CACHE=1.0, HOT_TIER=2.0, WARM_TIER=5.0, COLD_TIER=10.0

  uint64_t base_cost = manifest.total_size_bytes() / (1024 * 1024);

  float strategy_multiplier = 1.0f;
  switch (strategy) {
    case RetrievalStrategy::SUMMARY_FIRST:
      strategy_multiplier = 0.1f;
      break;
    case RetrievalStrategy::EXACT:
      strategy_multiplier = 1.0f;
      break;
    case RetrievalStrategy::APPROXIMATE:
      strategy_multiplier = 0.3f;
      break;
    case RetrievalStrategy::LAZY:
      strategy_multiplier = 0.05f;
      break;
    case RetrievalStrategy::CACHED:
      strategy_multiplier = 0.2f;
      break;
  }

  float location_multiplier = 5.0f; // Default WARM_TIER
  switch (location) {
    case RetrievalLocation::LOCAL_CACHE:
      location_multiplier = 1.0f;
      break;
    case RetrievalLocation::HOT_TIER:
      location_multiplier = 2.0f;
      break;
    case RetrievalLocation::WARM_TIER:
      location_multiplier = 5.0f;
      break;
    case RetrievalLocation::COLD_TIER:
      location_multiplier = 10.0f;
      break;
    case RetrievalLocation::ANY_TIER:
      location_multiplier = 3.0f; // Average
      break;
  }

  return static_cast<uint64_t>(base_cost * strategy_multiplier *
                               location_multiplier);
}

RetrievalStrategy DefaultDistributedTensorPlanner::select_retrieval_strategy(
    const ArtifactManifest& manifest,
    const std::vector<TensorDependency>& dependencies) const noexcept {
  // Strategy selection heuristic:
  // 1. If artifact is very large (> 1GB), prefer SUMMARY_FIRST.
  // 2. If dependencies allow approximation, prefer APPROXIMATE.
  // 3. For derived artifacts, prefer LAZY.
  // 4. Default: EXACT.

  if (manifest.total_size_bytes() > 1000000000ULL) {
    return RetrievalStrategy::SUMMARY_FIRST;
  }

  for (const auto& dep : dependencies) {
    if (dep.allow_approximation) {
      return RetrievalStrategy::APPROXIMATE;
    }
  }

  if (manifest.artifact_class() == ArtifactClass::DERIVED) {
    return RetrievalStrategy::LAZY;
  }

  return RetrievalStrategy::EXACT;
}

uint64_t DefaultDistributedTensorPlanner::calculate_retrieval_time(
    uint64_t data_size,
    float bandwidth_mbps,
    uint32_t parallel_streams) const noexcept {
  // Estimate retrieval time in milliseconds.
  // Formula: (data_size_mb / (bandwidth_mbps * parallel_streams)) * 1000

  if (bandwidth_mbps == 0.0f || parallel_streams == 0) {
    return 0;
  }

  uint64_t data_size_mb = data_size / (1024 * 1024);
  if (data_size_mb == 0) {
    return 10; // Minimum 10ms.
  }

  uint64_t time_ms = (data_size_mb * 1000) /
                     static_cast<uint64_t>(bandwidth_mbps * parallel_streams);

  return time_ms > 0 ? time_ms : 10; // Minimum 10ms.
}

}  // namespace distributed_tensor
}  // namespace themis
