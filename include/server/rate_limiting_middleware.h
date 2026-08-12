/**
 * @file rate_limiting_middleware.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "server/rate_limiter_v2.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace themis {
namespace server {

/**
 * @brief Rate limiting middleware with configurable per-client token bucket.
 *
 * Provides HTTP-request-level rate limiting with:
 *  - Per-client (IP or authenticated user) token bucket tracking via PerClientRateLimiter
 *  - Per-endpoint limit overrides (e.g., tighter limits for bulk-write paths)
 *  - IP whitelist bypass
 *  - Standard rate limit response headers (X-RateLimit-Limit, X-RateLimit-Remaining,
 *    X-RateLimit-Reset, Retry-After)
 *
 * ### Typical usage in the HTTP server
 * ```cpp
 * RateLimitingMiddleware rl_mw(RateLimitingMiddleware::Config{
 *     .default_capacity    = 100,
 *     .default_refill_rate = 100.0 / 60.0,  // 100 req/min
 *     .whitelist_ips       = {"127.0.0.1", "::1"},
 * });
 *
 * // In request handler:
 * auto result = rl_mw.check(client_ip_or_user_id, req_path);
 * if (!result.allowed) {
 *     return make_429_response(result.retry_after_seconds, result.headers);
 * }
 * // Attach informational headers to the successful response:
 * for (auto& [k, v] : result.headers) { response.set(k, v); }
 * ```
 *
 * ### Thread safety
 * All public methods are thread-safe.
 */
class RateLimitingMiddleware {
public:
    /**
     * @brief Per-endpoint rate limit override.
     *
     * Requests whose path starts with `path_prefix` use `capacity` and
     * `refill_rate` instead of the defaults.  The most-specific (longest)
     * matching prefix wins.
     */
    struct EndpointLimit {
        std::string path_prefix;   ///< e.g. "/v2/documents", "/api/bulk"
        size_t      capacity;      ///< token bucket size (burst)
        double      refill_rate;   ///< tokens per second
    };

    /**
     * @brief Configuration for RateLimitingMiddleware.
     */
    struct Config {
        /// Default per-client burst capacity (tokens).
        size_t default_capacity    = 100;

        /// Default per-client refill rate (tokens per second).
        /// Default: 100 req/min ≈ 1.67 req/s.
        double default_refill_rate = 100.0 / 60.0;

        /// Per-endpoint overrides; evaluated in longest-prefix order.
        std::vector<EndpointLimit> endpoint_overrides;

        /// IP addresses exempt from rate limiting (bypass entirely).
        std::vector<std::string> whitelist_ips;

        /// Maximum distinct clients tracked simultaneously.
        /// Prevents memory exhaustion under adversarial traffic.
        size_t max_clients = 10000;

        /// Attach X-RateLimit-* and Retry-After headers to all responses.
        bool send_rate_limit_headers = true;

        Config() = default;
    };

    /**
     * @brief Result of a check() call.
     */
    struct CheckResult {
        bool     allowed              = true;
        uint32_t retry_after_seconds  = 0;   ///< 0 when request is allowed
        size_t   remaining_tokens     = 0;   ///< tokens remaining after this request
        size_t   limit                = 0;   ///< effective bucket capacity for this endpoint

        /// HTTP headers to attach to the response.
        std::unordered_map<std::string, std::string> headers;
    };

    /** @brief Construct with default configuration. */
    RateLimitingMiddleware();

    /** @brief Construct with a custom configuration. */
    explicit RateLimitingMiddleware(const Config& config);

    /**
     * @brief Decide whether a request is allowed.
     *
     * @param client_key  Client identifier: IP address or authenticated user ID.
     * @param path        Request path used to select per-endpoint limit overrides.
     * @param cost        Token cost of this request (default 1).
     * @return CheckResult with decision, rate-limit headers, and retry guidance.
     */
    CheckResult check(const std::string& client_key,
                      const std::string& path,
                      size_t cost = 1);

    /**
     * @brief Replace the current configuration at runtime.
     *
     * Existing per-client bucket state is cleared so all clients start fresh
     * under the new limits.
     */
    void updateConfig(const Config& config);

    /** @brief Return a copy of the current configuration. */
    Config getConfig() const;

    /**
     * @brief Aggregate statistics since construction or last reset().
     */
    struct Stats {
        uint64_t total_requests    = 0;
        uint64_t allowed_requests  = 0;
        uint64_t rejected_requests = 0;
        size_t   active_clients    = 0;
    };

    Stats getStats() const;

    /**
     * @brief Reset all per-client state and counters.
     *
     * Intended for testing; in production prefer updateConfig() which also
     * resets state.
     */
    void reset();

private:
    /// Find the index of the most-specific matching endpoint override for `path`.
    /// Returns config_.endpoint_overrides.size() (sentinel) when no override matches.
    /// config_mutex_ must be held by the caller.
    std::size_t findOverrideIndex(const std::string& path) const;

    /// Return the effective (capacity, refill_rate) for the given request path.
    /// config_mutex_ must be held by the caller.
    std::pair<size_t, double> limitForPath(const std::string& path) const;

    /// Rebuild per-endpoint PerClientRateLimiter instances from config_.
    /// Must be called with config_mutex_ held.
    void rebuildLimiters();

    // ── State ────────────────────────────────────────────────────────────
    mutable std::mutex config_mutex_;
    Config config_;

    // Whitelist set for O(1) lookup
    std::unordered_set<std::string> whitelist_set_;

    // Default per-client limiter (for requests that don't match any override).
    std::unique_ptr<PerClientRateLimiter> default_limiter_;

    // Per-endpoint override limiters, parallel to config_.endpoint_overrides.
    std::vector<std::unique_ptr<PerClientRateLimiter>> override_limiters_;

    // Aggregate counters (updated without the config_mutex_ held).
    std::atomic<uint64_t> total_requests_{0};
    std::atomic<uint64_t> allowed_requests_{0};
    std::atomic<uint64_t> rejected_requests_{0};
};

} // namespace server
} // namespace themis
