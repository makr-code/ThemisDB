/**
 * @file auth_rate_limiter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "auth/rate_limiter_backend.h"
#include "server/rate_limiter.h"
#include "utils/audit_logger.h"
#include <string>
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <shared_mutex>
#include <chrono>
#include <memory>
#include <optional>
#include <functional>
#include <cstdint>

namespace themis {
namespace auth {

// Forward declaration to avoid circular includes.
class AuthMetrics;

enum class CredentialStuffingOutcome {
    ALLOWED,           ///< No special action required
    CAPTCHA_REQUIRED,  ///< First breach: challenge with CAPTCHA
    OTP_REQUIRED,      ///< Second breach: require email OTP
    ACCOUNT_LOCKED_24H ///< Third+ breach: lock account for 24 hours
};

struct AuthRateLimitConfig {
    // Per-IP rate limit for auth attempts
    size_t max_attempts_per_ip_per_minute = 10;
    
    // Per-user rate limit for auth attempts
    size_t max_attempts_per_user_per_minute = 5;
    
    // Account lockout configuration
    // Compatibility alias for the historical name used by older auth tests.
    // Prefer lockout_failed_attempts for new code; the effective value resolves
    // to the non-default legacy alias when it is explicitly set.
    size_t lockout_failed_attempts = 5;        // Lock after N failed attempts
    size_t max_failures_before_lockout = 5;    // Deprecated alias kept for compatibility
    std::chrono::minutes lockout_window{15};   // Within this time window
    std::chrono::minutes lockout_duration{15}; // Lock for this duration
    
    // Enable/disable features
    bool enable_ip_rate_limiting = true;
    bool enable_user_rate_limiting = true;
    bool enable_account_lockout = true;
    
    // Whitelist IPs (no rate limiting)
    std::vector<std::string> whitelist_ips;

    // ── Credential-stuffing detection ────────────────────────────────────
    // When a single IP attempts authentication against at least
    // credential_stuffing_user_threshold distinct usernames within
    // credential_stuffing_window_seconds, a CREDENTIAL_STUFFING_SUSPECTED
    // anomaly event is fired.
    bool   enable_credential_stuffing_detection = true;
    size_t credential_stuffing_user_threshold   = 10;  ///< distinct usernames per IP
    uint32_t credential_stuffing_window_seconds = 60;  ///< rolling window (seconds)

    // ── Credential-stuffing persistent backend (Redis) ───────────────────
    // When true, per-user breach counts are persisted in Redis under the key
    // namespace "cs:{user_id}:{YYYYMMDD}" with a 25-hour TTL.  This enables
    // cross-session, cross-restart detection and supports the exponential
    // back-off escalation policy.  Falls back to an in-process counter map
    // when Redis is unavailable or when THEMIS_ENABLE_REDIS is not defined.
    bool enable_cs_persistent_backend = false;

    struct CredentialStuffingRedisConfig {
        std::string host        = "127.0.0.1";
        int         port        = 6379;
        std::string auth;                    ///< empty = no AUTH
        int         timeout_ms  = 5000;
    };
    CredentialStuffingRedisConfig cs_redis;
};

struct AuthAnomalyEvent {
    enum class Type {
        BRUTE_FORCE_DETECTED,           ///< Account locked after repeated failures from an IP
        CREDENTIAL_STUFFING_SUSPECTED,  ///< Many distinct usernames tried from one IP
        ACCOUNT_LOCKOUT_TRIGGERED,      ///< Account locked due to failed-attempt threshold
    };
    Type        type;
    std::string ip;
    std::string user_id;   ///< empty for IP-level events
    std::string detail;
    std::chrono::system_clock::time_point timestamp;

    CredentialStuffingOutcome cs_outcome = CredentialStuffingOutcome::ALLOWED;
};

using AuthAnomalyCallback = std::function<void(const AuthAnomalyEvent&)>;

struct FailedAttempt {
    std::chrono::system_clock::time_point timestamp;
    std::string ip_address;
    std::string user_id;
    std::string reason;  // e.g., "invalid_password", "invalid_token"
};

struct LockoutInfo {
    bool is_locked = false;
    std::chrono::system_clock::time_point locked_until;
    size_t failed_attempts = 0;
    std::chrono::system_clock::time_point first_failure;
    std::chrono::system_clock::time_point last_failure;
    std::vector<FailedAttempt> recent_failures;
};

class AccountLockoutManager {
public:
    /**
     * @brief Account Lockout Manager.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit AccountLockoutManager(const AuthRateLimitConfig& config);
    
    /**
     * @brief Record Failed Attempt.
     * @param[in] user_id Identifier of the user.
     * @param[in] ip_address Input parameter.
     * @param[in] reason Input parameter.
     * @return True when the operation succeeds.
     */
    bool recordFailedAttempt(
        const std::string& user_id,
        const std::string& ip_address,
        const std::string& reason
    );
    
    /**
     * @brief Record Successful Auth.
     * @param[in] user_id Identifier of the user.
     */
    void recordSuccessfulAuth(const std::string& user_id);
    
    /**
     * @brief Is Account Locked.
     * @param[in] user_id Identifier of the user.
     * @return True when the operation succeeds.
     */
    bool isAccountLocked(const std::string& user_id) const;
    
    /**
     * @brief Get Lockout Info.
     * @param[in] user_id Identifier of the user.
     * @return Return value.
     */
    std::optional<LockoutInfo> getLockoutInfo(const std::string& user_id) const;
    
    /**
     * @brief Unlock Account.
     * @param[in] user_id Identifier of the user.
     * @return True when the operation succeeds.
     */
    bool unlockAccount(const std::string& user_id);
    
    /**
     * @brief Get Locked Account Count.
     * @return Return value.
     */
    size_t getLockedAccountCount() const;

    /**
     * @brief Force Lock Account.
     * @param[in] user_id Identifier of the user.
     * @param[in] duration Input parameter.
     */
    void forceLockAccount(const std::string& user_id, std::chrono::seconds duration);
    
    /**
     * @brief Cleanup.
     */
    void cleanup();
    
    /**
     * @brief Reset the modification detection flag.
     */
    void reset();

