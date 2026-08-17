/**
 * @file distributed_rag_evaluator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "rag/rag_judge.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <vector>

namespace themis::rag::distributed {

/**
 * @brief Strategy for aggregating results from multiple judges.
 */
enum class AggregationStrategy {
    MEAN,             ///< Simple arithmetic mean of all dimension scores
    WEIGHTED_MEAN,    ///< Weighted mean using per-judge confidence weights
    MAJORITY_VOTING,  ///< Binary pass/fail per dimension; majority wins
    BEST_OF_N,        ///< Return the result with the highest overall_score
};

/**
 * @brief Per-judge configuration entry.
 */
struct JudgeWorkerConfig {
    /// Human-readable identifier for this judge (e.g. model name).
    std::string judge_id;

    /// Evaluation configuration forwarded to the RAGJudge.
    judge::RAGJudgeConfig judge_config;

    /// Weight used by WEIGHTED_MEAN aggregation (default 1.0).
    double weight = 1.0;
};

/**
 * @brief Configuration for DistributedRAGEvaluator.
 */
struct DistributedEvaluatorConfig {
    /// Maximum number of judge tasks to run in parallel.
    /// Defaults to the number of configured judges (full parallelism).
    size_t max_parallel_judges = 0;

    /// Aggregation strategy for combining per-judge results.
    AggregationStrategy aggregation = AggregationStrategy::WEIGHTED_MEAN;

    /// Timeout for a single judge evaluation.  Zero means no timeout.
    std::chrono::milliseconds per_judge_timeout{0};

    /// If true, a judge that times out or throws is skipped rather than
    /// causing evaluate() to throw.
    bool skip_failed_judges = true;

    /// Minimum number of successful judge responses required.
    /// If fewer judges respond successfully, evaluate() throws std::runtime_error.
    size_t min_successful_judges = 1;
};

/**
 * @brief Metadata emitted alongside the aggregated result.
 */
struct DistributedEvaluationMeta {
    /// Number of judges that completed successfully.
    size_t successful_judges = 0;

    /// Number of judges that failed (timeout / exception).
    size_t failed_judges = 0;

    /// Per-judge individual results (order matches JudgeWorkerConfig order).
    std::vector<judge::EvaluationResult> individual_results;

    /// Inter-judge agreement score [0, 1] on the overall dimension.
    double inter_judge_agreement = 0.0;

    /// Wall-clock time for the entire distributed evaluation round.
    std::chrono::milliseconds total_elapsed{0};
};

/**
 * @brief Distributed RAG evaluator (Issue: #2245).
 *
 * Distributes a single EvaluationInput to all configured judges, runs them in
 * parallel, and aggregates the results.
 *
 * Example usage:
 * @code
 *   DistributedEvaluatorConfig dist_cfg;
 *   dist_cfg.aggregation = AggregationStrategy::WEIGHTED_MEAN;
 *
 *   std::vector<JudgeWorkerConfig> workers;
 *   workers.push_back({"judge-fast",     judge::RAGJudgeFactory::createFast()->getConfig(),    0.5});
 *   workers.push_back({"judge-thorough", judge::RAGJudgeFactory::createThorough()->getConfig(), 1.5});
 *
 *   DistributedRAGEvaluator evaluator(workers, dist_cfg);
 *   auto [result, meta] = evaluator.evaluate(input);
 * @endcode
 */
class DistributedRAGEvaluator {
public:
    /**
     * @brief Construct evaluator with worker specifications and config.
     * @param workers       Per-judge configuration list (at least one required).
     * @param config        Global distribution configuration.
     * @throws std::invalid_argument if workers is empty.
     */
    explicit DistributedRAGEvaluator(
        std::vector<JudgeWorkerConfig>      workers,
        const DistributedEvaluatorConfig&   config = DistributedEvaluatorConfig{}
    );

    ~DistributedRAGEvaluator();

    // Non-copyable; movable.
    DistributedRAGEvaluator(const DistributedRAGEvaluator&)            = delete;
    DistributedRAGEvaluator& operator=(const DistributedRAGEvaluator&) = delete;
    DistributedRAGEvaluator(DistributedRAGEvaluator&&)                 noexcept = default;
    DistributedRAGEvaluator& operator=(DistributedRAGEvaluator&&)      noexcept = default;

    /**
     * @brief Evaluate a single input across all judges.
     * @param input  Evaluation context (query + documents + generated answer).
     * @return Pair of aggregated result and distribution metadata.
     * @throws std::runtime_error if fewer than min_successful_judges respond.
     */
    std::pair<judge::EvaluationResult, DistributedEvaluationMeta>
    evaluate(const judge::EvaluationInput& input);

    /**
     * @brief Batch evaluate multiple inputs across all judges.
     * @param inputs  Vector of evaluation inputs.
     * @return Vector of (aggregated result, metadata) pairs, one per input.
     */
    std::vector<std::pair<judge::EvaluationResult, DistributedEvaluationMeta>>
    batchEvaluate(const std::vector<judge::EvaluationInput>& inputs);

    /**
     * @brief Set (or replace) the aggregation strategy at runtime.
     */
    void setAggregationStrategy(AggregationStrategy strategy);

    /**
     * @brief Return the current configuration.
     */
    DistributedEvaluatorConfig getConfig() const;

    /**
     * @brief Return the number of configured judge workers.
     */
    size_t judgeCount() const;

    /**
     * @brief Total evaluations completed since construction.
     */
    uint64_t totalEvaluations() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    /// Aggregate a vector of per-judge results using the configured strategy.
    judge::EvaluationResult aggregateResults(
        const std::vector<judge::EvaluationResult>& results,
        const std::vector<double>&                  weights,
        AggregationStrategy                         strategy
    ) const;

    /// Compute inter-judge agreement on overall_score in [0, 1].
    static double computeAgreement(
        const std::vector<judge::EvaluationResult>& results
    );
};

/**
 * @brief Factory helpers for common distributed-evaluator setups.
 */
class DistributedEvaluatorFactory {
public:
    /**
     * @brief Create an evaluator with N identical judges running in parallel.
     * @param judge_count   Number of parallel judge workers (>= 1).
     * @param mode          Evaluation mode forwarded to each RAGJudge.
     * @param aggregation   How to combine the N results.
     */
    static std::unique_ptr<DistributedRAGEvaluator> createHomogeneous(
        size_t                   judge_count,
        judge::EvaluationMode    mode        = judge::EvaluationMode::BALANCED,
        AggregationStrategy      aggregation = AggregationStrategy::MEAN
    );

    /**
     * @brief Create an evaluator with one fast and one thorough judge.
     *
     * The fast judge receives weight 0.4, the thorough judge weight 0.6 in
     * WEIGHTED_MEAN aggregation.
     */
    static std::unique_ptr<DistributedRAGEvaluator> createFastThorough();
};

} // namespace themis::rag::distributed
