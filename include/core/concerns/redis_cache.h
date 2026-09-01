/**
 * @file redis_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
#include <condition_variable>
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
 *
 * The configuration is intentionally fail-safe: invalid connection details
 * should be rejected by the factory rather than converted into a permissive
 * fallback that hides deployment issues.
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
        * The factory validates the URL structure and normalizes the node list
        * before constructing the cache. Authentication, routing, and socket
        * setup remain lazy until the first cache operation or an explicit
        * health check.
        *
        * @param url Redis URL string.
        * @return Constructed RedisCache instance.
     */
    static std::unique_ptr<RedisCache> create(const std::string& url);

    /**
     * @brief Create a RedisCache from an explicit configuration struct.
     *
        * Invalid node addresses, empty node lists, or malformed prefixes are
        * handled by the constructor's internal normalisation logic; the returned
        * cache still prefers fail-closed connection behavior at runtime.
        *
        * @param config Full configuration.
        * @return Constructed RedisCache instance.
     */
    static std::unique_ptr<RedisCache> create(const RedisCacheConfig& config);

    /**
     * @brief Release Redis connections, stop the subscriber thread, and shut
     *        the cache down.
     *
     * Shutdown is idempotent; callers may invoke it multiple times.
     */
    ~RedisCache() override;

    // Non-copyable, non-movable (background thread + sockets).
    RedisCache(const RedisCache&) = delete;
    RedisCache& operator=(const RedisCache&) = delete;
    RedisCache(RedisCache&&) = delete;
    RedisCache& operator=(RedisCache&&) = delete;

    // -----------------------------------------------------------------------
    // ICache interface
    // -----------------------------------------------------------------------

    /**
     * @brief Retrieve a cache entry by key.
     *
     * Redis connection failures and transport errors are reported as cache
     * misses so the caller can fall back to the origin data source without
     * handling exceptions.
     *
     * @param key Cache key before the configured prefix is applied.
     * @return Cached entry when present and readable, otherwise std::nullopt.
     */
    std::optional<CacheEntry> get(std::string_view key) const override;

    /**
     * @brief Insert or replace a cache entry.
     *
     * If Redis is unavailable, the method returns false instead of throwing.
     * When ttl_ms is zero the cache's default TTL is used.
     *
     * @param key Cache key before prefixing.
     * @param entry Value to store.
     * @param ttl_ms Optional per-entry TTL in milliseconds.
     * @return true on success, false when the entry could not be persisted.
     */
    bool put(std::string_view key, const CacheEntry& entry,
             uint64_t ttl_ms = 0) override;

    /**
     * @brief Remove a single entry from the cache.
     *
     * Missing keys and Redis transport errors are both treated as best-effort
     * invalidation requests.
     *
     * @param key Cache key before prefixing.
     */
    void invalidate(std::string_view key) override;

    /**
     * @brief Remove all entries from the cache.
     *
     * The operation is best-effort across the configured Redis nodes and may
     * partially succeed if one node is temporarily unavailable.
     */
    void clear() override;

    /**
     * @brief Remove all entries whose keys match a glob-style pattern.
     *
     * Unsupported or failing nodes are skipped so that a single unhealthy
     * Redis instance does not block invalidation across the rest of the ring.
     *
     * @param pattern Glob-style pattern evaluated after the configured prefix
     *                is applied.
     */
    void invalidatePattern(std::string_view pattern) override;

    /**
     * @brief Return the number of entries visible to the current cache node.
     *
     * For a distributed Redis deployment this is a diagnostic count rather
     * than a cluster-wide strong total.
     */
    size_t size() const override;

    /**
     * @brief Return the cumulative number of cache hits.
     */
    uint64_t hitCount() const override;

    /**
     * @brief Return the cumulative number of cache misses.
     */
    uint64_t missCount() const override;

    /**
     * @brief Return the cache hit rate in the range [0.0, 1.0].
     *
     * Returns 0.0 when no lookups have been attempted yet.
     */
    double hitRate() const override;

    /**
     * @brief Update the maximum number of entries allowed in the cache.
     *
     * A zero value disables the explicit capacity limit; eviction remains
     * governed by Redis and the configured TTL policy.
     *
     * @param maxSize New capacity limit.
     */
    void setMaxSize(size_t maxSize) override;

    /**
     * @brief Update the default TTL applied when callers pass ttl_ms = 0.
     *
     * @param ttl_ms TTL in milliseconds. A zero value disables TTL-based
     *               expiration for entries that rely on the default.
     */
    void setDefaultTTL(uint64_t ttl_ms) override;

    void flush() noexcept override {}

    /**
     * @brief Stop background activity and close all Redis sockets.
     *
     * The method is safe to call repeatedly and should leave the object in a
     * quiescent state even if some nodes are already disconnected.
     */
    void shutdown() noexcept override;

    /**
     * @brief Probe whether the cache backend is reachable and healthy.
     *
     * @return Healthy probe when the primary Redis connection is usable;
     *         a descriptive failure otherwise.
     */
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
      * Callback execution should be non-blocking because it runs on the
      * invalidation delivery path.
     *
     * @param cb  Callback to invoke for each invalidation message.
     */
    void subscribeInvalidations(InvalidationCallback cb);

    /**
      * @brief Return true when the primary Redis connection is established.
      *
      * The result is a point-in-time diagnostic and may change immediately
      * after the call in a multi-threaded deployment.
     */
    bool isConnected() const;

    /**
     * @brief Return the Redis node (host:port) that owns the given key
     *        according to the consistent hash ring.
     *
     * Useful for diagnostics and testing the hash distribution.
     *
    * @param key Cache key (before applying the key_prefix).
    * @return "host:port" string, or an empty string if the ring is not
    *         initialised.
     */
    std::string nodeForKey(std::string_view key) const;

    /**
     * @brief Return the number of virtual ring positions in the consistent
     *        hash ring.
     *
     * This is useful for validating the consistent-hashing distribution in
     * tests and diagnostics.
     */
    size_t hashRingSize() const;

    /**
     * @brief Return the number of physical Redis nodes configured.
     *
     * @return Number of configured nodes after URL/config normalisation.
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
    void ensureSubscriberLoopStarted();

    void subscriberLoop();
    void subscriberSession(SocketFd fd);
    static bool readPubSubMessage(SocketFd fd,
                                  std::string& channel_out,
                                  std::string& payload_out) noexcept;
    void dispatchInvalidation(const std::string& payload);

    std::thread            sub_thread_;
    std::mutex             sub_thread_mutex_;
    mutable std::mutex     sub_sleep_mutex_;
    std::condition_variable sub_sleep_cv_;
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
