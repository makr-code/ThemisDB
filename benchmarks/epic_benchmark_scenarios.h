// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include "distributed_tensor/artifact_manifest.h"
#include "distributed_tensor/distributed_planner.h"
#include "distributed_tensor/integrity_verification.h"
#include "distributed_tensor/recovery_manager.h"
#include "distributed_tensor/shard_placement.h"

#include <cstdint>
#include <string>
#include <vector>

namespace themis::bench::epic {

inline distributed_tensor::ArtifactManifest make_manifest(
    uint32_t shard_count,
    uint64_t shard_size_bytes,
    distributed_tensor::ArtifactClass artifact_class =
        distributed_tensor::ArtifactClass::PRIMARY,
    distributed_tensor::ArtifactLifecycleStage lifecycle_stage =
        distributed_tensor::ArtifactLifecycleStage::ACTIVE,
    const std::string& recovery_strategy = "replication") {
  using namespace distributed_tensor;

  ArtifactManifest manifest("bench-artifact", artifact_class);
  manifest.set_version("bench-v1");
  manifest.set_total_size_bytes(static_cast<uint64_t>(shard_count) *
                                shard_size_bytes);
  manifest.set_lifecycle_stage(lifecycle_stage);
  manifest.set_recovery_strategy(recovery_strategy);

  if (artifact_class == ArtifactClass::PRIMARY) {
    manifest.set_content_hash("artifact-hash-primary");
  } else if (artifact_class == ArtifactClass::DERIVED) {
    manifest.set_parent_artifact_id("bench-parent");
  }

  for (uint32_t shard_index = 0; shard_index < shard_count; ++shard_index) {
    ShardPlacement placement;
    placement.shard_id = "bench-artifact:shard:" + std::to_string(shard_index);
    placement.node_id = "node-" + std::to_string(shard_index % 8);
    placement.shard_size_bytes = shard_size_bytes;
    placement.shard_content_hash = "hash-" + std::to_string(shard_index);
    placement.tier_level = shard_index % 3 == 0
                               ? "hot"
                               : (shard_index % 3 == 1 ? "warm" : "cold");
    manifest.add_shard_placement(std::move(placement));
  }

  return manifest;
}

inline void attach_integrity_receipt(
    distributed_tensor::ArtifactManifest& manifest,
    bool is_valid = true) {
  distributed_tensor::IntegrityReceipt receipt;
  receipt.merkle_root_hash = is_valid ? "merkle-root-valid" : "merkle-root-bad";
  receipt.verification_method = "SHA256";
  receipt.verified_at = "2026-07-13T00:00:00Z";
  receipt.is_valid = is_valid;

  for (const auto& shard : manifest.shard_placements()) {
    receipt.shard_hashes.emplace(shard.shard_id, shard.shard_content_hash);
  }

  manifest.set_integrity_receipt(std::move(receipt));
}

inline std::vector<distributed_tensor::TensorDependency> make_dependencies(
    bool allow_approximation = false,
    bool optional_dependency = false,
    distributed_tensor::ArtifactLifecycleStage required_stage =
        distributed_tensor::ArtifactLifecycleStage::ACTIVE,
    uint32_t max_staleness_seconds = 0) {
  distributed_tensor::TensorDependency dependency;
  dependency.artifact_id = "bench-artifact";
  dependency.required_lifecycle_stage = required_stage;
  dependency.is_required = !optional_dependency;
  dependency.max_staleness_seconds = max_staleness_seconds;
  dependency.allow_approximation = allow_approximation;
  return {dependency};
}

inline std::vector<distributed_tensor::NodeCapacity> make_nodes(
    uint32_t node_count,
    uint64_t node_capacity_bytes,
    bool include_accelerators = true) {
  std::vector<distributed_tensor::NodeCapacity> nodes;
  nodes.reserve(node_count);

  for (uint32_t node_index = 0; node_index < node_count; ++node_index) {
    distributed_tensor::NodeCapacity node;
    node.node_id = "node-" + std::to_string(node_index);
    node.available_capacity_bytes = node_capacity_bytes;
    node.tier_level = node_index % 2 == 0 ? "hot" : "warm";
    node.accelerator_type =
        include_accelerators && (node_index % 3 == 0) ? "GPU" : "CPU";
    node.network_latency_ms = 2 + node_index;
    node.bandwidth_utilization = 0.05f * static_cast<float>(node_index % 5);
    node.load_factor = 0.1f * static_cast<float>(node_index % 4);
    node.topology_zone = "zone-" + std::to_string(node_index % 3);
    nodes.push_back(std::move(node));
  }

  return nodes;
}

inline distributed_tensor::ArtifactManifest make_manifest_from_plan(
    const distributed_tensor::PlacementPlan& plan,
    distributed_tensor::ArtifactClass artifact_class =
        distributed_tensor::ArtifactClass::PRIMARY,
    distributed_tensor::ArtifactLifecycleStage lifecycle_stage =
        distributed_tensor::ArtifactLifecycleStage::ACTIVE) {
  using namespace distributed_tensor;

  ArtifactManifest manifest(plan.artifact_id.empty() ? "bench-artifact"
                                                     : plan.artifact_id,
                            artifact_class);
  manifest.set_version("bench-v1");
  manifest.set_content_hash("artifact-hash-primary");
  manifest.set_total_size_bytes(0);
  manifest.set_lifecycle_stage(lifecycle_stage);
  manifest.set_recovery_strategy("replication");

  uint64_t total_size_bytes = 0;
  for (const auto& shard : plan.shard_placements) {
    total_size_bytes += shard.shard_size_bytes;
    manifest.add_shard_placement(shard);
  }
  manifest.set_total_size_bytes(total_size_bytes);

  return manifest;
}

}  // namespace themis::bench::epic
