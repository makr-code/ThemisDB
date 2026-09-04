/**
 * @file advanced_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <chrono>
#include <deque>
#include <map>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace themis {
namespace observability {

// ============================================================================
// Result types
// ============================================================================

/**
 * @brief Result of querying a summary metric.
 */
struct SummaryResult {
    /// Metric name.
    std::string metric_name;
    /// Computed quantile values: quantile → value (e.g. 0.99 → 250.0 ms).
    std::map<double, double> quantile_values;
    /// Number of observations retained (up to `kMaxSummarySamples`).
    uint64_t count{0};
    /// Sum of all retained observations.
    double sum{0.0};
    /// Timestamp at which the result was computed.
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};
};

/**
 * @brief A single bucket in an exponential histogram.
 */
struct ExponentialHistogramBucket {
    /// Inclusive lower bound of the bucket.
    double lower_bound{0.0};
    /// Exclusive upper bound of the bucket.
    double upper_bound{0.0};
    /// Number of observations that fell in this bucket.
    uint64_t count{0};
};

/**
 * @brief Result of querying an exponential histogram metric.
 */
struct ExponentialHistogramResult {
    /// Metric name.
    std::string metric_name;
    /// Exponential base used for bucket boundaries.
    double scale{2.0};
    /// Sorted bucket list (ascending by lower_bound).
    std::vector<ExponentialHistogramBucket> buckets;
    /// Total number of observations recorded (including non-positive values).
    uint64_t total_count{0};
    /// Sum of all positive observation values.
    double sum{0.0};
    /// Count of non-positive (≤ 0) observations tracked separately.
    uint64_t zero_count{0};
    /// Timestamp at which the result was computed.
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};
};

// ============================================================================
// AdvancedMetrics
// ============================================================================

/**
 * @brief Extended metric types beyond counters, gauges, and histograms.
 *
 * See file-level documentation for a description of the five metric types and
 * usage examples.
 */
class AdvancedMetrics {
public:
    /// Maximum number of raw values retained per summary metric.
    static constexpr size_t kMaxSummarySamples = 1000;

    /// Maximum number of raw values retained per exponential histogram metric.
    static constexpr size_t kMaxExpHistSamples = 10000;

    AdvancedMetrics() = default;
    ~AdvancedMetrics() = default;

    // Non-copyable, non-movable (mutex member prevents move semantics).
    AdvancedMetrics(const AdvancedMetrics&) = delete;
    AdvancedMetrics& operator=(const AdvancedMetrics&) = delete;
    AdvancedMetrics(AdvancedMetrics&&) = delete;
    AdvancedMetrics& operator=(AdvancedMetrics&&) = delete;

    // =========================================================================
    // Summary
    // =========================================================================

    /**
     * @brief Record a value for a summary metric.
     *
     * Up to `kMaxSummarySamples` most-recent observations are retained per
     * metric name (oldest are evicted when the buffer is full).  Quantiles are
     * computed lazily when `getSummary()` is called.
     *
     * @param name  Metric name.
     * @param value Observed value.
     */
    void recordSummary(const std::string& name, double value);

    /**
     * @brief Compute quantile values for a named summary metric.
     *
     * @param name      Metric name.
     * @param quantiles Quantiles to compute; each must be in [0, 1].
     * @return SummaryResult containing computed quantile_values, count, and
     *         sum.  If no samples exist, quantile_values are 0.0 and count is 0.
     */
    SummaryResult getSummary(
        const std::string& name,
        const std::vector<double>& quantiles = {0.5, 0.9, 0.95, 0.99}) const;

    // =========================================================================
    // Exponential histogram
    // =========================================================================

    /**
     * @brief Record a value for an exponential histogram metric.
     *
     * Non-positive values are counted separately in `zero_count` and are not
     * bucketed.  The exponential base (scale) is locked in on the first call
     * for a given metric name; subsequent calls with a different scale are
     * silently ignored.
     *
     * @param name  Metric name.
     * @param value Observed value.
     * @param scale Exponential base for bucket boundaries (must be > 1.0).
     *              Defaults to 2.0.
     */
    void recordExponentialHistogram(const std::string& name, double value,
                                    double scale = 2.0);

