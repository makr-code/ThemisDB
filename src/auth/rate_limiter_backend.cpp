/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rate_limiter_backend.cpp                           ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 18:48:41                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     311                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • daf194f998  2026-03-12  feat(auth): implement rate limiter distributed state sync... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "auth/rate_limiter_backend.h"
#include "utils/logger.h"

#ifdef THEMIS_ENABLE_REDIS
#include <hiredis/hiredis.h>
#endif

#include <algorithm>
#include <cstring>

namespace themis {
namespace auth {

// ============================================================================
// InMemoryRateLimiterBackend
// ============================================================================

int64_t InMemoryRateLimiterBackend::increment(const std::string& key,
                                               uint32_t window_seconds)
{
    auto now    = std::chrono::steady_clock::now();
    auto cutoff = now - std::chrono::seconds(window_seconds);

    std::lock_guard<std::mutex> lock(mutex_);

    auto& timestamps = counters_[key];

    // Prune entries that have fallen outside the sliding window.
    // The vector is maintained in insertion order (monotonically increasing),
    // so all stale entries form a contiguous prefix — erase in O(k).
    auto first_valid = std::lower_bound(timestamps.begin(), timestamps.end(), cutoff);
    timestamps.erase(timestamps.begin(), first_valid);

    // Record this request.
    timestamps.push_back(now);

    return static_cast<int64_t>(timestamps.size());
}

int64_t InMemoryRateLimiterBackend::getCount(const std::string& key,
                                              uint32_t window_seconds) const
{
    auto now    = std::chrono::steady_clock::now();
    auto cutoff = now - std::chrono::seconds(window_seconds);

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = counters_.find(key);
    if (it == counters_.end()) {
        return 0;
    }

    const auto& timestamps = it->second;
    auto first_valid = std::lower_bound(timestamps.begin(), timestamps.end(), cutoff);
    return static_cast<int64_t>(std::distance(first_valid, timestamps.end()));
}

void InMemoryRateLimiterBackend::reset(const std::string& key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    counters_.erase(key);
}

// ============================================================================
// RedisRateLimiterBackend
// ============================================================================

#ifdef THEMIS_ENABLE_REDIS

std::atomic<uint64_t> RedisRateLimiterBackend::member_counter_{0};

std::string RedisRateLimiterBackend::makeKey(const std::string& key) const
{
    return config_.key_prefix + key;
}

bool RedisRateLimiterBackend::connect()
{
    if (ctx_) {
        redisFree(ctx_);
        ctx_ = nullptr;
    }

    struct timeval tv;
    tv.tv_sec  = config_.connect_timeout_ms / 1000;
    tv.tv_usec = (config_.connect_timeout_ms % 1000) * 1000;

    ctx_ = redisConnectWithTimeout(config_.host.c_str(), config_.port, tv);
    if (!ctx_ || ctx_->err) {
        THEMIS_WARN("RedisRateLimiterBackend: connect to {}:{} failed: {}",
                    config_.host, config_.port,
                    ctx_ ? ctx_->errstr : "allocation failure");
        if (ctx_) { redisFree(ctx_); ctx_ = nullptr; }
        return false;
    }

    if (!config_.auth.empty()) {
        redisReply* reply = static_cast<redisReply*>(
            redisCommand(ctx_, "AUTH %s", config_.auth.c_str()));
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            THEMIS_WARN("RedisRateLimiterBackend: AUTH failed: {}",
                        reply ? reply->str : "no reply");
            if (reply) freeReplyObject(reply);
            redisFree(ctx_);
            ctx_ = nullptr;
            return false;
        }
        freeReplyObject(reply);
    }

    THEMIS_INFO("RedisRateLimiterBackend: connected to {}:{}", config_.host, config_.port);
    return true;
}

void RedisRateLimiterBackend::disconnect()
{
    if (ctx_) {
        redisFree(ctx_);
        ctx_ = nullptr;
    }
}

RedisRateLimiterBackend::RedisRateLimiterBackend(const Config& config)
    : config_(config)
{
    connect();
}

RedisRateLimiterBackend::~RedisRateLimiterBackend()
{
    std::lock_guard<std::mutex> lock(mutex_);
    disconnect();
}

int64_t RedisRateLimiterBackend::increment(const std::string& key,
                                            uint32_t window_seconds)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) {
        THEMIS_WARN("RedisRateLimiterBackend::increment: not connected – allowing request (fail-open)");
        return 0;
    }

    auto now_us    = static_cast<long long>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    auto window_us = static_cast<long long>(window_seconds) * 1'000'000LL;

    // Each sorted-set member must be unique; use timestamp + per-node counter.
    uint64_t seq   = member_counter_.fetch_add(1, std::memory_order_relaxed);
    std::string member = std::to_string(now_us) + ":" + std::to_string(seq);

    const std::string full_key = makeKey(key);

    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "EVAL %s 1 %s %lld %lld %u %s",
                     kIncrScript,
                     full_key.c_str(),
                     now_us,
                     window_us,
                     window_seconds,
                     member.c_str()));

    if (!reply) {
        THEMIS_WARN("RedisRateLimiterBackend::increment: command failed: {}", ctx_->errstr);
        disconnect();
        return 0; // fail-open
    }

    int64_t count = 0;
    if (reply->type == REDIS_REPLY_INTEGER) {
        count = reply->integer;
    } else if (reply->type == REDIS_REPLY_ERROR) {
        THEMIS_WARN("RedisRateLimiterBackend::increment: Lua error: {}",
                    reply->str ? reply->str : "unknown");
    }

    freeReplyObject(reply);
    return count;
}

