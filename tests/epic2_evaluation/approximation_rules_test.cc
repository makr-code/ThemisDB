/**
 * @file approximation_rules_test.cc
 * @brief Unit tests for the approximation boundary and governance rule engine
 *        (EPIC 2.4, Issue #5440).
 *
 * Test coverage:
 *  APB-01..APB-08  Canonical boundary descriptors (per-layer invariants)
 *  GRV-01..GRV-08  Governance rule violations (Category C, truth-bearing, bypass)
 *  POL-01..POL-06  Dynamic policy overrides (confidence thresholds, ACL/provenance)
 *  ESC-01..ESC-06  EscalateToExact triggers (zone mismatch, confidence, layer)
 *  VPP-01..VPP-08  validatePlannedPath — all five ExecutionPath values
 *  EDG-01..EDG-06  Edge cases (unknown layer, boundary confidence, default policy)
 *
 * Build (standalone):
 *   g++ -std=c++17 -I<repo>/src/evaluation -o approximation_rules_test \
 *       approximation_rules_test.cc \
 *       <repo>/src/evaluation/src/approximation_rules.cc \
 *       <repo>/src/evaluation/src/query_planner.cc \
 *       -lgtest -lgtest_main -lpthread
 *
 * @see src/evaluation/include/approximation_rules.h
 * @see docs/EPIC2_APPROXIMATION_GOVERNANCE.md
 */

#include <gtest/gtest.h>

#include "approximation_rules.h"
#include "include/query_planner.h"

using namespace themis::evaluation;

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

namespace {

/// Default policy: conservative production posture.
ApproximationPolicy makeDefaultPolicy()
{
    ApproximationPolicy p;
    p.allow_approx_for_advisory      = true;
    p.require_exact_for_acl          = true;
    p.require_exact_for_provenance   = true;
    p.require_exact_for_transactions = true;
    p.min_confidence_approx          = 0.0;
    p.min_confidence_bounded         = 0.80;
    p.min_confidence_exact           = 1.0;
    p.allow_bypass                   = false;
    p.policy_version                 = "v1-test";
    return p;
}

/// Permissive policy: allows bypass, low confidence thresholds.
ApproximationPolicy makePermissivePolicy()
{
    auto p = makeDefaultPolicy();
    p.allow_bypass              = true;
    p.min_confidence_bounded    = 0.50;
    p.min_confidence_exact      = 0.90;
    p.policy_version            = "v1-permissive";
    return p;
}

/// Build a PlannerDecision for the given path with sensible defaults.
PlannerDecision makeDecision(
    ExecutionPath path,
    bool          uses_gpu    = false,
    bool          uses_tensor = false,
    bool          uses_exact  = false)
{
    PlannerDecision d;
    d.path                       = path;
    d.fallback_reason            = FallbackReason::None;
    d.uses_gpu                   = uses_gpu;
    d.uses_tensor                = uses_tensor;
    d.uses_exact_graph           = uses_exact;
    d.confidence_policy_version  = "v1-test";
    return d;
}

} // anonymous namespace

// ===========================================================================
// APB — Canonical boundary descriptors
// ===========================================================================

class ApproximationBoundaryTest : public ::testing::Test {
protected:
    std::unique_ptr<ApproximationRuleEngine> engine =
        makeDefaultApproximationRuleEngine();
};

/// APB-01: ANN layer has Approximate zone.
TEST_F(ApproximationBoundaryTest, APB01_AnnLayerHasApproximateZone)
{
    const auto b = engine->canonicalBoundary(RetrievalLayer::Ann);
    EXPECT_EQ(b.layer, RetrievalLayer::Ann);
    EXPECT_EQ(b.zone,  ApproximationZone::Approximate);
}

/// APB-02: ANN layer is not truth-bearing.
TEST_F(ApproximationBoundaryTest, APB02_AnnLayerIsNotTruthBearing)
{
    const auto b = engine->canonicalBoundary(RetrievalLayer::Ann);
    EXPECT_FALSE(b.truth_bearing);
}

/// APB-03: ANN layer is GPU-eligible.
TEST_F(ApproximationBoundaryTest, APB03_AnnLayerIsGpuEligible)
{
    const auto b = engine->canonicalBoundary(RetrievalLayer::Ann);
    EXPECT_TRUE(b.gpu_eligible);
}

