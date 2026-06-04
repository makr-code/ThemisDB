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

/*
 * ThemisDB | File: raft_wal_integration.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <memory>
#include <string>
#include <condition_variable>
#include <map>
#include <set>
#include <mutex>
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
    struct Config {
        std::string node_id;
        std::shared_ptr<RaftState> raft_state;
        std::shared_ptr<RaftLog> raft_log;
        std::shared_ptr<WALManager> wal_manager;
        std::shared_ptr<WALShipper> wal_shipper;
        std::shared_ptr<WALApplier> wal_applier;
    };
    
    struct WriteResult {
        bool success;
        LSN lsn;
        std::string error_message;
    };
    
    explicit RaftWALIntegration(const Config& config);
    ~RaftWALIntegration();
    
    // Write with quorum (blocks until majority replicated)
    WriteResult write(const WALEntry& entry);
    
    // Linearizable read from leader
    std::optional<WALEntry> read(const LSN& lsn);
    
    // Leadership management
    void onBecomeLeader();
    void onBecomeFollower();
    
    // Log compaction
    void compact(uint64_t snapshot_index);
    
    // Status queries
    bool isLeader() const;
    std::string getLeaderId() const;
    
private:
    Config config_;
    bool is_leader_;
    std::mutex mutex_;
    std::condition_variable cv_;  ///< Notified by onAppendEntriesResponse() when ACKs arrive
    
    // Track pending writes for quorum
    struct PendingWrite {
        uint64_t log_index;
        LSN wal_lsn;
        std::set<std::string> acknowledgments;
        bool committed;
    };
    
    std::map<uint64_t, PendingWrite> pending_writes_;
    
    // Helper methods
    void startWALShipper();
    void stopWALShipper();
    void startWALApplier();
    void stopWALApplier();
    
    bool hasQuorum(const std::set<std::string>& acks) const;
    void onAppendEntriesResponse(const std::string& follower_id, uint64_t match_index);
};

} // namespace sharding
} // namespace themisdb
