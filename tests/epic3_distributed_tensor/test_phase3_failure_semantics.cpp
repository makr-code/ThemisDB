// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#include "distributed_tensor/artifact_manifest.h"
#include "distributed_tensor/distributed_planner.h"
#include "distributed_tensor/integrity_verification.h"
#include "distributed_tensor/recovery_manager.h"
#include "distributed_tensor/shard_placement.h"

#include <gtest/gtest.h>

namespace themis { namespace distributed_tensor { 
namespace {

ArtifactManifest make_primary_manifest() {
  ArtifactManifest manifest("artifact-1", ArtifactClass::PRIMARY);
  manifest.set_version("v1");
  manifest.set_content_hash("artifact-hash");
  manifest.set_total_size_bytes(2048);
  manifest.set_lifecycle_stage(ArtifactLifecycleStage::ACTIVE);

  ShardPlacement shard;
  shard.shard_id = "artifact-1:shard:0";
  shard.node_id = "node-a";
  shard.shard_size_bytes = 2048;
  shard.shard_content_hash = "shard-hash-0";
  manifest.add_shard_placement(std::move(shard));

  return manifest;
}

ArtifactManifest make_derived_manifest() {
  ArtifactManifest manifest("artifact-derived", ArtifactClass::DERIVED);
  manifest.set_version("v1");
  manifest.set_total_size_bytes(4096);
  manifest.set_lifecycle_stage(ArtifactLifecycleStage::ACTIVE);
  manifest.set_parent_artifact_id("artifact-parent");

  ShardPlacement shard_a;
  shard_a.shard_id = "artifact-derived:shard:0";
  shard_a.node_id = "node-a";
  shard_a.shard_size_bytes = 2048;
  shard_a.shard_content_hash = "derived-hash-0";
  manifest.add_shard_placement(std::move(shard_a));

  ShardPlacement shard_b;
  shard_b.shard_id = "artifact-derived:shard:1";
  shard_b.node_id = "node-b";
  shard_b.shard_size_bytes = 2048;
  shard_b.shard_content_hash = "derived-hash-1";
  manifest.add_shard_placement(std::move(shard_b));

  return manifest;
}

TEST(DistributedTensorPhase3Test, ManifestCompletenessRejectsDeletedOrIncompleteShards) {
  auto manifest = make_primary_manifest();
  EXPECT_TRUE(manifest.is_complete());

  manifest.set_lifecycle_stage(ArtifactLifecycleStage::DELETED);
  EXPECT_FALSE(manifest.is_complete());

  auto invalid_manifest = make_primary_manifest();
  invalid_manifest.set_lifecycle_stage(ArtifactLifecycleStage::ACTIVE);
  ShardPlacement invalid_shard;
  invalid_shard.shard_id = "artifact-1:shard:1";
  invalid_shard.node_id = "node-b";
  invalid_manifest.add_shard_placement(std::move(invalid_shard));
  EXPECT_FALSE(invalid_manifest.is_complete());
}

TEST(DistributedTensorPhase3Test, IntegrityVerificationFailsOnPartialCoverage) {
  auto manifest = make_primary_manifest();
  ShardPlacement shard;
  shard.shard_id = "artifact-1:shard:1";
  shard.node_id = "node-b";
  shard.shard_size_bytes = 1024;
  manifest.add_shard_placement(std::move(shard));
  manifest.set_total_size_bytes(3072);

  DefaultIntegrityVerificationEngine engine;
  auto receipt = engine.compute_verification(manifest);

  EXPECT_FALSE(receipt.is_verified());
  EXPECT_FALSE(receipt.verification_error().empty());
  EXPECT_FALSE(engine.verify_integrity(manifest, receipt));
}

TEST(DistributedTensorPhase3Test, PlacementFailsClosedWhenConstraintsCannotBeMet) {
  DefaultShardPlacementStrategy strategy;

  NodeCapacity cpu_only_node;
  cpu_only_node.node_id = "node-a";
  cpu_only_node.available_capacity_bytes = 16 * 1024;
  cpu_only_node.accelerator_type = "CPU";

  auto plan = strategy.compute_placement(
      "artifact-1", 2, 8 * 1024, {cpu_only_node}, std::nullopt,
      PlacementConstraint::ACCELERATOR_REQUIRED);

  EXPECT_FALSE(plan.satisfies_hard_constraints);
  EXPECT_TRUE(plan.shard_placements.empty());
  EXPECT_FALSE(strategy.validate_placement(plan,
                                           PlacementConstraint::ACCELERATOR_REQUIRED));
}

TEST(DistributedTensorPhase3Test, RecoveryPlanningBlocksUnsafeReplicationLoss) {
  auto manifest = make_primary_manifest();
  DefaultRecoveryManager manager;

  auto plan = manager.create_recovery_plan(manifest, {"artifact-1:shard:0"});

  EXPECT_FALSE(plan.is_recoverable);
  EXPECT_FALSE(plan.allow_degraded_mode);
  ASSERT_TRUE(plan.blocking_failure_mode.has_value());
  EXPECT_EQ(*plan.blocking_failure_mode,
            RecoveryFailureMode::INSUFFICIENT_REDUNDANCY);
}

TEST(DistributedTensorPhase3Test, PlannerAllowsExplicitDegradedReadsOnlyWhenPermitted) {
  auto manifest = make_derived_manifest();
  manifest.set_lifecycle_stage(ArtifactLifecycleStage::RECOVERING);

  IntegrityReceipt valid_receipt;
  valid_receipt.is_valid = true;
  valid_receipt.merkle_root_hash = "root";
  manifest.set_integrity_receipt(std::move(valid_receipt));

  TensorDependency optional_dependency;
  optional_dependency.artifact_id = manifest.artifact_id();
  optional_dependency.required_lifecycle_stage =
      ArtifactLifecycleStage::ACTIVE;
  optional_dependency.is_required = false;
  optional_dependency.allow_approximation = true;

  DefaultDistributedTensorPlanner planner;
  auto degraded_plan =
      planner.plan_tensor_retrieval(manifest, {optional_dependency});

  EXPECT_TRUE(degraded_plan.can_execute);
  EXPECT_TRUE(degraded_plan.degraded_mode);
  EXPECT_FALSE(degraded_plan.execution_rationale.empty());

  IntegrityReceipt invalid_receipt;
  invalid_receipt.is_valid = false;
  invalid_receipt.merkle_root_hash = "bad-root";
  manifest.set_integrity_receipt(std::move(invalid_receipt));

  auto blocked_plan = planner.plan_tensor_retrieval(manifest, {optional_dependency});
  EXPECT_FALSE(blocked_plan.can_execute);
  EXPECT_FALSE(blocked_plan.degraded_mode);
}

}  // namespace
} } // namespace themis::distributed_tensor
