/**
 * @file redis_token_blacklist.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=10, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/redis_token_blacklist.h"
#include "utils/logger.h"

#ifdef THEMIS_ENABLE_REDIS
#include <hiredis/hiredis.h>
#endif

#include <stdexcept>
#include <cstring>

namespace themis {
namespace auth {

#ifdef THEMIS_ENABLE_REDIS

// ============================================================================
// Helpers
// ============================================================================

namespace {
const char* redisErrStrSafe(const redisContext* ctx) {
    if (!ctx || !ctx->errstr) {
        return "unknown redis error";
    }
    return ctx->errstr;
}
} // namespace

std::string RedisTokenBlacklist::makeKey(const std::string& jti) const {
    return config_.key_prefix + jti;
}

bool RedisTokenBlacklist::connect() {
    if (ctx_) {
        redisFree(ctx_);
        ctx_ = nullptr;
    }

    struct timeval tv;
    tv.tv_sec  = config_.connect_timeout_ms / 1000;
    tv.tv_usec = (config_.connect_timeout_ms % 1000) * 1000;

    ctx_ = redisConnectWithTimeout(config_.host.c_str(), config_.port, tv);
    if (!ctx_ || ctx_->err) {
        THEMIS_WARN("RedisTokenBlacklist: connect to {}:{} failed: {}",
                    config_.host, config_.port,
                    ctx_ ? redisErrStrSafe(ctx_) : "allocation failure");
        if (ctx_) { redisFree(ctx_); ctx_ = nullptr; }
        return false;
    }

    if (!config_.auth.empty()) {
        redisReply* reply = static_cast<redisReply*>(
            redisCommand(ctx_, "AUTH %s", config_.auth.c_str()));
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            THEMIS_WARN("RedisTokenBlacklist: AUTH failed: {}",
                        reply ? reply->str : "no reply");
            if (reply) freeReplyObject(reply);
            redisFree(ctx_);
            ctx_ = nullptr;
            return false;
        }
        freeReplyObject(reply);
    }

    THEMIS_INFO("RedisTokenBlacklist: connected to {}:{}", config_.host, config_.port);
    return true;
}

void RedisTokenBlacklist::disconnect() {
    if (ctx_) {
        redisFree(ctx_);
        ctx_ = nullptr;
    }
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

RedisTokenBlacklist::RedisTokenBlacklist()
    : RedisTokenBlacklist(Config{})
{}

RedisTokenBlacklist::RedisTokenBlacklist(const Config& config)
    : config_(config)
{
    connect();
}

RedisTokenBlacklist::~RedisTokenBlacklist() {
    std::lock_guard<std::mutex> lock(mutex_);
    disconnect();
}

// ============================================================================
// ITokenBlacklist interface
// ============================================================================

void RedisTokenBlacklist::add(const std::string& jti,
                               std::chrono::system_clock::time_point expiry) {
    if (jti.empty()) return;

    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) {
        THEMIS_WARN("RedisTokenBlacklist::add: not connected – skipping JTI '{}'", jti);
        return;
    }

    auto now   = std::chrono::system_clock::now();
    auto delta = std::chrono::duration_cast<std::chrono::seconds>(expiry - now).count();

    // If the token is already expired (or expires now), do not record it in Redis.
    // This keeps behavior consistent with other backends, where expired tokens
    // are not considered revoked.
    if (delta <= 0) {
        THEMIS_DEBUG("RedisTokenBlacklist::add: expiry <= now for JTI '{}', skipping", jti);
        return;
    }

    long long ttl = static_cast<long long>(delta);

    const std::string key = makeKey(jti);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "SET %s 1 EX %lld NX", key.c_str(), ttl));

    if (!reply) {
        THEMIS_WARN("RedisTokenBlacklist::add: command failed for JTI '{}': {}",
                    jti, redisErrStrSafe(ctx_));
        disconnect();
        return;
    }

    if (reply->type == REDIS_REPLY_STATUS &&
        reply->str != nullptr &&
        std::strcmp(reply->str, "OK") == 0) {
        THEMIS_DEBUG("RedisTokenBlacklist: revoked JTI '{}' with TTL {}s", jti, ttl);
    } else if (reply->type == REDIS_REPLY_NIL) {
        // Key already exists; NX caused no-op — token was already blacklisted.
        THEMIS_DEBUG("RedisTokenBlacklist::add: JTI '{}' already blacklisted – no-op", jti);
    } else if (reply->type == REDIS_REPLY_ERROR) {
        THEMIS_WARN("RedisTokenBlacklist::add: Redis returned error for JTI '{}': {}",
                    jti, (reply->str ? reply->str : "unknown error"));
        freeReplyObject(reply);
        disconnect();
        return;
    } else {
        THEMIS_WARN("RedisTokenBlacklist::add: unexpected reply type {} for JTI '{}'",
                    reply->type, jti);
    }

    freeReplyObject(reply);
}

bool RedisTokenBlacklist::isRevoked(const std::string& jti) const {
    if (jti.empty()) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    if (!ctx_) return false;

    const std::string key = makeKey(jti);
    redisReply* reply = static_cast<redisReply*>(
        redisCommand(ctx_, "EXISTS %s", key.c_str()));

    if (!reply) {
        THEMIS_WARN("RedisTokenBlacklist::isRevoked: command failed: {}", redisErrStrSafe(ctx_));
        // Cast away const to reset connection state on error
        const_cast<RedisTokenBlacklist*>(this)->disconnect();
        return false;
    }

    bool revoked = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
    freeReplyObject(reply);
    return revoked;
}

void RedisTokenBlacklist::purgeExpired() {
    // Redis TTL handles expiry automatically; nothing to do.
}

bool RedisTokenBlacklist::isConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return ctx_ != nullptr && ctx_->err == 0;
}

bool RedisTokenBlacklist::reconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    return connect();
}

#else // !THEMIS_ENABLE_REDIS

// ============================================================================
// In-memory fallback — compiled when hiredis is not available
//
// Token revocations are honoured within the lifetime of a single process.
// They are NOT propagated to other nodes or persisted across restarts.
// Multi-node deployments must enable THEMIS_ENABLE_REDIS for distributed
// blacklisting.
// ============================================================================

RedisTokenBlacklist::RedisTokenBlacklist(const Config& config)
    : config_(config)
{
    THEMIS_WARN("RedisTokenBlacklist: built without hiredis (THEMIS_ENABLE_REDIS not "
                "defined).  Token revocations are stored in-process only and will "
                "NOT propagate to other nodes or survive process restart.  "
                "Enable the 'redis' vcpkg feature to activate distributed blacklisting.");
}

RedisTokenBlacklist::RedisTokenBlacklist()
    : RedisTokenBlacklist(Config{})
{}

RedisTokenBlacklist::~RedisTokenBlacklist() = default;

void RedisTokenBlacklist::add(const std::string& jti,
                               std::chrono::system_clock::time_point expiry) {
    std::lock_guard<std::mutex> lock(fallback_mutex_);
    fallback_map_[jti] = expiry;
}

bool RedisTokenBlacklist::isRevoked(const std::string& jti) const {
    std::lock_guard<std::mutex> lock(fallback_mutex_);
    auto it = fallback_map_.find(jti);
    if (it == fallback_map_.end()) {
        return false;
    }
    // Honour natural expiry — expired entries are not considered revoked.
    // Cleanup of expired entries is deferred to purgeExpired() to keep
    // isRevoked() semantically read-only (no map mutation).
    return std::chrono::system_clock::now() < it->second;
}

void RedisTokenBlacklist::purgeExpired() {
    const auto now = std::chrono::system_clock::now();
    std::lock_guard<std::mutex> lock(fallback_mutex_);
    for (auto it = fallback_map_.begin(); it != fallback_map_.end(); ) {
        if (now >= it->second) {
            it = fallback_map_.erase(it);
        } else {
            ++it;
        }
    }
}

bool RedisTokenBlacklist::isConnected() const {
    return false;  // no Redis connection in this build
}

bool RedisTokenBlacklist::reconnect() {
    return false;  // no Redis connection in this build
}

#endif // THEMIS_ENABLE_REDIS

} // namespace auth
} // namespace themis
