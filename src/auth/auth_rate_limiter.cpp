/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            auth_rate_limiter.cpp                              ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     391                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "auth/auth_rate_limiter.h"
#include "utils/logger.h"
#include <algorithm>

namespace themis {
namespace auth {

// ============================================================================
// AccountLockoutManager Implementation
// ============================================================================

AccountLockoutManager::AccountLockoutManager(const AuthRateLimitConfig& config)
    : config_(config)
    , last_cleanup_(std::chrono::steady_clock::now())
{
}

bool AccountLockoutManager::recordFailedAttempt(
    const std::string& user_id,
    const std::string& ip_address,
    const std::string& reason)
{
    if (!config_.enable_account_lockout) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::system_clock::now();
    auto& info = lockout_state_[user_id];
    
    // If account is already locked, just return true
    if (info.is_locked && now < info.locked_until) {
        utils::Logger::warn("Authentication attempt on locked account: " + user_id);
        return true;
    }
    
    // If lockout expired, reset the state
    if (info.is_locked && now >= info.locked_until) {
        info.is_locked = false;
        info.failed_attempts = 0;
        info.recent_failures.clear();
    }
    
    // Record the failed attempt
    FailedAttempt attempt{now, ip_address, user_id, reason};
    info.recent_failures.push_back(attempt);
    info.last_failure = now;
    
    if (info.failed_attempts == 0) {
        info.first_failure = now;
    }
    
    // Clean up old failures outside the window
    auto window_start = now - config_.lockout_window;
    info.recent_failures.erase(
        std::remove_if(info.recent_failures.begin(), info.recent_failures.end(),
            [window_start](const FailedAttempt& fa) {
                return fa.timestamp < window_start;
            }),
        info.recent_failures.end()
    );
    
    // Count failures within window
    info.failed_attempts = info.recent_failures.size();
    
    // Check if should lock
    if (shouldLockAccount(info)) {
        lockAccount(user_id, info);
        return true;
    }
    
    return false;
}

void AccountLockoutManager::recordSuccessfulAuth(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = lockout_state_.find(user_id);
    if (it != lockout_state_.end()) {
        // Reset failure count on successful auth
        it->second.failed_attempts = 0;
        it->second.recent_failures.clear();
        
        // Don't unlock if account is locked - requires explicit unlock
        if (!it->second.is_locked) {
            lockout_state_.erase(it);
        }
    }
}

bool AccountLockoutManager::isAccountLocked(const std::string& user_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = lockout_state_.find(user_id);
    if (it == lockout_state_.end()) {
        return false;
    }
    
    auto now = std::chrono::system_clock::now();
    if (it->second.is_locked && now < it->second.locked_until) {
        return true;
    }
    
    return false;
}

std::optional<LockoutInfo> AccountLockoutManager::getLockoutInfo(
    const std::string& user_id) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = lockout_state_.find(user_id);
    if (it == lockout_state_.end()) {
        return std::nullopt;
    }
    
    return it->second;
}

bool AccountLockoutManager::unlockAccount(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = lockout_state_.find(user_id);
    if (it == lockout_state_.end() || !it->second.is_locked) {
        return false;
    }
    
    utils::Logger::info("Manually unlocking account: " + user_id);
    it->second.is_locked = false;
    it->second.failed_attempts = 0;
    it->second.recent_failures.clear();
    lockout_state_.erase(it);
    
    return true;
}

size_t AccountLockoutManager::getLockedAccountCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::system_clock::now();
    size_t count = 0;
    
    for (const auto& [user_id, info] : lockout_state_) {
        if (info.is_locked && now < info.locked_until) {
            count++;
        }
    }
    
    return count;
}

void AccountLockoutManager::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = std::chrono::system_clock::now();
    
    // Remove expired lockouts and old records
    for (auto it = lockout_state_.begin(); it != lockout_state_.end();) {
        auto& info = it->second;
        
        // Remove expired lockouts
        if (info.is_locked && now >= info.locked_until) {
            it = lockout_state_.erase(it);
            continue;
        }
        
        // Remove old records with no recent activity
        auto window_start = now - config_.lockout_window - config_.lockout_duration;
        if (!info.is_locked && info.last_failure < std::chrono::system_clock::from_time_t(
                std::chrono::system_clock::to_time_t(window_start))) {
            it = lockout_state_.erase(it);
            continue;
        }
        
        ++it;
    }
    
    last_cleanup_ = std::chrono::steady_clock::now();
}

