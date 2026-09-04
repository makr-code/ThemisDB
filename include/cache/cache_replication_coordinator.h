/**
 * @file cache_replication_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Message exchanged between cache nodes for replication and invalidation.
 *
 * Two message types exist:
 * - ENTRY_PUT   – a new entry was stored on the originating node; peers may
 *                 pre-populate their own L1/L2 from this message.
 * - INVALIDATE  – a key or pattern was invalidated on the originating node;
 *                 peers must evict matching L1/L2 entries.
 */
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

/**
 * @brief Abstract coordinator interface for cache replication across nodes.
 *
 * Implementations publish local cache mutations to remote peers and deliver
 * remote mutations to a registered AdaptiveQueryCache instance via callbacks.
 *
 * Graceful degradation:
 *   Any exception thrown by publish methods is caught by the caller
 *   (AdaptiveQueryCache) and demoted to a warning log; the local cache
 *   operation always completes regardless of coordinator state.
 */
class ICacheCoordinator {
public:
    virtual ~ICacheCoordinator() = default;

    // -----------------------------------------------------------------
    // Publisher side (called by the local AdaptiveQueryCache)
    // -----------------------------------------------------------------

    /**
     * @brief Publish a new cache entry so peers can pre-populate their caches.
     *
     * @param key         Cache key (fingerprint).
     * @param result      Serialised query result.
     * @param ttl_seconds Remaining TTL for the entry.
     * @param tenant_id   Tenant identifier; empty string = no tenant.
     */
    virtual void publishEntry(const std::string& key,
                              const nlohmann::json& result,
                              int ttl_seconds,
                              const std::string& tenant_id) = 0;

    /**
     * @brief Publish an invalidation event so peers evict matching entries.
     *
     * @param pattern   Key or regex pattern that was invalidated locally.
     * @param tenant_id Tenant identifier; empty = global invalidation.
     */
    virtual void publishInvalidation(const std::string& pattern,
                                     const std::string& tenant_id = "") = 0;

    // -----------------------------------------------------------------
    // Subscriber side (registered once by AdaptiveQueryCache)
    // -----------------------------------------------------------------

    /** Callback invoked when a peer publishes a new entry. */
    using EntryCallback = std::function<void(const ReplicationMessage&)>;

    /** Callback invoked when a peer publishes an invalidation. */
    using InvalidationCallback = std::function<void(const ReplicationMessage&)>;

    /**
     * @brief Register a callback for incoming replicated entries.
     *
     * Only one callback is supported per coordinator instance; subsequent
     * calls overwrite the previous registration.
     */
    virtual void subscribeEntries(EntryCallback callback) = 0;

    /**
     * @brief Register a callback for incoming invalidation messages.
     *
     * Only one callback is supported per coordinator instance; subsequent
     * calls overwrite the previous registration.
     */
    virtual void subscribeInvalidations(InvalidationCallback callback) = 0;

    // -----------------------------------------------------------------
    // Health / diagnostics
    // -----------------------------------------------------------------

    /** @return true if the coordinator channel is operational. */
    virtual bool isConnected() const = 0;

    /** @return Coordinator name / transport description for logging. */
    virtual std::string name() const = 0;

    /** @return JSON snapshot of coordinator metrics (messages sent/received). */
    virtual nlohmann::json getStats() const = 0;
};

// ============================================================================
// InProcessCacheCoordinator
// ============================================================================

/**
 * @brief In-process coordinator for single-binary multi-cache-instance
 *        deployments and unit tests.
 *
 * Multiple AdaptiveQueryCache instances share one coordinator bus.  Any
 * message published on one instance is delivered synchronously to all other
 * instances registered on the same bus.
 *
 * Thread-safety: all methods are protected by an internal mutex.
 *
 * Production note: for true multi-node deployments replace this with a
 * network-backed implementation (e.g. Redis pub/sub via hiredis, or a
 * ThemisDB native cluster bus).  The ICacheCoordinator interface remains
 * stable across implementations.
 */
class InProcessCacheCoordinator final : public ICacheCoordinator {
public:
    /**
     * @brief Shared message bus that links multiple coordinator instances.
     *
     * All coordinators created with the same Bus share the same set of
     * subscribers; a message published on any one coordinator is delivered
     * to all *other* coordinators on the same bus.
     */
    struct Bus {
        std::mutex                              mutex = {};
        std::vector<InProcessCacheCoordinator*> peers;

        void addPeer(InProcessCacheCoordinator* peer) {
            std::lock_guard<std::mutex> lk(mutex);
            peers.push_back(peer);
        }

        void removePeer(InProcessCacheCoordinator* peer) {
            std::lock_guard<std::mutex> lk(mutex);
            peers.erase(std::remove(peers.begin(), peers.end(), peer), peers.end());
        }
    };

    /**
     * @brief Construct a coordinator and join a shared bus.
     *
     * @param bus  Shared bus instance.  If nullptr a standalone coordinator
     *             is created (messages are only delivered to the same
     *             instance – useful for integration tests of a single cache).
     */
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
     * @brief Deliver a message directly to this coordinator instance.
     *
     * Called by sibling coordinators on the same bus; not intended for
     * external callers.
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

/**
 * @brief Abstract interface for a remote cache peer reachable over the network.
 *
 * Implementations (e.g. GrpcRemoteCachePeer) contact a specific remote node
 * and deliver cache invalidation messages.  Each method is expected to be
 * callable from a background fanout thread and must be thread-safe.
 */
class IRemoteCachePeer {
public:
    virtual ~IRemoteCachePeer() = default;

