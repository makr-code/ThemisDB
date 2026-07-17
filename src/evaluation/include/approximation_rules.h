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

/**
 * @brief The three approximation zones used to classify retrieval results.
 *
 * The zone determines whether a result is truth-bearing, advisory, or
 * bounded-advisory, and governs downstream validation requirements.
 */
enum class ApproximationZone : uint8_t {
    /**
     * @brief Advisory output only; no truth guarantee.
     *
     * Used for ANN candidate generation (Category A kernels).  Callers
     * must not use approximate results in truth-bearing decisions without
     * an additional validation step.
     */
    Approximate = 1,

    /**
     * @brief Approximate with strict eligibility gates and CPU parity checks.
     *
     * Used for tensor-summary refinement (Category A/B kernels).  Results
     * are advisory; CPU parity confirmation is mandatory before any
     * truth-bearing use.
     */
    Bounded = 2,

    /**
     * @brief Truth-bearing; full exactness guarantee.
     *
     * Required for ACL enforcement, provenance construction, and
     * transaction consistency verification (Category C operations).
     * GPU dispatch is never permitted.  Violations are fail-closed.
     */
    Exact = 3,
};

// ---------------------------------------------------------------------------
// Retrieval layers
// ---------------------------------------------------------------------------

/**
 * @brief The four canonical retrieval layers in the hybrid planner.
 *
 * Each layer has a canonical @ref ApproximationZone and a set of governance
 * rules enforced by the @ref ApproximationRuleEngine.
 */
enum class RetrievalLayer : uint8_t {
    /**
     * @brief ANN candidate generation.
     *
     * Canonical zone: @ref ApproximationZone::Approximate.
     * GPU-eligible for Category A kernels after error-handling gate.
     * Output is advisory; downstream graph validation is mandatory.
     */
    Ann = 1,

    /**
     * @brief Tensor-artifact summary lookup and optional CPU/GPU refinement.
     *
     * Canonical zone: @ref ApproximationZone::Bounded.
     * GPU-eligible for Category A/B kernels after parity gate.
     * Artifact must be fresh; result is advisory (never final truth).
     */
    TensorSummary = 2,

    /**
     * @brief Exact graph traversal.
     *
     * Canonical zone: @ref ApproximationZone::Exact.
     * CPU-only (GPU dispatch never permitted).
     * Results are truth-bearing; violations are fail-closed.
     */
    ExactGraph = 3,

    /**
     * @brief Multi-shard distributed retrieval.
     *
     * Zone is dynamic: @ref ApproximationZone::Bounded when shard summary
     * confidence meets the policy threshold; escalates to
     * @ref ApproximationZone::Exact per shard when confidence is insufficient.
     */
    DistributedShard = 4,
};

// ---------------------------------------------------------------------------
// Governance decisions
// ---------------------------------------------------------------------------

/**
 * @brief The outcome of an approximation boundary or governance check.
 */
enum class GovernanceDecision : uint8_t {
    /**
     * @brief The requested layer / zone combination is permitted under policy.
     */
    Allow = 1,

    /**
     * @brief The request is rejected.
     *
     * Used for fail-closed violations: Category C on a non-Exact path,
     * or a truth-bearing query on an advisory path without bypass.
     */
    Deny = 2,

    /**
     * @brief The request is not rejected but must escalate to the Exact zone.
     *
     * Used for bounded-zone queries that fall below the confidence threshold,
     * or for shard-local exact-on-demand triggers.
     */
    EscalateToExact = 3,

    /**
     * @brief The approximation boundary is overridden by explicit policy bypass.
     *
     * Only issued when @ref ApproximationPolicy::allow_bypass is true and the
     * caller provides a bypass token.  Every bypass decision is logged with
     * the policy version for audit purposes.
     */
    Bypass = 4,
};

// ---------------------------------------------------------------------------
// Exactness violation codes
// ---------------------------------------------------------------------------

/**
 * @brief Machine-readable codes for exactness and policy violations.
 *
 * Carried by @ref BoundaryCheckResult::violation.  @ref ExactnessViolation::None
 * indicates no violation and the decision is @ref GovernanceDecision::Allow.
 */
enum class ExactnessViolation : uint16_t {
    /// No violation; the request conforms to policy.
    None = 0,

    // Zone mismatches
    /// An approximate result was returned for a truth-bearing query.
    ApproximateForTruthBearing   = 100,
    /// A bounded-zone result was returned for a truth-bearing query.
    BoundedForTruthBearing       = 101,

