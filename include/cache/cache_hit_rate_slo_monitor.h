/**
 * @file cache_hit_rate_slo_monitor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "cache/cache_metrics.h"
#include "observability/alertmanager.h"

namespace themis {
namespace cache {

/**
 * @brief Cache hit rate SLO (Service Level Objective) monitor.
 *
 * Evaluates the overall cache hit rate against configured thresholds and fires
 * alerts via an Alertmanager when the SLO is violated. Resolves open alerts
 * automatically when the hit rate recovers above the configured thresholds.
 *
 * ## Usage
 * @code
 * CacheHitRateSloMonitor::Config cfg;
 * cfg.critical_threshold = 0.40;   // fire CRITICAL alert below 40% hit rate
 * cfg.warning_threshold  = 0.60;   // fire WARNING alert below 60% hit rate
 * cfg.min_requests       = 100;    // do not evaluate with fewer requests
 *
 * auto alertmanager = std::make_shared<themis::observability::DefaultAlertmanager>();
 * CacheHitRateSloMonitor monitor(cfg, alertmanager);
 *
 * // Call evaluate() periodically (e.g. from a metrics scrape or background thread).
 * monitor.evaluate(cache.getEnhancedMetrics());
 *
 * // Inspect SLO status at any time.
 * auto status = monitor.getStatus();
 * @endcode
 *
 * Thread-safety: all public methods are thread-safe.
 */
class CacheHitRateSloMonitor {
public:
    // -----------------------------------------------------------------------
    // Types
    // -----------------------------------------------------------------------

    /**
     * @brief Alert severity levels for SLO violations.
     */
    enum class ViolationLevel {
        NONE,       ///< Hit rate is within SLO
        WARNING,    ///< Hit rate is below warning threshold
        CRITICAL    ///< Hit rate is below critical threshold
    };

    /**
     * @brief Cache tier identifier for per-tier latency tracking.
     */
    enum class Tier : std::size_t {
        L1 = 0, ///< In-process LRU tier
        L2 = 1, ///< Secondary (e.g. off-heap or Redis) tier
        L3 = 2, ///< Tertiary (e.g. disk) tier
        COUNT   ///< Sentinel – number of tiers
    };

    /**
     * @brief Configuration for the SLO monitor.
     */
    struct Config {
        /// Minimum cache hit rate before a WARNING alert fires (default: 0.60 = 60%).
        double warning_threshold = 0.60;

        /// Minimum cache hit rate before a CRITICAL alert fires (default: 0.40 = 40%).
        double critical_threshold = 0.40;

        /// Minimum number of requests (hits + misses) required before evaluating the
        /// SLO. Prevents false alerts during low-traffic periods (default: 100).
        uint64_t min_requests = 100;

        /// Minimum seconds between repeated alerts of the same level to reduce noise
        /// (default: 300 = 5 minutes).
        int alert_cooldown_seconds = 300;

        /// Human-readable name of this cache instance, used in alert labels.
        std::string cache_name = "adaptive_query_cache";

        /// p99 latency (ms) above which a WARNING latency alert fires.  Set to 0 to
        /// disable latency alerting (default: 0 = disabled).
        double p99_warn_ms = 0.0;

        /// p99 latency (ms) above which a CRITICAL latency alert fires.  Must be
        /// greater than `p99_warn_ms` when both are non-zero (default: 0 = disabled).
        double p99_critical_ms = 0.0;

