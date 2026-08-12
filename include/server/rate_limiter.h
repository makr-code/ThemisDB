/**
 * @file rate_limiter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <memory>

namespace themis {
namespace server {

/**
 * @brief Token Bucket configuration for rate limiting
 */
struct RateLimitConfig {
    // Maximum number of tokens in bucket (burst capacity)
    size_t bucket_capacity = 100;
    
    // Tokens refilled per second
    double refill_rate = 100.0 / 60.0; // 100 requests per minute default
    
    // Time window for rate limit (seconds)
    uint32_t window_seconds = 60;
    
    // Enable per-IP rate limiting
    bool per_ip_enabled = true;
    
    // Enable per-user rate limiting (requires authentication)
    bool per_user_enabled = true;
    
    // Whitelist IPs (no rate limiting)
    std::vector<std::string> whitelist_ips;
    
    // Custom rate limits for specific IPs/users
    std::unordered_map<std::string, size_t> custom_limits;

    // ── Adaptive throttling ──────────────────────────────────────────────
    // When an IP exceeds adaptive_rejection_threshold rejections within
    // adaptive_window_seconds, each subsequent request must consume
    // 2 tokens instead of 1 (2x harder to pass) during the penalty window.
    // The penalty is removed after adaptive_penalty_duration_seconds.
    bool adaptive_throttling_enabled = false;
    uint32_t adaptive_rejection_threshold = 10;   ///< rejections that trigger penalty
    uint32_t adaptive_window_seconds      = 60;   ///< rolling window for counting
    double   adaptive_penalty_factor      = 0.25; ///< reserved for future use
    uint32_t adaptive_penalty_duration_seconds = 120; ///< how long the penalty lasts
};

/**
 * @brief Token Bucket for rate limiting
 * 
 * Implements the Token Bucket algorithm:
 * - Bucket holds tokens (capacity limit)
 * - Tokens refill at constant rate
 * - Each request consumes 1 token
 * - Request rejected if no tokens available
 */
class TokenBucket {
public:
    TokenBucket(size_t capacity, double refill_rate);
    
    /**
     * @brief Try to consume tokens from bucket
     * @param tokens Number of tokens to consume (default: 1)
     * @return true if tokens consumed, false if insufficient tokens
     */
    bool tryConsume(size_t tokens = 1);
    
    /**
     * @brief Get current token count
     */
    double getTokens() const;
    
    /**
     * @brief Get time until next token available (milliseconds)
     */
    uint64_t getRetryAfterMs() const;
    
    /**
     * @brief Reset bucket to full capacity
     */
    void reset();

private:
    void refill();
    
    size_t capacity_;
    double tokens_;
    double refill_rate_;
    std::chrono::steady_clock::time_point last_refill_;
    mutable std::shared_mutex mutex_;
};

/**
 * @brief Anomaly event fired by RateLimiter when suspicious behaviour is detected.
 *
 * Callers register a callback via RateLimiter::setAnomalyCallback() to receive
 * these events and forward them to their alerting / SIEM pipeline.
 */
struct AnomalyEvent {
    enum class Type {
        ADAPTIVE_THROTTLE_TRIGGERED, ///< IP exceeded rejection threshold; penalty applied
        IP_BLACKLISTED,              ///< IP was programmatically added to the blacklist
    };
    Type        type;
    std::string ip;
    std::string detail;
    std::chrono::system_clock::time_point timestamp;
};

/// Callback invoked (with mutex NOT held) on each detected anomaly.
using AnomalyCallback = std::function<void(const AnomalyEvent&)>;

/**
 * @brief Rate Limiter with per-IP and per-user tracking
 * 
 * Features:
 * - Token bucket algorithm for smooth rate limiting
 * - Per-IP and per-user buckets
 * - Configurable limits and whitelists
 * - Thread-safe
 * - Automatic cleanup of old buckets
 * - Anomaly detection callbacks for SIEM / alerting integration
 */
class RateLimiter {
public:
    explicit RateLimiter(const RateLimitConfig& config = RateLimitConfig());
    
    /**
     * @brief Check if request is allowed
     * @param ip Client IP address
     * @param user_id Optional user identifier (from JWT/auth)
     * @return true if request allowed, false if rate limit exceeded
     */
    bool allowRequest(const std::string& ip, const std::string& user_id = "");
    
