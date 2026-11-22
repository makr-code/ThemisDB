// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#include "distributed_tensor/artifact_manifest.h"
#include "distributed_tensor/distributed_planner.h"
#include "distributed_tensor/integrity_verification.h"
#include "distributed_tensor/shard_placement.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <string>
#include <utility>
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

constexpr char kSha256Abc[] =
    "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
constexpr char kSha256Def[] =
    "cb8379ac2098aa165029e3938a51da0bcecfc008fd6795f401178647f96c5b34";

ArtifactManifest make_integrity_manifest(
    const std::vector<std::pair<std::string, std::string>>& shard_hashes) {
  ArtifactManifest manifest("art-secure", ArtifactClass::PRIMARY);
  manifest.set_version("v1");
  manifest.set_content_hash("manifest-v1");
  manifest.set_total_size_bytes(shard_hashes.size());
  manifest.set_lifecycle_stage(ArtifactLifecycleStage::ACTIVE);
  manifest.set_recovery_strategy("replication");

  std::size_t index = 0;
  for (const auto& [shard_id, hash] : shard_hashes) {
    ShardPlacement shard;
    shard.shard_id = shard_id;
    shard.node_id = "node-" + std::to_string(index++);
    shard.shard_size_bytes = 1;
    shard.shard_content_hash = hash;
    manifest.add_shard_placement(std::move(shard));
  }
  return manifest;
}

std::vector<std::pair<std::string, std::string>> make_integrity_shard_hashes() {
  return {
      {"art-secure:shard:0", kSha256Abc},
      {"art-secure:shard:1", kSha256Def},
  };
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

TEST(DistributedTensorIntegrityTest, SHA256KnownAnswersAndRawDataAreEnforced) {
  DefaultIntegrityVerificationEngine engine;
#if defined(THEMIS_TEST_HAS_OPENSSL_SHA256)
  EXPECT_TRUE(engine.verify_shard("abc", kSha256Abc));
  EXPECT_TRUE(engine.verify_shard(
      "", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
  EXPECT_FALSE(engine.verify_shard("abd", kSha256Abc));
  EXPECT_FALSE(engine.verify_shard(kSha256Abc, kSha256Abc));
#else
  EXPECT_FALSE(engine.verify_shard("abc", kSha256Abc));
  const auto receipt = engine.compute_verification(
      make_integrity_manifest(make_integrity_shard_hashes()));
  EXPECT_FALSE(receipt.is_verified());
  EXPECT_NE(receipt.verification_error().find("OpenSSL Crypto"),
            std::string::npos);
#endif
}

TEST(DistributedTensorIntegrityTest, RejectsUnsupportedAlgorithmsAndMalformedShardHashes) {
  DefaultIntegrityVerificationEngine unsupported(HashAlgorithm::BLAKE3);
  const auto unsupported_receipt = unsupported.compute_verification(
      make_integrity_manifest(make_integrity_shard_hashes()));
  EXPECT_FALSE(unsupported_receipt.is_verified());
  EXPECT_FALSE(unsupported_receipt.verification_error().empty());

  DefaultIntegrityVerificationEngine sha256;
  auto missing_hash = make_integrity_manifest(
      {{"art-secure:shard:0", ""}});
  auto malformed_hash = make_integrity_manifest(
      {{"art-secure:shard:0", "not-a-sha256-digest"}});
  EXPECT_FALSE(sha256.compute_verification(missing_hash).is_verified());
  EXPECT_FALSE(sha256.compute_verification(malformed_hash).is_verified());
  EXPECT_FALSE(sha256.verify_shard("abc", "not-a-sha256-digest"));
}

TEST(DistributedTensorIntegrityTest, ShardOrderDoesNotChangeVersionedReceiptRoot) {
  DefaultIntegrityVerificationEngine engine;
  auto hashes = make_integrity_shard_hashes();
  auto forward = make_integrity_manifest(hashes);
  std::reverse(hashes.begin(), hashes.end());
  auto reverse = make_integrity_manifest(hashes);
  auto new_version = make_integrity_manifest(make_integrity_shard_hashes());
  new_version.set_version("v2");

  const auto forward_receipt = engine.compute_verification(forward);
  const auto reverse_receipt = engine.compute_verification(reverse);
  const auto new_version_receipt = engine.compute_verification(new_version);
#if defined(THEMIS_TEST_HAS_OPENSSL_SHA256)
  ASSERT_TRUE(forward_receipt.is_verified());
  ASSERT_TRUE(reverse_receipt.is_verified());
  ASSERT_TRUE(new_version_receipt.is_verified());
  EXPECT_EQ(forward_receipt.merkle_root_hash(),
            reverse_receipt.merkle_root_hash());
  EXPECT_NE(forward_receipt.merkle_root_hash(),
            new_version_receipt.merkle_root_hash());
#else
  EXPECT_FALSE(forward_receipt.is_verified());
  EXPECT_FALSE(reverse_receipt.is_verified());
  EXPECT_FALSE(new_version_receipt.is_verified());
#endif
}

TEST(DistributedTensorIntegrityTest, RejectsLegacyFnvReceipt) {
  DefaultIntegrityVerificationEngine engine;
  const auto hashes = make_integrity_shard_hashes();
  const auto manifest = make_integrity_manifest(hashes);

  IntegrityVerificationReceipt legacy("art-secure", HashAlgorithm::SHA256);
  legacy.set_merkle_root_hash(
      "0000000000000000000000000000000000000000000000000573b730c9ef5ff0");
  for (const auto& [shard_id, hash] : hashes) {
    legacy.add_shard_hash(shard_id, hash);
  }
  legacy.mark_verified();

  EXPECT_FALSE(engine.verify_integrity(manifest, legacy));
}

}  // namespace
}  // namespace themis::distributed_tensor
