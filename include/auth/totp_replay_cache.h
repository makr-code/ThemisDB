/**
 * @file totp_replay_cache.h
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
#include <mutex>
#include <chrono>
#include <memory>

namespace themis {
namespace auth {

class TOTPReplayCache {
public:
    struct Config {
        // How long to keep used codes in cache (should match TOTP window)
        std::chrono::seconds retention_period{90};  // 3 time steps @ 30s
        
        // Cleanup interval for expired entries
        std::chrono::seconds cleanup_interval{300};  // 5 minutes
        
        // Maximum entries per user (prevents memory exhaustion)
        size_t max_entries_per_user = 10;
        /**
         * @brief Defaults.
         * @return Return value.
         * @details Implements defaults without additional internal calls.
         */
        static Config defaults() { return {}; }
    };
    
    explicit TOTPReplayCache(const Config& config = Config::defaults());
    ~TOTPReplayCache() = default;
    
    // Disable copy, allow move
    TOTPReplayCache(const TOTPReplayCache&) = delete;
    TOTPReplayCache& operator=(const TOTPReplayCache&) = delete;
    TOTPReplayCache(TOTPReplayCache&&) noexcept = default;
    TOTPReplayCache& operator=(TOTPReplayCache&&) noexcept = default;
    
    /**
     * @brief Check And Mark Used.
     * @param[in] user_id Identifier of the user.
     * @param[in] code Input parameter.
     * @return True when the operation succeeds.
     */
    bool checkAndMarkUsed(const std::string& user_id, const std::string& code);
    
    /**
     * @brief Is Used.
     * @param[in] user_id Identifier of the user.
     * @param[in] code Input parameter.
     * @return True when the operation succeeds.
     */
    bool isUsed(const std::string& user_id, const std::string& code) const;
    
    /**
     * @brief Clear User.
     * @param[in] user_id Identifier of the user.
     */
    void clearUser(const std::string& user_id);
    
    /**
     * @brief Clear.
     */
    void clear();
    
    /**
     * @brief Cleanup.
     */
    void cleanup();
    
    struct Statistics {
        size_t total_users = 0;
        size_t total_codes = 0;
        size_t replay_attempts_blocked = 0;
        size_t entries_expired = 0;
    };
    
    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    Statistics getStatistics() const;

private:
    struct UsedCode {
        std::string code = {};
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
    
    /**
     * @brief Helper: Remove expired codes for a user
     * @param[in] user_id Identifier of the user.
     */
    void cleanupUser(const std::string& user_id);
    
    /**
     * @brief Helper: Check if cleanup is needed
     * @return True when the operation succeeds.
     */
    bool needsCleanup() const;
};

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
        /**
         * @brief Defaults.
         * @return Return value.
         * @details Implements defaults without additional internal calls.
         */
        static Config defaults() { return {}; }
    };
    
    explicit SecureMFAValidator(const Config& config = Config::defaults());
    
    /**
     * @brief Validate TOTP.
     * @param[in] user_id Identifier of the user.
     * @param[in] secret_base32 Input parameter.
     * @param[in] code Input parameter.
     * @return True when the operation succeeds.
     */
    bool validateTOTP(
        const std::string& user_id,
        const std::string& secret_base32,
        const std::string& code
    );
    
    /**
     * @brief Clear User Cache.
     * @param[in] user_id Identifier of the user.
     */
    void clearUserCache(const std::string& user_id);
    
    /**
     * @brief Get Replay Statistics.
     * @return Return value.
     */
    TOTPReplayCache::Statistics getReplayStatistics() const;

private:
    Config config_;
    std::unique_ptr<TOTPReplayCache> replay_cache_;
};

} // namespace auth
} // namespace themis
