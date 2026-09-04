/**
 * @file query_planner.h
 * @brief Hybrid query planner contract for ANN, tensor, graph, and distributed
 *        retrieval flows with explicit CPU/GPU execution-boundary enforcement.
 *
 * This header defines the typed contract for EPIC 2.5 Query Planner as described in
 * `docs/EPIC2_QUERY_PLANNER.md` and ratified by ADR E2-003. It encodes:
 *   - The five canonical execution paths (ANN-only → distributed exact-on-demand).
 *   - Kernel category eligibility checks (Category A / B / C).
 *   - Tensor artifact freshness evaluation.
 *   - Fallback reason codes and fail-closed enforcement rules.
 *   - Observability hooks via @ref PlannerObserver for latency and fallback monitoring.
 *
 * Design constraints:
 *   - Tensor artifacts are ADVISORY ONLY and are never treated as final truth.
 *   - Category C operations (policy, provenance, transactions) are CPU-only;
 *     no GPU dispatch is permitted and violations are fail-closed.
 *   - Every fallback decision carries a machine-readable @ref FallbackReason;
 *     silent fallback is forbidden (ADR E2-005).
 *   - Observer callbacks are invoked after every @ref selectPath() call;
 *     they must not throw and must not call back into the planner (no re-entrancy).
 *
 * @note Status: Phases 1-5 complete (contract, implementation, error handling,
 *       tests, observability). Phase 6-7 documentation and integration complete.
 *
 * @see docs/EPIC2_QUERY_PLANNER.md
 * @see docs/adr/adr-e2-003-query-planner-routing-model.md
 * @see docs/adr/adr-e2-005-cross-layer-fallback-confidence-policy.md
 * @see ai_working/KERNEL_CLASSIFICATION_REVIEW.md
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace themis {
namespace evaluation {

// ---------------------------------------------------------------------------
// Execution path identifiers
// ---------------------------------------------------------------------------

/**
 * @brief The five canonical planner execution paths, ordered from most approximate
 *        (lowest cost) to most exact (highest cost).
 *
 * The planner selects the lowest-cost eligible path for each request and falls back
 * to a higher-cost path whenever a gate is not satisfied.
 */
enum class ExecutionPath : uint8_t {
    /**
     * @brief ANN candidate generation only (GPU Category A eligible).
     *
     * Condition: non-truth-bearing retrieval, no tensor or graph validation required.
     * GPU gate: Category A error-handling threshold satisfied (50 % gap fix).
     */
    AnnOnly = 1,

    /**
     * @brief ANN candidate generation followed by tensor summary lookup and optional
     *        CPU/GPU refinement.
     *
     * Condition: tensor artifact is fresh (age < max_staleness_ms), residual ≥ 0.95,
     * rebuild not in progress. Result is advisory — never final truth.
     * GPU gate for optional matmul: Category A/B parity validated.
     */
    AnnTensorSummary = 2,

    /**
     * @brief ANN candidate generation, tensor refinement, and exact graph validation.
     *
     * Condition: quality-critical query where tensor improves ranking but graph truth
     * must confirm the final answer. Category C sub-paths (ACL, provenance, transaction)
     * are CPU-only within this path.
     */
    AnnTensorExactGraph = 3,

    /**
     * @brief Direct exact graph traversal, bypassing ANN and tensor stages.
     *
     * Condition: tensor artifact is absent, stale, or below quality threshold; or
     * `force_exact` is set; or a bounded kernel failed parity verification.
     * All Category C operations use CPU only.
     */
    DirectExactGraph = 4,

    /**
     * @brief Distributed summary-first retrieval across shards with exact graph loading
     *        per shard on demand when summary confidence falls below threshold.
     *
     * Condition: query spans ≥ 2 shards. Exact-on-demand triggers per shard when
     * shard summary confidence < policy threshold. Cross-shard FK validation is
     * enforced during 2PC prepare phase for graph-consistent queries.
     */
    DistributedSummaryFirstExactOnDemand = 5,
};

// ---------------------------------------------------------------------------
// Kernel execution categories
// ---------------------------------------------------------------------------

/**
 * @brief Kernel execution category, reflecting the classification from
 *        `ai_working/KERNEL_CLASSIFICATION_REVIEW.md` and ADR E2-003.
 */
enum class KernelCategory : uint8_t {
    /**
     * @brief Acceleration-eligible, advisory only.
     *
     * ANN distance kernels (L2/Cosine/IP), TopK selection, Vec KNN insert,
     * tensor-core matmul. GPU dispatch allowed after the 50–60 % error-handling
     * gap fix gate is satisfied. Output is advisory; CPU validation is mandatory.
     */
    A = 1,

