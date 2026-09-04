/**
 * @file ablation_framework.cc
 * @brief Implementation of the ablation study framework (EPIC 2 Phase 2).
 *
 * Implements @ref AblationRunner and the @ref AblationReport helper methods
 * declared in `ablation_framework.h`.
 *
 * ## Execution model
 *
 * For each registered experiment, the runner iterates over the query batch and
 * applies the following logic per query:
 *
 * 1. Adjust the query snapshot to simulate the configured freshness variant
 *    (fresh vs stale).
 * 2. Compute retrieval quality metrics via `computeRetrievalQuality()`.
 * 3. Record the adjusted snapshot in a @ref MetricCollector.
 * 4. After all queries, aggregate via `MetricCollector::summarizeTensorGraph()`.
 *
 * Per-query `MetricError` exceptions are caught and recorded in
 * `AblationResult::per_query_errors`; they do not abort the experiment.
 *
 * ## Edge-case handling
 * - GPU requested but unavailable: experiment runs on CPU and sets
 *   `AblationResult::gpu_fallback_occurred = true`.
 * - Stale freshness variant: `snapshot.artifact_age_ms` is replaced with
 *   `config.stale_artifact_age_ms`. When `stale_artifact_age_ms` is not set
 *   (== 0), a default 10× freshness threshold is used.
 * - Unrecovered summary-first false negatives: detected per snapshot and
 *   aggregated into `AblationResult::has_unrecovered_false_negatives`.
 * - Unsafe residual: any snapshot with `residual_error > max_residual` sets
 *   `AblationResult::has_unsafe_residual = true`.
 *
 * @see include/ablation_framework.h
 * @see include/retrieval_metrics.h
 */

#include "ablation_framework.h"

#include <algorithm>
#include <stdexcept>
#include <unordered_set>

