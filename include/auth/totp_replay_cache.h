/**
 * @file totp_replay_cache.h
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
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <chrono>
#include <memory>

namespace themis {
namespace auth {

/**
 * @brief TOTP Replay Cache - prevents reuse of TOTP codes
 * 
 * Security Feature: Prevents replay attacks where an attacker captures
 * a valid TOTP code and attempts to reuse it within the time window.
 * 
 * Implementation:
 * - Tracks used codes per user within the TOTP time window
 * - Automatically expires entries after code validity period
 * - Thread-safe for concurrent authentication attempts
 * - In-memory storage (can be extended to Redis for distributed systems)
 * 
 * Time window: Typically 30-90 seconds (configurable)
 * Memory: Bounded by active users and cleanup interval
 * 
 * @note This is a P1 (High Priority) security hardening feature
 */
class TOTPReplayCache {
public:
    struct Config {
        // How long to keep used codes in cache (should match TOTP window)
        std::chrono::seconds retention_period{90};  // 3 time steps @ 30s
        
        // Cleanup interval for expired entries
        std::chrono::seconds cleanup_interval{300};  // 5 minutes
        
        // Maximum entries per user (prevents memory exhaustion)
        size_t max_entries_per_user = 10;
        static Config defaults() { return {}; }
    };
    
    explicit TOTPReplayCache(const Config& config = Config::defaults());
    ~TOTPReplayCache() = default;
    
    // Disable copy, allow move
    TOTPReplayCache(const TOTPReplayCache&) = delete;
    TOTPReplayCache& operator=(const TOTPReplayCache&) = delete;
    TOTPReplayCache(TOTPReplayCache&&) = default;
    TOTPReplayCache& operator=(TOTPReplayCache&&) = default;
    
    /**
     * @brief Check if code has been used and mark it as used
     * 
     * This is an atomic operation: checks and marks in one call.
     * 
     * @param user_id User identifier
     * @param code TOTP code to check
     * @return true if code was NOT previously used (authentication allowed)
     * @return false if code was already used (replay attack detected)
     */
    bool checkAndMarkUsed(const std::string& user_id, const std::string& code);
    
    /**
     * @brief Check if code has been used (without marking)
     * 
     * @param user_id User identifier
     * @param code TOTP code to check
     * @return true if code was already used
     */
    bool isUsed(const std::string& user_id, const std::string& code) const;
    
    /**
     * @brief Clear all used codes for a user
     * 
     * Use case: User successfully authenticated or account locked
     * 
     * @param user_id User identifier
     */
    void clearUser(const std::string& user_id);
    
    /**
     * @brief Clear all entries (for testing or maintenance)
     */
    void clear();
    
    /**
     * @brief Remove expired entries
     * 
     * Called automatically during normal operations, but can be
     * called manually for immediate cleanup.
     */
    void cleanup();
    
    /**
     * @brief Get cache statistics
     */
    struct Statistics {
        size_t total_users = 0;
        size_t total_codes = 0;
        size_t replay_attempts_blocked = 0;
        size_t entries_expired = 0;
    };
    
    Statistics getStatistics() const;

private:
    struct UsedCode {
        std::string code;
        std::chrono::system_clock::time_point used_at;
    };
    
    Config config_;
    
    // Per-user cache of used codes with timestamps
    std::unordered_map<std::string, std::vector<UsedCode>> user_caches_;
    
    // Statistics
    mutable Statistics stats_;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Last cleanup time
    std::chrono::steady_clock::time_point last_cleanup_;
    
    // Helper: Remove expired codes for a user
    void cleanupUser(const std::string& user_id);
    
    // Helper: Check if cleanup is needed
    bool needsCleanup() const;
};

/**
 * @brief Enhanced MFA validator with replay protection
 * 
 * Wraps MFAAuthenticator with replay cache for production use.
 * Provides the same interface with added security.
 */
class SecureMFAValidator {
public:
    struct Config {
        // Include MFA config
        int time_step_seconds = 30;
        int code_length = 6;
        int time_window = 1;
        std::string issuer = "ThemisDB";
        
        // Replay cache config
        bool enable_replay_protection = true;
        TOTPReplayCache::Config replay_cache_config;
        static Config defaults() { return {}; }
    };
    
    explicit SecureMFAValidator(const Config& config = Config::defaults());
    
    /**
     * @brief Validate TOTP code with replay protection
     * 
     * @param user_id User identifier
     * @param secret_base32 User's TOTP secret
     * @param code TOTP code to validate
     * @return true if code is valid and not replayed
     * @throws std::runtime_error on replay attempt (if enabled)
     */
    bool validateTOTP(
        const std::string& user_id,
        const std::string& secret_base32,
        const std::string& code
    );
    
    /**
     * @brief Clear replay cache for user (after successful login)
     * 
     * @param user_id User identifier
     */
    void clearUserCache(const std::string& user_id);
    
    /**
     * @brief Get replay cache statistics
     */
    TOTPReplayCache::Statistics getReplayStatistics() const;

private:
    Config config_;
    std::unique_ptr<TOTPReplayCache> replay_cache_;
};

} // namespace auth
} // namespace themis
