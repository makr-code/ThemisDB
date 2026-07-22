/**
 * @file approximation_rules.cc
 * @brief Production implementation of approximation boundary and governance
 *        rules for layered retrieval (EPIC 2.4).
 *
 * Implements `DefaultApproximationRuleEngine`, the concrete rule-engine that
 * enforces per-layer approximation zones, Category C fail-closed rules, and
 * dynamic policy overrides as defined in:
 *   - `docs/EPIC2_APPROXIMATION_GOVERNANCE.md`
 *   - `docs/adr/adr-e2-004-approximation-governance-rules.md`
 *
 * ## Boundary enforcement overview
 *
 * For each `checkBoundary()` call the engine evaluates rules in priority order:
 *
 * 1. **Category C fail-closed** — any Category C kernel on a non-Exact path is
 *    rejected immediately regardless of other policy settings.
 * 2. **GPU attempt on Exact layer** — GPU dispatch to ExactGraph is rejected.
 * 3. **Zone/layer mismatch** — requesting a looser zone than the canonical
 *    minimum for the layer triggers EscalateToExact.
 * 4. **Truth-bearing mismatch** — approximate/bounded zone requested for a
 *    truth-bearing layer is rejected unless bypass is in effect.
 * 5. **Confidence threshold** — confidence below the policy minimum for the
 *    requested zone triggers EscalateToExact (Exact zone) or Deny (Exact zone
 *    with confidence below 1.0).
 * 6. **Policy ACL / provenance / transaction override** — known Category C
 *    workload types force Exact zone regardless of the requested zone.
 * 7. **Bypass** — only applied when `policy.allow_bypass == true` and no
 *    Category C / GPU invariant was violated.
 *
 * @see include/approximation_rules.h
 * @see include/query_planner.h
 */

#include "approximation_rules.h"

#include <array>
#include <string_view>

