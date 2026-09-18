/**
 * @file cache_replication_coordinator.h
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

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <mutex>
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <thread>
#include <nlohmann/json.hpp>

namespace themis {
namespace cache {

struct ReplicationMessage {
    enum class Type {
        ENTRY_PUT,   ///< Replicate a new cache entry to peers
        INVALIDATE   ///< Propagate an invalidation to peers
    };

    Type        type;
    std::string key;        ///< Cache key (fingerprint for PUT; pattern for INVALIDATE)
    std::string tenant_id;  ///< Tenant identifier; empty = global
    int         ttl_seconds = 0;  ///< Remaining TTL (PUT only)
    nlohmann::json result;  ///< Serialised result value (PUT only; empty for INVALIDATE)
};

class ICacheCoordinator {
public:
    /**
     * @brief ICache Coordinator.
     * @return Return value.
     */
    virtual ~ICacheCoordinator() = default;

    /**
     * @brief ----------------------------------------------------------------- Publisher side (called by the local AdaptiveQueryCache) -----------------------------------------------------------------
     * @param[in] key Input parameter.
     * @param[in] result Input parameter.
     * @param[in] ttl_seconds Input parameter.
     * @param[in] tenant_id Identifier of the tenant.
     */

    virtual void publishEntry(const std::string& key,
                              const nlohmann::json& result,
                              int ttl_seconds,
                              const std::string& tenant_id) = 0;

    virtual void publishInvalidation(const std::string& pattern,
                                     const std::string& tenant_id = "") = 0;

    // -----------------------------------------------------------------
    // Subscriber side (registered once by AdaptiveQueryCache)
    // -----------------------------------------------------------------

    using EntryCallback = std::function<void(const ReplicationMessage&)>;

    using InvalidationCallback = std::function<void(const ReplicationMessage&)>;

    /**
     * @brief Subscribe Entries.
     * @param[in] callback Input parameter.
     */
    virtual void subscribeEntries(EntryCallback callback) = 0;

    /**
     * @brief Subscribe Invalidations.
     * @param[in] callback Input parameter.
     */
    virtual void subscribeInvalidations(InvalidationCallback callback) = 0;

    // -----------------------------------------------------------------
    // Health / diagnostics
    // -----------------------------------------------------------------

    /**
     * @brief Is Connected.
     * @return True when the operation succeeds.
     */
    virtual bool isConnected() const = 0;

    /**
     * @brief Name.
     * @return Return value.
     */
    virtual std::string name() const = 0;

    /**
     * @brief Get Stats.
     * @return Return value.
     */
    virtual nlohmann::json getStats() const = 0;
};

// ============================================================================
// InProcessCacheCoordinator
// ============================================================================

class InProcessCacheCoordinator final : public ICacheCoordinator {
public:
    struct Bus {
        std::mutex                              mutex = {};
        std::vector<InProcessCacheCoordinator*> peers;

        /**
         * @brief Add Peer.
         * @param[in,out] peer Input/output parameter.
         * @details Calls: lk(), push_back().
         */
        void addPeer(InProcessCacheCoordinator* peer) {
            std::lock_guard<std::mutex> lk(mutex);
            peers.push_back(peer);
        }

        /**
         * @brief Remove Peer.
         * @param[in,out] peer Input/output parameter.
         * @details Calls: lk(), erase(), std::remove(), begin(), end().
         */
        void removePeer(InProcessCacheCoordinator* peer) {
            std::lock_guard<std::mutex> lk(mutex);
            peers.erase(std::remove(peers.begin(), peers.end(), peer), peers.end());
        }
    };

    explicit InProcessCacheCoordinator(std::shared_ptr<Bus> bus = nullptr);
    ~InProcessCacheCoordinator() override;

    // ICacheCoordinator
    void publishEntry(const std::string& key,
                      const nlohmann::json& result,
                      int ttl_seconds,
                      const std::string& tenant_id) override;

    void publishInvalidation(const std::string& pattern,
                             const std::string& tenant_id = "") override;

    void subscribeEntries(EntryCallback callback) override;
    void subscribeInvalidations(InvalidationCallback callback) override;

    bool         isConnected() const override { return true; }
    std::string  name()        const override { return "InProcessCacheCoordinator"; }
    nlohmann::json getStats()  const override;

    /**
     * @brief Deliver.
     * @param[in] msg Input parameter.
     */
    void deliver(const ReplicationMessage& msg);

private:
    std::shared_ptr<Bus>  bus_;
    mutable std::mutex    mutex_;
    EntryCallback         entry_cb_;
    InvalidationCallback  invalidation_cb_;

