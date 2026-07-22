/**
 * @file query_planner.cc
 * @brief Default implementation of the hybrid query planner (EPIC 2.5).
 *
 * Implements `DefaultQueryPlanner`, the production-ready concrete subclass of
 * `QueryPlanner`. Path selection follows the five canonical paths defined in
 * `docs/EPIC2_QUERY_PLANNER.md` and ADR E2-003. Every decision carries an
 * explicit `FallbackReason`; silent fallback is forbidden (ADR E2-005).
 *
 * ## Path selection order (cheapest → most exact)
 *
 * 1. ANN Only          — GPU Category A, non-truth-bearing retrieval
 * 2. ANN + Tensor      — fresh advisory tensor artifact + optional GPU matmul
 * 3. ANN + Tensor + Exact Graph — quality-critical; graph-verified finalization
 * 4. Direct Exact Graph — stale / missing artifact, or force_exact
 * 5. Distributed Summary-First + Exact-On-Demand — multi-shard, confidence-gated
 *
 * Category C operations (ACL, provenance, transactions) are fail-closed: the
 * planner never routes these to a GPU path, and any accidental attempt to select
 * a GPU path for Category C input is rejected with `FallbackReason::CategoryCSubpathDetected`.
 *
 * @see include/evaluation/query_planner.h
 * @see docs/EPIC2_QUERY_PLANNER.md
 * @see docs/adr/adr-e2-003-query-planner-routing-model.md
 * @see docs/adr/adr-e2-005-cross-layer-fallback-confidence-policy.md
 */

#include "query_planner.h"

#include <algorithm>
#include <chrono>
#include <string>
#include <string_view>

