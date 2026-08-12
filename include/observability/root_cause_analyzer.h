/**
 * @file root_cause_analyzer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "observability/performance_analyzer.h"

namespace themis {
namespace observability {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// TimeSeries – a named sequence of (timestamp, value) observations
// ---------------------------------------------------------------------------

/**
 * @brief A single data point in a time series.
 */
struct TimeSeriesPoint {
    std::chrono::system_clock::time_point timestamp;
    double value{0.0};
};

/**
 * @brief A named metric time series.
 */
struct TimeSeries {
    std::string name;
    std::string unit;
    std::vector<TimeSeriesPoint> points;

    /// Returns the change between the last and first value, as a percentage.
    double change_percent() const;

    /// Returns the average value across all points.
    double mean() const;
};

// ---------------------------------------------------------------------------
// SystemSnapshot – a point-in-time capture of key system metrics
// ---------------------------------------------------------------------------

/**
 * @brief Captures the state of key performance metrics at a point in time.
 *
 * Used to compare "before" and "after" states when diagnosing a
 * performance regression.
 */
struct SystemSnapshot {
    std::chrono::system_clock::time_point captured_at;

    // Storage metrics
    double write_amplification{0.0};
    double read_amplification{0.0};
    double compaction_rate_mb_s{0.0};
    double memtable_flush_rate_mb_s{0.0};
    double sstable_count{0.0};

    // Cache metrics
    double block_cache_hit_rate_pct{0.0};
    double row_cache_hit_rate_pct{0.0};

    // Query metrics
    double avg_query_latency_ms{0.0};
    double p99_query_latency_ms{0.0};
    double queries_per_second{0.0};

    // Resource metrics
    double cpu_usage_pct{0.0};
    double memory_usage_mb{0.0};
    double disk_io_utilization_pct{0.0};

    /// Arbitrary extra metrics not covered by the named fields above.
    std::map<std::string, double> extra_metrics;

    json toJSON() const;
};

// ---------------------------------------------------------------------------
// CorrelatedMetric – a metric correlated with a target metric
// ---------------------------------------------------------------------------

/**
 * @brief Describes the correlation between a target metric and another metric.
 */
struct CorrelatedMetric {
    std::string metric_name;
    /// Pearson correlation coefficient in [-1, 1].
    double correlation_coefficient{0.0};
    /// Approximate lag in seconds (positive = metric lags the target).
    double lag_seconds{0.0};
    /// Human-readable description of the relationship.
    std::string description;
};

// ---------------------------------------------------------------------------
// CausalGraph – a directed acyclic graph of causal relationships
// ---------------------------------------------------------------------------

/**
 * @brief A directed edge in the causal graph.
 */
struct CausalEdge {
    std::string from_metric;
    std::string to_metric;
    /// Estimated causal strength in [0, 1].
    double strength{0.0};
    /// Estimated lag in seconds from cause to effect.
    double lag_seconds{0.0};
};

/**
 * @brief A directed acyclic graph representing causal relationships between
 *        metrics.
 */
struct CausalGraph {
    std::vector<std::string> nodes;       // metric names
    std::vector<CausalEdge>  edges;

    /// Returns the root nodes (nodes with no incoming edges).
    std::vector<std::string> rootNodes() const;

    json toJSON() const;
};

// ---------------------------------------------------------------------------
// RootCauseReport – the output of a root cause analysis
// ---------------------------------------------------------------------------

/**
 * @brief The result of an automated root cause analysis.
 *
 * Example:
 * @code
 *   RootCauseReport report = analyzer.analyzeIssue(issue, before, after);
 *   // report.primary_cause  → "High compaction rate"
 *   // report.confidence     → 0.87
 * @endcode
 */
struct RootCauseReport {
    /// Short description of the most likely root cause.
    std::string primary_cause;

    /// Stable machine-readable reason code describing the primary cause class.
    std::string primary_reason_code;

    /// Confidence in the primary cause, in [0, 1].
    double confidence{0.0};

    /// Secondary factors that contributed to the issue.
    std::vector<std::string> contributing_factors;

    /// Ordered list of recommended remediation actions.
    std::vector<std::string> remediation_steps;