void AccountLockoutManager::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    lockout_state_.clear();
}

void AccountLockoutManager::lockAccount(
    const std::string& user_id,
    const LockoutInfo& info)
{
    auto now = std::chrono::system_clock::now();
    
    lockout_state_[user_id].is_locked = true;
    lockout_state_[user_id].locked_until = now + config_.lockout_duration;
    
    utils::Logger::warn("Account locked due to failed authentication attempts: " + 
                       user_id + " (failed attempts: " + 
                       std::to_string(info.failed_attempts) + ")");
}

bool AccountLockoutManager::shouldLockAccount(const LockoutInfo& info) const {
    return info.failed_attempts >= config_.lockout_failed_attempts;
}

// ============================================================================
// AuthRateLimiter Implementation
// ============================================================================

AuthRateLimiter::AuthRateLimiter(const AuthRateLimitConfig& config)
    : config_(config)
{
    // Create IP rate limiter
    server::RateLimitConfig ip_config;
    ip_config.bucket_capacity = config.max_attempts_per_ip_per_minute;
    ip_config.refill_rate = static_cast<double>(config.max_attempts_per_ip_per_minute) / 60.0;
    ip_config.window_seconds = 60;
    ip_config.per_ip_enabled = true;
    ip_config.per_user_enabled = false;
    ip_config.whitelist_ips = config.whitelist_ips;
    ip_rate_limiter_ = std::make_unique<server::RateLimiter>(ip_config);
    
    // Create user rate limiter
    server::RateLimitConfig user_config;
    user_config.bucket_capacity = config.max_attempts_per_user_per_minute;
    user_config.refill_rate = static_cast<double>(config.max_attempts_per_user_per_minute) / 60.0;
    user_config.window_seconds = 60;
    user_config.per_ip_enabled = false;
    user_config.per_user_enabled = true;
    user_rate_limiter_ = std::make_unique<server::RateLimiter>(user_config);
    
    // Create lockout manager
    lockout_manager_ = std::make_unique<AccountLockoutManager>(config);
}

bool AuthRateLimiter::allowAuthAttempt(
    const std::string& ip_address,
    const std::string& user_id)
{
    bool stuffing_alert = false;

    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_auth_attempts++;

        // Check if IP is whitelisted
        if (isWhitelisted(ip_address)) {
            stats_.allowed_attempts++;
            return true;
        }

        // Check account lockout first
        if (!user_id.empty() && config_.enable_account_lockout) {
            if (lockout_manager_->isAccountLocked(user_id)) {
                stats_.lockout_blocked_attempts++;
                utils::Logger::warn("Authentication blocked - account locked: " + user_id);
                return false;
            }
        }

        // Check IP rate limit
        if (config_.enable_ip_rate_limiting) {
            if (!ip_rate_limiter_->allowRequest(ip_address)) {
                stats_.rate_limited_attempts++;
                utils::Logger::warn("Authentication rate limited by IP: " + ip_address);
                return false;
            }
        }

        // Check user rate limit
        if (!user_id.empty() && config_.enable_user_rate_limiting) {
            if (!user_rate_limiter_->allowRequest("", user_id)) {
                stats_.rate_limited_attempts++;
                utils::Logger::warn("Authentication rate limited for user: " + user_id);
                return false;
            }
        }

        // Track this attempt for credential-stuffing detection (may set stuffing_alert)
        if (!user_id.empty()) {
            stuffing_alert = trackCredentialStuffing(ip_address, user_id);
        }

        stats_.allowed_attempts++;
    }

    // Fire anomaly events outside the lock so callbacks can safely call back into us.
    if (stuffing_alert) {
        const std::string detail =
            "credential stuffing suspected: threshold reached from ip=" + ip_address;
        fireAuthAnomaly(AuthAnomalyEvent::Type::CREDENTIAL_STUFFING_SUSPECTED,
                        ip_address, "", detail);
    }

    return true;
}

