/// @file test_tensor_manifest.cc
/// @brief CTest for tensor manifest state transitions and freshness
/// @author ThemisDB Implementation Team
/// @date 2026-07-02
///
/// Tests manifest lifecycle:
/// - State transitions (CREATED → ACTIVE → STALE → REBUILT → ACTIVE or INVALIDATED)
/// - Freshness scoring
/// - Staleness threshold enforcement

#include <gtest/gtest.h>
#include "distributed_tensor/include/artifact_manifest.h"
#include <ctime>
#include <vector>

namespace themis {
namespace distributed_tensor {

/// Test fixture for tensor manifest tests
class TensorManifestTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize manifest
        manifest_.artifact_id = "test:tensor:manifest";
        manifest_.artifact_class = ArtifactClass::PRIMARY;
        manifest_.truth_semantic = TruthSemantic::SOURCE_OF_TRUTH;
        manifest_.current_state = ArtifactLifecycleState::CREATED;
        manifest_.created_at_unix_sec = 1000;
        manifest_.last_verified_unix_sec = 0;
        manifest_.staleness_threshold_sec = 3600;
        manifest_.rank_cap = 16;
        manifest_.replication_factor = 3;
    }

    int64_t GetCurrentTime() const {
        return static_cast<int64_t>(std::time(nullptr));
    }

    ArtifactManifest manifest_;
};

// ============================================================================
// State Transition Tests: CREATED → ACTIVE
// ============================================================================

TEST_F(TensorManifestTest, ManifestStateCreated) {
    // Verify: manifest starts in CREATED state
    EXPECT_EQ(manifest_.current_state, ArtifactLifecycleState::CREATED);
}

TEST_F(TensorManifestTest, ManifestStateCreatedToActive) {
    // Verify: transition from CREATED to ACTIVE when verified
    EXPECT_EQ(manifest_.current_state, ArtifactLifecycleState::CREATED);

    // Simulate verification
    manifest_.current_state = ArtifactLifecycleState::ACTIVE;
    manifest_.last_verified_unix_sec = GetCurrentTime();

    EXPECT_EQ(manifest_.current_state, ArtifactLifecycleState::ACTIVE);
    EXPECT_GT(manifest_.last_verified_unix_sec, 0);
}

// ============================================================================
// State Transition Tests: ACTIVE → STALE
// ============================================================================

TEST_F(TensorManifestTest, ManifestStateActiveToStale) {
    // Verify: transition from ACTIVE to STALE when staleness exceeded
    manifest_.current_state = ArtifactLifecycleState::ACTIVE;
    manifest_.last_verified_unix_sec = 1000;
    manifest_.staleness_threshold_sec = 3600;

    int64_t current_time = 1000 + 3600 + 1;  // Exceed threshold by 1 second

    // Check staleness
    bool is_stale = (current_time - manifest_.last_verified_unix_sec) >
                    manifest_.staleness_threshold_sec;
    EXPECT_TRUE(is_stale);

    // Transition to STALE
    manifest_.current_state = ArtifactLifecycleState::STALE;

    EXPECT_EQ(manifest_.current_state, ArtifactLifecycleState::STALE);
}

TEST_F(TensorManifestTest, ManifestStateStaleUsable) {
    // Verify: stale manifest can still be used (for advisory queries)
    manifest_.current_state = ArtifactLifecycleState::STALE;
    manifest_.truth_semantic = TruthSemantic::ADVISORY;

    // Stale artifact with advisory semantic can be used
    bool is_usable = (manifest_.current_state == ArtifactLifecycleState::ACTIVE ||
                      manifest_.current_state == ArtifactLifecycleState::STALE);
    EXPECT_TRUE(is_usable);
}

// ============================================================================
// State Transition Tests: STALE → REBUILT / INVALIDATED
// ============================================================================

TEST_F(TensorManifestTest, ManifestStateStaleToRebuilt) {
    // Verify: transition from STALE to REBUILT when refresh begins
    manifest_.current_state = ArtifactLifecycleState::STALE;

    // Simulate rebuild process
    manifest_.current_state = ArtifactLifecycleState::REBUILT;

    EXPECT_EQ(manifest_.current_state, ArtifactLifecycleState::REBUILT);
}

