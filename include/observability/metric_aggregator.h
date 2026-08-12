/**
 * @file metric_aggregator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <algorithm>
#include <chrono>
#include <deque>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

namespace themis {
namespace observability {

/**
 * @brief Aggregation function applied to a set of metric observations.
 */
enum class AggregationType {
    SUM,   ///< Sum all observations
    AVG,   ///< Arithmetic mean
    MAX,   ///< Maximum value
    MIN,   ///< Minimum value
    P50,   ///< 50th percentile (median)
    P95,   ///< 95th percentile
    P99,   ///< 99th percentile
    RATE,  ///< Per-second rate of change of a counter (requires two samples)
};

/**
 * @brief Rule that controls how a named metric should be aggregated.
 */
struct AggregationRule {
    /// Metric name this rule applies to (exact match).
    std::string metric_name;
    /// Aggregation function to apply.
    AggregationType type{AggregationType::AVG};
    /// Time window over which rate samples are collected (only used for RATE).
    std::chrono::seconds interval{60};
    /// Labels to preserve in the output.  All other labels are dropped.
    std::vector<std::string> group_by_labels;
    /// High-cardinality labels to remove before aggregation.
    std::vector<std::string> drop_labels;
};

/**
 * @brief A snapshot of histogram observations from one source (e.g. a shard).
 */
struct HistogramSnapshot {
    /// Metric name.
    std::string metric_name;
    /// Label set attached to this snapshot.
    std::map<std::string, std::string> labels;
    /// Raw observation values.
    std::vector<double> values;
    /// When the snapshot was captured.
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};
};

/**
 * @brief The result of applying an aggregation operation.
 */
struct AggregatedMetric {
    /// Metric name.
    std::string metric_name;
    /// Label set (after applying drop_labels / group_by_labels from the rule).
    std::map<std::string, std::string> labels;
    /// Aggregated scalar value.
    double value{0.0};
    /// Which aggregation was applied.
    AggregationType type{AggregationType::AVG};
    /// Timestamp of the result.
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};
};

/**
 * @brief Metric observations sourced from a single shard.
 *
 * Each `ShardMetrics` bundles all named metric readings collected from one
 * database shard into a single transfer object suitable for cross-shard
 * aggregation via `MetricAggregator::aggregateShardMetrics()`.
 */
struct ShardMetrics {
    /// Logical shard identifier (e.g. "shard-0", "replica-us-east-1").
    std::string shard_id;
    /// Per-metric raw observation vectors.  Key is the metric name.
    std::map<std::string, std::vector<double>> metrics;
    /// Shard-level labels applied to every metric in this snapshot.
    std::map<std::string, std::string> labels;
    /// When this snapshot was captured (defaults to now).
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};
};

/**
 * @brief Result of a cross-shard metric aggregation pass.
 *
 * Produced by `MetricAggregator::aggregateShardMetrics()`.  Contains one
 * `AggregatedMetric` entry for each rule/group combination that matched the
 * supplied shard data.
 */
struct ShardAggregationSnapshot {
    /// Aggregated metric values, one per matching rule × group combination.
    std::vector<AggregatedMetric> metrics;
    /// Wall-clock time at which the snapshot was produced.
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};
};

/**
 * @brief Prometheus-style advanced metrics aggregation.
 *
 * Provides three key capabilities for the observability module:
 *
 * 1. **Rate calculation** — tracks successive counter samples and computes
 *    the per-second rate of change, consistent with Prometheus `rate()`.
 *
 * 2. **Histogram aggregation** — merges `HistogramSnapshot` objects (e.g.
 *    one per shard) and applies percentile, min/max/sum/avg reductions.
 *
 * 3. **Rule-based aggregation** — `AggregationRule` objects allow callers to
 *    declare how a named metric should be reduced, which labels to keep, and
 *    which high-cardinality labels to drop.  `applyRules()` executes all
 *    registered rules against currently buffered snapshot data.
 *
 * **Thread safety:** all public methods are fully guarded by an internal mutex.
 */
