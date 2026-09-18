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

enum class MetricErrorKind : uint8_t {
    EmptyGroundTruth = 1,
    InvalidK = 2,
    NonFiniteInput = 3,
    InvalidRange = 4,
    DuplicateEntries = 5,
    MissingGroundTruthLabels = 6,
    DoubleCountedItems = 7,
    SummaryFirstFalseNegativeNoFallback = 8,
    ResidualTooHighForPlanner = 9,
};

class MetricError : public std::runtime_error {
public:
    /**
     * @brief Metric Error.
     * @param[in] kind Input parameter.
     * @param[in] what Input parameter.
     * @return Return value.
     */
    explicit MetricError(MetricErrorKind kind, std::string_view what)
        : std::runtime_error(std::string{what}), kind_(kind) {}

    [[nodiscard]] MetricErrorKind kind() const noexcept { return kind_; }

private:
    MetricErrorKind kind_;
};

// ============================================================================
// § 1  Retrieval quality metrics
// ============================================================================

struct RankedResult {
    std::string id;      ///< Unique document / node identifier.
    double      score;   ///< Relevance score (higher is better).
};

struct RetrievalQualityMetrics {
    double recall_at_k{0.0};              ///< Recall\@k: fraction of ground-truth items in top-k.
    double precision_at_k{0.0};           ///< Precision\@k: fraction of top-k items that are relevant.
    double ndcg_at_k{0.0};                ///< Normalized Discounted Cumulative Gain\@k.
    double mrr{0.0};                      ///< Mean Reciprocal Rank of the first relevant result.
    double candidate_reduction_ratio{0.0};///< (1 – top_k / total_candidates); 0 when total == 0.
};

[[nodiscard]] RetrievalQualityMetrics computeRetrievalQuality(
    const std::vector<RankedResult>& ranked,
    const std::vector<std::string>&  ground_truth,
    std::size_t                      k,
    std::size_t                      total_candidates = 0);

// ============================================================================
// § 2  Evidence quality metrics
// ============================================================================

struct EvidenceQualityMetrics {
    double coverage_rate{0.0};        ///< Fraction of required evidence facts covered.
    double evidence_precision{0.0};   ///< Fraction of returned evidence that is relevant.
    double multi_hop_support{0.0};    ///< Multi-hop evidence support score in [0, 1].
};

[[nodiscard]] EvidenceQualityMetrics computeEvidenceQuality(
    const std::vector<std::string>& returned_evidence_ids,
    const std::vector<std::string>& required_evidence_ids,
    const std::vector<int>&         hop_chain_lengths = {});

// ============================================================================
// § 3  Provenance quality metrics
// ============================================================================

struct ProvenanceAssertion {
    std::string claim_id;   ///< Claim being attributed.
    std::string source_id;  ///< Attributed source document or node.
    double      confidence; ///< Attribution confidence in [0, 1].
};

struct ProvenanceQualityMetrics {
    double fidelity_score{0.0};             ///< Fraction of attributions matching ground-truth.
    double source_attribution_completeness{0.0}; ///< Fraction of claims with a verifiable source.
    double trust_signal_correctness{0.0};   ///< Fraction of trust signals correctly assigned.
};

[[nodiscard]] ProvenanceQualityMetrics computeProvenanceQuality(
    const std::vector<ProvenanceAssertion>& returned,
    const std::vector<ProvenanceAssertion>& ground_truth);

// ============================================================================
// § 4  Compression / tensor metrics
// ============================================================================

struct CompressionMetrics {
    double compression_ratio{0.0};      ///< original_size / compressed_size; ≥ 1.0 or 0 when uncompressed.
    double approximation_loss{0.0};     ///< Mean squared error between original and reconstructed vectors.
    double redundancy_elimination{0.0}; ///< Fraction of redundant information removed.
    double residual_error{0.0};         ///< Residual approximation error; lower is better.
    double rank_growth_rate{0.0};       ///< Rate of rank increase per snapshot epoch (0 = stable).
};

[[nodiscard]] CompressionMetrics computeCompressionMetrics(
    std::size_t                   original_size_bytes,
    std::size_t                   compressed_size_bytes,
    const std::vector<double>&    approximation_errors,
    const std::vector<int>&       rank_samples = {});

// ============================================================================
// § 5  LLM answer quality metrics
// ============================================================================