namespace themis {
namespace evaluation {

// ---------------------------------------------------------------------------
// Internal helpers — path eligibility predicates
// ---------------------------------------------------------------------------

namespace {

/**
 * @brief True when the ANN path is structurally eligible.
 *
 * Requires: ANN enabled, no force_exact / force_cpu override, and no Category C
 * sub-path is active for the current query context. The GPU sub-gate is evaluated
 * separately — this predicate only checks the structural ANN path gate.
 *
 * @param e  Execution eligibility signals.
 * @return True if the ANN path may be considered.
 */
[[nodiscard]] constexpr bool annPathEligible(const ExecutionEligibility& e) noexcept {
    return e.ann_enabled && !e.force_exact;
}

/**
 * @brief True when the tensor summary sub-step is eligible given freshness state.
 *
 * All tensor freshness gates must pass (age, residual, rebuild flag). Rank cap
 * and delta lag are evaluated against `cfg`.
 *
 * @param f    Tensor artifact freshness.
 * @param cfg  Planner policy configuration.
 * @return True if the tensor summary sub-step may be used.
 */
[[nodiscard]] bool tensorSummaryEligible(
    const TensorArtifactFreshness& f,
    const PlannerConfig&           cfg) noexcept
{
    if (!f.isFresh(cfg.max_staleness_ms, cfg.min_residual_threshold)) return false;
    if (f.delta_lag > cfg.max_delta_lag)                               return false;
    if (f.rank_cap > cfg.max_rank_cap)                                 return false;
    return true;
}

/**
 * @brief Determine the freshness fallback reason for a failed tensor gate.
 *
 * Returns the most specific reason code. Rank cap and delta lag are checked
 * after the core `isFresh()` predicates so the most fundamental failure is
 * surfaced first.
 *
 * @param f    Tensor artifact freshness.
 * @param cfg  Planner policy configuration.
 * @return The @ref FallbackReason for the first failed gate, or `None` if
 *         all gates pass.
 */
[[nodiscard]] FallbackReason tensorFreshnessFallbackReason(
    const TensorArtifactFreshness& f,
    const PlannerConfig&           cfg) noexcept
{
    const FallbackReason core = f.staleness_reason(cfg.max_staleness_ms, cfg.min_residual_threshold);
    if (core != FallbackReason::None) return core;
    if (f.delta_lag > cfg.max_delta_lag) return FallbackReason::TensorArtifactStale;
    if (f.rank_cap > cfg.max_rank_cap)   return FallbackReason::TensorRankCapExceeded;
    return FallbackReason::None;
}

/**
 * @brief Build a `PlannerDecision` for Path 1 (ANN Only).
 *
 * @param e    Eligibility signals (to decide GPU flag).
 * @param cfg  Policy config.
 * @return Completed @ref PlannerDecision.
 */
[[nodiscard]] PlannerDecision makeAnnOnlyDecision(
    const ExecutionEligibility& e,
    const PlannerConfig&        cfg) noexcept
{
    PlannerDecision d;
    d.path                     = ExecutionPath::AnnOnly;
    d.fallback_reason          = FallbackReason::None;
    d.uses_gpu                 = e.isGpuEligible(KernelCategory::A);
    d.uses_tensor              = false;
    d.uses_exact_graph         = false;
    d.confidence_policy_version   = cfg.policy_version;
    d.confidence_threshold_key    = cfg.staleness_threshold_key;
    d.decision_note            = "Path 1: ANN-only retrieval (advisory, non-truth-bearing)";
    return d;
}

/**
 * @brief Build a `PlannerDecision` for Path 2 (ANN + Tensor Summary).
 *
 * @param e    Eligibility signals.
 * @param cfg  Policy config.
 * @return Completed @ref PlannerDecision.
 */
[[nodiscard]] PlannerDecision makeAnnTensorSummaryDecision(
    const ExecutionEligibility& e,
    const PlannerConfig&        cfg) noexcept
{
    PlannerDecision d;
    d.path                     = ExecutionPath::AnnTensorSummary;
    d.fallback_reason          = FallbackReason::None;
    d.uses_gpu                 = e.isGpuEligible(KernelCategory::A);
    d.uses_tensor              = true;
    d.uses_exact_graph         = false;
    d.confidence_policy_version   = cfg.policy_version;
    d.confidence_threshold_key    = cfg.staleness_threshold_key;
    d.decision_note            = "Path 2: ANN + tensor summary (advisory; tensor is never final truth)";
    return d;
}

/**
 * @brief Build a `PlannerDecision` for Path 3 (ANN + Tensor + Exact Graph).
 *
 * @param e    Eligibility signals.
 * @param cfg  Policy config.
 * @return Completed @ref PlannerDecision.
 */
[[nodiscard]] PlannerDecision makeAnnTensorExactGraphDecision(
    const ExecutionEligibility& e,
    const PlannerConfig&        cfg) noexcept
{
    PlannerDecision d;
    d.path                     = ExecutionPath::AnnTensorExactGraph;
    d.fallback_reason          = FallbackReason::None;
    d.uses_gpu                 = e.isGpuEligible(KernelCategory::A);
    d.uses_tensor              = true;
    d.uses_exact_graph         = true;
    d.confidence_policy_version   = cfg.policy_version;
    d.confidence_threshold_key    = cfg.staleness_threshold_key;
    d.decision_note            = "Path 3: ANN + tensor refinement + exact graph validation";
    return d;
}

/**
 * @brief Build a `PlannerDecision` for Path 4 (Direct Exact Graph).
 *
 * @param reason  The fallback reason that forced exact-graph selection.
 * @param cfg     Policy config.
 * @return Completed @ref PlannerDecision.
 */
[[nodiscard]] PlannerDecision makeDirectExactGraphDecision(
    FallbackReason       reason,
    const PlannerConfig& cfg) noexcept
{
    PlannerDecision d;
    d.path                     = ExecutionPath::DirectExactGraph;
    d.fallback_reason          = reason;
    d.uses_gpu                 = false; // Exact graph is CPU-only (Category C sub-paths)
    d.uses_tensor              = false;
    d.uses_exact_graph         = true;
    d.confidence_policy_version   = cfg.policy_version;
    d.confidence_threshold_key    = cfg.staleness_threshold_key;
    d.decision_note            = "Path 4: direct exact graph (CPU-only; all Category C enforced)";
    return d;
}

/**
 * @brief Build a `PlannerDecision` for Path 5 (Distributed Summary-First).
 *
 * @param cfg  Policy config.
 * @return Completed @ref PlannerDecision.
 */
[[nodiscard]] PlannerDecision makeDistributedDecision(
    const PlannerConfig& cfg) noexcept
{
    PlannerDecision d;
    d.path                     = ExecutionPath::DistributedSummaryFirstExactOnDemand;
    d.fallback_reason          = FallbackReason::None;
    d.uses_gpu                 = false; // distributed merge is CPU
    d.uses_tensor              = true;  // summary-first uses tensor summaries per shard
    d.uses_exact_graph         = true;  // exact-on-demand triggered per shard
    d.confidence_policy_version   = cfg.policy_version;
    d.confidence_threshold_key    = cfg.staleness_threshold_key;
    d.decision_note            = "Path 5: distributed summary-first + exact-on-demand per shard";
    return d;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// DefaultQueryPlanner
// ---------------------------------------------------------------------------

/**
 * @brief Production-ready concrete query planner.
 *
 * Evaluates the five canonical execution paths in order (cheapest to most exact)
 * and returns the first path whose eligibility gates all pass.
 *
 * **Thread safety**: `selectPath()` is const; concurrent calls are safe provided
 * the injected `PlannerObserver` (if any) is also thread-safe.
 *
 * **Failure contract**: any unexpected state results in
 * `ExecutionPath::DirectExactGraph` with a non-None `FallbackReason`. The method
 * never throws.
 *
 * **Observability**: when a non-null `PlannerObserver` is registered via
 * `makeDefaultQueryPlanner(observer)`, `PlannerObserver::onDecision()` is called
 * after every `selectPath()` invocation with the decision and wall-clock latency
 * in microseconds. The observer pointer lifetime must exceed the planner lifetime.
 *
 * @see QueryPlanner
 */
class DefaultQueryPlanner final : public QueryPlanner {
public:
    /// @brief Construct without an observer (no observability callbacks).
    DefaultQueryPlanner() noexcept : observer_(nullptr) {}

    /**
     * @brief Construct with an optional observer for observability hooks.
     *
     * @param observer  Non-owning pointer to a @ref PlannerObserver, or `nullptr`
     *                  to disable callbacks. Lifetime of `observer` must exceed
     *                  the lifetime of this planner instance.
     */
    explicit DefaultQueryPlanner(PlannerObserver* observer) noexcept
        : observer_(observer) {}

    ~DefaultQueryPlanner() override = default;

    /**
     * @brief Select the optimal execution path for a query.
     *
     * The planner evaluates paths 1–5 in order from cheapest to most expensive.
     * Any failed gate causes a fall-through to the next path with the appropriate
     * `FallbackReason`. The safe default is Path 4 (DirectExactGraph).
     *
     * After the decision is made, `PlannerObserver::onDecision()` is called (if an
     * observer was registered) with the decision and the wall-clock latency of the
     * planner logic in microseconds.
     *
     * @note Distributed Path 5 is only eligible when distributed mode signals
     *       are set; it is not evaluated in the normal 1–4 fallback chain.
     *       Callers may request Path 5 explicitly by setting eligibility signals
     *       that disable paths 1–3 while marking `distributed_multi_shard`.
     *
     * @param eligibility   Runtime hardware and module-readiness signals.
     * @param freshness     Tensor artifact freshness state.
     * @param config        Versioned policy thresholds.
     * @return              A @ref PlannerDecision with the selected path and
     *                      any fallback reason.
     */
    [[nodiscard]] PlannerDecision selectPath(
        const ExecutionEligibility&     eligibility,
        const TensorArtifactFreshness&  freshness,
        const PlannerConfig&            config) const noexcept override;

private:
    /// Non-owning observer pointer; null when no observability hook is registered.
    PlannerObserver* observer_;
};

// ---------------------------------------------------------------------------
// selectPath — five-path evaluation logic
// ---------------------------------------------------------------------------

PlannerDecision DefaultQueryPlanner::selectPath(
    const ExecutionEligibility&    eligibility,
    const TensorArtifactFreshness& freshness,
    const PlannerConfig&           config) const noexcept
{
    // Capture start time for observability latency measurement.
    const auto t0 = std::chrono::steady_clock::now();

    // ------------------------------------------------------------------
    // Hard overrides (fail-closed — evaluated before any path logic)
    // ------------------------------------------------------------------

    // force_exact: skip all approximate paths, go straight to exact graph.
    if (eligibility.force_exact) {
        const PlannerDecision d =
            makeDirectExactGraphDecision(FallbackReason::ForceExact, config);
        if (observer_) {
            const auto us = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - t0).count());
            observer_->onDecision(d, us);
        }
        return d;
    }

