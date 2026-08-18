/**
 * @file adaptive_rate_limiter.h
 * @brief Adaptive rate limiting for API connectors with backpressure support.
 *
 * Phase 2.9 (Bounded Resources + Stress Tests) — ING-IMPL-008
 *
 * Provides:
 * - Rate limit header parsing (RateLimit-Limit, RateLimit-Remaining, Retry-After)
 * - Adaptive throttling based on API responses
 * - Graceful degradation when rate limits are exceeded
 * - Per-connector rate limit tracking
 *
 * @see src/ingestion/ROADMAP.md — Phase 2.9 item
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace ingestion {

// ============================================================================
// Rate limit header parsing
// ============================================================================

/**
 * @brief Parsed rate limit information from HTTP response headers.
 *
 * Extracted from common rate limit headers:
 * - RateLimit-Limit: total request limit
 * - RateLimit-Remaining: requests remaining in current window
 * - RateLimit-Reset: Unix timestamp when limit resets
 * - Retry-After: seconds to wait before retrying
 * - X-RateLimit-* variants (GitHub, other APIs)
 */
struct RateLimitInfo {
    bool is_rate_limited = false;           ///< true if rate-limited
    std::uint64_t limit_per_window = 0;     ///< Total requests allowed per window
    std::uint64_t remaining_in_window = 0;  ///< Requests still available
    std::uint64_t reset_timestamp = 0;      ///< Unix timestamp when limit resets
    std::chrono::seconds retry_after{0};    ///< Time to wait before retry
    std::string header_source;              ///< Which header was parsed (for debugging)
};

/**
 * @brief Parse rate limit information from HTTP response headers.
 *
 * Attempts to parse common rate limit header formats:
 * - RateLimit-Limit, RateLimit-Remaining, RateLimit-Reset (standard)
 * - X-RateLimit-Limit, X-RateLimit-Remaining, X-RateLimit-Reset (GitHub, etc.)
 * - Retry-After (standard; may be seconds or HTTP-date)
 *
 * @param response_headers Map of header name -> value pairs
 * @return RateLimitInfo with is_rate_limited set based on headers found
 */
RateLimitInfo parseRateLimitHeaders(
    const std::map<std::string, std::string>& response_headers);

// ============================================================================
// Adaptive rate limiter configuration
// ============================================================================

/**
 * @brief Configuration for adaptive rate limiting behavior.
 *
 * Controls how the rate limiter responds to API rate limits and throttles
 * requests to maintain sustainable throughput.
 */
struct AdaptiveRateLimiterConfig {
    /**
     * @brief Initial requests per second limit (before adaptive adjustment).
     *
     * Default: 10 requests/sec.
     */
    double initial_requests_per_sec = 10.0;

    /**
     * @brief Minimum requests per second (floor for adaptive backoff).
     *
     * When the rate limiter reduces throughput due to 429 responses,
     * it will not go below this rate. Default: 0.1 requests/sec (1 req/10s).
     */
    double min_requests_per_sec = 0.1;

    /**
     * @brief Maximum requests per second (ceiling for adaptive speedup).
     *
     * When the rate limiter detects spare capacity, it gradually
     * increases throughput, but not beyond this rate.
     * Default: 100 requests/sec.
     */
    double max_requests_per_sec = 100.0;

    /**
     * @brief Multiplicative backoff factor when rate limit is hit.
     *
     * When a 429 (Too Many Requests) response is received,
     * the rate limit is multiplied by this factor (should be < 1.0).
     * Default: 0.5 (cut in half).
     */
    double backoff_multiplier = 0.5;

    /**
     * @brief Multiplicative speedup factor when capacity is available.
     *
     * When the API has spare capacity (indicated by high Remaining count),
     * the rate limit is multiplied by this factor (should be > 1.0).
     * Default: 1.1 (increase by 10%).
     */
    double speedup_multiplier = 1.1;

