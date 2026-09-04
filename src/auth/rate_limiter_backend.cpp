/**
 * @file rate_limiter_backend.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=5, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/rate_limiter_backend.h"
#include <stdexcept>
#include "utils/logger.h"

#ifdef THEMIS_ENABLE_REDIS
#include <hiredis/hiredis.h>
#endif

#include <algorithm>
#include <cstring>

namespace themis {
namespace auth {

namespace {
std::mutex                              s_redis_rate_bridge_mutex;
RedisRateLimiterBackend::IncrementFn    s_increment_fn;
RedisRateLimiterBackend::GetCountFn     s_get_count_fn;
RedisRateLimiterBackend::ResetFn        s_reset_fn;
RedisRateLimiterBackend::IsConnectedFn  s_is_connected_fn;
RedisRateLimiterBackend::ReconnectFn    s_reconnect_fn;
}

void RedisRateLimiterBackend::setIncrementFn(IncrementFn fn) {
    std::lock_guard<std::mutex> lk(s_redis_rate_bridge_mutex);
    s_increment_fn = std::move(fn);
}

void RedisRateLimiterBackend::setGetCountFn(GetCountFn fn) {
    std::lock_guard<std::mutex> lk(s_redis_rate_bridge_mutex);
    s_get_count_fn = std::move(fn);
}

void RedisRateLimiterBackend::setResetFn(ResetFn fn) {
    std::lock_guard<std::mutex> lk(s_redis_rate_bridge_mutex);
    s_reset_fn = std::move(fn);
}

void RedisRateLimiterBackend::setIsConnectedFn(IsConnectedFn fn) {
    std::lock_guard<std::mutex> lk(s_redis_rate_bridge_mutex);
    s_is_connected_fn = std::move(fn);
}

void RedisRateLimiterBackend::setReconnectFn(ReconnectFn fn) {
    std::lock_guard<std::mutex> lk(s_redis_rate_bridge_mutex);
    s_reconnect_fn = std::move(fn);
}

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

namespace {
const char* redisErrStrSafe(const redisContext* ctx) {
    if (!ctx || !ctx->errstr) {
        return "unknown redis error";
    }
    return ctx->errstr;
}
} // namespace

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
                    ctx_ ? redisErrStrSafe(ctx_) : "allocation failure");
        if (ctx_) { redisFree(ctx_); ctx_ = nullptr; }
        return false;
    }

    if (!config_.auth.empty()) {
        redisReply* reply = static_cast<redisReply*>(
            redisCommand(ctx_, "AUTH %s", config_.auth.c_str()));
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            THEMIS_WARN("RedisRateLimiterBackend: AUTH failed: {}",
                        reply ? reply->str : "no reply");
            if (reply) {
              freeReplyObject(reply);
            }
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
        THEMIS_WARN("RedisRateLimiterBackend::increment: command failed: {}", redisErrStrSafe(ctx_));
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
    if (!ctx_) {
      return 0;
    }

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
        THEMIS_WARN("RedisRateLimiterBackend::getCount: command failed: {}", redisErrStrSafe(ctx_));
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
    if (!ctx_) {
      return;
    }

    const std::string full_key = makeKey(key);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "DEL %s", full_key.c_str()));

    if (reply) {
        freeReplyObject(reply);
    } else {
        THEMIS_WARN("RedisRateLimiterBackend::reset: command failed: {}", redisErrStrSafe(ctx_));
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

// PERMANENT FALLBACK NOTE:
// Purpose: Allow ThemisDB to be built without hiredis.  All Redis-backed
//   distributed rate-limiting operations delegate to a process-local in-memory
//   fallback (InMemoryRateLimiterBackend).  This fallback is intentional and
//   permanent for builds without the Redis feature.
// Activation: `THEMIS_ENABLE_REDIS` is not defined at compile time (default for
//   builds without the 'redis' vcpkg feature or without libhiredis).
// Production Delta: Cross-replica/shared Redis counters are unavailable; limits
//   are enforced per process only and do not synchronize across ThemisDB replicas.
// Real implementation: Install libhiredis (`apt install libhiredis-dev` or enable
//   the 'redis' vcpkg feature) and set `-DTHEMIS_ENABLE_REDIS=1` in CMake.
//   The full hiredis-backed sliding-window ZSET implementation is in the
//   `#ifdef THEMIS_ENABLE_REDIS` block above (connect, increment, getCount, reset).
// Roadmap ref: src/auth/FUTURE_ENHANCEMENTS.md §"Redis Rate Limiter Activation"

namespace {
InMemoryRateLimiterBackend& redisFallbackBackend()
{
    static InMemoryRateLimiterBackend backend;
    return backend;
}
} // namespace

RedisRateLimiterBackend::RedisRateLimiterBackend(const Config& config)
    : config_(config)
{
    THEMIS_WARN("RedisRateLimiterBackend: built without hiredis (THEMIS_ENABLE_REDIS not "
                "defined).  Using process-local in-memory fallback counters; distributed "
                "rate-limit synchronization across replicas is unavailable. Enable the "
                "'redis' vcpkg feature to activate Redis-backed shared counters.");
}

RedisRateLimiterBackend::~RedisRateLimiterBackend() = default;

int64_t RedisRateLimiterBackend::increment(const std::string& key,
                                            uint32_t window_seconds)
{
    IncrementFn fn;
    {
        std::lock_guard<std::mutex> lk(s_redis_rate_bridge_mutex);
        fn = s_increment_fn;
    }
    if (fn) {
        try {
            return fn(key, window_seconds);
        } catch (const std::exception& ex) {
            utils::Logger::warn("RedisRateLimiterBackend: increment bridge threw: " + std::string(ex.what()) + "; using in-memory fallback");
        } catch (...) {
            utils::Logger::warn("RedisRateLimiterBackend: increment bridge threw unknown exception; using in-memory fallback");
        }
    }
    return redisFallbackBackend().increment(key, window_seconds);
}

int64_t RedisRateLimiterBackend::getCount(const std::string& key,
                                           uint32_t window_seconds) const
{
    GetCountFn fn;
    {
        std::lock_guard<std::mutex> lk(s_redis_rate_bridge_mutex);
        fn = s_get_count_fn;
    }
    if (fn) {
        try {
            return fn(key, window_seconds);
        } catch (const std::exception& ex) {
            utils::Logger::warn("RedisRateLimiterBackend: getCount bridge threw: " + std::string(ex.what()) + "; using in-memory fallback");
        } catch (...) {
            utils::Logger::warn("RedisRateLimiterBackend: getCount bridge threw unknown exception; using in-memory fallback");
        }
    }
    return redisFallbackBackend().getCount(key, window_seconds);
}

void RedisRateLimiterBackend::reset(const std::string& key)
{
    ResetFn fn;
    {
        std::lock_guard<std::mutex> lk(s_redis_rate_bridge_mutex);
        fn = s_reset_fn;
    }
    if (fn) {
        try {
            fn(key);
            return;
        } catch (const std::exception& ex) {
            utils::Logger::warn("RedisRateLimiterBackend: reset bridge threw: " + std::string(ex.what()) + "; using in-memory fallback");
        } catch (...) {
            utils::Logger::warn("RedisRateLimiterBackend: reset bridge threw unknown exception; using in-memory fallback");
        }
    }
    redisFallbackBackend().reset(key);
}

bool RedisRateLimiterBackend::isConnected() const
{
    IsConnectedFn fn;
    {
        std::lock_guard<std::mutex> lk(s_redis_rate_bridge_mutex);
        fn = s_is_connected_fn;
    }
    if (fn) {
        try {
            return fn();
        } catch (const std::exception& ex) {
            utils::Logger::warn("RedisRateLimiterBackend: isConnected bridge threw: " + std::string(ex.what()));
            return false;
        } catch (...) {
            utils::Logger::warn("RedisRateLimiterBackend: isConnected bridge threw unknown exception");
            return false;
        }
    }
    return false;
}

bool RedisRateLimiterBackend::reconnect()
{
    ReconnectFn fn;
    {
        std::lock_guard<std::mutex> lk(s_redis_rate_bridge_mutex);
        fn = s_reconnect_fn;
    }
    if (fn) {
        try {
            return fn();
        } catch (const std::exception& ex) {
            utils::Logger::warn("RedisRateLimiterBackend: reconnect bridge threw: " + std::string(ex.what()));
            return false;
        } catch (...) {
            utils::Logger::warn("RedisRateLimiterBackend: reconnect bridge threw unknown exception");
            return false;
        }
    }
    return false;
}

#endif // THEMIS_ENABLE_REDIS

} // namespace auth
} // namespace themis