    // Metrics
    uint64_t messages_sent_     = 0;
    uint64_t messages_received_ = 0;
};

// ============================================================================
// IRemoteCachePeer – cross-node invalidation interface
// ============================================================================

class IRemoteCachePeer {
public:
    /**
     * @brief IRemote Cache Peer.
     * @return Return value.
     */
    virtual ~IRemoteCachePeer() = default;

    virtual void invalidate(const std::string& key,
                            const std::string& tenant_id = "") = 0;

    /**
     * @brief Invalidate Tenant.
     * @param[in] tenant_id Identifier of the tenant.
     */
    virtual void invalidateTenant(const std::string& tenant_id) = 0;

    /**
     * @brief Address.
     * @return Return value.
     */
    virtual std::string address() const = 0;

    /**
     * @brief Is Healthy.
     * @return True when the operation succeeds.
     */
    virtual bool isHealthy() const = 0;
};

// ============================================================================
// IClusterView – cluster membership abstraction
// ============================================================================

class IClusterView {
public:
    /**
     * @brief ICluster View.
     * @return Return value.
     */
    virtual ~IClusterView() = default;

    /**
     * @brief Get Peer Addresses.
     * @return Return value.
     */
    virtual std::vector<std::string> getPeerAddresses() const = 0;
};

// ============================================================================
// CacheReplicationCoordinator
// ============================================================================

class CacheReplicationCoordinator final : public ICacheCoordinator {
public:
    static constexpr std::size_t kRetryQueueCapacity = 1024;

    static constexpr int kMaxRetryAttempts = 3;

    using PeerFactory =
        std::function<std::unique_ptr<IRemoteCachePeer>(const std::string& addr)>;

    explicit CacheReplicationCoordinator(
        IClusterView*                                    cluster_view,
        std::shared_ptr<InProcessCacheCoordinator::Bus>  bus          = nullptr,
        PeerFactory                                      peer_factory = nullptr);

    ~CacheReplicationCoordinator() override;

    // ICacheCoordinator
    void publishEntry(const std::string& key,
                      const nlohmann::json& result,
                      int ttl_seconds,
                      const std::string& tenant_id) override;

    void publishInvalidation(const std::string& pattern,
                             const std::string& tenant_id = "") override;

    void subscribeEntries(EntryCallback callback) override;
    void subscribeInvalidations(InvalidationCallback callback) override;

    bool           isConnected() const override;
    std::string    name()        const override { return "CacheReplicationCoordinator"; }
    nlohmann::json getStats()    const override;

    /**
     * @brief Refresh Peers.
     */
    void refreshPeers();

private:
    // ── Fanout queue item ─────────────────────────────────────────────────────

    struct FanoutItem {
        enum class Kind { INVALIDATE_KEY, INVALIDATE_TENANT } kind;
        std::string key;
        std::string tenant_id;
        int         attempts = 0;
        std::vector<std::shared_ptr<IRemoteCachePeer>> target_peers;

        FanoutItem asRetry(std::vector<std::shared_ptr<IRemoteCachePeer>> peers) const {
            FanoutItem r = *this;
            r.attempts++;
            r.target_peers = std::move(peers);
            return r;
        }
    };

    /**
     * @brief Fanout Worker.
     */
    void fanoutWorker();
    /**
     * @brief Enqueue Fanout.
     * @param[in] item Input parameter.
     */
    void enqueueFanout(FanoutItem item);

    // ── Members ───────────────────────────────────────────────────────────────

    IClusterView*          cluster_view_;
    PeerFactory            peer_factory_;

    InProcessCacheCoordinator local_;

    mutable std::mutex                                   peers_mutex_;
    std::vector<std::shared_ptr<IRemoteCachePeer>>       remote_peers_;

    // ── Bounded async fanout queue ────────────────────────────────────────────

    std::mutex              queue_mutex_;
    std::condition_variable queue_cv_;
    std::queue<FanoutItem>  fanout_queue_;
    bool                    stopping_ = false;
    std::thread             fanout_thread_;

    // ── Metrics ───────────────────────────────────────────────────────────────

    mutable std::mutex metrics_mutex_;
    uint64_t fanout_enqueued_  = 0;
    uint64_t fanout_dropped_   = 0;
    uint64_t fanout_delivered_ = 0;
    uint64_t fanout_retried_   = 0;
    uint64_t fanout_failed_    = 0;
};

} // namespace cache
} // namespace themis
