// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#include "distributed_tensor/artifact_manifest.h"
#include "distributed_tensor/distributed_planner.h"
#include "distributed_tensor/shard_placement.h"
#include "distributed_tensor/tensor_infrastructure.h"

#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <vector>

namespace themis::distributed_tensor {
namespace {

ShardPlacement make_shard(const std::string& shard_id,
                         const std::string& node_id,
                         uint64_t size_bytes = 4096) {
  ShardPlacement placement;
  placement.shard_id = shard_id;
  placement.node_id = node_id;
  placement.shard_size_bytes = size_bytes;
  placement.shard_content_hash = "hash-" + shard_id;
  placement.replication_factor = 1;
  return placement;
}

ArtifactManifest make_large_manifest(const std::string& artifact_id, size_t shard_count) {
  ArtifactManifest manifest(artifact_id, ArtifactClass::PRIMARY);
  manifest.set_version("v1");
  manifest.set_total_size_bytes(static_cast<uint64_t>(shard_count) * 4096ULL);
  manifest.set_content_hash("hash-for-" + artifact_id);
  manifest.set_lifecycle_stage(ArtifactLifecycleStage::ACTIVE);

  for (size_t index = 0; index < shard_count; ++index) {
    ShardPlacement shard;
    shard.shard_id = artifact_id + ":shard:" + std::to_string(index);
    shard.node_id = index % 2 == 0 ? "node-a" : "node-b";
    shard.shard_size_bytes = 4096;
    shard.shard_content_hash = "hash-" + std::to_string(index);
    manifest.add_shard_placement(shard);
  }

  return manifest;
}

std::vector<NodeCapacity> make_capacity_pool(size_t count) {
  std::vector<NodeCapacity> nodes;
  nodes.reserve(count);
  for (size_t index = 0; index < count; ++index) {
    NodeCapacity node;
    node.node_id = "node-" + std::to_string(index);
    node.available_capacity_bytes = 256 * 1024;
    node.accelerator_type = index % 2 == 0 ? "GPU" : "CPU";
    nodes.push_back(node);
  }
  return nodes;
}

TEST(DistributedTensorPerformanceTest, PlacementBenchmarkStaysBelowLatencyTarget) {
  DefaultShardPlacementStrategy strategy;
  auto start = std::chrono::steady_clock::now();

  for (int iteration = 0; iteration < 200; ++iteration) {
    const auto plan = strategy.compute_placement(
        "artifact-benchmark", 8, 4096, make_capacity_pool(8), std::nullopt,
        PlacementConstraint::NONE);
    EXPECT_TRUE(plan.satisfies_hard_constraints);
  }

  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start);
  EXPECT_LT(elapsed.count(), 500);
}

TEST(DistributedTensorPerformanceTest, NodeRegistrationScalesAcrossManyNodes) {
  DefaultTensorInfrastructureManager manager;

  for (int index = 0; index < 200; ++index) {
    ClusterNode node;
    node.node_id = "node-" + std::to_string(index);
    node.endpoint = "10.0.0." + std::to_string(index % 255) + ":50051";
    node.total_storage_bytes = 1024ULL * 1024ULL * 16ULL;
    node.used_storage_bytes = 1024ULL * 1024ULL;
    node.status = NodeStatus::HEALTHY;
    EXPECT_TRUE(manager.register_node(node));
  }

  EXPECT_EQ(manager.list_nodes().size(), 200U);
  EXPECT_EQ(manager.get_healthy_nodes().size(), 200U);
}

TEST(DistributedTensorPerformanceTest, PlacementValidationStillFastForLargeBatches) {
  DefaultShardPlacementStrategy strategy;
  const auto nodes = make_capacity_pool(32);

  auto start = std::chrono::steady_clock::now();
  for (int iteration = 0; iteration < 150; ++iteration) {
    const auto plan = strategy.compute_placement(
        "artifact-large-batch", 12, 4096, nodes, std::nullopt,
        PlacementConstraint::NONE);
    EXPECT_TRUE(strategy.validate_placement(plan, PlacementConstraint::NONE));
  }

  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start);
  EXPECT_LT(elapsed.count(), 750);
}

TEST(DistributedTensorPerformanceTest, RetrievalCostEstimationRemainsDeterministic) {
  DefaultDistributedTensorPlanner planner;
  const auto manifest = make_large_manifest("artifact-cost", 32);

  const auto cost_a = planner.estimate_retrieval_cost(
      manifest, RetrievalStrategy::EXACT, RetrievalLocation::HOT_TIER);
  const auto cost_b = planner.estimate_retrieval_cost(
      manifest, RetrievalStrategy::EXACT, RetrievalLocation::HOT_TIER);

  EXPECT_EQ(cost_a, cost_b);
  EXPECT_GT(cost_a, 0ULL);
}

TEST(DistributedTensorPerformanceTest, ManifestHashComputingHandlesManyShards) {
  ArtifactManifest manifest("manifest-shards", ArtifactClass::PRIMARY);
  manifest.set_version("v1");
  manifest.set_total_size_bytes(1ULL << 20);
  manifest.set_lifecycle_stage(ArtifactLifecycleStage::ACTIVE);
  manifest.set_content_hash("content-hash");

  for (int index = 0; index < 200; ++index) {
    ShardPlacement shard;
    shard.shard_id = "manifest-shards:shard:" + std::to_string(index);
    shard.node_id = "node-" + std::to_string(index % 8);
    shard.shard_size_bytes = 4096;
    shard.shard_content_hash = "hash-" + std::to_string(index);
    manifest.add_shard_placement(shard);
  }

  manifest.compute_manifest_hash();
  EXPECT_FALSE(manifest.manifest_hash().empty());
}

TEST(DistributedTensorPerformanceTest, RepeatedFreshnessUpdatesStayStable) {
  ArtifactManifest manifest("manifest-freshness", ArtifactClass::PRIMARY);
  manifest.set_version("v1");
  manifest.set_content_hash("fresh-hash");
  manifest.set_total_size_bytes(4096);
  manifest.set_lifecycle_stage(ArtifactLifecycleStage::ACTIVE);
  manifest.add_shard_placement(make_shard("manifest-freshness:shard:0", "node-a", 4096));

  for (int index = 0; index < 100; ++index) {
    manifest.update_freshness();
  }

  EXPECT_FALSE(manifest.freshness_timestamp().empty());
}

TEST(DistributedTensorPerformanceTest, PlannerRunsStayPredictableAcrossIterations) {
  DefaultDistributedTensorPlanner planner;
  auto start = std::chrono::steady_clock::now();

  for (int iteration = 0; iteration < 100; ++iteration) {
    const auto manifest = make_large_manifest("artifact-plan", 16);
    const auto plan = planner.plan_tensor_retrieval(
        manifest, {}, RetrievalLocation::ANY_TIER);
    EXPECT_TRUE(plan.can_execute);
  }

  auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start);
  EXPECT_LT(elapsed.count(), 600);
}

}  // namespace
}  // namespace themis::distributed_tensor
