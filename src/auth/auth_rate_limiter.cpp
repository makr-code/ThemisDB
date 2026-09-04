/**
 * @file auth_rate_limiter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/auth_rate_limiter.h"

#include <algorithm>
#include <cstdio>
#include <ctime>

#include "auth/auth_audit_logger.h"
#include "auth/auth_metrics.h"
#include "utils/logger.h"

#ifdef THEMIS_ENABLE_REDIS
#include <hiredis/hiredis.h>
#endif

namespace themis {
namespace auth {

// ============================================================================
// AccountLockoutManager Implementation
// ============================================================================

AccountLockoutManager::AccountLockoutManager(const AuthRateLimitConfig &config)
    : config_(config), last_cleanup_(std::chrono::steady_clock::now()) {}

bool AccountLockoutManager::recordFailedAttempt(const std::string &user_id, const std::string &ip_address,
                                                const std::string &reason) {
    if (!config_.enable_account_lockout) {
        return false;
    }

    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto now   = std::chrono::system_clock::now();
    auto &info = lockout_state_[user_id];

    // If account is already locked, just return true
    if (info.is_locked && now < info.locked_until) {
        utils::Logger::warn("Authentication attempt on locked account: " + user_id);
        return true;
    }

    // If lockout expired, reset the state
    if (info.is_locked && now >= info.locked_until) {
        info.is_locked       = false;
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
                       [window_start](const FailedAttempt &fa) { return fa.timestamp < window_start; }),
        info.recent_failures.end());

    // Count failures within window
    info.failed_attempts = info.recent_failures.size();

    // Check if should lock
    if (shouldLockAccount(info)) {
        lockAccount(user_id, info);
        return true;
    }

    return false;
}

void AccountLockoutManager::recordSuccessfulAuth(const std::string &user_id) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

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

bool AccountLockoutManager::isAccountLocked(const std::string &user_id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

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

std::optional<LockoutInfo> AccountLockoutManager::getLockoutInfo(const std::string &user_id) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto it = lockout_state_.find(user_id);
    if (it == lockout_state_.end()) {
        return std::nullopt;
    }

    return it->second;
}

bool AccountLockoutManager::unlockAccount(const std::string &user_id) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto it = lockout_state_.find(user_id);
    if (it == lockout_state_.end() || !it->second.is_locked) {
        return false;
    }

    utils::Logger::info("Manually unlocking account: " + user_id);
    it->second.is_locked       = false;
    it->second.failed_attempts = 0;
    it->second.recent_failures.clear();
    lockout_state_.erase(it);

    return true;
}

size_t AccountLockoutManager::getLockedAccountCount() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);

    auto now     = std::chrono::system_clock::now();
    size_t count = 0;

    for (const auto &[user_id, info] : lockout_state_) {
        if (info.is_locked && now < info.locked_until) {
            count++;
        }
    }

    return count;
}

void AccountLockoutManager::forceLockAccount(const std::string &user_id, std::chrono::seconds duration) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    auto now          = std::chrono::system_clock::now();
    auto &info        = lockout_state_[user_id];
    info.is_locked    = true;
    info.locked_until = now + duration;
    utils::Logger::warn("Account force-locked for " + std::to_string(duration.count())
                        + "s (credential stuffing): " + user_id);
}

void AccountLockoutManager::cleanup() {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    auto now = std::chrono::system_clock::now();

    // Remove expired lockouts and old records
    for (auto it = lockout_state_.begin(); it != lockout_state_.end();) {
        auto &info = it->second;

        // Remove expired lockouts
        if (info.is_locked && now >= info.locked_until) {
            it = lockout_state_.erase(it);
            continue;
        }

        // Remove old records with no recent activity
        auto window_start = now - config_.lockout_window - config_.lockout_duration;
        if (!info.is_locked
            && info.last_failure
                   < std::chrono::system_clock::from_time_t(std::chrono::system_clock::to_time_t(window_start))) {
            it = lockout_state_.erase(it);
            continue;
        }

        ++it;
    }

    last_cleanup_ = std::chrono::steady_clock::now();
}

void AccountLockoutManager::reset() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    lockout_state_.clear();
}

void AccountLockoutManager::lockAccount(const std::string &user_id, const LockoutInfo &info) {
    auto now = std::chrono::system_clock::now();

    lockout_state_[user_id].is_locked    = true;
    lockout_state_[user_id].locked_until = now + config_.lockout_duration;

    utils::Logger::warn("Account locked due to failed authentication attempts: " + user_id
                        + " (failed attempts: " + std::to_string(info.failed_attempts) + ")");
}

bool AccountLockoutManager::shouldLockAccount(const LockoutInfo &info) const {
    return info.failed_attempts >= config_.lockout_failed_attempts;
}

// ============================================================================
// AuthRateLimiter Implementation
// ============================================================================

AuthRateLimiter::AuthRateLimiter(const AuthRateLimitConfig &config) : config_(config) {
    // Create IP rate limiter
    server::RateLimitConfig ip_config;
    ip_config.bucket_capacity  = config.max_attempts_per_ip_per_minute;
    ip_config.refill_rate      = static_cast<double>(config.max_attempts_per_ip_per_minute) / 60.0;
    ip_config.window_seconds   = 60;
    ip_config.per_ip_enabled   = true;
    ip_config.per_user_enabled = false;
    ip_config.whitelist_ips    = config.whitelist_ips;
    ip_rate_limiter_           = std::make_unique<server::RateLimiter>(ip_config);

    // Create user rate limiter
    server::RateLimitConfig user_config;
    user_config.bucket_capacity  = config.max_attempts_per_user_per_minute;
    user_config.refill_rate      = static_cast<double>(config.max_attempts_per_user_per_minute) / 60.0;
    user_config.window_seconds   = 60;
    user_config.per_ip_enabled   = false;
    user_config.per_user_enabled = true;
    user_rate_limiter_           = std::make_unique<server::RateLimiter>(user_config);

    // Create lockout manager
    lockout_manager_ = std::make_unique<AccountLockoutManager>(config);

#ifdef THEMIS_ENABLE_REDIS
    if (config_.enable_cs_persistent_backend) {
        try {
            std::lock_guard<std::mutex> rlock(cs_redis_mutex_);
            connectCsRedis();
        } catch (const std::exception &ex) {
            utils::Logger::error("AuthRateLimiter: unexpected exception during CS Redis init: "
                                 + std::string(ex.what()) + "; using in-process fallback");
        } catch (...) {
            utils::Logger::error("AuthRateLimiter: unknown exception during CS Redis init; "
                                 "using in-process fallback");
        }
    }
#else
    if (config_.enable_cs_persistent_backend) {
        utils::Logger::warn("AuthRateLimiter: enable_cs_persistent_backend=true but "
                            "built without THEMIS_ENABLE_REDIS; using in-process fallback");
    }
#endif
}

bool AuthRateLimiter::allowAuthAttempt(const std::string &ip_address, const std::string &user_id) {
    stat_total_auth_attempts_.fetch_add(1, std::memory_order_relaxed);

    // Snapshot the mutable shared state that requires a lock (config_ and
    // backend_) using a brief shared read-lock.  All rate-limiter calls
    // and any backend network I/O are then performed WITHOUT holding the
    // mutex, eliminating the lock-hold during Redis round-trips.
    AuthRateLimitConfig cfg;
    std::shared_ptr<IRateLimiterBackend> backend;
    {
        std::shared_lock<std::shared_mutex> lock(stats_mutex_);
        cfg     = config_;
        backend = backend_;
    }

    // Check if IP is whitelisted
    if (isWhitelisted(ip_address)) {
        stat_allowed_attempts_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    // Check account lockout first
    if (!user_id.empty() && cfg.enable_account_lockout) {
        if (lockout_manager_->isAccountLocked(user_id)) {
            stat_lockout_blocked_attempts_.fetch_add(1, std::memory_order_relaxed);
            utils::Logger::warn("Authentication blocked - account locked: " + user_id);
            return false;
        }
    }

    // Check IP rate limit
    if (cfg.enable_ip_rate_limiting) {
        if (backend) {
            auto count = backend->increment("ip:" + ip_address, 60);
            if (static_cast<size_t>(count) > cfg.max_attempts_per_ip_per_minute) {
                stat_rate_limited_attempts_.fetch_add(1, std::memory_order_relaxed);
                utils::Logger::warn("Authentication rate limited by IP (distributed): " + ip_address);
                return false;
            }
        } else if (!ip_rate_limiter_->allowRequest(ip_address)) {
            stat_rate_limited_attempts_.fetch_add(1, std::memory_order_relaxed);
            utils::Logger::warn("Authentication rate limited by IP: " + ip_address);
            return false;
        }
    }

    // Check user rate limit
    if (!user_id.empty() && cfg.enable_user_rate_limiting) {
        if (backend) {
            auto count = backend->increment("user:" + user_id, 60);
            if (static_cast<size_t>(count) > cfg.max_attempts_per_user_per_minute) {
                stat_rate_limited_attempts_.fetch_add(1, std::memory_order_relaxed);
                utils::Logger::warn("Authentication rate limited for user (distributed): " + user_id);
                return false;
            }
        } else if (!user_rate_limiter_->allowRequest("", user_id)) {
            stat_rate_limited_attempts_.fetch_add(1, std::memory_order_relaxed);
            utils::Logger::warn("Authentication rate limited for user: " + user_id);
            return false;
        }
    }

    // Track this attempt for credential-stuffing detection under stuffing_mutex_
    // (separate from stats_mutex_ so rate-limiter calls are not serialised here).
    bool stuffing_alert = false;
    if (!user_id.empty()) {
        std::unique_lock<std::mutex> slock(stuffing_mutex_);
        stuffing_alert = trackCredentialStuffing(ip_address, user_id, cfg);
    }

    stat_allowed_attempts_.fetch_add(1, std::memory_order_relaxed);

    // Fire anomaly events without holding any lock so callbacks can safely
    // call back into us.
    if (stuffing_alert) {
        const std::string detail          = "credential stuffing suspected: threshold reached from ip=" + ip_address;
        CredentialStuffingOutcome outcome = CredentialStuffingOutcome::ALLOWED;
        if (!user_id.empty()) {
            outcome = escalateCredentialStuffing(user_id, ip_address);
        }
        fireAuthAnomaly(AuthAnomalyEvent::Type::CREDENTIAL_STUFFING_SUSPECTED, ip_address, user_id, detail, outcome);
    }

    return true;
}

void AuthRateLimiter::recordFailedAuth(const std::string &user_id, const std::string &ip_address,
                                       const std::string &reason) {
    bool lockout_triggered = false;
    bool stuffing_alert    = false;

    stat_failed_auths_.fetch_add(1, std::memory_order_relaxed);

    // Snapshot config_ under a brief read lock before acquiring stuffing_mutex_.
    // This avoids a nested lock acquisition (stuffing_mutex_ then stats_mutex_)
    // in trackCredentialStuffing().
    AuthRateLimitConfig cfg;
    {
        std::shared_lock<std::shared_mutex> lock(stats_mutex_);
        cfg = config_;
    }

    // Track stuffing state under its own mutex (not stats_mutex_).
    if (!user_id.empty()) {
        std::unique_lock<std::mutex> slock(stuffing_mutex_);
        stuffing_alert = trackCredentialStuffing(ip_address, user_id, cfg);
    }

    if (!user_id.empty()) {
        lockout_triggered = lockout_manager_->recordFailedAttempt(user_id, ip_address, reason);
        if (lockout_triggered) {
            stat_currently_locked_accounts_.store(lockout_manager_->getLockedAccountCount(), std::memory_order_relaxed);
        }
    }

    // Fire anomaly events without holding any lock.
    if (stuffing_alert) {
        const std::string detail = "credential stuffing suspected: threshold reached from ip=" + ip_address;
        utils::Logger::warn("Auth anomaly [CREDENTIAL_STUFFING_SUSPECTED] ip=" + ip_address + " " + detail);
        CredentialStuffingOutcome outcome = CredentialStuffingOutcome::ALLOWED;
        if (!user_id.empty()) {
            outcome = escalateCredentialStuffing(user_id, ip_address);
        }
        fireAuthAnomaly(AuthAnomalyEvent::Type::CREDENTIAL_STUFFING_SUSPECTED, ip_address, user_id, detail, outcome);
    }

    if (lockout_triggered && !user_id.empty()) {
        const std::string lockout_detail = "account locked after repeated failures (reason: " + reason + ")";
        utils::Logger::warn("Auth anomaly [ACCOUNT_LOCKOUT_TRIGGERED] user=" + user_id + " ip=" + ip_address + " "
                            + lockout_detail);
        fireAuthAnomaly(AuthAnomalyEvent::Type::ACCOUNT_LOCKOUT_TRIGGERED, ip_address, user_id, lockout_detail);

        const std::string bf_detail
            = "brute-force detected: lockout triggered from ip=" + ip_address + " (reason: " + reason + ")";
        utils::Logger::warn("Auth anomaly [BRUTE_FORCE_DETECTED] user=" + user_id + " ip=" + ip_address + " "
                            + bf_detail);
        fireAuthAnomaly(AuthAnomalyEvent::Type::BRUTE_FORCE_DETECTED, ip_address, user_id, bf_detail);
    }
}

void AuthRateLimiter::recordSuccessfulAuth(const std::string &user_id, [[maybe_unused]] const std::string &ip_address) {
    stat_successful_auths_.fetch_add(1, std::memory_order_relaxed);

    if (!user_id.empty()) {
        lockout_manager_->recordSuccessfulAuth(user_id);
    }
}

bool AuthRateLimiter::isAccountLocked(const std::string &user_id) const {
    return lockout_manager_->isAccountLocked(user_id);
}

std::optional<LockoutInfo> AuthRateLimiter::getLockoutInfo(const std::string &user_id) const {
    return lockout_manager_->getLockoutInfo(user_id);
}

bool AuthRateLimiter::unlockAccount(const std::string &user_id) {
    bool unlocked = lockout_manager_->unlockAccount(user_id);
    if (unlocked) {
        stat_currently_locked_accounts_.store(lockout_manager_->getLockedAccountCount(), std::memory_order_relaxed);
    }
    return unlocked;
}

uint32_t AuthRateLimiter::getRetryAfter(const std::string &ip_address) const {
    std::shared_ptr<IRateLimiterBackend> backend;
    size_t max_per_min = {};
    {
        std::shared_lock<std::shared_mutex> lock(stats_mutex_);
        backend     = backend_;
        max_per_min = config_.max_attempts_per_ip_per_minute;
    }
    if (backend) {
        auto count = backend->getCount("ip:" + ip_address, 60);
        if (static_cast<size_t>(count) >= max_per_min) {
            return 60;
        }
        return 0;
    }
    return ip_rate_limiter_->getRetryAfter(ip_address, "");
}

bool AuthRateLimiter::isWhitelisted(const std::string &ip_address) const {
    return ip_rate_limiter_->isWhitelisted(ip_address);
}

void AuthRateLimiter::setAnomalyCallback([[maybe_unused]] AuthAnomalyCallback callback) {
    std::unique_lock<std::shared_mutex> lock([[maybe_unused]] callback_mutex_);
    anomaly_callback_ = std::move([[maybe_unused]] callback);
}

void AuthRateLimiter::setAuditLogger(utils::AuditLogger *logger) {
    std::unique_lock<std::shared_mutex> lock([[maybe_unused]] callback_mutex_);
    audit_logger_ = logger;
}

void AuthRateLimiter::setBackend(std::shared_ptr<IRateLimiterBackend> backend) {
    std::unique_lock<std::shared_mutex> lock(stats_mutex_);
    backend_ = std::move(backend);
}

void AuthRateLimiter::setMetrics(AuthMetrics *metrics) {
    std::unique_lock<std::shared_mutex> lock([[maybe_unused]] callback_mutex_);
    metrics_ = metrics;
}

// ── Credential-stuffing persistent breach-count helpers ──────────────────────

/*static*/ std::string AuthRateLimiter::csBreachKey(const std::string &user_id) {
    // Key format: "cs:{user_id}:{YYYYMMDD}" (UTC day)
    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char day[16];
    std::snprintf(day, sizeof(day), "%04d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    return "cs:" + user_id + ":" + day;
}

// TTL applied to Redis breach-count keys: 25 hours ensures the key outlives
// the UTC day boundary and is cleaned up shortly after midnight.
static constexpr int kCsBreachKeyTtlSeconds = 25 * 3600;

// Duration of the exponential back-off hard lock applied on the third breach.
static constexpr auto kCsLockDuration = std::chrono::hours(24);

// Convert a CredentialStuffingOutcome to the canonical string used as a
// Prometheus label value and in log messages.
static std::string outcomeToString(CredentialStuffingOutcome outcome) {
    switch (outcome) {
        case CredentialStuffingOutcome::CAPTCHA_REQUIRED:
            return "captcha_required";
        case CredentialStuffingOutcome::OTP_REQUIRED:
            return "otp_required";
        case CredentialStuffingOutcome::ACCOUNT_LOCKED_24H:
            return "account_locked_24h";
        default:
            return "allowed";
    }
}

/*static*/ CredentialStuffingOutcome AuthRateLimiter::outcomeFromBreachCount([[maybe_unused]] uint32_t count) {
    if (count >= 3) {
        return CredentialStuffingOutcome::ACCOUNT_LOCKED_24H;
    }
    if (count == 2) {
        return CredentialStuffingOutcome::OTP_REQUIRED;
    }
    if (count == 1) {
        return CredentialStuffingOutcome::CAPTCHA_REQUIRED;
    }
    return CredentialStuffingOutcome::ALLOWED;
}

uint32_t AuthRateLimiter::incrementAndGetBreachCount(const std::string &user_id) {
    const std::string key = csBreachKey(user_id);

#ifdef THEMIS_ENABLE_REDIS
    if (config_.enable_cs_persistent_backend) {
        try {
            std::lock_guard<std::mutex> rlock(cs_redis_mutex_);
            if (!cs_redis_ctx_ && !connectCsRedis()) {
                // Redis unavailable – fall through to in-memory
            } else if (cs_redis_ctx_) {
                // INCR key
                redisReply *reply = static_cast<redisReply *>(redisCommand(cs_redis_ctx_, "INCR %s", key.c_str()));
                if (reply && reply->type == REDIS_REPLY_INTEGER) {
                    long long count = reply->integer;
                    freeReplyObject(reply);
                    // Set TTL only when the key is first created so it expires near
                    // the intended day boundary and is not extended by subsequent
                    // breaches under the same key.
                    if (count == 1) {
                        redisReply *ex = static_cast<redisReply *>(
                            redisCommand(cs_redis_ctx_, "EXPIRE %s %d", key.c_str(), kCsBreachKeyTtlSeconds));
                        if (ex)
                            freeReplyObject(ex);
                    }
                    return static_cast<uint32_t>(count);
                }
                if (reply)
                    freeReplyObject(reply);
                // Redis error – fall through to in-memory
                redisFree(cs_redis_ctx_);
                cs_redis_ctx_ = nullptr;
            }
        } catch (const std::exception &ex) {
            utils::Logger::error("AuthRateLimiter: exception in CS Redis increment for key '" + key
                                 + "': " + ex.what() + "; falling back to in-process counter");
            // cs_redis_ctx_ may be in an unknown state; invalidate it so the
            // next call attempts a fresh reconnect.
            if (cs_redis_ctx_) {
                redisFree(cs_redis_ctx_);
                cs_redis_ctx_ = nullptr;
            }
        } catch (...) {
            utils::Logger::error("AuthRateLimiter: unknown exception in CS Redis increment for key '" + key
                                 + "'; falling back to in-process counter");
            if (cs_redis_ctx_) {
                redisFree(cs_redis_ctx_);
                cs_redis_ctx_ = nullptr;
            }
        }
    }
#endif

    // In-memory fallback
    std::lock_guard<std::mutex> mlock(cs_breach_mutex_);
    return ++cs_breach_count_[key];
}

CredentialStuffingOutcome AuthRateLimiter::escalateCredentialStuffing(const std::string &user_id,
                                                                      const std::string &ip) {
    if (user_id.empty()) {
        return CredentialStuffingOutcome::ALLOWED;
    }

    const uint32_t count = incrementAndGetBreachCount(user_id);
    const auto outcome   = outcomeFromBreachCount(count);

    if (outcome == CredentialStuffingOutcome::ACCOUNT_LOCKED_24H) {
        lockout_manager_->forceLockAccount(user_id, kCsLockDuration);
        utils::Logger::warn("Credential stuffing escalation: 24h lock applied for user=" + user_id + " ip=" + ip
                            + " (breach_count=" + std::to_string(count) + ")");
        stat_currently_locked_accounts_.store(lockout_manager_->getLockedAccountCount(), std::memory_order_relaxed);
    } else if (outcome == CredentialStuffingOutcome::OTP_REQUIRED) {
        utils::Logger::warn("Credential stuffing escalation: OTP required for user=" + user_id + " ip=" + ip
                            + " (breach_count=" + std::to_string(count) + ")");
    } else if (outcome == CredentialStuffingOutcome::CAPTCHA_REQUIRED) {
        utils::Logger::warn("Credential stuffing escalation: CAPTCHA required for user=" + user_id + " ip=" + ip
                            + " (breach_count=" + std::to_string(count) + ")");
    }

    return outcome;
}

#ifdef THEMIS_ENABLE_REDIS
bool AuthRateLimiter::connectCsRedis() {
    // Caller must hold cs_redis_mutex_.
    struct timeval tv;
    tv.tv_sec  = config_.cs_redis.timeout_ms / 1000;
    tv.tv_usec = (config_.cs_redis.timeout_ms % 1000) * 1000;

    cs_redis_ctx_ = redisConnectWithTimeout(config_.cs_redis.host.c_str(), config_.cs_redis.port, tv);
    if (!cs_redis_ctx_ || cs_redis_ctx_->err) {
        utils::Logger::warn("AuthRateLimiter: CS Redis connect to " + config_.cs_redis.host + ":"
                            + std::to_string(config_.cs_redis.port) + " failed; using in-process fallback");
        if (cs_redis_ctx_) {
            redisFree(cs_redis_ctx_);
            cs_redis_ctx_ = nullptr;
        }
        return false;
    }
    if (!config_.cs_redis.auth.empty()) {
        redisReply *reply
            = static_cast<redisReply *>(redisCommand(cs_redis_ctx_, "AUTH %s", config_.cs_redis.auth.c_str()));
        bool ok = reply && reply->type != REDIS_REPLY_ERROR;
        if (reply)
            freeReplyObject(reply);
        if (!ok) {
            utils::Logger::warn("AuthRateLimiter: CS Redis AUTH failed");
            redisFree(cs_redis_ctx_);
            cs_redis_ctx_ = nullptr;
            return false;
        }
    }
    utils::Logger::info("AuthRateLimiter: CS Redis connected to " + config_.cs_redis.host + ":"
                        + std::to_string(config_.cs_redis.port));
    return true;
}
#endif

// ─────────────────────────────────────────────────────────────────────────────

void AuthRateLimiter::fireAuthAnomaly(AuthAnomalyEvent::Type type, const std::string &ip, const std::string &user_id,
                                      const std::string &detail, CredentialStuffingOutcome cs_outcome) const {
    AuthAnomalyCallback cb;
    utils::AuditLogger *al = nullptr;
    AuthMetrics *met       = nullptr;
    {
        std::shared_lock<std::shared_mutex> lock([[maybe_unused]] callback_mutex_);
        cb  = anomaly_callback_;
        al  = audit_logger_;
        met = metrics_;
    }
    if (cb) {
        AuthAnomalyEvent ev{type, ip, user_id, detail, std::chrono::system_clock::now(), cs_outcome};
        cb(ev);
    }
    if ([[maybe_unused]] met && type == AuthAnomalyEvent::Type::CREDENTIAL_STUFFING_SUSPECTED) {
        met->recordCredentialStuffingAttempt(user_id, ip, outcomeToString(cs_outcome));
    }
    if (al) {
        AuthAuditLogger audit(al);
        switch (type) {
            case AuthAnomalyEvent::Type::BRUTE_FORCE_DETECTED:
                audit.logBruteForceDetected(user_id, ip, config_.lockout_failed_attempts);
                break;
            case AuthAnomalyEvent::Type::CREDENTIAL_STUFFING_SUSPECTED:
                audit.logCredentialStuffingSuspected(ip, config_.credential_stuffing_user_threshold);
                break;
            case AuthAnomalyEvent::Type::ACCOUNT_LOCKOUT_TRIGGERED:
                audit.logAccountLockoutTriggered(user_id, ip);
                break;
        }
    }
}

bool AuthRateLimiter::trackCredentialStuffing(const std::string &ip, const std::string &user_id,
                                              const AuthRateLimitConfig &cfg) {
    // Called with stuffing_mutex_ held.
    // cfg is a snapshot captured by the caller before acquiring stuffing_mutex_,
    // so no further lock on stats_mutex_ is needed here.
    if (!cfg.enable_credential_stuffing_detection || user_id.empty()) {
        return false;
    }

    auto now    = std::chrono::steady_clock::now();
    auto window = std::chrono::seconds(cfg.credential_stuffing_window_seconds);
    auto &entry = stuffing_state_[ip];
    auto cutoff = now - window;

    // attempt_times is maintained in chronological (insertion) order, so all
    // expired entries form a contiguous prefix. Erase it in O(k) rather than
    // the O(n) erase-remove approach.
    auto first_valid = std::lower_bound(entry.attempt_times.begin(), entry.attempt_times.end(), cutoff);
    entry.attempt_times.erase(entry.attempt_times.begin(), first_valid);

    // If the window has fully expired, reset alert state so a new attack wave
    // can trigger another alert.
    if (entry.attempt_times.empty()) {
        entry.alerted = false;
        entry.usernames.clear();
    }

    entry.attempt_times.push_back(now);
    entry.usernames.insert(user_id);

    if (!entry.alerted && entry.usernames.size() >= cfg.credential_stuffing_user_threshold) {
        entry.alerted = true;
        // Caller fires the anomaly event outside the lock.
        return true;
    }
    return false;
}

void AuthRateLimiter::updateConfig(const AuthRateLimitConfig &config) {
    // Update rate limiters
    server::RateLimitConfig ip_config;
    ip_config.bucket_capacity = config.max_attempts_per_ip_per_minute;
    ip_config.refill_rate     = static_cast<double>(config.max_attempts_per_ip_per_minute) / 60.0;
    ip_config.whitelist_ips   = config.whitelist_ips;
    ip_rate_limiter_->updateConfig(ip_config);

    server::RateLimitConfig user_config;
    user_config.bucket_capacity = config.max_attempts_per_user_per_minute;
    user_config.refill_rate     = static_cast<double>(config.max_attempts_per_user_per_minute) / 60.0;
    user_rate_limiter_->updateConfig(user_config);

    std::unique_lock<std::shared_mutex> lock(stats_mutex_);
    config_ = config;
    // Clearing the credential-stuffing state on a config update is intentional: it
    // ensures the new threshold and window take effect immediately for all IPs.
    // A side-effect is that any IP currently being tracked will restart from zero.
    // Callers that need continuity should trigger cleanup() before updateConfig().
    //
    // stats_mutex_ is held here; we then acquire stuffing_mutex_ — the one place
    // in the codebase where both locks are nested.  The hierarchy (stats_mutex_
    // before stuffing_mutex_) is documented in include/auth/auth_rate_limiter.h.
    // Hot paths (allowAuthAttempt / recordFailedAuth) never nest these two locks.
    std::unique_lock<std::mutex> slock(stuffing_mutex_);
    stuffing_state_.clear();
}

AuthRateLimiter::Statistics AuthRateLimiter::getStatistics() const {
    Statistics s;
    s.total_auth_attempts       = stat_total_auth_attempts_.load(std::memory_order_relaxed);
    s.allowed_attempts          = stat_allowed_attempts_.load(std::memory_order_relaxed);
    s.rate_limited_attempts     = stat_rate_limited_attempts_.load(std::memory_order_relaxed);
    s.lockout_blocked_attempts  = stat_lockout_blocked_attempts_.load(std::memory_order_relaxed);
    s.successful_auths          = stat_successful_auths_.load(std::memory_order_relaxed);
    s.failed_auths              = stat_failed_auths_.load(std::memory_order_relaxed);
    s.currently_locked_accounts = stat_currently_locked_accounts_.load(std::memory_order_relaxed);
    return s;
}

void AuthRateLimiter::reset() {
    // Reset sub-objects WITHOUT holding stats_mutex_ to avoid a lock-ordering
    // violation: AccountLockoutManager::reset() acquires lockout_manager_->mutex_
    // internally, and other hot paths (recordFailedAuth, allowAuthAttempt) acquire
    // that same mutex WITHOUT stats_mutex_, so nesting them would invert the order
    // and risk a deadlock.  stats_mutex_ guards only config_ and backend_; it does
    // not need to span the sub-object resets.
    ip_rate_limiter_->reset();
    user_rate_limiter_->reset();
    lockout_manager_->reset();
    {
        std::unique_lock<std::mutex> slock(stuffing_mutex_);
        stuffing_state_.clear();
    }
    // Zero all atomic counters.
    stat_total_auth_attempts_.store(0, std::memory_order_relaxed);
    stat_allowed_attempts_.store(0, std::memory_order_relaxed);
    stat_rate_limited_attempts_.store(0, std::memory_order_relaxed);
    stat_lockout_blocked_attempts_.store(0, std::memory_order_relaxed);
    stat_successful_auths_.store(0, std::memory_order_relaxed);
    stat_failed_auths_.store(0, std::memory_order_relaxed);
    stat_currently_locked_accounts_.store(0, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> mlock(cs_breach_mutex_);
        cs_breach_count_.clear();
    }
}

void AuthRateLimiter::cleanup() {
    ip_rate_limiter_->cleanup();
    user_rate_limiter_->cleanup();
    lockout_manager_->cleanup();

    // Prune stale credential-stuffing state for IPs whose rolling window has expired.
    size_t stuffing_window_secs = {};
    {
        std::shared_lock<std::shared_mutex> lock(stats_mutex_);
        stuffing_window_secs = config_.credential_stuffing_window_seconds;
    }
    auto now    = std::chrono::steady_clock::now();
    auto cutoff = now - std::chrono::seconds(stuffing_window_secs);

    std::unique_lock<std::mutex> slock(stuffing_mutex_);
    for (auto it = stuffing_state_.begin(); it != stuffing_state_.end();) {
        auto &entry = it->second;
        // attempt_times is chronological – erase the expired prefix efficiently.
        auto first_valid = std::lower_bound(entry.attempt_times.begin(), entry.attempt_times.end(), cutoff);
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