    /**
     * @brief Bounded / conditional GPU eligibility with strict gates.
     *
     * Geo distance/containment (valid WGS84 coords required), graph BFS
     * (≤ 3 hops, ≤ 10 000 frontier), graph Dijkstra (≤ 1 000 vertex pairs,
     * non-negative edge weights). GPU result is advisory; CPU parity check
     * is mandatory before any truth-bearing use.
     */
    B = 2,

    /**
     * @brief CPU-only. GPU dispatch is never permitted for this category.
     *
     * Policy-aware traversal (ACL enforcement), provenance chain construction
     * (audit trail), transaction consistency verification (ACID / MVCC).
     * Violation is fail-closed — the request is rejected, not downgraded.
     */
    C = 3,
};

// ---------------------------------------------------------------------------
// Fallback reason codes
// ---------------------------------------------------------------------------

/**
 * @brief Machine-readable reason codes for fallback decisions.
 *
 * Every fallback must carry one of these codes. Silent fallback is forbidden
 * (ADR E2-005).
 */
enum class FallbackReason : uint16_t {
    None = 0,

    // GPU / kernel failures
    GpuKernelError          = 100, ///< GPU kernel returned an error code.
    GpuKernelTimeout        = 101, ///< Kernel exceeded 5-second SLA.
    GpuOutputValidation     = 102, ///< Output size, range, or CPU parity check failed.
    GpuParityCheckFailed    = 103, ///< GPU result did not match CPU verification.
    GpuUnavailable          = 104, ///< CUDA device not available.

    // Bound violations (Category B)
    BfsFrontierExceeded     = 200, ///< BFS frontier > 10 000 nodes.
    BfsHopsExceeded         = 201, ///< BFS depth > 3 hops.
    DijkstraPairsExceeded   = 202, ///< Dijkstra vertex pair count > 1 000.
    GeoCoordInvalid         = 203, ///< Input coordinates failed WGS84 validation.
    GeoOutputRangeInvalid   = 204, ///< Output distances or containment flags out of range.

    // Tensor artifact staleness / quality
    TensorArtifactStale     = 300, ///< artifact_age_ms >= max_staleness_ms.
    TensorArtifactMissing   = 301, ///< No tensor artifact available.
    TensorRebuildInProgress = 302, ///< rebuild_in_progress == true.
    TensorResidualLow       = 303, ///< residual_threshold < configured minimum.
    TensorRankCapExceeded   = 304, ///< rank_cap outside policy range.
    TensorSeqIncompatible   = 305, ///< source_seq_range incompatible with current snapshot.

    // Module readiness / gap analysis
    ModuleGapThreshold      = 400, ///< Module gap severity above the safe threshold.

    // Policy / category enforcement
    CategoryCSubpathDetected = 500, ///< ACL / provenance / transaction path detected.
    ForceExact              = 501, ///< force_exact flag set on query context.
    ForceCpu                = 502, ///< force_cpu flag set on query context.

    // Distributed / shard
    ShardSummaryLowConfidence = 600, ///< Shard summary confidence below threshold.
    ShardManifestMissing      = 601, ///< Shard manifest unavailable.
};

// ---------------------------------------------------------------------------
// Tensor artifact freshness
// ---------------------------------------------------------------------------

/**
 * @brief Planner-facing representation of tensor artifact freshness state.
 *
 * Values are sourced from the artifact manifest
 * (`src/distributed_tensor/src/manifest_store.cc`). The planner evaluates
 * freshness before choosing any tensor-based execution path (Path 2 or 3).
 *
 * @note Tensor artifacts are ADVISORY ONLY. A fresh artifact does not grant
 *       truth-bearing semantics. Graph validation is always required for
 *       truth-bearing results.
 */
struct TensorArtifactFreshness {
    /// Time elapsed since the artifact was last rebuilt, in milliseconds.
    uint64_t artifact_age_ms{0};

    /// Number of unprocessed log entries since the last snapshot (index drift indicator).
    uint64_t delta_lag{0};

    /// Sequence number of the first snapshot covered by this artifact.
    uint64_t source_seq_start{0};

    /// Sequence number of the most recent snapshot covered by this artifact.
    uint64_t source_seq_end{0};

    /// Quality metric in [0.0, 1.0]. Values below 0.95 trigger exact fallback.
    double residual_threshold{0.0};

