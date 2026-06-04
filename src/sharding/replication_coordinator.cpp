/**
 * @file replication_coordinator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: replication_coordinator.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 187
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0
 * PR History (last 5): #1031 Implement comprehensive res... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "sharding/replication_coordinator.h"
#include "utils/logger.h"
#include <algorithm>

namespace themis::sharding {

ReplicationCoordinator::ReplicationCoordinator(std::shared_ptr<WALShipper> shipper)
    : shipper_(std::move(shipper)) {
    if (!shipper_) {
        THEMIS_WARN("ReplicationCoordinator created without WALShipper; replication waits will be no-ops");
    }
}

ReplicationCoordinator::~ReplicationCoordinator() {
    // Notify any waiting threads
    pending_cv_.notify_all();
}

ReplicationCoordinator::ReplicationResult ReplicationCoordinator::waitForReplication(
    const LSN& entry_lsn,
    const WriteConcernConfig& concern
) {
    ReplicationResult result;
    result.success = false;

    if (!enabled_ || !shipper_) {
        // Coordinator disabled or no shipper; treat as ONE (local only)
        result.success = true;
        result.replicas_acknowledged = 1; // Primary
        result.replicas_required = 1;
        return result;
    }

    size_t total_replicas = getReplicaCount() + 1; // +1 for primary
    size_t required = calculateRequiredReplicas(concern.level, total_replicas);
    result.replicas_required = required;

    // If only primary required (ONE), return immediately
    if (concern.level == WriteConcern::ONE) {
        result.success = true;
        result.replicas_acknowledged = 1;
        return result;
    }

    auto start = std::chrono::steady_clock::now();
    std::string lsn_key = entry_lsn.toString();

    // Register pending write
    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        pending_writes_.try_emplace(lsn_key, entry_lsn, concern, 1);
    }

    bool met_concern = false;
    {
        // Wait for acknowledgments with timeout
        std::unique_lock<std::mutex> lock(pending_mutex_);
        met_concern = pending_cv_.wait_for(
            lock,
            concern.timeout,
            [this, &lsn_key, required, total_replicas]() {
                auto it = pending_writes_.find(lsn_key);
                if (it == pending_writes_.end()) return false;
                return hasMetConcern(it->second, total_replicas);
            }
        );

        auto end = std::chrono::steady_clock::now();
        result.latency = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        // Check final status
        auto it = pending_writes_.find(lsn_key);
        if (it != pending_writes_.end()) {
            result.replicas_acknowledged = it->second.ack_count.load(std::memory_order_acquire);
            it->second.completed.store(true, std::memory_order_release);
        }

        // Cleanup this entry
        pending_writes_.erase(lsn_key);
    }

    if (met_concern) {
        result.success = true;
        THEMIS_DEBUG("Write concern {} met for LSN {} ({}/{} replicas, {}ms)",
                     toString(concern.level), lsn_key,
                     result.replicas_acknowledged, required, result.latency.count());
    } else {
        result.error_message = "Write concern timeout: only " +
                      std::to_string(result.replicas_acknowledged) + "/" +
                      std::to_string(required) + " replicas acknowledged within " +
                      std::to_string(concern.timeout.count()) + "ms";
        THEMIS_WARN("{}", result.error_message);
    }

    // Periodic cleanup of old entries
    cleanupPendingWrites();

    return result;
}

void ReplicationCoordinator::recordAcknowledgment(const std::string& replica_id, const LSN& lsn) {
    // Fail-closed: reject empty replica_id immediately
    if (replica_id.empty()) {
        spdlog::error("ReplicationCoordinator::recordAcknowledgment: replica_id is empty");
        return;
    }

    if (!enabled_) return;

    std::string lsn_key = lsn.toString();
    bool notify = false;

    {
        std::lock_guard<std::mutex> lock(pending_mutex_);
        auto it = pending_writes_.find(lsn_key);
        if (it != pending_writes_.end() &&
            !it->second.completed.load(std::memory_order_acquire)) {
            it->second.ack_count.fetch_add(1, std::memory_order_release);
            notify = true;
            THEMIS_DEBUG("Replica {} acknowledged LSN {}", replica_id, lsn_key);
        }
    }

    if (notify) {
        pending_cv_.notify_all();
    }
}

size_t ReplicationCoordinator::getReplicaCount() const {
    if (!shipper_) return 0;
    return shipper_->getReplicaInfo().size();
}

void ReplicationCoordinator::setEnabled(bool enabled) {
    enabled_.store(enabled, std::memory_order_release);
    if (!enabled) {
        // Wake up any waiting threads
        pending_cv_.notify_all();
    }
}

bool ReplicationCoordinator::isEnabled() const {
    return enabled_.load(std::memory_order_acquire);
}

std::vector<ReplicaInfo> ReplicationCoordinator::getReplicaInfo() const {
    if (shipper_) {
        return shipper_->getReplicaInfo();
    }
    return {};
}

WALShipperStats ReplicationCoordinator::getShipperStats() const {
    if (shipper_) {
        return shipper_->getStatistics();
    }
    return {};
}

bool ReplicationCoordinator::hasMetConcern(const PendingWrite& write, size_t total_replicas) const {
    size_t required = calculateRequiredReplicas(write.concern.level, total_replicas);
    size_t current_acks = write.ack_count.load(std::memory_order_acquire);
    return current_acks >= required;
}

void ReplicationCoordinator::cleanupPendingWrites() {
    auto now = std::chrono::steady_clock::now();
    std::vector<std::string> to_remove;
    std::lock_guard<std::mutex> lock(pending_mutex_);
    for (const auto& [lsn, pending] : pending_writes_) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - pending.start_time
        );
        // Remove entries older than 60 seconds or already completed
        if (elapsed.count() > 60000 ||
            pending.completed.load(std::memory_order_acquire)) {
            to_remove.push_back(lsn);
        }
    }
    for (const auto& lsn : to_remove) {
        pending_writes_.erase(lsn);
    }
}

} // namespace themis::sharding
