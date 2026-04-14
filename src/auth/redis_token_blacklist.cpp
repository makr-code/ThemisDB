/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            redis_token_blacklist.cpp                          ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-14 18:46:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   85.0/100                                       ║
    • Total Lines:     246                                            ║
    • Open Issues:     TODOs: 0, Stubs: 3                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 67965456c8  2026-03-22  Add constructors with default config for various classes ... ║
    • 3a23233d60  2026-03-12  fix(auth): address PR review comments on token blacklist ... ║
    • e93c27150c  2026-03-12  feat(auth): implement ITokenBlacklist interface, Bloom fi... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
                    ctx_ ? ctx_->errstr : "allocation failure");
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
                    jti, ctx_->errstr);
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
        THEMIS_WARN("RedisTokenBlacklist::isRevoked: command failed: {}", ctx_->errstr);
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
// No-op stub — compiled when hiredis is not available
// ============================================================================

RedisTokenBlacklist::RedisTokenBlacklist(const Config& config)
    : config_(config)
{
    THEMIS_WARN("RedisTokenBlacklist: built without hiredis (THEMIS_ENABLE_REDIS not "
                "defined).  Token revocations will NOT be persisted to Redis.  "
                "Enable the 'redis' vcpkg feature to activate distributed blacklisting.");
}

RedisTokenBlacklist::~RedisTokenBlacklist() = default;

void RedisTokenBlacklist::add(const std::string& /*jti*/,
                               std::chrono::system_clock::time_point /*expiry*/) {
    // no-op stub
}

bool RedisTokenBlacklist::isRevoked(const std::string& /*jti*/) const {
    return false;  // safe default: no false positives
}

void RedisTokenBlacklist::purgeExpired() {
    // no-op stub
}

bool RedisTokenBlacklist::isConnected() const {
    return false;
}

bool RedisTokenBlacklist::reconnect() {
    return false;
}

#endif // THEMIS_ENABLE_REDIS

} // namespace auth
} // namespace themis