    /// Maximum rank considered during index construction.
    int rank_cap{0};

    /// True if a snapshot rebuild is currently in progress.
    bool rebuild_in_progress{false};

    /**
     * @brief Evaluate whether the artifact is fresh enough to use for refinement.
     *
     * An artifact is considered fresh when:
     *   - `artifact_age_ms` < @p max_age_ms
     *   - `residual_threshold` >= @p min_residual (default 0.95)
     *   - `rebuild_in_progress` is false
     *
     * @param max_age_ms    Maximum acceptable artifact age in milliseconds.
     * @param min_residual  Minimum acceptable quality metric. Defaults to 0.95.
     * @return True if the artifact passes all freshness gates; false otherwise.
     */
    [[nodiscard]] bool isFresh(
        uint64_t max_age_ms,
        double   min_residual = 0.95) const noexcept
    {
        if (rebuild_in_progress) {
          return false;
        }
        if (artifact_age_ms >= max_age_ms) {
          return false;
        }
        if (residual_threshold < min_residual) {
          return false;
        }
        return true;
    }

    /**
     * @brief Determine the fallback reason if the artifact is not fresh.
     *
     * @param max_age_ms   Maximum acceptable artifact age in milliseconds.
     * @param min_residual Minimum acceptable quality metric. Defaults to 0.95.
     * @return The most specific @ref FallbackReason for the failed gate, or
     *         FallbackReason::None if the artifact is fresh.
     */
    [[nodiscard]] FallbackReason staleness_reason(
        uint64_t max_age_ms,
        double   min_residual = 0.95) const noexcept
    {
        if (rebuild_in_progress) {
          return FallbackReason::TensorRebuildInProgress;
        }
        if (artifact_age_ms >= max_age_ms) {
          return FallbackReason::TensorArtifactStale;
        }
        if (residual_threshold < min_residual) {
          return FallbackReason::TensorResidualLow;
        }
        return FallbackReason::None;
    }
};

// ---------------------------------------------------------------------------
// Execution eligibility signals
// ---------------------------------------------------------------------------

/**
 * @brief Runtime signals used by the planner to evaluate execution eligibility.
 *
 * These values are collected from the hardware profile, query context, module
 * readiness checks, and gap analysis thresholds. The planner uses them to select
 * an @ref ExecutionPath and to block GPU dispatch when conditions are unsafe.
 */
struct ExecutionEligibility {
    // Hardware signals
    bool cuda_available{false};         ///< CUDA device detected and initialized.

    // ANN path gates
    bool ann_enabled{false};            ///< ANN retrieval enabled for this query.
    bool gpu_error_handling_gate{false};///< Category A gate: 50 % CUDA error-handling gaps fixed.

    // Tensor / bounded kernel gates
    bool gpu_parity_validated{false};   ///< Category B gate: GPU/CPU parity tests pass.

    // Policy overrides
    bool force_exact{false};            ///< Override: always use exact graph (Path 4).
    bool force_cpu{false};              ///< Override: disable all GPU paths.

    /// Quality-critical flag: when true and tensor freshness gates pass, the planner
    /// selects Path 3 (ANN + Tensor Refinement + Exact Graph) instead of Path 2
    /// (ANN + Tensor Summary). Use for queries where tensor improves ranking but
    /// graph truth must confirm the final answer (e.g., security-sensitive lookups).
    bool requires_exact_graph_validation{false};

    // Module readiness (gap analysis thresholds)
    bool query_thread_safety_ok{false}; ///< Parallel plan optimization is safe.
    bool query_exception_handling_ok{false}; ///< Fallback logic is exception-safe.
    bool index_buffer_safety_ok{false}; ///< ANN candidate buffers are lifecycle-safe.

    // Distributed / sharding signals (Path 5)
    bool distributed_multi_shard{false};  ///< Query spans ≥ 2 shards; activate Path 5.
    bool shard_manifests_available{false};///< All required shard manifests are reachable.

    /**
     * @brief Check whether a given kernel category is eligible for GPU dispatch.
     *
     * @param category  The kernel category to check.
     * @return True if GPU dispatch is permitted; false if CPU fallback is required.
     *
     * Category C always returns false. Category A requires `cuda_available` and
     * `gpu_error_handling_gate`. Category B additionally requires `gpu_parity_validated`.
     * `force_cpu` overrides all categories to false.
     */
    [[nodiscard]] bool isGpuEligible(KernelCategory category) const noexcept {
        if (force_cpu) {
          return false;
        }
        switch (category) {
            case KernelCategory::C:
                return false; // Never GPU
            case KernelCategory::A:
                return cuda_available && gpu_error_handling_gate;
            case KernelCategory::B:
                return cuda_available && gpu_error_handling_gate && gpu_parity_validated;
        }
        return false;
    }
};

