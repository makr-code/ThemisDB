// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

/**
 * @file test_lifecycle_staleness_management.cpp
 * @brief Unit tests for derived-artifact lifecycle and staleness management (issue #5442).
 *
 * Test IDs:
 *   LSM-01  ArtifactManifest defaults to READY/ACTIVE lifecycle state
 *   LSM-02  ArtifactManifest defaults to ADVISORY_ONLY truth semantic
 *   LSM-03  ArtifactManifest defaults to DERIVED artifact class
 *   LSM-04  ArtifactClassifier::isValidCombination — DERIVED + ADVISORY_ONLY is valid
 *   LSM-05  ArtifactClassifier::isValidCombination — SOURCE_OF_TRUTH + GROUND_TRUTH is valid
 *   LSM-06  ArtifactClassifier::isValidCombination — DERIVED + GROUND_TRUTH is invalid
 *   LSM-07  ArtifactClassifier::isValidCombination — SOURCE_OF_TRUTH + ADVISORY_ONLY is invalid
 *   LSM-08  ArtifactLifecyclePolicy — only READY/STALE are usable for planning
 *   LSM-09  ArtifactManifest::isUsable — returns true for READY and STALE
 *   LSM-10  ArtifactManifest::isUsable — returns false for INVALIDATED, REBUILDING, FAILED
 *   LSM-11  ArtifactManifest::isStale — returns false when staleness_threshold_sec == 0
 *   LSM-12  ArtifactManifest::isStale — returns true when age exceeds threshold
 *   LSM-13  ArtifactManifest::isStale — returns false when age is within threshold
 *   LSM-14  ArtifactManifest::isStale — returns true when last_verified_unix_sec == 0 with threshold set
 *   LSM-15  ArtifactManifest::getFreshnessScore — returns 1.0 for age==0
 *   LSM-16  ArtifactManifest::getFreshnessScore — returns 0.0 for age>=threshold
 *   LSM-17  ArtifactManifest::getFreshnessScore — returns 0.5 for age == threshold/2
 *   LSM-18  ArtifactManifest::validate — rejects source_seq_end < source_seq_start
 *   LSM-19  ArtifactManifest::validate — rejects residual out of [0, 1000]
 *   LSM-20  ArtifactManifest::validate — rejects rank_status > rank_cap
 *   LSM-21  ArtifactManifest::validate — rejects invalid class/semantic combination
 *   LSM-22  ArtifactManifest::validate — accepts a well-formed lifecycle manifest
 *   LSM-23  InvalidationReason string round-trip
 *   LSM-24  RebuildState string round-trip
 *   LSM-25  UpdateMode string round-trip
 *   LSM-26  LifecycleState string round-trip (ArtifactLifecyclePolicy)
 *   LSM-27  ArtifactManifest::toJSON / fromJSON round-trip preserves lifecycle fields
 *   LSM-28  ArtifactInvalidationManager marks STALE correctly
 *   LSM-29  ArtifactInvalidationManager marks INVALIDATED with reason
 *   LSM-30  ArtifactInvalidationManager::shouldInvalidateForRankBreach detects breach
 *   LSM-31  ArtifactInvalidationManager::shouldInvalidateForResidual detects threshold breach
 *   LSM-32  ArtifactInvalidationManager::transitionToRebuilding sets REBUILDING mode and timestamp
 *   LSM-33  ArtifactInvalidationManager::transitionToReadyAfterRebuild rematerializes READY artifact
 *   LSM-34  ArtifactInvalidationManager::transitionToFailed records failed rebuild state
 *   LSM-35  ArtifactInvalidationManager::shouldRejectForPlanner enforces lifecycle freshness gates
 */

#include "src/distributed_tensor/include/artifact_manifest.h"
#include "src/distributed_tensor/include/tensor_artifact_classes.h"
#include "src/distributed_tensor/include/artifact_invalidation.h"

#include <gtest/gtest.h>

