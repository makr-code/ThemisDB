/**
 * @file rate_limiter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class TokenBucket {
public:
    TokenBucket(size_t capacity, double refill_rate);
    
    bool tryConsume(size_t tokens = 1);
    
    /**
     * @brief Get Tokens.
     * @return Return value.
     */
    double getTokens() const;
    
    /**
     * @brief Get Retry After Ms.
     * @return Return value.
     */
    uint64_t getRetryAfterMs() const;
    
    /**
     * @brief Reset the modification detection flag.
     */
    void reset();

private:
    /**
     * @brief Refill.
     */
    void refill();
    
    size_t capacity_;
    double tokens_;
    double refill_rate_;
    std::chrono::steady_clock::time_point last_refill_;
    mutable std::shared_mutex mutex_;
};

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

using AnomalyCallback = std::function<void(const AnomalyEvent&)>;

class RateLimiter {
public:
    explicit RateLimiter(const RateLimitConfig& config = RateLimitConfig());
    
    bool allowRequest(const std::string& ip, const std::string& user_id = "");
    
    uint32_t getRetryAfter(const std::string& ip, const std::string& user_id = "") const;
    
    /**
     * @brief Is Whitelisted.
     * @param[in] ip Input parameter.
     * @return True when the operation succeeds.
     */
    bool isWhitelisted(const std::string& ip) const;
    
    /**
     * @brief Set Anomaly Callback.
     * @param[in] callback Input parameter.
     */
    void setAnomalyCallback(AnomalyCallback callback);

    /**
     * @brief Blacklist IP.
     * @param[in] ip Input parameter.
     */
    void blacklistIP(const std::string& ip);
    
    /**
     * @brief Unblacklist IP.
     * @param[in] ip Input parameter.
     */
    void unblacklistIP(const std::string& ip);
    
    /**
     * @brief Is Blacklisted.
     * @param[in] ip Input parameter.
     * @return True when the operation succeeds.
     */
    bool isBlacklisted(const std::string& ip) const;

    /**
     * @brief Is Adaptively Throttled.
     * @param[in] ip Input parameter.
     * @return True when the operation succeeds.
     */
    bool isAdaptivelyThrottled(const std::string& ip) const;
    
    /**
     * @brief Update the access control configuration.
     * @param[in] config New access control configuration.
     */
    void updateConfig(const RateLimitConfig& config);
    
    struct Statistics {
        size_t total_requests = 0;
        size_t allowed_requests = 0;
        size_t rejected_requests = 0;
        size_t active_ip_buckets = 0;
        size_t active_user_buckets = 0;
        size_t adaptive_throttle_penalties = 0; ///< IPs currently penalised
    };
    
    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    Statistics getStatistics() const;
    
    /**
     * @brief Reset the modification detection flag.
     */
    void reset();
    
    /**
     * @brief Cleanup.
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

    /**
     * @brief Record Rejection For Adaptive.
     * @param[in] ip Input parameter.
     */
    void recordRejectionForAdaptive(const std::string& ip);

    // Anomaly detection callback – protected by a dedicated mutex so that
    // fireAnomaly() can be called while mutex_ is held without risk of deadlock.
    mutable std::shared_mutex callback_mutex_;
    AnomalyCallback anomaly_callback_;
    /**
     * @brief Fire Anomaly.
     * @param[in] type Input parameter.
     * @param[in] ip Input parameter.
     * @param[in] detail Input parameter.
     */
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