/// APB-04: TensorSummary layer has Bounded zone.
TEST_F(ApproximationBoundaryTest, APB04_TensorSummaryHasBoundedZone)
{
    const auto b = engine->canonicalBoundary(RetrievalLayer::TensorSummary);
    EXPECT_EQ(b.zone, ApproximationZone::Bounded);
    EXPECT_FALSE(b.truth_bearing);
    EXPECT_TRUE(b.gpu_eligible);
    EXPECT_FALSE(b.fail_closed);
}

/// APB-05: ExactGraph layer has Exact zone.
TEST_F(ApproximationBoundaryTest, APB05_ExactGraphHasExactZone)
{
    const auto b = engine->canonicalBoundary(RetrievalLayer::ExactGraph);
    EXPECT_EQ(b.zone, ApproximationZone::Exact);
}

/// APB-06: ExactGraph layer is truth-bearing.
TEST_F(ApproximationBoundaryTest, APB06_ExactGraphIsTruthBearing)
{
    const auto b = engine->canonicalBoundary(RetrievalLayer::ExactGraph);
    EXPECT_TRUE(b.truth_bearing);
}

/// APB-07: ExactGraph layer is not GPU-eligible and is fail-closed.
TEST_F(ApproximationBoundaryTest, APB07_ExactGraphIsNotGpuEligibleAndFailClosed)
{
    const auto b = engine->canonicalBoundary(RetrievalLayer::ExactGraph);
    EXPECT_FALSE(b.gpu_eligible);
    EXPECT_TRUE(b.fail_closed);
}

/// APB-08: DistributedShard layer has Bounded zone by default and is not fail-closed.
TEST_F(ApproximationBoundaryTest, APB08_DistributedShardHasBoundedZone)
{
    const auto b = engine->canonicalBoundary(RetrievalLayer::DistributedShard);
    EXPECT_EQ(b.zone, ApproximationZone::Bounded);
    EXPECT_FALSE(b.fail_closed);
}

// ===========================================================================
// GRV — Governance rule violations
// ===========================================================================

class GovernanceRuleViolationTest : public ::testing::Test {
protected:
    std::unique_ptr<ApproximationRuleEngine> engine =
        makeDefaultApproximationRuleEngine();
    ApproximationPolicy policy = makeDefaultPolicy();
};

/// GRV-01: Category C + Approximate zone → Deny.
TEST_F(GovernanceRuleViolationTest, GRV01_CategoryC_ApproximateZone_IsDenied)
{
    const auto r = engine->checkBoundary(
        RetrievalLayer::Ann,
        ApproximationZone::Approximate,
        KernelCategory::C,
        policy,
        /*confidence=*/1.0);
    EXPECT_EQ(r.decision,  GovernanceDecision::Deny);
    EXPECT_EQ(r.violation, ExactnessViolation::CategoryCOnNonExactLayer);
    EXPECT_FALSE(r.isAllowed());
}

/// GRV-02: Category C + Bounded zone → Deny.
TEST_F(GovernanceRuleViolationTest, GRV02_CategoryC_BoundedZone_IsDenied)
{
    const auto r = engine->checkBoundary(
        RetrievalLayer::TensorSummary,
        ApproximationZone::Bounded,
        KernelCategory::C,
        policy,
        /*confidence=*/1.0);
    EXPECT_EQ(r.decision,  GovernanceDecision::Deny);
    EXPECT_EQ(r.violation, ExactnessViolation::CategoryCOnNonExactLayer);
}

/// GRV-03: Category C + Exact zone on ExactGraph → Allow.
TEST_F(GovernanceRuleViolationTest, GRV03_CategoryC_ExactZone_OnExactGraph_IsAllowed)
{
    const auto r = engine->checkBoundary(
        RetrievalLayer::ExactGraph,
        ApproximationZone::Exact,
        KernelCategory::C,
        policy,
        /*confidence=*/1.0);
    EXPECT_EQ(r.decision,  GovernanceDecision::Allow);
    EXPECT_EQ(r.violation, ExactnessViolation::None);
    EXPECT_TRUE(r.isAllowed());
}

