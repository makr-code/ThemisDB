/**
 * @file raft_wal_integration.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <condition_variable>
#include <map>
#include <set>
#include <mutex>
#include <chrono>
#include "raft_state.h"
#include "raft_log.h"
#include "wal_manager.h"
#include "wal_shipper.h"
#include "wal_applier.h"

namespace themisdb {
namespace sharding {

// Bridge WAL types from themis::sharding into the themisdb::sharding namespace.
using themis::sharding::LSN;
using themis::sharding::WALEntry;
using themis::sharding::WALManager;
using themis::sharding::WALShipper;
using themis::sharding::WALApplier;

/**
 * @brief Integrates Raft consensus with WAL replication for automatic failover
 * 
 * Provides:
 * - Quorum-based writes (blocks until majority acknowledgment)
 * - Automatic failover on leader failure
 * - Strong consistency (linearizable reads/writes)
 * - Leadership management (starts/stops WAL Shipper)
 */
class RaftWALIntegration {
public:
    /**
     * @brief Dependencies required to bridge Raft and WAL subsystems.
     */
    struct Config {
        std::string node_id;                         ///< Local node identifier.
        std::shared_ptr<RaftState> raft_state;       ///< Shared Raft state machine.
        std::shared_ptr<RaftLog> raft_log;           ///< Shared Raft log used for commit tracking.
        std::shared_ptr<WALManager> wal_manager;     ///< Local WAL storage manager.
        std::shared_ptr<WALShipper> wal_shipper;     ///< Leader-side WAL replication shipper.
        std::shared_ptr<WALApplier> wal_applier;     ///< Follower-side WAL applier.
       std::chrono::milliseconds write_timeout{5000}; ///< FIXED: Timeout for write lock acquisition (prevents indefinite blocking)
    };

    /**
     * @brief Result of a quorum-enforced WAL write.
     */
    struct WriteResult {
        bool success = 0;               ///< True when quorum commit succeeded.
        LSN lsn;                    ///< Assigned WAL LSN, even on quorum failure when append succeeded.
        std::string error_message;  ///< Failure reason when success is false.
    };

    /**
     * @brief Construct Raft/WAL integration bridge.
     * @param config Shared subsystem dependencies and node identity.
     */
    explicit RaftWALIntegration(const Config& config);

    /** @brief Stop active shipper/applier role during destruction. */
    ~RaftWALIntegration();

    // Write with quorum (blocks until majority replicated)
    /**
     * @brief Append WAL entry locally and wait for Raft/WAL quorum acknowledgment.
     * @param entry WAL entry payload to append and replicate.
     * @return Write result including assigned LSN and timeout/redirect errors.
     */
    WriteResult write(const WALEntry& entry);

    // Linearizable read from leader
    /**
     * @brief Read WAL entry only when this node is current leader.
     * @param lsn WAL LSN to read.
     * @return WAL entry when present and served by leader, otherwise nullopt.
     */
    std::optional<WALEntry> read(const LSN& lsn);

    // Leadership management
    /** @brief Switch to leader mode and start leader-side WAL shipping. */
    void onBecomeLeader();
    /** @brief Switch to follower mode and start follower-side apply readiness. */
    void onBecomeFollower();

    // Log compaction
    /**
     * @brief Advance compaction-related commit metadata up to snapshot index.
     * @param snapshot_index Highest index captured by snapshot.
     */
    void compact(uint64_t snapshot_index);

    // Status queries
    /** @brief Return whether integration is currently in leader mode. */
    bool isLeader() const;
    /** @brief Return current leader ID from shared Raft state. */
    std::string getLeaderId() const;

private:
    Config config_;                     ///< Shared subsystem dependencies.
    bool is_leader_;                    ///< Cached local leadership mode.
    mutable std::timed_mutex mutex_;                  ///< FIXED: Changed to timed_mutex to enforce write_timeout. Protects leadership and pending write state.
    std::condition_variable_any cv_;  ///< Notified by onAppendEntriesResponse() when ACKs arrive (works with timed_mutex)

    // Track pending writes for quorum
    /** @brief Pending quorum-tracked write keyed by Raft log index. */
    struct PendingWrite {
        uint64_t log_index = 0;                    ///< Raft log index associated with write.
        LSN wal_lsn;                           ///< WAL LSN assigned to entry.
        std::set<std::string> acknowledgments; ///< Node IDs that acknowledged replication.
        bool committed;                        ///< True once quorum was reached.
    };

    std::map<uint64_t, PendingWrite> pending_writes_; ///< Pending writes keyed by Raft log index.

    // Helper methods
    /** @brief Start leader-side WAL shipper if configured. */
    void startWALShipper();
    /** @brief Stop leader-side WAL shipper if configured. */
    void stopWALShipper();
    /** @brief Prepare follower-side WAL applier for use. */
    void startWALApplier();
    /** @brief Stop follower-side WAL applier when role changes. */
    void stopWALApplier();

    /**
     * @brief Check whether acknowledgment set satisfies current cluster quorum.
     * @param acks Node IDs that acknowledged write.
     * @return True when majority quorum is reached.
     */
    bool hasQuorum(const std::set<std::string>& acks) const;

    /**
     * @brief Record follower replication progress and notify waiting writers.
     * @param follower_id Follower node that responded.
     * @param match_index Highest replicated log index on that follower.
     */
    void onAppendEntriesResponse(const std::string& follower_id, uint64_t match_index);
};

} // namespace sharding
} // namespace themisdb