void AuthRateLimiter::recordFailedAuth(
    const std::string& user_id,
    const std::string& ip_address,
    const std::string& reason)
{
    bool lockout_triggered = false;
    bool stuffing_alert    = false;

    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.failed_auths++;

        // Track for credential-stuffing detection
        if (!user_id.empty()) {
            stuffing_alert = trackCredentialStuffing(ip_address, user_id);
        }

        if (!user_id.empty()) {
            lockout_triggered =
                lockout_manager_->recordFailedAttempt(user_id, ip_address, reason);
            if (lockout_triggered) {
                stats_.currently_locked_accounts =
                    lockout_manager_->getLockedAccountCount();
            }
        }
    }

    // Fire anomaly events outside the lock.
    if (stuffing_alert) {
        const std::string detail =
            "credential stuffing suspected: threshold reached from ip=" + ip_address;
        utils::Logger::warn("Auth anomaly [CREDENTIAL_STUFFING_SUSPECTED] ip=" +
                            ip_address + " " + detail);
        fireAuthAnomaly(AuthAnomalyEvent::Type::CREDENTIAL_STUFFING_SUSPECTED,
                        ip_address, "", detail);
    }

    if (lockout_triggered && !user_id.empty()) {
        const std::string lockout_detail =
            "account locked after repeated failures (reason: " + reason + ")";
        utils::Logger::warn("Auth anomaly [ACCOUNT_LOCKOUT_TRIGGERED] user=" +
                            user_id + " ip=" + ip_address + " " + lockout_detail);
        fireAuthAnomaly(AuthAnomalyEvent::Type::ACCOUNT_LOCKOUT_TRIGGERED,
                        ip_address, user_id, lockout_detail);

        const std::string bf_detail =
            "brute-force detected: lockout triggered from ip=" + ip_address +
            " (reason: " + reason + ")";
        utils::Logger::warn("Auth anomaly [BRUTE_FORCE_DETECTED] user=" +
                            user_id + " ip=" + ip_address + " " + bf_detail);
        fireAuthAnomaly(AuthAnomalyEvent::Type::BRUTE_FORCE_DETECTED,
                        ip_address, user_id, bf_detail);
    }
}

void AuthRateLimiter::recordSuccessfulAuth(
    const std::string& user_id,
    const std::string& ip_address)
{
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.successful_auths++;
    
    if (!user_id.empty()) {
        lockout_manager_->recordSuccessfulAuth(user_id);
    }
}

bool AuthRateLimiter::isAccountLocked(const std::string& user_id) const {
    return lockout_manager_->isAccountLocked(user_id);
}

std::optional<LockoutInfo> AuthRateLimiter::getLockoutInfo(
    const std::string& user_id) const
{
    return lockout_manager_->getLockoutInfo(user_id);
}

bool AuthRateLimiter::unlockAccount(const std::string& user_id) {
    bool unlocked = lockout_manager_->unlockAccount(user_id);
    if (unlocked) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.currently_locked_accounts = lockout_manager_->getLockedAccountCount();
    }
    return unlocked;
}

uint32_t AuthRateLimiter::getRetryAfter(const std::string& ip_address) const {
    return ip_rate_limiter_->getRetryAfter(ip_address, "");
}

bool AuthRateLimiter::isWhitelisted(const std::string& ip_address) const {
    return ip_rate_limiter_->isWhitelisted(ip_address);
}

void AuthRateLimiter::setAnomalyCallback(AuthAnomalyCallback callback) {
    std::lock_guard<std::mutex> lock(callback_mutex_);
    anomaly_callback_ = std::move(callback);
}

void AuthRateLimiter::fireAuthAnomaly(AuthAnomalyEvent::Type type,
                                       const std::string& ip,
                                       const std::string& user_id,
                                       const std::string& detail) const {
    AuthAnomalyCallback cb;
    {
        std::lock_guard<std::mutex> lock(callback_mutex_);
        cb = anomaly_callback_;
    }
    if (cb) {
        AuthAnomalyEvent ev{type, ip, user_id, detail, std::chrono::system_clock::now()};
        cb(ev);
    }
}