        /**
         * @brief Validate configuration parameters.
         *
         * Checks:
         * - `critical_threshold` < `warning_threshold` (critical is stricter)
         * - Both thresholds are in the range [0.0, 1.0]
         * - `alert_cooldown_seconds` >= 0
         * - `p99_critical_ms` < `p99_warn_ms` when both are non-zero
         *   (critical must be a tighter bound than warning)
         *
         * @param error_msg  If non-null and validation fails, filled with a
         *                   human-readable description of the error.
         * @return true if configuration is valid, false otherwise.
         */
        bool validate(std::string* error_msg = nullptr) const {
            if (critical_threshold >= warning_threshold) {
                if (error_msg) {
                    *error_msg = "critical_threshold (" + std::to_string(critical_threshold) +
                                 ") must be less than warning_threshold (" +
                                 std::to_string(warning_threshold) + ")";
                }
                return false;
            }
            if (critical_threshold < 0.0 || critical_threshold > 1.0) {
                if (error_msg) {
                    *error_msg = "critical_threshold must be in [0.0, 1.0], got " +
                                 std::to_string(critical_threshold);
                }
                return false;
            }
            if (warning_threshold < 0.0 || warning_threshold > 1.0) {
                if (error_msg) {
                    *error_msg = "warning_threshold must be in [0.0, 1.0], got " +
                                 std::to_string(warning_threshold);
                }
                return false;
            }
            if (alert_cooldown_seconds < 0) {
                if (error_msg) {
                    *error_msg = "alert_cooldown_seconds must be >= 0, got " +
                                 std::to_string(alert_cooldown_seconds);
                }
                return false;
            }
            if (p99_warn_ms < 0.0) {
                if (error_msg) {
                    *error_msg = "p99_warn_ms must be >= 0, got " + std::to_string(p99_warn_ms);
                }
                return false;
            }
            if (p99_critical_ms < 0.0) {
                if (error_msg) {
                    *error_msg = "p99_critical_ms must be >= 0, got " +
                                 std::to_string(p99_critical_ms);
                }
                return false;
            }
            if (p99_warn_ms > 0.0 && p99_critical_ms > 0.0 &&
                p99_critical_ms <= p99_warn_ms) {
                if (error_msg) {
                    *error_msg = "p99_critical_ms (" + std::to_string(p99_critical_ms) +
                                 ") must be greater than p99_warn_ms (" +
                                 std::to_string(p99_warn_ms) + ")";
                }
                return false;
            }
            return true;
        }
    };

    /**
     * @brief Current SLO evaluation result.
     */
    struct EvaluationResult {
        double hit_rate        = 0.0;           ///< Computed hit rate [0, 1]
        uint64_t total_requests = 0;            ///< Total requests evaluated
        ViolationLevel level   = ViolationLevel::NONE; ///< Current violation level
        bool alert_fired       = false;         ///< True if a hit-rate alert was fired this call
        bool alert_resolved    = false;         ///< True if a hit-rate alert was resolved this call

        // Latency percentiles (milliseconds) computed across all tiers at evaluation time.
        // Zero when no latency samples have been recorded yet.
        double p50_latency_ms  = 0.0;
        double p95_latency_ms  = 0.0;
        double p99_latency_ms  = 0.0;

        ViolationLevel latency_level  = ViolationLevel::NONE; ///< Current latency violation level
        bool latency_alert_fired      = false; ///< True if a latency alert was fired this call
        bool latency_alert_resolved   = false; ///< True if a latency alert was resolved this call
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * @brief Construct a monitor with the given config and optional alertmanager.
     *
     * If @p alertmanager is nullptr, SLO violations are only logged; no alerts
     * are dispatched.
     */
    explicit CacheHitRateSloMonitor(
        std::shared_ptr<observability::Alertmanager> alertmanager = nullptr);
    CacheHitRateSloMonitor(
        const Config& config,
        std::shared_ptr<observability::Alertmanager> alertmanager = nullptr);

    ~CacheHitRateSloMonitor() = default;

    // Non-copyable, moveable
    CacheHitRateSloMonitor(const CacheHitRateSloMonitor&)            = delete;
    CacheHitRateSloMonitor& operator=(const CacheHitRateSloMonitor&) = delete;
    CacheHitRateSloMonitor(CacheHitRateSloMonitor&&)                 noexcept = default;
    CacheHitRateSloMonitor& operator=(CacheHitRateSloMonitor&&)      noexcept = default;

    // -----------------------------------------------------------------------
    // Core API
    // -----------------------------------------------------------------------

