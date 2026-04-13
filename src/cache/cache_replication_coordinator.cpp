/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cache_replication_coordinator.cpp                  ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-13 04:24:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     383                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 25f9a09910  2026-04-02  Refactor tests and improve assertions   ║
    • efdbcc2fc8  2026-03-19  merge: resolve conflicts with develop - keep predictive p... ║
    • 0a465da9ae  2026-03-19  fix(cache): address all code review issues for CacheRepli... ║
    • f7f2be3028  2026-03-18  feat(cache): implement network-backed peer discovery for ... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "cache/cache_replication_coordinator.h"
#include "utils/logger.h"
#include <unordered_map>

namespace themis {
namespace cache {

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

    {
        std::lock_guard<std::mutex> lk(mutex_);
        ++messages_sent_;
    }

    if (!bus_) {
        return;  // Standalone: no peers to notify
    }

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

    {
        std::lock_guard<std::mutex> lk(mutex_);
        ++messages_sent_;
    }

    if (!bus_) {
        return;  // Standalone: no peers to notify
    }

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
    uint64_t sent, received;
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
    uint64_t enqueued, dropped, delivered, retried, failed;
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
    std::lock_guard<std::mutex> lk(queue_mutex_);
    if (fanout_queue_.size() >= kRetryQueueCapacity) {
        THEMIS_WARN("[CacheReplicationCoordinator] fanout queue full ({} entries); "
                    "dropping invalidation for key='{}'",
                    kRetryQueueCapacity, item.key);
        std::lock_guard<std::mutex> ml(metrics_mutex_);
        ++fanout_dropped_;
        return;
    }
    {
        std::lock_guard<std::mutex> ml(metrics_mutex_);
        ++fanout_enqueued_;
    }
    fanout_queue_.push(std::move(item));
    queue_cv_.notify_one();
}

void CacheReplicationCoordinator::fanoutWorker() {
    while (true) {
        FanoutItem item;
        {
            std::unique_lock<std::mutex> lk(queue_mutex_);
            queue_cv_.wait(lk, [this] {
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
                failed_peers.push_back(peer);
            } catch (...) {
                THEMIS_WARN("[CacheReplicationCoordinator] fanout to peer '{}' failed "
                            "(unknown exception)", peer->address());
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
                std::lock_guard<std::mutex> ml(metrics_mutex_);
                ++fanout_failed_;
            }
        }
    }
}

} // namespace cache
} // namespace themis
