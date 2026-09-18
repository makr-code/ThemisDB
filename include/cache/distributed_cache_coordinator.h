/**
 * @file distributed_cache_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
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

struct RedisCacheCoordinatorConfig {
    std::string host = "127.0.0.1";

    uint16_t port = 6379;

    std::string channel_prefix = "themis:cache";

    std::string password;

    int db_index = 0;

    int connect_timeout_ms = 2000;

    int reconnect_interval_ms = 5000;

    size_t max_message_bytes = 65536;

    std::string hmac_secret;
};

// ---------------------------------------------------------------------------
// RedisCacheCoordinator
// ---------------------------------------------------------------------------

class RedisCacheCoordinator final : public ICacheCoordinator {
public:
    explicit RedisCacheCoordinator(const RedisCacheCoordinatorConfig& config = {});

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
     * @brief Entry Channel.
     * @return Return value.
     */
    std::string entryChannel()        const;
    /**
     * @brief Invalidation Channel.
     * @return Return value.
     */
    std::string invalidationChannel() const;

private:
    // Internal socket handle (platform-independent typedef for int fd)
    using SocketFd = int;
    static constexpr SocketFd kInvalidSocket = -1;

    /**
     * @brief ----------------------------------------------------------------------- Low-level TCP / RESP helpers -----------------------------------------------------------------------
     * @return Return value.
     */

    SocketFd tcpConnect();

    /**
     * @brief Close Socket.
     * @param[in,out] fd Input/output parameter.
     */
    static void closeSocket(SocketFd& fd);

    /**
     * @brief Send All.
     * @param[in] fd Input parameter.
     * @param[in] buf Input parameter.
     * @return True when the operation succeeds.
     */
    static bool sendAll(SocketFd fd, const std::string& buf);

    /**
     * @brief Read Line.
     * @param[in] fd Input parameter.
     * @param[in,out] line_out Input/output parameter.
     * @return True when the operation succeeds.
     */
    static bool readLine(SocketFd fd, std::string& line_out);

    /**
     * @brief Redis Handshake.
     * @param[in] fd Input parameter.
     * @return True when the operation succeeds.
     */
    bool redisHandshake(SocketFd fd);

    /**
     * @brief Build Resp Command.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    static std::string buildRespCommand(const std::vector<std::string>& args);

    /**
     * @brief Redis Publish.
     * @param[in] channel Input parameter.
     * @param[in] payload Input parameter.
     * @return True when the operation succeeds.
     */
    bool redisPublish(const std::string& channel, const std::string& payload);

    // -----------------------------------------------------------------------
    // Publisher connection management
    // -----------------------------------------------------------------------

    /**
     * @brief Ensure Publisher Connected.
     * @return True when the operation succeeds.
     */
    bool ensurePublisherConnected();

    // -----------------------------------------------------------------------
    // Subscriber thread
    // -----------------------------------------------------------------------

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
     */
    static bool readPubSubMessage(SocketFd fd,
                                  std::string& channel_out,
                                  std::string& payload_out);

    /**
     * @brief Dispatch Message.
     * @param[in] channel Input parameter.
     * @param[in] payload Input parameter.
     */
    void dispatchMessage(const std::string& channel,
                         const std::string& payload);

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

    // -----------------------------------------------------------------------
    // Members
    // -----------------------------------------------------------------------

    RedisCacheCoordinatorConfig config_;

    // Publisher connection
    mutable std::mutex pub_mutex_;
    SocketFd           pub_fd_   = kInvalidSocket;
    std::atomic<bool>  pub_ok_{false};  // D-3: atomic for lock-free reads in isConnected()

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
    using RedisPublishBridgeFn = std::function<bool(const std::string& channel,
                                                    const std::string& payload)>;

    /**
     * @brief Set Redis Publish Bridge Fn.
     * @param[in] fn Input parameter.
     */
    static void setRedisPublishBridgeFn(RedisPublishBridgeFn fn);
};

} // namespace cache
} // namespace themis