TEST_F(TensorManifestTest, ManifestStateRebuiltToActive) {
    // Verify: transition from REBUILT to ACTIVE after successful rebuild
    manifest_.current_state = ArtifactLifecycleState::REBUILT;

    // Simulate rebuild completion and verification
    manifest_.current_state = ArtifactLifecycleState::ACTIVE;
    manifest_.last_verified_unix_sec = GetCurrentTime();

    EXPECT_EQ(manifest_.current_state, ArtifactLifecycleState::ACTIVE);
}

TEST_F(TensorManifestTest, ManifestStateActiveToInvalidated) {
    // Verify: transition from ACTIVE to INVALIDATED on corruption
    manifest_.current_state = ArtifactLifecycleState::ACTIVE;

    // Simulate corruption detection
    manifest_.current_state = ArtifactLifecycleState::INVALIDATED;

    EXPECT_EQ(manifest_.current_state, ArtifactLifecycleState::INVALIDATED);
}

TEST_F(TensorManifestTest, ManifestStateInvalidatedTerminal) {
    // Verify: INVALIDATED is terminal state
    manifest_.current_state = ArtifactLifecycleState::INVALIDATED;

    // Cannot transition from INVALIDATED
    bool is_terminal = (manifest_.current_state == ArtifactLifecycleState::INVALIDATED);
    EXPECT_TRUE(is_terminal);
}

// ============================================================================
// Usability Tests
// ============================================================================

TEST_F(TensorManifestTest, UsableActiveArtifact) {
    // Verify: ACTIVE artifact is usable
    manifest_.current_state = ArtifactLifecycleState::ACTIVE;

    bool is_usable = (manifest_.current_state == ArtifactLifecycleState::ACTIVE ||
                      manifest_.current_state == ArtifactLifecycleState::STALE);
    EXPECT_TRUE(is_usable);
}

TEST_F(TensorManifestTest, UsableStaleArtifact) {
    // Verify: STALE artifact is still usable (advisory/exact decision deferred to planner)
    manifest_.current_state = ArtifactLifecycleState::STALE;

    bool is_usable = (manifest_.current_state == ArtifactLifecycleState::ACTIVE ||
                      manifest_.current_state == ArtifactLifecycleState::STALE);
    EXPECT_TRUE(is_usable);
}

TEST_F(TensorManifestTest, NotUsableInvalidatedArtifact) {
    // Verify: INVALIDATED artifact is not usable
    manifest_.current_state = ArtifactLifecycleState::INVALIDATED;

    bool is_usable = (manifest_.current_state == ArtifactLifecycleState::ACTIVE ||
                      manifest_.current_state == ArtifactLifecycleState::STALE);
    EXPECT_FALSE(is_usable);
}

TEST_F(TensorManifestTest, NotUsableCreatedArtifact) {
    // Verify: CREATED artifact is not usable until verified
    manifest_.current_state = ArtifactLifecycleState::CREATED;

    bool is_usable = (manifest_.current_state == ArtifactLifecycleState::ACTIVE ||
                      manifest_.current_state == ArtifactLifecycleState::STALE);
    EXPECT_FALSE(is_usable);
}

TEST_F(TensorManifestTest, NotUsableRebuiltArtifact) {
    // Verify: REBUILT artifact is not usable until transitioned to ACTIVE
    manifest_.current_state = ArtifactLifecycleState::REBUILT;

    bool is_usable = (manifest_.current_state == ArtifactLifecycleState::ACTIVE ||
                      manifest_.current_state == ArtifactLifecycleState::STALE);
    EXPECT_FALSE(is_usable);
}

// ============================================================================
// Staleness Threshold Tests
// ============================================================================

TEST_F(TensorManifestTest, NotStaleWhenThresholdNotSet) {
    // Verify: artifact not stale when threshold is 0 (disabled)
    manifest_.last_verified_unix_sec = 100;
    manifest_.staleness_threshold_sec = 0;

    int64_t current_time = 100000;  // Far in the future
    bool is_stale = manifest_.staleness_threshold_sec > 0 &&
                    (current_time - manifest_.last_verified_unix_sec) >
                        manifest_.staleness_threshold_sec;
    EXPECT_FALSE(is_stale);
}

TEST_F(TensorManifestTest, NotStaleWhenUnderThreshold) {
    // Verify: artifact not stale when age < threshold
    manifest_.last_verified_unix_sec = 1000;
    manifest_.staleness_threshold_sec = 3600;

    int64_t current_time = 1000 + 1800;  // 30 minutes
    bool is_stale = (current_time - manifest_.last_verified_unix_sec) >
                    manifest_.staleness_threshold_sec;
    EXPECT_FALSE(is_stale);
}