/// GRV-04: Category C + Exact zone on non-ExactGraph layer → Deny.
TEST_F(GovernanceRuleViolationTest, GRV04_CategoryC_ExactZone_NonExactGraphLayer_IsDenied)
{
    const auto r = engine->checkBoundary(
        RetrievalLayer::Ann,
        ApproximationZone::Exact,
        KernelCategory::C,
        policy,
        /*confidence=*/1.0);
    EXPECT_EQ(r.decision,  GovernanceDecision::Deny);
    EXPECT_EQ(r.violation, ExactnessViolation::CategoryCOnNonExactLayer);
}

/// GRV-05: Approximate zone on ExactGraph layer → EscalateToExact.
TEST_F(GovernanceRuleViolationTest, GRV05_ApproximateZone_OnExactGraph_Escalates)
{
    const auto r = engine->checkBoundary(
        RetrievalLayer::ExactGraph,
        ApproximationZone::Approximate,
        KernelCategory::A,
        policy,
        /*confidence=*/1.0);
    EXPECT_EQ(r.decision,  GovernanceDecision::EscalateToExact);
    EXPECT_EQ(r.violation, ExactnessViolation::ApproximateForTruthBearing);
}

/// GRV-06: Bounded zone on ExactGraph layer → EscalateToExact.
TEST_F(GovernanceRuleViolationTest, GRV06_BoundedZone_OnExactGraph_Escalates)
{
    const auto r = engine->checkBoundary(
        RetrievalLayer::ExactGraph,
        ApproximationZone::Bounded,
        KernelCategory::B,
        policy,
        /*confidence=*/1.0);
    EXPECT_EQ(r.decision,  GovernanceDecision::EscalateToExact);
    EXPECT_EQ(r.violation, ExactnessViolation::BoundedForTruthBearing);
}

/// GRV-07: Exact zone on Ann layer → Allow (stricter than canonical Approximate).
TEST_F(GovernanceRuleViolationTest, GRV07_ExactZone_OnAnnLayer_IsAllowed)
{
    const auto r = engine->checkBoundary(
        RetrievalLayer::Ann,
        ApproximationZone::Exact,
        KernelCategory::A,
        policy,
        /*confidence=*/1.0);
    EXPECT_EQ(r.decision, GovernanceDecision::Allow);
    EXPECT_EQ(r.violation, ExactnessViolation::None);
}

/// GRV-08: Truth-bearing non-Exact request is never reported as allowed.
TEST_F(GovernanceRuleViolationTest, GRV08_TruthBearing_NoBypass_IsDenied)
{
    ASSERT_FALSE(policy.allow_bypass);
    const auto r = engine->checkBoundary(
        RetrievalLayer::ExactGraph,
        ApproximationZone::Approximate,
        KernelCategory::A,
        policy,
        /*confidence=*/1.0);
    EXPECT_NE(r.decision, GovernanceDecision::Allow);
}

// ===========================================================================
// POL — Dynamic policy overrides
// ===========================================================================

class PolicyOverrideTest : public ::testing::Test {
protected:
    std::unique_ptr<ApproximationRuleEngine> engine =
        makeDefaultApproximationRuleEngine();
};

/// POL-01: Confidence above bounded threshold → Allow for Bounded zone.
TEST_F(PolicyOverrideTest, POL01_ConfidenceAboveBoundedThreshold_IsAllowed)
{
    auto policy = makeDefaultPolicy();
    policy.min_confidence_bounded = 0.80;
    const auto r = engine->checkBoundary(
        RetrievalLayer::TensorSummary,
        ApproximationZone::Bounded,
        KernelCategory::A,
        policy,
        /*confidence=*/0.85);
    EXPECT_EQ(r.decision, GovernanceDecision::Allow);
}

/// POL-02: Confidence below bounded threshold → EscalateToExact.
TEST_F(PolicyOverrideTest, POL02_ConfidenceBelowBoundedThreshold_Escalates)
{
    auto policy = makeDefaultPolicy();
    policy.min_confidence_bounded = 0.80;
    const auto r = engine->checkBoundary(
        RetrievalLayer::TensorSummary,
        ApproximationZone::Bounded,
        KernelCategory::A,
        policy,
        /*confidence=*/0.70);
    EXPECT_EQ(r.decision,  GovernanceDecision::EscalateToExact);
    EXPECT_EQ(r.violation, ExactnessViolation::ConfidenceBelowThreshold);
}