int64_t RedisRateLimiterBackend::getCount(const std::string& key,
                                           uint32_t window_seconds) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return 0;

    auto now_us    = static_cast<long long>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    auto window_us = static_cast<long long>(window_seconds) * 1'000'000LL;

    const std::string full_key = makeKey(key);

    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "EVAL %s 1 %s %lld %lld",
                     kCountScript,
                     full_key.c_str(),
                     now_us,
                     window_us));

    if (!reply) {
        THEMIS_WARN("RedisRateLimiterBackend::getCount: command failed: {}", ctx_->errstr);
        const_cast<RedisRateLimiterBackend*>(this)->disconnect();
        return 0;
    }

    int64_t count = 0;
    if (reply->type == REDIS_REPLY_INTEGER) {
        count = reply->integer;
    }

    freeReplyObject(reply);
    return count;
}

void RedisRateLimiterBackend::reset(const std::string& key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return;

    const std::string full_key = makeKey(key);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "DEL %s", full_key.c_str()));

    if (reply) {
        freeReplyObject(reply);
    } else {
        THEMIS_WARN("RedisRateLimiterBackend::reset: command failed: {}", ctx_->errstr);
        disconnect();
    }
}

bool RedisRateLimiterBackend::isConnected() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return ctx_ != nullptr && ctx_->err == 0;
}

bool RedisRateLimiterBackend::reconnect()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return connect();
}

#else // !THEMIS_ENABLE_REDIS

// Use the interface sentinel constant (IRateLimiterBackend::kBackendUnavailable)
// for fail-closed rate limiting so callers can distinguish "backend unavailable"
// from an arithmetically-computed rate-limit count.
static constexpr int64_t kRateLimitFailClosed = IRateLimiterBackend::kBackendUnavailable;

// STUB/SIMULATION NOTE:
// Purpose: Allow ThemisDB to be built without hiredis.  All Redis-backed
//   distributed rate-limiting operations are now fail-CLOSED: `increment()`
//   returns kRateLimitFailClosed (== IRateLimiterBackend::kBackendUnavailable)
//   so that every call appears to exceed the rate limit, `getCount()` returns
//   kRateLimitFailClosed, `reset()` is a no-op, and `isConnected()` /
//   `reconnect()` always return false.
// Activation: `THEMIS_ENABLE_REDIS` is not defined at compile time (default for
//   builds without the 'redis' vcpkg feature or without libhiredis).
// Production Delta: Distributed rate limiting is disabled and all requests are
//   REJECTED (fail-closed) to prevent DoS/bypass.  Use InMemoryRateLimiterBackend
//   for a single-node in-process fallback that does not share state across
//   ThemisDB replicas.
// Removal Plan: Install libhiredis (`apt install libhiredis-dev` or enable the
//   'redis' vcpkg feature) and set `-DTHEMIS_ENABLE_REDIS=1` in CMake.
// Roadmap ref: src/auth/FUTURE_ENHANCEMENTS.md §"Redis Rate Limiter Activation"

RedisRateLimiterBackend::RedisRateLimiterBackend(const Config& config)
    : config_(config)
{
    THEMIS_WARN("RedisRateLimiterBackend: built without hiredis (THEMIS_ENABLE_REDIS not "
                "defined).  Distributed rate limiting is unavailable; all requests will be "
                "REJECTED (fail-closed).  Use InMemoryRateLimiterBackend for single-node "
                "rate limiting, or enable the 'redis' vcpkg feature to activate distributed "
                "rate limiting.");
}

RedisRateLimiterBackend::~RedisRateLimiterBackend() = default;

int64_t RedisRateLimiterBackend::increment(const std::string& /*key*/,
                                            uint32_t /*window_seconds*/)
{
    return kRateLimitFailClosed; // fail-closed: report maximum count so callers reject the request
}

int64_t RedisRateLimiterBackend::getCount(const std::string& /*key*/,
                                           uint32_t /*window_seconds*/) const
{
    return kRateLimitFailClosed; // fail-closed: appear at maximum rate
}

void RedisRateLimiterBackend::reset(const std::string& /*key*/)
{
    // no-op stub
}

bool RedisRateLimiterBackend::isConnected() const
{
    return false;
}

bool RedisRateLimiterBackend::reconnect()
{
    return false;
}

#endif // THEMIS_ENABLE_REDIS

} // namespace auth
} // namespace themis
