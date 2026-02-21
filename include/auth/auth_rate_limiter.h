/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            auth_rate_limiter.h                                ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:19:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     306                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "server/rate_limiter.h"
#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <memory>
#include <optional>

namespace themis {
namespace auth {

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
};

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
    
    mutable std::mutex mutex_;
    
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
    
    // Statistics
    mutable Statistics stats_;
    mutable std::mutex stats_mutex_;
};

} // namespace auth
} // namespace themis
