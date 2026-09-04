/**
 * @file totp_replay_cache.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/totp_replay_cache.h"
#include "auth/mfa_authenticator.h"
#include "utils/logger.h"
#include <algorithm>

namespace themis {
namespace auth {

// ============================================================================
// TOTPReplayCache Implementation
// ============================================================================

TOTPReplayCache::TOTPReplayCache(const Config& config)
    : config_(config)
    , last_cleanup_(std::chrono::steady_clock::now())
{
    utils::Logger::info("TOTP Replay Cache initialized:");
    utils::Logger::info("  Retention period: {}s", config_.retention_period.count());
    utils::Logger::info("  Cleanup interval: {}s", config_.cleanup_interval.count());
    utils::Logger::info("  Max entries/user: {}", config_.max_entries_per_user);
}

bool TOTPReplayCache::checkAndMarkUsed(const std::string& user_id, const std::string& code) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Periodic cleanup
    if (needsCleanup()) {
        cleanup();
    }
    
    auto now = std::chrono::system_clock::now();
    
    // Check if code already used.
    // entries in user_cache are expected to share the same length, and we use
    // the standard std::string equality operator for comparison here.
    // This replay-cache lookup is a secondary defence: a valid TOTP code must
    // already have passed cryptographic HMAC validation before reaching this
    // path, so we do not rely on constant-time comparison at this stage and
    // any timing differences are not intended to be a security boundary.
    auto& user_cache = user_caches_[user_id];
    
    for (const auto& used_code : user_cache) {
        if (used_code.code == code) {
            // Check if still within retention period
            auto age = now - used_code.used_at;
            if (age < config_.retention_period) {
                // Replay attempt detected!
                stats_.replay_attempts_blocked++;
                utils::Logger::warn("TOTP replay attempt blocked for user: {}", user_id);
                return false;
            }
        }
    }
    
    // Code not used before - mark as used
    user_cache.push_back({code, now});
    
    // Enforce max entries per user (prevent memory exhaustion)
    if (static_cast<int>(user_cache.size()) > config_.max_entries_per_user) {
        // Remove oldest entries
        std::sort(user_cache.begin(), user_cache.end(),
                 [](const UsedCode& a, const UsedCode& b) {
                     return a.used_at < b.used_at;
                 });
        user_cache.erase(user_cache.begin(),
                        user_cache.begin() + (static_cast<int>(user_cache.size()) - config_.max_entries_per_user));
    }
    
    stats_.total_codes = 0;
    for (const auto& [uid, codes] : user_caches_) {
        stats_.total_codes += codes.size();
    }
    stats_.total_users = user_caches_.size();
    
    return true;  // Code allowed
}

bool TOTPReplayCache::isUsed(const std::string& user_id, const std::string& code) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = user_caches_.find(user_id);
    if (it == user_caches_.end()) {
        return false;
    }
    
    auto now = std::chrono::system_clock::now();
    
    for (const auto& used_code : it->second) {
        if (used_code.code == code) {
            // Check if still within retention period
            auto age = now - used_code.used_at;
            if (age < config_.retention_period) {
                return true;  // Code was used recently
            }
        }
    }
    
    return false;
}

void TOTPReplayCache::clearUser(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = user_caches_.find(user_id);
    if (it != user_caches_.end()) {
        stats_.total_codes -= it-> static_cast<int>(second.size());
        user_caches_.erase(it);
        stats_.total_users = user_caches_.size();
    }
}

void TOTPReplayCache::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    user_caches_.clear();
    stats_ = Statistics{};
    
    utils::Logger::info("TOTP replay cache cleared");
}

void TOTPReplayCache::cleanup() {
    // Note: Caller must hold mutex
    
    auto now = std::chrono::system_clock::now();
    size_t expired_count = 0;
    
    for (auto it = user_caches_.begin(); it != user_caches_.end();) {
        auto& user_cache = it->second;
        
        // Remove expired codes
        auto original_size = user_cache.size();
        user_cache.erase(
            std::remove_if(user_cache.begin(), user_cache.end(),
                [now, this](const UsedCode& used_code) {
                    return (now - used_code.used_at) >= config_.retention_period;
                }),
            user_cache.end()
        );
        
        expired_count += (original_size - user_cache.size());
        
        // Remove user if no codes left
        if (user_cache.empty()) {
            it = user_caches_.erase(it);
        } else {
            ++it;
        }
    }
    
    stats_.entries_expired += expired_count;
    stats_.total_users = user_caches_.size();
    stats_.total_codes = 0;
    for (const auto& [uid, codes] : user_caches_) {
        stats_.total_codes += codes.size();
    }
    
    last_cleanup_ = std::chrono::steady_clock::now();
    
    if (expired_count > 0) {
        utils::Logger::debug("TOTP replay cache cleanup: {} entries expired", expired_count);
    }
}

void TOTPReplayCache::cleanupUser(const std::string& user_id) {
    // Note: Caller must hold mutex
    
    auto it = user_caches_.find(user_id);
    if (it == user_caches_.end()) {
        return;
    }
    
    auto now = std::chrono::system_clock::now();
    auto& user_cache = it->second;
    
    auto original_size = user_cache.size();
    user_cache.erase(
        std::remove_if(user_cache.begin(), user_cache.end(),
            [now, this](const UsedCode& used_code) {
                return (now - used_code.used_at) >= config_.retention_period;
            }),
        user_cache.end()
    );
    
    stats_.entries_expired += (original_size - user_cache.size());
    
    if (user_cache.empty()) {
        user_caches_.erase(it);
    }
}

bool TOTPReplayCache::needsCleanup() const {
    // Note: Caller must hold mutex
    auto now = std::chrono::steady_clock::now();
    return (now - last_cleanup_) >= std::chrono::duration_cast<std::chrono::steady_clock::duration>(config_.cleanup_interval);
}

TOTPReplayCache::Statistics TOTPReplayCache::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

// ============================================================================
// SecureMFAValidator Implementation
// ============================================================================

SecureMFAValidator::SecureMFAValidator(const Config& config)
    : config_(config)
{
    if (config_.enable_replay_protection) {
        replay_cache_ = std::make_unique<TOTPReplayCache>(config_.replay_cache_config);
    }
}

bool SecureMFAValidator::validateTOTP(
    const std::string& user_id,
    const std::string& secret_base32,
    const std::string& code)
{
    // First, validate the code itself using standard MFA validator
    MFAAuthenticator::Config mfa_config;
    mfa_config.time_step_seconds = config_.time_step_seconds;
    mfa_config.code_length = config_.code_length;
    mfa_config.time_window = config_.time_window;
    mfa_config.issuer = config_.issuer;
    
    MFAAuthenticator mfa(mfa_config);
    
    bool code_valid = mfa.validateTOTP(secret_base32, code);
    
    if (!code_valid) {
        return false;  // Invalid code
    }
    
    // Code is valid - now check for replay
    if (config_.enable_replay_protection && replay_cache_) {
        if (!replay_cache_->checkAndMarkUsed(user_id, code)) {
            // Replay detected!
            utils::Logger::warn("TOTP replay attack detected for user: {}", user_id);
            throw std::runtime_error("TOTP code has already been used");
        }
    }
    
    return true;  // Code valid and not replayed
}

void SecureMFAValidator::clearUserCache(const std::string& user_id) {
    if (replay_cache_) {
        replay_cache_->clearUser(user_id);
    }
}

TOTPReplayCache::Statistics SecureMFAValidator::getReplayStatistics() const {
    if (replay_cache_) {
        return replay_cache_->getStatistics();
    }
    return TOTPReplayCache::Statistics{};
}

} // namespace auth
} // namespace themis
