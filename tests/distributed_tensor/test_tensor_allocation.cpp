// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

#include "distributed_tensor/artifact_manifest.h"
#include "distributed_tensor/tensor_artifact_classes.h"
#include "distributed_tensor/tensor_infrastructure.h"

#include <gtest/gtest.h>

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

TEST(DistributedTensorAllocationTest, PrimaryArtifactStartsInStaging) {
  PrimaryTensorArtifact artifact("artifact-1", "v1", DurabilityLevel::REPLICATED);

  EXPECT_EQ(artifact.metadata().artifact_id, "artifact-1");
  EXPECT_EQ(artifact.metadata().artifact_class, ArtifactClass::PRIMARY);
  EXPECT_EQ(artifact.metadata().version, "v1");
  EXPECT_EQ(artifact.metadata().lifecycle_stage, ArtifactLifecycleStage::STAGING);
}

TEST(DistributedTensorAllocationTest, DerivedArtifactIsRebuildable) {
  DerivedTensorArtifact artifact("artifact-derived", "parent-artifact",
                                DurabilityLevel::SINGLE_COPY);

  EXPECT_EQ(artifact.metadata().artifact_class, ArtifactClass::DERIVED);
  EXPECT_TRUE(artifact.is_rebuildable());
  EXPECT_EQ(artifact.parent_artifact_id(), "parent-artifact");
  EXPECT_FALSE(artifact.metadata().rebuild_instruction.empty());
}

TEST(DistributedTensorAllocationTest, EphemeralArtifactKeepsSessionScope) {
  EphemeralTensorArtifact artifact("temp-artifact", "session-42");

  EXPECT_EQ(artifact.metadata().artifact_class, ArtifactClass::EPHEMERAL);
  EXPECT_EQ(artifact.metadata().durability_level, DurabilityLevel::NONE);
  EXPECT_FALSE(artifact.is_rebuildable());
  EXPECT_EQ(artifact.session_id(), "session-42");
}

TEST(DistributedTensorAllocationTest, ManifestRejectsIncompletePrimaryArtifact) {
  ArtifactManifest manifest("manifest-1", ArtifactClass::PRIMARY);
  manifest.set_version("v1");
  manifest.set_total_size_bytes(8192);
  manifest.set_lifecycle_stage(ArtifactLifecycleStage::ACTIVE);

  EXPECT_FALSE(manifest.is_complete());

  manifest.add_shard_placement(make_shard("manifest-1:shard:0", "node-a", 4096));
  manifest.add_shard_placement(make_shard("manifest-1:shard:1", "node-b", 4096));
  EXPECT_FALSE(manifest.is_complete());

  manifest.set_content_hash("hash-contents");
  EXPECT_TRUE(manifest.is_complete());
}

TEST(DistributedTensorAllocationTest, ManifestTracksShardPlacementRoundTrip) {
  ArtifactManifest manifest("manifest-2", ArtifactClass::PRIMARY);
  manifest.set_version("v2");
  manifest.set_content_hash("manifest-hash");
  manifest.set_total_size_bytes(8192);
  manifest.set_lifecycle_stage(ArtifactLifecycleStage::ACTIVE);

  const auto shard = make_shard("manifest-2:shard:0", "node-a", 4096);
  manifest.add_shard_placement(shard);

  const auto stored = manifest.get_shard_placement("manifest-2:shard:0");
  ASSERT_TRUE(stored.has_value());
  EXPECT_EQ(stored->node_id, "node-a");
  EXPECT_EQ(stored->shard_size_bytes, 4096U);
  EXPECT_EQ(manifest.num_shards(), 1U);
}

TEST(DistributedTensorAllocationTest, ManifestHashIsUpdatedWhenContentChanges) {
  ArtifactManifest manifest("manifest-3", ArtifactClass::PRIMARY);
  manifest.set_version("v3");
  manifest.set_total_size_bytes(1024);
  manifest.set_lifecycle_stage(ArtifactLifecycleStage::ACTIVE);
  manifest.set_content_hash("before");
  manifest.add_shard_placement(make_shard("manifest-3:shard:0", "node-a", 1024));

  manifest.compute_manifest_hash();
  const std::string before_hash = manifest.manifest_hash();

  manifest.set_content_hash("after");
  manifest.compute_manifest_hash();

  EXPECT_NE(before_hash, manifest.manifest_hash());
}

TEST(DistributedTensorAllocationTest, InfrastructureManagerRegistersNodeAndTracksStatus) {
  DefaultTensorInfrastructureManager manager;
  ClusterNode node;
  node.node_id = "node-1";
  node.endpoint = "10.0.0.1:50051";
  node.total_storage_bytes = 100 * 1024 * 1024;
  node.used_storage_bytes = 20 * 1024 * 1024;
  node.status = NodeStatus::HEALTHY;

  EXPECT_TRUE(manager.register_node(node));
  EXPECT_TRUE(manager.is_node_available("node-1", 10 * 1024 * 1024));

  const auto stored = manager.get_node("node-1");
  ASSERT_TRUE(stored.has_value());
  EXPECT_EQ(stored->endpoint, "10.0.0.1:50051");

  EXPECT_TRUE(manager.update_node_status("node-1", NodeStatus::DEGRADED));
  EXPECT_TRUE(manager.get_node("node-1")->status == NodeStatus::DEGRADED);
}

TEST(DistributedTensorAllocationTest, InfrastructureManagerRejectsEmptyAndUnknownNodes) {
  DefaultTensorInfrastructureManager manager;

  ClusterNode empty_node;
  EXPECT_FALSE(manager.register_node(empty_node));

  EXPECT_FALSE(manager.unregister_node("missing-node"));
  EXPECT_FALSE(manager.is_node_available("missing-node", 1024));

  ClusterNode node;
  node.node_id = "node-2";
  node.total_storage_bytes = 1024;
  node.used_storage_bytes = 512;
  node.status = NodeStatus::OFFLINE;
  EXPECT_TRUE(manager.register_node(node));
  EXPECT_FALSE(manager.is_node_available("node-2", 1));
}

}  // namespace
}  // namespace themis::distributed_tensor