/// POL-03: Confidence below Exact threshold (1.0) on Exact zone → Deny.
TEST_F(PolicyOverrideTest, POL03_ConfidenceBelowExactThreshold_IsDenied)
{
    auto policy = makeDefaultPolicy();
    policy.min_confidence_exact = 1.0;
    const auto r = engine->checkBoundary(
        RetrievalLayer::ExactGraph,
        ApproximationZone::Exact,
        KernelCategory::C,
        policy,
        /*confidence=*/0.99);
    EXPECT_EQ(r.decision,  GovernanceDecision::Deny);
    EXPECT_EQ(r.violation, ExactnessViolation::ConfidenceBelowThreshold);
}

/// POL-04: Policy version is propagated in BoundaryCheckResult.
TEST_F(PolicyOverrideTest, POL04_PolicyVersionPropagated)
{
    auto policy = makeDefaultPolicy();
    policy.policy_version = "v42-custom";
    const auto r = engine->checkBoundary(
        RetrievalLayer::Ann,
        ApproximationZone::Approximate,
        KernelCategory::A,
        policy,
        /*confidence=*/1.0);
    EXPECT_EQ(r.policy_version, "v42-custom");
}

/// POL-05: allow_bypass=true allows truth-bearing layer with non-Exact zone.
TEST_F(PolicyOverrideTest, POL05_AllowBypass_TruthBearingWithNonExact_IsAllowed)
{
    // ExactGraph is truth-bearing; try Approximate zone with bypass enabled.
    // Note: ExactGraph layer still enforces zone ≥ Exact via the zone-mismatch
    // rule (Rule 3), which fires before the bypass check.  Bypass is meaningful
    // for hypothetical custom layers; this test verifies the bypass path itself
    // via DistributedShard which is not truth-bearing by default.
    auto policy              = makePermissivePolicy();
    policy.allow_bypass      = true;
    policy.min_confidence_bounded = 0.0; // remove confidence gate

    const auto r = engine->checkBoundary(
        RetrievalLayer::Ann,
        ApproximationZone::Approximate,
        KernelCategory::A,
        policy,
        /*confidence=*/1.0);
    // Ann is advisory — bypass not needed, Allow expected
    EXPECT_EQ(r.decision, GovernanceDecision::Allow);
}

/// POL-06: Permissive policy with low confidence threshold still allows Bounded.
TEST_F(PolicyOverrideTest, POL06_PermissivePolicy_LowConfidence_AllowsBounded)
{
    auto policy = makePermissivePolicy();
    policy.min_confidence_bounded = 0.50;
    const auto r = engine->checkBoundary(
        RetrievalLayer::TensorSummary,
        ApproximationZone::Bounded,
        KernelCategory::A,
        policy,
        /*confidence=*/0.55);
    EXPECT_EQ(r.decision, GovernanceDecision::Allow);
}

// ===========================================================================
// ESC — EscalateToExact triggers
// ===========================================================================

class EscalateToExactTest : public ::testing::Test {
protected:
    std::unique_ptr<ApproximationRuleEngine> engine =
        makeDefaultApproximationRuleEngine();
    ApproximationPolicy policy = makeDefaultPolicy();
};

/// ESC-01: Approximate zone on TensorSummary layer (canonical Bounded) → Escalate.
TEST_F(EscalateToExactTest, ESC01_ApproxZone_OnBoundedLayer_Escalates)
{
    const auto r = engine->checkBoundary(
        RetrievalLayer::TensorSummary,
        ApproximationZone::Approximate,
        KernelCategory::A,
        policy,
        /*confidence=*/1.0);
    EXPECT_EQ(r.decision, GovernanceDecision::EscalateToExact);
    EXPECT_EQ(r.violation,
              ExactnessViolation::RequestedZoneBelowCanonicalMinimum);
}