    /**
     * @brief Evaluate the current cache metrics against the configured SLO.
     *
     * Fires or resolves alerts via the alertmanager as appropriate.  Safe to
     * call from any thread; protected internally by a mutex.
     *
     * @param metrics  Current snapshot of CacheMetrics (e.g. from
     *                 AdaptiveQueryCache::getEnhancedMetrics()).
     * @return Evaluation result describing the current SLO state.
     */
    EvaluationResult evaluate(const CacheMetrics& metrics);

    /**
     * @brief Record a single cache-get latency sample for a specific tier.
     *
     * Call this on every cache `get()` to accumulate the rolling latency
     * histogram used for p50/p95/p99 computation and latency SLO alerting.
     * Thread-safe; backed by per-bucket atomic counters (no global lock).
     *
     * @param tier        The cache tier that serviced the request.
     * @param latency_ms  Observed latency in milliseconds (must be >= 0).
     */
    void recordLatency(Tier tier, double latency_ms);

    // -----------------------------------------------------------------------
    // Status & Inspection
    // -----------------------------------------------------------------------

    /**
     * @brief Return the most recent evaluation result without re-evaluating.
     */
    EvaluationResult getLastResult() const;

    /**
     * @brief Return the current violation level without re-evaluating.
     */
    ViolationLevel getCurrentViolationLevel() const;

    /**
     * @brief Check whether any SLO violation is currently active (FIRING).
     */
    bool isSloViolated() const;

    /**
     * @brief Export current SLO status as JSON.
     *
     * Example output:
     * @code
     * {
     *   "hit_rate": 0.45,
     *   "total_requests": 5000,
     *   "violation_level": "WARNING",
     *   "thresholds": { "warning": 0.60, "critical": 0.40 },
     *   "alerts": [ { "id": "cache_hit_rate_warning", "status": "FIRING" } ],
     *   "latency": {
     *     "p50_ms": 0.8, "p95_ms": 4.2, "p99_ms": 12.1,
     *     "violation_level": "NONE",
     *     "l1": { "p50_ms": 0.3, "p95_ms": 1.2, "p99_ms": 3.0 },
     *     "l2": { "p50_ms": 1.5, "p95_ms": 6.0, "p99_ms": 14.0 },
     *     "l3": { "p50_ms": 5.0, "p95_ms": 22.0, "p99_ms": 55.0 }
     *   }
     * }
     * @endcode
     */
    nlohmann::json getStatus() const;

    /**
     * @brief Return IDs of all currently active (FIRING) SLO alerts.
     */
    std::vector<std::string> getActiveAlertIds() const;

    /**
     * @brief Update the alertmanager (may be nullptr to disable alert dispatch).
     */
    void setAlertmanager(std::shared_ptr<observability::Alertmanager> alertmanager);

    /**
     * @brief Return a human-readable string for a ViolationLevel.
     */
    static std::string violationLevelToString(ViolationLevel level);

private:
    // -----------------------------------------------------------------------
    // Rolling latency histogram (lock-free per-bucket atomics)
    // -----------------------------------------------------------------------

    /**
     * @brief Simple fixed-bucket latency histogram for p50/p95/p99 computation.
     *
     * Buckets cover the range 0 – 500+ ms in logarithmic steps.  All counters
     * are atomics so `record()` is lock-free and safe to call concurrently from
     * multiple cache-get paths.  `percentileMs()` reads are eventually consistent
     * (relaxed memory order) and do not need the monitor mutex.
     */
    struct LatencyHistogram {
        // Bucket upper bounds (exclusive) in milliseconds.
        // Values >= last bound go into the overflow (last) bucket.
        // Buckets: <0.1, 0.1-0.5, 0.5-1, 1-2, 2-5, 5-10, 10-25, 25-50,
        //          50-100, 100-250, 250-500, >=500
        static constexpr std::size_t kNumBuckets = 12;
        static constexpr double kBucketBoundsMs[kNumBuckets] = {
            0.1, 0.5, 1.0, 2.0, 5.0, 10.0, 25.0, 50.0, 100.0, 250.0, 500.0,
            // last bucket is the overflow – no upper bound
            0.0
        };
        // Representative midpoint (ms) returned for each bucket.
        static constexpr double kMidpointsMs[kNumBuckets] = {
            0.05, 0.3, 0.75, 1.5, 3.5, 7.5, 17.5, 37.5, 75.0, 175.0, 375.0, 750.0
        };

