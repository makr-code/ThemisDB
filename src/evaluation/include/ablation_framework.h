/**
 * @file ablation_framework.h
 * @brief Ablation study API for the layered hybrid retrieval architecture.
 *
 * Defines the typed contract for running and comparing ablation experiments
 * across the ThemisDB retrieval stack. An ablation study systematically
 * disables or varies one architectural layer at a time to quantify its
 * contribution to overall retrieval quality.
 *
 * ## Supported ablation dimensions
 *
 * 1. **Retrieval path** — ANN-only vs ANN+Tensor vs ANN+Tensor+Graph.
 * 2. **Tensor freshness** — fresh vs stale tensor summaries.
 * 3. **Update path** — patch path vs full rebuild path.
 * 4. **Compute backend** — CPU SIMD vs GPU refinement.
 *
 * ## Usage pattern
 *
 * ```cpp
 * AblationConfig cfg;
 * cfg.path_variant = PathVariant::AnnTensorGraph;
 * cfg.freshness_variant = FreshnessVariant::Fresh;
 * cfg.update_variant = UpdateVariant::Patch;
 * cfg.compute_variant = ComputeVariant::Gpu;
 *
 * AblationRunner runner;
 * runner.addExperiment("ann+tensor+graph/fresh", cfg);
 *
 * cfg.path_variant = PathVariant::AnnOnly;
 * runner.addExperiment("ann-only/baseline", cfg);
 *
 * AblationReport report = runner.run(query_batch);
 * ```
 *
 * ## Error contract
 *
 * @ref AblationError is thrown when an experiment configuration is
 * inconsistent, when the query batch is empty, or when metrics cannot be
 * computed due to missing ground-truth data.
 *
 * ## Thread safety
 *
 * @ref AblationRunner is NOT thread-safe. Experiments are registered and
 * executed sequentially. Callers must synchronize externally when sharing
 * a runner across threads.
 *
 * @see src/evaluation/include/retrieval_metrics.h
 * @see EVALUATION_FRAMEWORK.md
 * @see docs/EPIC2_QUERY_PLANNER.md
 */

#pragma once

#include "retrieval_metrics.h"

#include <functional>
#include <string>
#include <vector>

namespace themis {
namespace evaluation {

// ============================================================================
// Ablation dimension enumerations
// ============================================================================

enum class PathVariant : uint8_t {
    AnnOnly        = 1, ///< ANN candidate generation only; no tensor or graph layer.
    AnnTensor      = 2, ///< ANN + tensor summary refinement; no graph validation.
    AnnTensorGraph = 3, ///< ANN + tensor + exact graph validation (full pipeline).
};

enum class FreshnessVariant : uint8_t {
    Fresh = 1, ///< Artifact is within the configured staleness threshold.
    Stale = 2, ///< Artifact exceeds the staleness threshold; exact fallback expected.
};

enum class UpdateVariant : uint8_t {
    Patch   = 1, ///< Incremental delta patch applied to existing artifact.
    Rebuild = 2, ///< Full rebuild of the tensor artifact from scratch.
};

enum class ComputeVariant : uint8_t {
    Cpu = 1, ///< CPU SIMD path; always available as fallback.
    Gpu = 2, ///< GPU refinement path; requires CUDA device.
};

// ============================================================================
// Ablation configuration
// ============================================================================

struct AblationConfig {
    PathVariant     path_variant     = PathVariant::AnnOnly;      ///< Retrieval path.
    FreshnessVariant freshness_variant = FreshnessVariant::Fresh; ///< Tensor freshness.
    UpdateVariant   update_variant   = UpdateVariant::Patch;      ///< Tensor update path.
    ComputeVariant  compute_variant  = ComputeVariant::Cpu;       ///< Compute backend.

    uint64_t max_artifact_age_ms{0};

    uint64_t stale_artifact_age_ms{0};

    bool gpu_available{false};