namespace themis {
namespace evaluation {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/**
 * @brief Static canonical boundary table (one entry per RetrievalLayer).
 *
 * Indexed by `static_cast<size_t>(layer) - 1` (layer values start at 1).
 */
constexpr std::array<ApproximationBoundary, 4> kCanonicalBoundaries = {{
    // Ann — Approximate, Category A, advisory, GPU eligible, not fail-closed
    {
        RetrievalLayer::Ann,
        ApproximationZone::Approximate,
        KernelCategory::A,
        /*truth_bearing=*/false,
        /*gpu_eligible=*/true,
        /*fail_closed=*/false,
    },
    // TensorSummary — Bounded, Category B, advisory, GPU eligible, not fail-closed
    {
        RetrievalLayer::TensorSummary,
        ApproximationZone::Bounded,
        KernelCategory::B,
        /*truth_bearing=*/false,
        /*gpu_eligible=*/true,
        /*fail_closed=*/false,
    },
    // ExactGraph — Exact, Category C, truth-bearing, GPU never, fail-closed
    {
        RetrievalLayer::ExactGraph,
        ApproximationZone::Exact,
        KernelCategory::C,
        /*truth_bearing=*/true,
        /*gpu_eligible=*/false,
        /*fail_closed=*/true,
    },
    // DistributedShard — dynamic (Bounded by default), truth-bearing varies
    {
        RetrievalLayer::DistributedShard,
        ApproximationZone::Bounded,
        KernelCategory::B,
        /*truth_bearing=*/false,
        /*gpu_eligible=*/true,
        /*fail_closed=*/false,
    },
}};

/**
 * @brief Look up the canonical boundary for a layer.
 *
 * @param layer  The retrieval layer.
 * @param out    Filled with the canonical boundary on success.
 * @return True on success; false if the layer value is out of range.
 */
[[nodiscard]] bool lookupCanonical(
    RetrievalLayer         layer,
    ApproximationBoundary& out) noexcept
{
    const auto idx = static_cast<size_t>(layer);
    if (idx < 1 || idx > kCanonicalBoundaries.size()) return false;
    out = kCanonicalBoundaries[idx - 1];
    return true;
}

/**
 * @brief True when the requested zone is at least as strict as the canonical zone.
 *
 * Zone strictness order (least strict → most strict):
 *   Approximate < Bounded < Exact
 *
 * A requested zone is "strict enough" when it is ≥ the canonical minimum.
 */
[[nodiscard]] constexpr bool isZoneStrictEnough(
    ApproximationZone requested,
    ApproximationZone canonical_minimum) noexcept
{
    return static_cast<uint8_t>(requested) >=
           static_cast<uint8_t>(canonical_minimum);
}

/**
 * @brief Return the minimum confidence threshold for the requested zone from policy.
 */
[[nodiscard]] double minConfidence(
    ApproximationZone          zone,
    const ApproximationPolicy& policy) noexcept
{
    switch (zone) {
        case ApproximationZone::Approximate: return policy.min_confidence_approx;
        case ApproximationZone::Bounded:     return policy.min_confidence_bounded;
        case ApproximationZone::Exact:       return policy.min_confidence_exact;
    }
    return 1.0; // unknown zone — conservative
}

[[nodiscard]] constexpr ExactnessViolation truthBearingViolationFor(
    ApproximationZone zone) noexcept
{
    return zone == ApproximationZone::Bounded
        ? ExactnessViolation::BoundedForTruthBearing
        : ExactnessViolation::ApproximateForTruthBearing;
}

[[nodiscard]] constexpr uint8_t decisionPriority(
    GovernanceDecision decision) noexcept
{
    switch (decision) {
        case GovernanceDecision::Allow:           return 0;
        case GovernanceDecision::Bypass:          return 1;
        case GovernanceDecision::EscalateToExact: return 2;
        case GovernanceDecision::Deny:            return 3;
    }
    return 0;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// DefaultApproximationRuleEngine
// ---------------------------------------------------------------------------

/**
 * @brief Default production approximation rule engine.
 *
 * Enforces the invariants from `docs/EPIC2_APPROXIMATION_GOVERNANCE.md`
 * and ADR E2-004.  All check methods are thread-safe and noexcept.
 */
class DefaultApproximationRuleEngine final : public ApproximationRuleEngine {
public:
    DefaultApproximationRuleEngine()  = default;
    ~DefaultApproximationRuleEngine() override = default;

    // -----------------------------------------------------------------
    // ApproximationRuleEngine interface
    // -----------------------------------------------------------------

    /**
     * @brief Evaluate approximation boundary and policy rules for one request.
     *
     * Applies rules in priority order (see file-level overview).  Returns
     * the first violation found, or Allow when all rules pass.
     *
     * @param layer       Retrieval layer being evaluated.
     * @param zone        Zone being requested by the caller.
     * @param category    Kernel category active at this layer.
     * @param policy      Active governance policy.
     * @param confidence  Query-time confidence in [0.0, 1.0].
     * @param uses_gpu    True when this layer is being asked to dispatch on GPU.
     * @return @ref BoundaryCheckResult with decision and optional violation.
     */
    [[nodiscard]] BoundaryCheckResult checkBoundary(
        RetrievalLayer             layer,
        ApproximationZone          zone,
        KernelCategory             category,
        const ApproximationPolicy& policy,
        double                     confidence,
        bool                       uses_gpu) const noexcept override
    {
        BoundaryCheckResult result;
        result.policy_version = policy.policy_version;

        ApproximationBoundary canonical{};
        if (!lookupCanonical(layer, canonical)) {
            result.decision  = GovernanceDecision::Deny;
            result.violation = ExactnessViolation::UnknownLayer;
            result.explanation =
                "Unknown RetrievalLayer value: " +
                std::to_string(static_cast<int>(layer));
            return result;
        }

        // ------------------------------------------------------------------
        // Rule 1: Category C fail-closed — always requires Exact zone.
        // GPU dispatch for Category C is also forbidden unconditionally.
        // ------------------------------------------------------------------
        if (category == KernelCategory::C) {
            if (zone != ApproximationZone::Exact) {
                result.decision  = GovernanceDecision::Deny;
                result.violation = ExactnessViolation::CategoryCOnNonExactLayer;
                result.explanation =
                    "Category C operation (ACL/provenance/transaction) requires "
                    "Exact zone; requested zone is not Exact. "
                    "policy=" + policy.policy_version;
                return result;
            }
            if (layer != RetrievalLayer::ExactGraph) {
                result.decision  = GovernanceDecision::Deny;
                result.violation = ExactnessViolation::CategoryCOnNonExactLayer;
                result.explanation =
                    "Category C operation must use ExactGraph layer; "
                    "requested layer is not ExactGraph. "
                    "policy=" + policy.policy_version;
                return result;
            }
        }

        // ------------------------------------------------------------------
        // Rule 2: CPU-only layers reject GPU dispatch attempts.
        // ------------------------------------------------------------------
        if (uses_gpu && !canonical.gpu_eligible) {
            result.decision  = GovernanceDecision::Deny;
            result.violation = ExactnessViolation::CategoryCGpuAttempt;
            result.explanation =
                "GPU dispatch is not permitted for layer "
                + std::to_string(static_cast<int>(layer)) + ". "
                "policy=" + policy.policy_version;
            return result;
        }

        // ------------------------------------------------------------------
        // Rule 3: ExactGraph requires the Exact zone.
        // ------------------------------------------------------------------
        if (layer == RetrievalLayer::ExactGraph &&
            zone != ApproximationZone::Exact)
        {
            result.decision  = GovernanceDecision::EscalateToExact;
            result.violation = truthBearingViolationFor(zone);
            result.explanation =
                "ExactGraph layer is truth-bearing and requires Exact zone; "
                "escalating from zone="
                + std::to_string(static_cast<int>(zone)) + ". "
                "policy=" + policy.policy_version;
            return result;
        }

        // ------------------------------------------------------------------
        // Rule 4: Policy-driven ACL / provenance / transaction enforcement.
        // These mirror Category C but are enforced per-policy flag so that
        // future policy relaxation is possible without code changes.
        // ------------------------------------------------------------------
        const bool isAcl         = (category == KernelCategory::C) &&
                                    policy.require_exact_for_acl;
        const bool isProvenance  = (category == KernelCategory::C) &&
                                    policy.require_exact_for_provenance;
        const bool isTransaction = (category == KernelCategory::C) &&
                                    policy.require_exact_for_transactions;

        if ((isAcl || isProvenance || isTransaction) &&
            zone != ApproximationZone::Exact)
        {
            result.decision  = GovernanceDecision::Deny;
            result.violation = ExactnessViolation::CategoryCOnNonExactLayer;
            result.explanation =
                "Policy requires Exact zone for ACL/provenance/transaction "
                "operations. policy=" + policy.policy_version;
            return result;
        }

        // ------------------------------------------------------------------
        // Rule 5: Zone strictness — requested zone must be ≥ canonical minimum.
        // ------------------------------------------------------------------
        if (!isZoneStrictEnough(zone, canonical.zone)) {
            // Looser zone than the canonical minimum requested — escalate
            // rather than deny, so the caller can upgrade the path safely.
            if (canonical.fail_closed) {
                result.decision  = GovernanceDecision::Deny;
                result.violation = canonical.truth_bearing
                    ? truthBearingViolationFor(zone)
                    : ExactnessViolation::RequestedZoneBelowCanonicalMinimum;
                result.explanation =
                    "Layer " + std::to_string(static_cast<int>(layer)) +
                    " is fail-closed and requires zone >= " +
                    std::to_string(static_cast<int>(canonical.zone)) +
                    "; requested zone=" +
                    std::to_string(static_cast<int>(zone)) + ". "
                    "policy=" + policy.policy_version;
                return result;
            }
            result.decision  = GovernanceDecision::EscalateToExact;
            result.violation = canonical.truth_bearing
                ? truthBearingViolationFor(zone)
                : ExactnessViolation::RequestedZoneBelowCanonicalMinimum;
            result.explanation =
                "Requested zone is looser than canonical minimum for layer "
                + std::to_string(static_cast<int>(layer)) +
                "; escalating to Exact zone. "
                "policy=" + policy.policy_version;
            return result;
        }

        // ------------------------------------------------------------------
        // Rule 6: Confidence threshold.
        // ------------------------------------------------------------------
        const double threshold = minConfidence(zone, policy);
        if (confidence < threshold) {
            if (zone == ApproximationZone::Exact) {
                // Exact zone requires confidence == 1.0; below threshold is Deny.
                result.decision  = GovernanceDecision::Deny;
                result.violation = ExactnessViolation::ConfidenceBelowThreshold;
                result.explanation =
                    "Exact zone requires confidence >= " +
                    std::to_string(threshold) + "; got " +
                    std::to_string(confidence) + ". "
                    "policy=" + policy.policy_version;
                return result;
            }
            // For Approximate/Bounded zones escalate to Exact so the caller
            // can choose a higher-quality path.
            result.decision  = GovernanceDecision::EscalateToExact;
            result.violation = ExactnessViolation::ConfidenceBelowThreshold;
            result.explanation =
                "Confidence " + std::to_string(confidence) +
                " is below zone threshold " + std::to_string(threshold) +
                "; escalating to Exact zone. "
                "policy=" + policy.policy_version;
            return result;
        }

        // ------------------------------------------------------------------
        // Rule 7: Advisory-query gate.
        // If the caller requests an approximate zone for a non-advisory query
        // (truth_bearing layer), and bypass is not allowed, deny.
        // ------------------------------------------------------------------
        if (canonical.truth_bearing && zone != ApproximationZone::Exact) {
            if (!policy.allow_bypass) {
                result.decision  = GovernanceDecision::Deny;
                result.violation = truthBearingViolationFor(zone);
                result.explanation =
                    "Layer is truth-bearing but requested zone is not Exact "
                    "and bypass is disabled. "
                    "policy=" + policy.policy_version;
                return result;
            }
            // Bypass is permitted — log it.
            result.decision  = GovernanceDecision::Bypass;
            result.violation = ExactnessViolation::None;
            result.explanation =
                "Policy bypass applied for truth-bearing layer with non-Exact "
                "zone. Audit note: bypass_requested=true. "
                "policy=" + policy.policy_version;
            return result;
        }

        // ------------------------------------------------------------------
        // All rules passed.
        // ------------------------------------------------------------------
        result.decision    = GovernanceDecision::Allow;
        result.violation   = ExactnessViolation::None;
        result.explanation = "Boundary check passed. policy=" + policy.policy_version;
        return result;
    }

    /**
     * @brief Validate a planner decision against approximation boundaries.
     *
     * Maps each @ref ExecutionPath to its implied layer(s) and applies
     * `checkBoundary()` for each.  Returns the most restrictive result.
     *
     * Path → layer(s) mapping:
     *   - AnnOnly                          → {Ann}
     *   - AnnTensorSummary                 → {Ann, TensorSummary}
     *   - AnnTensorExactGraph              → {Ann, TensorSummary, ExactGraph}
     *   - DirectExactGraph                 → {ExactGraph}
     *   - DistributedSummaryFirstExactOnDemand → {DistributedShard, ExactGraph}
     *
     * @param decision  The planner decision to validate.
     * @param policy    Active governance policy.
     * @return @ref BoundaryCheckResult representing the overall compliance.
     */
    [[nodiscard]] BoundaryCheckResult validatePlannedPath(
        const PlannerDecision&     decision,
        const ApproximationPolicy& policy) const noexcept override
    {
        // Derive implied zones from the decision flags.
        // GPU usage implies Category A/B; exact graph implies Category C.
        const KernelCategory graphCategory = KernelCategory::C;
        const KernelCategory annCategory   = KernelCategory::A;

        // Helper lambda: run checkBoundary and return Deny/Escalate early.
        auto check = [&](RetrievalLayer  layer,
                         ApproximationZone zone,
                         KernelCategory    cat,
                         double            conf,
                         bool              uses_gpu = false) -> BoundaryCheckResult {
            return checkBoundary(layer, zone, cat, policy, conf, uses_gpu);
        };

        BoundaryCheckResult worst;
        worst.policy_version = policy.policy_version;
        worst.decision       = GovernanceDecision::Allow;
        worst.violation      = ExactnessViolation::None;

        auto merge = [&](const BoundaryCheckResult& r) {
            if (decisionPriority(r.decision) >
                decisionPriority(worst.decision))
            {
                worst = r;
            }
        };

        switch (decision.path) {
            case ExecutionPath::AnnOnly:
                merge(check(RetrievalLayer::Ann,
                            ApproximationZone::Approximate,
                            annCategory, 1.0, decision.uses_gpu));
                break;

            case ExecutionPath::AnnTensorSummary:
                merge(check(RetrievalLayer::Ann,
                            ApproximationZone::Approximate,
                            annCategory, 1.0, decision.uses_gpu));
                merge(check(RetrievalLayer::TensorSummary,
                            ApproximationZone::Bounded,
                            annCategory, policy.min_confidence_bounded, decision.uses_gpu));
                break;

            case ExecutionPath::AnnTensorExactGraph:
                merge(check(RetrievalLayer::Ann,
                            ApproximationZone::Approximate,
                            annCategory, 1.0, decision.uses_gpu));
                merge(check(RetrievalLayer::TensorSummary,
                            ApproximationZone::Bounded,
                            annCategory, policy.min_confidence_bounded, decision.uses_gpu));
                merge(check(RetrievalLayer::ExactGraph,
                            ApproximationZone::Exact,
                            graphCategory, 1.0));
                break;

            case ExecutionPath::DirectExactGraph:
                merge(check(RetrievalLayer::ExactGraph,
                            ApproximationZone::Exact,
                            graphCategory, 1.0, decision.uses_gpu));
                break;

            case ExecutionPath::DistributedSummaryFirstExactOnDemand:
                merge(check(RetrievalLayer::DistributedShard,
                            ApproximationZone::Bounded,
                            annCategory, policy.min_confidence_bounded, decision.uses_gpu));
                merge(check(RetrievalLayer::ExactGraph,
                            ApproximationZone::Exact,
                            graphCategory, 1.0));
                break;
        }

        return worst;
    }

    /**
     * @brief Return the canonical boundary descriptor for a retrieval layer.
     *
     * @param layer  The retrieval layer.
     * @return Canonical @ref ApproximationBoundary.  Returns a safe default
     *         (ExactGraph / Exact / fail-closed) for unrecognized layer values.
     */
    [[nodiscard]] ApproximationBoundary canonicalBoundary(
        RetrievalLayer layer) const noexcept override
    {
        ApproximationBoundary b{};
        if (lookupCanonical(layer, b)) return b;
        // Safe default for unknown layers: fail-closed Exact.
        return ApproximationBoundary{
            layer,
            ApproximationZone::Exact,
            KernelCategory::C,
            /*truth_bearing=*/true,
            /*gpu_eligible=*/false,
            /*fail_closed=*/true,
        };
    }
};

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

/**
 * @brief Create the default production approximation rule engine.
 *
 * @return std::unique_ptr<ApproximationRuleEngine> owning the default engine.
 */
[[nodiscard]] std::unique_ptr<ApproximationRuleEngine>
makeDefaultApproximationRuleEngine()
{
    return std::make_unique<DefaultApproximationRuleEngine>();
}

} // namespace evaluation
} // namespace themis
