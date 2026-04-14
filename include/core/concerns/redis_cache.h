/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            redis_cache.h                                      ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-14 11:24:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     375                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9c5d3c282d  2026-03-13  fix(core): fix deadlock in RedisCache::invalidatePattern ... ║
    • e1c78c3604  2026-03-13  feat(core): implement RedisCache distributed cache adapte... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

/**
 * @file redis_cache.h
 * @brief Redis-backed ICache implementation with consistent hashing and
 *        pub/sub invalidation for distributed caching across cluster nodes.
 *
 * Implements the Distributed Cache Integration roadmap item (v1.6.0).
 *
 * Features:
 *  - Cluster-wide cache invalidation via Redis pub/sub PUBLISH
 *  - Consistent hashing (hash ring with virtual nodes) for key routing
 *  - TTL support via Redis PSETEX (millisecond precision)
 *  - Pub/sub for cache invalidation messages (background subscriber thread)
 *  - Graceful degradation: cache operations never throw when Redis is down
 *  - Thread-safe: all public methods are safe to call concurrently
 *
 * Use Cases:
 *  - Query result caching across nodes
 *  - Session state management
 *  - Distributed rate limiting state
 *
 * Usage:
 * @code
 *   auto redis_cache = RedisCache::create("redis://cluster:6379");
 *   auto context = ConcernsContext::createCustom(
 *       logger, tracer, metrics, std::move(redis_cache)
 *   );
 * @endcode
 */

#include "core/concerns/i_cache.h"

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

namespace themis {
namespace core {
namespace concerns {

// ---------------------------------------------------------------------------
// RedisCacheConfig
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for the Redis-backed distributed cache.
 */
struct RedisCacheConfig {
    /// Redis node addresses as "host:port" strings.
    /// Multiple nodes enable consistent-hash key routing.
    /// Defaults to a single local Redis instance.
    std::vector<std::string> nodes = {"127.0.0.1:6379"};

    /// Optional AUTH password (empty = no authentication).
    std::string password;

    /// Redis database index (0–15).
    int db_index = 0;

    /// TCP connect timeout in milliseconds.
    int connect_timeout_ms = 2000;

    /// Interval between reconnect attempts when connection is lost (ms).
    int reconnect_interval_ms = 5000;

    /// Key prefix applied to every Redis key to avoid collisions with other
    /// tenants or applications sharing the same Redis instance.
    std::string key_prefix = "themis:";

    /// Number of virtual nodes per physical node in the consistent hash ring.
    /// Higher values give more uniform key distribution.
    int virtual_nodes_per_node = 150;

    /// Pub/sub channel name for cluster-wide cache invalidation messages.
    std::string invalidation_channel = "themis:cache:invalidations";

    /// Default TTL in milliseconds (0 = no TTL, entries persist until evicted
    /// or the Redis instance runs out of memory).
    uint64_t default_ttl_ms = 0;

    /// Maximum number of entries (0 = unlimited; enforced only on SET, the
    /// Redis server does not count entries for the caller).
    size_t max_size = 0;
};

// ---------------------------------------------------------------------------
// RedisCache
// ---------------------------------------------------------------------------

/**
 * @brief Redis-backed ICache implementation for distributed caching.
 *
 * All data is stored in Redis so that every cluster node shares the same
 * cache state.  Keys are routed to specific Redis nodes using a consistent
 * hash ring, minimising remapping when nodes are added or removed.
 *
 * A background subscriber thread listens on the invalidation pub/sub channel.
 * When any node calls invalidate() or clear(), it publishes a message so that
 * all other nodes can take action (e.g. expire local L1 caches).
 *
 * Graceful degradation: if the Redis connection fails, get() returns
 * std::nullopt and put() returns false; no exception is thrown.  A background
 * reconnect loop re-establishes connections at configurable intervals.
 */
class RedisCache final : public ICache {
public:
    // -----------------------------------------------------------------------
    // Factory methods
    // -----------------------------------------------------------------------

    /**
     * @brief Create a RedisCache from a Redis URL.
     *
     * Supported URL formats:
     *   - redis://host:port
     *   - redis://host:port,host2:port2   (multi-node)
     *   - redis://:password@host:port
     *
     * @param url  Redis URL string.
     * @return     Constructed RedisCache instance.
     */
    static std::unique_ptr<RedisCache> create(const std::string& url);

    /**
     * @brief Create a RedisCache from an explicit configuration struct.
     *
     * @param config  Full configuration.
     * @return        Constructed RedisCache instance.
     */
    static std::unique_ptr<RedisCache> create(const RedisCacheConfig& config);

    ~RedisCache() override;

    // Non-copyable, non-movable (background thread + sockets).
    RedisCache(const RedisCache&) = delete;
    RedisCache& operator=(const RedisCache&) = delete;
    RedisCache(RedisCache&&) = delete;
    RedisCache& operator=(RedisCache&&) = delete;

    // -----------------------------------------------------------------------
    // ICache interface
    // -----------------------------------------------------------------------

    std::optional<CacheEntry> get(std::string_view key) const override;

    bool put(std::string_view key, const CacheEntry& entry,
             uint64_t ttl_ms = 0) override;

    void invalidate(std::string_view key) override;

    void clear() override;

    void invalidatePattern(std::string_view pattern) override;

    size_t size() const override;

    uint64_t hitCount() const override;

    uint64_t missCount() const override;

    double hitRate() const override;

    void setMaxSize(size_t maxSize) override;

    void setDefaultTTL(uint64_t ttl_ms) override;

    void flush() noexcept override {}

    void shutdown() noexcept override;

    ProbeResult isHealthy() const override;

    // -----------------------------------------------------------------------
    // Distributed-cache-specific extensions
    // -----------------------------------------------------------------------