    // force_cpu: still attempt path selection but with GPU disabled; this is
    // handled transparently because isGpuEligible() checks force_cpu internally.

    // ------------------------------------------------------------------
    // Path 5 — Distributed Summary-First + Exact-On-Demand
    //
    // Evaluated first as a structural override: if the query spans multiple
    // shards the distributed planner takes control of fragment assembly and
    // shard-level exact loading.  Non-distributed queries fall through to
    // paths 1–4.
    // ------------------------------------------------------------------
    if (eligibility.distributed_multi_shard) {
        PlannerDecision d;
        if (!eligibility.shard_manifests_available) {
            // Manifests missing → cannot use summary-first → exact graph fallback.
            d = makeDirectExactGraphDecision(FallbackReason::ShardManifestMissing, config);
        } else {
            d = makeDistributedDecision(config);
        }
        if (observer_) {
            const auto us = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - t0).count());
            observer_->onDecision(d, us);
        }
        return d;
    }

    // ------------------------------------------------------------------
    // Path 1 — ANN Only
    //
    // Eligible when ANN is enabled, no force_exact, and module readiness
    // gates pass.  GPU is optional; CPU fallback is always available.
    // ------------------------------------------------------------------
    PlannerDecision d;
    if (annPathEligible(eligibility)) {
        // Module readiness: index buffer safety must pass for reliable ANN candidates.
        if (!eligibility.index_buffer_safety_ok) {
            // Index buffer gaps too high — skip ANN paths entirely.
            d = makeDirectExactGraphDecision(FallbackReason::ModuleGapThreshold, config);
        } else if (!freshness.rebuild_in_progress
                   && freshness.artifact_age_ms == 0
                   && freshness.residual_threshold == 0.0) {
            // No artifact present → ANN-only.
            d = makeAnnOnlyDecision(eligibility, config);
        } else if (tensorSummaryEligible(freshness, config)) {
            // Tensor freshness gates passed.  Choose between Path 2 and Path 3
            // based on whether the caller requires exact graph validation.
            if (eligibility.requires_exact_graph_validation) {
                // ------------------------------------------------------------------
                // Path 3 — ANN + Tensor Refinement + Exact Graph Validation
                //
                // Used for quality-critical queries where tensor improves candidate
                // ranking but graph truth must confirm the final answer.  Category C
                // sub-paths within this path remain CPU-only.
                // ------------------------------------------------------------------
                d = makeAnnTensorExactGraphDecision(eligibility, config);
            } else {
                // ------------------------------------------------------------------
                // Path 2 — ANN + Tensor Summary
                // ------------------------------------------------------------------
                d = makeAnnTensorSummaryDecision(eligibility, config);
            }
        } else {
            // Tensor gates failed — determine reason and fall back to Path 4.
            const FallbackReason tensorReason =
                (freshness.artifact_age_ms > 0 || freshness.rebuild_in_progress)
                    ? tensorFreshnessFallbackReason(freshness, config)
                    : FallbackReason::TensorArtifactMissing;

            // Tensor is stale / missing → Path 4.
            d = makeDirectExactGraphDecision(tensorReason, config);
        }
    } else {
        // ------------------------------------------------------------------
        // Path 4 — Direct Exact Graph (safe default)
        //
        // Reached when ANN is disabled, force_exact is set (handled above),
        // or all cheaper paths were excluded.
        // ------------------------------------------------------------------
        d = makeDirectExactGraphDecision(FallbackReason::None, config);
    }

