/**
 * @file redis_token_blacklist.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class RedisTokenBlacklist final : public ITokenBlacklist {
public:
    struct Config {
        std::string host = "127.0.0.1";
        int port = 6379;
        std::string auth;
        std::string key_prefix = "themis:jbl:";
        int connect_timeout_ms = 200;
    };

    RedisTokenBlacklist();
    /**
     * @brief Redis Token Blacklist.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit RedisTokenBlacklist(const Config& config);
    ~RedisTokenBlacklist() override;

    RedisTokenBlacklist(const RedisTokenBlacklist&) = delete;
    RedisTokenBlacklist& operator=(const RedisTokenBlacklist&) = delete;

    // -----------------------------------------------------------------------
    // ITokenBlacklist interface
    // -----------------------------------------------------------------------

    void add(const std::string& jti,
             std::chrono::system_clock::time_point expiry) override;

    bool isRevoked(const std::string& jti) const override;

    void purgeExpired() override;

    // -----------------------------------------------------------------------
    // Connectivity
    // -----------------------------------------------------------------------

    /**
     * @brief Is Connected.
     * @return True when the operation succeeds.
     */
    bool isConnected() const;

    /**
     * @brief Reconnect.
     * @return True when the operation succeeds.
     */
    bool reconnect();

private:
    Config config_;

#ifdef THEMIS_ENABLE_REDIS
    mutable std::mutex  mutex_;
    redisContext*       ctx_{nullptr};

    /**
     * @brief Connect.
     * @return True when the operation succeeds.
     */
    bool connect();
    /**
     * @brief Disconnect.
     */
    void disconnect();
    /**
     * @brief Make Key.
     * @param[in] jti Input parameter.
     * @return Return value.
     */
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

