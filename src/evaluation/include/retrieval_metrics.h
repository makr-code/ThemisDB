/**
 * @file retrieval_metrics.h
 * @brief Evaluation metrics for the layered hybrid retrieval architecture.
 *
 * Defines the typed contract for collecting, computing, and reporting
 * retrieval quality metrics across all layers of the ThemisDB retrieval stack:
 * ANN, tensor mid-layer, graph validation, LLM/LoRA grounded generation, and
 * distributed cross-shard execution.
 *
 * ## Metric domains
 *
 * 1. **Retrieval quality** — Recall\@k, Precision\@k, NDCG, MRR, candidate
 *    reduction effectiveness.
 * 2. **Evidence quality** — coverage, completeness, relevance, multi-hop
 *    support.
 * 3. **Provenance quality** — fidelity, source traceability, trust correctness.
 * 4. **Compression / tensor** — compression ratio, approximation loss,
 *    redundancy elimination.
 * 5. **LLM answer quality** — faithfulness, groundedness, hallucination rate,
 *    prompt token cost, answer support density.
 * 6. **Distributed efficiency** — cross-shard requests, bytes, summary-first
 *    selectivity.
 * 7. **Tensor-graph runtime** — artifact freshness, delta lag, residual /
 *    approximation error, rank growth, rebuild frequency, exact fallback
 *    frequency, summary-first false-negative rate, graph-verified finalization
 *    pass rate.
 *
 * ## Design constraints
 * - All metric structs are value types; no heap ownership or virtual dispatch.
 * - Computation functions are stateless and `[[nodiscard]]`; results are never
 *   silently discarded.
 * - Invalid inputs (NaN, negative counts, empty ground-truth sets) are detected
 *   and reported via @ref MetricError; no silent numeric failure.
 * - Thread safety: individual metric value types are immutable after construction.
 *   @ref MetricCollector is NOT thread-safe; callers must synchronize externally.
 *
 * @see EVALUATION_FRAMEWORK.md
 * @see docs/EPIC2_QUERY_PLANNER.md
 * @see src/evaluation/include/query_planner.h
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace evaluation {

// ============================================================================
// Error reporting
// ============================================================================

/**
 * @brief Category of metric computation error.
 *
 * Used by @ref MetricError to convey the root cause without relying on
 * exception type hierarchies.
 */
enum class MetricErrorKind : uint8_t {
    /// Ground-truth set is empty; metric is undefined.
    EmptyGroundTruth = 1,
    /// k <= 0 or k larger than candidate set; metric is undefined.
    InvalidK = 2,
    /// Input contains NaN or ±Inf values.
    NonFiniteInput = 3,
    /// Negative count or probability out of [0, 1].
    InvalidRange = 4,
    /// Duplicate entries detected in a set where uniqueness is required.
    DuplicateEntries = 5,
    /// Missing ground-truth labels required for provenance evaluation.
    MissingGroundTruthLabels = 6,
    /// Double-counted items detected (same result counted in multiple strata).
    DoubleCountedItems = 7,
    /// Summary-first false negative occurred without an exact fallback being
    /// triggered, making the recall metric potentially understated.
    SummaryFirstFalseNegativeNoFallback = 8,
    /// Residual value is too high to be safely used by the planner without
    /// an exact fallback guard.
    ResidualTooHighForPlanner = 9,
};

/**
 * @brief Exception thrown when metric computation detects an invalid input
 *        or a logically inconsistent metric state.
 *
 * @throws MetricError on precondition violations in any `compute*` function.
 */
class MetricError : public std::runtime_error {
public:
    /**
     * @brief Construct a MetricError with a kind code and description.
     *
     * @param kind  Root-cause category.
     * @param what  Human-readable description including parameter values.
     */
    explicit MetricError(MetricErrorKind kind, std::string_view what)
        : std::runtime_error(std::string{what}), kind_(kind) {}

    /// @brief Return the root-cause category.
    [[nodiscard]] MetricErrorKind kind() const noexcept { return kind_; }

private:
    MetricErrorKind kind_;
};

// ============================================================================
// § 1  Retrieval quality metrics
// ============================================================================