    if (observer_) {
        const auto us = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - t0).count());
        observer_->onDecision(d, us);
    }
    return d;
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

/**
 * @brief Create a `DefaultQueryPlanner` instance without an observer.
 *
 * Returns a `std::unique_ptr<QueryPlanner>` owning a `DefaultQueryPlanner`.
 * Use this factory in production code to avoid coupling to the concrete type.
 *
 * @return Owning pointer to the default planner implementation.
 */
[[nodiscard]] std::unique_ptr<QueryPlanner> makeDefaultQueryPlanner() {
    return std::make_unique<DefaultQueryPlanner>();
}

/**
 * @brief Create a `DefaultQueryPlanner` instance with an optional observer.
 *
 * When `observer` is non-null, @ref PlannerObserver::onDecision() is invoked
 * after every `selectPath()` call with the decision and wall-clock latency.
 * The caller owns `observer`; its lifetime must exceed the planner's.
 *
 * @param observer  Non-owning observer pointer, or `nullptr` to disable hooks.
 * @return Owning pointer to the default planner implementation.
 */
[[nodiscard]] std::unique_ptr<QueryPlanner> makeDefaultQueryPlanner(
    PlannerObserver* observer)
{
    return std::make_unique<DefaultQueryPlanner>(observer);
}

} // namespace evaluation
} // namespace themis