    /**
     * @brief Get retry-after time in seconds for rate-limited client
     * @param ip Client IP address
     * @param user_id Optional user identifier
     * @return Seconds until next request allowed (0 if not rate limited)
     */
    uint32_t getRetryAfter(const std::string& ip, const std::string& user_id = "") const;
    
    /**
     * @brief Check if IP is whitelisted
     */
    bool isWhitelisted(const std::string& ip) const;
    
    /**
     * @brief Register a callback invoked whenever an anomaly is detected.
     *
     * The callback is invoked outside of the internal mutex so it is safe to
     * perform I/O (e.g. write to an audit log or send to a SIEM) without risk
     * of deadlock.  Pass nullptr or an empty function to deregister.
     */
    void setAnomalyCallback(AnomalyCallback callback);

    /**
     * @brief Add IP to blacklist (immediately block all requests from this IP)
     * @param ip IP address to block
     */
    void blacklistIP(const std::string& ip);
    
    /**
     * @brief Remove IP from blacklist
     * @param ip IP address to unblock
     */
    void unblacklistIP(const std::string& ip);
    
    /**
     * @brief Check if IP is blacklisted
     * @param ip IP address to check
     */
    bool isBlacklisted(const std::string& ip) const;

    /**
     * @brief Return true if an IP is currently under an adaptive throttle penalty.
     * @param ip IP address to check.
     */
    bool isAdaptivelyThrottled(const std::string& ip) const;
    
    /**
     * @brief Update configuration at runtime
     */
    void updateConfig(const RateLimitConfig& config);
    
    /**
     * @brief Get current statistics
     */
    struct Statistics {
        size_t total_requests = 0;
        size_t allowed_requests = 0;
        size_t rejected_requests = 0;
        size_t active_ip_buckets = 0;
        size_t active_user_buckets = 0;
        size_t adaptive_throttle_penalties = 0; ///< IPs currently penalised
    };
    
    Statistics getStatistics() const;
    
    /**
     * @brief Clear all buckets (for testing)
     */
    void reset();
    
    /**
     * @brief Cleanup old inactive buckets (called periodically)
     */
    void cleanup();

private:
    std::shared_ptr<TokenBucket> getOrCreateBucket(
        const std::string& key,
        std::unordered_map<std::string, std::shared_ptr<TokenBucket>>& buckets
    );
    
    RateLimitConfig config_;
    
    // Per-IP buckets
    std::unordered_map<std::string, std::shared_ptr<TokenBucket>> ip_buckets_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> ip_last_access_;
    
    // Per-user buckets
    std::unordered_map<std::string, std::shared_ptr<TokenBucket>> user_buckets_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> user_last_access_;
    
    // IP blacklist (blocked regardless of rate limit)
    std::unordered_set<std::string> blacklisted_ips_;

    // ── Adaptive throttling state ────────────────────────────────────────
    struct AdaptiveEntry {
        // Timestamps of recent rejections within the rolling window
        std::vector<std::chrono::steady_clock::time_point> rejection_times;
        // If non-zero, IP is currently penalised
        std::chrono::steady_clock::time_point penalty_until;
        bool under_penalty = false;
    };
    std::unordered_map<std::string, AdaptiveEntry> adaptive_state_;

    void recordRejectionForAdaptive(const std::string& ip);

    // Anomaly detection callback – protected by a dedicated mutex so that
    // fireAnomaly() can be called while mutex_ is held without risk of deadlock.
    mutable std::shared_mutex callback_mutex_;
    AnomalyCallback anomaly_callback_;
    // Fire the anomaly callback (safe to call while mutex_ is held).
    void fireAnomaly(AnomalyEvent::Type type, const std::string& ip, const std::string& detail) const;
    
    // Statistics
    mutable Statistics stats_;
    
    mutable std::shared_mutex mutex_;
    
    // Cleanup interval (5 minutes)
    static constexpr uint32_t CLEANUP_INTERVAL_SECONDS = 300;
    std::chrono::steady_clock::time_point last_cleanup_;
};

} // namespace server
} // namespace themis
