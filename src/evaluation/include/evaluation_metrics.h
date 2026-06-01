/**
 * @file evaluation_metrics.h
 * @brief Evaluation metrics framework and ablation support.
 *
 * Provides a unified interface for recording, aggregating, and reporting
 * quality metrics (Recall@k, Evidence Quality, Provenance Score, Latency,
 * LLM Answer Quality) across all retrieval layers.
 *
 * Planned in: docs/EPIC2_EVALUATION_METRICS.md
 * Sub-issue:   #5439
 */

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::evaluation {

/// Named metric dimension.
enum class MetricDimension {
    RecallAtK,
    PrecisionAtK,
    MeanReciprocalRank,
    NormalisedDCG,
    EvidenceQuality,
    ProvenanceScore,
    CompressionRatio,
    LatencyP50Ms,
    LatencyP99Ms,
    LlmAnswerScore,
};

/// A single metric observation.
struct MetricObservation {
    MetricDimension dimension;
    double          value;
    std::string     experiment_id; ///< Ablation group label (empty = baseline)
    std::uint64_t   sample_index  = 0;
};

/// Aggregated metric report for one dimension and experiment.
struct MetricReport {
    MetricDimension dimension;
    std::string     experiment_id;
    double          mean   = 0.0;
    double          stddev = 0.0;
    double          p50    = 0.0;
    double          p99    = 0.0;
    std::uint64_t   n      = 0;
};

/// Ablation configuration: one variant definition.
struct AblationVariant {
    std::string id;          ///< Unique variant label
    std::string description;
    std::unordered_map<std::string, std::string> params; ///< Key-value overrides
};

/**
 * @brief Evaluation metrics collector and ablation runner.
 */
class IEvaluationMetrics {
public:
    virtual ~IEvaluationMetrics() = default;

    /// Record a single metric observation.
    virtual void record(const MetricObservation& obs) = 0;

    /// Compute aggregate report for a dimension / experiment pair.
    virtual MetricReport report(MetricDimension dim,
                                 const std::string& experiment_id = "") const = 0;

    /// Return all dimension reports for a given experiment.
    virtual std::vector<MetricReport> reportAll(
        const std::string& experiment_id = "") const = 0;

    /// Register an ablation variant.
    virtual void registerVariant(AblationVariant variant) = 0;

    /// Return all registered variant IDs.
    virtual std::vector<std::string> listVariants() const = 0;

    /// Reset all collected observations (useful between benchmark runs).
    virtual void reset() = 0;
};

/// Factory: create an in-memory metrics collector.
std::unique_ptr<IEvaluationMetrics> makeEvaluationMetrics();

} // namespace themis::evaluation
