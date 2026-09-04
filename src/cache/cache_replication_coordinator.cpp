/**
 * @file cache_replication_coordinator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=2; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, blocking_no_timeout=0(fixed), C=1, H=5, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "cache/cache_replication_coordinator.h"
#include <chrono>
#include <stdexcept>
#include "utils/logger.h"
#include <unordered_map>

namespace {
// Maximum time the fanout worker blocks waiting for new queue items.
// A bounded deadline ensures the thread re-checks stopping_ periodically,
// guarding against indefinite hangs during abnormal shutdown sequences.
constexpr auto kFanoutWorkerWakeInterval = std::chrono::milliseconds(500);
} // namespace

namespace themis {
namespace cache {

// ============================================================================
// LOCK ORDER — must always be acquired in this canonical sequence to prevent
// circular lock ordering and deadlock throughout this translation unit:
//
// InProcessCacheCoordinator:
//   1. mutex_           (instance stats: messages_sent_, messages_received_,
//                        and callback slots entry_cb_ / invalidation_cb_)
//   2. bus_->mutex      (shared Bus peer list; lives in the Bus struct)
//
//   These two are NEVER held simultaneously. mutex_ is always released before
//   bus_->mutex is acquired (sequential, not nested). This is a deliberate
//   design choice: holding both simultaneously is unnecessary and avoided to
//   eliminate all ordering risk.
//
// CacheReplicationCoordinator:
//   1. peers_mutex_     (remote_peers_ vector)
//   2. queue_mutex_     (fanout_queue_ and stopping_ flag)
//   3. metrics_mutex_   (fanout counters: enqueued, dropped, delivered, etc.)
//
//   Rules:
//   - Never acquire metrics_mutex_ while holding queue_mutex_ (nested order
//     risk). enqueueFanout() restructures metric updates to occur outside the
//     queue lock to respect this hierarchy.
//   - peers_mutex_ is never held during network I/O; a snapshot of
//     remote_peers_ is taken under the lock then released before any I/O.
// ============================================================================

// ============================================================================
// InProcessCacheCoordinator
// ============================================================================

InProcessCacheCoordinator::InProcessCacheCoordinator(std::shared_ptr<Bus> bus)
    : bus_(std::move(bus)) {
    if (bus_) {
        bus_->addPeer(this);
    }
    THEMIS_DEBUG("InProcessCacheCoordinator created (bus={})",
                 bus_ ? "shared" : "standalone");
}

InProcessCacheCoordinator::~InProcessCacheCoordinator() {
    if (bus_) {
        bus_->removePeer(this);
    }
}

void InProcessCacheCoordinator::publishEntry(const std::string& key,
                                              const nlohmann::json& result,
                                              int ttl_seconds,
                                              const std::string& tenant_id) {
    ReplicationMessage msg;
    msg.type        = ReplicationMessage::Type::ENTRY_PUT;
    msg.key         = key;
    msg.result      = result;
    msg.ttl_seconds = ttl_seconds;
    msg.tenant_id   = tenant_id;

    // LOCK ORDER: mutex_ is acquired and released first (stats increment only),
    // then bus_->mutex is acquired separately for the peer fanout. These two
    // mutexes are intentionally NOT held simultaneously; the sequential pattern
    // is safe and avoids any circular ordering risk.
    {
        std::lock_guard<std::mutex> lk(mutex_);
        ++messages_sent_;
    }

    if (!bus_) {
        return;  // Standalone: no peers to notify
    }

    // bus_->mutex acquired only after mutex_ has been fully released (above).
    std::lock_guard<std::mutex> bus_lk(bus_->mutex);
    for (auto* peer : bus_->peers) {
        if (peer != this) {
            peer->deliver(msg);
        }
    }
}

void InProcessCacheCoordinator::publishInvalidation(const std::string& pattern,
                                                     const std::string& tenant_id) {
    ReplicationMessage msg;
    msg.type      = ReplicationMessage::Type::INVALIDATE;
    msg.key       = pattern;
    msg.tenant_id = tenant_id;

    // LOCK ORDER: mutex_ is acquired and released first (stats increment only),
    // then bus_->mutex is acquired separately for the peer fanout. These two
    // mutexes are intentionally NOT held simultaneously; the sequential pattern
    // is safe and avoids any circular ordering risk.
    {
        std::lock_guard<std::mutex> lk(mutex_);
        ++messages_sent_;
    }

    if (!bus_) {
        return;  // Standalone: no peers to notify
    }

    // bus_->mutex acquired only after mutex_ has been fully released (above).
    std::lock_guard<std::mutex> bus_lk(bus_->mutex);
    for (auto* peer : bus_->peers) {
        if (peer != this) {
            peer->deliver(msg);
        }
    }
}

void InProcessCacheCoordinator::subscribeEntries(EntryCallback callback) {
    std::lock_guard<std::mutex> lk(mutex_);
    entry_cb_ = std::move(callback);
}

void InProcessCacheCoordinator::subscribeInvalidations(InvalidationCallback callback) {
    std::lock_guard<std::mutex> lk(mutex_);
    invalidation_cb_ = std::move(callback);
}

void InProcessCacheCoordinator::deliver(const ReplicationMessage& msg) {
    EntryCallback        entry_cb;
    InvalidationCallback inv_cb;

    {
        std::lock_guard<std::mutex> lk(mutex_);
        ++messages_received_;
        entry_cb = entry_cb_;
        inv_cb   = invalidation_cb_;
    }

    try {
        if (msg.type == ReplicationMessage::Type::ENTRY_PUT) {
            if (entry_cb) {
                entry_cb(msg);
            }
        } else {
            if (inv_cb) {
                inv_cb(msg);
            }
        }
    } catch (const std::exception& e) {
        THEMIS_WARN("CacheReplicationCoordinator: exception in subscriber callback: {}",
                    e.what());
    }
}

nlohmann::json InProcessCacheCoordinator::getStats() const {
    uint64_t sent = 0, received = 0;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        sent     = messages_sent_;
        received = messages_received_;
    }
    size_t peer_count = 0;
    if (bus_) {
        std::lock_guard<std::mutex> bus_lk(bus_->mutex);
        peer_count = bus_->peers.size() > 0 ? bus_->peers.size() - 1 : 0;
    }
    return {
        {"name",               name()},
        {"connected",          isConnected()},
        {"messages_sent",      sent},
        {"messages_received",  received},
        {"peer_count",         peer_count}
    };
}

// ============================================================================
// CacheReplicationCoordinator
// ============================================================================

CacheReplicationCoordinator::CacheReplicationCoordinator(
    IClusterView*                                    cluster_view,
    std::shared_ptr<InProcessCacheCoordinator::Bus>  bus,
    PeerFactory                                      peer_factory)
    : cluster_view_(cluster_view)
    , peer_factory_(std::move(peer_factory))
    , local_(std::move(bus))
{
    refreshPeers();
    fanout_thread_ = std::thread(&CacheReplicationCoordinator::fanoutWorker, this);
    THEMIS_DEBUG("CacheReplicationCoordinator created ({} remote peers)",
                 remote_peers_.size());
}

CacheReplicationCoordinator::~CacheReplicationCoordinator() {
    {
        std::lock_guard<std::mutex> lk(queue_mutex_);
        stopping_ = true;
    }
    queue_cv_.notify_all();
    if (fanout_thread_.joinable()) {
        fanout_thread_.join();
    }
}

void CacheReplicationCoordinator::refreshPeers() {
    if (!cluster_view_ || !peer_factory_) {
        return;
    }

    const auto addresses = cluster_view_->getPeerAddresses();

    std::lock_guard<std::mutex> lk(peers_mutex_);

    // Build a lookup map from address → existing peer to avoid
    // tearing down and rebuilding connections that haven't changed.
    std::unordered_map<std::string, std::shared_ptr<IRemoteCachePeer>> existing;
    existing.reserve(remote_peers_.size());
    for (auto& p : remote_peers_) {
        existing[p->address()] = p;
    }

    std::vector<std::shared_ptr<IRemoteCachePeer>> new_peers;
    new_peers.reserve(addresses.size());
    for (const auto& addr : addresses) {
        auto it = existing.find(addr);
        if (it != existing.end()) {
            // Reuse the existing connection for this address.
            new_peers.emplace_back(it->second);
        } else {
            // New address: create a fresh peer via the factory.
            auto peer = peer_factory_(addr);
            if (peer) {
                new_peers.emplace_back(std::move(peer));
            }
        }
    }

    remote_peers_ = std::move(new_peers);

    THEMIS_DEBUG("CacheReplicationCoordinator: refreshed {} remote peers",
                 remote_peers_.size());
}

void CacheReplicationCoordinator::publishEntry(const std::string& key,
                                                const nlohmann::json& result,
                                                int ttl_seconds,
                                                const std::string& tenant_id) {
    // Forward to local in-process bus only; do not block on remote acknowledgment.
    local_.publishEntry(key, result, ttl_seconds, tenant_id);
}

void CacheReplicationCoordinator::publishInvalidation(const std::string& pattern,
                                                       const std::string& tenant_id) {
    // Deliver to local bus synchronously.
    local_.publishInvalidation(pattern, tenant_id);

    // Enqueue async fanout to remote peers.
    FanoutItem item;
    item.kind      = FanoutItem::Kind::INVALIDATE_KEY;
    item.key       = pattern;
    item.tenant_id = tenant_id;
    item.attempts  = 0;
    enqueueFanout(std::move(item));
}

void CacheReplicationCoordinator::subscribeEntries(EntryCallback callback) {
    local_.subscribeEntries(std::move(callback));
}

void CacheReplicationCoordinator::subscribeInvalidations(InvalidationCallback callback) {
    local_.subscribeInvalidations(std::move(callback));
}

bool CacheReplicationCoordinator::isConnected() const {
    return local_.isConnected();
}

nlohmann::json CacheReplicationCoordinator::getStats() const {
    uint64_t enqueued = 0, dropped = 0, delivered = 0, retried = 0, failed = 0;
    {
        std::lock_guard<std::mutex> lk(metrics_mutex_);
        enqueued  = fanout_enqueued_;
        dropped   = fanout_dropped_;
        delivered = fanout_delivered_;
        retried   = fanout_retried_;
        failed    = fanout_failed_;
    }
    std::size_t peer_count = 0;
    {
        std::lock_guard<std::mutex> lk(peers_mutex_);
        peer_count = remote_peers_.size();
    }
    auto stats = local_.getStats();
    stats["name"]               = name();
    stats["remote_peer_count"]  = peer_count;
    stats["fanout_enqueued"]    = enqueued;
    stats["fanout_dropped"]     = dropped;
    stats["fanout_delivered"]   = delivered;
    stats["fanout_retried"]     = retried;
    stats["fanout_failed"]      = failed;
    return stats;
}

// ── Private helpers ───────────────────────────────────────────────────────────

void CacheReplicationCoordinator::enqueueFanout(FanoutItem item) {
    // LOCK ORDER: queue_mutex_ and metrics_mutex_ must never be nested.
    // Determine the outcome under queue_mutex_, then update metrics separately
    // under metrics_mutex_ once the queue lock has been released.
    enum class EnqueueResult { Enqueued, Dropped };
    EnqueueResult result = EnqueueResult::Dropped;

    {
        std::lock_guard<std::mutex> lk(queue_mutex_);
        if (static_cast<int>(fanout_queue_.size()) >= kRetryQueueCapacity) {
            THEMIS_WARN("[CacheReplicationCoordinator] fanout queue full ({} entries); "
                        "dropping invalidation for key='{}'",
                        kRetryQueueCapacity, item.key);
            // C3: Structured eviction tracking telemetry.
            THEMIS_WARN("{{\"event\":\"eviction_fanout_drop\",\"eviction_reason\":\"queue_full\","
                        "\"evicted_entries\":1,\"freed_bytes\":0,\"key\":\"{}\"}}",
                        item.key);
            result = EnqueueResult::Dropped;
        } else {
            fanout_queue_.push(std::move(item));
            result = EnqueueResult::Enqueued;
        }
    } // queue_mutex_ released before metrics_mutex_ is acquired

    // Update metrics outside queue_mutex_ to eliminate the nested-lock
    // circular_lock_ordering risk flagged at HIGH severity.
    {
        std::lock_guard<std::mutex> ml(metrics_mutex_);
        if (result == EnqueueResult::Enqueued) {
            ++fanout_enqueued_;
        } else {
            ++fanout_dropped_;
        }
    }

    if (result == EnqueueResult::Enqueued) {
        queue_cv_.notify_one();
    }
}

void CacheReplicationCoordinator::fanoutWorker() {
    while (true) {
        FanoutItem item;
        {
            std::unique_lock<std::mutex> lk(queue_mutex_);
            // Use wait_for with a bounded deadline to prevent indefinite blocking.
            // The worker wakes up at least every kFanoutWorkerWakeInterval and
            // re-checks stopping_, so abnormal shutdown paths cannot hang forever.
            queue_cv_.wait_for(lk, kFanoutWorkerWakeInterval, [this] {
                return !fanout_queue_.empty() || stopping_;
            });
            if (stopping_ && fanout_queue_.empty()) {
                break;
            }
            item = std::move(fanout_queue_.front());
            fanout_queue_.pop();
        }

        // Snapshot the peer list to avoid holding the lock during network I/O.
        // shared_ptr copies keep the objects alive even if refreshPeers() runs
        // concurrently and replaces remote_peers_.
        // On retries, item.target_peers contains only the peers that previously
        // failed so we do not re-deliver to peers that already succeeded.
        std::vector<std::shared_ptr<IRemoteCachePeer>> peers_to_contact;
        if (!item.target_peers.empty()) {
            peers_to_contact = item.target_peers;
        } else {
            std::lock_guard<std::mutex> pl(peers_mutex_);
            peers_to_contact = remote_peers_;
        }

        std::vector<std::shared_ptr<IRemoteCachePeer>> failed_peers;
        for (const auto& peer : peers_to_contact) {
            try {
                if (item.kind == FanoutItem::Kind::INVALIDATE_KEY) {
                    peer->invalidate(item.key, item.tenant_id);
                } else {
                    peer->invalidateTenant(item.tenant_id);
                }
                {
                    std::lock_guard<std::mutex> ml(metrics_mutex_);
                    ++fanout_delivered_;
                }
            } catch (const std::exception& e) {
                THEMIS_WARN("[CacheReplicationCoordinator] fanout to peer '{}' failed: {}",
                            peer->address(), e.what());
                // C3: Structured replication failure telemetry.
                THEMIS_WARN("{{\"event\":\"replication_failure\",\"replica_id\":\"{}\","
                            "\"reason\":\"{}\",\"retry_count\":{}}}",
                            peer->address(), e.what(), item.attempts);
                failed_peers.push_back(peer);
            } catch (const std::string& e) {
                THEMIS_WARN("[CacheReplicationCoordinator] fanout to peer '{}' failed: {}",
                            peer->address(), e);
                failed_peers.push_back(peer);
            } catch (const char* e) {
                THEMIS_WARN("[CacheReplicationCoordinator] fanout to peer '{}' failed: {}",
                            peer->address(), (e ? e : "<null>"));
                failed_peers.push_back(peer);
            }
        }

        if (!failed_peers.empty()) {
            if (item.attempts + 1 < kMaxRetryAttempts) {
                {
                    std::lock_guard<std::mutex> ml(metrics_mutex_);
                    ++fanout_retried_;
                }
                enqueueFanout(item.asRetry(std::move(failed_peers)));
            } else {
                THEMIS_WARN("[CacheReplicationCoordinator] dropping invalidation for "
                            "key='{}' after {} attempts", item.key, kMaxRetryAttempts);
                // C3: Structured max-retry eviction telemetry.
                THEMIS_WARN("{{\"event\":\"eviction_fanout_drop\",\"eviction_reason\":\"max_retries\","
                            "\"evicted_entries\":1,\"freed_bytes\":0,\"key\":\"{}\"}}",
                            item.key);
                std::lock_guard<std::mutex> ml(metrics_mutex_);
                ++fanout_failed_;
            }
        }
    }
}

} // namespace cache
} // namespace themis