    /**
     * @brief Measurement window for evaluating capacity (in seconds).
     *
     * The rate limiter tracks success/failure rates over this window
     * to decide when to speed up or slow down. Default: 60 seconds.
     */
    std::chrono::seconds measurement_window{60};

    /**
     * @brief Enable honor-Retry-After header from API responses.
     *
     * When true, if an API response includes a Retry-After header,
     * wait that long before retrying. Default: true.
     */
    bool honor_retry_after = true;

    /**
     * @brief Maximum number of consecutive 429 responses before failing.
     *
     * If this many 429 responses occur in a row, the rate limiter
     * gives up and returns an error instead of retrying.
     * Default: 10.
     */
    int max_consecutive_throttles = 10;
};

// ============================================================================
// Adaptive rate limiter for HTTP-based connectors
// ============================================================================

/**
 * @brief Adaptive rate limiter that adjusts throughput based on API feedback.
 *
 * Monitors HTTP response status codes and rate limit headers to dynamically
 * adjust the request rate. Provides both blocking (wait) and non-blocking
 * (fail) semantics for rate-limited conditions.
 *
 * Example usage:
 * @code
 * AdaptiveRateLimiterConfig config;
 * config.initial_requests_per_sec = 5.0;
 * AdaptiveRateLimiter limiter("my_api", config);
 *
 * // Before making a request
 * if (!limiter.tryAcquireToken(true)) {
 *     // Rate limited; decide whether to retry or fail
 * }
 *
 * // After receiving response
 * limiter.recordResponse(200, response_headers);  // Success
 * // or
 * limiter.recordResponse(429, response_headers);  // Too Many Requests
 *
 * // Monitor current rate
 * auto stats = limiter.getStats();
 * std::cout << "Current rate: " << stats.current_rate_limit << " req/s" << std::endl;
 * @endcode
 */
class AdaptiveRateLimiter {
public:
    /**
     * @brief Statistics for a rate limiter instance.
     */
    struct Stats {
        double current_rate_limit = 0.0;     ///< Current rate limit (req/sec)
        double observed_success_rate = 0.0;  ///< Fraction of requests that succeeded
        std::uint64_t total_requests = 0;    ///< Total requests tracked
        std::uint64_t successful_requests = 0; ///< Successful requests (not 429)
        std::uint64_t throttled_requests = 0;  ///< Requests blocked due to rate limit
        int consecutive_throttles = 0;       ///< Current streak of 429 responses
    };

    /**
     * @brief Construct a rate limiter for a specific connector/API.
     * @param connector_name Name of the connector (for logging/debugging)
     * @param config Rate limiting configuration
     */
    AdaptiveRateLimiter(const std::string& connector_name,
                        const AdaptiveRateLimiterConfig& config = {})
        : connector_name_(connector_name), config_(config),
          current_rate_limit_(config.initial_requests_per_sec) {}

    ~AdaptiveRateLimiter() = default;

    // Delete copy/move
    AdaptiveRateLimiter(const AdaptiveRateLimiter&) = delete;
    AdaptiveRateLimiter& operator=(const AdaptiveRateLimiter&) = delete;

    // ── Token bucket rate control ───────────────────────────────────────────

    /**
     * @brief Attempt to acquire a request token.
     *
     * Uses a token bucket algorithm to enforce the current rate limit.
     * If allow_wait is true, blocks until a token is available.
     * If allow_wait is false, returns immediately.
     *
     * @param allow_wait If true, block until token available; if false, return immediately
     * @return true if token acquired; false if rate-limited and not waiting
     */
    bool tryAcquireToken(bool allow_wait = false);

    /**
     * @brief Acquire a token (blocks indefinitely until available).
     *
     * Useful for simple cases where blocking is acceptable.
     */
    void acquireToken() {
        while (!tryAcquireToken(true)) {
            // Retry until successful
        }
    }

    // ── Response feedback ───────────────────────────────────────────────────

