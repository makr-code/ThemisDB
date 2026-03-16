/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cache_replication_coordinator.cpp                  ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-03-16 04:13:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     163                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 0d58fbec9  2026-02-24  feat(cache): Add cache replication for high-availability ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "cache/cache_replication_coordinator.h"
#include "utils/logger.h"

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

} // namespace cache
} // namespace themis