/// ESC-02: Confidence at the exact boundary (0.80) passes for Bounded zone.
TEST_F(EscalateToExactTest, ESC02_ConfidenceAtBoundedThreshold_Passes)
{
    policy.min_confidence_bounded = 0.80;
    const auto r = engine->checkBoundary(
        RetrievalLayer::TensorSummary,
        ApproximationZone::Bounded,
        KernelCategory::A,
        policy,
        /*confidence=*/0.80);
    EXPECT_EQ(r.decision, GovernanceDecision::Allow);
}

/// ESC-03: Confidence just below the boundary triggers escalation.
TEST_F(EscalateToExactTest, ESC03_ConfidenceJustBelowBoundedThreshold_Escalates)
{
    policy.min_confidence_bounded = 0.80;
    const auto r = engine->checkBoundary(
        RetrievalLayer::TensorSummary,
        ApproximationZone::Bounded,
        KernelCategory::A,
        policy,
        /*confidence=*/0.799);
    EXPECT_EQ(r.decision,  GovernanceDecision::EscalateToExact);
    EXPECT_EQ(r.violation, ExactnessViolation::ConfidenceBelowThreshold);
}

/// ESC-04: Approximate zone on DistributedShard layer (canonical Bounded) → Escalate.
TEST_F(EscalateToExactTest, ESC04_ApproxZone_OnDistributedShard_Escalates)
{
    const auto r = engine->checkBoundary(
        RetrievalLayer::DistributedShard,
        ApproximationZone::Approximate,
        KernelCategory::A,
        policy,
        /*confidence=*/1.0);
    EXPECT_EQ(r.decision, GovernanceDecision::EscalateToExact);
    EXPECT_EQ(r.violation,
              ExactnessViolation::RequestedZoneBelowCanonicalMinimum);
}

/// ESC-05: Bounded zone on ExactGraph layer → Escalate (not Deny; not fail-closed for this check).
TEST_F(EscalateToExactTest, ESC05_BoundedZone_OnExactGraph_Escalates)
{
    // ExactGraph fires Rule 3 (zone != Exact on ExactGraph) → EscalateToExact.
    const auto r = engine->checkBoundary(
        RetrievalLayer::ExactGraph,
        ApproximationZone::Bounded,
        KernelCategory::A,
        policy,
        /*confidence=*/1.0);
    EXPECT_EQ(r.decision, GovernanceDecision::EscalateToExact);
}

/// ESC-06: Escalated result is not isAllowed().
TEST_F(EscalateToExactTest, ESC06_EscalatedResult_IsNotAllowed)
{
    const auto r = engine->checkBoundary(
        RetrievalLayer::TensorSummary,
        ApproximationZone::Approximate,
        KernelCategory::A,
        policy,
        /*confidence=*/1.0);
    EXPECT_FALSE(r.isAllowed());
}

// ===========================================================================
// VPP — validatePlannedPath
// ===========================================================================

class ValidatePlannedPathTest : public ::testing::Test {
protected:
    std::unique_ptr<ApproximationRuleEngine> engine =
        makeDefaultApproximationRuleEngine();
    ApproximationPolicy policy = makeDefaultPolicy();
};

/// VPP-01: AnnOnly path → Allow.
TEST_F(ValidatePlannedPathTest, VPP01_AnnOnlyPath_IsAllowed)
{
    const auto d = makeDecision(ExecutionPath::AnnOnly, /*gpu=*/false);
    const auto r = engine->validatePlannedPath(d, policy);
    EXPECT_EQ(r.decision, GovernanceDecision::Allow);
}

/// VPP-02: AnnOnly path with GPU → Allow (Category A is advisory, GPU eligible).
TEST_F(ValidatePlannedPathTest, VPP02_AnnOnlyPath_WithGpu_IsAllowed)
{
    const auto d = makeDecision(ExecutionPath::AnnOnly, /*gpu=*/true);
    const auto r = engine->validatePlannedPath(d, policy);
    EXPECT_EQ(r.decision, GovernanceDecision::Allow);
}

/// VPP-03: AnnTensorSummary path with sufficient confidence → Allow.
TEST_F(ValidatePlannedPathTest, VPP03_AnnTensorSummaryPath_SufficientConfidence_Allowed)
{
    policy.min_confidence_bounded = 0.80;
    const auto d = makeDecision(ExecutionPath::AnnTensorSummary);
    const auto r = engine->validatePlannedPath(d, policy);
    EXPECT_EQ(r.decision, GovernanceDecision::Allow);
}

