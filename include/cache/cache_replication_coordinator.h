/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cache_replication_coordinator.h                    ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-03-16 04:05:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     233                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 6f1a5731d  2026-02-24  fix(cache): code audit – missing <algorithm>, use-after-f... ║
    • 0d58fbec9  2026-02-24  feat(cache): Add cache replication for high-availability ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
        std::mutex                              mutex;
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

} // namespace cache
} // namespace themis
