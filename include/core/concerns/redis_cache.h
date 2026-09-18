/**
 * @file redis_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
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

struct RedisCacheConfig {
    std::vector<std::string> nodes = {"127.0.0.1:6379"};

    std::string password;

    int db_index = 0;

    int connect_timeout_ms = 2000;

    int reconnect_interval_ms = 5000;

    std::string key_prefix = "themis:";

    int virtual_nodes_per_node = 150;

    std::string invalidation_channel = "themis:cache:invalidations";

    uint64_t default_ttl_ms = 0;

    size_t max_size = 0;
};

// ---------------------------------------------------------------------------
// RedisCache
// ---------------------------------------------------------------------------

class RedisCache final : public ICache {
public:
    // -----------------------------------------------------------------------
    // Factory methods
    // -----------------------------------------------------------------------

    /**
     * @brief Create.
     * @param[in] url Input parameter.
     * @return Return value.
     */
    static std::unique_ptr<RedisCache> create(const std::string& url);

    /**
     * @brief Create.
     * @param[in] config Input parameter.
     * @return Return value.
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

    using InvalidationCallback = std::function<void(const std::string& key_or_pattern)>;

    /**
     * @brief Subscribe Invalidations.
     * @param[in] cb Input parameter.
     */
    void subscribeInvalidations(InvalidationCallback cb);

    /**
     * @brief Is Connected.
     * @return True when the operation succeeds.
     */
    bool isConnected() const;

    /**
     * @brief Node For Key.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    std::string nodeForKey(std::string_view key) const;

    /**
     * @brief Hash Ring Size.
     * @return Return value.
     */
    size_t hashRingSize() const;

    size_t nodeCount() const { return config_.nodes.size(); }

private:
    /**
     * @brief Private constructor – use factory methods.
     * @param[in] config Input parameter.
     * @return Return value.
     */
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

    /**
     * @brief Fnv1a32.
     * @param[in] data Input parameter.
     * @param[in] len Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static uint32_t fnv1a32(const char* data, size_t len) noexcept;

    /**
     * @brief Build Hash Ring.
     */
    void buildHashRing();

    /**
     * @brief Node Index For Key.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    size_t nodeIndexForKey(std::string_view key) const;

    std::map<uint32_t, size_t> hash_ring_;

    // -----------------------------------------------------------------------
    // TCP / RESP helpers (per-node command connections)
    // -----------------------------------------------------------------------

    struct NodeConn {
        std::string host = {};
        uint16_t    port = 6379;
        mutable std::mutex  mutex;
        mutable SocketFd    fd       = kInvalidSocket;
        mutable bool        ok       = false;
    };

    std::vector<std::unique_ptr<NodeConn>> nodes_;

    /**
     * @brief Tcp Connect.
     * @param[in] host Input parameter.
     * @param[in] port Input parameter.
     * @return Return value.
     */
    SocketFd tcpConnect(const std::string& host, uint16_t port) const;
    /**
     * @brief Close Socket.
     * @param[in,out] fd Input/output parameter.
     * @note Exception safety: noexcept.
     */
    static void closeSocket(SocketFd& fd) noexcept;
    /**
     * @brief Send All.
     * @param[in] fd Input parameter.
     * @param[in] buf Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    static bool sendAll(SocketFd fd, const std::string& buf) noexcept;

    /**
     * @brief Read Line.
     * @param[in] fd Input parameter.
     * @param[in,out] out Input/output parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    static bool readLine(SocketFd fd, std::string& out) noexcept;

    /**
     * @brief Redis Handshake.
     * @param[in] fd Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool redisHandshake(SocketFd fd) const noexcept;

    /**
     * @brief Build Resp Command.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    static std::string buildRespCommand(const std::vector<std::string>& args);

    /**
     * @brief Ensure Connected.
     * @param[in,out] nc Input/output parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool ensureConnected(NodeConn& nc) const noexcept;

    /**
     * @brief Send Command.
     * @param[in,out] nc Input/output parameter.
     * @param[in] args Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    std::optional<std::string> sendCommand(NodeConn& nc,
                                           const std::vector<std::string>& args) const noexcept;

    /**
     * @brief Send Command Locked.
     * @param[in,out] nc Input/output parameter.
     * @param[in] args Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    std::optional<std::string> sendCommandLocked(NodeConn& nc,
                                                 const std::vector<std::string>& args) const noexcept;

    /**
     * @brief Read Reply.
     * @param[in] fd Input parameter.
     * @param[in,out] out Input/output parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    static bool readReply(SocketFd fd, std::string& out) noexcept;

    // -----------------------------------------------------------------------
    // Serialization of CacheEntry
    // -----------------------------------------------------------------------

    /**
     * @brief Encode Entry.
     * @param[in] e Input parameter.
     * @return Return value.
     */
    static std::string encodeEntry(const CacheEntry& e);

    /**
     * @brief Decode Entry.
     * @param[in] raw Input parameter.
     * @return Return value.
     */
    static std::optional<CacheEntry> decodeEntry(const std::string& raw);

    /**
     * @brief ----------------------------------------------------------------------- Pub/sub subscriber (background thread) -----------------------------------------------------------------------
     * @param[in] key_or_pattern Input parameter.
     */

    void publishInvalidation(const std::string& key_or_pattern);
    /**
     * @brief Ensure Subscriber Loop Started.
     */
    void ensureSubscriberLoopStarted();

    /**
     * @brief Subscriber Loop.
     */
    void subscriberLoop();
    /**
     * @brief Subscriber Session.
     * @param[in] fd Input parameter.
     */
    void subscriberSession(SocketFd fd);
    /**
     * @brief Read Pub Sub Message.
     * @param[in] fd Input parameter.
     * @param[in,out] channel_out Input/output parameter.
     * @param[in,out] payload_out Input/output parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    static bool readPubSubMessage(SocketFd fd,
                                  std::string& channel_out,
                                  std::string& payload_out) noexcept;
    /**
     * @brief Dispatch Invalidation.
     * @param[in] payload Input parameter.
     */
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
