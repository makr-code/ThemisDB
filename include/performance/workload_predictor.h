/**
 * @file workload_predictor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB - ML-Based Workload Predictor for Proactive Resource Scaling
// Phase 4: ML-Based Optimization & CI Integration
// Roadmap: Long-term (6-12 months), Issue #2214
//
// Implements a lightweight ML predictor using:
//   1. Exponential Moving Average (EMA) for noise-robust smoothing.
//   2. Ordinary Least-Squares (OLS) linear regression for trend extrapolation.
//   3. Coefficient-of-variation (CV) derived confidence score.
//
// The predictor maintains a fixed-size sliding window of WorkloadSnapshot
// observations.  Predictions are generated on demand (no background threads),
// keeping overhead below 1 µs per predict() / recommend_scaling() call.
//
// Compile-time gate: THEMIS_ENABLE_ML_WORKLOAD_PREDICTOR
// Runtime gate:      PerformanceFeatureFlags::instance().ml_workload_predictor_enabled()

#pragma once

#include <cstdint>
#include <deque>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <utility>
#include <vector>

namespace themis {
namespace performance {

// ---------------------------------------------------------------------------
// WorkloadSnapshot – point-in-time workload observation
// ---------------------------------------------------------------------------

/// A point-in-time snapshot of system workload metrics.
struct WorkloadSnapshot {
    uint64_t timestamp_us{0};        ///< Microseconds since Unix epoch
    double   qps{0.0};               ///< Queries per second observed in this interval
    double   cpu_utilization{0.0};   ///< CPU utilization [0.0, 1.0]
    double   memory_utilization{0.0};///< Memory utilization [0.0, 1.0]
    double   avg_latency_us{0.0};    ///< Average query latency (µs)
    double   p99_latency_us{0.0};    ///< P99 query latency (µs)
    uint32_t active_connections{0};  ///< Number of active client connections
};

// ---------------------------------------------------------------------------
// WorkloadForecast – predicted workload at a future time
// ---------------------------------------------------------------------------

/// Predicted workload at a future point in time.
struct WorkloadForecast {
    uint64_t forecast_timestamp_us{0};    ///< Target time of prediction
    double   predicted_qps{0.0};          ///< Forecasted queries per second
    double   predicted_cpu_utilization{0.0};    ///< Forecasted CPU utilization [0.0, 1.0]
    double   predicted_memory_utilization{0.0}; ///< Forecasted memory utilization [0.0, 1.0]
    double   predicted_avg_latency_us{0.0};     ///< Forecasted average latency (µs)
    double   confidence{0.0};             ///< Forecast confidence [0.0, 1.0]; higher = more reliable
};

// ---------------------------------------------------------------------------
// ScaleDirection / ScaleRecommendation – resource scaling advice
// ---------------------------------------------------------------------------

/// Direction of a resource scaling recommendation.
enum class ScaleDirection {
    NONE,  ///< Current resources are adequate; no change needed
    UP,    ///< Workload trend suggests resources should be increased
    DOWN   ///< Workload trend suggests resources can be reduced
};

/// Actionable recommendation produced by WorkloadPredictor::recommend_scaling().
struct ScaleRecommendation {
    ScaleDirection direction{ScaleDirection::NONE};
    uint32_t recommended_thread_pool_size{0}; ///< Suggested thread pool size
    uint64_t recommended_cache_size_mb{0};    ///< Suggested cache size in MiB
    double   confidence{0.0};                 ///< Confidence of the recommendation [0.0, 1.0]
    std::string reason;                        ///< Human-readable explanation
};

// ---------------------------------------------------------------------------
// WorkloadPredictor
// ---------------------------------------------------------------------------

/**
 * @brief ML-based workload predictor for proactive resource scaling.
 *
 * Records a sliding window of WorkloadSnapshot observations and uses
 * Exponential Moving Average (EMA) smoothing combined with Ordinary
 * Least-Squares (OLS) linear regression to forecast future workload and
 * recommend resource scaling actions.
 *
 * Thread-safety: all public methods are thread-safe via an internal mutex.
 *
 * Usage:
 * @code
 *   WorkloadPredictor predictor;
 *   predictor.record({.timestamp_us = now_us(), .qps = 1200, .cpu_utilization = 0.65});
 *   // ... record more snapshots over time ...
 *   auto forecast = predictor.predict(30 * 1'000'000ULL); // predict 30 s into the future
 *   auto rec = predictor.recommend_scaling(32, 4096);
 *   if (rec.direction == ScaleDirection::UP) {
 *       resize_thread_pool(rec.recommended_thread_pool_size);
 *   }
 * @endcode
 */
