/**
 * @file auth_rate_limiter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Outcome of a per-user credential-stuffing escalation check.
 *
 * When the persistent breach counter for a user crosses successive thresholds
 * the required mitigation action escalates:
 *   breach 1 → CAPTCHA_REQUIRED
 *   breach 2 → OTP_REQUIRED
 *   breach 3+ → ACCOUNT_LOCKED_24H
 */
enum class CredentialStuffingOutcome {
    ALLOWED,           ///< No special action required
    CAPTCHA_REQUIRED,  ///< First breach: challenge with CAPTCHA
    OTP_REQUIRED,      ///< Second breach: require email OTP
    ACCOUNT_LOCKED_24H ///< Third+ breach: lock account for 24 hours
};

/**
 * @brief Configuration for authentication rate limiting
 */
struct AuthRateLimitConfig {
    // Per-IP rate limit for auth attempts
    size_t max_attempts_per_ip_per_minute = 10;
    
    // Per-user rate limit for auth attempts
    size_t max_attempts_per_user_per_minute = 5;
    
    // Account lockout configuration
    size_t lockout_failed_attempts = 5;        // Lock after N failed attempts
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

/**
 * @brief Anomaly event emitted by AuthRateLimiter when a suspicious authentication
 *        pattern is detected (brute-force, credential stuffing, account lockout).
 *
 * Register a handler via AuthRateLimiter::setAnomalyCallback() to forward events
 * to an audit log, SIEM, or alerting system.
 */
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

    /// Escalation outcome when type == CREDENTIAL_STUFFING_SUSPECTED.
    /// ALLOWED means no special action beyond the detection alert itself.
    CredentialStuffingOutcome cs_outcome = CredentialStuffingOutcome::ALLOWED;
};

/// Callback invoked (outside internal mutex) on each detected auth anomaly.
using AuthAnomalyCallback = std::function<void(const AuthAnomalyEvent&)>;

/**
 * @brief Failed authentication attempt record
 */
struct FailedAttempt {
    std::chrono::system_clock::time_point timestamp;
    std::string ip_address;
    std::string user_id;
    std::string reason;  // e.g., "invalid_password", "invalid_token"
};

/**
 * @brief Account lockout information
 */
struct LockoutInfo {
    bool is_locked = false;
    std::chrono::system_clock::time_point locked_until;
    size_t failed_attempts = 0;
    std::chrono::system_clock::time_point first_failure;
    std::chrono::system_clock::time_point last_failure;
    std::vector<FailedAttempt> recent_failures;
};

/**
 * @brief Account Lockout Manager
 * 
 * Tracks failed authentication attempts and locks accounts after
 * too many failures to prevent brute-force attacks.
 * 
 * Features:
 * - Tracks failed attempts per user
 * - Automatic lockout after N failures within time window
 * - Configurable lockout duration
 * - Admin unlock capability
 * - Thread-safe
 * - Automatic cleanup of old records
 */
class AccountLockoutManager {
public:
    explicit AccountLockoutManager(const AuthRateLimitConfig& config);
    
    /**
     * @brief Record a failed authentication attempt
     * @param user_id User identifier
     * @param ip_address Client IP address
     * @param reason Failure reason
     * @return true if account should be locked
     */
    bool recordFailedAttempt(
        const std::string& user_id,
        const std::string& ip_address,
        const std::string& reason
    );
    
    /**
     * @brief Record a successful authentication (resets failure count)
     * @param user_id User identifier
     */
    void recordSuccessfulAuth(const std::string& user_id);
    
    /**
     * @brief Check if account is currently locked
     * @param user_id User identifier
     * @return true if account is locked
     */
    bool isAccountLocked(const std::string& user_id) const;
    
    /**
     * @brief Get lockout information for user
     * @param user_id User identifier
     * @return Lockout info if exists
     */
    std::optional<LockoutInfo> getLockoutInfo(const std::string& user_id) const;
    
