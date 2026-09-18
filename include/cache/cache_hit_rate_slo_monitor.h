/**
 * @file cache_hit_rate_slo_monitor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class CacheHitRateSloMonitor {
public:
    // -----------------------------------------------------------------------
    // Types
    // -----------------------------------------------------------------------

    enum class ViolationLevel {
        NONE,       ///< Hit rate is within SLO
        WARNING,    ///< Hit rate is below warning threshold
        CRITICAL    ///< Hit rate is below critical threshold
    };

    enum class Tier : std::size_t {
        L1 = 0, ///< In-process LRU tier
        L2 = 1, ///< Secondary (e.g. off-heap or Redis) tier
        L3 = 2, ///< Tertiary (e.g. disk) tier
        COUNT   ///< Sentinel – number of tiers
    };

    struct Config {
        double warning_threshold = 0.60;

        double critical_threshold = 0.40;

        uint64_t min_requests = 100;

        int alert_cooldown_seconds = 300;

        std::string cache_name = "adaptive_query_cache";

        double p99_warn_ms = 0.0;

        double p99_critical_ms = 0.0;

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
     * @brief Evaluate.
     * @param[in] metrics Input parameter.
     * @return Return value.
     */
    EvaluationResult evaluate(const CacheMetrics& metrics);

    /**
     * @brief Record Latency.
     * @param[in] tier Input parameter.
     * @param[in] latency_ms Input parameter.
     */
    void recordLatency(Tier tier, double latency_ms);

    // -----------------------------------------------------------------------
    // Status & Inspection
    // -----------------------------------------------------------------------

    /**
     * @brief Get Last Result.
     * @return Return value.
     */
    EvaluationResult getLastResult() const;

    /**
     * @brief Get Current Violation Level.
     * @return Return value.
     */
    ViolationLevel getCurrentViolationLevel() const;

    /**
     * @brief Is Slo Violated.
     * @return True when the operation succeeds.
     */
    bool isSloViolated() const;

    /**
     * @brief Get Status.
     * @return Return value.
     */
    nlohmann::json getStatus() const;

    /**
     * @brief Get Active Alert Ids.
     * @return Return value.
     */
    std::vector<std::string> getActiveAlertIds() const;

    /**
     * @brief Record Tenant Eviction Rate.
     * @param[in] tenant_id Identifier of the tenant.
     * @param[in] eviction_rate Input parameter.
     * @param[in] threshold Input parameter.
     * @return True when the operation succeeds.
     */
    bool recordTenantEvictionRate(const std::string& tenant_id,
                                   double eviction_rate,
                                   double threshold);

    /**
     * @brief Set Alertmanager.
     * @param[in] alertmanager Input parameter.
     */
    void setAlertmanager(std::shared_ptr<observability::Alertmanager> alertmanager);

    /**
     * @brief Violation Level To String.
     * @param[in] level Input parameter.
     * @return Return value.
     */
    static std::string violationLevelToString(ViolationLevel level);

private:
    // -----------------------------------------------------------------------
    // Rolling latency histogram (lock-free per-bucket atomics)
    // -----------------------------------------------------------------------

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
    /**
     * @brief Fire Alert.
     * @param[in] level Input parameter.
     * @param[in] hit_rate Input parameter.
     * @param[in] total_requests Input parameter.
     * @param[in,out] result Input/output parameter.
     */
    void fireAlert(ViolationLevel level, double hit_rate, uint64_t total_requests, EvaluationResult& result);
    /**
     * @brief Resolve Active Alerts.
     * @param[in,out] result Input/output parameter.
     */
    void resolveActiveAlerts(EvaluationResult& result);
    /**
     * @brief Build Alert.
     * @param[in] level Input parameter.
     * @param[in] hit_rate Input parameter.
     * @param[in] total_requests Input parameter.
     * @return Return value.
     */
    observability::Alert buildAlert(ViolationLevel level, double hit_rate, uint64_t total_requests) const;
    /**
     * @brief Is Cooldown Expired.
     * @param[in] level Input parameter.
     * @return True when the operation succeeds.
     */
    bool isCooldownExpired(ViolationLevel level) const;
    /**
     * @brief Make Alert Id.
     * @param[in] cache_name Name of the cache.
     * @param[in] level Input parameter.
     * @return Return value.
     */
    static std::string makeAlertId(const std::string& cache_name, ViolationLevel level);

    // Latency alert helpers
    /**
     * @brief Fire Latency Alert.
     * @param[in] level Input parameter.
     * @param[in] p99_ms Input parameter.
     * @param[in,out] result Input/output parameter.
     */
    void fireLatencyAlert(ViolationLevel level, double p99_ms, EvaluationResult& result);
    /**
     * @brief Resolve Latency Alerts.
     * @param[in,out] result Input/output parameter.
     */
    void resolveLatencyAlerts(EvaluationResult& result);
    /**
     * @brief Build Latency Alert.
     * @param[in] level Input parameter.
     * @param[in] p99_ms Input parameter.
     * @return Return value.
     */
    observability::Alert buildLatencyAlert(ViolationLevel level, double p99_ms) const;
    /**
     * @brief Is Latency Cooldown Expired.
     * @param[in] level Input parameter.
     * @return True when the operation succeeds.
     */
    bool isLatencyCooldownExpired(ViolationLevel level) const;
    /**
     * @brief Make Latency Alert Id.
     * @param[in] cache_name Name of the cache.
     * @param[in] level Input parameter.
     * @return Return value.
     */
    static std::string makeLatencyAlertId(const std::string& cache_name, ViolationLevel level);
};

} // namespace cache
} // namespace themis
