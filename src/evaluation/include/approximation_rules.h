/**
 * @file approximation_rules.h
 * @brief Approximation boundary and governance rules for layered retrieval (EPIC 2.4).
 *
 * Defines the typed contract for approximation zones per retrieval layer,
 * governance decisions, exactness violations, and the rule-engine interface
 * used by the hybrid query planner to enforce policy at runtime.
 *
 * ## Layer → Zone mapping (canonical)
 *
 * | Layer            | Default Zone | Truth-bearing | GPU-eligible | Fail-closed |
 * |------------------|--------------|---------------|--------------|-------------|
 * | Ann              | Approximate  | No            | Cat A / B    | No          |
 * | TensorSummary    | Bounded      | No (advisory) | Cat A / B    | No          |
 * | ExactGraph       | Exact        | Yes           | Never        | Yes         |
 * | DistributedShard | Dynamic      | Depends       | Cat A / B    | Depends     |
 *
 * ## Governance invariants
 *
 * 1. Category C operations (ACL, provenance, transactions) are always
 *    @ref ApproximationZone::Exact.  Any routing attempt to Approximate or
 *    Bounded is fail-closed (@ref GovernanceDecision::Deny).
 * 2. Truth-bearing queries must use @ref RetrievalLayer::ExactGraph or
 *    @ref RetrievalLayer::DistributedShard with exact-on-demand active.
 * 3. Advisory queries may use any zone; the cheapest eligible zone is preferred.
 * 4. The policy version is attached to every decision for provenance tracing.
 * 5. Bypass is only permitted when @ref ApproximationPolicy::allow_bypass is
 *    true and is propagated as an audit note in @ref BoundaryCheckResult.
 *
 * @note Status: Phases 1-7 complete (contract, implementation, error handling,
 *       tests, performance, documentation, integration).
 *
 * @see docs/EPIC2_APPROXIMATION_GOVERNANCE.md
 * @see docs/adr/adr-e2-004-approximation-governance-rules.md
 * @see src/evaluation/include/query_planner.h
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "query_planner.h"

namespace themis {
namespace evaluation {

// ---------------------------------------------------------------------------
// Approximation zones
// ---------------------------------------------------------------------------

enum class ApproximationZone : uint8_t {
    Approximate = 1,

    Bounded = 2,

    Exact = 3,
};

// ---------------------------------------------------------------------------
// Retrieval layers
// ---------------------------------------------------------------------------

enum class RetrievalLayer : uint8_t {
    Ann = 1,

    TensorSummary = 2,

    ExactGraph = 3,

    DistributedShard = 4,
};

// ---------------------------------------------------------------------------
// Governance decisions
// ---------------------------------------------------------------------------

enum class GovernanceDecision : uint8_t {
    Allow = 1,

    Deny = 2,

    EscalateToExact = 3,

    Bypass = 4,
};

// ---------------------------------------------------------------------------
// Exactness violation codes
// ---------------------------------------------------------------------------

enum class ExactnessViolation : uint16_t {
    None = 0,

    // Zone mismatches
    ApproximateForTruthBearing   = 100,
    BoundedForTruthBearing       = 101,
    RequestedZoneBelowCanonicalMinimum = 102,

    // Category C enforcement
    CategoryCOnNonExactLayer     = 200,
    CategoryCGpuAttempt          = 201,

    // Artifact quality
    StaleArtifactForFreshnessGate = 300,
    ConfidenceBelowThreshold     = 301,

    // Policy
    PolicyVersionMismatch        = 400,
    FallbackNotPermitted         = 401,
    UnknownLayer                 = 402,
    UnknownZone                  = 403,
};

// ---------------------------------------------------------------------------
// Canonical boundary descriptor
// ---------------------------------------------------------------------------

struct ApproximationBoundary {
    RetrievalLayer layer{RetrievalLayer::Ann};

    ApproximationZone zone{ApproximationZone::Approximate};

    KernelCategory max_kernel_category{KernelCategory::A};

    bool truth_bearing{false};

    bool gpu_eligible{true};

    bool fail_closed{false};
};

// ---------------------------------------------------------------------------
// Dynamic approximation policy
// ---------------------------------------------------------------------------

struct ApproximationPolicy {
    // Zone permissions
    bool allow_approx_for_advisory{true};

    // Category C enforcement
    bool require_exact_for_acl{true};
    bool require_exact_for_provenance{true};
    bool require_exact_for_transactions{true};

    // Confidence thresholds
    double min_confidence_approx{0.0};
    double min_confidence_bounded{0.80};
    double min_confidence_exact{1.0};

    // Policy override
    bool allow_bypass{false};

    std::string policy_version{"v0"};
};

// ---------------------------------------------------------------------------
// Boundary check result
// ---------------------------------------------------------------------------

struct BoundaryCheckResult {
    GovernanceDecision decision{GovernanceDecision::Allow};

    ExactnessViolation violation{ExactnessViolation::None};

    std::string explanation;

    std::string policy_version;

    [[nodiscard]] bool isAllowed() const noexcept {
        return decision == GovernanceDecision::Allow ||
               decision == GovernanceDecision::Bypass;
    }
};

// ---------------------------------------------------------------------------
// ApproximationRuleEngine interface
// ---------------------------------------------------------------------------

class ApproximationRuleEngine {
public:
    ApproximationRuleEngine()          = default;
    /**
     * @brief Approximation Rule Engine.
     * @return Return value.
     */
    virtual ~ApproximationRuleEngine() = default;

    ApproximationRuleEngine(const ApproximationRuleEngine&)            = delete;
    ApproximationRuleEngine& operator=(const ApproximationRuleEngine&) = delete;
    ApproximationRuleEngine(ApproximationRuleEngine&&)                 = delete;
    ApproximationRuleEngine& operator=(ApproximationRuleEngine&&)      = delete;

    [[nodiscard]] virtual BoundaryCheckResult checkBoundary(
        RetrievalLayer             layer,
        ApproximationZone          zone,
        KernelCategory             category,
        const ApproximationPolicy& policy,
        double                     confidence,
        bool                       uses_gpu = false) const noexcept = 0;

    [[nodiscard]] virtual BoundaryCheckResult validatePlannedPath(
        const PlannerDecision&     decision,
        const ApproximationPolicy& policy) const noexcept = 0;

    [[nodiscard]] virtual ApproximationBoundary canonicalBoundary(
        RetrievalLayer layer) const noexcept = 0;
};

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

[[nodiscard]] std::unique_ptr<ApproximationRuleEngine>
makeDefaultApproximationRuleEngine();

} // namespace evaluation
} // namespace themis