class WorkloadPredictor {
public:
    /// Configuration for the predictor.
    struct Config {
        /// Maximum number of snapshots retained in the sliding window.
        /// Older snapshots are evicted when the window is full.
        size_t history_window{60};

        /// EMA smoothing factor α ∈ (0, 1].
        /// Smaller values produce slower but more stable smoothing.
        double ema_alpha{0.2};

        /// CPU/memory utilization threshold above which scale-up is recommended.
        double scale_up_threshold{0.80};

        /// CPU/memory utilization threshold below which scale-down is recommended.
        double scale_down_threshold{0.30};

        /// Minimum allowed thread pool size for scaling recommendations.
        uint32_t min_thread_pool_size{4};

        /// Maximum allowed thread pool size for scaling recommendations.
        uint32_t max_thread_pool_size{256};

        /// Minimum allowed cache size (MiB) for scaling recommendations.
        uint64_t min_cache_size_mb{256};

        /// Maximum allowed cache size (MiB) for scaling recommendations.
        uint64_t max_cache_size_mb{32768};
    };

    /// Construct a predictor with default configuration.
    WorkloadPredictor();

    /// Construct a predictor with the given configuration.
    explicit WorkloadPredictor(const Config& config);

    /// Record a new workload snapshot.
    /// If the sliding window is full the oldest snapshot is evicted.
    void record(const WorkloadSnapshot& snapshot);

    /**
     * @brief Predict the workload @p horizon_us microseconds into the future.
     *
     * Requires at least 2 observations; returns a zero-confidence forecast
     * when fewer observations are available.
     *
     * @param horizon_us  Prediction horizon in microseconds (e.g. 30'000'000 = 30 s).
     * @return WorkloadForecast with predicted values and a confidence score.
     */
    WorkloadForecast predict(uint64_t horizon_us) const;

    /**
     * @brief Recommend a resource scaling action based on the current trend.
     *
     * Evaluates the forecasted utilization against the configured thresholds
     * and proposes an adjusted thread pool size and cache size.
     *
     * @param current_thread_pool_size  Current thread pool size.
     * @param current_cache_size_mb     Current cache size in MiB.
     * @return ScaleRecommendation with direction, target sizes, confidence, and reason.
     */
    ScaleRecommendation recommend_scaling(uint32_t current_thread_pool_size,
                                          uint64_t current_cache_size_mb) const;

    /// Return the number of observations currently in the sliding window.
    size_t observation_count() const noexcept;

    /// Clear all recorded observations and reset internal EMA state.
    void reset() noexcept;

    /// Return a copy of the current configuration.
    Config config() const noexcept { return config_; }

private:
    // ------------------------------------------------------------------
    // Internal helpers (called with mutex_ held)
    // ------------------------------------------------------------------

    /// Compute the Exponential Moving Average over @p values.
    double compute_ema(const std::vector<double>& values) const noexcept;

    /**
     * @brief Fit a least-squares line y = a + b*x to @p values (x = 0..n-1).
     * @return {slope b, intercept a}.  Returns {0, last_value} for < 2 points.
     */
    std::pair<double, double> linear_regression(const std::vector<double>& values) const noexcept;

    /**
     * @brief Compute a confidence score ∈ [0, 1] based on the coefficient of
     * variation (CV = stddev / mean) of @p values.
     *
     * Low CV (stable signal)  → confidence near 1.
     * High CV (noisy signal)  → confidence near 0.
     */
    double compute_confidence(const std::vector<double>& values) const noexcept;

    /// Clamp @p v to [lo, hi].
    static double clamp(double v, double lo, double hi) noexcept;

    Config config_;
    mutable std::shared_mutex mutex_;
    std::deque<WorkloadSnapshot> history_;
};

} // namespace performance
} // namespace themis