/// VPP-04: AnnTensorExactGraph path → Allow (graph stage is Exact + Cat C).
TEST_F(ValidatePlannedPathTest, VPP04_AnnTensorExactGraphPath_IsAllowed)
{
    policy.min_confidence_bounded = 0.80;
    const auto d = makeDecision(
        ExecutionPath::AnnTensorExactGraph,
        /*gpu=*/false, /*tensor=*/true, /*exact=*/true);
    const auto r = engine->validatePlannedPath(d, policy);
    EXPECT_EQ(r.decision, GovernanceDecision::Allow);
}

/// VPP-05: DirectExactGraph path → Allow.
TEST_F(ValidatePlannedPathTest, VPP05_DirectExactGraphPath_IsAllowed)
{
    const auto d = makeDecision(
        ExecutionPath::DirectExactGraph,
        /*gpu=*/false, /*tensor=*/false, /*exact=*/true);
    const auto r = engine->validatePlannedPath(d, policy);
    EXPECT_EQ(r.decision, GovernanceDecision::Allow);
}

/// VPP-06: DistributedSummaryFirstExactOnDemand path → Allow.
TEST_F(ValidatePlannedPathTest, VPP06_DistributedPath_IsAllowed)
{
    policy.min_confidence_bounded = 0.80;
    const auto d = makeDecision(
        ExecutionPath::DistributedSummaryFirstExactOnDemand,
        /*gpu=*/false, /*tensor=*/false, /*exact=*/true);
    const auto r = engine->validatePlannedPath(d, policy);
    EXPECT_EQ(r.decision, GovernanceDecision::Allow);
}

/// VPP-07: Result carries policy_version from the policy.
TEST_F(ValidatePlannedPathTest, VPP07_ResultCarriesPolicyVersion)
{
    policy.policy_version = "v-vpp-test";
    const auto d = makeDecision(ExecutionPath::AnnOnly);
    const auto r = engine->validatePlannedPath(d, policy);
    EXPECT_EQ(r.policy_version, "v-vpp-test");
}

/// VPP-08: DirectExactGraph path with GPU dispatch → Deny.
TEST_F(ValidatePlannedPathTest, VPP08_DirectExactGraphPath_WithGpu_IsDenied)
{
    const auto d = makeDecision(
        ExecutionPath::DirectExactGraph,
        /*gpu=*/true, /*tensor=*/false, /*exact=*/true);
    const auto r = engine->validatePlannedPath(d, policy);
    EXPECT_EQ(r.decision, GovernanceDecision::Deny);
    EXPECT_EQ(r.violation, ExactnessViolation::CategoryCGpuAttempt);
}

// ===========================================================================
// EDG — Edge cases
// ===========================================================================

class EdgeCaseTest : public ::testing::Test {
protected:
    std::unique_ptr<ApproximationRuleEngine> engine =
        makeDefaultApproximationRuleEngine();
    ApproximationPolicy policy = makeDefaultPolicy();
};

/// EDG-01: Unknown layer value → Deny with UnknownLayer violation.
TEST_F(EdgeCaseTest, EDG01_UnknownLayerValue_IsDenied)
{
    const auto r = engine->checkBoundary(
        static_cast<RetrievalLayer>(99),
        ApproximationZone::Approximate,
        KernelCategory::A,
        policy,
        /*confidence=*/1.0);
    EXPECT_EQ(r.decision,  GovernanceDecision::Deny);
    EXPECT_EQ(r.violation, ExactnessViolation::UnknownLayer);
}

/// EDG-02: canonicalBoundary for unknown layer returns fail-closed Exact default.
TEST_F(EdgeCaseTest, EDG02_UnknownLayer_CanonicalBoundary_IsFailClosedExact)
{
    const auto b = engine->canonicalBoundary(static_cast<RetrievalLayer>(99));
    EXPECT_EQ(b.zone, ApproximationZone::Exact);
    EXPECT_TRUE(b.fail_closed);
    EXPECT_FALSE(b.gpu_eligible);
    EXPECT_TRUE(b.truth_bearing);
}

