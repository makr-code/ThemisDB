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

/**
 * @brief Retrieval path variant for the ablation study.
 *
 * Controls which retrieval layers are active during the experiment. The
 * baseline is @ref AnnOnly; each successive variant adds one layer.
 */
enum class PathVariant : uint8_t {
    AnnOnly        = 1, ///< ANN candidate generation only; no tensor or graph layer.
    AnnTensor      = 2, ///< ANN + tensor summary refinement; no graph validation.
    AnnTensorGraph = 3, ///< ANN + tensor + exact graph validation (full pipeline).
};

/**
 * @brief Tensor artifact freshness variant.
 *
 * Controls whether the tensor artifact used during the experiment is
 * within the freshness threshold (fresh) or intentionally stale.
 *
 * Only relevant when `PathVariant` includes a tensor layer.
 */
enum class FreshnessVariant : uint8_t {
    Fresh = 1, ///< Artifact is within the configured staleness threshold.
    Stale = 2, ///< Artifact exceeds the staleness threshold; exact fallback expected.
};

/**
 * @brief Tensor update path variant.
 *
 * Controls whether incremental patching or a full rebuild is used when
 * the tensor artifact is updated.
 *
 * Only relevant when `PathVariant` includes a tensor layer.
 */
enum class UpdateVariant : uint8_t {
    Patch   = 1, ///< Incremental delta patch applied to existing artifact.
    Rebuild = 2, ///< Full rebuild of the tensor artifact from scratch.
};

/**
 * @brief Compute backend variant.
 *
 * Controls whether CPU SIMD or GPU refinement is used for the vector
 * similarity computations.
 */
enum class ComputeVariant : uint8_t {
    Cpu = 1, ///< CPU SIMD path; always available as fallback.
    Gpu = 2, ///< GPU refinement path; requires CUDA device.
};

// ============================================================================
// Ablation configuration
// ============================================================================

/**
 * @brief Configuration for a single ablation experiment.
 *
 * Each field specifies one dimension of variation. Dimensions that are
 * not applicable to the chosen `path_variant` are ignored during execution
 * but must still be set to a valid enumerator to avoid undefined behavior.
 *
 * ## Validity constraints
 * - `FreshnessVariant::Stale` is only meaningful when `path_variant` includes
 *   a tensor layer. When combined with `PathVariant::AnnOnly`, the freshness
 *   field is ignored and a warning is recorded.
 * - `ComputeVariant::Gpu` requires `gpu_available == true` in the runtime
 *   context. If the GPU is not available the experiment falls back to CPU and
 *   records the deviation in the result.
 */
struct AblationConfig {
    PathVariant     path_variant     = PathVariant::AnnOnly;      ///< Retrieval path.
    FreshnessVariant freshness_variant = FreshnessVariant::Fresh; ///< Tensor freshness.
    UpdateVariant   update_variant   = UpdateVariant::Patch;      ///< Tensor update path.
    ComputeVariant  compute_variant  = ComputeVariant::Cpu;       ///< Compute backend.

    /// @brief Maximum artifact age for the fresh variant (ms). 0 uses module default.
    uint64_t max_artifact_age_ms{0};

    /// @brief Stale artifact age to inject for `FreshnessVariant::Stale` (ms).
    /// Must be > `max_artifact_age_ms` when freshness_variant == Stale.
    uint64_t stale_artifact_age_ms{0};

    /// @brief Whether a GPU is available in the runtime context for this experiment.
    bool gpu_available{false};

    /// @brief Human-readable description of this configuration (for reporting).
    std::string description;
};

// ============================================================================
// Ablation query input
// ============================================================================

/**
 * @brief A single query input for an ablation experiment.
 */
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

/**
 * @brief Aggregated results for one ablation experiment run over a query batch.
 */
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

    /// @brief True if the GPU was requested but was not available and the
    ///        experiment fell back to CPU.
    bool gpu_fallback_occurred{false};

    /// @brief True if any query in the batch produced an unrecovered
    ///        summary-first false negative (MetricErrorKind::SummaryFirstFalseNegativeNoFallback).
    bool has_unrecovered_false_negatives{false};

    /// @brief True if any snapshot had a residual error above the configured
    ///        threshold, indicating unsafe planner use.
    bool has_unsafe_residual{false};

    /// @brief Per-query error messages (empty when error_count == 0).
    std::vector<std::string> per_query_errors;
};

// ============================================================================
// Ablation report — comparison across experiments
// ============================================================================

/**
 * @brief Report comparing multiple ablation experiments.
 *
 * Produced by @ref AblationRunner::run(). Each entry in `results` corresponds
 * to one registered experiment. The `bestByRecall()`, `bestByNdcg()` helpers
 * identify the top-performing configuration across the dimensions evaluated.
 */
