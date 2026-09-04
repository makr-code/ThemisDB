/**
 * @file rate_limiter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/rate_limiter.h"
#include "utils/logger.h"
#include <algorithm>

namespace themis {
namespace server {

// ============================================================================
// TokenBucket Implementation
// ============================================================================

TokenBucket::TokenBucket(size_t capacity, double refill_rate)
    : capacity_(capacity)
    , tokens_(static_cast<double>(capacity))
    , refill_rate_(refill_rate)
    , last_refill_(std::chrono::steady_clock::now())
{}

void TokenBucket::refill() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_refill_).count();
    
    if (elapsed > 0) {
        double tokens_to_add = (elapsed / 1000.0) * refill_rate_;
        tokens_ = std::min(static_cast<double>(capacity_), tokens_ + tokens_to_add);
        last_refill_ = now;
    }
}

bool TokenBucket::tryConsume([[maybe_unused]] size_t tokens) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    refill();
    
    if (tokens_ >= static_cast<double>(tokens)) {
        tokens_ -= static_cast<double>(tokens);
        return true;
    }
    return false;
}

double TokenBucket::getTokens() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return tokens_;
}

uint64_t TokenBucket::getRetryAfterMs() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    if (tokens_ >= 1.0) {
        return 0;
    }
    
    // Calculate time until 1 token is available
    double tokens_needed = 1.0 - tokens_;
    double seconds = tokens_needed / refill_rate_;
    return static_cast<uint64_t>(seconds * 1000.0);
}

void TokenBucket::reset() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    tokens_ = static_cast<double>(capacity_);
    last_refill_ = std::chrono::steady_clock::now();
}

// ============================================================================
// RateLimiter Implementation
// ============================================================================

RateLimiter::RateLimiter(const RateLimitConfig& config)
    : config_(config)
    , last_cleanup_(std::chrono::steady_clock::now())
{
    THEMIS_INFO("Rate Limiter initialized: {} req/min, bucket capacity: {}", 
        config_.refill_rate * 60.0, config_.bucket_capacity);
}

bool RateLimiter::isWhitelisted(const std::string& ip) const {
    return std::find(config_.whitelist_ips.begin(), 
                     config_.whitelist_ips.end(), ip) != config_.whitelist_ips.end();
}

void RateLimiter::setAnomalyCallback([[maybe_unused]] AnomalyCallback callback) {
    std::unique_lock<std::shared_mutex> lock([[maybe_unused]] callback_mutex_);
    anomaly_callback_ = std::move([[maybe_unused]] callback);
}

void RateLimiter::fireAnomaly(AnomalyEvent::Type type,
                               const std::string& ip,
                               const std::string& detail) const {
    // Use a separate mutex so this is safe to call while mutex_ is held.
    AnomalyCallback cb;
    {
        std::shared_lock<std::shared_mutex> lock([[maybe_unused]] callback_mutex_);
        cb = anomaly_callback_;
    }
    if (cb) {
        AnomalyEvent ev{type, ip, detail, std::chrono::system_clock::now()};
        cb(ev);
    }
}

void RateLimiter::blacklistIP(const std::string& ip) {
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        blacklisted_ips_.insert(ip);
    }
    THEMIS_WARN("IP added to blacklist: {}", ip);
    fireAnomaly(AnomalyEvent::Type::IP_BLACKLISTED, ip, "IP manually blacklisted");
}

void RateLimiter::unblacklistIP(const std::string& ip) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    blacklisted_ips_.erase(ip);
    THEMIS_INFO("IP removed from blacklist: {}", ip);
}

bool RateLimiter::isBlacklisted(const std::string& ip) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return blacklisted_ips_.count(ip) > 0;
}

bool RateLimiter::isAdaptivelyThrottled(const std::string& ip) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = adaptive_state_.find(ip);
    if (it == adaptive_state_.end()) return false;
    if (!it->second.under_penalty) return false;
    return std::chrono::steady_clock::now() < it->second.penalty_until;
}

void RateLimiter::recordRejectionForAdaptive(const std::string& ip) {
    // Called with mutex_ held
    if (!config_.adaptive_throttling_enabled) return;
    auto now = std::chrono::steady_clock::now();
    auto& entry = adaptive_state_[ip];

    // Prune old rejection timestamps outside the rolling window
    auto window = std::chrono::seconds(config_.adaptive_window_seconds);
    entry.rejection_times.erase(
        std::remove_if(entry.rejection_times.begin(), entry.rejection_times.end(),
                       [&]([[maybe_unused]] const auto& t){ return (now - t) > window; }),
        entry.rejection_times.end());

    entry.rejection_times.push_back(now);

    if (!entry.under_penalty &&
        entry.rejection_times.size() >= config_.adaptive_rejection_threshold) {
        entry.under_penalty = true;
        entry.penalty_until = now + std::chrono::seconds(
            config_.adaptive_penalty_duration_seconds);
        THEMIS_WARN("Adaptive throttle penalty applied to IP: {} ({} rejections in {}s)",
                    ip, entry.rejection_times.size(),
                    config_.adaptive_window_seconds);
        // Fire anomaly callback while mutex_ is held; callback_mutex_ is separate.
        fireAnomaly(AnomalyEvent::Type::ADAPTIVE_THROTTLE_TRIGGERED, ip,
                    "adaptive penalty: " + std::to_string(entry.rejection_times.size()) +
                    " rejections in " + std::to_string(config_.adaptive_window_seconds) + "s");
    }
}

std::shared_ptr<TokenBucket> RateLimiter::getOrCreateBucket(
    const std::string& key,
    std::unordered_map<std::string, std::shared_ptr<TokenBucket>>& buckets
) {
    auto it = buckets.find(key);
    if (it != buckets.end()) {
        return it->second;
    }
    
    // Check for custom limit
    size_t capacity = config_.bucket_capacity;
    double refill_rate = config_.refill_rate;
    
    auto custom_it = config_.custom_limits.find(key);
    if (custom_it != config_.custom_limits.end()) {
        capacity = custom_it->second;
        refill_rate = static_cast<double>(custom_it->second) / 60.0;
    }
    
    auto bucket = std::make_shared<TokenBucket>(capacity, refill_rate);
    buckets[key] = bucket;
    
    THEMIS_DEBUG("Created new rate limit bucket: key={}, capacity={}, rate={}/min", 
        key, capacity, refill_rate * 60.0);
    
    return bucket;
}

bool RateLimiter::allowRequest(const std::string& ip, const std::string& user_id) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    stats_.total_requests++;
    
    // Check blacklist (always block, regardless of whitelist or rate limits)
    if (blacklisted_ips_.count(ip) > 0) {
        stats_.rejected_requests++;
        THEMIS_WARN("Blocked request from blacklisted IP: {}", ip);
        return false;
    }
    
    // Check whitelist
    if (isWhitelisted(ip)) {
        stats_.allowed_requests++;
        return true;
    }

    // Adaptive throttle check: temporarily reduce capacity for misbehaving IPs
    if (config_.adaptive_throttling_enabled && !ip.empty()) {
        auto& entry = adaptive_state_[ip];
        auto now = std::chrono::steady_clock::now();
        if (entry.under_penalty) {
            if (now >= entry.penalty_until) {
                // Penalty has expired
                entry.under_penalty = false;
                entry.rejection_times.clear();
                THEMIS_INFO("Adaptive throttle penalty expired for IP: {}", ip);
            } else {
                // Still penalised: consume two tokens (2x harder to pass)
                auto bucket = getOrCreateBucket(ip, ip_buckets_);
                ip_last_access_[ip] = now;
                bool passed = bucket->tryConsume(2);
                if (passed) {
                    stats_.allowed_requests++;
                } else {
                    stats_.rejected_requests++;
                    recordRejectionForAdaptive(ip);
                }
                return passed;
            }
        }
    }
    
    bool allowed = true;
    
    // Check per-IP rate limit
    if (config_.per_ip_enabled && !ip.empty()) {
        auto bucket = getOrCreateBucket(ip, ip_buckets_);
        ip_last_access_[ip] = std::chrono::steady_clock::now();
        
        if (!bucket->tryConsume(1)) {
            THEMIS_WARN("Rate limit exceeded for IP: {}", ip);
            allowed = false;
        }
    }
    
    // Check per-user rate limit (stricter)
    if (config_.per_user_enabled && !user_id.empty()) {
        auto bucket = getOrCreateBucket(user_id, user_buckets_);
        user_last_access_[user_id] = std::chrono::steady_clock::now();
        
        if (!bucket->tryConsume(1)) {
            THEMIS_WARN("Rate limit exceeded for user: {}", user_id);
            allowed = false;
        }
    }
    
    if (allowed) {
        stats_.allowed_requests++;
    } else {
        stats_.rejected_requests++;
        if (config_.adaptive_throttling_enabled && !ip.empty()) {
            recordRejectionForAdaptive(ip);
        }
    }
    
    // Periodic cleanup
    auto now = std::chrono::steady_clock::now();
    auto cleanup_elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - last_cleanup_).count();
    
    if (cleanup_elapsed >= CLEANUP_INTERVAL_SECONDS) {
        cleanup();
        last_cleanup_ = now;
    }
    
    return allowed;
}

uint32_t RateLimiter::getRetryAfter(const std::string& ip, const std::string& user_id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    uint64_t max_retry_ms = 0;
    
    // Check IP bucket
    if (config_.per_ip_enabled && !ip.empty()) {
        auto it = ip_buckets_.find(ip);
        if (it != ip_buckets_.end()) {
            max_retry_ms = std::max(max_retry_ms, it->second->getRetryAfterMs());
        }
    }
    
    // Check user bucket
    if (config_.per_user_enabled && !user_id.empty()) {
        auto it = user_buckets_.find(user_id);
        if (it != user_buckets_.end()) {
            max_retry_ms = std::max(max_retry_ms, it->second->getRetryAfterMs());
        }
    }
    
    // Convert to seconds (round up)
    return static_cast<uint32_t>((max_retry_ms + 999) / 1000);
}

void RateLimiter::updateConfig(const RateLimitConfig& config) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    config_ = config;
    
    THEMIS_INFO("Rate Limiter config updated: {} req/min, bucket capacity: {}", 
        config_.refill_rate * 60.0, config_.bucket_capacity);
}

RateLimiter::Statistics RateLimiter::getStatistics() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    Statistics stats = stats_;
    stats.active_ip_buckets = ip_buckets_.size();
    stats.active_user_buckets = user_buckets_.size();

    // Count IPs currently under an active penalty
    auto now = std::chrono::steady_clock::now();
    size_t penalised = 0;
    for (const auto& [ip, entry] : adaptive_state_) {
        if (entry.under_penalty && now < entry.penalty_until) penalised++;
    }
    stats.adaptive_throttle_penalties = penalised;
    
    return stats;
}

void RateLimiter::reset() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    ip_buckets_.clear();
    ip_last_access_.clear();
    user_buckets_.clear();
    user_last_access_.clear();
    blacklisted_ips_.clear();
    adaptive_state_.clear();
    
    stats_ = Statistics();
    
    THEMIS_INFO("Rate Limiter reset");
}

void RateLimiter::cleanup() {
    auto now = std::chrono::steady_clock::now();
    
    // Cleanup inactive IP buckets (no activity for 10 minutes)
    size_t ip_removed = 0;
    for (auto it = ip_last_access_.begin(); it != ip_last_access_.end(); ) {
        auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(
            now - it->second).count();
        
        if (elapsed >= 10) {
            ip_buckets_.erase(it->first);
            it = ip_last_access_.erase(it);
            ip_removed++;
        } else {
            ++it;
        }
    }
    
    // Cleanup inactive user buckets (no activity for 10 minutes)
    size_t user_removed = 0;
    for (auto it = user_last_access_.begin(); it != user_last_access_.end(); ) {
        auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(
            now - it->second).count();
        
        if (elapsed >= 10) {
            user_buckets_.erase(it->first);
            it = user_last_access_.erase(it);
            user_removed++;
        } else {
            ++it;
        }
    }
    
    if (ip_removed > 0 || user_removed > 0) {
        THEMIS_DEBUG("Rate Limiter cleanup: removed {} IP buckets, {} user buckets", 
            ip_removed, user_removed);
    }
}

} // namespace server
} // namespace themis
