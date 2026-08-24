/**
 * @file test_evaluation_artifact_lifecycle_focused.cpp
 * @brief Group AL — Unit tests for ArtifactLifecycleManager and StalenessPolicy.
 *
 * Exercises:
 *   AL1-AL4  computeState() state transitions
 *   AL5-AL6  isUsableForPlanning() and requiresImmediateRebuild() classification
 *   AL7-AL8  invalidate() and beginRebuild() transitions
 *   AL9      StalenessPolicy threshold wiring
 */

#include <gtest/gtest.h>
#include "artifact_lifecycle.h"
#include <cstdint>

using namespace themis::evaluation;

// ── helpers ───────────────────────────────────────────────────────────────────

static LifecycleMetadata make_ready_metadata(const std::string& id = "art1") {
    LifecycleMetadata m;
    m.artifact_id        = id;
    m.state              = LifecycleState::READY;
    m.artifact_age_ms    = 100;
    m.delta_lag          = 0;
    m.approximation_residual = 0.01;
    m.max_permissible_rank   = 16;
    return m;
}

// ── Group AL — Artifact Lifecycle ─────────────────────────────────────────────

// AL1: READY artifact with no threshold exceeded stays READY
TEST(EvaluationArtifactLifecycleFocusedTests, AL1_ReadyArtifact_BelowThresholds_StaysReady) {
    ArtifactLifecycleManager mgr;
    auto meta = make_ready_metadata();

    StalenessPolicy policy;
    policy.withAgeThresholdMs(10000)
          .withDeltaLagThreshold(1000)
          .withResidualThreshold(0.10);

    EXPECT_EQ(mgr.computeState(meta, policy), LifecycleState::READY);
}

// AL2: READY artifact exceeding age threshold becomes STALE
TEST(EvaluationArtifactLifecycleFocusedTests, AL2_ReadyArtifact_AgeExceeded_BecomesStale) {
    ArtifactLifecycleManager mgr;
    auto meta = make_ready_metadata();
    meta.artifact_age_ms = 20000;  // exceeds threshold of 5000

    StalenessPolicy policy;
    policy.withAgeThresholdMs(5000);

    EXPECT_EQ(mgr.computeState(meta, policy), LifecycleState::STALE);
}

// AL3: READY artifact exceeding delta_lag threshold becomes STALE
TEST(EvaluationArtifactLifecycleFocusedTests, AL3_ReadyArtifact_DeltaLagExceeded_BecomesStale) {
    ArtifactLifecycleManager mgr;
    auto meta = make_ready_metadata();
    meta.delta_lag = 5000;  // exceeds threshold of 100

    StalenessPolicy policy;
    policy.withDeltaLagThreshold(100);

    EXPECT_EQ(mgr.computeState(meta, policy), LifecycleState::STALE);
}

// AL4: INVALIDATED artifact remains INVALIDATED regardless of policy
TEST(EvaluationArtifactLifecycleFocusedTests, AL4_InvalidatedArtifact_StaysInvalidated) {
    ArtifactLifecycleManager mgr;
    auto meta = make_ready_metadata();
    meta.state = LifecycleState::INVALIDATED;

    StalenessPolicy policy;  // no thresholds set
    EXPECT_EQ(mgr.computeState(meta, policy), LifecycleState::INVALIDATED);
}

// AL5: isUsableForPlanning is true for READY and STALE
TEST(EvaluationArtifactLifecycleFocusedTests, AL5_IsUsableForPlanning_ReadyAndStale) {
    EXPECT_TRUE(ArtifactLifecycleManager::isUsableForPlanning(LifecycleState::READY));
    EXPECT_TRUE(ArtifactLifecycleManager::isUsableForPlanning(LifecycleState::STALE));
    EXPECT_FALSE(ArtifactLifecycleManager::isUsableForPlanning(LifecycleState::INVALIDATED));
    EXPECT_FALSE(ArtifactLifecycleManager::isUsableForPlanning(LifecycleState::REBUILDING));
    EXPECT_FALSE(ArtifactLifecycleManager::isUsableForPlanning(LifecycleState::FAILED));
}

// AL6: requiresImmediateRebuild is true for INVALIDATED and FAILED
TEST(EvaluationArtifactLifecycleFocusedTests, AL6_RequiresImmediateRebuild_InvalidatedAndFailed) {
    EXPECT_TRUE(ArtifactLifecycleManager::requiresImmediateRebuild(LifecycleState::INVALIDATED));
    EXPECT_TRUE(ArtifactLifecycleManager::requiresImmediateRebuild(LifecycleState::FAILED));
    EXPECT_FALSE(ArtifactLifecycleManager::requiresImmediateRebuild(LifecycleState::READY));
    EXPECT_FALSE(ArtifactLifecycleManager::requiresImmediateRebuild(LifecycleState::STALE));
    EXPECT_FALSE(ArtifactLifecycleManager::requiresImmediateRebuild(LifecycleState::REBUILDING));
}

// AL7: invalidate() transitions to INVALIDATED with correct reason
TEST(EvaluationArtifactLifecycleFocusedTests, AL7_Invalidate_SetsStateAndReason) {
    auto meta = make_ready_metadata();
    auto updated = ArtifactLifecycleManager::invalidate(
        meta, InvalidationReason::DELTA_LAG_EXCEEDED);

    EXPECT_EQ(updated.state, LifecycleState::INVALIDATED);
    EXPECT_EQ(updated.invalidation_reason, InvalidationReason::DELTA_LAG_EXCEEDED);
    EXPECT_EQ(updated.artifact_id, meta.artifact_id);  // id preserved
}

// AL8: beginRebuild() transitions to REBUILDING and increments attempt count
TEST(EvaluationArtifactLifecycleFocusedTests, AL8_BeginRebuild_SetsRebuildingAndIncrementsCount) {
    auto meta = ArtifactLifecycleManager::invalidate(
        make_ready_metadata(), InvalidationReason::AGE_EXCEEDED);
    auto rebuilding = ArtifactLifecycleManager::beginRebuild(meta);

    EXPECT_EQ(rebuilding.state, LifecycleState::REBUILDING);
    EXPECT_EQ(rebuilding.rebuild_attempt_count, meta.rebuild_attempt_count + 1u);
}

// AL9: StalenessPolicy residual threshold triggers STALE
TEST(EvaluationArtifactLifecycleFocusedTests, AL9_ResidualThreshold_TriggersStaleness) {
    ArtifactLifecycleManager mgr;
    auto meta = make_ready_metadata();
    meta.approximation_residual = 0.20;  // exceeds threshold of 0.05

    StalenessPolicy policy;
    policy.withResidualThreshold(0.05);

    EXPECT_EQ(mgr.computeState(meta, policy), LifecycleState::STALE);
}

// AL10: PRISTINE artifact with no staleness policy stays PRISTINE
TEST(EvaluationArtifactLifecycleFocusedTests, AL10_PristineArtifact_StaysPristine) {
    ArtifactLifecycleManager mgr;
    LifecycleMetadata meta;
    meta.artifact_id = "new";
    meta.state       = LifecycleState::PRISTINE;

    StalenessPolicy policy;  // no thresholds set
    EXPECT_EQ(mgr.computeState(meta, policy), LifecycleState::PRISTINE);
}