namespace themis { namespace distributed_tensor { 
namespace {

// ---------------------------------------------------------------------------
// Helper: build a minimal valid lifecycle manifest
// ---------------------------------------------------------------------------
ArtifactManifest makeLifecycleManifest(const std::string& id = "art-lsm-01") {
    ArtifactManifest m;
    m.artifact_id          = id;
    m.tensor_name          = "users/embedding";
    m.kind                 = ArtifactKind::ADVISORY_SUMMARY;
    m.artifact_class       = ArtifactClass::DERIVED;
    m.truth_semantic       = TruthSemantic::ADVISORY_ONLY;
    m.lifecycle_state      = LifecycleState::READY;
    m.rebuild_state        = RebuildState::PRISTINE;
    m.update_mode          = UpdateMode::REBUILD;
    m.invalidation_reason  = InvalidationReason::UNKNOWN;
    m.version              = 1;
    m.source_seq_start     = 100;
    m.source_seq_end       = 500;
    m.delta_lag            = 12;
    m.residual             = 0.02;
    m.rank_cap             = 128;
    m.rank_status          = 64;
    m.staleness_threshold_sec = 300;
    m.last_verified_unix_sec  = 1000;
    m.created_at_unix_sec     = 900;
    m.updated_at_unix_sec     = 1000;
    m.replication_factor      = 1;
    m.advisory_only           = true;
    return m;
}

// ===========================================================================
// LSM-01..03: Default state invariants
// ===========================================================================

TEST(LifecycleStalenessManagement, LSM01_DefaultLifecycleStateIsReady) {
    ArtifactManifest m;
    EXPECT_EQ(m.lifecycle_state, LifecycleState::READY);
    // ACTIVE is a compile-time alias for READY (same underlying value)
    EXPECT_EQ(static_cast<int>(m.lifecycle_state),
              static_cast<int>(LifecycleState::ACTIVE));
}

TEST(LifecycleStalenessManagement, LSM02_DefaultTruthSemanticIsAdvisoryOnly) {
    ArtifactManifest m;
    EXPECT_EQ(m.truth_semantic, TruthSemantic::ADVISORY_ONLY);
}

TEST(LifecycleStalenessManagement, LSM03_DefaultArtifactClassIsDerived) {
    ArtifactManifest m;
    EXPECT_EQ(m.artifact_class, ArtifactClass::DERIVED);
}

// ===========================================================================
// LSM-04..07: ArtifactClassifier::isValidCombination
// ===========================================================================

TEST(LifecycleStalenessManagement, LSM04_DerivedAdvisoryOnlyIsValid) {
    EXPECT_TRUE(ArtifactClassifier::isValidCombination(
        ArtifactClass::DERIVED, TruthSemantic::ADVISORY_ONLY));
}

TEST(LifecycleStalenessManagement, LSM05_SourceOfTruthGroundTruthIsValid) {
    EXPECT_TRUE(ArtifactClassifier::isValidCombination(
        ArtifactClass::SOURCE_OF_TRUTH, TruthSemantic::GROUND_TRUTH));
}

TEST(LifecycleStalenessManagement, LSM06_DerivedGroundTruthIsInvalid) {
    EXPECT_FALSE(ArtifactClassifier::isValidCombination(
        ArtifactClass::DERIVED, TruthSemantic::GROUND_TRUTH));
}

TEST(LifecycleStalenessManagement, LSM07_SourceOfTruthAdvisoryOnlyIsInvalid) {
    EXPECT_FALSE(ArtifactClassifier::isValidCombination(
        ArtifactClass::SOURCE_OF_TRUTH, TruthSemantic::ADVISORY_ONLY));
}

// ===========================================================================
// LSM-08: ArtifactLifecyclePolicy::isUsableForPlanning
// ===========================================================================

TEST(LifecycleStalenessManagement, LSM08_UsableStatesAreReadyAndStale) {
    EXPECT_TRUE(ArtifactLifecyclePolicy::isUsableForPlanning(LifecycleState::READY));
    EXPECT_TRUE(ArtifactLifecyclePolicy::isUsableForPlanning(LifecycleState::STALE));
    EXPECT_FALSE(ArtifactLifecyclePolicy::isUsableForPlanning(LifecycleState::INVALIDATED));
    EXPECT_FALSE(ArtifactLifecyclePolicy::isUsableForPlanning(LifecycleState::REBUILDING));
    EXPECT_FALSE(ArtifactLifecyclePolicy::isUsableForPlanning(LifecycleState::FAILED));
}

// ===========================================================================
// LSM-09..10: ArtifactManifest::isUsable
// ===========================================================================

TEST(LifecycleStalenessManagement, LSM09_IsUsableTrueForReadyAndStale) {
    ArtifactManifest m = makeLifecycleManifest();
    m.lifecycle_state = LifecycleState::READY;
    EXPECT_TRUE(m.isUsable(1000));

    m.lifecycle_state = LifecycleState::STALE;
    EXPECT_TRUE(m.isUsable(1000));
}

TEST(LifecycleStalenessManagement, LSM10_IsUsableFalseForNonUsableStates) {
    ArtifactManifest m = makeLifecycleManifest();

    m.lifecycle_state = LifecycleState::INVALIDATED;
    EXPECT_FALSE(m.isUsable(1000));

    m.lifecycle_state = LifecycleState::REBUILDING;
    EXPECT_FALSE(m.isUsable(1000));

    m.lifecycle_state = LifecycleState::FAILED;
    EXPECT_FALSE(m.isUsable(1000));
}

// ===========================================================================
// LSM-11..14: ArtifactManifest::isStale
// ===========================================================================

TEST(LifecycleStalenessManagement, LSM11_IsStaleReturnsFalseWhenNoThreshold) {
    ArtifactManifest m = makeLifecycleManifest();
    m.staleness_threshold_sec  = 0;
    m.last_verified_unix_sec   = 0;
    EXPECT_FALSE(m.isStale(1000000));  // Any time → not stale without threshold
}

TEST(LifecycleStalenessManagement, LSM12_IsStaleReturnsTrueWhenAgeExceedsThreshold) {
    ArtifactManifest m = makeLifecycleManifest();
    m.staleness_threshold_sec = 300;
    m.last_verified_unix_sec  = 1000;
    // now = 1000 + 301 → age 301 > 300 → stale
    EXPECT_TRUE(m.isStale(1301));
}

TEST(LifecycleStalenessManagement, LSM13_IsStaleReturnsFalseWhenAgeWithinThreshold) {
    ArtifactManifest m = makeLifecycleManifest();
    m.staleness_threshold_sec = 300;
    m.last_verified_unix_sec  = 1000;
    // now = 1000 + 299 → age 299 < 300 → not stale
    EXPECT_FALSE(m.isStale(1299));
}

TEST(LifecycleStalenessManagement, LSM14_IsStaleReturnsTrueWhenNeverVerifiedWithThreshold) {
    ArtifactManifest m = makeLifecycleManifest();
    m.staleness_threshold_sec = 300;
    m.last_verified_unix_sec  = 0;  // Never verified
    EXPECT_TRUE(m.isStale(1000));
}

// ===========================================================================
// LSM-15..17: ArtifactManifest::getFreshnessScore
// ===========================================================================

TEST(LifecycleStalenessManagement, LSM15_FreshnessScore1WhenAgeZero) {
    ArtifactManifest m = makeLifecycleManifest();
    m.staleness_threshold_sec = 300;
    m.last_verified_unix_sec  = 1000;
    EXPECT_DOUBLE_EQ(m.getFreshnessScore(1000), 1.0);
}

TEST(LifecycleStalenessManagement, LSM16_FreshnessScore0WhenAgeAtOrBeyondThreshold) {
    ArtifactManifest m = makeLifecycleManifest();
    m.staleness_threshold_sec = 300;
    m.last_verified_unix_sec  = 1000;
    EXPECT_DOUBLE_EQ(m.getFreshnessScore(1300), 0.0);  // age == threshold
    EXPECT_DOUBLE_EQ(m.getFreshnessScore(2000), 0.0);  // age >> threshold (clamped)
}

TEST(LifecycleStalenessManagement, LSM17_FreshnessScoreHalfWhenAgeIsHalfThreshold) {
    ArtifactManifest m = makeLifecycleManifest();
    m.staleness_threshold_sec = 300;
    m.last_verified_unix_sec  = 1000;
    // age = 150 = threshold/2 → score = 0.5
    EXPECT_DOUBLE_EQ(m.getFreshnessScore(1150), 0.5);
}

// ===========================================================================
// LSM-18..22: ArtifactManifest::validate
// ===========================================================================

TEST(LifecycleStalenessManagement, LSM18_ValidateRejectsInvalidSeqRange) {
    ArtifactManifest m = makeLifecycleManifest();
    m.source_seq_start = 500;
    m.source_seq_end   = 100;  // end < start → invalid
    EXPECT_FALSE(m.validate());
}

TEST(LifecycleStalenessManagement, LSM19_ValidateRejectsResidualOutOfRange) {
    ArtifactManifest m = makeLifecycleManifest();
    m.residual = -0.1;  // negative residual
    EXPECT_FALSE(m.validate());

    m.residual = 1001.0;  // > 1000 upper bound
    EXPECT_FALSE(m.validate());
}

TEST(LifecycleStalenessManagement, LSM20_ValidateRejectsRankStatusExceedingCap) {
    ArtifactManifest m = makeLifecycleManifest();
    m.rank_cap    = 64;
    m.rank_status = 128;  // exceeds cap
    EXPECT_FALSE(m.validate());
}

TEST(LifecycleStalenessManagement, LSM21_ValidateRejectsInvalidClassSemanticCombination) {
    ArtifactManifest m = makeLifecycleManifest();
    m.artifact_class = ArtifactClass::SOURCE_OF_TRUTH;
    m.truth_semantic = TruthSemantic::ADVISORY_ONLY;  // invalid: SOT must be GROUND_TRUTH
    EXPECT_FALSE(m.validate());
}

TEST(LifecycleStalenessManagement, LSM22_ValidateAcceptsWellFormedManifest) {
    ArtifactManifest m = makeLifecycleManifest();
    EXPECT_TRUE(m.validate());
}

// ===========================================================================
// LSM-23..26: Enum string round-trips
// ===========================================================================

TEST(LifecycleStalenessManagement, LSM23_InvalidationReasonStringRoundTrip) {
    const auto check = [](InvalidationReason r) {
        auto s   = InvalidationReasonUtils::reasonToString(r);
        auto opt = InvalidationReasonUtils::stringToReason(s);
        ASSERT_TRUE(opt.has_value()) << "Failed for: " << s;
        EXPECT_EQ(*opt, r);
    };
    check(InvalidationReason::UNKNOWN);
    check(InvalidationReason::INTEGRITY_CHECK_FAILED);
    check(InvalidationReason::STALENESS_EXCEEDED);
    check(InvalidationReason::SOURCE_INVALIDATED);
    check(InvalidationReason::SOURCE_LINEAGE_CORRUPTED);
    check(InvalidationReason::POLICY_VIOLATION);
    check(InvalidationReason::ADMIN_REQUESTED);
    check(InvalidationReason::SHARD_UNAVAILABLE);
}

TEST(LifecycleStalenessManagement, LSM24_RebuildStateStringRoundTrip) {
    const auto check = [](RebuildState rs) {
        auto s   = RebuildStateUtils::stateToString(rs);
        auto opt = RebuildStateUtils::stringToState(s);
        ASSERT_TRUE(opt.has_value()) << "Failed for: " << s;
        EXPECT_EQ(*opt, rs);
    };
    check(RebuildState::PRISTINE);
    check(RebuildState::PATCHED);
    check(RebuildState::PARTIAL_REFITTED);
    check(RebuildState::REBUILT);
}

TEST(LifecycleStalenessManagement, LSM25_UpdateModeStringRoundTrip) {
    const auto check = [](UpdateMode um) {
        auto s   = UpdateModeUtils::modeToString(um);
        auto opt = UpdateModeUtils::stringToMode(s);
        ASSERT_TRUE(opt.has_value()) << "Failed for: " << s;
        EXPECT_EQ(*opt, um);
    };
    check(UpdateMode::PATCH);
    check(UpdateMode::PARTIAL_REFIT);
    check(UpdateMode::REBUILD);
}

TEST(LifecycleStalenessManagement, LSM26_LifecycleStateStringRoundTrip) {
    const auto check = [](LifecycleState ls, const std::string& expected_str) {
        auto s   = ArtifactLifecyclePolicy::stateToString(ls);
        EXPECT_EQ(s, expected_str) << "Wrong string for state " << expected_str;
        auto opt = ArtifactLifecyclePolicy::stringToState(s);
        ASSERT_TRUE(opt.has_value()) << "Failed for: " << s;
        EXPECT_EQ(*opt, ls);
    };
    check(LifecycleState::READY,       "READY");
    check(LifecycleState::STALE,       "STALE");
    check(LifecycleState::INVALIDATED, "INVALIDATED");
    check(LifecycleState::REBUILDING,  "REBUILDING");
    check(LifecycleState::FAILED,      "FAILED");
}

// ===========================================================================
// LSM-27: JSON round-trip preserves lifecycle fields
// ===========================================================================

TEST(LifecycleStalenessManagement, LSM27_JsonRoundTripPreservesLifecycleFields) {
    ArtifactManifest m = makeLifecycleManifest("art-json-roundtrip");
    m.lifecycle_state      = LifecycleState::STALE;
    m.rebuild_state        = RebuildState::PATCHED;
    m.update_mode          = UpdateMode::PATCH;
    m.invalidation_reason  = InvalidationReason::STALENESS_EXCEEDED;
    m.source_seq_start     = 200;
    m.source_seq_end       = 800;
    m.delta_lag            = 42;
    m.residual             = 0.15;
    m.rank_cap             = 256;
    m.rank_status          = 192;
    m.artifact_age_ms      = 120000;

    const std::string json_str = m.toJSON();
    ASSERT_FALSE(json_str.empty());

    const auto opt = ArtifactManifest::fromJSON(json_str);
    ASSERT_TRUE(opt.has_value()) << "fromJSON failed";
    const auto& r = *opt;

    EXPECT_EQ(r.artifact_id,        m.artifact_id);
    EXPECT_EQ(r.lifecycle_state,    LifecycleState::STALE);
    EXPECT_EQ(r.rebuild_state,      RebuildState::PATCHED);
    EXPECT_EQ(r.update_mode,        UpdateMode::PATCH);
    EXPECT_EQ(r.invalidation_reason, InvalidationReason::STALENESS_EXCEEDED);
    EXPECT_EQ(r.source_seq_start,   200u);
    EXPECT_EQ(r.source_seq_end,     800u);
    EXPECT_EQ(r.delta_lag,          42u);
    EXPECT_DOUBLE_EQ(r.residual,    0.15);
    EXPECT_EQ(r.rank_cap,           256u);
    EXPECT_EQ(r.rank_status,        192u);
    EXPECT_EQ(r.artifact_age_ms,    120000u);
}

TEST(LifecycleStalenessManagement, LSM27b_YamlRoundTripPreservesLifecycleFields) {
    ArtifactManifest m = makeLifecycleManifest("art-yaml-roundtrip");
    m.lifecycle_state      = LifecycleState::STALE;
    m.rebuild_state        = RebuildState::PARTIAL_REFITTED;
    m.update_mode          = UpdateMode::PARTIAL_REFIT;
    m.invalidation_reason  = InvalidationReason::POLICY_VIOLATION;
    m.source_seq_start     = 111;
    m.source_seq_end       = 777;
    m.delta_lag            = 31;
    m.residual             = 0.25;
    m.rank_cap             = 300;
    m.rank_status          = 299;
    m.artifact_age_ms      = 9000;

    const std::string yaml_str = m.toYAML();
    ASSERT_FALSE(yaml_str.empty());

    const auto opt = ArtifactManifest::fromYAML(yaml_str);
    ASSERT_TRUE(opt.has_value()) << "fromYAML failed";
    const auto& r = *opt;

    EXPECT_EQ(r.artifact_id,         m.artifact_id);
    EXPECT_EQ(r.version,             m.version);
    EXPECT_EQ(r.lifecycle_state,     LifecycleState::STALE);
    EXPECT_EQ(r.rebuild_state,       RebuildState::PARTIAL_REFITTED);
    EXPECT_EQ(r.update_mode,         UpdateMode::PARTIAL_REFIT);
    EXPECT_EQ(r.invalidation_reason, InvalidationReason::POLICY_VIOLATION);
    EXPECT_EQ(r.source_seq_start,    111u);
    EXPECT_EQ(r.source_seq_end,      777u);
    EXPECT_EQ(r.delta_lag,           31u);
    EXPECT_DOUBLE_EQ(r.residual,     0.25);
    EXPECT_EQ(r.rank_cap,            300u);
    EXPECT_EQ(r.rank_status,         299u);
    EXPECT_EQ(r.artifact_age_ms,     9000u);
}

// ===========================================================================
// LSM-28..31: ArtifactInvalidationManager
// ===========================================================================

TEST(LifecycleStalenessManagement, LSM28_InvalidationManagerMarksStaleCorrectly) {
    ArtifactManifest m = makeLifecycleManifest();
    m.lifecycle_state = LifecycleState::READY;

    ArtifactInvalidationManager mgr;
    ArtifactManifest stale = mgr.markStale(m);

    EXPECT_EQ(stale.lifecycle_state, LifecycleState::STALE);
    // Original must not be mutated
    EXPECT_EQ(m.lifecycle_state, LifecycleState::READY);
}

TEST(LifecycleStalenessManagement, LSM29_InvalidationManagerMarksInvalidatedWithReason) {
    ArtifactManifest m = makeLifecycleManifest();
    m.lifecycle_state = LifecycleState::STALE;

    ArtifactInvalidationManager mgr;
    ArtifactManifest inv = mgr.invalidate(m, InvalidationReason::POLICY_VIOLATION);

    EXPECT_EQ(inv.lifecycle_state,    LifecycleState::INVALIDATED);
    EXPECT_EQ(inv.invalidation_reason, InvalidationReason::POLICY_VIOLATION);
    // Original must not be mutated
    EXPECT_EQ(m.lifecycle_state,    LifecycleState::STALE);
    EXPECT_EQ(m.invalidation_reason, InvalidationReason::UNKNOWN);
}

TEST(LifecycleStalenessManagement, LSM30_InvalidationManagerDetectsRankCapBreach) {
    ArtifactManifest m = makeLifecycleManifest();

    ArtifactInvalidationManager mgr;

    // No breach when rank_status <= rank_cap
    m.rank_cap    = 128;
    m.rank_status = 64;
    EXPECT_FALSE(mgr.shouldInvalidateForRankBreach(m));

    // Breach when rank_status > rank_cap
    m.rank_status = 200;
    EXPECT_TRUE(mgr.shouldInvalidateForRankBreach(m));

    // No breach when rank_cap == 0 (no cap configured)
    m.rank_cap    = 0;
    m.rank_status = 999;
    EXPECT_FALSE(mgr.shouldInvalidateForRankBreach(m));
}

TEST(LifecycleStalenessManagement, LSM31_InvalidationManagerDetectsResidualBreach) {
    ArtifactManifest m = makeLifecycleManifest();
    m.residual = 0.5;

    ArtifactInvalidationManager mgr;

    // Below threshold
    EXPECT_FALSE(mgr.shouldInvalidateForResidual(m, 0.8));

    // Above threshold
    EXPECT_TRUE(mgr.shouldInvalidateForResidual(m, 0.3));

    // Exactly at threshold
    EXPECT_FALSE(mgr.shouldInvalidateForResidual(m, 0.5));
}

TEST(LifecycleStalenessManagement, LSM32_InvalidationManagerTransitionsToRebuilding) {
    ArtifactManifest m = makeLifecycleManifest();
    m.lifecycle_state = LifecycleState::STALE;
    m.update_mode = UpdateMode::PATCH;
    m.updated_at_unix_sec = 1000;

    ArtifactInvalidationManager mgr;
    const ArtifactManifest rebuilding = mgr.transitionToRebuilding(m, UpdateMode::PARTIAL_REFIT, 2000);

    EXPECT_EQ(rebuilding.lifecycle_state, LifecycleState::REBUILDING);
    EXPECT_EQ(rebuilding.update_mode, UpdateMode::PARTIAL_REFIT);
    EXPECT_EQ(rebuilding.updated_at_unix_sec, 2000);
    EXPECT_EQ(m.lifecycle_state, LifecycleState::STALE);
}

TEST(LifecycleStalenessManagement, LSM33_InvalidationManagerTransitionsToReadyAfterRebuild) {
    ArtifactManifest m = makeLifecycleManifest();
    m.lifecycle_state = LifecycleState::REBUILDING;
    m.delta_lag = 99;
    m.artifact_age_ms = 5000;
    m.invalidation_reason = InvalidationReason::STALENESS_EXCEEDED;
    m.last_rebuild_at_unix_sec = 1200;
    m.last_verified_unix_sec = 1100;
    m.updated_at_unix_sec = 1200;

    ArtifactInvalidationManager mgr;
    const ArtifactManifest ready = mgr.transitionToReadyAfterRebuild(
        m,
        RebuildState::REBUILT,
        501,
        900,
        3000);

    EXPECT_EQ(ready.lifecycle_state, LifecycleState::READY);
    EXPECT_EQ(ready.rebuild_state, RebuildState::REBUILT);
    EXPECT_EQ(ready.source_seq_start, 501u);
    EXPECT_EQ(ready.source_seq_end, 900u);
    EXPECT_EQ(ready.delta_lag, 0u);
    EXPECT_EQ(ready.artifact_age_ms, 0u);
    EXPECT_EQ(ready.invalidation_reason, InvalidationReason::UNKNOWN);
    EXPECT_EQ(ready.last_rebuild_at_unix_sec, 3000);
    EXPECT_EQ(ready.last_verified_unix_sec, 3000);
    EXPECT_EQ(ready.updated_at_unix_sec, 3000);
}

TEST(LifecycleStalenessManagement, LSM34_InvalidationManagerTransitionsToFailed) {
    ArtifactManifest m = makeLifecycleManifest();
    m.lifecycle_state = LifecycleState::REBUILDING;
    m.updated_at_unix_sec = 1000;

    ArtifactInvalidationManager mgr;
    const ArtifactManifest failed = mgr.transitionToFailed(
        m,
        InvalidationReason::SHARD_UNAVAILABLE,
        2500);

    EXPECT_EQ(failed.lifecycle_state, LifecycleState::FAILED);
    EXPECT_EQ(failed.invalidation_reason, InvalidationReason::SHARD_UNAVAILABLE);
    EXPECT_EQ(failed.updated_at_unix_sec, 2500);
}

TEST(LifecycleStalenessManagement, LSM35_InvalidationManagerPlannerGateRejectsCorrectly) {
    ArtifactInvalidationManager mgr;
    ArtifactManifest m = makeLifecycleManifest();

    m.lifecycle_state = LifecycleState::READY;
    m.last_verified_unix_sec = 1000;
    m.staleness_threshold_sec = 300;
    m.delta_lag = 10;
    m.residual = 0.1;
    EXPECT_FALSE(mgr.shouldRejectForPlanner(m, 1200, 100, 0.5));

    m.lifecycle_state = LifecycleState::REBUILDING;
    EXPECT_TRUE(mgr.shouldRejectForPlanner(m, 1200, 100, 0.5));

    m.lifecycle_state = LifecycleState::READY;
    m.last_verified_unix_sec = 500;
    EXPECT_TRUE(mgr.shouldRejectForPlanner(m, 1200, 100, 0.5));

    m.last_verified_unix_sec = 1200;
    m.delta_lag = 200;
    EXPECT_TRUE(mgr.shouldRejectForPlanner(m, 1200, 100, 0.5));

    m.delta_lag = 10;
    m.residual = 0.9;
    EXPECT_TRUE(mgr.shouldRejectForPlanner(m, 1200, 100, 0.5));
}

}  // namespace
} } // namespace themis::distributed_tensor