bool AuthRateLimiter::trackCredentialStuffing(const std::string& ip,
                                              const std::string& user_id) {
    // Called with stats_mutex_ held.
    if (!config_.enable_credential_stuffing_detection || user_id.empty()) return false;

    auto now = std::chrono::steady_clock::now();
    auto  window    = std::chrono::seconds(config_.credential_stuffing_window_seconds);
    auto& entry     = stuffing_state_[ip];
    auto  cutoff    = now - window;

    // attempt_times is maintained in chronological (insertion) order, so all
    // expired entries form a contiguous prefix. Erase it in O(k) rather than
    // the O(n) erase-remove approach.
    auto first_valid = std::lower_bound(entry.attempt_times.begin(),
                                        entry.attempt_times.end(),
                                        cutoff);
    entry.attempt_times.erase(entry.attempt_times.begin(), first_valid);

    // If the window has fully expired, reset alert state so a new attack wave
    // can trigger another alert.
    if (entry.attempt_times.empty()) {
        entry.alerted = false;
        entry.usernames.clear();
    }

    entry.attempt_times.push_back(now);
    entry.usernames.insert(user_id);

    if (!entry.alerted &&
        entry.usernames.size() >= config_.credential_stuffing_user_threshold) {
        entry.alerted = true;
        // Caller fires the anomaly event outside the lock.
        return true;
    }
    return false;
}

void AuthRateLimiter::updateConfig(const AuthRateLimitConfig& config) {
    // Update rate limiters
    server::RateLimitConfig ip_config;
    ip_config.bucket_capacity = config.max_attempts_per_ip_per_minute;
    ip_config.refill_rate = static_cast<double>(config.max_attempts_per_ip_per_minute) / 60.0;
    ip_config.whitelist_ips = config.whitelist_ips;
    ip_rate_limiter_->updateConfig(ip_config);
    
    server::RateLimitConfig user_config;
    user_config.bucket_capacity = config.max_attempts_per_user_per_minute;
    user_config.refill_rate = static_cast<double>(config.max_attempts_per_user_per_minute) / 60.0;
    user_rate_limiter_->updateConfig(user_config);

    std::lock_guard<std::mutex> lock(stats_mutex_);
    config_ = config;
    // Clearing the credential-stuffing state on a config update is intentional: it
    // ensures the new threshold and window take effect immediately for all IPs.
    // A side-effect is that any IP currently being tracked will restart from zero.
    // Callers that need continuity should trigger cleanup() before updateConfig().
    stuffing_state_.clear();
}

AuthRateLimiter::Statistics AuthRateLimiter::getStatistics() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void AuthRateLimiter::reset() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    ip_rate_limiter_->reset();
    user_rate_limiter_->reset();
    lockout_manager_->reset();
    stuffing_state_.clear();
    
    stats_ = Statistics{};
}

void AuthRateLimiter::cleanup() {
    ip_rate_limiter_->cleanup();
    user_rate_limiter_->cleanup();
    lockout_manager_->cleanup();

    // Prune stale credential-stuffing state for IPs whose rolling window has expired.
    std::lock_guard<std::mutex> lock(stats_mutex_);
    auto now    = std::chrono::steady_clock::now();
    auto window = std::chrono::seconds(config_.credential_stuffing_window_seconds);
    auto cutoff = now - window;
    for (auto it = stuffing_state_.begin(); it != stuffing_state_.end(); ) {
        auto& entry = it->second;
        // attempt_times is chronological – erase the expired prefix efficiently.
        auto first_valid = std::lower_bound(entry.attempt_times.begin(),
                                            entry.attempt_times.end(),
                                            cutoff);
        entry.attempt_times.erase(entry.attempt_times.begin(), first_valid);
        // If no recent attempts remain, remove the entire entry
        if (entry.attempt_times.empty()) {
            it = stuffing_state_.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace auth
} // namespace themis
