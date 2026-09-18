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

enum class ExecutionPath : uint8_t {
    AnnOnly = 1,

    AnnTensorSummary = 2,

    AnnTensorExactGraph = 3,

    DirectExactGraph = 4,

    DistributedSummaryFirstExactOnDemand = 5,
};

// ---------------------------------------------------------------------------
// Kernel execution categories
// ---------------------------------------------------------------------------

enum class KernelCategory : uint8_t {
    A = 1,

    B = 2,

    C = 3,
};

// ---------------------------------------------------------------------------
// Fallback reason codes
// ---------------------------------------------------------------------------

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

struct TensorArtifactFreshness {
    uint64_t artifact_age_ms{0};

    uint64_t delta_lag{0};

    uint64_t source_seq_start{0};

    uint64_t source_seq_end{0};

    double residual_threshold{0.0};

    int rank_cap{0};

    bool rebuild_in_progress{false};

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

    bool requires_exact_graph_validation{false};

    // Module readiness (gap analysis thresholds)
    bool query_thread_safety_ok{false}; ///< Parallel plan optimization is safe.
    bool query_exception_handling_ok{false}; ///< Fallback logic is exception-safe.
    bool index_buffer_safety_ok{false}; ///< ANN candidate buffers are lifecycle-safe.

    // Distributed / sharding signals (Path 5)
    bool distributed_multi_shard{false};  ///< Query spans ≥ 2 shards; activate Path 5.
    bool shard_manifests_available{false};///< All required shard manifests are reachable.

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

struct PlannerDecision {
    ExecutionPath path{ExecutionPath::DirectExactGraph};

    FallbackReason fallback_reason{FallbackReason::None};

    std::string decision_note;

    std::string confidence_policy_version;

    std::string confidence_threshold_key;

    bool uses_gpu{false};

    bool uses_tensor{false};

    bool uses_exact_graph{false};
};

// ---------------------------------------------------------------------------
// Planner configuration
// ---------------------------------------------------------------------------

struct PlannerConfig {
    uint64_t max_staleness_ms{5'000};

    uint64_t max_delta_lag{1'000};

    double min_residual_threshold{0.95};

    int max_rank_cap{1'000};

    double min_shard_summary_confidence{0.80};

    std::string policy_version{"v0"};

    std::string staleness_threshold_key{"tensor.max_staleness_ms"};
};

// ---------------------------------------------------------------------------
// QueryPlanner interface
// ---------------------------------------------------------------------------

class QueryPlanner {
public:
    QueryPlanner()          = default;
    /**
     * @brief Query Planner.
     * @return Return value.
     */
    virtual ~QueryPlanner() = default;

    QueryPlanner(const QueryPlanner&)            = delete;
    QueryPlanner& operator=(const QueryPlanner&) = delete;
    QueryPlanner(QueryPlanner&&)                 = delete;
    QueryPlanner& operator=(QueryPlanner&&)      = delete;

    [[nodiscard]] virtual PlannerDecision selectPath(
        const ExecutionEligibility&     eligibility,
        const TensorArtifactFreshness&  freshness,
        const PlannerConfig&            config) const noexcept = 0;

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

class PlannerObserver {
public:
    PlannerObserver()          = default;
    /**
     * @brief Planner Observer.
     * @return Return value.
     */
    virtual ~PlannerObserver() = default;

    PlannerObserver(const PlannerObserver&)            = delete;
    PlannerObserver& operator=(const PlannerObserver&) = delete;
    PlannerObserver(PlannerObserver&&)                 = delete;
    PlannerObserver& operator=(PlannerObserver&&)      = delete;

    /**
     * @brief On Decision.
     * @param[in] decision Input parameter.
     * @param[in] latency_us Input parameter.
     * @note Exception safety: noexcept.
     */
    virtual void onDecision(
        const PlannerDecision& decision,
        uint64_t               latency_us) noexcept = 0;
};

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

[[nodiscard]] std::unique_ptr<QueryPlanner> makeDefaultQueryPlanner();

[[nodiscard]] std::unique_ptr<QueryPlanner> makeDefaultQueryPlanner(
    PlannerObserver* observer);

} // namespace evaluation
} // namespace themis