private:
    /**
     * @brief Lock Account.
     * @param[in] user_id Identifier of the user.
     * @param[in] info Input parameter.
     */
    void lockAccount(const std::string& user_id, const LockoutInfo& info);
    /**
     * @brief Should Lock Account.
     * @param[in] info Input parameter.
     * @return True when the operation succeeds.
     */
    bool shouldLockAccount(const LockoutInfo& info) const;
    
    AuthRateLimitConfig config_;
    
    // Per-user lockout state
    std::unordered_map<std::string, LockoutInfo> lockout_state_;
    
    mutable std::shared_mutex mutex_;
    
    // Cleanup interval (5 minutes)
    static constexpr uint32_t CLEANUP_INTERVAL_SECONDS = 300;
    std::chrono::steady_clock::time_point last_cleanup_;
};

class AuthRateLimiter {
public:
    explicit AuthRateLimiter(const AuthRateLimitConfig& config = AuthRateLimitConfig());
    
    bool allowAuthAttempt(
        const std::string& ip_address,
        const std::string& user_id = ""
    );
    
    /**
     * @brief Record Failed Auth.
     * @param[in] user_id Identifier of the user.
     * @param[in] ip_address Input parameter.
     * @param[in] reason Input parameter.
     */
    void recordFailedAuth(
        const std::string& user_id,
        const std::string& ip_address,
        const std::string& reason
    );
    
    /**
     * @brief Record Successful Auth.
     * @param[in] user_id Identifier of the user.
     * @param[in] ip_address Input parameter.
     */
    void recordSuccessfulAuth(
        const std::string& user_id,
        const std::string& ip_address
    );
    