    /**
     * @brief Manually unlock an account (admin operation)
     * @param user_id User identifier
     * @return true if account was locked and is now unlocked
     */
    bool unlockAccount(const std::string& user_id);
    
    /**
     * @brief Get number of currently locked accounts
     */
    size_t getLockedAccountCount() const;

    /**
     * @brief Forcefully lock an account for a specified duration.
     *
     * Used by the credential-stuffing escalation policy to apply a 24-hour
     * lock regardless of the normal failed-attempt threshold.
     *
     * @param user_id  User to lock
     * @param duration Lock duration (e.g. std::chrono::hours(24))
     */
    void forceLockAccount(const std::string& user_id, std::chrono::seconds duration);
    
    /**
     * @brief Cleanup expired lockouts and old records
     */
    void cleanup();
    
    /**
     * @brief Reset all lockouts (for testing)
     */
    void reset();

private:
    void lockAccount(const std::string& user_id, const LockoutInfo& info);
    bool shouldLockAccount(const LockoutInfo& info) const;
    
    AuthRateLimitConfig config_;
    
    // Per-user lockout state
    std::unordered_map<std::string, LockoutInfo> lockout_state_;
    
    mutable std::shared_mutex mutex_;
    
    // Cleanup interval (5 minutes)
    static constexpr uint32_t CLEANUP_INTERVAL_SECONDS = 300;
    std::chrono::steady_clock::time_point last_cleanup_;
};

/**
 * @brief Authentication-specific rate limiter
 * 
 * Combines IP-based rate limiting and account lockout protection
 * specifically for authentication endpoints.
 * 
 * Features:
 * - Per-IP rate limiting (prevent distributed brute-force)
 * - Per-user rate limiting (prevent targeted attacks)
 * - Account lockout after failed attempts
 * - Whitelisting for trusted IPs
 * - Metrics and monitoring
 */
class AuthRateLimiter {
public:
    explicit AuthRateLimiter(const AuthRateLimitConfig& config = AuthRateLimitConfig());
    
    /**
     * @brief Check if authentication attempt is allowed
     * @param ip_address Client IP address
     * @param user_id User identifier (empty for pre-auth checks)
     * @return true if attempt allowed, false if rate limited or locked
     */
    bool allowAuthAttempt(
        const std::string& ip_address,
        const std::string& user_id = ""
    );
    
    /**
     * @brief Record a failed authentication attempt
     * @param user_id User identifier
     * @param ip_address Client IP address
     * @param reason Failure reason
     */
    void recordFailedAuth(
        const std::string& user_id,
        const std::string& ip_address,
        const std::string& reason
    );
    
    /**
     * @brief Record a successful authentication
     * @param user_id User identifier
     * @param ip_address Client IP address
     */
    void recordSuccessfulAuth(
        const std::string& user_id,
        const std::string& ip_address
    );
    
    /**
     * @brief Check if account is locked
     * @param user_id User identifier
     * @return true if account is locked
     */
    bool isAccountLocked(const std::string& user_id) const;
    
    /**
     * @brief Get lockout information for user
     * @param user_id User identifier
     * @return Lockout info if exists
     */
    std::optional<LockoutInfo> getLockoutInfo(const std::string& user_id) const;
    
    /**
     * @brief Manually unlock an account (admin operation)
     * @param user_id User identifier
     * @return true if account was unlocked
     */
    bool unlockAccount(const std::string& user_id);
    
    /**
     * @brief Get retry-after time in seconds for rate-limited client
     * @param ip_address Client IP address
     * @return Seconds until next attempt allowed (0 if not rate limited)
     */
    uint32_t getRetryAfter(const std::string& ip_address) const;
    
    /**
     * @brief Check if IP is whitelisted
     */
    bool isWhitelisted(const std::string& ip_address) const;