    std::string description;
};

// ============================================================================
// Ablation query input
// ============================================================================

struct AblationQuery {
    std::string                  query_id;      ///< Unique query identifier.
    std::vector<RankedResult>    results;        ///< Ranked results returned by the retrieval system.
    std::vector<std::string>     ground_truth;  ///< Relevant document ids for recall/precision.
    std::size_t                  k{10};         ///< Cutoff depth for quality metrics.
    std::size_t                  total_candidates{0}; ///< Candidate pool size (0 to skip reduction ratio).
    TensorGraphSnapshot          snapshot;      ///< Runtime snapshot for tensor-graph metrics.
};

// ============================================================================
// Ablation result for a single experiment
// ============================================================================

struct AblationResult {
    std::string name;            ///< Experiment name as registered with @ref AblationRunner.
    AblationConfig config;       ///< Configuration used for this experiment.

    // --- Aggregate metric means over all queries ---
    double mean_recall_at_k{0.0};        ///< Mean Recall\@k across all queries.
    double mean_precision_at_k{0.0};     ///< Mean Precision\@k across all queries.
    double mean_ndcg_at_k{0.0};          ///< Mean NDCG\@k across all queries.
    double mean_mrr{0.0};                ///< Mean MRR across all queries.
    double mean_candidate_reduction{0.0};///< Mean candidate reduction ratio.

    TensorGraphRuntimeMetrics tensor_graph; ///< Aggregated tensor-graph runtime metrics.

    std::size_t query_count{0};      ///< Number of queries evaluated.
    std::size_t error_count{0};      ///< Number of queries that produced a MetricError.

    bool gpu_fallback_occurred{false};

    bool has_unrecovered_false_negatives{false};

    bool has_unsafe_residual{false};

    std::vector<std::string> per_query_errors;
};

// ============================================================================
// Ablation report — comparison across experiments
// ============================================================================

struct AblationReport {
    std::vector<AblationResult> results; ///< One entry per registered experiment.

    [[nodiscard]] std::string bestByRecall() const noexcept;

    [[nodiscard]] std::string bestByNdcg() const noexcept;

    [[nodiscard]] std::string bestByFallbackEfficiency() const noexcept;

    [[nodiscard]] std::optional<double> recallGain(
        std::string_view a, std::string_view b) const noexcept;

    [[nodiscard]] std::optional<double> ndcgGain(
        std::string_view a, std::string_view b) const noexcept;
};

// ============================================================================
// AblationError
// ============================================================================

class AblationError : public std::runtime_error {
public:
    /**
     * @brief Ablation Error.
     * @param[in] what Input parameter.
     * @return Return value.
     */
    explicit AblationError(std::string_view what)
        : std::runtime_error(std::string{what}) {}
};

// ============================================================================
// AblationRunner
// ============================================================================

class AblationRunner {
public:
    AblationRunner() = default;

    // Non-copyable, movable.
    AblationRunner(const AblationRunner&)            = delete;
    AblationRunner& operator=(const AblationRunner&) = delete;
    AblationRunner(AblationRunner&&)                 = default;
    AblationRunner& operator=(AblationRunner&&)      = default;

    /**
     * @brief Add Experiment.
     * @param[in] name Input parameter.
     * @param[in] config Input parameter.
     */
    void addExperiment(std::string name, AblationConfig config);

    [[nodiscard]] std::size_t experimentCount() const noexcept;

    [[nodiscard]] AblationReport run(
        const std::vector<AblationQuery>& queries,
        double                            max_residual = 0.10) const;

    /**
     * @brief Reset the modification detection flag.
     * @note Exception safety: noexcept.
     */
    void reset() noexcept;

private:
    struct Experiment {
        std::string    name;
        AblationConfig config;
    };

    std::vector<Experiment> experiments_;

    [[nodiscard]] AblationResult runExperiment(
        const Experiment&                  exp,
        const std::vector<AblationQuery>&  queries,
        double                             max_residual) const;
};

} // namespace evaluation
} // namespace themis