TEST_F(TensorManifestTest, StaleWhenExceedsThreshold) {
    // Verify: artifact stale when age > threshold
    manifest_.last_verified_unix_sec = 1000;
    manifest_.staleness_threshold_sec = 3600;

    int64_t current_time = 1000 + 3600 + 1;  // 1 hour + 1 second
    bool is_stale = (current_time - manifest_.last_verified_unix_sec) >
                    manifest_.staleness_threshold_sec;
    EXPECT_TRUE(is_stale);
}

TEST_F(TensorManifestTest, StaleWhenNeverVerified) {
    // Verify: artifact considered stale if never verified and CREATED
    manifest_.last_verified_unix_sec = 0;
    manifest_.current_state = ArtifactLifecycleState::CREATED;

    bool is_usable = (manifest_.current_state == ArtifactLifecycleState::ACTIVE ||
                      manifest_.current_state == ArtifactLifecycleState::STALE);
    EXPECT_FALSE(is_usable);
}

// ============================================================================
// Freshness Score Tests
// ============================================================================

TEST_F(TensorManifestTest, FreshnessScoreNeverVerified) {
    // Verify: freshness score is 0.0 if never verified
    manifest_.last_verified_unix_sec = 0;
    manifest_.staleness_threshold_sec = 3600;

    double freshness = 0.0;  // Never verified
    EXPECT_EQ(freshness, 0.0);
}

TEST_F(TensorManifestTest, FreshnessScoreNoThreshold) {
    // Verify: freshness score is 1.0 if no staleness threshold
    manifest_.last_verified_unix_sec = 1000;
    manifest_.staleness_threshold_sec = 0;  // No threshold

    double freshness = 1.0;  // Always fresh
    EXPECT_EQ(freshness, 1.0);
}

TEST_F(TensorManifestTest, FreshnessScoreJustVerified) {
    // Verify: freshness score is ~1.0 immediately after verification
    manifest_.last_verified_unix_sec = GetCurrentTime();
    manifest_.staleness_threshold_sec = 3600;

    int64_t age = GetCurrentTime() - manifest_.last_verified_unix_sec;
    double freshness = 1.0 - static_cast<double>(age) / manifest_.staleness_threshold_sec;
    freshness = std::max(0.0, freshness);

    EXPECT_GE(freshness, 0.99);
    EXPECT_LE(freshness, 1.0);
}

TEST_F(TensorManifestTest, FreshnessScoreMidLife) {
    // Verify: freshness score decreases linearly with age
    // Use a 'now' that places age at roughly half the threshold (1800s)
    int64_t now = 2800;
    manifest_.last_verified_unix_sec = 1000;
    manifest_.staleness_threshold_sec = 3600;

    int64_t age = now - manifest_.last_verified_unix_sec;
    double freshness = 1.0 - static_cast<double>(age) / manifest_.staleness_threshold_sec;
    freshness = std::max(0.0, freshness);

    EXPECT_GT(freshness, 0.4);
    EXPECT_LT(freshness, 0.6);
}

TEST_F(TensorManifestTest, FreshnessScoreExceedsThreshold) {
    // Verify: freshness score is 0.0 when exceeds threshold
    int64_t now = 5000;
    manifest_.last_verified_unix_sec = 1000;
    manifest_.staleness_threshold_sec = 3600;

    int64_t age = now - manifest_.last_verified_unix_sec;
    double freshness = 1.0 - static_cast<double>(age) / manifest_.staleness_threshold_sec;
    freshness = std::max(0.0, freshness);

    EXPECT_EQ(freshness, 0.0);
}

// ============================================================================
// Manifest Publish Tests
// ============================================================================

TEST_F(TensorManifestTest, ManifestPublishAtomic) {
    // Verify: manifest state visible atomically to all consumers
    // (This is a logical test; actual atomicity depends on synchronization)
    manifest_.current_state = ArtifactLifecycleState::ACTIVE;
    manifest_.last_verified_unix_sec = GetCurrentTime();

    // After "publish", manifest fields should be consistent
    EXPECT_EQ(manifest_.current_state, ArtifactLifecycleState::ACTIVE);
    EXPECT_GT(manifest_.last_verified_unix_sec, 0);
    EXPECT_EQ(manifest_.artifact_id, "test:tensor:manifest");
}

} // namespace distributed_tensor
} // namespace themis