    /**
     * @brief Register a callback invoked whenever an authentication anomaly is detected.
     *
     * The callback is invoked outside the internal mutex, so performing I/O (e.g.
     * writing to an audit log or forwarding to a SIEM) is safe without risking a
     * deadlock.  Pass nullptr or an empty function to deregister.
     *
     * Detected anomaly types:
     *   - ACCOUNT_LOCKOUT_TRIGGERED  – account locked after repeated failures
     *   - BRUTE_FORCE_DETECTED       – same IP responsible for locking an account
     *   - CREDENTIAL_STUFFING_SUSPECTED – one IP tried many distinct usernames
     *     (cs_outcome field carries the escalation level for the targeted user)
     */
    void setAnomalyCallback(AuthAnomalyCallback callback);

    /**
     * @brief Attach an audit logger for structured anomaly event logging.
     *
     * When set, every anomaly event (brute-force, credential stuffing, account
     * lockout) is forwarded to the audit logger in addition to the anomaly
     * callback.  Pass nullptr to detach.  Does not take ownership.
     */
    void setAuditLogger(utils::AuditLogger* logger);

    /**
     * @brief Inject a shared rate-limiter backend for distributed counter storage.
     *
     * When a backend is set, AuthRateLimiter uses it instead of the internal
     * in-process token-bucket for IP and user rate limiting.  Two or more
     * AuthRateLimiter instances that share the same backend instance will
     * observe each other's request counts, enabling consistent rate limiting
     * across nodes (or across multiple in-process instances for testing).
     *
     * - Pass an InMemoryRateLimiterBackend instance for single-node deployments
     *   (or shared between multiple in-process instances for testing).
     * - Pass a RedisRateLimiterBackend instance for multi-node deployments.
     * - Pass nullptr to revert to the default in-process token-bucket behaviour.
     *
     * Typically called once during initialisation before any concurrent access.
     */
    void setBackend(std::shared_ptr<IRateLimiterBackend> backend);

    /**
     * @brief Attach a metrics collector for credential-stuffing instrumentation.
     *
     * When set, every credential-stuffing detection event calls
     * AuthMetrics::recordCredentialStuffingAttempt().  Pass nullptr to detach.
     * Does not take ownership.
     */
    void setMetrics(AuthMetrics* metrics);
    
    /**
     * @brief Update configuration at runtime
     */
    void updateConfig(const AuthRateLimitConfig& config);
    
    /**
     * @brief Get current statistics
     */
    struct Statistics {
        size_t total_auth_attempts = 0;
        size_t allowed_attempts = 0;
        size_t rate_limited_attempts = 0;
        size_t lockout_blocked_attempts = 0;
        size_t successful_auths = 0;
        size_t failed_auths = 0;
        size_t currently_locked_accounts = 0;
    };
    
    Statistics getStatistics() const;
    
    /**
     * @brief Reset all state (for testing)
     */
    void reset();
    
    /**
     * @brief Cleanup old records
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

    // Track credential-stuffing for a given (ip, user_id) pair.
    // Returns true if the credential-stuffing alert threshold was just crossed.
    // Must be called with stuffing_mutex_ held.
    bool trackCredentialStuffing(const std::string& ip, const std::string& user_id,
                                  const AuthRateLimitConfig& cfg);

    // ── Per-user persistent breach-count tracking ────────────────────────
    // Build the Redis/in-memory key for a user on the current UTC day.
    // Format: "cs:{user_id}:{YYYYMMDD}"
    static std::string csBreachKey(const std::string& user_id);

    // Atomically increment the daily breach counter for user_id and return
    // the new count.  Uses Redis when available; otherwise falls back to the
    // in-process map.  Must NOT be called with stats_mutex_ held (may block
    // on network I/O).
    uint32_t incrementAndGetBreachCount(const std::string& user_id);

    // Determine the escalation outcome from a raw breach count.
    static CredentialStuffingOutcome outcomeFromBreachCount(uint32_t count);

    // Called after the IP-level stuffing threshold fires.  Increments the
    // per-user daily breach counter and fires the appropriate escalation
    // response (CAPTCHA / OTP / 24h lock).  Returns the outcome.
    // Must NOT be called with stats_mutex_ held.
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