    /**
     * @brief Return the current exponential histogram for @p name.
     *
     * Bucket boundaries are computed from the stored observations using the
     * scale locked in at first recording.
     *
     * @param name Metric name.
     * @return ExponentialHistogramResult (empty buckets if no values recorded).
     */
    ExponentialHistogramResult getExponentialHistogram(const std::string& name) const;

    // =========================================================================
    // Cardinality
    // =========================================================================

    /**
     * @brief Record a string value for a cardinality metric.
     *
     * Each unique value is counted once per metric name.
     *
     * @param name  Metric name (e.g. "active_tenants").
     * @param value String value to track (e.g. a tenant ID).
     */
    void recordCardinality(const std::string& name, const std::string& value);

    /**
     * @brief Return the number of distinct values recorded for @p name.
     *
     * @param name Metric name.
     * @return Exact cardinality, or 0 if the metric has not been recorded.
     */
    size_t getCardinalityEstimate(const std::string& name) const;

    // =========================================================================
    // Time-weighted average
    // =========================================================================

    /**
     * @brief Record a value for a time-weighted average metric.
     *
     * Samples older than @p window are pruned on each call.  The
     * time-weighted average integrates `value × dt` over the retained window.
     *
     * @param name   Metric name.
     * @param value  Observed gauge value.
     * @param window Sliding window; samples older than this are discarded.
     */
    void recordTimeWeightedAverage(const std::string& name, double value,
                                   std::chrono::seconds window);

    /**
     * @brief Compute the current time-weighted average for @p name.
     *
     * With a single retained sample the sample value itself is returned.
     * Returns 0.0 if no samples exist.
     *
     * @param name Metric name.
     * @return Time-weighted average of the gauge over the retained window.
     */
    double getTimeWeightedAverage(const std::string& name) const;

    // =========================================================================
    // Rate
    // =========================================================================

    /**
     * @brief Record a raw value sample for a rate metric.
     *
     * Unlike the integer-counter rate in `MetricAggregator`, this method
     * accepts arbitrary double values.  Samples older than @p interval are
     * pruned on each call.
     *
     * @param name     Metric name.
     * @param value    Raw sample value.
     * @param interval Sliding window; samples older than this are discarded.
     */
    void recordRate(const std::string& name, double value,
                    std::chrono::seconds interval);

    /**
     * @brief Compute the per-second rate of change for @p name.
     *
     * Rate is computed as `(newest_value - oldest_value) / elapsed_seconds`
     * from the retained samples.  Returns 0.0 if fewer than two samples exist.
     *
     * @param name Metric name.
     * @return Per-second rate, or 0.0 when insufficient samples exist.
     */
    double getRate(const std::string& name) const;

    // =========================================================================
    // Utilities
    // =========================================================================

    /**
     * @brief Clear all stored data for all metric types.
     */
    void reset();

private:
    // ---- Summary state -------------------------------------------------------

    struct SummaryData {
        std::deque<double> values;
        double sum{0.0};
    };
    std::map<std::string, SummaryData> summary_data_;

    // ---- Exponential histogram state ----------------------------------------

    struct ExpHistData {
        double scale{2.0};
        std::deque<double> values;
        double sum{0.0};
        uint64_t zero_count{0};
    };
    std::map<std::string, ExpHistData> exp_hist_data_;

    // ---- Cardinality state --------------------------------------------------

    std::map<std::string, std::unordered_set<std::string>> cardinality_sets_;

    // ---- Time-weighted average state ----------------------------------------

    struct TWASample {
        double value = 0;
        std::chrono::steady_clock::time_point timestamp;
    };
    std::map<std::string, std::deque<TWASample>> twa_samples_;

    // ---- Rate state ----------------------------------------------------------

    struct RateSample {
        double value = 0;
        std::chrono::steady_clock::time_point timestamp;
    };
    std::map<std::string, std::deque<RateSample>> rate_samples_;

    // ---- Mutex ---------------------------------------------------------------

    mutable std::mutex mutex_;

    // ---- Helpers -------------------------------------------------------------

    /// Compute the @p q quantile from a sorted vector using the nearest-rank
    /// method.  @p q must be in [0, 1].
    static double computeQuantile(const std::vector<double>& sorted_vals, double q);
};

}  // namespace observability
}  // namespace themis
