/**
 * @file redis_token_blacklist.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "auth/token_blacklist.h"

#include <string>
#include <mutex>
#include <cstdint>
#include <unordered_map>
#include <chrono>

// Full hiredis types are only needed in the implementation; forward-declare
// here so the header compiles regardless of THEMIS_ENABLE_REDIS.
struct redisContext;

namespace themis {
namespace auth {

/**
 * @brief Redis-backed token blacklist for distributed deployments.
 *
 * Stores each revoked JTI as a Redis key with a TTL matching the token's
 * remaining lifetime:
 *   SET <prefix><jti> 1 EX <ttl_seconds> NX
 *
 * isRevoked() performs a single EXISTS command which is O(1) server-side.
 * purgeExpired() is a no-op because Redis handles expiry via TTL automatically.
 *
 * When built without hiredis (THEMIS_ENABLE_REDIS not defined), all methods
 * compile and link but log a warning; isRevoked() always returns false so the
 * system degrades gracefully without crashing.
 *
 * Thread-safety: all public methods are thread-safe.
 *
 * Performance target: isRevoked() ≤ 2 ms P99 on a local-network Redis.
 */
class RedisTokenBlacklist final : public ITokenBlacklist {
public:
    struct Config {
        /// Redis server hostname or IP address.
        std::string host = "127.0.0.1";
        /// Redis server port.
        int port = 6379;
        /// Optional AUTH password (empty = no authentication).
        std::string auth;
        /// Key prefix for all blacklist entries in Redis.
        std::string key_prefix = "themis:jbl:";
        /// Connection timeout in milliseconds.
        int connect_timeout_ms = 200;
    };

    /**
     * @brief Construct and connect to a Redis instance.
     *
     * The connection is established eagerly; if it fails a warning is logged
     * and the instance operates as a no-op stub until reconnected.
     */
    RedisTokenBlacklist();
    explicit RedisTokenBlacklist(const Config& config);
    ~RedisTokenBlacklist() override;

    RedisTokenBlacklist(const RedisTokenBlacklist&) = delete;
    RedisTokenBlacklist& operator=(const RedisTokenBlacklist&) = delete;

    // -----------------------------------------------------------------------
    // ITokenBlacklist interface
    // -----------------------------------------------------------------------

    /**
     * @brief Revoke a token in Redis: SET <key> 1 EX <ttl> NX
     *
     * If the token has already expired (expiry ≤ now) a TTL of
     * Config::min_ttl_seconds is used to handle clock skew gracefully.
     */
    void add(const std::string& jti,
             std::chrono::system_clock::time_point expiry) override;

    /**
     * @brief Check revocation via Redis EXISTS.
     *
     * @return true if the Redis key exists (not expired), false otherwise.
     */
    bool isRevoked(const std::string& jti) const override;

    /**
     * @brief No-op: Redis TTL handles expiry automatically.
     */
    void purgeExpired() override;

    // -----------------------------------------------------------------------
    // Connectivity
    // -----------------------------------------------------------------------

    /** @return true if the Redis connection is currently alive. */
    bool isConnected() const;

    /**
     * @brief Attempt to (re)connect to Redis.
     * @return true on success.
     */
    bool reconnect();

private:
    Config config_;

#ifdef THEMIS_ENABLE_REDIS
    mutable std::mutex  mutex_;
    redisContext*       ctx_{nullptr};

    bool connect();
    void disconnect();
    std::string makeKey(const std::string& jti) const;
#else
    // In-memory fallback: revocations are not shared across processes but
    // are honoured within the lifetime of this process.
    mutable std::mutex fallback_mutex_;
    mutable std::unordered_map<std::string,
                               std::chrono::system_clock::time_point> fallback_map_;
#endif
};

} // namespace auth
} // namespace themis

