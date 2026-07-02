/// @file artifact_manifest_test.cc
/// @brief Comprehensive tests for artifact manifest schema and operations
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-02

#include "src/distributed_tensor/include/artifact_manifest.h"
#include "src/distributed_tensor/include/tensor_artifact_classes.h"

#include <gtest/gtest.h>
#include <ctime>

namespace themis {
namespace distributed_tensor {

// ============================================================================
// Test Fixtures
// ============================================================================

class ArtifactManifestTest : public ::testing::Test {
 protected:
  ArtifactManifestTest() {
    now_unix_sec_ = static_cast<int64_t>(std::time(nullptr));
  }

  int64_t now_unix_sec_;

  /// Helper to create a valid manifest for testing
  ArtifactManifest createValidManifest(const std::string& artifact_id = "test:v1:adapter:model-x") {
    ArtifactManifest manifest;
    manifest.artifact_id = artifact_id;
    manifest.artifact_class = ArtifactClass::DERIVED;
    manifest.truth_semantic = TruthSemantic::TRUTH_ADJACENT;
    manifest.lifecycle_state = LifecycleState::ACTIVE;
    manifest.version = "1.0.0";
    manifest.content_hash = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    manifest.created_at_unix_sec = now_unix_sec_ - 3600;
    manifest.updated_at_unix_sec = now_unix_sec_ - 1800;
    manifest.last_verified_unix_sec = now_unix_sec_ - 600;
    manifest.staleness_threshold_sec = 86400;  // 1 day
    manifest.source_artifact_id = "base:v1:model:model-x";
    manifest.shard_placements = {"shard-1", "shard-2"};
    manifest.is_rebuildable = true;
    manifest.replication_factor = 2;
    return manifest;
  }
};

class ManifestValidationTest : public ArtifactManifestTest {};
class ManifestFreshnessTest : public ArtifactManifestTest {};
class ManifestSerializationTest : public ArtifactManifestTest {};
class ManifestStateTransitionTest : public ArtifactManifestTest {};
class ManifestPlannerIntegrationTest : public ArtifactManifestTest {};

// ============================================================================
// ManifestValidationTest: Field Validation Tests
// ============================================================================

TEST_F(ManifestValidationTest, ValidManifestPasses) {
  ArtifactManifest manifest = createValidManifest();
  EXPECT_TRUE(manifest.validate());
}

TEST_F(ManifestValidationTest, EmptyArtifactIdFails) {
  ArtifactManifest manifest = createValidManifest();
  manifest.artifact_id = "";
  EXPECT_FALSE(manifest.validate());
}

TEST_F(ManifestValidationTest, InvalidContentHashFails) {
  ArtifactManifest manifest = createValidManifest();
  manifest.content_hash = "invalid_hex";  // Not enough characters and not hex
  EXPECT_FALSE(manifest.validate());
}

TEST_F(ManifestValidationTest, InconsistentTimestampsFails) {
  ArtifactManifest manifest = createValidManifest();
  manifest.updated_at_unix_sec = manifest.created_at_unix_sec - 100;
  EXPECT_FALSE(manifest.validate());
}

TEST_F(ManifestValidationTest, InvalidSequenceRangeFails) {
  ArtifactManifest manifest = createValidManifest();
  manifest.source_seq_start = 1000;
  manifest.source_seq_end = 500;  // Backwards
  EXPECT_FALSE(manifest.validate());
}

TEST_F(ManifestValidationTest, InvalidResidualFails) {
  ArtifactManifest manifest = createValidManifest();
  manifest.residual = -0.1;  // Negative
  EXPECT_FALSE(manifest.validate());
}

TEST_F(ManifestValidationTest, InvalidRankStatusFails) {
  ArtifactManifest manifest = createValidManifest();
  manifest.rank_cap = 10;
  manifest.rank_status = 20;  // Greater than cap
  EXPECT_FALSE(manifest.validate());
}

TEST_F(ManifestValidationTest, InvalidReplicationFactorFails) {
  ArtifactManifest manifest = createValidManifest();
  manifest.replication_factor = 0;
  EXPECT_FALSE(manifest.validate());
}

TEST_F(ManifestValidationTest, InvalidArtifactClassSemanticCombinationFails) {
  ArtifactManifest manifest = createValidManifest();
  manifest.artifact_class = ArtifactClass::PRIMARY;
  manifest.truth_semantic = TruthSemantic::ADVISORY;  // Invalid: PRIMARY cannot be ADVISORY
  EXPECT_FALSE(manifest.validate());
}

// ============================================================================
// ManifestFreshnessTest: Freshness and Staleness Tracking
// ============================================================================

TEST_F(ManifestFreshnessTest, UsableActiveArtifact) {
  ArtifactManifest manifest = createValidManifest();
  manifest.lifecycle_state = LifecycleState::ACTIVE;
  EXPECT_TRUE(manifest.isUsable(now_unix_sec_));
}

TEST_F(ManifestFreshnessTest, UsableStaleArtifact) {
  ArtifactManifest manifest = createValidManifest();
  manifest.lifecycle_state = LifecycleState::STALE;
  EXPECT_TRUE(manifest.isUsable(now_unix_sec_));
}

TEST_F(ManifestFreshnessTest, NotUsableInvalidatedArtifact) {
  ArtifactManifest manifest = createValidManifest();
  manifest.lifecycle_state = LifecycleState::INVALIDATED;
  EXPECT_FALSE(manifest.isUsable(now_unix_sec_));
}

TEST_F(ManifestFreshnessTest, NotUsableCreatedArtifact) {
  ArtifactManifest manifest = createValidManifest();
  manifest.lifecycle_state = LifecycleState::CREATED;
  EXPECT_FALSE(manifest.isUsable(now_unix_sec_));
}

TEST_F(ManifestFreshnessTest, NotStaleWhenThresholdNotSet) {
  ArtifactManifest manifest = createValidManifest();
  manifest.staleness_threshold_sec = 0;  // No threshold
  EXPECT_FALSE(manifest.isStale(now_unix_sec_));
}

TEST_F(ManifestFreshnessTest, NotStaleWhenUnderThreshold) {
  ArtifactManifest manifest = createValidManifest();
  manifest.staleness_threshold_sec = 86400;  // 1 day
  manifest.last_verified_unix_sec = now_unix_sec_ - 3600;  // 1 hour ago
  EXPECT_FALSE(manifest.isStale(now_unix_sec_));
}

TEST_F(ManifestFreshnessTest, StaleWhenExceedsThreshold) {
  ArtifactManifest manifest = createValidManifest();
  manifest.staleness_threshold_sec = 3600;  // 1 hour
  manifest.last_verified_unix_sec = now_unix_sec_ - 7200;  // 2 hours ago
  EXPECT_TRUE(manifest.isStale(now_unix_sec_));
}

TEST_F(ManifestFreshnessTest, StaleWhenNeverVerified) {
  ArtifactManifest manifest = createValidManifest();
  manifest.staleness_threshold_sec = 3600;
  manifest.last_verified_unix_sec = 0;  // Never verified
  EXPECT_TRUE(manifest.isStale(now_unix_sec_));
}

TEST_F(ManifestFreshnessTest, FreshnessScoreNeverVerified) {
  ArtifactManifest manifest = createValidManifest();
  manifest.last_verified_unix_sec = 0;
  EXPECT_EQ(manifest.getFreshnessScore(now_unix_sec_), 0.0);
}

TEST_F(ManifestFreshnessTest, FreshnessScoreNoThreshold) {
  ArtifactManifest manifest = createValidManifest();
  manifest.staleness_threshold_sec = 0;
  manifest.last_verified_unix_sec = now_unix_sec_;
  EXPECT_EQ(manifest.getFreshnessScore(now_unix_sec_), 1.0);
}

TEST_F(ManifestFreshnessTest, FreshnessScoreJustVerified) {
  ArtifactManifest manifest = createValidManifest();
  manifest.staleness_threshold_sec = 3600;
  manifest.last_verified_unix_sec = now_unix_sec_;
  EXPECT_NEAR(manifest.getFreshnessScore(now_unix_sec_), 1.0, 0.01);
}

TEST_F(ManifestFreshnessTest, FreshnessScoreMidLife) {
  ArtifactManifest manifest = createValidManifest();
  manifest.staleness_threshold_sec = 3600;  // 1 hour
  manifest.last_verified_unix_sec = now_unix_sec_ - 1800;  // 30 minutes ago
  double freshness = manifest.getFreshnessScore(now_unix_sec_);
  EXPECT_NEAR(freshness, 0.5, 0.01);  // Should be 50% fresh
}

TEST_F(ManifestFreshnessTest, FreshnessScoreExceedsThreshold) {
  ArtifactManifest manifest = createValidManifest();
  manifest.staleness_threshold_sec = 3600;
  manifest.last_verified_unix_sec = now_unix_sec_ - 7200;  // 2 hours ago
  EXPECT_NEAR(manifest.getFreshnessScore(now_unix_sec_), 0.0, 0.01);
}

// ============================================================================
// ManifestSerializationTest: JSON/YAML Serialization
// ============================================================================

TEST_F(ManifestSerializationTest, SerializeToJSON) {
  ArtifactManifest manifest = createValidManifest();
  std::string json = manifest.toJSON();
  EXPECT_FALSE(json.empty());
  EXPECT_TRUE(json.find("artifact_id") != std::string::npos);
  EXPECT_TRUE(json.find("test:v1:adapter:model-x") != std::string::npos);
}

TEST_F(ManifestSerializationTest, DeserializeFromJSON) {
  ArtifactManifest original = createValidManifest();
  std::string json = original.toJSON();

  auto deserialized = ArtifactManifest::fromJSON(json);
  EXPECT_TRUE(deserialized.has_value());

  EXPECT_EQ(deserialized->artifact_id, original.artifact_id);
  EXPECT_EQ(deserialized->artifact_class, original.artifact_class);
  EXPECT_EQ(deserialized->version, original.version);
  EXPECT_EQ(deserialized->content_hash, original.content_hash);
}

TEST_F(ManifestSerializationTest, RoundTripJSON) {
  ArtifactManifest original = createValidManifest();
  original.source_seq_start = 100;
  original.source_seq_end = 5000;
  original.delta_lag = 500;
  original.residual = 0.05;
  original.rank_cap = 128;
  original.rank_status = 100;
  original.rebuild_state = RebuildState::PATCHED;
  original.update_mode = UpdateMode::PARTIAL_REFIT;
  original.invalidation_reason = InvalidationReason::STALENESS_EXCEEDED;

  std::string json = original.toJSON();
  auto deserialized = ArtifactManifest::fromJSON(json);

  EXPECT_TRUE(deserialized.has_value());
  EXPECT_EQ(deserialized->source_seq_start, 100u);
  EXPECT_EQ(deserialized->source_seq_end, 5000u);
  EXPECT_EQ(deserialized->delta_lag, 500u);
  EXPECT_NEAR(deserialized->residual, 0.05, 0.001);
  EXPECT_EQ(deserialized->rank_cap, 128u);
  EXPECT_EQ(deserialized->rank_status, 100u);
  EXPECT_EQ(deserialized->rebuild_state, RebuildState::PATCHED);
  EXPECT_EQ(deserialized->update_mode, UpdateMode::PARTIAL_REFIT);
  EXPECT_EQ(deserialized->invalidation_reason, InvalidationReason::STALENESS_EXCEEDED);
}

TEST_F(ManifestSerializationTest, InvalidJSONReturnsNullopt) {
  std::string invalid_json = "{ not valid json }";
  auto result = ArtifactManifest::fromJSON(invalid_json);
  EXPECT_FALSE(result.has_value());
}

TEST_F(ManifestSerializationTest, SerializeProvenanceChain) {
  ArtifactManifest manifest = createValidManifest();
  manifest.provenance_chain = {"base:v1", "adapter:v1", "patch:v1"};
  std::string json = manifest.toJSON();

  auto deserialized = ArtifactManifest::fromJSON(json);
  EXPECT_TRUE(deserialized.has_value());
  EXPECT_EQ(deserialized->provenance_chain.size(), 3);
  EXPECT_EQ(deserialized->provenance_chain[0], "base:v1");
}

TEST_F(ManifestSerializationTest, SerializeCustomAttributes) {
  ArtifactManifest manifest = createValidManifest();
  manifest.custom_attributes["model_name"] = "llama-7b";
  manifest.custom_attributes["training_date"] = "2026-06-15";
  std::string json = manifest.toJSON();

  auto deserialized = ArtifactManifest::fromJSON(json);
  EXPECT_TRUE(deserialized.has_value());
  EXPECT_EQ(deserialized->custom_attributes["model_name"], "llama-7b");
  EXPECT_EQ(deserialized->custom_attributes["training_date"], "2026-06-15");
}

// ============================================================================
// ManifestStateTransitionTest: Rebuild and Invalidation States
// ============================================================================

TEST_F(ManifestStateTransitionTest, RebuildStateConversion) {
  EXPECT_EQ(RebuildStateUtils::stateToString(RebuildState::PRISTINE), "PRISTINE");
  EXPECT_EQ(RebuildStateUtils::stateToString(RebuildState::PATCHED), "PATCHED");
  EXPECT_EQ(RebuildStateUtils::stateToString(RebuildState::PARTIAL_REFITTED), "PARTIAL_REFITTED");
  EXPECT_EQ(RebuildStateUtils::stateToString(RebuildState::REBUILT), "REBUILT");
}

TEST_F(ManifestStateTransitionTest, RebuildStateParsing) {
  auto pristine = RebuildStateUtils::stringToState("PRISTINE");
  EXPECT_TRUE(pristine.has_value());
  EXPECT_EQ(*pristine, RebuildState::PRISTINE);

  auto patched = RebuildStateUtils::stringToState("PATCHED");
  EXPECT_TRUE(patched.has_value());
  EXPECT_EQ(*patched, RebuildState::PATCHED);

  auto invalid = RebuildStateUtils::stringToState("INVALID_STATE");
  EXPECT_FALSE(invalid.has_value());
}

TEST_F(ManifestStateTransitionTest, UpdateModeConversion) {
  EXPECT_EQ(UpdateModeUtils::modeToString(UpdateMode::PATCH), "patch");
  EXPECT_EQ(UpdateModeUtils::modeToString(UpdateMode::PARTIAL_REFIT), "partial_refit");
  EXPECT_EQ(UpdateModeUtils::modeToString(UpdateMode::REBUILD), "rebuild");
}

TEST_F(ManifestStateTransitionTest, UpdateModeParsing) {
  auto patch = UpdateModeUtils::stringToMode("patch");
  EXPECT_TRUE(patch.has_value());
  EXPECT_EQ(*patch, UpdateMode::PATCH);

  auto partial = UpdateModeUtils::stringToMode("partial_refit");
  EXPECT_TRUE(partial.has_value());
  EXPECT_EQ(*partial, UpdateMode::PARTIAL_REFIT);

  auto rebuild = UpdateModeUtils::stringToMode("rebuild");
  EXPECT_TRUE(rebuild.has_value());
  EXPECT_EQ(*rebuild, UpdateMode::REBUILD);
}

TEST_F(ManifestStateTransitionTest, InvalidationReasonConversion) {
  EXPECT_EQ(InvalidationReasonUtils::reasonToString(InvalidationReason::INTEGRITY_CHECK_FAILED), "INTEGRITY_CHECK_FAILED");
  EXPECT_EQ(InvalidationReasonUtils::reasonToString(InvalidationReason::STALENESS_EXCEEDED), "STALENESS_EXCEEDED");
  EXPECT_EQ(InvalidationReasonUtils::reasonToString(InvalidationReason::SOURCE_INVALIDATED), "SOURCE_INVALIDATED");
  EXPECT_EQ(InvalidationReasonUtils::reasonToString(InvalidationReason::POLICY_VIOLATION), "POLICY_VIOLATION");
  EXPECT_EQ(InvalidationReasonUtils::reasonToString(InvalidationReason::ADMIN_REQUESTED), "ADMIN_REQUESTED");
}

TEST_F(ManifestStateTransitionTest, InvalidationReasonParsing) {
  auto integrity = InvalidationReasonUtils::stringToReason("INTEGRITY_CHECK_FAILED");
  EXPECT_TRUE(integrity.has_value());
  EXPECT_EQ(*integrity, InvalidationReason::INTEGRITY_CHECK_FAILED);

  auto staleness = InvalidationReasonUtils::stringToReason("STALENESS_EXCEEDED");
  EXPECT_TRUE(staleness.has_value());
  EXPECT_EQ(*staleness, InvalidationReason::STALENESS_EXCEEDED);

  auto invalid = InvalidationReasonUtils::stringToReason("INVALID_REASON");
  EXPECT_FALSE(invalid.has_value());
}

// ============================================================================
// ManifestPlannerIntegrationTest: Planner-Specific Fields
// ============================================================================

TEST_F(ManifestPlannerIntegrationTest, AdvisoryOnlyFlagPrimary) {
  ArtifactManifest manifest = createValidManifest();
  manifest.artifact_class = ArtifactClass::PRIMARY;
  manifest.truth_semantic = TruthSemantic::SOURCE_OF_TRUTH;
  manifest.advisory_only = false;
  EXPECT_FALSE(manifest.advisory_only);
}

TEST_F(ManifestPlannerIntegrationTest, AdvisoryOnlyFlagAdvisory) {
  ArtifactManifest manifest = createValidManifest();
  manifest.artifact_class = ArtifactClass::ADVISORY_ONLY;
  manifest.truth_semantic = TruthSemantic::ADVISORY;
  manifest.advisory_only = true;
  EXPECT_TRUE(manifest.advisory_only);
}

TEST_F(ManifestPlannerIntegrationTest, RankCapForPlanner) {
  ArtifactManifest manifest = createValidManifest();
  manifest.rank_cap = 256;
  manifest.rank_status = 200;
  EXPECT_EQ(manifest.rank_cap, 256u);
  EXPECT_EQ(manifest.rank_status, 200u);
  EXPECT_TRUE(manifest.validate());  // Should be valid
}

TEST_F(ManifestPlannerIntegrationTest, DeltaLagForFreshness) {
  ArtifactManifest manifest = createValidManifest();
  manifest.source_seq_start = 0;
  manifest.source_seq_end = 10000;
  manifest.delta_lag = 500;
  EXPECT_EQ(manifest.delta_lag, 500u);
  // Planner can determine artifact is 5% behind source
  double freshness_percent = (10000.0 - manifest.delta_lag) / 10000.0;
  EXPECT_NEAR(freshness_percent, 0.95, 0.01);
}

TEST_F(ManifestPlannerIntegrationTest, ResidualForAccuracy) {
  ArtifactManifest manifest = createValidManifest();
  manifest.residual = 0.001;  // 0.1% error
  EXPECT_NEAR(manifest.residual, 0.001, 0.0001);
  // Planner can determine if artifact is suitable for accuracy-critical queries
}

TEST_F(ManifestPlannerIntegrationTest, ArtifactAgeTracking) {
  ArtifactManifest manifest = createValidManifest();
  manifest.created_at_unix_sec = now_unix_sec_ - 86400;  // 1 day ago
  manifest.artifact_age_ms = (now_unix_sec_ - manifest.created_at_unix_sec) * 1000;
  EXPECT_EQ(manifest.artifact_age_ms, 86400000u);  // 1 day in milliseconds
}

TEST_F(ManifestPlannerIntegrationTest, ReconstructionInstructions) {
  ArtifactManifest manifest = createValidManifest();
  manifest.rebuild_state = RebuildState::PATCHED;
  manifest.reconstruction_instructions = "apply patches [p1, p2, p3]";
  EXPECT_EQ(manifest.rebuild_state, RebuildState::PATCHED);
  EXPECT_FALSE(manifest.reconstruction_instructions.empty());
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(ArtifactManifestTest, ComplexManifestScenario) {
  // Scenario: LoRA adapter with dynamic updates
  ArtifactManifest manifest;
  manifest.artifact_id = "LoRA:v2.1:medical-domain:gpt4-32k";
  manifest.artifact_class = ArtifactClass::DERIVED;
  manifest.truth_semantic = TruthSemantic::TRUTH_ADJACENT;
  manifest.lifecycle_state = LifecycleState::STALE;
  manifest.version = "2.1.0-patch-3";
  manifest.content_hash = "a1b2c3d4e5f6a7b8c9d0e1f2a3b4c5d6e7f8a9b0c1d2e3f4a5b6c7d8e9f0a1";

  // Temporal metadata
  manifest.created_at_unix_sec = now_unix_sec_ - 604800;  // 1 week ago
  manifest.updated_at_unix_sec = now_unix_sec_ - 86400;   // 1 day ago
  manifest.last_verified_unix_sec = now_unix_sec_ - 3600; // 1 hour ago
  manifest.last_rebuild_at_unix_sec = now_unix_sec_ - 172800; // 2 days ago
  manifest.staleness_threshold_sec = 3600; // 1 hour

  // Dynamic update tracking
  manifest.source_seq_start = 0;
  manifest.source_seq_end = 50000;
  manifest.delta_lag = 2500;
  manifest.artifact_age_ms = 604800000; // 1 week in ms

  // Quality metrics
  manifest.residual = 0.025; // 2.5% error
  manifest.rank_cap = 512;
  manifest.rank_status = 480;

  // Update history
  manifest.rebuild_state = RebuildState::PATCHED;
  manifest.update_mode = UpdateMode::PARTIAL_REFIT;
  manifest.invalidation_reason = InvalidationReason::UNKNOWN;

  // Provenance
  manifest.source_artifact_id = "base:gpt4-32k:v1";
  manifest.provenance_chain = {"base:v1", "adapter:v1", "medical-refit:v1", "patch:v3"};
  manifest.reconstruction_instructions = "apply 3 sequential patches to medical-refit v1 adapter";

  // Placement
  manifest.shard_placements = {"shard-us-west-1", "shard-us-east-1", "shard-eu-central-1"};
  manifest.requires_full_replication = false;
  manifest.is_rebuildable = true;
  manifest.replication_factor = 2;
  manifest.backup_shard_placements = {"shard-backup-1"};

  // Compatibility
  manifest.compatibility_metadata = {
      {"model_version", "GPT-4-32k-v1"},
      {"cuda_version", "12.2"},
      {"gpu_arch", "RTX40xx"}
  };
  manifest.min_planner_version = "2.4.0";
  manifest.advisory_only = false;

  // Custom metadata
  manifest.custom_attributes = {
      {"training_domain", "medical"},
      {"training_date", "2026-06-15"},
      {"training_samples", "50000"}
  };
  manifest.description = "Medical domain LoRA adapter with 3 patches applied";

  // Validate
  EXPECT_TRUE(manifest.validate());

  // Test state transitions
  EXPECT_TRUE(manifest.isStale(now_unix_sec_)); // Should be stale
  EXPECT_TRUE(manifest.isUsable(now_unix_sec_)); // Still usable

  // Test freshness
  double freshness = manifest.getFreshnessScore(now_unix_sec_);
  EXPECT_GT(freshness, 0.0);
  EXPECT_LT(freshness, 1.0);

  // Test serialization
  std::string json = manifest.toJSON();
  auto deserialized = ArtifactManifest::fromJSON(json);
  EXPECT_TRUE(deserialized.has_value());
  EXPECT_EQ(deserialized->artifact_id, manifest.artifact_id);
  EXPECT_EQ(deserialized->rebuild_state, RebuildState::PATCHED);
  EXPECT_EQ(deserialized->provenance_chain.size(), 4);
}

}  // namespace distributed_tensor
}  // namespace themis