class MetricAggregator {
public:
    MetricAggregator() = default;
    ~MetricAggregator() = default;

    // Non-copyable, non-movable (mutex member prevents move semantics)
    MetricAggregator(const MetricAggregator&) = delete;
    MetricAggregator& operator=(const MetricAggregator&) = delete;
    MetricAggregator(MetricAggregator&&) = delete;
    MetricAggregator& operator=(MetricAggregator&&) = delete;

    // =========================================================================
    // Rate calculation
    // =========================================================================

    /**
     * @brief Record a raw counter sample for later rate calculation.
     *
     * Samples are retained in a sliding window controlled by @p window.  If
     * the window is 0 the default of 120 seconds is used.  Old samples are
     * pruned on every call.
     *
     * @param name   Metric name.
     * @param value  Monotonically increasing counter value.
     * @param labels Optional label set (used as part of the series key).
     * @param window Retention window for rate samples.
     */
    void recordCounterSample(
        const std::string& name, int64_t value,
        const std::map<std::string, std::string>& labels = {},
        std::chrono::seconds window = std::chrono::seconds{120});

    /**
     * @brief Calculate the per-second rate of change for a counter series.
     *
     * Uses the oldest and newest samples within the retention window.
     * Returns 0.0 if fewer than two samples are available.
     *
     * @param name   Metric name.
     * @param labels Optional label set.
     * @return Per-second rate, or 0.0 when insufficient samples exist.
     */
    double calculateRate(
        const std::string& name,
        const std::map<std::string, std::string>& labels = {}) const;

    // =========================================================================
    // Histogram aggregation
    // =========================================================================

    /**
     * @brief Add a histogram snapshot to the internal buffer.
     *
     * Snapshots are accumulated until `aggregateHistograms()` or `reset()` is
     * called.  Multiple snapshots with the same metric_name are merged.
     */
    void addHistogramSnapshot(const HistogramSnapshot& snapshot);

    /**
     * @brief Aggregate all buffered snapshots for @p metric_name.
     *
     * @param metric_name Metric to aggregate.
     * @param type        Reduction to apply (SUM/AVG/MAX/MIN/P50/P95/P99).
     *                    RATE is not valid here; pass P99 instead.
     * @param filter_labels Only include snapshots whose label set contains all
     *                      key/value pairs in @p filter_labels (empty = all).
     * @return AggregatedMetric with the computed value.
     * @throws std::invalid_argument if @p type is RATE or no snapshots are
     *         available for @p metric_name.
     */
    AggregatedMetric aggregateHistograms(
        const std::string& metric_name,
        AggregationType type,
        const std::map<std::string, std::string>& filter_labels = {}) const;

    // =========================================================================
    // Rule-based aggregation
    // =========================================================================

    /**
     * @brief Register an aggregation rule.
     *
     * Duplicate rules (same metric_name + type combination) are replaced.
     */
    void addAggregationRule(const AggregationRule& rule);

    /**
     * @brief Remove a previously registered rule.
     *
     * @param metric_name Metric name of the rule to remove.
     * @return true if a rule was found and removed, false otherwise.
     */
    bool removeAggregationRule(const std::string& metric_name);

    /**
     * @brief Return a copy of all registered rules.
     */
    std::vector<AggregationRule> getRules() const;

    /**
     * @brief Execute all registered rules against buffered snapshot data.
     *
     * For each rule, the matching histogram snapshots (after applying
     * `drop_labels`) are aggregated using the rule's `type`.  Rules that
     * target metrics with no buffered snapshots are skipped.
     *
     * @return Vector of aggregated results, one per applicable rule.
     */
    std::vector<AggregatedMetric> applyRules() const;

