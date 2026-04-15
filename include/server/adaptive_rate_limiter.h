/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adaptive_rate_limiter.h                            ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 04:13:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     230                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e963d4e9ba  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 71d99c4f28  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • b09d74434e  2026-03-13  fix(server): address all code review comments on rate lim... ║
    • 855ed0268a  2026-03-13  feat(server): add adaptive and cost-based rate limiters f... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace server {

/**
 * @brief Backend health metrics sample used by AdaptiveRateLimiter.
 *
 * Call AdaptiveRateLimiter::recordSample() each time a backend request
 * completes to supply fresh latency and error-rate data.
 */
struct BackendHealthSample {
    /// Observed end-to-end latency for one backend call.
    std::chrono::milliseconds latency_ms{0};

    /// Whether this backend call resulted in an error.
    bool is_error = false;
};

/**
 * @brief Adaptive Rate Limiter — automatically adjusts rate limits based
 *        on observed backend health (latency + error rate).
 *
 * ### Algorithm
 * A sliding window (default: 60 s) accumulates BackendHealthSample
 * observations.  After each batch of @c min_samples_to_adapt samples, the
 * current effective capacity is recalculated:
 *
 *   - p99 latency > high_latency_threshold_ms  → reduce to 50 %
 *   - error rate  > high_error_rate            → reduce to 20 %
 *   - p99 latency < low_latency_threshold_ms
 *     AND error rate < low_error_rate          → increase by recovery_step
 *     (capped at base_capacity)
 *
 * ### Per-tenant support
 * Call allowRequest() with a non-empty @c tenant_id to get independent
 * adaptive limits per tenant.  Each tenant starts at @c base_capacity and
 * is adjusted independently.
 *
 * ### Thread-safety
 * All public methods are thread-safe.
 *
 * ### Example
 * @code
 *   AdaptiveRateLimiter::Config cfg;
 *   cfg.base_capacity             = 1000;  // Normal: 1 000 req/min
 *   cfg.high_latency_threshold_ms = 500;   // p99 > 500 ms → reduce
 *   cfg.high_error_rate           = 0.05;  // errors > 5 % → reduce
 *   AdaptiveRateLimiter limiter(cfg);
 *
 *   // In request handler:
 *   if (!limiter.allowRequest("tenant_a")) {
 *       return HTTP_429;
 *   }
 *
 *   // After backend call:
 *   BackendHealthSample sample;
 *   sample.latency_ms = latency;
 *   sample.is_error   = had_error;
 *   limiter.recordSample("tenant_a", sample);
 * @endcode
 */
class AdaptiveRateLimiter {
public:
    struct Config {
        /// Normal-operation token capacity (req per window).
        size_t base_capacity = 1000;

        /// p99 latency (ms) above which rate is halved.
        uint64_t high_latency_threshold_ms = 500;

        /// p99 latency (ms) below which rate can recover.
        uint64_t low_latency_threshold_ms = 100;

        /// Error rate (0.0–1.0) above which rate is reduced to 20 %.
        double high_error_rate = 0.05;

        /// Error rate (0.0–1.0) below which rate can recover.
        double low_error_rate = 0.01;

        /// Fraction of base_capacity to step up during recovery per window.
        double recovery_step = 0.1;

        /// Size of the sliding observation window (seconds).
        uint32_t window_seconds = 60;

        /// Minimum number of samples in the window before the capacity is
        /// adjusted; prevents over-reacting on sparse traffic.
        size_t min_samples_to_adapt = 10;
    };

    explicit AdaptiveRateLimiter(const Config& config);

    /**
     * @brief Record one backend health sample.
     *
     * This must be called after every backend call so the limiter can track
     * latency and errors.  If @c tenant_id is empty the sample updates the
     * global (default) tenant state.
     *
     * @param tenant_id  Tenant identifier (empty = global).
     * @param sample     Observed latency and error flag.
     */
    void recordSample(const std::string& tenant_id,
                      const BackendHealthSample& sample);

    /**
     * @brief Check whether a request for @c tenant_id should be allowed.
     *
     * Consumes one token from the tenant's adaptive bucket.  If the bucket
     * is empty the request is rejected (returns false).
     *
     * @param tenant_id  Tenant identifier (empty = global).
     * @return true if the request is allowed, false if rate-limited.
     */
    bool allowRequest(const std::string& tenant_id = "");

    /**
     * @brief Return the current effective capacity for @c tenant_id.
     */
    size_t getCurrentCapacity(const std::string& tenant_id = "") const;

    /**
     * @brief Total requests seen (allowed + rejected).
     */
    uint64_t getTotalRequests() const {
        return total_requests_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Total requests rejected due to rate limiting.
     */
    uint64_t getTotalRejections() const {
        return total_rejections_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Reset all tenant states and metrics (for testing).
     */
    void reset();

private:
    // ── Internal types ───────────────────────────────────────────────────────

    struct TimedSample {
        std::chrono::steady_clock::time_point ts;
        std::chrono::milliseconds latency_ms;
        bool is_error;
    };

    struct TenantState {
        /// Rolling window of raw samples.
        std::vector<TimedSample> window;

        /// Current token count (adaptive capacity).
        size_t current_capacity;

        /// Available tokens in the current window.
        size_t available_tokens;

        /// Start of the current token-replenishment window.
        std::chrono::steady_clock::time_point window_start;

        explicit TenantState(size_t base_cap)
            : current_capacity(base_cap)
            , available_tokens(base_cap)
            , window_start(std::chrono::steady_clock::now())
        {}
    };

    // ── Helpers ───────────────────────────────────────────────────────────────

    /// Prune samples older than window_seconds and recompute the effective
    /// capacity for @p state.  Must be called with tenants_mutex_ held.
    void pruneAndAdapt(TenantState& state);

    /// Compute the 99th-percentile latency over @p samples.
    static std::chrono::milliseconds computeP99(
        const std::vector<TimedSample>& samples);

    /// Compute the error rate over @p samples.
    static double computeErrorRate(const std::vector<TimedSample>& samples);

    // ── State ─────────────────────────────────────────────────────────────────

    Config config_;

    mutable std::shared_mutex tenants_mutex_;
    std::unordered_map<std::string, TenantState> tenants_;

    std::atomic<uint64_t> total_requests_{0};
    std::atomic<uint64_t> total_rejections_{0};
};

} // namespace server
} // namespace themis
