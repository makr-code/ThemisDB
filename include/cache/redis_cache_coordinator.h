/**
 * @file redis_cache_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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
        /// Redis server hostname or IP address.
        std::string host = "127.0.0.1";

        /// Redis server port.
        int port = 6379;

        /// Optional password for AUTH command.  Empty = no authentication.
        std::string auth;

        /// Prefix for the pub/sub channel name.
        /// Actual channel: "{channel_prefix}:replication"
        std::string channel_prefix = "themis_cache";

        /// Connection timeout in milliseconds.
        int connect_timeout_ms = 5000;

        /// Interval between reconnection attempts when the subscribe
        /// connection is down.
        int reconnect_interval_ms = 1000;

        /// Number of publish connections in the pool.
        /// Currently only 1 is used (mutex-protected); reserved for future
        /// async pool expansion.
        int pool_size = 4;

        /// Node identifier appended to published messages so that the
        /// originating node can skip its own echo.  Defaults to
        /// host:port if left empty.
        std::string node_id;

        /// Optional HMAC-SHA256 secret for message signing/verification.
        ///
        /// When non-empty, every published message is signed with
        /// HMAC-SHA256(hmac_secret, payload) and the resulting hex digest is
        /// included in the "sig" field.  Received messages whose "sig" field
        /// is absent or does not match are silently discarded.
        /// When empty, signing and verification are disabled (default).
        std::string hmac_secret;
    };

    // -----------------------------------------------------------------------
    // Construction / destruction
    // -----------------------------------------------------------------------

    /**
     * @brief Construct the coordinator and start the background subscribe thread.
     *
     * Connections are established lazily on first use; construction is
     * non-blocking and never throws even if Redis is unavailable.
     *
     * @param config  Coordinator configuration.
     */
    explicit RedisCacheCoordinator(const Config& config);

    /**
     * @brief Stop the background thread and release Redis connections.
     */
    ~RedisCacheCoordinator() override;

    // Non-copyable
    RedisCacheCoordinator(const RedisCacheCoordinator&) = delete;
    RedisCacheCoordinator& operator=(const RedisCacheCoordinator&) = delete;

    // -----------------------------------------------------------------------
    // ICacheCoordinator interface
    // -----------------------------------------------------------------------

    /**
     * @brief Publish a new cache entry so peer nodes can pre-populate their L1/L2.
     *
     * Serialises the entry as JSON and PUBLISHes it to the replication channel.
     * On Redis error, logs a warning and returns without throwing.
     */
    void publishEntry(const std::string& key,
                      const nlohmann::json& result,
                      int ttl_seconds,
                      const std::string& tenant_id) override;

    /**
     * @brief Publish an invalidation event so peer nodes evict matching entries.
     *
     * On Redis error, logs a warning and returns without throwing.
     */
    void publishInvalidation(const std::string& pattern,
                             const std::string& tenant_id = "") override;

    /**
     * @brief Register a callback invoked for each ENTRY_PUT message received
     *        from a peer node.
     */
    void subscribeEntries(EntryCallback callback) override;

    /**
     * @brief Register a callback invoked for each INVALIDATE message received
     *        from a peer node.
     */
    void subscribeInvalidations(InvalidationCallback callback) override;

    /**
     * @return true if the publish connection to Redis is active.
     */
    bool isConnected() const override;

    /**
     * @return Human-readable transport description including host:port.
     */
    std::string name() const override;

    /**
     * @brief JSON statistics snapshot.
     *
     * Fields:
     *   - messages_published (uint64): total PUBLISH calls that succeeded
     *   - messages_received  (uint64): total messages delivered to callbacks
     *   - publish_errors     (uint64): failed PUBLISH calls (Redis errors)
     *   - reconnect_count    (uint64): number of reconnect attempts
     *   - connected          (bool)
     *   - channel            (string)
     *   - node_id            (string)
     */
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

    /// Establish (or re-establish) the publish connection.  Called under pub_mutex_.
    bool connectPublish();

    /// Establish the subscribe connection and issue SUBSCRIBE command.
    bool connectSubscribe();

    /// Background subscribe loop – runs in sub_thread_.
    void subscribeLoop();

    /// Dispatch an incoming message to the appropriate callback.
    void handleMessage(const std::string& payload);

    /// Serialise a ReplicationMessage to a JSON string for PUBLISH.
    std::string serializeMessage(const ReplicationMessage& msg) const;

    /// Deserialise a JSON string back to a ReplicationMessage.
    /// Returns nullopt on parse failure.
    std::optional<ReplicationMessage> deserializeMessage(const std::string& data) const;

    /// Compute HMAC-SHA256(config_.hmac_secret, payload) and return hex string.
    /// Returns empty string when hmac_secret is empty.
    std::string computeHmac(const std::string& payload) const;

    /// Verify that the "sig" field in the JSON object matches the expected HMAC.
    /// Returns true when hmac_secret is empty (signing disabled) or when the
    /// signature matches.  Returns false on mismatch or when signing is enabled
    /// but the field is absent.
    bool verifyHmac(const nlohmann::json& j) const;

  public:
    // -----------------------------------------------------------------------
    // Injectable publish bridge (STUB #42)
    // -----------------------------------------------------------------------
    /// Callback type: given a channel name and a JSON payload string, publish
    /// the message and return true on success.  Used as a drop-in transport
    /// when THEMIS_ENABLE_REDIS is not defined (hiredis absent).
    using RedisPublishFn = std::function<bool(const std::string& channel,
                                              const std::string& payload)>;

    /// Register a publish function used by `publishEntry()` and
    /// `publishInvalidation()` in non-hiredis builds.
    /// Pass an empty `std::function` to clear and revert to the no-op fallback.
    /// Thread-safe (guarded by a static mutex).
    static void setRedisPublishFn(RedisPublishFn fn);
};

} // namespace cache
} // namespace themis