/**
 * @brief A single ranked result item with a unique document identifier.
 *
 * Used as input to Recall\@k, Precision\@k, NDCG, and MRR computations.
 * The `id` field must be unique within a single result list; duplicates
 * cause a `MetricErrorKind::DuplicateEntries` error.
 */
struct RankedResult {
    std::string id;      ///< Unique document / node identifier.
    double      score;   ///< Relevance score (higher is better).
};

/**
 * @brief Computed retrieval quality metrics for a single query.
 *
 * All values are in [0.0, 1.0] unless noted otherwise.
 */
struct RetrievalQualityMetrics {
    double recall_at_k{0.0};              ///< Recall\@k: fraction of ground-truth items in top-k.
    double precision_at_k{0.0};           ///< Precision\@k: fraction of top-k items that are relevant.
    double ndcg_at_k{0.0};                ///< Normalized Discounted Cumulative Gain\@k.
    double mrr{0.0};                      ///< Mean Reciprocal Rank of the first relevant result.
    double candidate_reduction_ratio{0.0};///< (1 – top_k / total_candidates); 0 when total == 0.
};

/**
 * @brief Compute retrieval quality metrics for a single query result list.
 *
 * @param ranked      Ordered list of results returned by the retrieval system.
 *                    Must not be empty when `k > 0`. Duplicate `id` values are
 *                    rejected with `MetricErrorKind::DuplicateEntries`.
 * @param ground_truth Set of relevant document ids for this query.
 *                    Must not be empty (`MetricErrorKind::EmptyGroundTruth`).
 * @param k           Cutoff depth. Must be in [1,static_cast<int>(ranked.size())];
 *                    `MetricErrorKind::InvalidK` otherwise.
 * @param total_candidates Total candidate pool size before ranking (used to
 *                    compute `candidate_reduction_ratio`). Pass 0 to skip.
 *
 * @return Computed @ref RetrievalQualityMetrics for this query.
 *
 * @throws MetricError on any precondition violation.
 */
[[nodiscard]] RetrievalQualityMetrics computeRetrievalQuality(
    const std::vector<RankedResult>& ranked,
    const std::vector<std::string>&  ground_truth,
    std::size_t                      k,
    std::size_t                      total_candidates = 0);

// ============================================================================
// § 2  Evidence quality metrics
// ============================================================================

/**
 * @brief Computed evidence quality metrics for a single query result.
 */
struct EvidenceQualityMetrics {
    double coverage_rate{0.0};        ///< Fraction of required evidence facts covered.
    double evidence_precision{0.0};   ///< Fraction of returned evidence that is relevant.
    double multi_hop_support{0.0};    ///< Multi-hop evidence support score in [0, 1].
};

/**
 * @brief Compute evidence quality metrics.
 *
 * @param returned_evidence_ids  Ids of evidence items returned by the system.
 * @param required_evidence_ids  Ids of evidence items required for a complete answer.
 *                               Must not be empty (`MetricErrorKind::EmptyGroundTruth`).
 * @param hop_chain_lengths      Number of hops in each returned multi-hop chain.
 *                               Pass an empty vector when multi-hop is not applicable.
 *
 * @return Computed @ref EvidenceQualityMetrics.
 *
 * @throws MetricError on empty required_evidence_ids or invalid input.
 */
[[nodiscard]] EvidenceQualityMetrics computeEvidenceQuality(
    const std::vector<std::string>& returned_evidence_ids,
    const std::vector<std::string>& required_evidence_ids,
    const std::vector<int>&         hop_chain_lengths = {});

// ============================================================================
// § 3  Provenance quality metrics
// ============================================================================

/**
 * @brief A single provenance attribution assertion.
 */
struct ProvenanceAssertion {
    std::string claim_id;   ///< Claim being attributed.
    std::string source_id;  ///< Attributed source document or node.
    double      confidence; ///< Attribution confidence in [0, 1].
};

/**
 * @brief Computed provenance quality metrics.
 */