struct AblationReport {
    std::vector<AblationResult> results; ///< One entry per registered experiment.

    /**
     * @brief Return the name of the experiment with the highest mean Recall\@k.
     *
     * @return Experiment name, or empty string if no results are available.
     */
    [[nodiscard]] std::string bestByRecall() const noexcept;

    /**
     * @brief Return the name of the experiment with the highest mean NDCG\@k.
     *
     * @return Experiment name, or empty string if no results are available.
     */
    [[nodiscard]] std::string bestByNdcg() const noexcept;

    /**
     * @brief Return the name of the experiment with the lowest exact fallback frequency.
     *
     * Lower fallback frequency means the tensor layer is more reliable.
     *
     * @return Experiment name, or empty string if no results are available.
     */
    [[nodiscard]] std::string bestByFallbackEfficiency() const noexcept;

    /**
     * @brief Compute the recall gain of experiment `a` over experiment `b`.
     *
     * @param a  Name of the experiment to compare.
     * @param b  Name of the baseline experiment.
     * @return Recall gain (positive = a is better); nullopt when either name is not found.
     */
    [[nodiscard]] std::optional<double> recallGain(
        std::string_view a, std::string_view b) const noexcept;

    /**
     * @brief Compute the NDCG gain of experiment `a` over experiment `b`.
     *
     * @param a  Name of the experiment to compare.
     * @param b  Name of the baseline experiment.
     * @return NDCG gain; nullopt when either name is not found.
     */
    [[nodiscard]] std::optional<double> ndcgGain(
        std::string_view a, std::string_view b) const noexcept;
};

// ============================================================================
// AblationError
// ============================================================================

/**
 * @brief Exception thrown when an ablation experiment configuration is invalid
 *        or cannot be executed.
 *
 * @throws AblationError from @ref AblationRunner::run() on config violations.
 */
class AblationError : public std::runtime_error {
public:
    /**
     * @brief Construct an AblationError with a message.
     *
     * @param what  Human-readable description of the error.
     */
    explicit AblationError(std::string_view what)
        : std::runtime_error(std::string{what}) {}
};

// ============================================================================
// AblationRunner
// ============================================================================

/**
 * @brief Orchestrates and runs ablation experiments over a query batch.
 *
 * Experiments are registered with a name and configuration, then executed
 * in registration order over the same query batch. Metric computation errors
 * for individual queries are recorded in `AblationResult::per_query_errors`
 * rather than aborting the run; the caller can inspect `error_count` to
 * determine whether the results are reliable.
 *
 * **NOT thread-safe.** External synchronization is required when sharing
 * across threads.
 */
class AblationRunner {
public:
    AblationRunner() = default;

    // Non-copyable, movable.
    AblationRunner(const AblationRunner&)            = delete;
    AblationRunner& operator=(const AblationRunner&) = delete;
    AblationRunner(AblationRunner&&)                 = default;
    AblationRunner& operator=(AblationRunner&&)      = default;

    /**
     * @brief Register an ablation experiment.
     *
     * @param name    Unique human-readable name for the experiment.
     * @param config  Experiment configuration.
     *
     * @throws AblationError when `name` is empty or already registered.
     */
    void addExperiment(std::string name, AblationConfig config);

    /**
     * @brief Return the number of registered experiments.
     *
     * @return Experiment count.
     */
    [[nodiscard]] std::size_t experimentCount() const noexcept;

    /**
     * @brief Execute all registered experiments over the given query batch.
     *
     * Each query is evaluated for every registered experiment using the
     * @ref computeRetrievalQuality and @ref computeTensorGraphRuntimeMetrics
     * functions. Per-query MetricErrors are captured and do not abort the run.
     *
     * @param queries        Batch of queries to evaluate. Must not be empty.
     * @param max_residual   Residual threshold for the tensor-graph metrics summary.
     *
     * @return @ref AblationReport with one result per registered experiment.
     *
     * @throws AblationError when `queries` is empty or no experiments have been
     *         registered.
     */
    [[nodiscard]] AblationReport run(
        const std::vector<AblationQuery>& queries,
        double                            max_residual = 0.10) const;

    /**
     * @brief Clear all registered experiments.
     */
    void reset() noexcept;

private:
    struct Experiment {
        std::string    name;
        AblationConfig config;
    };

    std::vector<Experiment> experiments_;

    /// @brief Execute a single experiment over the query batch.
    [[nodiscard]] AblationResult runExperiment(
        const Experiment&                  exp,
        const std::vector<AblationQuery>&  queries,
        double                             max_residual) const;
};

} // namespace evaluation
} // namespace themis
