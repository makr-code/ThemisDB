/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cache_hit_rate_slo_monitor.h                       ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-02 03:52:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     252                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e62290192  2026-02-24  audit(cache): add Config::validate(), cooldown, label, an... ║
    • 5fdae26bd  2026-02-24  feat(cache): implement cache hit rate SLO alerting ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <atomic>
#include <chrono>
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

        /**
         * @brief Validate configuration parameters.
         *
         * Checks:
         * - `critical_threshold` < `warning_threshold` (critical is stricter)
         * - Both thresholds are in the range [0.0, 1.0]
         * - `alert_cooldown_seconds` >= 0
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
        bool alert_fired       = false;         ///< True if an alert was fired this call
        bool alert_resolved    = false;         ///< True if an alert was resolved this call
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
        const Config& config = Config(),
        std::shared_ptr<observability::Alertmanager> alertmanager = nullptr);

    ~CacheHitRateSloMonitor() = default;

    // Non-copyable, moveable
    CacheHitRateSloMonitor(const CacheHitRateSloMonitor&)            = delete;
    CacheHitRateSloMonitor& operator=(const CacheHitRateSloMonitor&) = delete;
    CacheHitRateSloMonitor(CacheHitRateSloMonitor&&)                 = default;
    CacheHitRateSloMonitor& operator=(CacheHitRateSloMonitor&&)      = default;

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
     *   "alerts": [ { "id": "cache_hit_rate_warning", "status": "FIRING" } ]
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
    // Internals
    // -----------------------------------------------------------------------

    Config config_;
    std::shared_ptr<observability::Alertmanager> alertmanager_;

    mutable std::mutex mutex_;

    // Last evaluation result
    EvaluationResult last_result_;

    // Alert state tracking
    ViolationLevel active_violation_ = ViolationLevel::NONE;
    std::string active_warning_alert_id_;
    std::string active_critical_alert_id_;
    std::chrono::steady_clock::time_point last_warning_alert_time_;
    std::chrono::steady_clock::time_point last_critical_alert_time_;

    // Helpers
    void fireAlert(ViolationLevel level, double hit_rate, uint64_t total_requests, EvaluationResult& result);
    void resolveActiveAlerts(EvaluationResult& result);
    observability::Alert buildAlert(ViolationLevel level, double hit_rate, uint64_t total_requests) const;
    bool isCooldownExpired(ViolationLevel level) const;
    static std::string makeAlertId(const std::string& cache_name, ViolationLevel level);
};

} // namespace cache
} // namespace themis
