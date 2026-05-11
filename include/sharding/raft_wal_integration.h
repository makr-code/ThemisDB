/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            raft_wal_integration.h                             ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:47:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     119                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e963d4e9ba  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
    • 71d99c4f28  2026-04-14  fix(concurrency): eliminate deadlocks, blocking I/O under... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <memory>
#include <string>
#include <condition_variable>
#include &lt;map&gt;
#include &lt;set&gt;
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