// ---------------------------------------------------------------------------
// Planner decision output
// ---------------------------------------------------------------------------

/**
 * @brief The result of a planner path-selection decision.
 *
 * Returned by @ref QueryPlanner::selectPath(). Consumers must inspect
 * `fallback_reason` to understand why the selected path differs from the
 * cheapest eligible path.
 */
struct PlannerDecision {
    /// The selected execution path.
    ExecutionPath path{ExecutionPath::DirectExactGraph};

    /// Machine-readable reason for any fallback from a cheaper path.
    /// FallbackReason::None when the cheapest eligible path was selected.
    FallbackReason fallback_reason{FallbackReason::None};

    /// Human-readable description of the decision (for logging / observability).
    std::string decision_note;

    /// Confidence policy version that governed the fallback thresholds (ADR E2-005).
    std::string confidence_policy_version;

    /// Key within the confidence policy that determined the freshness threshold.
    std::string confidence_threshold_key;

    /// True when any GPU dispatch is planned for this path.
    bool uses_gpu{false};

    /// True when the tensor refinement stage is included in this path.
    bool uses_tensor{false};

    /// True when exact graph validation is included in this path.
    bool uses_exact_graph{false};
};

// ---------------------------------------------------------------------------
// Planner configuration
// ---------------------------------------------------------------------------

/**
 * @brief Policy parameters that govern planner thresholds.
 *
 * Values are versioned and must be attached to decisions for provenance tracing
 * (ADR E2-005 confidence contract). The caller is responsible for loading these
 * from the policy store and keeping them up to date.
 */