namespace themis {
namespace evaluation {

// ============================================================================
// AblationReport helpers
// ============================================================================

namespace {

/// @brief Find the result with the best value for a metric getter.
template <typename Getter>
[[nodiscard]] std::string bestResult(
    const std::vector<AblationResult>& results, Getter get)
{
    if (results.empty()) return {};
    auto it = std::max_element(results.begin(), results.end(),
        [&](const AblationResult& a, const AblationResult& b) {
            return get(a) < get(b);
        });
    return it->name;
}

/// @brief Find a result by name; return nullptr if not found.
[[nodiscard]] const AblationResult* findResult(
    const std::vector<AblationResult>& results,
    std::string_view                   name) noexcept
{
    for (const auto& r : results) {
        if (r.name == name) {
          return &r;
        }
    }
    return nullptr;
}

} // namespace

std::string AblationReport::bestByRecall() const noexcept {
    return bestResult(results, [](const AblationResult& r) {
        return r.mean_recall_at_k;
    });
}

std::string AblationReport::bestByNdcg() const noexcept {
    return bestResult(results, [](const AblationResult& r) {
        return r.mean_ndcg_at_k;
    });
}

std::string AblationReport::bestByFallbackEfficiency() const noexcept {
    // Lower exact_fallback_frequency is better → invert for max_element.
    if (results.empty()) return {};
    auto it = std::min_element(results.begin(), results.end(),
        [](const AblationResult& a, const AblationResult& b) {
            return a.tensor_graph.exact_fallback_frequency <
                   b.tensor_graph.exact_fallback_frequency;
        });
    return it->name;
}

std::optional<double> AblationReport::recallGain(
    std::string_view a, std::string_view b) const noexcept
{
    const auto* ra = findResult(results, a);
    const auto* rb = findResult(results, b);
    if (!ra || !rb) {
      return std::nullopt;
    }
    return ra->mean_recall_at_k - rb->mean_recall_at_k;
}

std::optional<double> AblationReport::ndcgGain(
    std::string_view a, std::string_view b) const noexcept
{
    const auto* ra = findResult(results, a);
    const auto* rb = findResult(results, b);
    if (!ra || !rb) {
      return std::nullopt;
    }
    return ra->mean_ndcg_at_k - rb->mean_ndcg_at_k;
}

// ============================================================================
// AblationRunner
// ============================================================================

void AblationRunner::addExperiment(std::string name, AblationConfig config) {
    if (name.empty()) {
        throw AblationError("Experiment name must not be empty");
    }
    for (const auto& e : experiments_) {
        if (e.name == name) {
            throw AblationError("Duplicate experiment name: " + name);
        }
    }
    experiments_.push_back({std::move(name), std::move(config)});
}

std::size_t AblationRunner::experimentCount() const noexcept {
    return experiments_.size();
}

void AblationRunner::reset() noexcept {
    experiments_.clear();
}

AblationReport AblationRunner::run(
    const std::vector<AblationQuery>& queries,
    double                            max_residual) const
{
    if (queries.empty()) {
        throw AblationError("Query batch must not be empty");
    }
    if (experiments_.empty()) {
        throw AblationError("No experiments registered; call addExperiment() first");
    }

    AblationReport report;
    report.results.reserve(experiments_.size());

    for (const auto& exp : experiments_) {
        report.results.push_back(runExperiment(exp, queries, max_residual));
    }

    return report;
}

AblationResult AblationRunner::runExperiment(
    const Experiment&                 exp,
    const std::vector<AblationQuery>& queries,
    double                            max_residual) const
{
    AblationResult result;
    result.name   = exp.name;
    result.config = exp.config;

    const AblationConfig& cfg = exp.config;

    // --- GPU availability check ---
    // When GPU is requested but unavailable, fall back to CPU and record deviation.
    const bool effective_gpu = cfg.compute_variant == ComputeVariant::Gpu
                               && cfg.gpu_available;
    result.gpu_fallback_occurred = (cfg.compute_variant == ComputeVariant::Gpu)
                                   && !cfg.gpu_available;

    // Determine effective max_artifact_age_ms for the freshness gate.
    constexpr uint64_t kDefaultMaxAgeMs = 5'000;
    const uint64_t effective_max_age =
        (cfg.max_artifact_age_ms > 0) ? cfg.max_artifact_age_ms : kDefaultMaxAgeMs;

    // Stale artifact age: must exceed the freshness threshold.
    const uint64_t effective_stale_age =
        (cfg.stale_artifact_age_ms > 0)
        ? cfg.stale_artifact_age_ms
        : effective_max_age * 10;

    MetricCollector collector;

    double sum_recall    = 0.0;
    double sum_precision = 0.0;
    double sum_ndcg      = 0.0;
    double sum_mrr       = 0.0;
    double sum_reduction = 0.0;

    for (const auto& q : queries) {
        // --- Build an adjusted snapshot based on the experiment configuration ---
        TensorGraphSnapshot adjusted = q.snapshot;

        // Apply freshness variant.
        switch (cfg.freshness_variant) {
            case FreshnessVariant::Fresh:
                // Clamp artifact age to be within the freshness threshold.
                if (adjusted.artifact_age_ms >= effective_max_age) {
                    adjusted.artifact_age_ms = effective_max_age / 2;
                }
                break;
            case FreshnessVariant::Stale:
                adjusted.artifact_age_ms = effective_stale_age;
                adjusted.exact_fallback_used = true;
                break;
        }

        // Apply path variant: disable tensor/graph signals for lower paths.
        if (cfg.path_variant == PathVariant::AnnOnly) {
            adjusted.graph_finalization_passed = false;
            adjusted.exact_fallback_used       = false;
            adjusted.rebuild_triggered         = false;
        } else if (cfg.path_variant == PathVariant::AnnTensor) {
            adjusted.graph_finalization_passed = false;
        }

        // Apply update variant: rebuild path sets rebuild_triggered.
        if (cfg.update_variant == UpdateVariant::Rebuild &&
            cfg.path_variant != PathVariant::AnnOnly)
        {
            adjusted.rebuild_triggered = true;
        }

        // Suppress GPU-related fallbacks when GPU is not effectively active.
        (void)effective_gpu; // GPU variant affects external kernel selection, not snapshot fields.

        // --- Check edge cases before metric computation ---

        // Edge case: summary-first false negative without exact fallback.
        if (adjusted.isUnrecoveredFalseNegative()) {
            result.has_unrecovered_false_negatives = true;
            result.per_query_errors.push_back(
                q.query_id + ": summary-first false negative without exact fallback");
        }

        // Edge case: residual too high for safe planner use.
        if (adjusted.isResidualUnsafe(max_residual)) {
            result.has_unsafe_residual = true;
            result.per_query_errors.push_back(
                q.query_id + ": residual_error=" +
                std::to_string(adjusted.residual_error) +
                " exceeds max_residual=" + std::to_string(max_residual));
        }

        // Record the adjusted snapshot.
        collector.recordSnapshot(adjusted);

        // --- Compute retrieval quality metrics ---
        try {
            const auto qm = computeRetrievalQuality(
                q.results, q.ground_truth, q.k, q.total_candidates);
            sum_recall    += qm.recall_at_k;
            sum_precision += qm.precision_at_k;
            sum_ndcg      += qm.ndcg_at_k;
            sum_mrr       += qm.mrr;
            sum_reduction += qm.candidate_reduction_ratio;
        } catch (const MetricError& e) {
            ++result.error_count;
            result.per_query_errors.push_back(
                q.query_id + ": " + e.what());
        }
    }

    result.query_count = queries.size();

    const auto valid = static_cast<double>(
        result.query_count - result.error_count);

    if (valid > 0.0) {
        result.mean_recall_at_k        = sum_recall    / valid;
        result.mean_precision_at_k     = sum_precision / valid;
        result.mean_ndcg_at_k          = sum_ndcg      / valid;
        result.mean_mrr                = sum_mrr       / valid;
        result.mean_candidate_reduction = sum_reduction / valid;
    }

    // Aggregate tensor-graph runtime metrics.
    if (collector.snapshotCount() > 0) {
        try {
            result.tensor_graph = collector.summarizeTensorGraph(max_residual);
        } catch (const MetricError& e) {
            result.per_query_errors.push_back(
                std::string("tensor-graph summary: ") + e.what());
        }
    }

    return result;
}

} // namespace evaluation
} // namespace themis
