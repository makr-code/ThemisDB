/**
 * @file artifact_lifecycle_test.cc
 * @brief Unit tests for EPIC 2.6 Artifact Lifecycle management.
 *
 * Tests cover all state transitions, staleness detection, batch operations,
 * and edge cases.
 */

#include <gtest/gtest.h>

#include "evaluation/include/artifact_lifecycle.h"

namespace themis {
namespace evaluation {
namespace {

// ---------------------------------------------------------------------------
// Test Fixtures
// ---------------------------------------------------------------------------

class ArtifactLifecycleTest : public ::testing::Test {
 protected:
    ArtifactLifecycleManager manager_;

    LifecycleMetadata createTestMetadata(
        const std::string& id,
        LifecycleState state = LifecycleState::READY,
        std::uint32_t age_ms = 1000,
        std::uint64_t delta_lag = 100,
        double residual = 0.01
    ) {
        return LifecycleMetadata{
            .artifact_id = id,
            .state = state,
            .invalidation_reason = InvalidationReason::UNKNOWN,
            .source_seq_start = 1000,
            .source_seq_end = 2000,
            .delta_lag = delta_lag,
            .artifact_age_ms = age_ms,
            .approximation_residual = residual,
            .residual_variance = 0.001,
            .max_permissible_rank = 100,
        };
    }
};

// ---------------------------------------------------------------------------
// LifecycleState Computation Tests
// ---------------------------------------------------------------------------

TEST_F(ArtifactLifecycleTest, ComputeStateReadyNoThresholds) {
    auto metadata = createTestMetadata("test1", LifecycleState::READY);
    StalenessPolicy policy;  // Empty policy

    auto state = manager_.computeState(metadata, policy);
    EXPECT_EQ(state, LifecycleState::READY);
}

TEST_F(ArtifactLifecycleTest, ComputeStateReadyAgeThresholdNotExceeded) {
    auto metadata = createTestMetadata("test1", LifecycleState::READY, 1000);
    StalenessPolicy policy;
    policy.withAgeThresholdMs(2000);

    auto state = manager_.computeState(metadata, policy);
    EXPECT_EQ(state, LifecycleState::READY);
}

TEST_F(ArtifactLifecycleTest, ComputeStateReadyAgeThresholdExceeded) {
    auto metadata = createTestMetadata("test1", LifecycleState::READY, 3000);
    StalenessPolicy policy;
    policy.withAgeThresholdMs(2000);

    auto state = manager_.computeState(metadata, policy);
    EXPECT_EQ(state, LifecycleState::STALE);
}

TEST_F(ArtifactLifecycleTest, ComputeStateDeltaLagThresholdExceeded) {
    auto metadata = createTestMetadata("test1", LifecycleState::READY, 1000, 2000);
    StalenessPolicy policy;
    policy.withDeltaLagThreshold(1000);

    auto state = manager_.computeState(metadata, policy);
    EXPECT_EQ(state, LifecycleState::STALE);
}

TEST_F(ArtifactLifecycleTest, ComputeStateResidualThresholdExceeded) {
    auto metadata = createTestMetadata("test1", LifecycleState::READY, 1000, 100, 0.05);
    StalenessPolicy policy;
    policy.withResidualThreshold(0.02);

    auto state = manager_.computeState(metadata, policy);
    EXPECT_EQ(state, LifecycleState::STALE);
}

TEST_F(ArtifactLifecycleTest, ComputeStateRankCapThresholdBreached) {
    auto metadata = createTestMetadata("test1", LifecycleState::READY);
    metadata.max_permissible_rank = 50;
    StalenessPolicy policy;
    policy.withRankCapThreshold(100);

    auto state = manager_.computeState(metadata, policy);
    EXPECT_EQ(state, LifecycleState::STALE);
}

TEST_F(ArtifactLifecycleTest, ComputeStateResidualVarianceThresholdExceeded) {
    auto metadata = createTestMetadata("test1", LifecycleState::READY);
    metadata.residual_variance = 0.005;
    StalenessPolicy policy;
    policy.withResidualVarianceThreshold(0.002);

    auto state = manager_.computeState(metadata, policy);
    EXPECT_EQ(state, LifecycleState::STALE);
}

TEST_F(ArtifactLifecycleTest, ComputStateInvalidatedPreserved) {
    auto metadata = createTestMetadata("test1", LifecycleState::INVALIDATED);
    StalenessPolicy policy;
    policy.withAgeThresholdMs(500);  // Would trigger staleness if checked

    auto state = manager_.computeState(metadata, policy);
    EXPECT_EQ(state, LifecycleState::INVALIDATED);
}

TEST_F(ArtifactLifecycleTest, ComputeStateRebuildingPreserved) {
    auto metadata = createTestMetadata("test1", LifecycleState::REBUILDING);
    StalenessPolicy policy;

    auto state = manager_.computeState(metadata, policy);
    EXPECT_EQ(state, LifecycleState::REBUILDING);
}

TEST_F(ArtifactLifecycleTest, ComputeStatePristineTransitionsToReady) {
    auto metadata = createTestMetadata("test1", LifecycleState::PRISTINE);
    StalenessPolicy policy;

    auto state = manager_.computeState(metadata, policy);
    EXPECT_EQ(state, LifecycleState::READY);
}

// ---------------------------------------------------------------------------
// Usability Tests
// ---------------------------------------------------------------------------

TEST_F(ArtifactLifecycleTest, IsUsableForPlanningReadyTrue) {
    EXPECT_TRUE(manager_.isUsableForPlanning(LifecycleState::READY));
}

TEST_F(ArtifactLifecycleTest, IsUsableForPlanningStaleTrue) {
    EXPECT_TRUE(manager_.isUsableForPlanning(LifecycleState::STALE));
}

TEST_F(ArtifactLifecycleTest, IsUsableForPlanningInvalidatedFalse) {
    EXPECT_FALSE(manager_.isUsableForPlanning(LifecycleState::INVALIDATED));
}

TEST_F(ArtifactLifecycleTest, IsUsableForPlanningRebuildingFalse) {
    EXPECT_FALSE(manager_.isUsableForPlanning(LifecycleState::REBUILDING));
}

TEST_F(ArtifactLifecycleTest, IsUsableForPlanningFailedFalse) {
    EXPECT_FALSE(manager_.isUsableForPlanning(LifecycleState::FAILED));
}

TEST_F(ArtifactLifecycleTest, IsUsableForPlanningPristineFalse) {
    EXPECT_FALSE(manager_.isUsableForPlanning(LifecycleState::PRISTINE));
}

// ---------------------------------------------------------------------------
// Rebuild Requirement Tests
// ---------------------------------------------------------------------------

TEST_F(ArtifactLifecycleTest, RequiresImmediateRebuildInvalidatedTrue) {
    EXPECT_TRUE(manager_.requiresImmediateRebuild(LifecycleState::INVALIDATED));
}

TEST_F(ArtifactLifecycleTest, RequiresImmediateRebuildFailedTrue) {
    EXPECT_TRUE(manager_.requiresImmediateRebuild(LifecycleState::FAILED));
}

TEST_F(ArtifactLifecycleTest, RequiresImmediateRebuildReadyFalse) {
    EXPECT_FALSE(manager_.requiresImmediateRebuild(LifecycleState::READY));
}

TEST_F(ArtifactLifecycleTest, RequiresImmediateRebuildStaleFalse) {
    EXPECT_FALSE(manager_.requiresImmediateRebuild(LifecycleState::STALE));
}

// ---------------------------------------------------------------------------
// State Transition Tests
// ---------------------------------------------------------------------------

TEST_F(ArtifactLifecycleTest, InvalidateTransitionFromReady) {
    auto metadata = createTestMetadata("test1", LifecycleState::READY);
    auto original_state = metadata.state;

    auto updated = ArtifactLifecycleManager::invalidate(
        metadata, InvalidationReason::INTEGRITY_CHECK_FAILED);

    EXPECT_EQ(updated.state, LifecycleState::INVALIDATED);
    EXPECT_EQ(updated.invalidation_reason, InvalidationReason::INTEGRITY_CHECK_FAILED);
    EXPECT_NE(updated.state_change_timestamp_ms, 0);
}

TEST_F(ArtifactLifecycleTest, BeginRebuildIncrementsAttemptCount) {
    auto metadata = createTestMetadata("test1", LifecycleState::INVALIDATED);
    metadata.rebuild_attempt_count = 0;

    auto updated = ArtifactLifecycleManager::beginRebuild(metadata);

    EXPECT_EQ(updated.state, LifecycleState::REBUILDING);
    EXPECT_EQ(updated.rebuild_attempt_count, 1);
}

TEST_F(ArtifactLifecycleTest, CompleteRebuildSuccessReturnsReady) {
    auto metadata = createTestMetadata("test1", LifecycleState::REBUILDING);

    auto updated = ArtifactLifecycleManager::completeRebuildSuccess(
        metadata, 500, 50, 0.005);

    EXPECT_EQ(updated.state, LifecycleState::READY);
    EXPECT_EQ(updated.artifact_age_ms, 500);
    EXPECT_EQ(updated.delta_lag, 50);
    EXPECT_EQ(updated.approximation_residual, 0.005);
    EXPECT_TRUE(updated.last_successful_rebuild_ms.has_value());
}

TEST_F(ArtifactLifecycleTest, CompleteRebuildFailureReturnsFailed) {
    auto metadata = createTestMetadata("test1", LifecycleState::REBUILDING);

    auto updated = ArtifactLifecycleManager::completeRebuildFailure(metadata);

    EXPECT_EQ(updated.state, LifecycleState::FAILED);
    EXPECT_TRUE(updated.last_failed_rebuild_ms.has_value());
}

TEST_F(ArtifactLifecycleTest, MarkReadyTransitionsToReady) {
    auto metadata = createTestMetadata("test1", LifecycleState::PRISTINE);

    auto updated = ArtifactLifecycleManager::markReady(metadata, 1000, 100, 0.01);

    EXPECT_EQ(updated.state, LifecycleState::READY);
    EXPECT_EQ(updated.artifact_age_ms, 1000);
    EXPECT_EQ(updated.delta_lag, 100);
    EXPECT_EQ(updated.approximation_residual, 0.01);
}

// ---------------------------------------------------------------------------
// Staleness Diagnosis Tests
// ---------------------------------------------------------------------------

TEST_F(ArtifactLifecycleTest, DiagnoseStalenessCauseAge) {
    auto metadata = createTestMetadata("test1", LifecycleState::READY, 3000);
    StalenessPolicy policy;
    policy.withAgeThresholdMs(2000);

    auto cause = manager_.diagnoseStalenessCause(metadata, policy);
    EXPECT_TRUE(cause.has_value());
    EXPECT_TRUE(cause.value().find("Age threshold exceeded") != std::string::npos);
}

TEST_F(ArtifactLifecycleTest, DiagnoseStalenessCauseDeltaLag) {
    auto metadata = createTestMetadata("test1", LifecycleState::READY, 1000, 2000);
    StalenessPolicy policy;
    policy.withDeltaLagThreshold(1000);

    auto cause = manager_.diagnoseStalenessCause(metadata, policy);
    EXPECT_TRUE(cause.has_value());
    EXPECT_TRUE(cause.value().find("Delta lag threshold exceeded") != std::string::npos);
}

TEST_F(ArtifactLifecycleTest, DiagnoseStalenessCauseNone) {
    auto metadata = createTestMetadata("test1", LifecycleState::READY, 1000);
    StalenessPolicy policy;
    policy.withAgeThresholdMs(2000);

    auto cause = manager_.diagnoseStalenessCause(metadata, policy);
    EXPECT_FALSE(cause.has_value());
}

// ---------------------------------------------------------------------------
// Batch Operation Tests
// ---------------------------------------------------------------------------

TEST_F(ArtifactLifecycleTest, ComputeStatesBatch) {
    std::vector<LifecycleMetadata> batch;
    batch.push_back(createTestMetadata("test1", LifecycleState::READY, 1000));
    batch.push_back(createTestMetadata("test2", LifecycleState::READY, 3000));
    batch.push_back(createTestMetadata("test3", LifecycleState::STALE, 2000));

    StalenessPolicy policy;
    policy.withAgeThresholdMs(2000);

    auto states = manager_.computeStatesBatch(batch, policy);

    EXPECT_EQ(states.size(), 3);
    EXPECT_EQ(states[0], LifecycleState::READY);
    EXPECT_EQ(states[1], LifecycleState::STALE);
    EXPECT_EQ(states[2], LifecycleState::STALE);
}

TEST_F(ArtifactLifecycleTest, FilterUsableArtifacts) {
    std::vector<LifecycleMetadata> batch;
    batch.push_back(createTestMetadata("test1", LifecycleState::READY));
    batch.push_back(createTestMetadata("test2", LifecycleState::STALE));
    batch.push_back(createTestMetadata("test3", LifecycleState::INVALIDATED));
    batch.push_back(createTestMetadata("test4", LifecycleState::REBUILDING));

    StalenessPolicy policy;

    auto usable = manager_.filterUsableArtifacts(batch, policy);

    EXPECT_EQ(usable.size(), 2);
    EXPECT_EQ(usable[0].artifact_id, "test1");
    EXPECT_EQ(usable[1].artifact_id, "test2");
}

TEST_F(ArtifactLifecycleTest, IdentifyRebuildCandidates) {
    std::vector<LifecycleMetadata> batch;
    batch.push_back(createTestMetadata("test1", LifecycleState::READY));
    batch.push_back(createTestMetadata("test2", LifecycleState::INVALIDATED));
    batch.push_back(createTestMetadata("test3", LifecycleState::STALE));
    batch.push_back(createTestMetadata("test4", LifecycleState::FAILED));

    auto candidates = manager_.identifyRebuildCandidates(batch);

    EXPECT_EQ(candidates.size(), 2);
    EXPECT_EQ(candidates[0].artifact_id, "test2");
    EXPECT_EQ(candidates[1].artifact_id, "test4");
}

// ---------------------------------------------------------------------------
// String Conversion Tests
// ---------------------------------------------------------------------------

TEST_F(ArtifactLifecycleTest, LifecycleStateToStringPristine) {
    EXPECT_EQ(lifecycleStateToString(LifecycleState::PRISTINE), "PRISTINE");
}

TEST_F(ArtifactLifecycleTest, LifecycleStateToStringReady) {
    EXPECT_EQ(lifecycleStateToString(LifecycleState::READY), "READY");
}

TEST_F(ArtifactLifecycleTest, LifecycleStateToStringStale) {
    EXPECT_EQ(lifecycleStateToString(LifecycleState::STALE), "STALE");
}

TEST_F(ArtifactLifecycleTest, LifecycleStateToStringInvalidated) {
    EXPECT_EQ(lifecycleStateToString(LifecycleState::INVALIDATED), "INVALIDATED");
}

TEST_F(ArtifactLifecycleTest, LifecycleStateToStringRebuilding) {
    EXPECT_EQ(lifecycleStateToString(LifecycleState::REBUILDING), "REBUILDING");
}

TEST_F(ArtifactLifecycleTest, LifecycleStateToStringFailed) {
    EXPECT_EQ(lifecycleStateToString(LifecycleState::FAILED), "FAILED");
}

TEST_F(ArtifactLifecycleTest, StringToLifecycleStateReady) {
    auto state = stringToLifecycleState("READY");
    EXPECT_TRUE(state.has_value());
    EXPECT_EQ(*state, LifecycleState::READY);
}

TEST_F(ArtifactLifecycleTest, StringToLifecycleStateInvalid) {
    auto state = stringToLifecycleState("INVALID_STATE");
    EXPECT_FALSE(state.has_value());
}

TEST_F(ArtifactLifecycleTest, InvalidationReasonToStringIntegrityCheckFailed) {
    EXPECT_EQ(invalidationReasonToString(InvalidationReason::INTEGRITY_CHECK_FAILED),
              "INTEGRITY_CHECK_FAILED");
}

TEST_F(ArtifactLifecycleTest, InvalidationReasonToStringStalenessExceeded) {
    EXPECT_EQ(invalidationReasonToString(InvalidationReason::STALENESS_EXCEEDED),
              "STALENESS_EXCEEDED");
}

TEST_F(ArtifactLifecycleTest, StringToInvalidationReasonPolicyViolation) {
    auto reason = stringToInvalidationReason("POLICY_VIOLATION");
    EXPECT_TRUE(reason.has_value());
    EXPECT_EQ(*reason, InvalidationReason::POLICY_VIOLATION);
}

TEST_F(ArtifactLifecycleTest, StringToInvalidationReasonInvalid) {
    auto reason = stringToInvalidationReason("INVALID_REASON");
    EXPECT_FALSE(reason.has_value());
}

// ---------------------------------------------------------------------------
// Integration Tests
// ---------------------------------------------------------------------------

TEST_F(ArtifactLifecycleTest, FullLifecycleReadyToStaleToInvalidatedToReady) {
    // Start with READY artifact
    auto metadata = createTestMetadata("test1", LifecycleState::READY, 1000);
    StalenessPolicy policy;
    policy.withAgeThresholdMs(2000);

    // Compute state (should remain READY)
    auto state1 = manager_.computeState(metadata, policy);
    EXPECT_EQ(state1, LifecycleState::READY);

    // Age exceeds threshold
    metadata.artifact_age_ms = 3000;
    auto state2 = manager_.computeState(metadata, policy);
    EXPECT_EQ(state2, LifecycleState::STALE);

    // Mark for invalidation
    auto invalidated = ArtifactLifecycleManager::invalidate(
        metadata, InvalidationReason::STALENESS_EXCEEDED);
    EXPECT_EQ(invalidated.state, LifecycleState::INVALIDATED);

    // Begin rebuild
    auto rebuilding = ArtifactLifecycleManager::beginRebuild(invalidated);
    EXPECT_EQ(rebuilding.state, LifecycleState::REBUILDING);
    EXPECT_EQ(rebuilding.rebuild_attempt_count, 1);

    // Complete rebuild successfully
    auto ready = ArtifactLifecycleManager::completeRebuildSuccess(
        rebuilding, 500, 50, 0.005);
    EXPECT_EQ(ready.state, LifecycleState::READY);
    EXPECT_EQ(ready.artifact_age_ms, 500);
}

}  // namespace
}  // namespace evaluation
}  // namespace themis

