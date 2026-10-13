// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#include "distributed_tensor/artifact_manifest.h"
#include "distributed_tensor/distributed_planner.h"
#include "distributed_tensor/shard_placement.h"

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <vector>

namespace themis::distributed_tensor {
namespace {

ArtifactManifest make_complete_manifest(const std::string& artifact_id,
                                       ArtifactClass artifact_class =
                                           ArtifactClass::PRIMARY) {
  ArtifactManifest manifest(artifact_id, artifact_class);
  manifest.set_version("v1");
  manifest.set_total_size_bytes(8192);
  manifest.set_lifecycle_stage(ArtifactLifecycleStage::ACTIVE);
  manifest.set_content_hash("hash-" + artifact_id);

  for (int shard_index = 0; shard_index < 2; ++shard_index) {
    ShardPlacement shard;
    shard.shard_id = artifact_id + ":shard:" + std::to_string(shard_index);
    shard.node_id = shard_index == 0 ? "node-a" : "node-b";
    shard.shard_size_bytes = 4096;
    shard.shard_content_hash = "hash-" + std::to_string(shard_index);
    manifest.add_shard_placement(shard);
  }

  return manifest;
}

std::vector<NodeCapacity> make_node_pool() {
  std::vector<NodeCapacity> nodes(3);
  nodes[0].node_id = "node-a";
  nodes[0].available_capacity_bytes = 200 * 1024;
  nodes[0].accelerator_type = "CPU";
  nodes[1].node_id = "node-b";
  nodes[1].available_capacity_bytes = 240 * 1024;
  nodes[1].accelerator_type = "GPU";
  nodes[2].node_id = "node-c";
  nodes[2].available_capacity_bytes = 400 * 1024;
  nodes[2].accelerator_type = "GPU";
  return nodes;
}

TEST(DistributedTensorOperationsTest, PlacementRejectsInvalidInput) {
  DefaultShardPlacementStrategy strategy;

  const auto plan = strategy.compute_placement("", 2, 1024, make_node_pool());
  EXPECT_FALSE(plan.satisfies_hard_constraints);
  EXPECT_TRUE(plan.placement_warnings.size() >= 1U);
  EXPECT_EQ(plan.shard_placements.size(), 0U);
}

TEST(DistributedTensorOperationsTest, PlacementAllocatesExpectedShardCount) {
  DefaultShardPlacementStrategy strategy;

  const auto plan = strategy.compute_placement(
      "artifact-ops", 4, 8192, make_node_pool(), std::nullopt,
      PlacementConstraint::NONE);

  EXPECT_TRUE(plan.satisfies_hard_constraints);
  EXPECT_EQ(plan.shard_placements.size(), 4U);
  EXPECT_EQ(plan.artifact_id, "artifact-ops");
}

TEST(DistributedTensorOperationsTest, PlacementRespectsCapacityConstraints) {
  DefaultShardPlacementStrategy strategy;
  std::vector<NodeCapacity> nodes = make_node_pool();
  nodes[0].available_capacity_bytes = 2048;
  nodes[1].available_capacity_bytes = 2048;

  const auto plan = strategy.compute_placement(
      "artifact-capacity", 3, 4096, nodes, std::nullopt,
      PlacementConstraint::NONE);

  EXPECT_TRUE(plan.satisfies_hard_constraints);
  EXPECT_EQ(plan.shard_placements.size(), 3U);
}

TEST(DistributedTensorOperationsTest, PlacementFailsClosedWhenConstraintEliminatesNodes) {
  DefaultShardPlacementStrategy strategy;
  std::vector<NodeCapacity> nodes = make_node_pool();
  nodes[0].accelerator_type = "CPU";
  nodes[1].accelerator_type = "CPU";
  nodes[2].accelerator_type = "CPU";

  const auto plan = strategy.compute_placement(
      "artifact-accelerator", 2, 1024, nodes, std::nullopt,
      PlacementConstraint::ACCELERATOR_REQUIRED);

  EXPECT_FALSE(plan.satisfies_hard_constraints);
  EXPECT_TRUE(plan.shard_placements.empty());
  EXPECT_FALSE(strategy.validate_placement(plan,
                                          PlacementConstraint::ACCELERATOR_REQUIRED));
}

TEST(DistributedTensorOperationsTest, PlacementValidationRejectsEmptyPlan) {
  DefaultShardPlacementStrategy strategy;
  PlacementPlan empty_plan;
  EXPECT_FALSE(strategy.validate_placement(empty_plan, PlacementConstraint::NONE));
}

TEST(DistributedTensorOperationsTest, PlacementOptimizationPreservesValidPlan) {
  DefaultShardPlacementStrategy strategy;
  const auto plan = strategy.compute_placement(
      "artifact-optimize", 2, 2048, make_node_pool(), std::nullopt,
      PlacementConstraint::NONE);

  const auto optimized = strategy.optimize_placement(plan, make_node_pool());
  EXPECT_EQ(optimized.artifact_id, "artifact-optimize");
  EXPECT_EQ(optimized.shard_placements.size(), 2U);
  EXPECT_FALSE(optimized.placement_rationale.empty());
}

TEST(DistributedTensorOperationsTest, PlannerAllowsActiveCompleteManifest) {
  DefaultDistributedTensorPlanner planner;
  const auto manifest = make_complete_manifest("artifact-active");

  const auto plan = planner.plan_tensor_retrieval(
      manifest, {}, RetrievalLocation::HOT_TIER);

  EXPECT_TRUE(plan.can_execute);
  EXPECT_TRUE(plan.parallel_retrieval);
  EXPECT_EQ(plan.artifact_id, "artifact-active");
}

TEST(DistributedTensorOperationsTest, PlannerBlocksInvalidIntegrityReceipt) {
  DefaultDistributedTensorPlanner planner;
  auto manifest = make_complete_manifest("artifact-invalid");
  IntegrityReceipt receipt;
  receipt.is_valid = false;
  receipt.merkle_root_hash = "bad-root";
  manifest.set_integrity_receipt(receipt);

  const auto plan = planner.plan_tensor_retrieval(
      manifest, {}, RetrievalLocation::WARM_TIER);

  EXPECT_FALSE(plan.can_execute);
  EXPECT_FALSE(plan.parallel_retrieval);
}

TEST(DistributedTensorOperationsTest, PlannerDegradesForOptionalStaleDependency) {
  DefaultDistributedTensorPlanner planner;
  auto manifest = make_complete_manifest("artifact-stale");
  manifest.set_lifecycle_stage(ArtifactLifecycleStage::STALE);

  TensorDependency dependency;
  dependency.artifact_id = "artifact-stale";
  dependency.required_lifecycle_stage = ArtifactLifecycleStage::ACTIVE;
  dependency.is_required = false;
  dependency.allow_approximation = true;

  const auto plan = planner.plan_tensor_retrieval(manifest, {dependency},
                                                RetrievalLocation::ANY_TIER);

  EXPECT_TRUE(plan.can_execute);
  EXPECT_TRUE(plan.degraded_mode);
  EXPECT_FALSE(plan.execution_rationale.empty());
}

TEST(DistributedTensorOperationsTest, PlannerReportsAvailabilityForActiveManifest) {
  DefaultDistributedTensorPlanner planner;
  const auto manifest = make_complete_manifest("artifact-available");

  EXPECT_TRUE(planner.is_tensor_available(manifest, ArtifactLifecycleStage::ACTIVE));
}

TEST(DistributedTensorOperationsTest, RetrievalCostAndStrategyStayWithinExpectedRange) {
  DefaultDistributedTensorPlanner planner;
  const auto manifest = make_complete_manifest("artifact-cost");

  const auto cost = planner.estimate_retrieval_cost(
      manifest, RetrievalStrategy::EXACT, RetrievalLocation::HOT_TIER);
  EXPECT_GT(cost, 0ULL);
  EXPECT_LT(cost, 10'000ULL);
}

}  // namespace
}  // namespace themis::distributed_tensor