    // Category C enforcement
    /// A Category C sub-path (ACL / provenance / transaction) was routed to
    /// a non-Exact layer.
    CategoryCOnNonExactLayer     = 200,
    /// GPU dispatch was attempted for a Category C kernel.
    CategoryCGpuAttempt          = 201,

    // Artifact quality
    /// A stale tensor artifact was used for a query that requires freshness.
    StaleArtifactForFreshnessGate = 300,
    /// Confidence is below the minimum threshold for the requested zone.
    ConfidenceBelowThreshold     = 301,

    // Policy
    /// The provided policy version does not match the active engine version.
    PolicyVersionMismatch        = 400,
    /// A fallback was requested but the active policy does not permit it.
    FallbackNotPermitted         = 401,
    /// The requested layer is not recognized by the engine.
    UnknownLayer                 = 402,
    /// The requested zone is not recognized by the engine.
    UnknownZone                  = 403,
};

// ---------------------------------------------------------------------------
// Canonical boundary descriptor
// ---------------------------------------------------------------------------

/**
 * @brief Canonical approximation boundary for a retrieval layer.
 *
 * These values encode the design-time invariants for each layer.  Runtime
 * checks use these as the authoritative reference before evaluating
 * dynamic policy overrides.
 */
struct ApproximationBoundary {
    /// The retrieval layer this boundary describes.
    RetrievalLayer layer{RetrievalLayer::Ann};

    /// The canonical approximation zone for this layer.
    ApproximationZone zone{ApproximationZone::Approximate};

    /// The strictest kernel category that may be used at this layer.
    /// @ref KernelCategory::C is only valid for @ref RetrievalLayer::ExactGraph.
    KernelCategory max_kernel_category{KernelCategory::A};

    /// True when the layer produces truth-bearing results.
    bool truth_bearing{false};

    /// True when GPU dispatch is structurally permitted for this layer.
    /// Category-level eligibility (A/B/C) must still be checked at call time.
    bool gpu_eligible{true};

    /// True when violations at this layer are fail-closed (reject, not downgrade).
    bool fail_closed{false};
};

// ---------------------------------------------------------------------------
// Dynamic approximation policy
// ---------------------------------------------------------------------------

/**
 * @brief Runtime policy controlling approximation zone enforcement.
 *
 * The policy is version-tagged and attached to every @ref BoundaryCheckResult
 * for provenance tracing in compliance with ADR E2-005.
 *
 * **Default values** reflect the conservative production posture:
 * advisory queries may use any zone; ACL, provenance, and transaction
 * queries always require @ref ApproximationZone::Exact.
 */
struct ApproximationPolicy {
    // Zone permissions
    /// Allow approximate results for advisory (non-truth-bearing) queries.
    bool allow_approx_for_advisory{true};

    // Category C enforcement
    /// ACL-enforcement queries must always use the Exact zone.
    bool require_exact_for_acl{true};
    /// Provenance-chain queries must always use the Exact zone.
    bool require_exact_for_provenance{true};
    /// Transaction consistency queries must always use the Exact zone.
    bool require_exact_for_transactions{true};

    // Confidence thresholds
    /// Minimum confidence score for the @ref ApproximationZone::Approximate zone.
    double min_confidence_approx{0.0};
    /// Minimum confidence score for the @ref ApproximationZone::Bounded zone.
    double min_confidence_bounded{0.80};
    /// Minimum confidence score for the @ref ApproximationZone::Exact zone.
    double min_confidence_exact{1.0};

    // Policy override
    /// When true, callers may request a @ref GovernanceDecision::Bypass.
    /// Every bypass is logged with the policy version.
    bool allow_bypass{false};

    /// Policy version string attached to all decisions for provenance tracing.
    std::string policy_version{"v0"};
};

// ---------------------------------------------------------------------------
// Boundary check result
// ---------------------------------------------------------------------------

/**
 * @brief The result of a single approximation boundary or governance check.
 *
 * Returned by every @ref ApproximationRuleEngine check method.  Callers
 * must inspect @ref decision and handle @ref GovernanceDecision::Deny as
 * a hard rejection; @ref GovernanceDecision::EscalateToExact as a mandatory
 * path upgrade; and @ref GovernanceDecision::Bypass as a policy-logged override.
 */
struct BoundaryCheckResult {
    /// The governance outcome.
    GovernanceDecision decision{GovernanceDecision::Allow};