    /// Per-metric change percentages observed during the incident.
    std::map<std::string, double> metric_impacts;

    json toJSON() const;
    std::string toReport() const;
};

// ---------------------------------------------------------------------------
// RootCauseAnalyzer – the main entry point
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for the root cause analyzer.
 */
struct RootCauseAnalyzerConfig {
    /// Minimum absolute Pearson correlation to consider two metrics correlated.
    double correlation_threshold{0.5};

    /// Maximum number of correlated metrics to return from findCorrelations().
    size_t max_correlations{10};

    /// Number of historical time series data points to keep per metric.
    size_t history_window{100};

    /// Minimum absolute percentage change for a metric delta to be reported
    /// as a contributing factor in a RootCauseReport.
    double significant_delta_pct{20.0};
};

/**
 * @brief Automated root cause identification for performance issues.
 *
 * The analyzer compares "before" and "after" system snapshots, identifies
 * metrics that changed significantly, and attributes the primary cause
 * using a rule-based + correlation-driven heuristic.
 *
 * Usage:
 * @code
 *   RootCauseAnalyzer analyzer;
 *   analyzer.addTimeSeries(write_amp_series);
 *   analyzer.addTimeSeries(cache_hit_rate_series);
 *
 *   auto report = analyzer.analyzeIssue(issue, before_snapshot, after_snapshot);
 *   auto correlations = analyzer.findCorrelations("query_latency_ms");
 *   auto graph = analyzer.buildCausalGraph({write_amp_series, cache_hit_rate_series});
 * @endcode
 */
class RootCauseAnalyzer {
public:
    explicit RootCauseAnalyzer(
        const RootCauseAnalyzerConfig& config = RootCauseAnalyzerConfig{});
    ~RootCauseAnalyzer();

    // Disable copy
    RootCauseAnalyzer(const RootCauseAnalyzer&) = delete;
    RootCauseAnalyzer& operator=(const RootCauseAnalyzer&) = delete;

    // -----------------------------------------------------------------------
    // Time-series registry
    // -----------------------------------------------------------------------

    /**
     * @brief Register a time series for correlation and causal analysis.
     *
     * If a series with the same name already exists it is replaced.
     */
    void addTimeSeries(const TimeSeries& series);

    /**
     * @brief Remove a previously registered time series.
     */
    void removeTimeSeries(const std::string& name);

    // -----------------------------------------------------------------------
    // Core analysis API
    // -----------------------------------------------------------------------

    /**
     * @brief Analyse a performance issue given before/after snapshots.
     *
     * Compares metric deltas, identifies anomalous changes, applies
     * category-specific rules, and returns a @c RootCauseReport.
     *
     * @param issue     The performance issue to investigate.
     * @param before    System snapshot immediately before the incident.
     * @param after     System snapshot during or after the incident.
     * @return          A populated RootCauseReport.
     */
    RootCauseReport analyzeIssue(const PerformanceIssue& issue,
                                 const SystemSnapshot& before,
                                 const SystemSnapshot& after);

    /**
     * @brief Find metrics correlated with the given metric name.
     *
     * Uses registered time series to compute Pearson correlation
     * coefficients and returns the top matches.
     *
     * @param metric_name  Name of the target metric.
     * @return             Sorted (descending |r|) list of correlated metrics.
     */
    std::vector<CorrelatedMetric> findCorrelations(const std::string& metric_name);

    /**
     * @brief Build a directed causal graph from the supplied time series.
     *
     * Applies a simple Granger-inspired heuristic: if metric A
     * consistently leads metric B by ~1 lag step with a strong correlation,
     * an edge A→B is added.
     *
     * @param metrics  Time series to analyse.
     * @return         A CausalGraph suitable for rendering or traversal.
     */
    CausalGraph buildCausalGraph(const std::vector<TimeSeries>& metrics);

    /**
     * @brief Return the current configuration.
     */
    RootCauseAnalyzerConfig getConfig() const;

    /**
     * @brief Replace the current configuration.
     */
    void setConfig(const RootCauseAnalyzerConfig& config);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace observability
} // namespace themis
