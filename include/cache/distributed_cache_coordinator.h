/**
 * @file distributed_cache_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "cache/cache_replication_coordinator.h"
#include <nlohmann/json.hpp>
#include <string>
#include <functional>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>

namespace themis {
namespace cache {

// ---------------------------------------------------------------------------
// RedisCacheCoordinatorConfig
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for the Redis-backed distributed cache coordinator.
 */
struct RedisCacheCoordinatorConfig {
    /// Redis server hostname or IP address.
    std::string host = "127.0.0.1";

    /// Redis server port.
    uint16_t port = 6379;

    /**
     * @brief Channel prefix for pub/sub.
     *
     * Two sub-channels are used:
     *   - "<channel_prefix>:entries"      – new entry notifications
     *   - "<channel_prefix>:invalidations" – key/pattern invalidations
     */
    std::string channel_prefix = "themis:cache";

    /// Optional Redis AUTH password (empty = no authentication).
    std::string password;

    /// Optional Redis database index (0–15).
    int db_index = 0;

    /// TCP connect timeout in milliseconds.
    int connect_timeout_ms = 2000;

    /// Interval between reconnect attempts when connection is lost (ms).
    int reconnect_interval_ms = 5000;

    /// Maximum number of bytes for a single published message.
    size_t max_message_bytes = 65536;

    /// Optional HMAC-SHA256 secret for message signing/verification.
    ///
    /// When non-empty, every published message is signed with
    /// HMAC-SHA256(hmac_secret, payload) and the resulting hex digest is
    /// appended as a "sig" field.  Received messages whose "sig" field
    /// is absent or does not match are silently discarded.
    /// When empty, signing and verification are disabled (default).
    std::string hmac_secret;
};

// ---------------------------------------------------------------------------
// RedisCacheCoordinator
// ---------------------------------------------------------------------------

/**
 * @brief Distributed cache coordinator backed by Redis pub/sub.
 *
 * Implements ICacheCoordinator using the RESP wire protocol.  Each instance
 * opens two TCP connections to the configured Redis server:
 *   1. Publisher connection – used by `publishEntry()` and
 *      `publishInvalidation()`.
 *   2. Subscriber connection – dedicated receive loop (background thread) that
 *      calls the registered callbacks when messages arrive.
 *
 * Thread-safety: all public methods are thread-safe.
 */
class RedisCacheCoordinator final : public ICacheCoordinator {
public:
    /**
     * @brief Construct the coordinator and start background connection.
     *
     * @param config  Configuration for the Redis connection and channels.
     *
     * The constructor starts the background subscriber thread immediately.
     * The actual TCP connections are established asynchronously; `isConnected()`
     * returns false until the first successful handshake.
     */
    explicit RedisCacheCoordinator(const RedisCacheCoordinatorConfig& config = {});

    /**
     * @brief Destroy the coordinator, close connections, and join threads.
     */
    ~RedisCacheCoordinator() override;

    // Non-copyable
    RedisCacheCoordinator(const RedisCacheCoordinator&) = delete;
    RedisCacheCoordinator& operator=(const RedisCacheCoordinator&) = delete;

    // -----------------------------------------------------------------------
    // ICacheCoordinator implementation
    // -----------------------------------------------------------------------

    void publishEntry(const std::string& key,
                      const nlohmann::json& result,
                      int ttl_seconds,
                      const std::string& tenant_id) override;

    void publishInvalidation(const std::string& pattern,
                             const std::string& tenant_id = "") override;

    void subscribeEntries(EntryCallback callback) override;
    void subscribeInvalidations(InvalidationCallback callback) override;

    bool           isConnected() const override;
    std::string    name()        const override { return "RedisCacheCoordinator"; }
    nlohmann::json getStats()    const override;

    // -----------------------------------------------------------------------
    // Additional diagnostics
    // -----------------------------------------------------------------------

    /**
     * @brief Return the effective channel names in use.
     */
    std::string entryChannel()        const;
    std::string invalidationChannel() const;

private:
    // Internal socket handle (platform-independent typedef for int fd)
    using SocketFd = int;
    static constexpr SocketFd kInvalidSocket = -1;

    // -----------------------------------------------------------------------
    // Low-level TCP / RESP helpers
    // -----------------------------------------------------------------------

    /// Open a blocking TCP connection to config_.host:config_.port.
    SocketFd tcpConnect();

    /// Close a socket safely.
    static void closeSocket(SocketFd& fd);

    /// Send all bytes in buf; returns false on error.
    static bool sendAll(SocketFd fd, const std::string& buf);