    /**
     * @brief Aggregate metrics from multiple shards into a single snapshot.
     *
     * Converts each `ShardMetrics` into histogram snapshots (tagging them
     * with the source `shard_id`), then applies all registered rules to
     * produce a unified `ShardAggregationSnapshot`.  This call does **not** mutate the
     * internal snapshot buffer; the supplied shard data is processed
     * transiently and the result is returned directly.
     *
     * @param shard_metrics One `ShardMetrics` per shard.
     * @return `ShardAggregationSnapshot` containing one `AggregatedMetric` per
     *         applicable rule × group combination.
     */
    ShardAggregationSnapshot aggregateShardMetrics(
        const std::vector<ShardMetrics>& shard_metrics) const;

    /**
     * @brief Prune buffered histogram snapshots older than @p window.
     *
     * Reduces cardinality by discarding stale shard observations.  Rate
     * samples are also pruned with the same window (converted from minutes to
     * seconds).  Known-series tracking for cardinality limits is NOT reset so
     * that new insertions continue to be validated against the same limits.
     *
     * @param window Retention window.  Snapshots whose timestamp falls before
     *               `now - window` are discarded.
     */
    void rollupMetrics(std::chrono::minutes window);

    // =========================================================================
    // Cardinality management
    // =========================================================================

    /**
     * @brief Set a per-metric cardinality limit.
     *
     * When the number of distinct label-set combinations for @p metric_name
     * reaches @p limit, subsequent `addHistogramSnapshot` calls with new label
     * sets are rejected (cardinality overflow is counted).
     *
     * Set to 0 to disable the limit for that metric.
     */
    void setMetricCardinalityLimit(const std::string& metric_name, size_t limit);

    /**
     * @brief Return the number of distinct label-set series tracked for @p
     *        metric_name.
     */
    size_t getSeriesCount(const std::string& metric_name) const;

    /**
     * @brief Return the total number of snapshot insertions rejected due to
     *        cardinality overflow.
     */
    int64_t getDroppedSnapshotCount() const;

    // =========================================================================
    // Utilities
    // =========================================================================

    /**
     * @brief Clear all buffered snapshots, rate samples, and dropped counter.
     *
     * Rules are preserved.
     */
    void reset();

    /**
     * @brief Remove rate samples older than @p window from all series.
     */
    void pruneRateSamples(std::chrono::seconds window = std::chrono::seconds{120});

private:
    // ---- rate state ----------------------------------------------------------

    struct RateSample {
        int64_t value;
        std::chrono::steady_clock::time_point timestamp;
    };

    // key → ordered deque of samples (oldest first)
    std::map<std::string, std::deque<RateSample>> rate_samples_;

    // ---- histogram state -----------------------------------------------------

    // key (metric_name + label fingerprint) → snapshots
    std::map<std::string, std::vector<HistogramSnapshot>> snapshots_;

    // ---- cardinality state ---------------------------------------------------

    // metric_name → series limit (0 = unlimited)
    std::map<std::string, size_t> cardinality_limits_;

    // metric_name → set of label fingerprints already seen
    std::map<std::string, std::vector<std::string>> known_series_;

    int64_t dropped_snapshots_{0};

    // ---- rule state ----------------------------------------------------------

    // metric_name → rule
    std::map<std::string, AggregationRule> rules_;

    // ---- internal helpers ----------------------------------------------------

    mutable std::mutex mutex_;

    static std::string makeSeriesKey(
        const std::string& name,
        const std::map<std::string, std::string>& labels);

    static std::string makeLabelFingerprint(
        const std::map<std::string, std::string>& labels);

    /// Apply drop_labels and return the remaining labels.
    static std::map<std::string, std::string> applyDropLabels(
        const std::map<std::string, std::string>& labels,
        const std::vector<std::string>& drop);

    /// Reduce a sorted vector of doubles using the given AggregationType.
    static double reduce(std::vector<double> sorted_values, AggregationType type);

    /// Check cardinality for a new snapshot (caller holds mutex_).
    bool checkSnapshotCardinality(const std::string& metric_name,
                                  const std::string& label_fingerprint);
};

}  // namespace observability
}  // namespace themis