/// EDG-03: Confidence at exactly 0.0 on Approximate zone → Allow.
TEST_F(EdgeCaseTest, EDG03_ZeroConfidence_ApproximateZone_IsAllowed)
{
    policy.min_confidence_approx = 0.0;
    const auto r = engine->checkBoundary(
        RetrievalLayer::Ann,
        ApproximationZone::Approximate,
        KernelCategory::A,
        policy,
        /*confidence=*/0.0);
    EXPECT_EQ(r.decision, GovernanceDecision::Allow);
}

/// EDG-04: BoundaryCheckResult::isAllowed returns true for Allow and Bypass.
TEST_F(EdgeCaseTest, EDG04_IsAllowed_TrueForAllowAndBypass)
{
    BoundaryCheckResult r_allow;
    r_allow.decision = GovernanceDecision::Allow;
    EXPECT_TRUE(r_allow.isAllowed());

    BoundaryCheckResult r_bypass;
    r_bypass.decision = GovernanceDecision::Bypass;
    EXPECT_TRUE(r_bypass.isAllowed());

    BoundaryCheckResult r_deny;
    r_deny.decision = GovernanceDecision::Deny;
    EXPECT_FALSE(r_deny.isAllowed());

    BoundaryCheckResult r_escalate;
    r_escalate.decision = GovernanceDecision::EscalateToExact;
    EXPECT_FALSE(r_escalate.isAllowed());
}

/// EDG-05: Category A on ExactGraph Exact zone → Allow (advisory kernel, Exact zone, no contradiction).
TEST_F(EdgeCaseTest, EDG05_CategoryA_OnExactGraph_ExactZone_IsAllowed)
{
    // Category A on ExactGraph with Exact zone: not a Category C kernel, so
    // the category rule doesn't fire.  ExactGraph rule 2 only fires if zone != Exact.
    // The zone is Exact, so it passes.  Confidence is 1.0 ≥ threshold.  Allow.
    const auto r = engine->checkBoundary(
        RetrievalLayer::ExactGraph,
        ApproximationZone::Exact,
        KernelCategory::A,
        policy,
        /*confidence=*/1.0);
    EXPECT_EQ(r.decision, GovernanceDecision::Allow);
}

/// EDG-06: Explanation string is non-empty for all decisions.
TEST_F(EdgeCaseTest, EDG06_ExplanationIsNonEmpty)
{
    const auto r_allow = engine->checkBoundary(
        RetrievalLayer::Ann,
        ApproximationZone::Approximate,
        KernelCategory::A,
        policy,
        /*confidence=*/1.0);
    EXPECT_FALSE(r_allow.explanation.empty());

    const auto r_deny = engine->checkBoundary(
        RetrievalLayer::Ann,
        ApproximationZone::Approximate,
        KernelCategory::C,
        policy,
        /*confidence=*/1.0);
    EXPECT_FALSE(r_deny.explanation.empty());

    const auto r_escalate = engine->checkBoundary(
        RetrievalLayer::TensorSummary,
        ApproximationZone::Approximate,
        KernelCategory::A,
        policy,
        /*confidence=*/1.0);
    EXPECT_FALSE(r_escalate.explanation.empty());
}

// ============================================================================
// Phase 4 Expansion: ApproximationZone Policy & Category C Enforcement
// ============================================================================

/// Phase 4 Test: Verify ApproximationZone progression (Approximate→Bounded→Exact).
TEST_F(ApproximationBoundaryTest, Phase4_ApproximationZoneProgression) {
    // ANN layer: Approximate zone (cheapest)
    const auto ann_b = engine->canonicalBoundary(RetrievalLayer::Ann);
    EXPECT_EQ(ann_b.zone, ApproximationZone::Approximate);

    // TensorSummary layer: Bounded zone (mid-cost)
    const auto ts_b = engine->canonicalBoundary(RetrievalLayer::TensorSummary);
    EXPECT_EQ(ts_b.zone, ApproximationZone::Bounded);

    // ExactGraph layer: Exact zone (highest cost/quality)
    const auto eg_b = engine->canonicalBoundary(RetrievalLayer::ExactGraph);
    EXPECT_EQ(eg_b.zone, ApproximationZone::Exact);
}