    /**
     * @brief Record an HTTP response for feedback-based rate adjustment.
     *
     * Parses rate limit headers from the response and adjusts the current
     * rate limit accordingly. Call after each HTTP request.
     *
     * @param http_status_code HTTP status code (200, 429, 503, etc.)
     * @param response_headers  Response headers (used to parse rate limit info)
     */
    void recordResponse(int http_status_code,
                       const std::map<std::string, std::string>& response_headers = {});

    // ── Configuration and state ─────────────────────────────────────────────

    /**
     * @brief Get current rate limiter statistics.
     */
    Stats getStats() const;

    /**
     * @brief Manually set the rate limit (overrides adaptive adjustment).
     *
     * Useful for explicit configuration or recovery from cascading failures.
     * @param requests_per_sec New rate limit (will be clamped to min/max)
     */
    void setRateLimit(double requests_per_sec);

    /**
     * @brief Get the current rate limit.
     */
    double getRateLimit() const;

    /**
     * @brief Reset to initial rate limit and clear failure history.
     *
     * Call this to recover from temporary overload conditions.
     */
    void resetToInitial();

    /**
     * @brief Get the connector name.
     */
    const std::string& getConnectorName() const { return connector_name_; }

    /**
     * @brief Set the configuration (takes effect on next token acquisition).
     */
    void setConfig(const AdaptiveRateLimiterConfig& new_config) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_ = new_config;
    }

    /**
     * @brief Get the current configuration.
     */
    AdaptiveRateLimiterConfig getConfig() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_;
    }

private:
    mutable std::mutex mutex_;
    std::string connector_name_;
    AdaptiveRateLimiterConfig config_;
    double current_rate_limit_;

    // Token bucket state
    std::chrono::steady_clock::time_point last_token_time_;
    double tokens_available_ = 0.0;

    // Statistics
    Stats stats_;
    std::chrono::steady_clock::time_point measurement_window_start_;

    /**
     * @brief Refill token bucket based on elapsed time.
     *
     * Called internally by tryAcquireToken.
     */
    void refillTokens();

    /**
     * @brief Update statistics based on a successful response.
     */
    void updateStatsSuccess();

    /**
     * @brief Update statistics based on a throttled response (429).
     */
    void updateStatsThrottle();

    /**
     * @brief Adjust rate limit based on API feedback.
     */
    void adjustRateLimit(const RateLimitInfo& info);
};

// ============================================================================
// Rate limiter pool for managing multiple connectors
// ============================================================================

/**
 * @brief Thread-safe pool of rate limiters for multiple API connectors.
 *
 * Provides a convenient way to manage rate limiters for different
 * connector types or API endpoints.
 */
class RateLimiterPool {
public:
    /**
     * @brief Construct an empty rate limiter pool.
     */
    RateLimiterPool() = default;

    ~RateLimiterPool() = default;

    /**
     * @brief Register a rate limiter for a connector.
     * @param connector_name Unique name of the connector
     * @param config Rate limiting configuration
     * @return Reference to the newly created rate limiter
     */
    AdaptiveRateLimiter& registerLimiter(const std::string& connector_name,
                                          const AdaptiveRateLimiterConfig& config = {});

    /**
     * @brief Get the rate limiter for a connector.
     * @param connector_name Name of the connector
     * @return Pointer to the rate limiter, or nullptr if not registered
     */
    AdaptiveRateLimiter* getLimiter(const std::string& connector_name);

    /**
     * @brief Get the rate limiter for a connector (const version).
     */
    const AdaptiveRateLimiter* getLimiter(const std::string& connector_name) const;

    /**
     * @brief Unregister and remove a rate limiter.
     * @return true if the limiter was found and removed
     */
    bool unregisterLimiter(const std::string& connector_name);

    /**
     * @brief Get list of all registered connector names.
     */
    std::vector<std::string> listLimiters() const;

    /**
     * @brief Clear all rate limiters.
     */
    void clear();

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::unique_ptr<AdaptiveRateLimiter>> limiters_;
};

}  // namespace ingestion
}  // namespace themis

#endif  // THEMISDB_INCLUDE_INGESTION_ADAPTIVE_RATE_LIMITER_H