struct ProvenanceQualityMetrics {
    double fidelity_score{0.0};             ///< Fraction of attributions matching ground-truth.
    double source_attribution_completeness{0.0}; ///< Fraction of claims with a verifiable source.
    double trust_signal_correctness{0.0};   ///< Fraction of trust signals correctly assigned.
};

/**
 * @brief Compute provenance quality metrics.
 *
 * @param returned  Provenance attributions returned by the system.
 * @param ground_truth Correct attributions for comparison.
 *                  Must not be empty (`MetricErrorKind::MissingGroundTruthLabels`).
 *
 * @return Computed @ref ProvenanceQualityMetrics.
 *
 * @throws MetricError on empty ground_truth or non-finite confidence values.
 */
[[nodiscard]] ProvenanceQualityMetrics computeProvenanceQuality(
    const std::vector<ProvenanceAssertion>& returned,
    const std::vector<ProvenanceAssertion>& ground_truth);

// ============================================================================
// § 4  Compression / tensor metrics
// ============================================================================

/**
 * @brief Computed tensor summary compression and quality metrics.
 */
struct CompressionMetrics {
    double compression_ratio{0.0};      ///< original_size / compressed_size; ≥ 1.0 or 0 when uncompressed.
    double approximation_loss{0.0};     ///< Mean squared error between original and reconstructed vectors.
    double redundancy_elimination{0.0}; ///< Fraction of redundant information removed.
    double residual_error{0.0};         ///< Residual approximation error; lower is better.
    double rank_growth_rate{0.0};       ///< Rate of rank increase per snapshot epoch (0 = stable).
};

/**
 * @brief Compute compression and tensor summary quality metrics.
 *
 * @param original_size_bytes    Size of the original (uncompressed) tensor in bytes. Must be > 0.
 * @param compressed_size_bytes  Size after compression. Must be in (0, original_size_bytes].
 * @param approximation_errors   Per-vector MSE values from reconstruction; must be finite.
 * @param rank_samples           Rank values observed across epochs (for rank growth rate).
 *
 * @return Computed @ref CompressionMetrics.
 *
 * @throws MetricError on invalid sizes, non-finite errors, or empty error vector.
 */
[[nodiscard]] CompressionMetrics computeCompressionMetrics(
    std::size_t                   original_size_bytes,
    std::size_t                   compressed_size_bytes,
    const std::vector<double>&    approximation_errors,
    const std::vector<int>&       rank_samples = {});

// ============================================================================
// § 5  LLM answer quality metrics
// ============================================================================

/**
 * @brief Computed LLM/LoRA grounded-generation quality metrics.
 */
struct LlmAnswerQualityMetrics {
    double faithfulness_score{0.0};      ///< Fraction of answer claims supported by retrieved evidence.
    double hallucination_rate{0.0};      ///< Fraction of answer claims unsupported by evidence (in [0, 1]).
    double groundedness_score{0.0};      ///< Composite grounding quality in [0, 1].
    double answer_support_density{0.0};  ///< Evidence-to-answer token ratio (higher = better supported).
    uint32_t prompt_token_count{0};      ///< Total prompt tokens consumed; 0 when unavailable.
};

/**
 * @brief Compute LLM answer quality metrics.
 *
 * @param supported_claims    Number of answer claims grounded in retrieved evidence. Must be ≥ 0.
 * @param total_claims        Total number of answer claims. Must be > 0.
 * @param evidence_tokens     Token count of the evidence provided to the LLM.
 * @param prompt_token_count  Total prompt token count (0 if unavailable).
 *
 * @return Computed @ref LlmAnswerQualityMetrics.
 *
 * @throws MetricError when total_claims == 0 or counts are inconsistent.
 */
[[nodiscard]] LlmAnswerQualityMetrics computeLlmAnswerQuality(
    uint32_t supported_claims,
    uint32_t total_claims,
    uint32_t evidence_tokens    = 0,
    uint32_t prompt_token_count = 0);

// ============================================================================
// § 6  Distributed / shard efficiency metrics
// ============================================================================

/**
 * @brief Computed distributed execution efficiency metrics.
 */
