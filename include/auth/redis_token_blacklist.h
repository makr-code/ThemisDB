/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            redis_token_blacklist.h                            ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-14 11:23:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     138                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 67965456c8  2026-03-22  Add constructors with default config for various classes ... ║
    • 3a23233d60  2026-03-12  fix(auth): address PR review comments on token blacklist ... ║
    • e93c27150c  2026-03-12  feat(auth): implement ITokenBlacklist interface, Bloom fi... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "auth/token_blacklist.h"

#include <string>
#include <mutex>
#include <cstdint>

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
#endif
};

} // namespace auth
} // namespace themis