    /**
     * @brief Invalidate one key (or a glob/regex pattern) on the remote peer.
     *
     * @param key       Cache key or pattern to invalidate.
     * @param tenant_id Optional tenant scope; empty = global.
     */
    virtual void invalidate(const std::string& key,
                            const std::string& tenant_id = "") = 0;

    /**
     * @brief Invalidate all keys belonging to a specific tenant on the peer.
     *
     * @param tenant_id Tenant identifier; must not be empty.
     */
    virtual void invalidateTenant(const std::string& tenant_id) = 0;

    /** @return Human-readable address/identifier of this peer (for logging). */
    virtual std::string address() const = 0;

    /** @return true when the peer connection is believed to be healthy. */
    virtual bool isHealthy() const = 0;
};

// ============================================================================
// IClusterView – cluster membership abstraction
// ============================================================================

/**
 * @brief Provides a snapshot of cluster peer addresses.
 *
 * An implementation may source peer addresses from a Raft log, a gossip
 * membership table, or a static configuration file.  The returned addresses
 * are used by CacheReplicationCoordinator to build its remote-peer list.
 */
class IClusterView {
public:
    virtual ~IClusterView() = default;

    /**
     * @brief Return addresses of all known cache-capable peers in the cluster.
     *
     * Addresses are in "host:port" format compatible with the gRPC channel API.
     * The local node's own address should NOT be included.
     */
    virtual std::vector<std::string> getPeerAddresses() const = 0;
};

// ============================================================================
// CacheReplicationCoordinator
// ============================================================================

/**
 * @brief Network-capable replication coordinator for clustered deployments.
 *
 * Wraps an InProcessCacheCoordinator for local (intra-process) fanout and
 * adds network fanout to remote peers obtained from a ClusterView.
 *
 * Fanout strategy (fire-and-forget):
 * - Invalidations are enqueued into a bounded retry queue
 *   (max kRetryQueueCapacity entries); if the queue is full the entry is
 *   dropped with a warning log.
 * - A background worker thread drains the queue and calls invalidate() /
 *   invalidateTenant() on each remote peer; failed calls are retried up to
 *   kMaxRetryAttempts times before being dropped.
 * - publishEntry() (triggered by put()) is NOT blocked on remote peer
 *   acknowledgment – only local in-process fanout is synchronous.
 *
 * Thread-safety: all public methods are thread-safe.
 */
class CacheReplicationCoordinator final : public ICacheCoordinator {
public:
    /// Maximum entries in the bounded async fanout queue.
    static constexpr std::size_t kRetryQueueCapacity = 1024;

    /// Maximum delivery attempts per message before dropping.
    static constexpr int kMaxRetryAttempts = 3;

    /**
     * @brief Factory function type for constructing IRemoteCachePeer instances.
     *
     * Injected to allow test doubles to be substituted without requiring a
     * live gRPC server.  The factory receives a peer address ("host:port")
     * and returns an owning pointer to the peer implementation.
     */
    using PeerFactory =
        std::function<std::unique_ptr<IRemoteCachePeer>(const std::string& addr)>;

    /**
     * @brief Construct a coordinator backed by a ClusterView.
     *
     * @param cluster_view  Provides peer addresses; must outlive this object.
     *                      May be nullptr (no remote peers; behaves like
     *                      InProcessCacheCoordinator).
     * @param bus           Optional intra-process bus forwarded to the inner
     *                      InProcessCacheCoordinator.
     * @param peer_factory  Factory for creating IRemoteCachePeer instances.
     *                      May be nullptr (remote fanout is disabled; useful
     *                      for single-node deployments and unit tests that do
     *                      not need gRPC).
     */
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
     * @brief Refresh the remote peer list from the injected ClusterView.
     *
     * Called automatically during construction; may also be invoked explicitly
     * when cluster membership changes (e.g. after a Raft leader election).
     * Thread-safe: can be called concurrently with publish methods.
     */
    void refreshPeers();

private:
    // ── Fanout queue item ─────────────────────────────────────────────────────

    struct FanoutItem {
        enum class Kind { INVALIDATE_KEY, INVALIDATE_TENANT } kind;
        std::string key;
        std::string tenant_id;
        int         attempts = 0;
        /// Non-empty on retries: only these specific peers need to be contacted.
        /// Empty means fanout to all current remote peers.
        std::vector<std::shared_ptr<IRemoteCachePeer>> target_peers;

        /// Return a copy of this item configured as a retry targeting @p peers.
        FanoutItem asRetry(std::vector<std::shared_ptr<IRemoteCachePeer>> peers) const {
            FanoutItem r = *this;
            r.attempts++;
            r.target_peers = std::move(peers);
            return r;
        }
    };

    void fanoutWorker();
    void enqueueFanout(FanoutItem item);

    // ── Members ───────────────────────────────────────────────────────────────

    IClusterView*          cluster_view_;
    PeerFactory            peer_factory_;

    /// Intra-process delegate (handles local Bus fanout and all subscriptions).
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