// ============================================================================
// Phase 4 Expansion: FAILED State & Overlapping Staleness Thresholds
// ============================================================================

namespace themis {
namespace evaluation {
namespace {

/// Phase 4 Test: Verify FAILED state transitions and state machine.
TEST_F(ArtifactLifecycleTest, Phase4_FailedState_IsUsableForPlanningFalse) {
    // FAILED state should never be usable for planning
    EXPECT_FALSE(manager_.isUsableForPlanning(LifecycleState::FAILED));
}

/// Phase 4 Test: Verify rebuild attempt increment on transition to REBUILDING.
TEST_F(ArtifactLifecycleTest, Phase4_RebuildAttemptCountIncrementsMonotonically) {
    auto metadata = createTestMetadata("test1", LifecycleState::FAILED);
    metadata.rebuild_attempt_count = 3;
    
    auto rebuilding = ArtifactLifecycleManager::beginRebuild(metadata);
    EXPECT_EQ(rebuilding.state, LifecycleState::REBUILDING);
    EXPECT_EQ(rebuilding.rebuild_attempt_count, 4);  // Incremented from 3
}

/// Phase 4 Test: Verify FAILED → REBUILDING transition via beginRebuild.
TEST_F(ArtifactLifecycleTest, Phase4_FailedToRebuildingTransition) {
    auto metadata = createTestMetadata("test1", LifecycleState::FAILED);
    
    auto rebuilding = ArtifactLifecycleManager::beginRebuild(metadata);
    
    EXPECT_EQ(rebuilding.state, LifecycleState::REBUILDING);
    EXPECT_NE(rebuilding.last_failed_rebuild_ms, metadata.last_failed_rebuild_ms);
}

/// Phase 4 Test: Verify overlapping age and delta lag staleness triggers.
TEST_F(ArtifactLifecycleTest, Phase4_OverlappingStalenessThresholds_AgeAndDeltaLag) {
    auto metadata = createTestMetadata("test1", LifecycleState::READY, 3000, 2000);
    StalenessPolicy policy;
    policy.withAgeThresholdMs(2000);      // age 3000 > 2000 → STALE
    policy.withDeltaLagThreshold(1000);   // delta_lag 2000 > 1000 → STALE
    
    auto state = manager_.computeState(metadata, policy);
    EXPECT_EQ(state, LifecycleState::STALE);
    
    // Diagnosis should identify one or both causes
    auto cause = manager_.diagnoseStalenessCause(metadata, policy);
    EXPECT_TRUE(cause.has_value());
}

/// Phase 4 Test: Verify overlapping residual and rank cap staleness triggers.
TEST_F(ArtifactLifecycleTest, Phase4_OverlappingStalenessThresholds_ResidualAndRankCap) {
    auto metadata = createTestMetadata("test1", LifecycleState::READY, 1000, 100, 0.05);
    metadata.max_permissible_rank = 80;  // Below threshold
    
    StalenessPolicy policy;
    policy.withResidualThreshold(0.02);  // residual 0.05 > 0.02 → STALE
    policy.withRankCapThreshold(100);    // rank 80 < 100 → STALE
    
    auto state = manager_.computeState(metadata, policy);
    EXPECT_EQ(state, LifecycleState::STALE);
}

/// Phase 4 Test: Verify InvalidationReason propagation in invalidate().
TEST_F(ArtifactLifecycleTest, Phase4_InvalidationReasonPropagation) {
    auto metadata = createTestMetadata("test1", LifecycleState::READY);
    
    std::vector<InvalidationReason> reasons = {
        InvalidationReason::INTEGRITY_CHECK_FAILED,
        InvalidationReason::STALENESS_EXCEEDED,
        InvalidationReason::POLICY_VIOLATION,
        InvalidationReason::SHARD_UNAVAILABLE
    };
    
    for (auto reason : reasons) {
        auto invalidated = ArtifactLifecycleManager::invalidate(metadata, reason);
        EXPECT_EQ(invalidated.state, LifecycleState::INVALIDATED);
        EXPECT_EQ(invalidated.invalidation_reason, reason);
    }
}

/// Phase 4 Test: Verify state machine rejects invalid transitions.
TEST_F(ArtifactLifecycleTest, Phase4_PreservesStateOnInvalidTransition) {
    // PRISTINE state should transition to READY, not remain PRISTINE
    auto metadata = createTestMetadata("test1", LifecycleState::PRISTINE);
    
    auto state = manager_.computeState(metadata, StalenessPolicy());
    EXPECT_EQ(state, LifecycleState::READY);
}

/// Phase 4 Test: Verify last_successful_rebuild_ms and last_failed_rebuild_ms timestamps.
TEST_F(ArtifactLifecycleTest, Phase4_RebuildTimestampsAreRecorded) {
    auto metadata = createTestMetadata("test1", LifecycleState::REBUILDING);
    
    // Success case
    auto success = ArtifactLifecycleManager::completeRebuildSuccess(
        metadata, 500, 50, 0.005);
    EXPECT_TRUE(success.last_successful_rebuild_ms.has_value());
    EXPECT_GT(success.last_successful_rebuild_ms.value(), 0u);
    
    // Failure case
    auto failure = ArtifactLifecycleManager::completeRebuildFailure(metadata);
    EXPECT_TRUE(failure.last_failed_rebuild_ms.has_value());
    EXPECT_GT(failure.last_failed_rebuild_ms.value(), 0u);
}

/// Phase 4 Test: Verify staleness diagnosis captures specific causes.
TEST_F(ArtifactLifecycleTest, Phase4_DiagnoseStalenessCauseCapturesDetails) {
    auto metadata = createTestMetadata("test1", LifecycleState::READY, 5000);
    StalenessPolicy policy;
    policy.withAgeThresholdMs(2000);
    
    auto cause = manager_.diagnoseStalenessCause(metadata, policy);
    EXPECT_TRUE(cause.has_value());
    // Cause should mention "Age" or similar
    EXPECT_TRUE(cause.value().find("Age") != std::string::npos ||
                cause.value().find("age") != std::string::npos);
}

/// Phase 4 Test: Verify batch operations preserve lifecycle state semantics.
TEST_F(ArtifactLifecycleTest, Phase4_BatchComputeStatesPreservesSemantics) {
    std::vector<LifecycleMetadata> batch;
    batch.push_back(createTestMetadata("test1", LifecycleState::READY, 500));   // Fresh
    batch.push_back(createTestMetadata("test2", LifecycleState::READY, 3000));  // Stale
    batch.push_back(createTestMetadata("test3", LifecycleState::INVALIDATED));   // Already invalid
    
    StalenessPolicy policy;
    policy.withAgeThresholdMs(2000);
    
    auto states = manager_.computeStatesBatch(batch, policy);
    
    EXPECT_EQ(states.size(), 3);
    EXPECT_EQ(states[0], LifecycleState::READY);      // Stays READY
    EXPECT_EQ(states[1], LifecycleState::STALE);      // Becomes STALE
    EXPECT_EQ(states[2], LifecycleState::INVALIDATED); // Stays INVALIDATED
}

}  // namespace
}  // namespace evaluation
}  // namespace themis