    /// Read a complete RESP simple-string or bulk-string reply line.
    /// Returns true on success; on error or "-ERR …" sets err_out.
    static bool readLine(SocketFd fd, std::string& line_out);

    /// Perform AUTH + SELECT handshake on a freshly connected socket.
    bool redisHandshake(SocketFd fd);

    /// Build a RESP array command string.
    static std::string buildRespCommand(const std::vector<std::string>& args);

    /// Publish a raw RESP payload on a channel.  Returns false on failure.
    bool redisPublish(const std::string& channel, const std::string& payload);

    // -----------------------------------------------------------------------
    // Publisher connection management
    // -----------------------------------------------------------------------

    /// Ensure the publisher socket is open; reconnect if necessary.
    bool ensurePublisherConnected();

    // -----------------------------------------------------------------------
    // Subscriber thread
    // -----------------------------------------------------------------------

    /// Entry point for the subscriber background thread.
    void subscriberLoop();

    /// Connect subscriber socket, send SUBSCRIBE, then pump messages.
    /// Returns when the connection drops or stop_ is set.
    void subscriberSession(SocketFd fd);

    /// Parse one pub/sub message frame from the subscriber socket.
    /// Returns true and populates channel/payload when a complete message
    /// has been received.  Returns false on connection error.
    static bool readPubSubMessage(SocketFd fd,
                                  std::string& channel_out,
                                  std::string& payload_out);

    /// Dispatch a received message to the appropriate callback.
    void dispatchMessage(const std::string& channel,
                         const std::string& payload);

    /// Compute HMAC-SHA256(config_.hmac_secret, payload) and return hex string.
    /// Returns empty string when hmac_secret is empty.
    std::string computeHmac(const std::string& payload) const;

    /// Verify the "sig" field in parsed JSON against the unsigned payload.
    /// Returns true when hmac_secret is empty (signing disabled) or when the
    /// signature matches.  Returns false on mismatch or absent sig field.
    bool verifyHmac(const nlohmann::json& j) const;

    // -----------------------------------------------------------------------
    // Members
    // -----------------------------------------------------------------------

    RedisCacheCoordinatorConfig config_;

    // Publisher connection
    mutable std::mutex pub_mutex_;
    SocketFd           pub_fd_   = kInvalidSocket;
    std::atomic<bool>  pub_ok_{false};  // D-3: atomic for lock-free reads in isConnected()

    /// C2: Double-checked locking guard for expensive publisher initialization.
    /// Set to true (memory_order_release) once ensurePublisherConnected() succeeds.
    /// Reset to false (memory_order_release) on any send/read failure.
    std::atomic<bool>  coordinator_ready_{false};

    // Subscriber thread
    std::thread        sub_thread_;
    std::atomic<bool>  stop_{false};
    std::atomic<bool>  sub_connected_{false};

    // Callbacks
    mutable std::mutex callbacks_mutex_;
    EntryCallback        entry_cb_;
    InvalidationCallback invalidation_cb_;

    // Statistics — all four counters are exclusively mutated and read under
    // stats_mutex_; std::atomic is not required because the mutex provides
    // the necessary sequencing and visibility guarantees.  (missing_volatile
    // scanner note: these are intentionally plain uint64_t, not volatile.)
    mutable std::mutex      stats_mutex_;
    uint64_t messages_published_  = 0;  ///< guarded by stats_mutex_
    uint64_t messages_received_   = 0;  ///< guarded by stats_mutex_
    uint64_t publish_errors_      = 0;  ///< guarded by stats_mutex_
    uint64_t reconnect_count_     = 0;  ///< guarded by stats_mutex_

public:
    // -----------------------------------------------------------------------
    // Injectable publish bridge (STUB #61)
    // -----------------------------------------------------------------------
    /// Callback type: given a channel name and a serialised JSON payload,
    /// publish the message and return true on success.  Used as a transport
    /// replacement when THEMIS_POSIX_SOCKETS is not defined (non-POSIX builds).
    using RedisPublishBridgeFn = std::function<bool(const std::string& channel,
                                                    const std::string& payload)>;

    /// Register a publish bridge used by `publishEntry()` and
    /// `publishInvalidation()` on non-POSIX builds.
    /// Pass an empty `std::function` to clear and revert to the no-op fallback.
    /// Thread-safe (guarded by a static mutex).
    static void setRedisPublishBridgeFn(RedisPublishBridgeFn fn);
};

} // namespace cache
} // namespace themis