    /**
     * @brief Callback invoked when a cluster-wide invalidation message is
     *        received on the pub/sub channel.
     *
     * The argument is the invalidated key or glob pattern.
     */
    using InvalidationCallback = std::function<void(const std::string& key_or_pattern)>;

    /**
     * @brief Register a callback for incoming invalidation pub/sub messages.
     *
     * The callback is invoked from the background subscriber thread.
     * Thread-safe; may be called before or after the subscriber connects.
     *
     * @param cb  Callback to invoke for each invalidation message.
     */
    void subscribeInvalidations(InvalidationCallback cb);

    /**
     * @brief Return true when the primary Redis connection is established.
     */
    bool isConnected() const;

    /**
     * @brief Return the Redis node (host:port) that owns the given key
     *        according to the consistent hash ring.
     *
     * Useful for diagnostics and testing the hash distribution.
     *
     * @param key  Cache key (before applying the key_prefix).
     * @return     "host:port" string.
     */
    std::string nodeForKey(std::string_view key) const;

    /**
     * @brief Return the number of virtual ring positions in the consistent
     *        hash ring.
     */
    size_t hashRingSize() const;

    /**
     * @brief Return the number of physical Redis nodes configured.
     */
    size_t nodeCount() const { return config_.nodes.size(); }

private:
    // Private constructor – use factory methods.
    explicit RedisCache(const RedisCacheConfig& config);

    // -----------------------------------------------------------------------
    // Platform socket types
    // -----------------------------------------------------------------------

#if defined(_WIN32)
    using SocketFd = uintptr_t;
    static constexpr SocketFd kInvalidSocket = static_cast<uintptr_t>(~0ULL);
#else
    using SocketFd = int;
    static constexpr SocketFd kInvalidSocket = -1;
#endif

    // -----------------------------------------------------------------------
    // Consistent hash ring
    // -----------------------------------------------------------------------

    /// 32-bit FNV-1a hash of an arbitrary byte string.
    static uint32_t fnv1a32(const char* data, size_t len) noexcept;

    /// Build the hash ring from config_.nodes.
    void buildHashRing();

    /// Return the node index (into config_.nodes) that owns the given key.
    size_t nodeIndexForKey(std::string_view key) const;

    /// Sorted map: ring position → node index.
    std::map<uint32_t, size_t> hash_ring_;

    // -----------------------------------------------------------------------
    // TCP / RESP helpers (per-node command connections)
    // -----------------------------------------------------------------------

    /// Node connection state (one per physical Redis node).
    struct NodeConn {
        std::string host;
        uint16_t    port = 6379;
        mutable std::mutex  mutex;
        mutable SocketFd    fd       = kInvalidSocket;
        mutable bool        ok       = false;
    };

    std::vector<std::unique_ptr<NodeConn>> nodes_;

    SocketFd tcpConnect(const std::string& host, uint16_t port) const;
    static void closeSocket(SocketFd& fd) noexcept;
    static bool sendAll(SocketFd fd, const std::string& buf) noexcept;

    /// Read a single CRLF-terminated line from the socket.
    static bool readLine(SocketFd fd, std::string& out) noexcept;

    /// Perform Redis AUTH + SELECT handshake.
    bool redisHandshake(SocketFd fd) const noexcept;

    /// Build a RESP array command.
    static std::string buildRespCommand(const std::vector<std::string>& args);

    /// Ensure the connection for the given node is open; reconnect if needed.
    bool ensureConnected(NodeConn& nc) const noexcept;

    /// Send a RESP command and read the reply.  Returns reply string on
    /// success, empty optional on error.
    std::optional<std::string> sendCommand(NodeConn& nc,
                                           const std::vector<std::string>& args) const noexcept;

    /// Same as sendCommand() but MUST be called with nc.mutex already held.
    /// Used inside invalidatePattern() which holds the lock while iterating.
    std::optional<std::string> sendCommandLocked(NodeConn& nc,
                                                 const std::vector<std::string>& args) const noexcept;

    /// Read a full RESP reply from fd and return the payload string.
    static bool readReply(SocketFd fd, std::string& out) noexcept;

    // -----------------------------------------------------------------------
    // Serialization of CacheEntry
    // -----------------------------------------------------------------------

    /// Encode a CacheEntry into the Redis value bytes.
    /// Format: "<version>\n<timestamp_ms>\n<payload>"
    static std::string encodeEntry(const CacheEntry& e);

    /// Decode Redis value bytes back into a CacheEntry.
    /// Returns nullopt if the bytes are malformed.
    static std::optional<CacheEntry> decodeEntry(const std::string& raw);

    // -----------------------------------------------------------------------
    // Pub/sub subscriber (background thread)
    // -----------------------------------------------------------------------

    /// Publish a cluster-wide invalidation message for key_or_pattern.
    void publishInvalidation(const std::string& key_or_pattern);

    void subscriberLoop();
    void subscriberSession(SocketFd fd);
    static bool readPubSubMessage(SocketFd fd,
                                  std::string& channel_out,
                                  std::string& payload_out) noexcept;
    void dispatchInvalidation(const std::string& payload);

    std::thread            sub_thread_;
    std::atomic<bool>      stop_{false};
    std::atomic<bool>      sub_connected_{false};

    mutable std::mutex     inv_cb_mutex_;
    InvalidationCallback   inv_callback_;

    // -----------------------------------------------------------------------
    // Config and statistics
    // -----------------------------------------------------------------------

    RedisCacheConfig       config_;
    mutable std::atomic<uint64_t> hits_{0};
    mutable std::atomic<uint64_t> misses_{0};
    std::atomic<size_t>   max_size_{0};
    std::atomic<uint64_t> default_ttl_ms_{0};
};

} // namespace concerns
} // namespace core
} // namespace themis