struct LlmAnswerQualityMetrics {
    double faithfulness_score{0.0};      ///< Fraction of answer claims supported by retrieved evidence.
    double hallucination_rate{0.0};      ///< Fraction of answer claims unsupported by evidence (in [0, 1]).
    double groundedness_score{0.0};      ///< Composite grounding quality in [0, 1].
    double answer_support_density{0.0};  ///< Evidence-to-answer token ratio (higher = better supported).
    uint32_t prompt_token_count{0};      ///< Total prompt tokens consumed; 0 when unavailable.
};

[[nodiscard]] LlmAnswerQualityMetrics computeLlmAnswerQuality(
    uint32_t supported_claims,
    uint32_t total_claims,
    uint32_t evidence_tokens    = 0,
    uint32_t prompt_token_count = 0);

// ============================================================================
// § 6  Distributed / shard efficiency metrics
// ============================================================================

struct DistributedEfficiencyMetrics {
    double   shard_fan_out{0.0};              ///< Average number of shards contacted per query.
    double   bytes_per_query{0.0};            ///< Average bytes transferred across shards per query.
    double   summary_first_selectivity{0.0};  ///< Fraction of shards skipped by summary-first routing.
    double   selective_exact_load{0.0};       ///< Fraction of exact loads triggered by selective fallback.
    uint32_t total_queries{0};                ///< Total queries evaluated.
};

[[nodiscard]] DistributedEfficiencyMetrics computeDistributedEfficiency(
    const std::vector<uint32_t>& per_query_shard_counts,
    const std::vector<double>&   per_query_bytes,
    const std::vector<uint32_t>& summary_skipped_shards,
    uint32_t                     total_shards);

// ============================================================================
// § 7  Tensor-graph runtime metrics
// ============================================================================

struct TensorGraphRuntimeMetrics {
    double mean_artifact_age_ms{0.0};

    double mean_delta_lag{0.0};

    double mean_residual_error{0.0};

    double rank_growth_fraction{0.0};

    double rebuild_frequency{0.0};

    double exact_fallback_frequency{0.0};

    double summary_first_false_negative_rate{0.0};

    double graph_verified_finalization_pass_rate{0.0};
};

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

    [[nodiscard]] bool isUnrecoveredFalseNegative() const noexcept {
        return summary_first_false_negative && !exact_fallback_used;
    }

    [[nodiscard]] bool isResidualUnsafe(double max_residual_error) const noexcept {
        return residual_error > max_residual_error;
    }
};

[[nodiscard]] TensorGraphRuntimeMetrics computeTensorGraphRuntimeMetrics(
    const std::vector<TensorGraphSnapshot>& snapshots,
    double                                  max_residual_error = 0.10);

// ============================================================================
// § 8  MetricCollector — aggregating collector across queries
// ============================================================================

class MetricCollector {
public:
    MetricCollector() = default;

    // Non-copyable but movable (snapshots vector may be large).
    MetricCollector(const MetricCollector&)            = delete;
    MetricCollector& operator=(const MetricCollector&) = delete;
    MetricCollector(MetricCollector&&)                 = default;
    MetricCollector& operator=(MetricCollector&&)      = default;

    /**
     * @brief Record Snapshot.
     * @param[in] snapshot Input parameter.
     */
    void recordSnapshot(TensorGraphSnapshot snapshot);

    /**
     * @brief Record Shard Query.
     * @param[in] shard_count Input parameter.
     * @param[in] bytes Input parameter.
     * @param[in] skipped Input parameter.
     */
    void recordShardQuery(uint32_t shard_count, double bytes, uint32_t skipped);

    [[nodiscard]] std::size_t snapshotCount() const noexcept;

    [[nodiscard]] TensorGraphRuntimeMetrics summarizeTensorGraph(
        double max_residual_error = 0.10) const;

    [[nodiscard]] DistributedEfficiencyMetrics summarizeDistributed(
        uint32_t total_shards) const;

    /**
     * @brief Reset the modification detection flag.
     * @note Exception safety: noexcept.
     */
    void reset() noexcept;

private:
    std::vector<TensorGraphSnapshot> snapshots_;
    std::vector<uint32_t>            shard_counts_;
    std::vector<double>              shard_bytes_;
    std::vector<uint32_t>            shard_skipped_;
};

} // namespace evaluation
} // namespace themis