struct PlannerConfig {
    /// Maximum tensor artifact age before staleness fallback triggers (ms).
    uint64_t max_staleness_ms{5'000};

    /// Maximum acceptable delta lag before exact fallback is forced.
    uint64_t max_delta_lag{1'000};

    /// Minimum tensor residual quality metric; below this triggers exact fallback.
    double min_residual_threshold{0.95};

    /// Maximum rank cap allowed for tensor refinement; exceeded triggers exact fallback.
    int max_rank_cap{1'000};

    /// Minimum shard summary confidence; below this triggers per-shard exact-on-demand.
    double min_shard_summary_confidence{0.80};

    /// Policy version string attached to all decisions for provenance tracing.
    std::string policy_version{"v0"};

    /// Staleness threshold key within the policy store.
    std::string staleness_threshold_key{"tensor.max_staleness_ms"};
};

// ---------------------------------------------------------------------------
// QueryPlanner interface
// ---------------------------------------------------------------------------

/**
 * @brief Hybrid query planner interface.
 *
 * Implementations select the optimal @ref ExecutionPath for each request given
 * the current @ref ExecutionEligibility signals, @ref TensorArtifactFreshness
 * state, and @ref PlannerConfig policy. The selected path and any fallback
 * reason are returned in a @ref PlannerDecision.
 *
 * **Thread safety**: implementations must be thread-safe for concurrent calls
 * to `selectPath()`. Internal state mutation (if any) must be protected by
 * appropriate synchronization.
 *
 * **Failure contract**: if path selection itself fails (e.g., config unavailable),
 * the implementation must return @ref ExecutionPath::DirectExactGraph with a
 * non-None fallback reason. It must not throw from `selectPath()`.
 */
class QueryPlanner {
public:
    QueryPlanner()          = default;
    virtual ~QueryPlanner() = default;

    QueryPlanner(const QueryPlanner&)            = delete;
    QueryPlanner& operator=(const QueryPlanner&) = delete;
    QueryPlanner(QueryPlanner&&)                 = delete;
    QueryPlanner& operator=(QueryPlanner&&)      = delete;

    /**
     * @brief Select the optimal execution path for a query.
     *
     * The planner evaluates the five canonical paths in order from cheapest to
     * most expensive and returns the first path whose eligibility gates all pass.
     * If no cheaper path is eligible, @ref ExecutionPath::DirectExactGraph is
     * returned as the safe default.
     *
     * @param eligibility   Runtime signals for hardware and module readiness.
     * @param freshness     Tensor artifact freshness state. Pass a default-constructed
     *                      value when no tensor artifact is available.
     * @param config        Policy configuration governing thresholds and versioning.
     * @return              A @ref PlannerDecision describing the selected path,
     *                      any fallback reason, and GPU / tensor / graph usage flags.
     *
     * @note This function must not throw. On any internal error, return a decision
     *       with path = DirectExactGraph and an appropriate FallbackReason.
     */
    [[nodiscard]] virtual PlannerDecision selectPath(
        const ExecutionEligibility&     eligibility,
        const TensorArtifactFreshness&  freshness,
        const PlannerConfig&            config) const noexcept = 0;

    /**
     * @brief Check whether a specific kernel category is eligible for GPU dispatch
     *        given the current eligibility signals.
     *
     * A convenience wrapper around @ref ExecutionEligibility::isGpuEligible().
     * Category C always returns false. The result is advisory; callers must still
     * enforce CPU-only semantics for Category C operations at the call site.
     *
     * @param category      The kernel category to check.
     * @param eligibility   Runtime eligibility signals.
     * @return True if GPU dispatch is permitted; false if CPU fallback is required.
     */
    [[nodiscard]] static bool isKernelEligibleForGpu(
        KernelCategory              category,
        const ExecutionEligibility& eligibility) noexcept
    {
        return eligibility.isGpuEligible(category);
    }
};

// ---------------------------------------------------------------------------
// Observability — PlannerObserver interface (Phase 5)
// ---------------------------------------------------------------------------

/**
 * @brief Observability hook invoked after every @ref QueryPlanner::selectPath() call.
 *
 * Implementations receive the completed @ref PlannerDecision and the wall-clock
 * latency of the planner decision in microseconds. Hooks are intended for metrics
 * export (Prometheus counters, latency histograms), structured logging, and test
 * assertion.
 *
 * **Contract**:
 * - @ref onDecision() must not throw (marked `noexcept`).
 * - @ref onDecision() must not call back into the planner that issued the notification
 *   (re-entrancy is forbidden and leads to undefined behaviour).
 * - The `PlannerDecision` reference is valid only for the duration of the call.
 * - Implementations are responsible for their own thread-safety if the planner is
 *   called concurrently from multiple threads.
 *
 * **Module gap threshold monitoring**:
 * When `decision.fallback_reason == FallbackReason::ModuleGapThreshold` the observer
 * should increment a dedicated counter so operators can track how often the ANN path
 * is blocked by insufficient gap-fix coverage.
 */
class PlannerObserver {
public:
    PlannerObserver()          = default;
    virtual ~PlannerObserver() = default;

    PlannerObserver(const PlannerObserver&)            = delete;
    PlannerObserver& operator=(const PlannerObserver&) = delete;
    PlannerObserver(PlannerObserver&&)                 = delete;
    PlannerObserver& operator=(PlannerObserver&&)      = delete;

    /**
     * @brief Called after every path-selection decision.
     *
     * @param decision    The completed planner decision (path, reason, flags).
     * @param latency_us  Wall-clock time spent inside @ref selectPath(), in microseconds.
     *                    This covers only the planner logic, not the downstream retrieval.
     */
    virtual void onDecision(
        const PlannerDecision& decision,
        uint64_t               latency_us) noexcept = 0;
};

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

/**
 * @brief Create the default production query planner without an observer.
 *
 * Returns an owning pointer to the `DefaultQueryPlanner` implementation.
 * No observability callbacks will be fired. Equivalent to
 * `makeDefaultQueryPlanner(nullptr)`.
 *
 * @return `std::unique_ptr<QueryPlanner>` owning a `DefaultQueryPlanner`.
 */
[[nodiscard]] std::unique_ptr<QueryPlanner> makeDefaultQueryPlanner();

/**
 * @brief Create the default production query planner with an optional observer.
 *
 * When `observer` is non-null, @ref PlannerObserver::onDecision() is called after
 * every @ref QueryPlanner::selectPath() invocation. The caller retains ownership of
 * `observer`; the planner stores only a raw pointer and the lifetime of the observer
 * must exceed the lifetime of the returned planner.
 *
 * @param observer  Non-owning pointer to an observer, or `nullptr` to disable hooks.
 * @return `std::unique_ptr<QueryPlanner>` owning a `DefaultQueryPlanner`.
 */
[[nodiscard]] std::unique_ptr<QueryPlanner> makeDefaultQueryPlanner(
    PlannerObserver* observer);

} // namespace evaluation
} // namespace themis
