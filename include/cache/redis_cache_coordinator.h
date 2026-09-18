/**
 * @file redis_cache_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include "cache/cache_replication_coordinator.h"
#include <functional>
#include <optional>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <nlohmann/json.hpp>

// ============================================================================
// RedisCacheCoordinator
//
// Redis pub/sub backed implementation of ICacheCoordinator for distributed
// cache coordination across ThemisDB nodes.
//
// Architecture:
//   - Two Redis connections per coordinator instance:
//       1. Publish connection  – used exclusively for PUBLISH commands
//          (mutex-protected, supports concurrent callers).
//       2. Subscribe connection – runs on a dedicated background thread;
//          delivers incoming peer messages via registered callbacks.
//   - Single logical channel: {channel_prefix}:replication
//     All ENTRY_PUT and INVALIDATE messages are JSON-serialised and sent on
//     this channel; the receiver discriminates by the "type" field.
//   - Graceful degradation: any Redis error (connection loss, timeout) is
//     caught, logged as a warning, and the local cache operation completes
//     regardless of coordinator state.  The background subscribe thread
//     attempts reconnection at `reconnect_interval_ms` intervals.
//
// Usage:
//   RedisCacheCoordinator::Config cfg;
//   cfg.host   = "redis-cluster.internal";
//   cfg.port   = 6379;
//   cfg.auth   = "s3cr3t";
//   auto coord = std::make_shared<RedisCacheCoordinator>(cfg);
//   cache.setCoordinator(coord);
//
// Thread-safety: all public methods are thread-safe.
// ============================================================================

#ifdef THEMIS_ENABLE_REDIS
#include <hiredis/hiredis.h>
#endif

namespace themis {
namespace cache {

class RedisCacheCoordinator final : public ICacheCoordinator {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------
    struct Config {
        std::string host = "127.0.0.1";

        int port = 6379;

        std::string auth;

        std::string channel_prefix = "themis_cache";

        int connect_timeout_ms = 5000;

        int reconnect_interval_ms = 1000;

        int pool_size = 4;

        std::string node_id;

        std::string hmac_secret;
    };

    // -----------------------------------------------------------------------
    // Construction / destruction
    // -----------------------------------------------------------------------

    /**
     * @brief Redis Cache Coordinator.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit RedisCacheCoordinator(const Config& config);

    ~RedisCacheCoordinator() override;

    // Non-copyable
    RedisCacheCoordinator(const RedisCacheCoordinator&) = delete;
    RedisCacheCoordinator& operator=(const RedisCacheCoordinator&) = delete;

    // -----------------------------------------------------------------------
    // ICacheCoordinator interface
    // -----------------------------------------------------------------------

    void publishEntry(const std::string& key,
                      const nlohmann::json& result,
                      int ttl_seconds,
                      const std::string& tenant_id) override;

    void publishInvalidation(const std::string& pattern,
                             const std::string& tenant_id = "") override;

    void subscribeEntries(EntryCallback callback) override;

    void subscribeInvalidations(InvalidationCallback callback) override;

    bool isConnected() const override;

    std::string name() const override;

    nlohmann::json getStats() const override;

private:
    Config      config_;
    std::string channel_;   ///< Full channel name: {prefix}:replication
    std::string node_id_;   ///< Effective node id (host:port or config value)

    // ------------------------------------------------------------------
    // Publish connection (mutex-protected synchronous connection)
    // ------------------------------------------------------------------
#ifdef THEMIS_ENABLE_REDIS
    redisContext*      pub_ctx_   = nullptr;
#endif
    mutable std::mutex pub_mutex_;
    std::atomic<bool>  pub_connected_{false};

    // ------------------------------------------------------------------
    // Subscribe connection (background thread)
    // ------------------------------------------------------------------
#ifdef THEMIS_ENABLE_REDIS
    redisContext*      sub_ctx_   = nullptr;
#endif
    std::thread        sub_thread_;
    std::atomic<bool>  running_{false};

    // ------------------------------------------------------------------
    // Registered callbacks
    // ------------------------------------------------------------------
    EntryCallback        entry_cb_;
    InvalidationCallback invalidation_cb_;
    mutable std::mutex   cb_mutex_;

    // ------------------------------------------------------------------
    // Metrics
    // ------------------------------------------------------------------
    mutable std::mutex stats_mutex_;
    uint64_t messages_published_ = 0;
    uint64_t messages_received_  = 0;
    uint64_t publish_errors_     = 0;
    uint64_t reconnect_count_    = 0;

    // ------------------------------------------------------------------
    // Internal helpers
    // ------------------------------------------------------------------

    /**
     * @brief Connect Publish.
     * @return True when the operation succeeds.
     */
    bool connectPublish();

    /**
     * @brief Connect Subscribe.
     * @return True when the operation succeeds.
     */
    bool connectSubscribe();

    /**
     * @brief Subscribe Loop.
     */
    void subscribeLoop();

    /**
     * @brief Handle Message.
     * @param[in] payload Input parameter.
     */
    void handleMessage(const std::string& payload);

    /**
     * @brief Serialize Message.
     * @param[in] msg Input parameter.
     * @return Return value.
     */
    std::string serializeMessage(const ReplicationMessage& msg) const;

    /**
     * @brief Deserialize Message.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    std::optional<ReplicationMessage> deserializeMessage(const std::string& data) const;

    /**
     * @brief Compute Hmac.
     * @param[in] payload Input parameter.
     * @return Return value.
     */
    std::string computeHmac(const std::string& payload) const;

    /**
     * @brief Verify Hmac.
     * @param[in] j Input parameter.
     * @return True when the operation succeeds.
     */
    bool verifyHmac(const nlohmann::json& j) const;

  public:
    // -----------------------------------------------------------------------
    // Injectable publish bridge (STUB #42)
    // -----------------------------------------------------------------------
    using RedisPublishFn = std::function<bool(const std::string& channel,
                                              const std::string& payload)>;

    /**
     * @brief Set Redis Publish Fn.
     * @param[in] fn Input parameter.
     */
    static void setRedisPublishFn(RedisPublishFn fn);
};

} // namespace cache
} // namespace themis