    /// The violation code if the decision is Deny, EscalateToExact, or Bypass.
    /// @ref ExactnessViolation::None when decision is Allow without escalation.
    ExactnessViolation violation{ExactnessViolation::None};

    /// Human-readable explanation of the decision (for logging and observability).
    std::string explanation;

    /// Policy version used for this decision (provenance tracing).
    std::string policy_version;

    /// True when the check passed without any violation or escalation.
    [[nodiscard]] bool isAllowed() const noexcept {
        return decision == GovernanceDecision::Allow ||
               decision == GovernanceDecision::Bypass;
    }
};

// ---------------------------------------------------------------------------
// ApproximationRuleEngine interface
// ---------------------------------------------------------------------------

/**
 * @brief Interface for evaluating approximation boundaries and governance rules.
 *
 * Implementations enforce the per-layer zone invariants and dynamic policy
 * constraints defined in `docs/EPIC2_APPROXIMATION_GOVERNANCE.md`.
 *
 * **Thread safety**: implementations must be thread-safe for concurrent calls
 * to all check methods.
 *
 * **Failure contract**: check methods must not throw.  On any internal error,
 * a result with @ref GovernanceDecision::Deny and an appropriate
 * @ref ExactnessViolation is returned.
 */
class ApproximationRuleEngine {
public:
    ApproximationRuleEngine()          = default;
    virtual ~ApproximationRuleEngine() = default;

    ApproximationRuleEngine(const ApproximationRuleEngine&)            = delete;
    ApproximationRuleEngine& operator=(const ApproximationRuleEngine&) = delete;
    ApproximationRuleEngine(ApproximationRuleEngine&&)                 = delete;
    ApproximationRuleEngine& operator=(ApproximationRuleEngine&&)      = delete;

    /**
     * @brief Check whether the requested layer/zone/category combination is
     *        permitted under the given policy and confidence score.
     *
     * Evaluates:
     * 1. Category C fail-closed rule (Category C → Exact required).
     * 2. Truth-bearing / zone mismatch rule.
     * 3. Confidence threshold for the requested zone.
     * 4. Policy-specific ACL / provenance / transaction overrides.
     *
     * @param layer       The retrieval layer being evaluated.
     * @param zone        The approximation zone being requested.
     * @param category    The kernel category in use at this layer.
     * @param policy      Active governance policy.
     * @param confidence  Query-time confidence score in [0.0, 1.0].
     * @return            A @ref BoundaryCheckResult with decision and optional
     *                    violation code.
     *
     * @note This method must not throw.
     */
    [[nodiscard]] virtual BoundaryCheckResult checkBoundary(
        RetrievalLayer             layer,
        ApproximationZone          zone,
        KernelCategory             category,
        const ApproximationPolicy& policy,
        double                     confidence) const noexcept = 0;

    /**
     * @brief Validate that a planner decision respects approximation boundaries.
     *
     * Maps the @ref PlannerDecision execution path to the implied layer(s) and
     * zones, then applies the policy rules for each implied layer.  Returns the
     * most restrictive violation found, or Allow when the decision is compliant.
     *
     * @param decision  The planner decision to validate.
     * @param policy    Active governance policy.
     * @return          A @ref BoundaryCheckResult representing the overall
     *                  compliance of the decision.
     *
     * @note This method must not throw.
     */
    [[nodiscard]] virtual BoundaryCheckResult validatePlannedPath(
        const PlannerDecision&     decision,
        const ApproximationPolicy& policy) const noexcept = 0;

    /**
     * @brief Return the canonical boundary descriptor for a retrieval layer.
     *
     * The canonical boundary encodes the design-time invariants and is
     * independent of any runtime policy.  It is used by the planner to
     * determine default zone eligibility before applying dynamic overrides.
     *
     * @param layer  The retrieval layer.
     * @return       The canonical @ref ApproximationBoundary.
     *
     * @note This method must not throw.
     */
    [[nodiscard]] virtual ApproximationBoundary canonicalBoundary(
        RetrievalLayer layer) const noexcept = 0;
};

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

/**
 * @brief Create the default production approximation rule engine.
 *
 * The returned engine enforces the invariants documented in
 * `docs/EPIC2_APPROXIMATION_GOVERNANCE.md` and ADR E2-004.
 *
 * @return `std::unique_ptr<ApproximationRuleEngine>` owning the default engine.
 */
[[nodiscard]] std::unique_ptr<ApproximationRuleEngine>
makeDefaultApproximationRuleEngine();

} // namespace evaluation
} // namespace themis
