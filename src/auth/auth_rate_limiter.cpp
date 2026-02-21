/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            auth_rate_limiter.cpp                              ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     372                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
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
    
    stats_.allowed_attempts++;
    return true;
}

void AuthRateLimiter::recordFailedAuth(
    const std::string& user_id,
    const std::string& ip_address,
    const std::string& reason)
{
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.failed_auths++;
    
    if (!user_id.empty()) {
        bool locked = lockout_manager_->recordFailedAttempt(user_id, ip_address, reason);
        if (locked) {
            stats_.currently_locked_accounts = lockout_manager_->getLockedAccountCount();
        }
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

void AuthRateLimiter::updateConfig(const AuthRateLimitConfig& config) {
    config_ = config;
    
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
    
    stats_ = Statistics{};
}

void AuthRateLimiter::cleanup() {
    ip_rate_limiter_->cleanup();
    user_rate_limiter_->cleanup();
    lockout_manager_->cleanup();
}

} // namespace auth
} // namespace themis