struct DistributedEfficiencyMetrics {
    double   shard_fan_out{0.0};              ///< Average number of shards contacted per query.
    double   bytes_per_query{0.0};            ///< Average bytes transferred across shards per query.
    double   summary_first_selectivity{0.0};  ///< Fraction of shards skipped by summary-first routing.
    double   selective_exact_load{0.0};       ///< Fraction of exact loads triggered by selective fallback.
    uint32_t total_queries{0};                ///< Total queries evaluated.
};

/**
 * @brief Compute distributed efficiency metrics over a query batch.
 *
 * @param per_query_shard_counts  Number of shards contacted for each query. Must not be empty.
 * @param per_query_bytes         Bytes transferred for each query (same length as shard_counts).
 * @param summary_skipped_shards  Per-query count of shards skipped by summary-first routing.
 * @param total_shards            Total available shards. Must be > 0.
 *
 * @return Computed @ref DistributedEfficiencyMetrics.
 *
 * @throws MetricError on empty input, length mismatch, or invalid total_shards.
 */
[[nodiscard]] DistributedEfficiencyMetrics computeDistributedEfficiency(
    const std::vector<uint32_t>& per_query_shard_counts,
    const std::vector<double>&   per_query_bytes,
    const std::vector<uint32_t>& summary_skipped_shards,
    uint32_t                     total_shards);

// ============================================================================
// § 7  Tensor-graph runtime metrics
// ============================================================================

/**
 * @brief Runtime health metrics for the tensor and graph retrieval layers.
 *
 * These metrics are collected continuously during query processing and
 * aggregated across a reporting window. They reflect the freshness and
 * reliability of the tensor mid-layer and graph validation layer.
 */
struct TensorGraphRuntimeMetrics {
    /// Mean artifact age across all queries in the window (ms).
    double mean_artifact_age_ms{0.0};

    /// Mean delta lag (unprocessed log entries) across all queries.
    double mean_delta_lag{0.0};

    /// Mean residual / approximation error (lower is better; 0.0 is perfect).
    double mean_residual_error{0.0};

    /// Fraction of queries where the tensor rank exceeded the configured cap.
    double rank_growth_fraction{0.0};

    /// Fraction of time windows in which a tensor rebuild was triggered.
    double rebuild_frequency{0.0};

    /// Fraction of queries that fell back to exact retrieval (graph or direct).
    double exact_fallback_frequency{0.0};

    /// Fraction of summary-first routed queries where a relevant result was
    /// missed (false negative) — including cases without an exact fallback.
    double summary_first_false_negative_rate{0.0};

    /// Fraction of ANN+Tensor+Graph queries that completed the graph-verified
    /// finalization pass (as opposed to falling back before the graph stage).
    double graph_verified_finalization_pass_rate{0.0};
};

/**
 * @brief Input snapshot for computing tensor-graph runtime metrics over a window.
 *
 * One snapshot is produced per query or per periodic sample.
 */
struct TensorGraphSnapshot {
    uint64_t artifact_age_ms{0};          ///< Artifact age at query time (ms).
    uint64_t delta_lag{0};                ///< Unprocessed delta log entries.
    double   residual_error{0.0};         ///< Residual approximation error for this sample.
    int      rank_cap_used{0};            ///< Effective rank cap applied.
    int      rank_cap_limit{0};           ///< Configured rank cap limit.
    bool     rebuild_triggered{false};    ///< True if a rebuild was triggered in this window.
    bool     exact_fallback_used{false};  ///< True if exact retrieval was used instead of tensor.
    bool     summary_first_false_negative{false}; ///< True if summary-first missed a relevant result.
    bool     graph_finalization_passed{false};     ///< True if graph-verified finalization completed.
    bool     summary_first_routing_used{false};    ///< True if this query used summary-first routing.

    /**
     * @brief Check the summary-first false-negative edge case.
     *
     * Returns true when `summary_first_false_negative` is true but
     * `exact_fallback_used` is false — indicating the false negative was NOT
     * recovered, which makes recalled\@k potentially understated.
     *
     * @return True when an unrecovered summary-first false negative is present.
     */
    [[nodiscard]] bool isUnrecoveredFalseNegative() const noexcept {
        return summary_first_false_negative && !exact_fallback_used;
    }

