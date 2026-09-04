/**
 * @file replication_coordinator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "sharding/write_concern.h"
#include "sharding/wal_shipper.h"
#include <memory>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace themisdb {
namespace sharding {

} // namespace sharding
} // namespace themisdb

namespace themis {
namespace sharding {

/**
 * @brief Coordinates replica acknowledgments for write-concern enforcement.
 *
 * The coordinator tracks pending writes by LSN and blocks callers until the
 * requested write concern is satisfied or times out.
 */
class ReplicationCoordinator {
public:
    /**
     * @brief Result of waitForReplication.
     */
    struct ReplicationResult {
        bool success = false;                       // Whether write succeeded
        size_t replicas_acknowledged = 0;          // Number of replicas that acknowledged
        size_t replicas_required = 0;              // Number of replicas required by write concern
        std::chrono::milliseconds latency{0};      // Time to acquire required acknowledgments
        std::string error_message;                 // Error details if failed
    };

    /**
     * @brief Construct coordinator.
     * @param shipper WAL shipper used for replica topology and replication signals.
     */
    explicit ReplicationCoordinator(std::shared_ptr<WALShipper> shipper);

    /** @brief Destructor; wakes waiting threads during shutdown. */
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
     * @param replica_id Replica that acknowledged (non-empty required)
     * @param lsn LSN that was acknowledged
     * @note Rejects acknowledgments with empty replica_id (fail-closed guard)
     */
    void recordAcknowledgment(const std::string& replica_id, const LSN& lsn);

    /**
     * @brief Return current replica count excluding primary.
     * @return Number of replicas known by WAL shipper.
     */
    size_t getReplicaCount() const;

    /**
     * Get current replica topology info (delegates to WALShipper)
     * Returns an empty vector if no shipper is configured.
     */
    std::vector<ReplicaInfo> getReplicaInfo() const;

    /**
     * @brief Return current WAL shipper statistics.
     * @return WAL shipper stats snapshot, or default-initialized stats when unavailable.
     */
    WALShipperStats getShipperStats() const;

    /**
     * @brief Enable or disable write-concern waiting.
     * @param enabled New enabled state.
     */
    void setEnabled(bool enabled);

    /** @brief Check whether coordinator is enabled. */
    bool isEnabled() const;

private:
    std::shared_ptr<WALShipper> shipper_;
    std::atomic<bool> enabled_{true};

    // ======================================================================
    // DEADLOCK PREVENTION: Canonical Lock Acquisition Order
    // ======================================================================
    // To prevent circular wait deadlocks in the ReplicationCoordinator,
    // the single lock (pending_mutex_) must follow these CRITICAL rules:
    //   1. NEVER hold pending_mutex_ across WAL shipper RPC calls
    //   2. ALWAYS release pending_mutex_ before calling shipper_ methods
    //   3. Use try-catch around all connection cleanup code
    //   4. Always check db_connection validity before cleanup
    //   5. Use shared_ptr (RAII) for all owned resources
    // ======================================================================
    // Track pending writes waiting for acknowledgment
    struct PendingWrite {
        LSN lsn;
        WriteConcernConfig concern;
        std::atomic<size_t> ack_count{0};
        std::chrono::steady_clock::time_point start_time;
        std::atomic<bool> completed{false};
        std::shared_ptr<void> db_connection;  // RAII: automatic cleanup on destruction

        PendingWrite() = default;
        
        // Parametrisierter Konstruktor für direkte Initialisierung
        PendingWrite(const LSN& l, const WriteConcernConfig& c, size_t ack = 1)
            : lsn(l), concern(c), ack_count(ack), 
              start_time(std::chrono::steady_clock::now()), completed(false), db_connection(nullptr) {}
        
        // Custom copy/move to support storage inside associative containers despite atomic member
        PendingWrite(const PendingWrite& other)
            : lsn(other.lsn), concern(other.concern),
              ack_count(other.ack_count.load()),
              start_time(other.start_time), completed(other.completed.load()) {}

        PendingWrite& operator=(const PendingWrite& other) {
            if (this == &other) {
              return *this;
            }
            lsn = other.lsn;
            concern = other.concern;
            ack_count.store(other.ack_count.load());
            start_time = other.start_time;
            completed.store(other.completed.load());
            return *this;
        }

        PendingWrite(PendingWrite&& other) noexcept
            : lsn(std::move(other.lsn)), concern(std::move(other.concern)),
              ack_count(other.ack_count.load()),
              start_time(other.start_time), completed(other.completed.load()) {}

        PendingWrite& operator=(PendingWrite&& other) noexcept {
            if (this == &other) {
              return *this;
            }
            lsn = std::move(other.lsn);
            concern = std::move(other.concern);
            ack_count.store(other.ack_count.load());
            start_time = other.start_time;
            completed.store(other.completed.load());
            return *this;
        }
    };

    mutable std::mutex pending_mutex_;
    std::unordered_map<std::string, PendingWrite> pending_writes_; // LSN string -> PendingWrite
    std::condition_variable pending_cv_;

    /**
     * @brief Check whether current acknowledgment count satisfies write concern.
     * @param write Pending write state.
     * @param total_replicas Total participants including primary.
     * @return true when required acknowledgments are reached.
     */
    bool hasMetConcern(const PendingWrite& write, size_t total_replicas) const;

    /** @brief Remove stale or completed pending-write records. */
    void cleanupPendingWrites();
};

} // namespace sharding
} // namespace themis

namespace themisdb {
namespace sharding {
using ReplicationCoordinator = themis::sharding::ReplicationCoordinator;
} // namespace sharding
} // namespace themisdb
