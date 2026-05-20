/*
 * ThemisDB | File: replication_coordinator.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "sharding/write_concern.h"
#include "sharding/wal_shipper.h"
#include <memory>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <unordered_map>

namespace themis::sharding {

/**
 * Replication Coordinator
 * 
 * Manages write concern enforcement and tracks replica acknowledgments.
 * Used by write API to ensure writes meet quorum requirements.
 */
class ReplicationCoordinator {
public:
    /**
     * Result of a replication wait operation
     */
    struct ReplicationResult {
        bool success = false;                       // Whether write succeeded
        size_t replicas_acknowledged = 0;          // Number of replicas that acknowledged
        size_t replicas_required = 0;              // Number of replicas required by write concern
        std::chrono::milliseconds latency{0};      // Time to acquire required acknowledgments
        std::string error_message;                 // Error details if failed
    };

    explicit ReplicationCoordinator(std::shared_ptr<WALShipper> shipper);
    ~ReplicationCoordinator();

    /**
     * Wait for replicas to acknowledge a write.
     * @param entry_lsn LSN of the written entry.
     * @param concern Write concern level.
     * @return ReplicationResult with success status and replica count.
     */
    ReplicationResult waitForReplication(
        const LSN& entry_lsn,
        const WriteConcernConfig& concern
    );

    /**
     * Record replica acknowledgment (called by shipper or apply endpoint)
     * @param replica_id Replica that acknowledged
     * @param lsn LSN that was acknowledged
     */
    void recordAcknowledgment(const std::string& replica_id, const LSN& lsn);

    /**
     * Get current replica count (for quorum calculation)
     */
    size_t getReplicaCount() const;

    /**
     * Get current replica topology info (delegates to WALShipper)
     * Returns an empty vector if no shipper is configured.
     */
    std::vector<ReplicaInfo> getReplicaInfo() const;

    /**
     * Get WAL shipper statistics (delegates to WALShipper)
     */
    WALShipperStats getShipperStats() const;

    /**
     * Enable/disable coordinator (useful for testing)
     */
    void setEnabled(bool enabled);
    bool isEnabled() const;

private:
    std::shared_ptr<WALShipper> shipper_;
    std::atomic<bool> enabled_{true};

    // Track pending writes waiting for acknowledgment
    struct PendingWrite {
        LSN lsn;
        WriteConcernConfig concern;
        std::atomic<size_t> ack_count{0};
        std::chrono::steady_clock::time_point start_time;
        bool completed{false};

        PendingWrite() = default;
        
        // Parametrisierter Konstruktor für direkte Initialisierung
        PendingWrite(const LSN& l, const WriteConcernConfig& c, size_t ack = 1)
            : lsn(l), concern(c), ack_count(ack), 
              start_time(std::chrono::steady_clock::now()), completed(false) {}
        
        // Custom copy/move to support storage inside associative containers despite atomic member
        PendingWrite(const PendingWrite& other)
            : lsn(other.lsn), concern(other.concern),
              ack_count(other.ack_count.load()),
              start_time(other.start_time), completed(other.completed) {}

        PendingWrite& operator=(const PendingWrite& other) {
            if (this == &other) return *this;
            lsn = other.lsn;
            concern = other.concern;
            ack_count.store(other.ack_count.load());
            start_time = other.start_time;
            completed = other.completed;
            return *this;
        }

        PendingWrite(PendingWrite&& other) noexcept
            : lsn(std::move(other.lsn)), concern(std::move(other.concern)),
              ack_count(other.ack_count.load()),
              start_time(other.start_time), completed(other.completed) {}

        PendingWrite& operator=(PendingWrite&& other) noexcept {
            if (this == &other) return *this;
            lsn = std::move(other.lsn);
            concern = std::move(other.concern);
            ack_count.store(other.ack_count.load());
            start_time = other.start_time;
            completed = other.completed;
            return *this;
        }
    };

    mutable std::mutex pending_mutex_;
    std::unordered_map<std::string, PendingWrite> pending_writes_; // LSN string -> PendingWrite
    std::condition_variable pending_cv_;

    /**
     * Check if write has met its concern requirement
     */
    bool hasMetConcern(const PendingWrite& write, size_t total_replicas) const;

    /**
     * Cleanup old pending writes (timed out or completed)
     */
    void cleanupPendingWrites();
};

} // namespace themis::sharding