    /**
     * @brief Is Account Locked.
     * @param[in] user_id Identifier of the user.
     * @return True when the operation succeeds.
     */
    bool isAccountLocked(const std::string& user_id) const;
    
    /**
     * @brief Get Lockout Info.
     * @param[in] user_id Identifier of the user.
     * @return Return value.
     */
    std::optional<LockoutInfo> getLockoutInfo(const std::string& user_id) const;
    
    /**
     * @brief Unlock Account.
     * @param[in] user_id Identifier of the user.
     * @return True when the operation succeeds.
     */
    bool unlockAccount(const std::string& user_id);
    
    /**
     * @brief Get Retry After.
     * @param[in] ip_address Input parameter.
     * @return Return value.
     */
    uint32_t getRetryAfter(const std::string& ip_address) const;
    
    /**
     * @brief Is Whitelisted.
     * @param[in] ip_address Input parameter.
     * @return True when the operation succeeds.
     */
    bool isWhitelisted(const std::string& ip_address) const;

    /**
     * @brief Set Anomaly Callback.
     * @param[in] callback Input parameter.
     */
    void setAnomalyCallback(AuthAnomalyCallback callback);

    /**
     * @brief Set Audit Logger.
     * @param[in,out] logger Input/output parameter.
     */
    void setAuditLogger(utils::AuditLogger* logger);

    /**
     * @brief Set Backend.
     * @param[in] backend Input parameter.
     */
    void setBackend(std::shared_ptr<IRateLimiterBackend> backend);

    /**
     * @brief Set Metrics.
     * @param[in,out] metrics Input/output parameter.
     */
    void setMetrics(AuthMetrics* metrics);
    
    /**
     * @brief Update the access control configuration.
     * @param[in] config New access control configuration.
     */
    void updateConfig(const AuthRateLimitConfig& config);
    
    struct Statistics {
        size_t total_auth_attempts = 0;
        size_t allowed_attempts = 0;
        size_t rate_limited_attempts = 0;
        size_t lockout_blocked_attempts = 0;
        size_t successful_auths = 0;
        size_t failed_auths = 0;
        size_t currently_locked_accounts = 0;
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
    AuthRateLimitConfig config_;
    
    // IP-based rate limiting
    std::unique_ptr<server::RateLimiter> ip_rate_limiter_;
    
    // Per-user rate limiting
    std::unique_ptr<server::RateLimiter> user_rate_limiter_;
    
    // Account lockout management
    std::unique_ptr<AccountLockoutManager> lockout_manager_;

    // Optional pluggable backend for distributed counter storage.
    // When set, replaces ip_rate_limiter_ and user_rate_limiter_ for counting.
    // Protected by stats_mutex_.
    std::shared_ptr<IRateLimiterBackend> backend_;

    // ── Credential-stuffing detection state ─────────────────────────────
    struct CredentialStuffingEntry {
        std::unordered_set<std::string> usernames;  ///< distinct usernames tried
        // Timestamps of each attempt (for rolling-window pruning)
        std::vector<std::chrono::steady_clock::time_point> attempt_times;
        bool alerted = false;  ///< prevent duplicate alerts per penalty window
    };
    std::unordered_map<std::string, CredentialStuffingEntry> stuffing_state_;

    // Anomaly detection callback – protected by a separate mutex so it can be
    // called safely while stats_mutex_ is held.
    mutable std::shared_mutex callback_mutex_;
    AuthAnomalyCallback anomaly_callback_;
    utils::AuditLogger* audit_logger_ = nullptr;  ///< Non-owning; may be nullptr.
    AuthMetrics*        metrics_      = nullptr;  ///< Non-owning; may be nullptr.
    void fireAuthAnomaly(AuthAnomalyEvent::Type type,
                         const std::string& ip,
                         const std::string& user_id,
                         const std::string& detail,
                         CredentialStuffingOutcome cs_outcome
                             = CredentialStuffingOutcome::ALLOWED) const;