        mutable std::array<std::atomic<uint64_t>, kNumBuckets> buckets{};
        std::atomic<uint64_t> count{0};

        LatencyHistogram() noexcept {
            for (auto& b : buckets) {
              b.store(0, std::memory_order_relaxed);
            }
            count.store(0, std::memory_order_relaxed);
        }

        // Non-copyable, non-movable (atomics)
        LatencyHistogram(const LatencyHistogram&) = delete;
        LatencyHistogram& operator=(const LatencyHistogram&) = delete;

        void record(double latency_ms) noexcept {
            count.fetch_add(1, std::memory_order_relaxed);
            std::size_t idx = kNumBuckets - 1;
            for (std::size_t i = 0; i < kNumBuckets - 1; ++i) {
                if (latency_ms < kBucketBoundsMs[i]) {
                    idx = i;
                    break;
                }
            }
            buckets[idx].fetch_add(1, std::memory_order_relaxed);
        }

        /// Returns the p-th percentile latency in milliseconds (0.0 when empty).
        double percentileMs(double p) const noexcept {
            uint64_t total = count.load(std::memory_order_relaxed);
            if (total == 0) {
              return 0.0;
            }
            uint64_t target = static_cast<uint64_t>(static_cast<double>(total) * p);
            if (target == 0) {
              target = 1;
            }
            uint64_t cumulative = 0;
            for (std::size_t i = 0; i < kNumBuckets; ++i) {
                cumulative += buckets[i].load(std::memory_order_relaxed);
                if (cumulative >= target) {
                  return kMidpointsMs[i];
                }
            }
            return kMidpointsMs[kNumBuckets - 1];
        }
    };

    // -----------------------------------------------------------------------
    // Internals
    // -----------------------------------------------------------------------

    Config config_;
    std::shared_ptr<observability::Alertmanager> alertmanager_;

    mutable std::mutex mutex_;

    // Last evaluation result
    EvaluationResult last_result_;

    // Hit-rate alert state tracking
    ViolationLevel active_violation_ = ViolationLevel::NONE;
    std::string active_warning_alert_id_;
    std::string active_critical_alert_id_;
    std::chrono::steady_clock::time_point last_warning_alert_time_;
    std::chrono::steady_clock::time_point last_critical_alert_time_;

    // Per-tier latency histograms (lock-free; no mutex needed for record())
    LatencyHistogram latency_hist_[static_cast<std::size_t>(Tier::COUNT)];

    // Latency SLO alert state (guarded by mutex_)
    ViolationLevel active_latency_violation_ = ViolationLevel::NONE;
    std::string active_latency_warning_alert_id_;
    std::string active_latency_critical_alert_id_;
    std::chrono::steady_clock::time_point last_latency_warning_alert_time_;
    std::chrono::steady_clock::time_point last_latency_critical_alert_time_;

    // Helpers
    void fireAlert(ViolationLevel level, double hit_rate, uint64_t total_requests, EvaluationResult& result);
    void resolveActiveAlerts(EvaluationResult& result);
    observability::Alert buildAlert(ViolationLevel level, double hit_rate, uint64_t total_requests) const;
    bool isCooldownExpired(ViolationLevel level) const;
    static std::string makeAlertId(const std::string& cache_name, ViolationLevel level);

    // Latency alert helpers
    void fireLatencyAlert(ViolationLevel level, double p99_ms, EvaluationResult& result);
    void resolveLatencyAlerts(EvaluationResult& result);
    observability::Alert buildLatencyAlert(ViolationLevel level, double p99_ms) const;
    bool isLatencyCooldownExpired(ViolationLevel level) const;
    static std::string makeLatencyAlertId(const std::string& cache_name, ViolationLevel level);
};

} // namespace cache
} // namespace themis