    /**
     * @brief Check whether the residual error is safe for planner use.
     *
     * A residual error > @p max_residual_error indicates approximation quality
     * is too low for the planner to rely on tensor summaries safely.
     *
     * @param max_residual_error  Maximum acceptable residual error threshold.
     * @return True when residual error exceeds the threshold.
     */
    [[nodiscard]] bool isResidualUnsafe(double max_residual_error) const noexcept {
        return residual_error > max_residual_error;
    }
};

/**
 * @brief Compute aggregated tensor-graph runtime metrics over a window of snapshots.
 *
 * @param snapshots           Per-query or per-period snapshots. Must not be empty.
 * @param max_residual_error  Maximum acceptable residual per snapshot.
 *                            Any snapshot above this threshold causes an error.
 *
 * @return Aggregated @ref TensorGraphRuntimeMetrics over the window.
 *
 * @throws MetricError when `snapshots` is empty.
 * @throws MetricError with kind @ref MetricErrorKind::ResidualTooHighForPlanner
 *         when a snapshot residual exceeds `max_residual_error`.
 */
[[nodiscard]] TensorGraphRuntimeMetrics computeTensorGraphRuntimeMetrics(
    const std::vector<TensorGraphSnapshot>& snapshots,
    double                                  max_residual_error = 0.10);

// ============================================================================
// § 8  MetricCollector — aggregating collector across queries
// ============================================================================

/**
 * @brief Aggregating collector for all metric domains across a query batch.
 *
 * Accumulates per-query snapshots, then computes aggregate metrics via
 * `summarize()`. NOT thread-safe; external synchronization is required when
 * used from multiple threads.
 *
 * @note Intended for offline evaluation and CI-level batch reporting, not for
 *       per-request hot paths.
 */
class MetricCollector {
public:
    MetricCollector() = default;

    // Non-copyable but movable (snapshots vector may be large).
    MetricCollector(const MetricCollector&)            = delete;
    MetricCollector& operator=(const MetricCollector&) = delete;
    MetricCollector(MetricCollector&&)                 = default;
    MetricCollector& operator=(MetricCollector&&)      = default;

    /**
     * @brief Record a tensor-graph snapshot for the current query.
     *
     * @param snapshot  Snapshot to add to the collection window.
     */
    void recordSnapshot(TensorGraphSnapshot snapshot);

    /**
     * @brief Record per-query shard efficiency data.
     *
     * @param shard_count  Number of shards contacted.
     * @param bytes        Bytes transferred.
     * @param skipped      Shards skipped by summary-first routing.
     */
    void recordShardQuery(uint32_t shard_count, double bytes, uint32_t skipped);

    /**
     * @brief Return the number of snapshots collected.
     *
     * @return Snapshot count.
     */
    [[nodiscard]] std::size_t snapshotCount() const noexcept;

    /**
     * @brief Compute aggregated tensor-graph runtime metrics over all collected snapshots.
     *
     * @param max_residual_error  Maximum acceptable residual per snapshot.
     * @return Aggregated @ref TensorGraphRuntimeMetrics.
     *
     * @throws MetricError when no snapshots have been recorded or a snapshot
     *         exceeds `max_residual_error`.
     */
    [[nodiscard]] TensorGraphRuntimeMetrics summarizeTensorGraph(
        double max_residual_error = 0.10) const;

    /**
     * @brief Compute aggregated distributed efficiency metrics.
     *
     * @param total_shards  Total available shards.
     * @return Aggregated @ref DistributedEfficiencyMetrics.
     *
     * @throws MetricError when no shard queries have been recorded or total_shards == 0.
     */
    [[nodiscard]] DistributedEfficiencyMetrics summarizeDistributed(
        uint32_t total_shards) const;

    /// @brief Reset all accumulated state.
    void reset() noexcept;

private:
    std::vector<TensorGraphSnapshot> snapshots_;
    std::vector<uint32_t>            shard_counts_;
    std::vector<double>              shard_bytes_;
    std::vector<uint32_t>            shard_skipped_;
};

} // namespace evaluation
} // namespace themis