    /**
     * @brief Track credential-stuffing for a given (ip, user_id) pair.
     * @param[in] ip Input parameter.
     * @param[in] user_id Identifier of the user.
     * @param[in] cfg Input parameter.
     * @return True when the operation succeeds.
     * @details Returns true if the credential-stuffing alert threshold was just crossed. Must be called with stuffing_mutex_ held.
     */
    bool trackCredentialStuffing(const std::string& ip, const std::string& user_id,
                                  const AuthRateLimitConfig& cfg);

    /**
     * @brief ── Per-user persistent breach-count tracking ──────────────────────── Build the Redis/in-memory key for a user on the current UTC day.
     * @param[in] user_id Identifier of the user.
     * @return Return value.
     * @details Format: "cs:{user_id}:{YYYYMMDD}"
     */
    static std::string csBreachKey(const std::string& user_id);

    /**
     * @brief Atomically increment the daily breach counter for user_id and return the new count.
     * @param[in] user_id Identifier of the user.
     * @return Return value.
     * @details Uses Redis when available; otherwise falls back to the in-process map. Must NOT be called with stats_mutex_ held (may block on network I/O).
     */
    uint32_t incrementAndGetBreachCount(const std::string& user_id);

    /**
     * @brief Determine the escalation outcome from a raw breach count.
     * @param[in] count Input parameter.
     * @return Return value.
     */
    static CredentialStuffingOutcome outcomeFromBreachCount(uint32_t count);

    /**
     * @brief Called after the IP-level stuffing threshold fires.
     * @param[in] user_id Identifier of the user.
     * @param[in] ip Input parameter.
     * @return Return value.
     * @details Increments the per-user daily breach counter and fires the appropriate escalation response (CAPTCHA / OTP / 24h lock). Returns the outcome. Must NOT be called with stats_mutex_ held.
     */
    CredentialStuffingOutcome escalateCredentialStuffing(const std::string& user_id,
                                                         const std::string& ip);

    // In-memory fallback breach-count map (key = csBreachKey(user_id)).
    // Guarded by cs_breach_mutex_.
    std::unordered_map<std::string, uint32_t> cs_breach_count_;
    mutable std::mutex cs_breach_mutex_;

#ifdef THEMIS_ENABLE_REDIS
    // Redis connection for persistent stuffing counters.
    // Guarded by cs_redis_mutex_.
    struct redisContext* cs_redis_ctx_ = nullptr;
    mutable std::mutex   cs_redis_mutex_;
    /**
     * @brief Connect Cs Redis.
     * @return True when the operation succeeds.
     */
    bool connectCsRedis();
#endif

    // Statistics — individual counters are atomic so they can be updated without
    // holding stats_mutex_, which is reserved for config_ and backend_.
    // getStatistics() snapshots all atomics into a Statistics struct.
    mutable std::atomic<size_t> stat_total_auth_attempts_{0};
    mutable std::atomic<size_t> stat_allowed_attempts_{0};
    mutable std::atomic<size_t> stat_rate_limited_attempts_{0};
    mutable std::atomic<size_t> stat_lockout_blocked_attempts_{0};
    mutable std::atomic<size_t> stat_successful_auths_{0};
    mutable std::atomic<size_t> stat_failed_auths_{0};
    mutable std::atomic<size_t> stat_currently_locked_accounts_{0};

    // Protects config_ and backend_.
    // Lock hierarchy: stats_mutex_ must be acquired BEFORE stuffing_mutex_ when
    // both are needed simultaneously.  Hot paths acquire them sequentially (never
    // nested) to avoid holding stats_mutex_ during I/O.
    mutable std::shared_mutex stats_mutex_;

    // Protects stuffing_state_ only (separate from stats_mutex_ so rate-limiter
    // calls and backend I/O are never serialised against stuffing detection).
    mutable std::mutex stuffing_mutex_;
};

} // namespace auth
} // namespace themis