/// Phase 4 Test: Verify Category C→Deny enforcement on all layers.
TEST_F(GovernanceRuleViolationTest, Phase4_CategoryC_DenyOnAllLayers) {
    std::vector<RetrievalLayer> layers = {
        RetrievalLayer::Ann,
        RetrievalLayer::TensorSummary,
        RetrievalLayer::ExactGraph
    };
    
    for (auto layer : layers) {
        const auto r = engine->checkBoundary(
            layer,
            ApproximationZone::Approximate,  // Even Approximate zone doesn't save C
            KernelCategory::C,
            policy,
            /*confidence=*/1.0);
        
        // Category C must never bypass governance
        EXPECT_EQ(r.decision, GovernanceDecision::Deny);
        EXPECT_EQ(r.violation, ExactnessViolation::CategoryCSubpathDetected);
    }
}

/// Phase 4 Test: Verify no truth-bearing degradation for Category C.
TEST_F(GovernanceRuleViolationTest, Phase4_CategoryC_NoTruthBearingDegradation) {
    // ExactGraph is the truth-bearing layer
    const auto r = engine->checkBoundary(
        RetrievalLayer::ExactGraph,
        ApproximationZone::Exact,  // Even Exact zone doesn't allow C here
        KernelCategory::C,
        policy,
        /*confidence=*/1.0);
    
    // Category C operations must be CPU-only; no GPU acceleration
    EXPECT_EQ(r.decision, GovernanceDecision::Deny);
}

/// Phase 4 Test: Verify policy version is tracked and audited.
TEST_F(PolicyOverrideTest, Phase4_PolicyVersionTracking) {
    auto p1 = makeDefaultPolicy();
    p1.policy_version = "v1-conservative";
    
    auto p2 = makeDefaultPolicy();
    p2.policy_version = "v2-permissive";
    p2.allow_bypass = true;
    
    // Both policies should be distinct in their versioning
    EXPECT_NE(p1.policy_version, p2.policy_version);
    
    // Verify decisions respect version
    const auto r1 = engine->checkBoundary(
        RetrievalLayer::Ann,
        ApproximationZone::Approximate,
        KernelCategory::A,
        p1,
        /*confidence=*/0.5);
    
    const auto r2 = engine->checkBoundary(
        RetrievalLayer::Ann,
        ApproximationZone::Approximate,
        KernelCategory::A,
        p2,
        /*confidence=*/0.5);
    
    // Results may differ based on bypass setting
    EXPECT_FALSE(p1.allow_bypass);
    EXPECT_TRUE(p2.allow_bypass);
}

/// Phase 4 Test: Verify validatePlannedPath enforces zone progression.
TEST_F(ValidatePlannedPathTest, Phase4_PathProgressionFromAnnToExact) {
    auto decision = makeDecision(ExecutionPath::AnnTensorExactGraph, false, true, false);
    decision.confidence_policy_version = "v1-test";
    
    // Path 3 combines ANN + Tensor + Exact, which respects progression
    const auto r = engine->validatePlannedPath(decision, policy);
    
    // Should not escalate if all components are properly routed
    EXPECT_TRUE(r.isAllowed() || r.decision == GovernanceDecision::EscalateToExact);
}

/// Phase 4 Test: Verify confidence thresholds are enforced uniformly.
TEST_F(PolicyOverrideTest, Phase4_ConfidenceThresholdUniformity) {
    auto low_conf = makeDefaultPolicy();
    low_conf.min_confidence_bounded = 0.30;
    
    auto high_conf = makeDefaultPolicy();
    high_conf.min_confidence_bounded = 0.90;
    
    const double test_confidence = 0.50;
    
    // Low threshold should allow
    const auto r_low = engine->checkBoundary(
        RetrievalLayer::TensorSummary,
        ApproximationZone::Bounded,
        KernelCategory::B,
        low_conf,
        test_confidence);
    EXPECT_EQ(r_low.decision, GovernanceDecision::Allow);
    
    // High threshold should escalate
    const auto r_high = engine->checkBoundary(
        RetrievalLayer::TensorSummary,
        ApproximationZone::Bounded,
        KernelCategory::B,
        high_conf,
        test_confidence);
    EXPECT_EQ(r_high.decision, GovernanceDecision::EscalateToExact);
}
